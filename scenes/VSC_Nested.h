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

#ifndef _VSC_NESTED_
#define _VSC_NESTED_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_nested_scene(vnx::VScene& scn) {
        scn.camera.yfov = radians(45.0);
        scn.camera.mOrigin = {0,10,15.0f};
        scn.camera.mTarget = {0,0,0};
        scn.camera.mUp = {0,1.0f,0};
		scn.id = "nested";

		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 4000;
		emissive->e_power = 300;

		auto diffuse = scn.add_material("diffuse");
		diffuse->kr = {0.9f,0.95f,0.95f};
		//diffuse->kr = one3f;



		auto transmissive = scn.add_material("transmissive");
        transmissive->ka = {0.1f,0.1f,0.1f};
		/*transmissive->sm_b1 = 1.03961212;
		transmissive->sm_b2 = 0.231792344;
		transmissive->sm_b3 = 1.01046945;
		transmissive->sm_c1 = 6.00069867e-3;
		transmissive->sm_c2 = 2.00179144e-2;
		transmissive->sm_c3 = 1.03560653e2;
		transmissive->k_sca = 0.0f;*/

        transmissive->ka = vec3vf{0.01f,0.01f,0.01f};
        transmissive->sm_b1 = 0.3306f;
        transmissive->sm_c1 = 0.1750f;
        transmissive->sm_b2 = 4.3356f;
        transmissive->sm_c2 = 0.1060f;
        transmissive->k_sca = 0.0f;

        auto diffuse2 = scn.add_material("diffuse2");
		diffuse2->kr = {0.8f,0.8f,0.8f};
        auto mtor = [](ygl::rng_state& rng, const VResult& hit,const vec3vf& n, VMaterial& mat) {
            if (std::abs(sin(hit.wor_pos.x*5)) < 0.07f || std::abs(sin(hit.wor_pos.z*5)) < 0.07f) {
                mat.kr = zero3vf;
            }
		};
		diffuse2->mutator = mtor;


		auto root = new vop_union("root", {
            new vop_invert("sph_inv",new vvo_sd_sphere("dome",diffuse2,50.0f)),
            new vvo_sd_sphere("light",emissive,0.5f),
            new vvo_sd_plane("plane",diffuse2),
            new vvo_sd_diamond("entity",transmissive),
		});

		scn.set_root(root);

        scn.set_translation("entity",{0,2.0f,0});
        scn.set_rotation_degs("entity",{45,45,0});
        scn.set_scale("entity",3.0f);
        scn.set_rotation_order("entity",VRO_YXZ);
		scn.set_translation("light",{5,0.5f,0});
		//scn.set_translation("plane",{0,-1.0f,0});
	}

}

#endif
