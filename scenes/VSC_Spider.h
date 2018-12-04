
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


#ifndef _VSC_SPIDER_
#define _VSC_SPIDER_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	vnx::VNode* make_shell(const std::string& name, vnx::VMaterial* mat, const ygl::vec3f& dims) {
		using namespace vnx;
		vop_subtraction* sh_root = new vop_subtraction(name, 0.02f, {
			new vvo_sd_ellipsoid(name + "_el",mat,dims),
			new vvo_sd_ellipsoid(name + "_sub",mat,dims),
			//new vvo_sd_ellipsoid(name + "_sub2",mat,{dims.x / 1.5f,dims.y*2,dims.z}),
		});

		sh_root->select(name + "_sub")->set_translation({ 0,0,-0.5f });
		//sh_root->_(name + "_sub2")->set_translation({ 0,dims.y/3.0f,0.0f });
		return sh_root;
	}

	vnx::VNode* make_arm(const std::string& name, vnx::VMaterial* mat1, vnx::VMaterial* mat2, vnx::VMaterial* mat3, bool dx) {
		using namespace vnx;

		auto hand = new vop_union(name + "_hand", {
			new vop_union("palm_fingers",0.1f,{
				new vop_union("palm",0.3f,{
					new vop_intersection("palm_main",{
						new vvo_sd_box("palm_main",mat1,{ 0.18f,0.18f,0.07f }),
						new vvo_sd_ellipsoid("palm_main_int",mat1,{ 0.2f,0.6f,0.05f }),
					}),

					new vvo_sd_ellipsoid("palm_main_2",mat1,{ 0.08f,0.1f,0.05f }),
				}),

				new vop_union("fingers",{
					new vop_union("finger_1",{
						new vvo_sd_sphere("ball",mat1,0.06f),
						new vvo_sd_ellipsoid("axel",mat1,{ 0.05f,0.18f,0.05f }),
						new vop_union("finger_1_1",{
							new vvo_sd_sphere("ball",mat1,0.05f),
							new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.07f,0.03f }),
						}),
					}),
					new vop_union("finger_2",{
						new vvo_sd_sphere("ball",mat1,0.06f),
						new vvo_sd_ellipsoid("axel",mat1,{ 0.05f,0.3f,0.05f }),
						new vop_union("finger_2_1",{
							new vvo_sd_sphere("ball",mat1,0.05f),
							new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.11f,0.03f }),
							new vop_union("finger_2_2",{
								new vvo_sd_sphere("ball",mat1,0.03f),
								new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.05f,0.03f }),
							}),
						}),
					}),
					new vop_union("finger_3",{
						new vvo_sd_sphere("ball",mat1,0.06f),
						new vvo_sd_ellipsoid("axel",mat1,{ 0.05f,0.3f,0.05f }),
						new vop_union("finger_3_1",{
							new vvo_sd_sphere("ball",mat1,0.05f),
							new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.11f,0.03f }),
							new vop_union("finger_3_2",{
								new vvo_sd_sphere("ball",mat1,0.03f),
								new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.05f,0.03f }),
							}),
						}),
					}),
					new vop_union("finger_4",{
						new vvo_sd_sphere("ball",mat1,0.06f),
						new vvo_sd_ellipsoid("axel",mat1,{ 0.05f,0.3f,0.05f }),
						new vop_union("finger_4_1",{
							new vvo_sd_sphere("ball",mat1,0.05f),
							new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.11f,0.03f }),
							new vop_union("finger_4_2",{
								new vvo_sd_sphere("ball",mat1,0.03f),
								new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.05f,0.03f }),
							}),
						}),
					}),
					new vop_union("finger_5",{
						new vvo_sd_sphere("ball",mat1,0.06f),
						new vvo_sd_ellipsoid("axel",mat1,{ 0.05f,0.3f,0.05f }),
						new vop_union("finger_5_1",{
							new vvo_sd_sphere("ball",mat1,0.05f),
							new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.11f,0.03f }),
							new vop_union("finger_5_2",{
								new vvo_sd_sphere("ball",mat1,0.03f),
								new vvo_sd_ellipsoid("axel",mat1,{ 0.03f,0.05f,0.03f }),
							}),
						}),
					}),
				}),
			}),
		});


		hand->select("palm_main_int")->set_translation(0, -0.2f, 0);

		if (!dx) {

			hand->select("palm")->set_translation(-0.05f, 0, 0.1f);

			hand->select("palm_main_2")->set_translation(-0.3f, -0.15f, 0);
			hand->select("palm_main_2")->set_rotation_degs(0, 0, -15);

			hand->select("finger_1")->set_translation(-0.37f, 0.07f, -0.03f);
			hand->select("finger_2")->set_translation(-0.20f, 0, 0);
			hand->select("finger_3")->set_translation(-0.10f, 0, 0);
			hand->select("finger_4")->set_translation(0.02f, 0, 0);
			hand->select("finger_5")->set_translation(0.13f, 0, 0);
			//hand->select("hand_torso")->set_rotation_degs(10, 0, 0);

			/*
			hand->select("finger_5")->set_rotation_degs(0, 0, 22);
			hand->select("finger_4")->set_rotation_degs(0, 0, 12);
			hand->select("finger_3")->set_rotation_degs(0, 0, 2);
			hand->select("finger_2")->set_rotation_degs(0, 0, -2);
			*/
			hand->select("finger_1")->set_rotation_degs(0, -90, -22);
		}
		else {
			hand->select("palm")->set_translation(0.05f, 0, 0.1f);

			hand->select("palm_main_2")->set_translation(0.3f, -0.15f, 0);
			hand->select("palm_main_2")->set_rotation_degs(0, 0, 15);

			hand->select("finger_1")->set_translation(0.37f, 0.12f, -0.03f);
			hand->select("finger_2")->set_translation(0.20f, 0, 0);
			hand->select("finger_3")->set_translation(0.10f, 0, 0);
			hand->select("finger_4")->set_translation(-0.02f, 0, 0);
			hand->select("finger_5")->set_translation(-0.13f, 0, 0);
			//hand->select("hand_torso")->set_rotation_degs(-10, 0, 0);

			/*
			hand->select("finger_5")->set_rotation_degs(0, 0, -22);
			hand->select("finger_4")->set_rotation_degs(0, 0, -12);
			hand->select("finger_3")->set_rotation_degs(0, 0, -2);
			hand->select("finger_2")->set_rotation_degs(0, 0, 2);
			*/
			hand->select("finger_1")->set_rotation_degs(0, 90, 22);

		}



		hand->select("finger_1")->select("axel")->set_translation(0, -0.17f, 0.05f);
		hand->select("finger_2")->select("axel")->set_translation(0, -0.3f, 0.05f);
		hand->select("finger_3")->select("axel")->set_translation(0, -0.3f, 0.05f);
		hand->select("finger_4")->select("axel")->set_translation(0, -0.3f, 0.05f);
		hand->select("finger_5")->select("axel")->set_translation(0, -0.3f, 0.05f);

		hand->select("finger_1_1")->set_translation(0, -0.33f, 0);
		hand->select("finger_2_1")->set_translation(0, -0.55f, 0);
		hand->select("finger_3_1")->set_translation(0, -0.55f, 0);
		hand->select("finger_4_1")->set_translation(0, -0.55f, 0);
		hand->select("finger_5_1")->set_translation(0, -0.55f, 0);

		hand->select("finger_1_1")->select("axel")->set_translation(0, -0.08f, 0);
		hand->select("finger_2_1")->select("axel")->set_translation(0, -0.15f, 0);
		hand->select("finger_3_1")->select("axel")->set_translation(0, -0.15f, 0);
		hand->select("finger_4_1")->select("axel")->set_translation(0, -0.15f, 0);
		hand->select("finger_5_1")->select("axel")->set_translation(0, -0.15f, 0);

		hand->select("finger_2_2")->set_translation(0, -0.28f, 0);
		hand->select("finger_3_2")->set_translation(0, -0.28f, 0);
		hand->select("finger_4_2")->set_translation(0, -0.28f, 0);
		hand->select("finger_5_2")->set_translation(0, -0.28f, 0);

		hand->select("finger_2_2")->select("axel")->set_translation(0, -0.03f, 0);
		hand->select("finger_3_2")->select("axel")->set_translation(0, -0.03f, 0);
		hand->select("finger_4_2")->select("axel")->set_translation(0, -0.03f, 0);
		hand->select("finger_5_2")->select("axel")->set_translation(0, -0.03f, 0);



		auto arm = new vop_union(name + "_0", 0.15f, {
			new vvo_sd_sphere(name + "_0_joint", mat3, 0.55f),
			new vvo_sd_ellipsoid(name + "_0_axel", mat1,{ 0.4f,0.8f,0.4f }),
			new vop_union(name + "_1",0.1f,{
				//new vvo_sd_sphere(name + "_1_joint", mat2, 0.35f),
				new vvo_sd_ellipsoid(name + "_1_axel", mat1,{ 0.3f,1.0f,0.3f }),
				hand,
			}),
		});

		hand->select("fingers")->set_translation(0, -0.20f, 0.1f);
		hand->select("palm_fingers")->set_translation(0, 0, 0);
		hand->set_translation(0, -1.8f, 0.03f);


		arm->select(name + "_0_axel")->set_translation(0, -1.0f, 0.0f);
		arm->select(name + "_1")->set_translation(0, -1.6f, 0.0f);
		arm->select(name + "_1_axel")->set_translation(0, -0.9f, 0.0f);

		return arm;
	}

	vnx::VNode* make_leg(const std::string& name, vnx::VMaterial* mat1, vnx::VMaterial* mat2, vnx::VMaterial* mat3, bool dx) {
		using namespace vnx;
		auto leg = new vop_union(name + "_0", 0.1f, {
			new vvo_sd_sphere(name + "_0_joint",mat3,0.5f),
			new vvo_sd_ellipsoid(name + "_0_axel",mat1,{ 0.5f,1.3f,0.5f }),
			new vop_union(name + "_1",0.1f,{
				//new vvo_sd_sphere(name + "_1_joint",mat2,0.4f),
				new vvo_sd_ellipsoid(name + "_1_axel",mat1,{ 0.35f,1.5f,0.35f }),
				make_shell(name + "_1_shell",mat3,{ 0.25f,1.4f,0.25f }),
				new vop_union(name + "_2",0.1f,{
					//new vvo_sd_sphere(name + "_2_joint",mat2,0.3f),
					new vvo_sd_ellipsoid(name + "_2_axel",mat1,{ 0.25f,1.5f,0.25f }),
					make_shell(name + "_2_shell",mat3,{ 0.25f,1.4f,0.25f }),
					new vop_union(name + "_3",0.1f,{
						//new vvo_sd_sphere(name + "_3_joint",mat2,0.2f),
						new vvo_sd_ellipsoid(name + "_3_axel",mat1,{ 0.15f,0.8f,0.15f }),
						new vop_subtraction(name + "_3_nail",{
							new vvo_sd_ellipsoid(name + "_3_nail_e",mat3,{ 0.25f,1.1f,0.15f }),
							new vvo_sd_ellipsoid(name + "_3_nail_e_sub",mat3,{ 0.25f,1.1f,0.15f }),
						}),
					}),
				}),
			}),
		});


		leg->select(name + "_0_axel")->set_translation({ 0 ,-1.5f, 0 });
		leg->select(name + "_1")->set_translation({ 0 ,-2.8f, 0 });
		leg->select(name + "_1_axel")->set_translation({ 0 ,-1.5f, 0 });
		leg->select(name + "_2")->set_translation({ 0 ,-3.0f, 0 });
		leg->select(name + "_2_axel")->set_translation({ 0 ,-1.55f, 0 });
		leg->select(name + "_3")->set_translation({ 0 ,-3.1f, 0 });
		leg->select(name + "_3_axel")->set_translation({ 0 ,-0.75f, 0 });

		if (!dx) {
			leg->select(name + "_1_shell")->set_translation({ 0.2f,-1.4f,0 });
			leg->select(name + "_2_shell")->set_translation({ 0.1f,-1.4f,0 });
			leg->select(name + "_3_nail")->set_translation({ 0.1f,-1.4f,0 });
			leg->select(name + "_3_nail_e_sub")->set_translation({ -0.15f,0.9f,0 });


			leg->select(name + "_1")->set_rotation_degs({ 0,0,-65 });
			leg->select(name + "_2")->set_rotation_degs({ 0,0,-35 });
			leg->select(name + "_3")->set_rotation_degs({ 0,0,-45 });
			leg->select(name + "_3_nail")->set_rotation_degs({ 0,0,-18 });
		}
		else {
			leg->select(name + "_1_shell")->set_translation({ -0.2f,-1.2f,0 });
			leg->select(name + "_2_shell")->set_translation({ -0.1f,-1.2f,0 });
			leg->select(name + "_3_nail")->set_translation({ -0.1f,-1.4f,0 });
			leg->select(name + "_3_nail_e_sub")->set_translation({ 0.15f,0.9f,0 });


			leg->select(name + "_1")->set_rotation_degs({ 0,0,65 });
			leg->select(name + "_2")->set_rotation_degs({ 0,0,35 });
			leg->select(name + "_3")->set_rotation_degs({ 0,0,45 });

			leg->select(name + "_3_nail")->set_rotation_degs({ 0,0,18 });
		}
		return leg;
	}

	vnx::VNode* make_hood(const std::string& name, vnx::VMaterial* mat1, vnx::VMaterial* mat2) {
		using namespace vnx;
		auto hood = new vop_subtraction(name, 0.2f, false, {
			new vop_union(name + "_u",{
				new vvo_sd_ellipsoid(name + "_h_0",mat2,{ 0.8f,1.3f,0.8f }),
				new vvo_sd_ellipsoid(name + "_h_1",mat2,{ 0.2f,0.5f,0.7f }),
				new vvo_sd_ellipsoid(name + "_h_2",mat2,{ 0.2f,0.5f,0.7f }),
			}),
			new vvo_sd_ellipsoid(name + "_h_s1",mat1,{ 0.5f,0.3f,0.5f }),
			new vvo_sd_ellipsoid(name + "_h_s2",mat1,{ 0.6f,0.4f,0.4f }),
			new vvo_sd_ellipsoid(name + "_h_s3",mat1,{ 0.3f,0.4f,0.4f }),
		});
		hood->select(name + "_h_1")->set_translation(0, -0.3f, 0);
		hood->select(name + "_h_2")->set_translation(0, -0.3f, 0);
		hood->select(name + "_h_s1")->set_translation(0, 0.1f, 0.5f);
		hood->select(name + "_h_s2")->set_translation(0, -0.3f, 0.5f);
		hood->select(name + "_h_s3")->set_translation(0, -0.6f, 0.5f);
		return hood;
	}

	void init_spider_scene(VScene& scn) {
		scn.id = "spider";

		scn.camera._frame = ygl::lookat_frame<float>({ 0, 6.1f, 52.0f }, { 0, 6.1f, 0.0f }, { 0, 1, 0 }); //diritto
		scn.camera.focus = ygl::length(ygl::vec3f{ 0, 6.1f, 52.0f } -ygl::vec3f{ 0, 1, 0 });
		scn.camera.yfov = 15 * ygl::pif / 180;
		scn.camera.aperture = 0.00f;


		auto grey_steel_material = scn.add_material("grey_steel_material");
		grey_steel_material->type = conductor;
		grey_steel_material->kr = { 0.541931f,0.496791f,0.449419f }; //titanium
		grey_steel_material->rs = 0.02f;

		auto red_ks_material = scn.add_material("red_ks_material");
		red_ks_material->type = conductor;
		red_ks_material->rs = 0.05f;
		red_ks_material->kr = { 0.9f,0.1f,0.1f };

		auto red_diff_material = scn.add_material("red_diff_material");
		red_ks_material->type = dielectric;
		red_ks_material->ior = 1.5f;
		red_ks_material->rs = 0.05f;
		red_ks_material->kr = { 0.9f,0.1f,0.1f };

		auto black_cheratin_material = scn.add_material("black_cheratin_material");
		black_cheratin_material->ior_type = non_wl_dependant;
		black_cheratin_material->ior = 1.2f;
		black_cheratin_material->kr = { 0.03f,0.03f,0.03f };
		black_cheratin_material->rs = 0.2f;

		auto grey_material = scn.add_material("grey_material");
		grey_material->ior_type = non_wl_dependant;
		grey_material->ior = 1.05f;
		grey_material->kr = { 0.5f,0.5f,0.5f };
		grey_material->rs = 0.2f;
		auto red_material = scn.add_material("red_material");

		red_material->kr = { 0.55f,0.0f,0.0f };
		auto black_material = scn.add_material("black_material");
		black_material->ior_type = non_wl_dependant;
		black_material->ior = 1.05f;
		black_material->kr = { 0.1f,0.1f,0.1f };
		black_material->rs = 0.1f;

		auto green_material = scn.add_material("green_material");
		green_material->kr = { 0.0f,0.2f,0.0f };


		auto grey_diff_material = scn.add_material("grey_diff_material");
		grey_diff_material->kr = { 0.3f,0.3f,0.3f };
		grey_diff_material->rs = 0.15f;

		auto grey_diffuse_no_refl = scn.add_material("grey_diffuse_no_refl");
		grey_diffuse_no_refl->kr = { 0.5f,0.5f,0.5f };
		grey_diffuse_no_refl->rs = 0.15f;

		auto blue_diffuse_transparent = scn.add_material("blue_diffuse_transparent");
		blue_diffuse_transparent->k_sca = 0.0f;
		blue_diffuse_transparent->ka = { 0.9f,0.9f,0.01f };
		blue_diffuse_transparent->sm_b1 = 1.03961212;
		blue_diffuse_transparent->sm_b2 = 0.231792344;
		blue_diffuse_transparent->sm_b3 = 1.01046945;
		blue_diffuse_transparent->sm_c1 = 6.00069867e-3;
		blue_diffuse_transparent->sm_c2 = 2.00179144e-2;
		blue_diffuse_transparent->sm_c3 = 1.03560653e2;


		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 6500;
		emissive->e_power = 12500;

		auto emissive_dim = scn.add_material("emissive_dim");
		emissive_dim->e_temp = 18500;
		emissive_dim->e_power = 400;

		auto main_light = new vvo_sd_sphere("main_light", emissive, 8.3f);

		auto grey_diffuse_no_refl_adv = scn.add_material("grey_diffuse_no_refl_adv");
		grey_diffuse_no_refl_adv->ior_type = non_wl_dependant;
		grey_diffuse_no_refl_adv->ior = 1.35f;
		grey_diffuse_no_refl_adv->kr = { 0.5f,0.5f,0.5f };
		grey_diffuse_no_refl_adv->rs = 0.2f;
		auto mtor = [](ygl::rng_state& rng, const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
            if (std::abs(sin(hit.loc_pos.x*5)) < 0.07f || std::abs(sin(hit.loc_pos.z*5)) < 0.07f) {
                mat.kr = zero3f;
            }
		};
		grey_diffuse_no_refl_adv->mutator = mtor;

		auto ftor = [](const ygl::vec3f& wor_pos, ygl::vec3f& loc_pos, const VNode* tref) {
			loc_pos = ygl::transform_point_inverse(tref->_frame, wor_pos) / tref->scale;
			float dv = ygl::dot(loc_pos, ygl::normalize(ygl::vec3f{ 0,1,0 }));

			dv += sin(wor_pos.x)*sin(wor_pos.y)*sin(wor_pos.z);
			return dv*(1.0f / 2.0f);
		};
		auto base_plane = new vvo_shadered("base_plane", grey_diffuse_no_refl_adv, ftor);
		auto base_plane_v = new vvo_sd_plane("base_plane_v", grey_diffuse_no_refl_adv);


		auto ftor2 = [](const ygl::vec3f& wor_pos, ygl::vec3f& loc_pos, const VNode* tref) {
			loc_pos = ygl::transform_point_inverse(tref->_frame, wor_pos) / tref->scale;
			float dv = ygl::dot(loc_pos, ygl::normalize(ygl::vec3f{ 0,1,0 }));

			dv += sin(wor_pos.x / 6.0f)*sin(wor_pos.z / 6.0f)*0.8f;

			return dv*(1.0f / 2.0f);
		};
		//auto sea = new vvo_shadered("sea", blue_diffuse_transparent, ftor2);

		main_light->set_translation(0.0f, 50.0f, 40.0f);
		base_plane->set_translation(0.0f, -1.0f, 0.0f);
		//sea->set_translation(0.0f, 3.0f, 0.0f);

		auto sword = new vop_union("sword", 0.05f, {
			new vop_intersection("lama",{
				new vvo_sd_ellipsoid("lama_e",grey_steel_material,{ 0.25f,4.0f,0.035f }),
				new vvo_sd_box("lama_ei",grey_steel_material,{ 2.0f,2.0f,2.0f }),
			}),

			new vop_union("hilt",{
				new vvo_sd_ellipsoid("hilt_e",black_material,{ 0.8f,0.1f,0.2f }),
				new vop_intersection("jewel_right",{
					new vvo_sd_sphere("j_e",red_diff_material,0.2f),
					new vvo_sd_sphere("j_ei",red_diff_material,0.15f),
				}),
				new vop_intersection("jewel_left",{
					new vvo_sd_sphere("j_e",red_diff_material,0.2f),
					new vvo_sd_sphere("j_ei",red_diff_material,0.15f),
				}),
			}),

			new vop_union("handle",{
				new vvo_sd_cylinder("handle_e",black_material,{ 0.1f,0.5f }),
				new vop_intersection("jewel_bottom",{
					new vvo_sd_sphere("j_e",red_diff_material,0.2f),
					new vvo_sd_sphere("j_ei",red_diff_material,0.15f),
				}),
			}),
		});
		//sword->set_translation(5.0f, 5.0f, 0);
		sword->select("hilt")->set_translation(0, 0.5f, 0);
		sword->select("lama")->set_translation(0, -1.2f, 0);
		sword->select("lama")->set_scale(1.0f, 1.6f, 1.0f);
		sword->select("lama_ei")->set_translation(0, 3.0f, 0);

		sword->select("jewel_left")->set_translation(0.7f, 0, 0);
		sword->select("jewel_right")->set_translation(-0.7f, 0, 0);
		sword->select("jewel_bottom")->set_translation(0, -0.6f, 0);

		auto root_union = new vop_union("root", {
			//sea,
			new vop_union("robot_root",{
				new vop_union("top_root",{
					new vop_union("sc_head_root",0.3f,{
						new vvo_sd_ellipsoid("sc_neck",black_material,{ 0.8f,0.5f,0.5f }),
						new vop_union("sc_head_group",{
							new vvo_sd_ellipsoid("sc_head",grey_material,{ 0.5f,1.0f,0.5f }),
							make_hood("hood",red_ks_material,black_material),
						}),
					}),
					new vop_union("body_root",0.02f,{
						new vop_union("body_up_bottom_blend",0.15f,{
							new vop_subtraction("sc_b_sub",0.25f,false,{
								new vop_union("sc_b_sub_u",0.12f,{
									new vvo_sd_ellipsoid("body_sh",grey_material,{ 1.6f,1.0f,0.8f }),
									new vvo_sd_ellipsoid("body", grey_material,{ 1.0f,2.0f,0.7f }),
									new vvo_sd_ellipsoid("body_d", grey_material,{ 0.9f,1.0f,0.7f }),
								}),
								new vop_union("body_shaper",0.5f,{
									new vvo_sd_box("body_shaper_top",black_material,{ 0.2f,0.9f,0.5f }),
									new vvo_sd_box("body_shaper_top2",black_material,{ 0.05f,0.7f,0.7f }),
									new vvo_sd_ellipsoid("body_shaper_sx",grey_material,{ 1.9f,3.5f,1.8f }),
									new vvo_sd_ellipsoid("body_shaper_dx",grey_material,{ 1.9f,3.5f,1.8f }),
									new vvo_sd_ellipsoid("body_shaper_3",grey_material,{ 0.5f,0.9f,0.3f }),
									new vvo_sd_box("body_shaper_4",grey_material,{ 1.9f,1.0f,0.3f }),
								}),
							}),
							new vop_subtraction("body_coat_root",{
								new vvo_sd_ellipsoid("body_coat",red_ks_material,{ 0.8f,1.8f,0.5f }),
								new vvo_sd_ellipsoid("body_coat_s1",red_ks_material,{ 0.4f,1.8f,0.45f }),
								new vvo_sd_ellipsoid("body_coat_s2",red_ks_material,{ 0.4f,1.8f,0.45f }),
							}),
							new vvo_sd_ellipsoid("body_rear_gem_root",black_material,{ 0.6f,1.0f,0.4f }),
							new vop_union("bottom_root",{
								new vop_union("legs_root",0.2f,{
									new vvo_sd_ellipsoid("womb",black_material,{ 1.5f,1.2f,2.8f }),
									new vop_subtraction("womb_rear",0.1f,false,{
										new vvo_sd_ellipsoid("womb_rear_e",black_material,{ 2.3f,1.5f,2.6f }),
										new vvo_sd_box("womb_rear_sub_0",red_ks_material,{ 0.1f,0.8f,1.6f }),
										new vvo_sd_box("womb_rear_sub_1",red_ks_material,{ 1.6f,0.8f,0.1f }),
									}),
									new vop_union("legs_group",{
										make_leg("leg_sx_frontmost",grey_material,red_material,black_cheratin_material,false),
										make_leg("leg_sx_front",grey_material,red_material,black_cheratin_material,false),
										make_leg("leg_sx_mid",grey_material,red_material,black_cheratin_material,false),
										make_leg("leg_sx_rear",grey_material,red_material,black_cheratin_material,false),
										make_leg("leg_dx_frontmost",grey_material,red_material,black_cheratin_material,true),
										make_leg("leg_dx_front",grey_material,red_material,black_cheratin_material,true),
										make_leg("leg_dx_mid",grey_material,red_material,black_cheratin_material,true),
										make_leg("leg_dx_rear",grey_material,red_material,black_cheratin_material,true),
									}),
								}),
							}),
						}),
						new vop_union("arms_root",{
							make_arm("arm_sx",grey_material,red_material,black_material,false),
							make_arm("arm_dx",grey_material,red_material,black_material,true),
						}),
					}),
				}),
			}),


			new vop_union("base_plane_group",20.0f,{
				base_plane,
				base_plane_v,
			}),
			main_light,
		});
		scn.set_root(root_union);


		auto sc_root = scn.select("robot_root");
		sc_root->set_translation(0.0f, 11.0f, -5.0f);
		sc_root->set_rotation_degs(0.0f, 45, 0.0f);

		scn.set_translation("body_rear_gem_root", { 0,0.5f,-0.2f });

		scn.set_translation("body_coat_root", { 0,0,0.4f });
		scn.set_translation("body_coat_s1", { -0.1f,0,0 });
		scn.set_translation("body_coat_s2", { 0.1f,0,0 });
		scn.set_rotation_degs("body_coat_s1", { 0,0,30 });
		scn.set_rotation_degs("body_coat_s2", { 0,0,-30 });

		scn.set_translation("hood", { 0, 1.2f, 0 });

		scn.set_translation("bottom_root", { 0,-2.2f,-2.3f });
		scn.set_rotation_degs("bottom_root", { -10,0,0 });
		scn.set_translation("legs_group", { 0,-0.7f,-0.5f });
		scn.set_translation("womb_rear", { 0,1.9f,-4.2f });
		scn.set_rotation_degs("womb_rear", { 35,0,0 });

		scn.set_translation("womb_rear_sub_0", { 0,2.05f,0 });
		scn.set_translation("womb_rear_sub_1", { 0,2.05f,0 });


		scn.set_rotation_degs("arm_sx_0", { -10,0,0 });
		//scn.rotate_node_degs("arm_dx_0", { -50,0,-50 }); // -70 0 -20
		scn.set_rotation_degs("arm_dx_0", { -50,0,-70 });

		scn.set_rotation_degs("arm_sx_1", { -120,0,0 });
		//scn.rotate_node_degs("arm_dx_1", { -150,0,40 });
		scn.set_rotation_degs("arm_dx_1", { 0,0,0 });

		scn.set_translation("arm_sx_0", { 1.5f, 1.3f, 0.0 });
		scn.set_translation("arm_dx_0", { -1.5f, 1.3f, 0.0 });

		sc_root->select("arm_sx_0")->select("arm_sx_hand")->set_rotation_degs(25, 180, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->set_rotation_degs(0, -100, -15);

		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_1_1")->set_rotation_degs(65, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_2_1")->set_rotation_degs(45, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_3_1")->set_rotation_degs(45, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_4_1")->set_rotation_degs(45, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_5_1")->set_rotation_degs(45, 0, 0);

		//sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_1")->mod_rotation_degs(0, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_2")->set_rotation_degs(5, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_3")->set_rotation_degs(5, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_4")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_5")->set_rotation_degs(10, 0, 0);

		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_2_2")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_3_2")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_4_2")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_sx_0")->select("arm_sx_hand")->select("finger_5_2")->set_rotation_degs(10, 0, 0);




		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_1_1")->set_rotation_degs(25, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_2_1")->set_rotation_degs(125, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_3_1")->set_rotation_degs(125, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_4_1")->set_rotation_degs(125, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_5_1")->set_rotation_degs(125, 0, 0);

		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_1")->mod_rotation_degs(0, 0, 10);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_2")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_3")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_4")->set_rotation_degs(10, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_5")->set_rotation_degs(10, 0, 0);

		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_2_2")->set_rotation_degs(5, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_3_2")->set_rotation_degs(5, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_4_2")->set_rotation_degs(5, 0, 0);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->select("finger_5_2")->set_rotation_degs(5, 0, 0);

		auto energy_ball = new vop_twist("energy",Y, 5.0f,
			new vop_union("energy_e", {
				new vvo_sd_box("ball",blue_diffuse_transparent,0.5f),
				new vvo_sd_box("ball_em",emissive_dim,0.3f),
			})
			);
		energy_ball->select("energy_e")->set_rotation_degs(45, 0, 0);
		energy_ball->set_translation(0, -1.0f, -1.7f);

		sc_root->select("arm_sx_0")->select("arm_sx_hand")->add_child(energy_ball);
		sc_root->select("arm_dx_0")->select("arm_dx_hand")->add_child(sword);
		sword->set_rotation_degs(35, 0, 270);
		sword->set_translation(0.23f, -0.4f, 0);

		scn.set_translation("leg_sx_frontmost_0", { 0.7f,0.1f,2.0f });
		scn.set_translation("leg_sx_front_0", { 1.1f,0,1.0f });
		scn.set_translation("leg_sx_mid_0", { 1.1f,0,0.15f });
		scn.set_translation("leg_sx_rear_0", { 0.9f,0.1f,-0.6f });

		scn.set_translation("leg_dx_frontmost_0", { -0.7f,0.1f,2.0f });
		scn.set_translation("leg_dx_front_0", { -1.1f,0,1.0f });
		scn.set_translation("leg_dx_mid_0", { -1.1f,0,0.15f });
		scn.set_translation("leg_dx_rear_0", { -0.9f,0.1f,-0.6f });

		scn.set_rotation_degs("leg_sx_frontmost_0", { 0,-20,135 });
		scn.set_rotation_degs("leg_sx_front_0", { 0,-10,135 });
		scn.set_rotation_degs("leg_sx_mid_0", { 0,0,135 });
		scn.set_rotation_degs("leg_sx_rear_0", { 0,20,135 });

		scn.set_rotation_degs("leg_dx_frontmost_0", { 0,20,-135 });
		scn.set_rotation_degs("leg_dx_front_0", { 0,10,-135 });
		scn.set_rotation_degs("leg_dx_mid_0", { 0,0,-135 });
		scn.set_rotation_degs("leg_dx_rear_0", { 0,-20,-135 });

		//rotazioni individuali
		//vnx::rotate_node_degs(scn, "leg_dx_frontmost_1", { 0,20,-20 });
		//vnx::rotate_node_degs(scn, "leg_dx_frontmost_3", { 0, 0, 80 });

		//vnx::rotate_node_degs(scn, "leg_sx_frontmost_1", { 0,20,-10 });
		//vnx::rotate_node_degs(scn, "leg_sx_frontmost_0", { 0,-40,115 });

		//

		scn.set_translation("plane_subtractor", { 0,10.0f,-20.0f });
		scn.set_translation("base_plane_v", { 0,0.0f,-70.0f });
		scn.set_rotation_degs("base_plane_v", { 90,0.0f,0 });

		scn.set_translation("body_d", { 0,0.5f,0.1f });
		scn.set_translation("body_shaper_3", { 0,0.35f,-0.4f });
		scn.set_translation("body_shaper_4", { 0,0.95f,-1.4f });

		scn.set_translation("body_sh", { 0.0f, 1.2f, 0.0f });
		scn.set_translation("body_shaper_sx", { 2.7f, 0, 2.0f });
		scn.set_translation("body_shaper_dx", { -2.7f, 0, 2.0f });
		scn.set_translation("body_shaper_top", { 0, 1.9f, 1.45f });
		scn.set_translation("body_shaper_top2", { 0, 2.47f, 1.3f });

		scn.set_rotation_degs("body_shaper_sx", { 0, 0, -55 });
		scn.set_rotation_degs("body_shaper_dx", { 0, 0, 55 });


		scn.set_translation("sc_head_root", { 0.0f, 1.80f, 0.0f });
		scn.set_translation("sc_head", { 0.0f, 1.2f, 0.0f });
		scn.set_translation("top_root", { 0.0f, -2.8f, 0.0f });


	}

}

#endif
