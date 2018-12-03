
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


#ifndef _VSC_CORNELL_
#define _VSC_CORNELL_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_cornell_scene(vnx::VScene& scn) {

		scn.camera._frame = ygl::lookat_frame<float>({ 0, 2.6f, 33.0f }, { 0, 0.1f, 0.0f }, vec3f{ 0, 1, 0 }); //diritto
		scn.camera.focus = ygl::length(ygl::vec3f{ 0, 0.1f,45.0f } -ygl::vec3f{ 0, 1, 0 });
		scn.camera.yfov = 15 * ygl::pif / 180;
		scn.camera.aperture = 0.00f;

		scn.id = "cornell";


		auto material_emissive = scn.add_material("material_emissive");
		material_emissive->e_temp = 6500.0f;
		material_emissive->e_power = 100.0f;

		auto diff_red = scn.add_material("diff_red");
		diff_red->kr = { 0.4f,0.0f,0.0f };

		auto diff_green = scn.add_material("diff_green");
		diff_green->kr = { 0.0f,0.4f,0.0f };

		auto diff_white = scn.add_material("diff_white");
		diff_white->kr = { 0.8f,0.8f,0.8f };
		//diff_white->ior_type = non_wl_dependant;
        //diff_white->ior = 1.2f;

		auto carbon_diamond_material = scn.add_material("diamond_material");
        carbon_diamond_material->ka = vec3f{0.01f,0.01f,0.01f};
        carbon_diamond_material->sm_b1 = 0.3306f;
        carbon_diamond_material->sm_c1 = 0.1750f;
        carbon_diamond_material->sm_b2 = 4.3356f;
        carbon_diamond_material->sm_c2 = 0.1060f;
        carbon_diamond_material->k_sca = 0.0f;
        //carbon_diamond_material->rs = 0.01f;

        auto water_material = scn.add_material("water_material");
        water_material->ka = vec3f{0.61f,0.61f,0.01f};
        water_material->ior_type = non_wl_dependant;
        water_material->ior = 1.370f;
        water_material->k_sca = 0.0f;

        auto borosilicate_glass_material = scn.add_material("borosilicate_glass_material");
        /*borosilicate_glass_material->ka = vec3f{0.51f,0.51f,0.51f};
        borosilicate_glass_material->ior_type = non_wl_dependant;
        borosilicate_glass_material->ior = 1.17f;
        borosilicate_glass_material->k_sca = 0.0f;
        */

		borosilicate_glass_material->sm_b1 = 1.03961212;
		borosilicate_glass_material->sm_b2 = 0.231792344;
		borosilicate_glass_material->sm_b3 = 1.01046945;
		borosilicate_glass_material->sm_c1 = 6.00069867e-3;
		borosilicate_glass_material->sm_c2 = 2.00179144e-2;
		borosilicate_glass_material->sm_c3 = 1.03560653e2;
		borosilicate_glass_material->k_sca = 0.0f;



        auto no_refr_media = scn.add_material("no_refr_media");
        no_refr_media->ka = vec3f{0.41f,0.41f,0.41f};
        no_refr_media->k_sca = 0.0f;

        auto sp_mat2 = scn.add_material("sp_mat2");
        sp_mat2->type = conductor;
        sp_mat2->kr = {0.8f,0.8f,0.8f};
        //sp_mat2->kr = {0.4f,0,0};
        sp_mat2->rs = 0.2f;

        auto sp_mat = scn.add_material("sp_mat");
        sp_mat->type = conductor;
        sp_mat->kr = {1.0f,1.0f,1.0f};
        sp_mat->rs = 0.03f;
        //sp_mat->rs = 0.1f;

        auto cornell_composite_mat = scn.add_material("cornell_composite_mat");
        cornell_composite_mat->kr = {0.4f,0.4f,0.4f};
		auto mtor_e = [](ygl::rng_state& rng, const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
           if(hit.loc_pos.x<=-3.945f){mat.kr = {0.0f,0.4f,0.0f};/*mat.ks={1.0f,1.0f,1.0f};*/}
           else if(hit.loc_pos.x>=3.945f){mat.kr = {0.4f,0.0f,0.0f};/*mat.ks={1.0f,1.0f,1.0f};*/}
           else if(hit.loc_pos.z<-3.9f){
                if(on_pattern_gradient_oblique(hit.loc_pos,45,5)){
                    mat.kr = zero3f;
                }
           }
		};
        cornell_composite_mat->mutator = mtor_e;

		auto root = new vop_union("root", {
            /*
			new vvo_sd_box("left_wall",diff_green,{ 0.1f,5.0f,5.0f }),
			new vvo_sd_box("right_wall",diff_red,{ 0.1f,5.0f,5.0f }),
			new vvo_sd_box("rear_wall",diff_white,{ 5.0f,5.0f,0.1f }),
			new vvo_sd_box("floor",diff_white,{ 5.0f,0.1f,5.0f }),
			new vop_subtraction("ceil_light",{
				new vvo_sd_box("ceil",diff_white,{5.0f,0.1f,5.0f}),
				new vvo_sd_box("light_sub",diff_white,{ 1.5f,0.5f,1.5f }),
			}),
            */
            new vop_union("cornell_union",0.0f,{
                new vop_subtraction("cornell",0.0f,{
                    new vvo_sd_box("box",cornell_composite_mat,4.0f),
                    new vvo_sd_box("box_subtr",cornell_composite_mat,3.95f),
                    new vvo_sd_box("box_subtr_ceil",cornell_composite_mat,{1.0f,0.5f,1.0f}),
                }),
                new vvo_sd_box("light",material_emissive,{ 1.0f,0.1f,1.0f }),
                //new vvo_sd_sphere("light",material_emissive,0.3f),
                new vop_union("glass",{
                    new vop_subtraction("glass_sub",0.1f,{
                        new vvo_sd_cylinder("outer",borosilicate_glass_material,{1.3f,2.3f}),
                        new vvo_sd_cylinder("inner",borosilicate_glass_material,{1.00f,2.2f})
                    }),
                    new vvo_sd_cylinder("inner_water",water_material,{0.98f,1.0f}),
                    new vvo_sd_box("obs",sp_mat2,{0.1f,3.0f,0.1f}),
                }),

                //new vvo_sd_sphere("sph",borosilicate_glass_material,1.0f),
                //new vvo_sd_pyramid4("sph",borosilicate_glass_material,{1,1,2.5}),
                new vvo_sd_box("mirror",sp_mat,{3.0f,3.0f,0.1f}),
                new vvo_sd_sphere("sph",diff_white),
            }),

		});

		scn.set_root(root);

		scn.set_translation("box_subtr",{0,0,0.1f});
        scn.set_translation("box_subtr_ceil",{0,3.8f,0.1f});
        scn.set_translation("light", { 0.0f,3.95f,0 });

        scn.set_translation("glass",0,-1.8f,0);
        scn.set_translation("inner",0,0.6f,0);
        scn.set_translation("inner_water",0,-1.2f,0);

        scn.set_translation("obs",{-0.6f,1.3f,0});
        scn.set_rotation_degs("obs",{0,0,20});

        scn.set_translation("sph",{2.2f,-3.0f,-2.3f});
        scn.set_rotation_degs("sph",{45,45,45});

        scn.set_translation("mirror",{-2.8f,0,-2.8f});
        scn.set_rotation_degs("mirror",{0,45,0});

        /*
		scn.set_translation("left_wall", { 5.0f,0,0 });
		scn.set_translation("right_wall", { -5.0f,0,0 });
		scn.set_translation("rear_wall", { 0.0f,0,-5.0f });
        scn.set_translation("light", { 0.0f,4.9f,0 });
		scn.set_translation("ceil_light", { 0.0f,5.0f,0 });
		scn.set_translation("floor", { 0,-5.0f,0 });
		*/

        auto ftor = [](const vec3f& p){
            const float f = 1.0f;
            const float fd = f*3;
            return (sin(f*p.x)*sin(f*p.y)*sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
        };
        scn.set_displacement("el_tw",ftor);


	}

}

#endif
