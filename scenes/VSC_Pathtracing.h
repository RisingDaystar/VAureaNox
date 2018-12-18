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

#ifndef _VSC_PATHTRACING_
#define _VSC_PATHTRACING_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_pathtracing_scene(vnx::VScene& scn) {
		scn.id = "pathtracing";
		scn.camera._frame = ygl::lookat_frame<float>({ 0.0, 1.2f, 30.0f }, { 0.0f, 0.1f, 0.0f }, { 0, 1, 0 }); //diritto
		scn.camera.focus = ygl::length(ygl::vec3f{ 0, 3.1f,30.0f } -ygl::vec3f{ 0, 1, 0 });
		scn.camera.yfov = 15 * ygl::pif / 180;
		scn.camera.aperture = 0.00f;


		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 6500;
		emissive->e_power = 9000;

		auto emissive_dim = scn.add_material("emissive_dim");
		emissive_dim->e_temp = 6500;
		emissive_dim->e_power = 200;

		auto diffuse = scn.add_material("diffuse");
		diffuse->kr = vec3f{0.5f,0.5f,0.65f};
		diffuse->ior_type = non_wl_dependant;
		diffuse->ior = 1.3f;
		diffuse->rs = 0.04f;
		auto mtor = [](ygl::rng_state& rng, const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
			if (std::abs(sin(hit.loc_pos.x*5)) < 0.07f || std::abs(sin(hit.loc_pos.z*5)) < 0.07f) {
                mat.kr = {0.03f,0.03f,0.03f};
                mat.ior = 1.0f;
            }
		};
		diffuse->mutator = mtor;


        auto diffuse2 = scn.add_material("diffuse2");
		diffuse2->kr = vec3f{1.0f,0.3f,0.3f};

        auto diffuse3 = scn.add_material("diffuse3");
		diffuse3->kr = vec3f{0.8f,0.0f,0.0f};

        auto metal = scn.add_material("metal");
        metal->type = conductor;
        metal->ior = 1.5f;
        metal->ior_type = non_wl_dependant;
		metal->kr = vec3f{0.4f,0.9f,0.4f};
		metal->rs = 0.05f;

        auto mirror = scn.add_material("mirror");
        mirror->type = conductor;
        mirror->ior = 1.7f;
        mirror->ior_type = non_wl_dependant;
		mirror->kr = vec3f{1.0f,1.0f,1.0f};

		auto glass = scn.add_material("glass");
		glass->k_sca = 0.0f;
		glass->sm_b1 = 1.03961212;
		glass->sm_b2 = 0.231792344;
		glass->sm_b3 = 1.01046945;
		glass->sm_c1 = 6.00069867e-3;
		glass->sm_c2 = 2.00179144e-2;
		glass->sm_c3 = 1.03560653e2;
		glass->ka = {0.01f,0.01f,0.01f};


        auto room = new vop_invert("room",new vvo_sd_box("box",diffuse,60.0f));
        auto light = new vvo_sd_sphere("light",emissive,10.0f);

        auto box = new vvo_sd_box("box",diffuse2,{0.5f,2.0f,0.5f});
        auto sphere = new vvo_sd_sphere("sphere",metal,1.0f);
        auto sphere2 = new vvo_sd_sphere("sphere2",glass,1.0f);
        //auto light2 = new vvo_sd_sphere("light2",emissive_dim,0.5f);
        auto slab = new vvo_sd_box("slab",mirror,{1.5f,0.1f,1.5f});
        auto ring = new vop_subtraction("ring",{
            new vvo_sd_cylinder("outer",mirror,{1.3f,0.3f}),
            new vvo_sd_cylinder("inner",mirror,{1.25f,0.4f})
            }
        );
        auto ring_pedestal = new vvo_sd_box("slab2",diffuse3,{1.5f,0.1f,1.5f});

		auto root = new vop_union("root", {
            light,
            box,
            sphere,
            sphere2,
            slab,
            new vop_union("ring_group",{ring,ring_pedestal}),
            room,
		});
		scn.set_root(root);



		scn.set_translation("root",{0,0.0f,0});
		scn.set_translation("room",{0,60.0f,0});

		scn.set_translation("box",{0,1.8f,0});
		scn.set_rotation_degs("box",{0,45,0});
		scn.set_translation("sphere",{2.0f,1.0f,0});
		scn.set_translation("sphere2",{-2.0f,1.5f,0});
        scn.set_translation("slab",{-2.0f,0,5.0f});
        scn.set_rotation_degs("slab",{0,25,0});

        scn.set_translation("ring",{0.0f,0.0f,0.0f});
        scn.set_translation("ring_group",{2.0f,0.0f,5.0f});

		scn.set_translation("light",{20.0f,30.0f,25.0f});
		scn.set_translation("light2",{1.5f,2.0f,1.5f});

	}

}

#endif
