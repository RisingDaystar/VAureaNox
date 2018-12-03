
//
//BSD 3-Clause License
//
//Copyright (c) 2017-2018, Alessandro Berti
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met:
//
//* Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//* Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//* Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from
//  this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


#ifndef _VSC_PATHTRACING_
#define _VSC_PATHTRACING_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_pathtracing_scene(vnx::VScene& scn) {
		scn.id = "pathtracing";
		scn.camera._frame = ygl::lookat_frame<float>({ 0.0, 3.2f, 30.0f }, { 0.0f, 0.1f, 0.0f }, { 0, 1, 0 }); //diritto
		scn.camera.focus = ygl::length(ygl::vec3f{ 0, 3.1f,30.0f } -ygl::vec3f{ 0, 1, 0 });
		scn.camera.yfov = 15 * ygl::pif / 180;
		scn.camera.aperture = 0.00f;


		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 6500;
		emissive->e_power = 6000;

		auto emissive_dim = scn.add_material("emissive_dim");
		emissive_dim->e_temp = 6500;
		emissive_dim->e_power = 20;

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
		diffuse3->kr = vec3f{0.2f,0.2f,0.2f};

        auto metal = scn.add_material("metal");
        metal->type = conductor;
        metal->ior = 1.5f;
        metal->ior_type = non_wl_dependant;
		metal->kr = vec3f{0.4f,0.9f,0.4f};
		metal->rs = 0.20f;

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


		auto gas = scn.add_material("gas");
		gas->k_sca = 0.2f;
		gas->ka = {0.0f,0.9f,0.9f};


        auto room = new vop_invert("room",new vvo_sd_box("box",diffuse,60.0f));
        auto light = new vvo_sd_sphere("light",emissive,10.0f);

        auto box = new vvo_sd_box("box",diffuse2,{0.5f,2.0f,0.5f});
        auto sphere = new vvo_sd_sphere("sphere",metal,1.0f);
        auto sphere2 = new vvo_sd_sphere("sphere2",glass,1.0f);
        auto sphere3 = new vvo_sd_sphere("sphere3",gas,1.0f);
        auto light2 = new vvo_sd_sphere("light2",emissive_dim,0.5f);
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
            sphere3,
            slab,
            new vop_union("ring_group",{ring,ring_pedestal,light2}),
            room,
		});
		scn.set_root(root);



		scn.set_translation("root",{0,0.0f,0});
		scn.set_translation("room",{0,60.0f,0});

		scn.set_translation("box",{0,1.8f,0});
		scn.set_rotation_degs("box",{0,45,0});
		scn.set_translation("sphere",{2.0f,1.0f,0});
		scn.set_translation("sphere2",{-2.0f,1.5f,0});
		scn.set_translation("sphere3",{4.0f,1.5f,0});
        scn.set_translation("slab",{-2.0f,0,5.0f});
        scn.set_rotation_degs("slab",{0,25,0});

        scn.set_translation("ring",{0.0f,0.0f,0.0f});
        scn.set_translation("ring_group",{2.0f,0.0f,5.0f});

		scn.set_translation("light",{20.0f,30.0f,25.0f});
		scn.set_translation("light2",{1.5f,2.0f,1.5f});

	}

}

#endif
