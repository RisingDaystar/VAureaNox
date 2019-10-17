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

#include "VVnxsParser.hpp"
#include "VFileConfigs.hpp"

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

	frame3d calculate_frame(const VNode* node, const frame3d& parent) {
		if (node == nullptr) { return identity_frame3d; }
		auto fr = parent * ygl::translation_frame(node->mTranslation);

		switch (node->mRotationOrder) {
		case VRO_XYZ:
			fr = fr * ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x) *
				ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y) *
				ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z);
			break;
		case VRO_XZY:
			fr = fr * ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x) *
				ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z) *
				ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y);
			break;
		case VRO_YXZ:
			fr = fr * ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y) *
				ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x) *
				ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z);
			break;
		case VRO_YZX:
			fr = fr * ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y) *
				ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z) *
				ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x);
			break;
		case VRO_ZXY:
			fr = fr * ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z) *
				ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x) *
				ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y);
			break;
		case VRO_ZYX:
			fr = fr * ygl::rotation_frame(vec3d{ 0,0,1 }, node->mRotation.z) *
				ygl::rotation_frame(vec3d{ 0,1,0 }, node->mRotation.y) *
				ygl::rotation_frame(vec3d{ 1,0,0 }, node->mRotation.x);
			break;
		}
		auto n_scale = !cmpf(node->mScale, zero3d) ? one3d / node->mScale : one3d;
		return fr * scaling_frame(n_scale);
	}

	void apply_transforms(VNode* node, const frame3d& parent) {
		node->mFrame = calculate_frame(node, parent);
		auto chs = node->get_childs();
		if (chs.empty()) { return; }
		for (auto& child : chs) { apply_transforms(child, node->mFrame); }
	}



	void shut(VScene& scn, VRenderer* renderer) {
		scn.shut();
		if (renderer != nullptr) { delete renderer; renderer = nullptr; }
	}

	void init(VScene& scn, VRenderer** renderer, const VFileConfigs& config) {

		auto renderer_type = config.TryGet("VAureaNox", "renderer_type", "");
		auto scn_to_render = config.TryGet("VAureaNox", "render_scene", "cornell");


		if (scn_to_render.substr(scn_to_render.find_last_of(".") + 1) == "vnxs") {
			std::cout << "**Loading external Scene : \"" << scn_to_render << "\"...\n";
			VVnxsParser sparser;
			scn_to_render = "scene_files/" + scn_to_render;
			sparser.parse(scn, scn_to_render);
		}
		else {
			std::cout << "**Loading internal Scene : \"" << scn_to_render << "\"...\n";
			if (stricmp(scn_to_render, std::string("cornell"))) { vscndef::init_cornell_scene(scn); }
			else if (stricmp(scn_to_render, std::string("spider"))) { vscndef::init_spider_scene(scn); }
			else if (stricmp(scn_to_render, std::string("pathtracing"))) { vscndef::init_pathtracing_scene(scn); }
			else if (stricmp(scn_to_render, std::string("gi_test"))) { vscndef::init_gi_test_scene(scn); }
			else if (stricmp(scn_to_render, std::string("nested"))) { vscndef::init_nested_scene(scn); }
			else { throw VException("Invalid Scene."); }
		}
		std::cout << "**Loaded Scene, id =  \"" << scn.mID << "\"\n";

		std::cout << "**Loading Renderer...\n";
		/*if (stricmp(renderer_type,std::string("pathtracer"))) {
			*renderer = new vnx::VRE_Pathtracer(config,"configs/VRE_Pathtracer.ini");
		}else */if (stricmp(renderer_type, std::string("experimental_pt"))) {
			*renderer = new vnx::VRE_Experimental_PT(config, "VRE_Experimental_PT");
		}

		if (*renderer == nullptr) { throw VException("Invalid Renderer."); }
		(*renderer)->Init(scn);
		std::cout << "**Loaded Renderer : \"" << (*renderer)->Type() << "\"\n";
	}

	void task(uint id, const VScene& scn, VRenderer* renderer, std::mutex& mtx, VRng& rng, volatile std::atomic<uint>& lrow, volatile std::atomic<uint>& rowCounter) {
		const uint width = scn.camera.mResolution.x;
		const uint height = scn.camera.mResolution.y;


		while (!renderer->mStatus.bStopped) {
			//sincronizzazione per evitare data race...insignificante overhead
			mtx.lock();
			uint j = lrow;
			lrow++;
			if (j >= height) {
				mtx.unlock(); break;
			}
			mtx.unlock();

			renderer->EvalImageRow(scn, rng, width, height, j);

			mtx.lock();
			std::cout << "Current Row:" << j << " -> Total:{" << (++rowCounter) << " / " << height << "} -> Worker:{" << id << "}\n";
			mtx.unlock();

		}
		mtx.lock();
		std::cout << "\t#Worker " << id << " finished\n";
		mtx.unlock();
	}


	//Utilizza "conio.h" , non standard, disattivabile in compilazione.
	void monitor_task(std::mutex& mtx, const VScene& scn, VRenderer* renderer, const std::string& e_save_format, bool b_apply_tonemap) {
#ifdef MONITOR_THREAD_SUPPORT
		while (!renderer->mStatus.bStopped && !renderer->mStatus.bFinished) {
			if (!_kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); continue; } //1s sleep, per evitare massiccio overhead
			auto k = _getch();

			if (k == 27) { //EXIT
				if (!renderer->mStatus.bPauseMode)mtx.lock();
				std::cout << "\n***Termination Requested***\n\n";
				renderer->mStatus.bStopped = true;
				if (!renderer->mStatus.bPauseMode)mtx.unlock();
			}
			else if (k == '1') { //PREVIEW
				if (!renderer->mStatus.bPauseMode)mtx.lock();
				auto fname = "VAureaNox_" + scn.mID + "_" + std::to_string(int(scn.camera.mResolution.x)) + "x" + std::to_string(int(scn.camera.mResolution.y)) + ("_" + renderer->Type()) + renderer->ImgAffix(scn) + "_PREVIEW";
				std::cout << "\n*Saving Preview image \"" << fname << "\" ";
				if (vnx::save_image(fname, scn.camera.ToImg(), e_save_format, b_apply_tonemap)) std::cout << "OK\n\n";
				else std::cout << "FAIL\n\n";
				if (!renderer->mStatus.bPauseMode)mtx.unlock();
			}
			else if (k == '2') { //DEBUG
				if (!renderer->mStatus.bPauseMode)mtx.lock();
				renderer->mStatus.bDebugMode = !renderer->mStatus.bDebugMode;
				if (renderer->mStatus.bDebugMode) std::cout << "\n*Debug flag set\n\n";
				else std::cout << "\n*Debug flag unset\n\n";
				if (!renderer->mStatus.bPauseMode)mtx.unlock();
			}
			else if (k == '3') { //PAUSE
				renderer->mStatus.bPauseMode = !renderer->mStatus.bPauseMode;
				if (renderer->mStatus.bPauseMode) { std::cout << "\n*Rendering Paused\n\n"; mtx.lock(); }
				else { std::cout << "\n*Rendering Resumed\n\n"; mtx.unlock(); }
			}
		}
