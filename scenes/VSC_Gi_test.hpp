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

#ifndef _VSC_GI_TEST_H_
#define _VSC_GI_TEST_H_

#include "../VSdfs.hpp"
#include "../VSdfOperators.hpp"
#include "../VScene.hpp"
using namespace vnx;

namespace vscndef {
	void init_gi_test_scene(vnx::VScene& scn) {
		scn.mCamera.mYfov = 45.0;
		scn.mCamera.mOrigin = { 0,5,20.0 };
		scn.mCamera.mTarget = { 0,0,0 };
		scn.mCamera.mUp = { 0,1.0,0 };
		scn.mID = "gi_test";

		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 6500; //orig : 15
		emissive->e_power = 24800; //orig : 15
		auto emissive_dim = scn.add_material("emissive_dim");
		emissive_dim->e_temp = 6500; //orig : 15
		emissive_dim->e_power = 880; //orig : 15

		auto diffuse_exp = scn.add_material("diffuse_exp");
		diffuse_exp->type = dielectric;
		diffuse_exp->ior_type = non_wl_dependant;
		diffuse_exp->ior = 1.4;
		diffuse_exp->kr = { 0.6,0.0f,0.0 };
		diffuse_exp->rs = 0.15;
		auto mtor_e = [](const VResult& hit, const vec3d& n, VMaterial& mat) {
			if (dot(hit.wor_pos, hit.loc_pos) > 0) { mat.e_temp = 6000.0; mat.e_power = 1000; }
		};
		diffuse_exp->mutator = mtor_e;

		auto diffuse_mat = scn.add_material("diffuse");
		diffuse_mat->type = diffuse;
		diffuse_mat->kr = { 0.8,0.8,0.8 };
		auto mtor = [](const VResult& hit, const vec3d& n, VMaterial& mat) {
			if (std::abs(sin(hit.wor_pos.x)) < 0.03 || std::abs(sin(hit.wor_pos.z)) < 0.03) {
				mat.type = conductor;
				mat.kr = { 0.4,0.4,0.5 };
				mat.rs = 0.9;
			}
		};
		diffuse_mat->mutator = mtor;

		auto diffuse_pure = scn.add_material("diffuse_pure");
		diffuse_pure->kr = { 0,1.0,0 };

		auto diffuse_sand = scn.add_material("diffuse_sand");
		diffuse_sand->kr = { 0.7,0.6,0.4 };

		auto steel = scn.add_material("steel");
		steel->type = conductor;
		steel->ior_type = non_wl_dependant;
		steel->kr = { 0.8,0.8,0.9 };
		steel->rs = 0.05;
		steel->ior = 2.6;

		auto sp_mat = scn.add_material("sp_mat");
		sp_mat->type = dielectric;
		sp_mat->kr = { 0.2,0.5,0.8 };
		sp_mat->ior_type = non_wl_dependant;
		sp_mat->ior = 2.2;

		auto ring = new vop_twist("ring", X, 0.9f, new vop_subtraction("ring_s", 1.0, {
				new vvo_sd_cylinder("ring_e_ex",diffuse_exp,{4.0,0.5}),
				new vvo_sd_cylinder("ring_e_in",diffuse_exp,{3.0,1.0}),
			})
			);

		scn.mRoot = new vop_union("root", 1.0, {
			//new vop_union("pv_blend",20.5,{
				new vop_union("p_b_blend",0.0,{ //0.8f
					new vop_subtraction("plane_sub",0.5,{
						//new vop_union("plane_union",10.0,{
							new vvo_sd_plane("plane",diffuse_mat),
							//new vvo_sd_plane("plane_v",diffuse_mat),
						//}),
						new vop_twist("plane_sub_t",Y,1.9,
							new vvo_sd_ellipsoid("plane_sub_e",diffuse_sand,{7.5,3.5,4.5})
						),
					}),
					ring,
				}),

			//}),
			/*
		   new vop_cut("sph2_group",{1,1,1},zero3d,0.1,{
			  //new vvo_sd_sphere("coat2",partecipating_test,1.8f),
			  new vop_onion("sph2_o",0.2f,new vvo_sd_sphere("sph2",steel,3.5)),
		   }),*/
		   new vvo_sd_sphere("light",emissive,80.5),
			});

		scn.set_translation("sph2_group", { 2,2,10 });
		scn.set_translation("light", { 0,180,0 });

		auto ftor = [](const vec3d& p) {
			const double f = 6.0;
			const double fd = f * 1;
			return (std::sin(f * p.x) * std::sin(f * p.y) * std::sin(f * p.z)) / fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
		};
		scn.set_displacement("ring", ftor);

		auto ftor3 = [](const vec3d& p) {
			const double f = 1;
			const double fd = f * 1;
			return (std::sin(f * p.x) * std::sin(f * p.y) * std::sin(f * p.z)) / fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
		};
		scn.set_displacement("plane_sub_t", ftor3);
		scn.set_scale("ring", { 1.2,1.0,1.2 });

		//scn.set_scale("coat",{3.0f,3.0f,3.0f});
		scn.set_translation("plane_v", { 0,0,-45 });
		scn.set_rotation_degs("plane_v", { 90,0,0 });
		scn.set_rotation_degs("sph2_group", { 25,25,25 });
	}
};

#endif // VSC_GI_TEST_H_INCLUDED
