/*
VAureaNox : Distance Fields Pathtracer
Copyright (C) 2017-2019  Alessandro Berti

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

#include "VSceneParser.hpp"

#include "scenes\VSC_Spider.hpp"
#include "scenes\VSC_Pathtracing.hpp"
#include "scenes\VSC_Cornell.hpp"
#include "scenes\VSC_Gi_test.hpp"
#include "scenes\VSC_Nested.hpp"

//#include "renderers\VRE_Pathtracer.h"
#include "renderers\VRE_Experimental_PT.hpp"

#define MONITOR_THREAD_SUPPORT

#ifdef MONITOR_THREAD_SUPPORT
#include <conio.h>
#endif

using namespace ygl;
using namespace vnx;

namespace vnx {

    frame3d calculate_frame(const VNode* node,const frame3d& parent){
        if (node == nullptr) { return identity_frame3d; }
        auto fr = parent*ygl::translation_frame(node->mTranslation);

        switch(node->mRotationOrder){
        case VRO_XYZ:
            fr=fr*ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x)*
                ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y)*
                ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z);
            break;
        case VRO_XZY:
            fr=fr*ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x)*
                ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z)*
                ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y);
            break;
        case VRO_YXZ:
            fr=fr*ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y)*
                ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x)*
                ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z);
            break;
        case VRO_YZX:
            fr=fr*ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y)*
                ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z)*
                ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x);
            break;
        case VRO_ZXY:
            fr=fr*ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z)*
                ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x)*
                ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y);
            break;
        case VRO_ZYX:
            fr=fr*ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z)*
                ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y)*
                ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x);
            break;
        }
        auto n_scale = !cmpf(node->mScale,zero3d) ? one3d/node->mScale : one3d;
        return fr*scaling_frame(n_scale);
	}

	void apply_transforms(VNode* node, const frame3d& parent) {
		node->mFrame = calculate_frame(node,parent);
		auto chs = node->get_childs();
		if (chs.empty()) { return; }
		for (auto& child : chs) { apply_transforms(child, node->mFrame); }
	}



	void shut(VScene& scn, VRenderer* renderer) {
		scn.shut();
		if (renderer != nullptr) { delete renderer; renderer = nullptr; }
	}

	void init(VScene& scn, VRenderer** renderer, image3d& img,VConfigurable& config) {

		auto renderer_type = config.TryGet("renderer_type", "");
		auto scn_to_render = config.TryGet("render_scene", "cornell");


		if(scn_to_render.substr(scn_to_render.find_last_of(".") + 1) == "vnxs"){
            printf("**Loading external Scene : \"%s\"...\n",scn_to_render.c_str());
            VSceneParser sparser;
            scn_to_render = "scene_files/"+scn_to_render;
            sparser.parse(scn,scn_to_render);
		}else{
		    printf("**Loading internal Scene : \"%s\"...\n",scn_to_render.c_str());
            if (stricmp(scn_to_render,std::string("cornell"))) { vscndef::init_cornell_scene(scn); }
            else if (stricmp(scn_to_render,std::string("spider"))) { vscndef::init_spider_scene(scn); }
            else if (stricmp(scn_to_render,std::string("pathtracing"))) { vscndef::init_pathtracing_scene(scn); }
            else if (stricmp(scn_to_render,std::string("gi_test"))) { vscndef::init_gi_test_scene(scn); }
            else if (stricmp(scn_to_render,std::string("nested"))){vscndef::init_nested_scene(scn);}
            else{ throw VException("Invalid Scene."); }
		}
        std::cout<<"**Loaded Scene, id =  \""<<scn.id<<"\"\n";

        printf("**Loading Renderer...\n");
		/*if (stricmp(renderer_type,std::string("pathtracer"))) {
			*renderer = new vnx::VRE_Pathtracer(config,"configs/VRE_Pathtracer.ini");
		}else */if (stricmp(renderer_type,std::string("experimental_pt"))) {
			*renderer = new vnx::VRE_Experimental_PT(config,"configs/VRE_Experimental_PT.ini");
		}

		if (*renderer == nullptr) { throw VException("Invalid Renderer."); }
        (*renderer)->Init();
        std::cout<<"**Loaded Renderer : \""<<(*renderer)->Type()<<"\"\n";

		int w = config.TryGet("i_image_width", 648);
		int h = config.TryGet("i_image_height", 480);
		if (w <= 0 || h <= 0) { throw VException("Invalid image dimensions."); }
		init_image(img,{w, h});
		std::cout<<"**Output resolution:  \""<<img.size.x<<"\" x \""<<img.size.y<<"\"\n";
		//scn.camera.Setup(scn,vec2f{float(w),float(h)});
	}

	void task(int id, const VScene& scn, VRenderer* renderer, image3d& img, std::mutex& mtx, rng_state& rng, volatile std::atomic<int>& lrow, volatile std::atomic<int>& rowCounter) {
		const int width = img.size.x;
		const int height = img.size.y;


		while (!renderer->mStatus.bStopped) {
			//sincronizzazione per evitare data race...insignificante overhead
			mtx.lock();
			int j = lrow;
			lrow++;
			if (j >= img.size.y) {
				mtx.unlock(); break;
			}
			mtx.unlock();

			renderer->EvalImageRow(scn, rng, img, width, height, j);

			mtx.lock();
			std::cout<<"Current Row:"<<j<<" -> Total:{"<<(++rowCounter)<<" / "<<height<<"} -> Worker:{"<<id<<"}\n";
			//printf("Current Row:{%u} -> Total:{%u / %u} -> Worker:{%d}\n", j, ++rowCounter, height, id);
			mtx.unlock();

		}
		mtx.lock();
		//printf("\t#Worker %d finished\n", id);
		std::cout<<"\t#Worker "<<id<<" finished\n";
		mtx.unlock();
	}


    //Utilizza "conio.h" , non standard, disattivabile in compilazione.
	void monitor_task(std::mutex& mtx, const VScene& scn,VRenderer* renderer,const image3d& img, int i_output_depth){
        #ifdef MONITOR_THREAD_SUPPORT
        while(!renderer->mStatus.bStopped && !renderer->mStatus.bFinished){
            if(!kbhit()){std::this_thread::sleep_for(std::chrono::milliseconds(1000));continue;} //1s sleep, per evitare massiccio overhead
            auto k = getch();

            if(k == 27){ //EXIT
                if(!renderer->mStatus.bPauseMode)mtx.lock();
                    printf("\n***Termination Requested***\n\n");
                    renderer->mStatus.bStopped = true;
                if(!renderer->mStatus.bPauseMode)mtx.unlock();
            }else if(k == '1'){ //PREVIEW
                if(!renderer->mStatus.bPauseMode)mtx.lock();
                    auto fname = "VAureaNox_"+scn.id+"_"+std::to_string(img.size.x) + "x" + std::to_string(img.size.y) + ("_"+renderer->Type())+renderer->ImgAffix(scn)+"_PREVIEW";
                    printf("\n*Saving Preview image \"%s\"  @ \"%d\" color depth... ", fname.c_str(),i_output_depth);
                    if(vnx::save_ppm(fname, img, i_output_depth)) printf("OK\n\n");
                    else printf("FAIL\n\n");
                if(!renderer->mStatus.bPauseMode)mtx.unlock();
            }else if(k=='2'){ //DEBUG
                if(!renderer->mStatus.bPauseMode)mtx.lock();
                    renderer->mStatus.bDebugMode = !renderer->mStatus.bDebugMode;
                    if(renderer->mStatus.bDebugMode) printf("\n*Debug flag set\n\n");
                    else printf("\n*Debug flag unset\n\n");
                if(!renderer->mStatus.bPauseMode)mtx.unlock();
            }else if(k=='3'){ //PAUSE
                renderer->mStatus.bPauseMode = !renderer->mStatus.bPauseMode;
                if(renderer->mStatus.bPauseMode) {printf("\n*Rendering Paused\n\n");mtx.lock();}
                else {printf("\n*Rendering Resumed\n\n");mtx.unlock();}
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
	image3d img;
	bool b_start_monitor = false;

	int i_output_depth = 255;

	try {
		printf("<--- VAureaNox - Distance Fields Renderer - v: 0.0.8 --->\n\n");
		config.parse();

        int i_em_evals = config.TryGet("i_em_evals",10000);
        if (i_em_evals <= 0) { throw VException("i_em_evals <= 0"); }

        int i_max_march_iterations = config.TryGet("i_max_march_iterations",512);
        if (i_max_march_iterations <= 0) { throw VException("i_max_march_iterations <= 0"); }

        double f_ray_tmin = config.TryGet("f_ray_tmin",0.0001);
        if (f_ray_tmin < 0.0) { throw VException("f_ray_tmin < 0.0"); }

        double f_ray_tmax = config.TryGet("f_ray_tmax",1000.0);
        if (f_ray_tmax < 0.0 || f_ray_tmax<f_ray_tmin) { throw VException("f_ray_tmax < 0.0 || < f_ray_tmin"); }

        double f_normal_eps = config.TryGet("f_normal_eps",0.001);
        if (f_normal_eps < 0.0) { throw VException("f_normal_eps < 0.0"); }

        std::string i_st_algo = config.TryGet("i_st_algo","naive");
        std::string i_normals_algo = config.TryGet("i_normals_algo","tht");
        std::string i_eh_precalc_algo = config.TryGet("i_eh_precalc_algo","progressive");

        i_output_depth = config.TryGet("i_output_depth",255);
        if(i_output_depth<2 || i_output_depth>65535){ throw VException("i_output_depth must be in range 2 - 65535 inclusive.");}

		init(scn, &renderer, img, config);

		if(stricmp(i_st_algo,std::string("relaxed"))) scn.intersect_algo = &VScene::intersect_rel;
		else if(stricmp(i_st_algo,std::string("enhanced"))) scn.intersect_algo = &VScene::intersect_enh;
		else scn.intersect_algo = &VScene::intersect_naive;

		if(stricmp(i_normals_algo,std::string("tht"))) scn.normals_algo = &VScene::eval_normals_tht;
		else if(stricmp(i_normals_algo,std::string("cnt"))) scn.normals_algo = &VScene::eval_normals_cnt;
		else scn.normals_algo = &VScene::eval_normals_tht;

		if(stricmp(i_eh_precalc_algo,std::string("full"))) scn.eh_precalc_algo = &VScene::precalc_emissive_hints_full;
		else if(stricmp(i_eh_precalc_algo,std::string("strict"))) scn.eh_precalc_algo = &VScene::precalc_emissive_hints_strict;
		else scn.eh_precalc_algo = &VScene::precalc_emissive_hints_full;

		b_start_monitor = config.TryGet("b_start_monitor",false);
		//
        if(config.TryGet("b_verbose_parsing",false)){
            config.print();
            renderer->print();
        }

        printf("\n**Preparing scene...\n");
        printf("**Transforming nodes coordinates...\n");
		apply_transforms(scn.root,identity_frame3d);
		printf("**Setting up camera...\n");
        scn.camera.Setup(vec2f{img.size.x,img.size.y});
        scn.camera.EvalAutoFocus(scn,f_ray_tmin,f_ray_tmax,i_max_march_iterations);

		printf("\n**Calculating emissive hints...\n");
		auto t_start = std::chrono::steady_clock::now();
		auto lc_stats = scn.populate_emissive_hints(i_em_evals,i_max_march_iterations,f_ray_tmin,f_ray_tmax,f_normal_eps,config.TryGet("b_verbose_precalc",false));
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - t_start).count();
        int n_lights = scn.emissive_hints.size();

        const auto mb_alloc = (sizeof(VResult)*lc_stats.x)/(1024*1024);
		printf("**Calculated : \"%d\" lights -> \"%d\" emissive hints --> (\"%d\" misses) -->(\"%d\" restarts) :: [Allocated %d MB] \n",n_lights,lc_stats.x,lc_stats.y,lc_stats.z,mb_alloc);
		printf("**In: ");
		print_hhmmss(seconds);
		printf("\n");

		renderer->PostInit(scn);
	}
	catch (std::exception& ex) {
		shut(scn, renderer);
		printf("**Fatal Exception: %s \nPress any key to quit...",ex.what());
		std::cin.get();
		return -1;
	}

	int i_max_threads = config.TryGet("i_max_threads", 0);
	int nThreads = std::thread::hardware_concurrency();
	if (i_max_threads>0) nThreads = std::min(nThreads, i_max_threads);

	auto t_start = std::chrono::steady_clock::now();

	std::thread* monitor = nullptr;

	printf("**Rendering \"%s\" using \"%d\" threads\n", scn.id.c_str(),  std::max(1,nThreads));
	if(b_start_monitor){
        monitor = new std::thread(monitor_task, std::ref(mtx), std::ref(scn), renderer, std::ref(img), i_output_depth);
        printf("**Monitor Thread started\n");
	}
	printf("\n<----    Rendering Log    ---->\n\n");

	if (nThreads > 1) {
        rng_state rng[nThreads];
        for(int n=0;n<nThreads;n++){
            rng[n] = make_rng(get_time()+n);
        }

		std::vector<std::thread*> worker(nThreads);
		for (int i = 0; i < nThreads; i++) {
            worker[i] = new std::thread(task, i, std::ref(scn), renderer, std::ref(img), std::ref(mtx), std::ref(rng[i]), std::ref(lastRow), std::ref(rowCounter));
        }
		for (int i = 0; i < nThreads; i++) { if (worker[i] && worker[i]->joinable()) { worker[i]->join(); }}
		for (int i = 0; i < nThreads; i++) { if (worker[i]){delete worker[i];} worker[i] = nullptr;}
        worker.clear();
	}
	else {
        rng_state rng = make_rng(get_time());
		task(0,scn, renderer, img, mtx, rng, lastRow, rowCounter);
	}
	renderer->mStatus.bFinished = true;
	auto t_end = std::chrono::steady_clock::now();

    if(monitor){
        if (monitor->joinable()) monitor->join();
        delete monitor;
        monitor = nullptr;
    }

    std::cout<<"\n*Primary Rays Evaluated : "<<renderer->mStatus.mRaysEvaled<<"\n";
    std::cout<<"\n*Primary Rays Lost : "<<renderer->mStatus.mRaysLost<<" : ("<<(100.0*(double(renderer->mStatus.mRaysLost)/double(renderer->mStatus.mRaysEvaled)))<<"%)\n";

	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();

	if(!renderer->mStatus.bStopped) printf("\n*Rendering completed in : ");
	else printf("\n*Rendering stopped after : ");
	print_hhmmss(seconds);

	auto fname = "VAureaNox_"+scn.id+"_"+std::to_string(img.size.x) + "x" + std::to_string(img.size.y) + ("_"+renderer->Type())+renderer->ImgAffix(scn);
	if(renderer->mStatus.bStopped) fname+="_UNCOMPLETED";

	printf("\n*Saving image \"%s\"  @ \"%d\" color depth... ", fname.c_str(),i_output_depth);
    if(vnx::save_ppm(fname, img, i_output_depth)) printf("OK\n");
    else {
        printf("FAIL\n");
        while(true){
            printf("<!>Press 1 to retry, other key to skip.\n");
            auto ch = getch();
            if(ch=='1'){
                printf("\n*Saving image \"%s\"  @ \"%d\" color depth... ", fname.c_str(),i_output_depth);
                if(vnx::save_ppm(fname, img, i_output_depth)) {printf("OK\n");break;}
                else printf("FAIL\n");
            }else break;
        }
    }

	shut(scn, renderer);
	printf("\nPress any key to quit...");
	std::cin.get();
	return 0;
}
