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

#include "VScene.hpp"

namespace vnx {
	void VScene::Shut() {
		if (mRoot) delete mRoot;
		mMaterials.clear();
		for (auto& emhg : mEmissiveHints) {
			emhg.clear();
		}
		mEmissiveHints.clear();
	}

	VMaterial* VScene::add_material(std::string id) {
		if (mMaterials.find(id) == mMaterials.end()) {
			mMaterials[id] = VMaterial();
		}
		return &mMaterials[id];
	}
	VMaterial* VScene::add_material(std::string id, VMaterial mtl) {
		if (mMaterials.find(id) == mMaterials.end()) {
			mMaterials[id] = mtl;
		}
		return &mMaterials[id];
	}

	bool VScene::set_translation(const std::string& idv, const vec3d& amount) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_translation(amount);
		return true;
	}
	bool VScene::set_translation(const std::string& idv, double x, double y, double z) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_translation(x, y, z);
		return true;
	}

	bool VScene::set_rotation(const std::string& idv, const vec3d& amount) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_rotation(amount);
		return true;
	}
	bool VScene::set_rotation(const std::string& idv, double x, double y, double z) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_rotation(x, y, z);
		return true;
	}

	bool VScene::set_rotation_degs(const std::string& idv, const vec3d& amount) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_rotation_degs(amount);
		return true;
	}
	bool VScene::set_rotation_degs(const std::string& idv, double x, double y, double z) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_rotation_degs(x, y, z);
		return true;
	}

	bool VScene::set_rotation_order(const std::string& idv, VRotationOrder ro) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_rotation_order(ro);
		return true;
	}

	bool VScene::set_scale(const std::string& idv, const vec3d& amount) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_scale(amount);
		return true;
	}
	bool VScene::set_scale(const std::string& idv, double x, double y, double z) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_scale(x, y, z);
		return true;
	}
	bool VScene::set_scale(const std::string& idv, double amount) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_scale(vec3d{ amount,amount,amount });
		return true;
	}

	bool VScene::set_rounding(const std::string& idv, double r) {
		auto node = select(idv);
		if (node == nullptr) { return false; }
		node->set_rounding(r);
		return true;
	}

	bool VScene::set_displacement(const std::string& idv, displ_ftor ftor) {
		auto node = select(idv);
		if (!node) { return false; }
		node->set_displacement(ftor);
		return true;
	}

	//TODO FIXME: wrong results for displaced & distance domain operators modified SDFS
	vec3i VScene::precalc_emissive_hints_warp(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame) {
		if (!ptr) { return zero3i; }
		vec3i stats = zero3i;

		auto fr = calculate_frame(ptr, parent_frame);

		if (!ptr->isOperator()) {
			if (verbose)std::cout << "::Testing Volume " << ptr->mID;
			VResult fvre;
			ptr->eval(transform_point(fr, zero3d), fvre);

			//OPTIMIZATION , if assigned material is not emissive and does not have a mutator , ignore.
			//WARNING, might need a check towards casted ptr, better than having to rely on isOperator();
			{
				const auto sdf_ptr = static_cast<VSdf*>(ptr);
				if (!sdf_ptr || !sdf_ptr->mMaterial ||
					(!sdf_ptr->mMaterial->is_emissive() && !sdf_ptr->mMaterial->mutator)) {
					if (verbose)std::cout << " ->Ignored\n";
					goto ignore;
				}
			}

			VResult vre = fvre;
			VRay ray;

			//ANALYZE INTO LOCAL SDF SPACE
			for (uint i = 0; i < n_em_e; i++) {
				const auto step = std::max(1, n_em_e / 5);
				if (verbose && i % step == 0) std::cout << ".";

				if (!vre.isFound()) {
					auto dir = sample_sphere_direction<double>(rng.next_vecd<2>());
					ray = offsetted_ray(fvre.wor_pos, {}, dir, tmin, tmax, dir, tmin);
					vre = intersect_node(ptr, ray, n_max_iters);
					stats.y++;
					if (!vre.isFound()) continue;
				} else if (rng.next_double() > 0.95) {
					auto dir = sample_sphere_direction<double>(rng.next_vecd<2>());
					ray = offsetted_ray(fvre.wor_pos, {}, dir, tmin, tmax, dir, tmin);
					vre = intersect_node(ptr, ray, n_max_iters);
					stats.z++;
					if (!vre.isFound()) continue;
				}
				auto norm = eval_normals(vre, neps);

				VMaterial vmtl;
				vre.getVMaterial(vmtl);
				vmtl.eval_mutator(vre, norm, vmtl);

				VMaterial mtl;
				vre.getMaterial(mtl);
				mtl.eval_mutator(vre, norm, mtl);

				if (true) { // TEST
					//WARPS INTO SCENE WORLD SDF SPACE
					VResult w_vre = eval(vre.wor_pos);
					auto w_norm = eval_normals(w_vre, neps);

					VMaterial w_vmtl;
					w_vre.getVMaterial(w_vmtl);
					w_vmtl.eval_mutator(w_vre, w_norm, w_vmtl);

					VMaterial w_mtl;
					w_vre.getMaterial(w_mtl);
					w_mtl.eval_mutator(w_vre, w_norm, w_mtl);

					if (w_vre.vsur && w_vmtl.is_emissive()) {
						w_vre._found = true;
						emap[w_vre.vsur->mID].push_back(w_vre);
						stats.x++;
					} else if (w_vre.sur && w_mtl.is_emissive()) {
						w_vre._found = true;
						emap[w_vre.sur->mID].push_back(w_vre);
						stats.x++;
					}
				}//

				auto fp = ygl::make_frame_fromz(zero3d, -norm);
				auto offalong = -norm;
				auto dir = ygl::transform_direction(fp, sample_hemisphere_direction<double>(rng.next_vecd<2>()));
				ray = offsetted_ray(vre.wor_pos, {}, dir, tmin, tmax, offalong, vre.dist);
				vre = intersect_node(ptr, ray, n_max_iters);
			}
			if (!stats.x) { stats.y = 0; stats.z = 0; } //if no emissive found, ignore other stats
			if (verbose)std::cout << "->Found: " << stats.x << "/(" << stats.y << "/" << stats.z << ")\n";
		} else {
			if (verbose)std::cout << "::Parsing Operator " << ptr->mID << "\n";
		}

	ignore:

		auto chs = ptr->get_childs();
		if (chs.empty()) { return stats; }
		for (auto node : chs) {
			stats += precalc_emissive_hints(rng, emap, node, n_em_e, n_max_iters, tmin, tmax, neps, verbose, fr);
		}

		return stats;
	}

	vec3i VScene::precalc_emissive_hints_full(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame) {
		if (!ptr) { return zero3i; }
		vec3i stats = zero3i;

		auto fr = calculate_frame(ptr, parent_frame);

		if (!ptr->isOperator()) {
			if (verbose)std::cout << "::Testing Volume " << ptr->mID;
			VResult vre;
			eval(transform_point(fr, zero3d), vre);

			//OPTIMIZATION , if assigned material is not emissive and does not have a mutator , ignore.
			//WARNING, might need a check towards casted ptr, better than having to rely on isOperator();
			{
				const auto sdf_ptr = static_cast<VSdf*>(ptr);
				if (!sdf_ptr || !sdf_ptr->mMaterial ||
					(!sdf_ptr->mMaterial->is_emissive() && !sdf_ptr->mMaterial->mutator)) {
					if (verbose)std::cout << " ->Ignored\n";
					goto ignore;
				}
			}

			//N.B: tests any possible volume , searching for emissive materials (which can happen even if assigned material is not
			//homogeneously emissive, because of procedural mutated materials)

			if (vre.sur && vre.mtl) { //allows for starting outside
				vec3d norm = eval_normals(vre, neps);

				VMaterial vre_material;
				vre.getMaterial(vre_material);
				if (vre_material.mutator != nullptr) {
					vre_material.eval_mutator(vre, norm, vre_material);
				};
				VMaterial vre_vmaterial;
				vre.getVMaterial(vre_vmaterial);
				if (vre_vmaterial.mutator != nullptr) {
					vre_vmaterial.eval_mutator(vre, norm, vre_vmaterial);
				};

				if (vre_vmaterial.is_emissive() && vre.vsur) {
					std::vector<VResult>* epoints = &emap[vre.vsur->mID];
					vre._found = true;
					epoints->push_back(vre); stats.x++;
				} else if (vre_material.is_emissive() && vre.sur) {
					std::vector<VResult>* epoints = &emap[vre.sur->mID];
					vre._found = true;
					epoints->push_back(vre); stats.x++;
				}

				vre._found = false;

				vec3d dir;
				VRay ray;
				VResult fvre = vre;
				for (int i = 0; i < n_em_e; i++) {
					const auto step = std::max(1, n_em_e / 10);
					if (verbose && i % step == 0) std::cout << ".";

					if (!vre.isFound()) {
						dir = sample_sphere_direction<double>(rng.next_vecd<2>());
						ray = offsetted_ray(fvre.wor_pos, {}, dir, tmin, tmax, dir, tmin);
						vre = intersect(ray, n_max_iters);
						stats.y++;
						if (!vre.isFound())continue;
					} else if (rng.next_double() > 0.95) {
						dir = sample_sphere_direction<double>(rng.next_vecd<2>());
						ray = offsetted_ray(fvre.wor_pos, {}, dir, tmin, tmax, dir, tmin);
						vre = intersect(ray, n_max_iters);
						//n_em_e++; //reduce math bias
						stats.z++;
						if (!vre.isFound())continue;
					}

					auto norm = eval_normals(vre, neps);
					VMaterial er_material;
					vre.getMaterial(er_material);
					if (er_material.mutator != nullptr) {
						er_material.eval_mutator(vre, norm, er_material);
					}
					VMaterial er_vmaterial;
					vre.getVMaterial(er_vmaterial);
					if (er_vmaterial.mutator != nullptr) {
						er_vmaterial.eval_mutator(vre, norm, er_vmaterial);
					}

					frame3d fp;
					vec3d offalong;
					bool straight = false;
					if (dot(ray.d, norm) > 0.0) {
						fp = ygl::make_frame_fromz(zero3d, -norm);
						offalong = -norm;
					} else {
						if (vre.vdist < 0.0) {
							if (er_material.is_emissive()) {
								fp = ygl::make_frame_fromz(zero3d, -norm);
								offalong = -norm;
							} else if (vre.dist < 0.0) {
								if (er_material.mutator && rng.next_double() < 0.95) {
									fp = ygl::make_frame_fromz(zero3d, -norm);
									offalong = -norm;
								} else {
									//fp = ygl::make_frame_fromz(zero3d, norm);
									straight = true;
									offalong = norm;
								}
							} else {
								fp = ygl::make_frame_fromz(zero3d, norm);
								offalong = norm;
							}
						} else {
							fp = ygl::make_frame_fromz(zero3d, -norm);
							offalong = -norm;
						}
					}
					if (!straight)dir = ygl::transform_direction(fp, sample_hemisphere_direction<double>(rng.next_vecd<2>()));
					else dir = ray.d;
					ray = offsetted_ray(vre.wor_pos, {}, dir, tmin, tmax, offalong, vre.dist); //TEST -norm && norm

					if (er_vmaterial.is_emissive() && vre.vsur) {
						std::vector<VResult>* epoints = &emap[vre.vsur->mID];
						vre._found = true;
						epoints->push_back(vre); stats.x++;
					} else if (er_material.is_emissive() && vre.sur) {
						std::vector<VResult>* epoints = &emap[vre.sur->mID];
						vre._found = true;
						epoints->push_back(vre); stats.x++;
					}
					vre = intersect(ray, n_max_iters);
				}
			}
			if (!stats.x) { stats.y = 0; stats.z = 0; } //if no emissive found, ignore other stats
			if (verbose)std::cout << "->Found: " << stats.x << "/(" << stats.y << "/" << stats.z << ")\n";
		} else {
			if (verbose)std::cout << "::Parsing Operator " << ptr->mID << "\n";
		}

	ignore:
		auto chs = ptr->get_childs();
		if (chs.empty()) { return stats; }
		for (auto node : chs) {
			stats += precalc_emissive_hints(rng, emap, node, n_em_e, n_max_iters, tmin, tmax, neps, verbose, fr);
		}

		return stats;
	}

	vec3i VScene::precalc_emissive_hints_strict(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame) {
		if (!ptr) { return { 0,0,0 }; }
		vec3i stats = { 0,0,0 };

		auto fr = calculate_frame(ptr, parent_frame);

		if (!ptr->isOperator()) {
			if (verbose)std::cout << "::Testing Volume " << ptr->mID;
			VResult vre;
			eval(transform_point(fr, zero3d), vre);

			//N.B: Searchs only inside "emissive material declared" volumes

			if (vre.vdist < 0.0 && vre.sur && vre.vsur) {
				VMaterial vre_material;
				vre.getMaterial(vre_material);
				vec3d norm = eval_normals(vre, neps);
				if (vre_material.mutator != nullptr) {
					vre_material.eval_mutator(vre, norm, vre_material);
				};
				VMaterial vre_vmaterial;
				vre.getVMaterial(vre_vmaterial);
				if (vre_vmaterial.mutator != nullptr) {
					vre_vmaterial.eval_mutator(vre, norm, vre_vmaterial);
				};

				if (vre_vmaterial.is_emissive() || vre_material.is_emissive()) {
					std::vector<VResult>* epoints = &emap[vre.sur->mID];
					vre._found = true;
					epoints->push_back(vre); stats.x++;

					vre._found = false;
					vec3d dir;
					VRay ray;
					VResult fvre = vre;
					for (int i = 0; i < n_em_e; i++) {
						const auto step = std::max(1, n_em_e / 10);
						if (verbose && i % step == 0) std::cout << ".";
						vre = intersect(ray, n_max_iters);

						if (!vre.isFound()) {
							dir = sample_sphere_direction<double>(rng.next_vecd<2>());
							ray = offsetted_ray(fvre.wor_pos, {}, dir, tmin, tmax, dir, tmin);
							vre = intersect(ray, n_max_iters);
							stats.y++;
							if (!vre.isFound())continue;
						} else if (rng.next_double() > 0.95) {
							dir = sample_sphere_direction<double>(rng.next_vecd<2>());
							ray = offsetted_ray(fvre.wor_pos, {}, dir, tmin, tmax, dir, tmin);
							vre = intersect(ray, n_max_iters);
							//n_em_e++; //reduce math bias
							stats.z++;
							if (!vre.isFound())continue;
						}

						auto norm = eval_normals(vre, neps);
						VMaterial er_material;
						vre.getMaterial(er_material);
						if (er_material.mutator != nullptr) {
							er_material.eval_mutator(vre, norm, er_material);
						}
						VMaterial er_vmaterial;
						vre.getVMaterial(er_vmaterial);
						if (er_vmaterial.mutator != nullptr) {
							er_vmaterial.eval_mutator(vre, norm, er_vmaterial);
						}

						frame3d fp;
						vec3d offalong;
						if (dot(ray.d, norm) > 0.0) {
							fp = ygl::make_frame_fromz(zero3d, -norm);
							offalong = -norm;
						} else {
							if (vre.vdist < 0.0) {
								if (er_material.is_emissive() || vre.dist < 0.0) {
									fp = ygl::make_frame_fromz(zero3d, -norm);
									offalong = -norm;
								} else {
									fp = ygl::make_frame_fromz(zero3d, norm);
									offalong = norm;
								}
							} else {
								fp = ygl::make_frame_fromz(zero3d, -norm);
								offalong = -norm;
							}
						}

						dir = ygl::transform_direction(fp, sample_hemisphere_direction<double>(rng.next_vecd<2>()));

						ray = offsetted_ray(vre.wor_pos, {}, dir, tmin, tmax, offalong, vre.dist); //TEST -norm && norm

						if (er_vmaterial.is_emissive()) {
							std::vector<VResult>* epoints = &emap[vre.vsur->mID];
							vre._found = true;
							epoints->push_back(vre); stats.x++;
						}
						vre = intersect(ray, n_max_iters);
					}
				}
			}
			if (!stats.x) { stats.y = 0; stats.z = 0; } //if no emissive found, ignore other stats
			if (verbose)std::cout << "->Found: " << stats.x << "/(" << stats.y << "/" << stats.z << ")\n";
		} else {
			if (verbose)std::cout << "::Parsing Operator " << ptr->mID << "\n";
		}
		auto chs = ptr->get_childs();
		if (chs.empty()) { return stats; }
		for (auto node : chs) {
			stats += precalc_emissive_hints(rng, emap, node, n_em_e, n_max_iters, tmin, tmax, neps, verbose, fr);
		}

		return stats;
	}

	vec3i VScene::populate_emissive_hints(int i_em_evals, int i_max_march_iterations, double f_ray_tmin, double f_ray_tmax, double f_normal_eps, bool verbose) {
		auto rng = VRng(get_time());
		std::map<std::string, std::vector<VResult>> tmpMap;
		vec3i stats = precalc_emissive_hints(rng, tmpMap, mRoot, i_em_evals, i_max_march_iterations, f_ray_tmin, f_ray_tmax, f_normal_eps, verbose, identity_frame3d);
		for (auto ceh : tmpMap) {
			mEmissiveHints.push_back(ceh.second);
		}
		return stats;
	}
};