#endif
	}

};



int main() {
	std::mutex mtx;
	std::atomic<uint> lastRow;
	lastRow.store(0);
	std::atomic<uint> rowCounter;
	rowCounter.store(0);

	VFileConfigs config("VAureaNox.vcfg");

	VScene scn;
	VRenderer* renderer = nullptr;
	bool b_start_monitor = false;

	const std::string section = "VAureaNox";
	std::string e_save_format = "hdr";

	try {
		std::cout << "<--- VAureaNox - Distance Fields Renderer - v: 0.0.9 --->\n\n";
		config.parse();


		int i_em_evals = config.TryGet(section, "i_em_evals", 10000);
		if (i_em_evals <= 0) { throw VException("i_em_evals <= 0"); }

		int i_max_march_iterations = config.TryGet(section, "i_max_march_iterations", 512);
		if (i_max_march_iterations <= 0) { throw VException("i_max_march_iterations <= 0"); }

		double f_ray_tmin = config.TryGet(section, "f_ray_tmin", 0.0001);
		if (f_ray_tmin < 0.0) { throw VException("f_ray_tmin < 0.0"); }

		double f_ray_tmax = config.TryGet(section, "f_ray_tmax", 1000.0);
		if (f_ray_tmax < 0.0 || f_ray_tmax < f_ray_tmin) { throw VException("f_ray_tmax < 0.0 || < f_ray_tmin"); }

		double f_normal_eps = config.TryGet(section, "f_normal_eps", 0.001);
		if (f_normal_eps < 0.0) { throw VException("f_normal_eps < 0.0"); }

		std::string i_st_algo = config.TryGet(section, "i_st_algo", "naive");
		std::string i_normals_algo = config.TryGet(section, "i_normals_algo", "tht");
		std::string i_eh_precalc_algo = config.TryGet(section, "i_eh_precalc_algo", "progressive");

		e_save_format = config.TryGet(section, "e_save_format", "hdr");

		init(scn, &renderer, config);

		int img_w = config.TryGet("VAureaNox", "i_image_width", 648);
		int img_h = config.TryGet("VAureaNox", "i_image_height", 480);
		if (img_w <= 0 || img_h <= 0) { throw VException("Invalid image dimensions."); }
		std::cout << "**Output resolution:  \"" << img_w << "\" x \"" << img_h << "\"\n";

		if (stricmp(i_st_algo, std::string("relaxed"))) scn.intersect_algo = &VScene::intersect_rel;
		else if (stricmp(i_st_algo, std::string("enhanced"))) scn.intersect_algo = &VScene::intersect_enh;
		else scn.intersect_algo = &VScene::intersect_naive;

		if (stricmp(i_normals_algo, std::string("tht"))) scn.normals_algo = &VScene::eval_normals_tht;
		else if (stricmp(i_normals_algo, std::string("cnt"))) scn.normals_algo = &VScene::eval_normals_cnt;
		else scn.normals_algo = &VScene::eval_normals_tht;

		if (stricmp(i_eh_precalc_algo, std::string("full"))) scn.eh_precalc_algo = &VScene::precalc_emissive_hints_full;
		else if (stricmp(i_eh_precalc_algo, std::string("strict"))) scn.eh_precalc_algo = &VScene::precalc_emissive_hints_strict;
		else if (stricmp(i_eh_precalc_algo, std::string("none"))) scn.eh_precalc_algo = nullptr;
		else scn.eh_precalc_algo = &VScene::precalc_emissive_hints_full;

		b_start_monitor = config.TryGet(section, "b_start_monitor", false);
		//


		std::cout << "\n**Preparing scene...\n";
		std::cout << "**Transforming nodes coordinates...\n";
		apply_transforms(scn.root, identity_frame3d);
		std::cout << "**Setting up camera...\n";
		scn.camera.Setup(vec2f{ float(img_w),float(img_h) });
		scn.camera.EvalAutoFocus(scn, f_ray_tmin, f_ray_tmax, i_max_march_iterations);

		if (scn.eh_precalc_algo != nullptr) {
			std::cout << "\n**Calculating emissive hints...\n";
			auto t_start = std::chrono::steady_clock::now();
			auto lc_stats = scn.populate_emissive_hints(i_em_evals, i_max_march_iterations, f_ray_tmin, f_ray_tmax, f_normal_eps, config.TryGet(section, "b_verbose_precalc", false));
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - t_start).count();
			int n_lights = scn.mEmissiveHints.size();

			const auto mb_alloc = (sizeof(VResult) * lc_stats.x) / (1024 * 1024);
			std::cout << "**Calculated : \"" << n_lights << "\" lights -> \"" << lc_stats.x << "\" emissive hints --> (\"" << lc_stats.y << "\" misses) -->(\"" << lc_stats.z << "\" restarts) :: [Allocated " << mb_alloc << " MB] \n";
			std::cout << "**In: ";
			print_hhmmss(seconds);
			std::cout << "\n";
		}
		else {
			std::cout << "\n**Skipping Emissive Hints calculation.\n";
		}

		renderer->PostInit(scn);
	}
	catch (std::exception & ex) {
		shut(scn, renderer);
		std::cout << "**Fatal Exception: " << ex.what() << " \nPress any key to quit...";
		std::cin.get();
		return -1;
	}

	bool b_apply_tonemap = config.TryGet(section, "b_apply_tonemap", true);


	unsigned int i_max_threads = config.TryGet(section, "i_max_threads", 0);
	const unsigned int nThreads = i_max_threads == 0 ? std::thread::hardware_concurrency() : std::min(std::thread::hardware_concurrency(), i_max_threads);

	std::thread* monitor = nullptr;

	std::cout << "**Rendering \"" << scn.mID << "\" using \"" << nThreads << "\" threads\n";
	if (b_start_monitor) {
		monitor = new std::thread(monitor_task, std::ref(mtx), std::ref(scn), renderer, e_save_format, b_apply_tonemap);
		std::cout << "**Monitor Thread started\n";
	}
	std::cout << "\n<----    Rendering Log    ---->\n\n";

	std::chrono::time_point<std::chrono::steady_clock> t_start;
	if (nThreads > 1) {
		VRng rng[nThreads];
		const auto tt = get_time();
		for (uint n = 0; n < nThreads; n++) {
			rng[n].seed(tt, tt, n + n, n + n + 1); //pcg32x2
			//rng[n].seed(tt,n+1); //pcg32 | splitmix
			//rng[n].seed(tt); //xoroshiro
			//rng[n].long_jump(); //xoroshiro
		}

		std::vector<std::thread*> worker(nThreads);
		t_start = std::chrono::steady_clock::now();

		for (uint i = 0; i < nThreads; i++) {
			worker[i] = new std::thread(task, i, std::ref(scn), renderer, std::ref(mtx), std::ref(rng[i]), std::ref(lastRow), std::ref(rowCounter));
		}
		for (uint i = 0; i < nThreads; i++) { if (worker[i] && worker[i]->joinable()) { worker[i]->join(); } }
		for (uint i = 0; i < nThreads; i++) { if (worker[i]) { delete worker[i]; } worker[i] = nullptr; }
		worker.clear();
	}
	else {
		VRng rng(get_time());
		t_start = std::chrono::steady_clock::now();
		task(0, scn, renderer, mtx, rng, lastRow, rowCounter);
	}
	renderer->mStatus.bFinished = true;
	auto t_end = std::chrono::steady_clock::now();

	if (monitor) {
		if (monitor->joinable()) monitor->join();
		delete monitor;
		monitor = nullptr;
	}

	std::cout << "\n*Primary Rays Evaluated : " << renderer->mStatus.mRaysEvaled << "\n";
	std::cout << "\n*Primary Rays Lost : " << renderer->mStatus.mRaysLost << " : (" << (100.0 * (double(renderer->mStatus.mRaysLost) / double(renderer->mStatus.mRaysEvaled))) << "%)\n";

	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();

	if (!renderer->mStatus.bStopped) std::cout << "\n*Rendering completed in : ";
	else std::cout << "\n*Rendering stopped after : ";
	print_hhmmss(seconds);

	auto fname = "VAureaNox_" + scn.mID + "_" + std::to_string(int(scn.camera.mResolution.x)) + "x" + std::to_string(int(scn.camera.mResolution.y)) + ("_" + renderer->Type()) + renderer->ImgAffix(scn);
	if (renderer->mStatus.bStopped) fname += "_UNCOMPLETED";

	std::cout << "\n*Saving image \"" << fname << "\" ";
	auto img = scn.camera.ToImg();
	if (vnx::save_image(fname, img, e_save_format, b_apply_tonemap)) std::cout << "OK\n";
	else {
		std::cout << "FAIL\n";
		while (true) {
			std::cout << "<!>Press 1 to retry, other key to skip.\n";
			auto ch = _getch();
			if (ch == '1') {
				std::cout << "\n*Saving image \"" << fname << "\"\n";
				if (vnx::save_image(fname, img, e_save_format, b_apply_tonemap)) { std::cout << "OK\n"; break; }
				else std::cout << "FAIL\n";
			}
			else break;
		}
	}

	shut(scn, renderer);
	std::cout << "\nPress any key to quit...";
	std::cin.get();
	return 0;
}
