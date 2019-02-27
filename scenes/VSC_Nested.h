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

#ifndef _VSC_NESTED_
#define _VSC_NESTED_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_nested_scene(vnx::VScene& scn) {
        scn.camera.yfov = radians(45);
        scn.camera.mOrigin = {0,0,15.0f};
        scn.camera.mTarget = {0,0,0};
        scn.camera.mUp = {0,1.0f,0};
		scn.id = "nested";

		auto emissive = scn.add_material("emissive");
		emissive->e_temp = 6500;
		emissive->e_power = 40;

		auto diffuse = scn.add_material("diffuse");
		diffuse->kr = {0.9f,0.95f,0.95f};
		//diffuse->kr = one3f;

		auto diffuse2 = scn.add_material("diffuse2");
		diffuse2->kr = {0.8f,0.3f,0.3f};
		diffuse2->type = conductor;

		auto transmissive = scn.add_material("transmissive");
		transmissive->k_sca = 0.0f;
		transmissive->ior_type = non_wl_dependant;
		transmissive->ior = 1.2f;

		auto root = new vop_union("root", {
            //new vop_invert("sph_inv",new vvo_sd_sphere("dome",emissive,30.0f)),
            new vvo_sd_sphere("light",emissive,3.0f),
            new vvo_sd_plane("plane",diffuse),
            new vvo_sd_sphere("sphere",transmissive,1.0f),
		});

		scn.set_root(root);

		scn.set_translation("light",{7,10.0f,2});
	}

}

#endif
