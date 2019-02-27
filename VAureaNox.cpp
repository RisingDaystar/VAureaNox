/*
VAureaNox : Distance Fields Pathtracer
Copyright (C) 2017-2018  Alessandro Berti

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "VVolumes.h"
#include "VOperators.h"

#include "scenes\VSC_Spider.h"
#include "scenes\VSC_Pathtracing.h"
#include "scenes\VSC_Cornell.h"
#include "scenes\VSC_Gi_test.h"
#include "scenes\VSC_Nested.h"

#include "renderers\VRE_Pathtracer.h"

#define MONITOR_THREAD_SUPPORT

#ifdef MONITOR_THREAD_SUPPORT
#include <conio.h>
#endif

using namespace ygl;
using namespace vnx;

//TODO, interfaccia file unificata
struct VFileConfigs{
    std::map<std::string,std::map<std::string,std::string>> mappings;

    void eval(const std::string& line){
        std::string value = "";
        std::string name = "";
        std::string section = "";
        int state = 0;
        for(int i=0;i<line.size();i++){
            if(line[i]==':' && state==0){state = 1;continue;}
            if(line[i]==':' && state!=0){break;}
            if(line[i] == ';') break;
            if(line[i] == ' ' || line[i]=='\n' || line[i]=='\r'){continue;}
            if(line[i]=='#' && state==0){state = -1;continue;}

            if(state==-1){section+=line[i];}
            else if(state==0){name+=line[i];}
            else if(state==1){value+=line[i];}
        }

        if(mappings.find(section)!=mappings.end()) return; // sezione duplicata

        if(!section.size() || !name.size() || !value.size()) throw new VException("Badly formatted config file.");
        mappings[section][name] = value;
    }
};


namespace vnx {

	void shut(VScene& scn, VRenderer* renderer) {
		scn.shut();
		if (renderer != nullptr) { delete renderer; renderer = nullptr; }
	}

	void init(VScene& scn, VRenderer** renderer, image3f& img, const VConfigurable& config) {

		auto renderer_type = config.try_get("renderer_type", "");
		auto render_scene = config.try_get("render_scene", "spider");

		if (stricmp(render_scene,"cornell")) { vscndef::init_cornell_scene(scn); }
		else if (stricmp(render_scene,"spider")) { vscndef::init_spider_scene(scn); }
		else if (stricmp(render_scene,"pathtracing")) { vscndef::init_pathtracing_scene(scn); }
		else if (stricmp(render_scene,"gi_test")) { vscndef::init_gi_test_scene(scn); }
		else if (stricmp(render_scene,"nested")){vscndef::init_nested_scene(scn);}
		else{ throw VException("Invalid Scene."); }

		if (stricmp(renderer_type,"pathtracer")) {
			*renderer = new vnx::VRE_Pathtracer("configs/VRE_Pathtracer.ini");
		}
		if (renderer == nullptr) { throw VException("Invalid Renderer."); }
        (*renderer)->init();

		int w = config.try_get("i_image_width", 648);
		int h = config.try_get("i_image_height", 480);
		if (w <= 0 || h <= 0) { throw VException("Invalid image dimensions."); }
		init_image(img,{w, h});
		scn.camera.Setup(vec2f{float(w),float(h)});
	}

	void task(int id, const VScene& scn, VRenderer* renderer, image3f& img, std::mutex& mtx, ygl::rng_state& rng, volatile std::atomic<int>& lrow, volatile std::atomic<int>& rowCounter) {
		const int width = img.size.x;
		const int height = img.size.y;


		while (!renderer->status.bStopped) {
			//sincronizzazione per evitare data race...insignificante overhead
			mtx.lock();
			int j = lrow;
			lrow++;
			if (j >= img.size.y) {
				mtx.unlock(); break;
			}
			mtx.unlock();

			renderer->eval_image(scn, rng, img, width, height, j);

			mtx.lock();
			printf("Current Row:{%u} -> Total:{%u / %u} -> Worker:{%d}\n", j, ++rowCounter, height, id);
			mtx.unlock();

		}
		mtx.lock();
		printf("\t#Worker %d finished\n", id);
		mtx.unlock();
	}


    //Utilizza "conio.h" , non standard, disattivabile in compilazione.
	void monitor_task(std::mutex& mtx, const VScene& scn,VRenderer* renderer,const image3f& img){
        #ifdef MONITOR_THREAD_SUPPORT
        while(!renderer->status.bStopped && !renderer->status.bFinished){
            if(!kbhit()){std::this_thread::sleep_for(std::chrono::milliseconds(1000));continue;} //1s sleep, per evitare massiccio overhead
            auto k = getch();

            if(k == 27){ //ESC
                mtx.lock();
                    std::cout<<"\n***Termination Requested***\n\n";
                    renderer->status.bStopped = true;
                mtx.unlock();
            }else if(k == '1'){
                mtx.lock();
                    auto fname = "VAureaNox_"+scn.id+"_"+std::to_string(img.size.x) + "x" + std::to_string(img.size.y) + ("_"+renderer->type())+renderer->img_affix(scn)+"_PREVIEW";
                    printf("\n*Saving Preview image \"%s\" \n", fname.c_str());
                    vnx::save_ppm(fname, img);
                    std::cout<<"*Preview Saved\n\n";
                mtx.unlock();
            }else if(k=='2'){
                mtx.lock();
                    renderer->status.bDebugMode = !renderer->status.bDebugMode;
                    if(renderer->status.bDebugMode) std::cout<<"\n*Debug enabled {TODO}\n\n";
                    else std::cout<<"\n*Debug disabled {TODO}\n\n";
                mtx.unlock();
            }
        }
        #endif
	}

};





int main() {
	std::mutex mtx;
	std::atomic<int> lastRow;
	lastRow.store(0);
	std::atomic<int> rowCounter;
	rowCounter.store(0);
	VConfigurable config("configs/VAureaNox.ini");
	VScene scn;
	VRenderer* renderer = nullptr;
	image3f img;
	bool b_start_monitor = false;
	VStatus status;

	try {
		printf("<--- VAureaNox - Distance Fields Renderer - v: %s --->\n", vnx::version);
		config.parse();
		init(scn, &renderer, img, config);

		//Controlli hardcoded, ridondanti per avere un grado extra di sicurezza visto che vengono uitilizzati a prescindere dal motore di rendering
		auto n_em_evals = renderer->try_get("n_em_evals", 0);
		if (n_em_evals < 0) { throw VException("n_em_evals < 0"); }

		b_start_monitor = config.try_get("b_start_monitor",false);
		//
        if(config.try_get("b_verbose_parsing",false)){
            config.print();
            renderer->print();
        }

        printf("\n**Preparing scene : \"%s\"\n\n",scn.id.c_str());
        printf("**Transforming nodes coordinates... **\n");
		apply_transforms(scn.root);
		printf("**Calculating emissive hints... **\n");
		populate_emissive_hints(scn, n_em_evals, renderer->try_get("f_ray_tmin",0.001f), renderer->try_get("f_normal_eps",0.0001f),config.try_get("b_verbose_precalc",false));
        int n_emiss = 0;
        int n_lights = scn.emissive_hints.size();
        for (auto ep : scn.emissive_hints) {n_emiss += ep.size();}
		printf("**Calculated : \"%d\" lights -> \"%d\" emissive hints **\n\n",n_lights,n_emiss);
        printf("**Selected renderer : \"%s\" **\n",renderer->type().c_str());
		renderer->post_init(scn);
	}
	catch (std::exception& ex) {
		shut(scn, renderer);
		printf("**Fatal Exception: %s \nPress any key to quit...",ex.what());
		std::cin.get();
		return -1;
	}




	int n_max_threads = config.try_get("n_max_threads", 0);
	int nThreads = std::thread::hardware_concurrency();
	if (n_max_threads>0) nThreads = std::min(nThreads, n_max_threads);

	auto t_start = std::chrono::steady_clock::now();

	std::thread* monitor = nullptr;

	printf("**Rendering \"%s\" using \"%d\" threads **\n", scn.id.c_str(),  std::max(1,nThreads));
	if(b_start_monitor){
        monitor = new std::thread(monitor_task, std::ref(mtx), std::ref(scn), renderer, std::ref(img));
        printf("**Monitor Thread started\n");
	}
	printf("\n");

	if (nThreads > 1) {
        ygl::rng_state rng[nThreads];
        for(int n=0;n<nThreads;n++){
            rng[n] = ygl::make_rng(ygl::get_time()+n);
        }

		std::vector<std::thread*> worker(nThreads);
		for (int i = 0; i < nThreads; i++) {
            worker[i] = new std::thread(task, i, std::ref(scn), renderer, std::ref(img), std::ref(mtx), std::ref(rng[i]), std::ref(lastRow), std::ref(rowCounter));
        }
		for (int i = 0; i < nThreads; i++) { if (worker[i] && worker[i]->joinable()) { worker[i]->join(); }}
		for (int i = 0; i < nThreads; i++) { delete worker[i]; worker[i] = nullptr;}
        worker.clear();
	}
	else {
        ygl::rng_state rng = ygl::make_rng(ygl::get_time());
		task(0,scn, renderer, img, mtx, rng, lastRow, rowCounter);
	}
	renderer->status.bFinished = true;

    if(monitor){
        if (monitor->joinable()) monitor->join();
        delete monitor;
        monitor = nullptr;
    }

	auto t_end = std::chrono::steady_clock::now();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();

	if(!renderer->status.bStopped) printf("\n*Rendering completed in : ");
	else printf("\n*Rendering stopped after : ");
	print_hhmmss(seconds);

	auto fname = "VAureaNox_"+scn.id+"_"+std::to_string(img.size.x) + "x" + std::to_string(img.size.y) + ("_"+renderer->type())+renderer->img_affix(scn);
	if(renderer->status.bStopped) fname+="_UNCOMPLETED";

	printf("\n*Saving image \"%s\" \n", fname.c_str());
	vnx::save_ppm(fname, img);
	printf("*Image saved\n");

	shut(scn, renderer);
	printf("Press any key to quit...");
	std::cin.get();
	return 0;
}
