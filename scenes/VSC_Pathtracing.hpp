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

#ifndef _VSC_PATHTRACING_
#define _VSC_PATHTRACING_

#include "../VSdfs.hpp"
#include "../VSdfOperators.hpp"
#include "../VScene.hpp"
using namespace vnx;

namespace vscndef {
	void init_pathtracing_scene(vnx::VScene& scn) {
		scn.mID = "pathtracing";
		scn.camera.mYfov = 45.0;
		scn.camera.mOrigin = { 0,0.8,8.0 };
		scn.camera.mTarget = { 0,0,0.01 };
		scn.camera.mUp = { 0,1.0,0 };
		scn.camera.mFocus = length(scn.camera.mTarget - scn.camera.mOrigin);
		scn.camera.mAutoFocus = true;
		scn.camera.mAperture = 0.0;

		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 6500;
		emissive->e_power = 10000;

		auto emissive_dim = scn.add_material("emissive_dim");
		emissive_dim->e_temp = 6500;
		emissive_dim->e_power = 100;

		auto diffuse_mat = scn.add_material("diffuse");
		diffuse_mat->type = diffuse;
		diffuse_mat->kr = vec3d{ 0.5,0.5,0.65 };
		diffuse_mat->ior_type = non_wl_dependant;
		diffuse_mat->ior = 1.0;
		diffuse_mat->rs = 1.0;
		auto mtor = [](const VResult& hit, const vec3d& n, VMaterial& mat) {
			if (std::abs(sin(hit.loc_pos.x * 5)) < 0.07f || std::abs(sin(hit.loc_pos.z * 5)) < 0.07f) {
				mat.type = diffuse;
				mat.kr = { 0.03,0.03,0.03 };
				mat.ior = 1.0;
			}
		};
		diffuse_mat->mutator = mtor;

		auto diffuse2 = scn.add_material("diffuse2");
		diffuse2->kr = vec3d{ 1.0f,0.3f,0.3f };

		auto diffuse3 = scn.add_material("diffuse3");
		diffuse3->kr = vec3d{ 0.3f,0.3f,0.3f };

		auto metal = scn.add_material("metal");
		metal->type = conductor;
		metal->ior = 1.5f;
		metal->ior_type = non_wl_dependant;
		metal->kr = vec3d{ 0.4f,0.9f,0.4f };
		metal->rs = 0.05f;

		auto mirror = scn.add_material("mirror");
		mirror->type = conductor;
		mirror->ior = 1.7f;
		mirror->ior_type = non_wl_dependant;
		mirror->kr = vec3d{ 1.0f,1.0f,1.0f };

		auto dispersive_material = scn.add_material("dispersive_material", get_material_archetype("water"));

		auto room = new vop_invert("room", new vvo_sd_box("box", diffuse_mat, 60.0f));
		//auto room  = new vvo_sd_box("room",glass,60.0f);
		auto light = new vvo_sd_sphere("light", emissive, 1.0f);

		auto box = new vvo_sd_box("occ_box", diffuse2, { 0.5f,2.0f,0.5f });
		auto sphere = new vvo_sd_sphere("sphere", metal, 1.0f);
		//auto sphere2 = new vvo_sd_sphere("sphere2",glass,1.0f);
		//auto diamond = new vvo_sd_diamond("diamond",carbon_diamond_material);
		auto light2 = new vvo_sd_sphere("light2", emissive_dim, 0.5f);
		auto slab = new vvo_sd_box("slab", mirror, { 1.5f,0.1f,1.5f });
		auto ring = new vop_subtraction("ring", {
			new vvo_sd_cylinder("outer",mirror,{1.3f,0.3f}),
			new vvo_sd_cylinder("inner",mirror,{1.25f,0.4f})
			}
		);
		auto ring_pedestal = new vvo_sd_box("slab2", diffuse3, { 1.5f,0.1f,1.5f });

		//auto partec = new vvo_sd_box("partecipating",partecipating,60.0f);

		auto root = new vop_union("root", {
			//partec,
			light,
			//light2,
			box,
			sphere,
			//diamond,
			slab,
			new vop_union("ring_group",0.0,{ring,ring_pedestal}),
			room,
			//new vvo_sd_box("water",dispersive_material,{10,0.2,100}),
			});
		scn.set_root(root);
		/*
		  auto ftor = [](const vec3d& p){
			   const double f = 6.0;
			   //const double fd = f*1;
			   //return (sin(f*p.x)*sin(f*p.y)*sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)

			   return (cos(sin(p.x)*p.x*cos(p.x*0.1))*sin(p.x)*
			   abs(cos(5*sin(p.y)*p.y*cos(p.y*0.1))*sin(p.y))*
			   cos(sin(p.z)*p.z*cos(p.z*0.1))*sin(p.z));
		   };*/
		   //scn.set_displacement("water",ftor);
		scn.set_translation("water", { -11,0,0 });

		scn.set_translation("root", { 0,0.0f,0 });
		scn.set_translation("room", { 0,60.0f,0 });

		scn.set_translation("occ_box", { 0,1.8,0 });
		scn.set_rotation_degs("occ_box", { 0,45,0 });
		scn.set_translation("sphere", { 2.0,1.0f,0 });
		scn.set_translation("diamond", { -2.0,1.5,0 });
		scn.set_rotation_degs("diamond", { -45,45,0 });
		scn.set_translation("slab", { -2.0,0,5.0 });
		scn.set_rotation_degs("slab", { 0,25,0 });

		scn.set_translation("ring", { 0.0,0.0,0.0 });
		scn.set_translation("ring_group", { 2.0,0.0,5.0 });

		scn.set_translation("light", { 5.0,10.0,-10.0 });
		scn.set_translation("light2", { 2.0,1.5f,5.0 });
	}
}

#endif
