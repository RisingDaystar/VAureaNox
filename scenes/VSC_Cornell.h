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

#ifndef _VSC_CORNELL_
#define _VSC_CORNELL_

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_cornell_scene(vnx::VScene& scn) {
        scn.camera.yfov = radians(45);
        scn.camera.mOrigin = {0,0,14.0f};
        scn.camera.mTarget = {0,0,0};
        scn.camera.mUp = {0,1.0f,0};
		scn.id = "cornell";


		auto material_emissive = scn.add_material("material_emissive");
		material_emissive->e_temp = 6500.0f;
		material_emissive->e_power = 200.0f;

		auto diff_red = scn.add_material("diff_red");
		diff_red->kr = { 0.4f,0.0f,0.0f };

		auto diff_green = scn.add_material("diff_green");
		diff_green->kr = { 0.0f,0.4f,0.0f };

		auto diff_white = scn.add_material("diff_white");
		diff_white->kr = { 0.8f,0.8f,0.8f };
		diff_white->rs = 0.5f;

		auto diff_white_lowks = scn.add_material("diff_white_lowks");
		diff_white_lowks->kr = {0.1f,0.1f,0.1f};
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
        water_material->ka = vec3f{0.5f,0.5,0.01f};
        water_material->ior_type = non_wl_dependant;
        water_material->ior = 1.370f;
        water_material->k_sca = 0.0f;

        auto highior_material = scn.add_material("highior_material");
        highior_material->ka = vec3f{0.0f,0.0f,0.0f};
        highior_material->ior_type = non_wl_dependant;
        highior_material->ior = 2.770f;
        highior_material->k_sca = 0.0f;

        auto partecipating = scn.add_material("partecipating");
        partecipating->ka = {0.8f,0.8f,0.8f};
        partecipating->k_sca = 0.08f;

        auto borosilicate_glass_material = scn.add_material("borosilicate_glass_material");
        /*borosilicate_glass_material->ka = vec3f{0.51f,0.51f,0.51f};
        borosilicate_glass_material->ior_type = non_wl_dependant;
        borosilicate_glass_material->ior = 1.17f;
        borosilicate_glass_material->k_sca = 0.0f;
        */
        borosilicate_glass_material->ka = {0.0f,0.0f,0.0f};
		borosilicate_glass_material->sm_b1 = 1.03961212;
		borosilicate_glass_material->sm_b2 = 0.231792344;
		borosilicate_glass_material->sm_b3 = 1.01046945;
		borosilicate_glass_material->sm_c1 = 6.00069867e-3;
		borosilicate_glass_material->sm_c2 = 2.00179144e-2;
		borosilicate_glass_material->sm_c3 = 1.03560653e2;
		borosilicate_glass_material->k_sca = 0.0f;
        //borosilicate_glass_material->rs = 0.05f;


        auto no_refr_media = scn.add_material("no_refr_media");
        no_refr_media->ka = vec3f{0.41f,0.41f,0.41f};
        no_refr_media->k_sca = 0.0f;

        auto sp_mat2 = scn.add_material("sp_mat2");
        sp_mat2->kr = {0.8f,0.8f,0.8f};

        auto sp_mat = scn.add_material("sp_mat");
        sp_mat->type = conductor;
        sp_mat->kr = {1.0f,1.0f,1.0f};
        sp_mat->rs = 0.03f;
        //sp_mat->rs = 0.1f;

        auto cornell_composite_mat = scn.add_material("cornell_composite_mat");
        cornell_composite_mat->kr = {0.4f,0.4f,0.4f};
        cornell_composite_mat->rs = 0.5f;
		auto mtor_e = [](ygl::rng_state& rng, const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
           if(hit.loc_pos.x<=-3.945f){mat.kr = {0.0f,0.4f,0.0f};/*mat.ks={1.0f,1.0f,1.0f};*/}
           else if(hit.loc_pos.x>=3.945f){mat.kr = {0.4f,0.0f,0.0f};/*mat.ks={1.0f,1.0f,1.0f};*/}
           else if(hit.loc_pos.z<-3.9f){
                if(on_pattern_gradient_oblique(hit.loc_pos,45,5)){
                    mat.kr = one3f; //zero3f
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
                    new vvo_sd_box("box_subtr_ceil",cornell_composite_mat,{1.0f,0.08f,1.0f}),
                }),
                new vvo_sd_box("light",material_emissive,{ 0.8f,0.03f,0.8f }),
                //new vvo_sd_sphere("light",material_emissive,0.4f),
                //new vvo_sd_box("occlusion",diff_white_lowks,{2.0f,0.1f,5.0f}),
                //new vvo_sd_sphere("glass_ball",borosilicate_glass_material,2.5f),


                /*new vop_union("glass",{
                    new vop_cut("glass_cylinder",{0,1,0},{0,-1.8f,0},0.0f,new vop_onion("_gc_eo",0.15f,new vvo_sd_cylinder("_gc_e",borosilicate_glass_material,{1.3f,2.3f}))),
                    new vvo_sd_cylinder("inner_water",water_material,{0.99f,1.0f}),
                    new vop_twist("obs",Z,0.8f,new vvo_sd_box("obs_t",diff_white,{0.1f,3.0f,0.1f})),
                }),*/



                //new vop_cut("mirror",{0,1,1},{0,0,0},0.1f,new vop_onion("mirror_o",0.06f,new vvo_sd_sphere("mirror_i",sp_mat))),

                /*
                new vop_union("sph",{
                    new vvo_sd_sphere("sph_s",borosilicate_glass_material),
                    new vvo_sd_sphere("sph_si",diff_white,0.4f),
                }),
                */


                //new vvo_sd_box("box1",diff_white,{1.0f,3.0f,1.0f}),
                new vvo_sd_box("box_partecipating",partecipating,1.5f),
                //new vvo_sd_box("box_occ",diff_white,{5.5,0.1f,5.5f}),
                //new vvo_sd_hex_prism("sph",borosilicate_glass_material,one2f),
                //new vvo_sd_tri_prism("box_tr2",borosilicate_glass_material,{1.0f,2.0f}),
                new vvo_sd_diamond("box_tr",carbon_diamond_material),


                new vop_cut("mirror",{0,1,1},{0,0,0},0.1f,new vop_onion("mirror_o",0.06f,new vvo_sd_sphere("mirror_i",sp_mat))),
                new vop_union("box_blend",0.2f,{
                    new vvo_sd_box("box2",diff_white,0.5f),
                    new vvo_sd_box("box3",diff_white,0.45f),
                    new vvo_sd_box("box4",diff_white,0.4f),
                }),


            }),

		});

		scn.set_root(root);

		scn.set_translation("box1",{-2.0f,-2.0f,-2.0f});
		scn.set_translation("box_tr",{-1.0f,-1.8f,1.0f});
		scn.set_scale("box_tr",2.0f);
		scn.set_translation("box_blend",{2.0f,-3.5f,0.7f});
		scn.set_translation("box2",{0,0.0f,0});
		scn.set_translation("box3",{0,0.6f,0});
		scn.set_translation("box4",{0,1.0f,0});

		scn.set_translation("box_occ",{0,2.2f,0.0f});

		scn.set_rotation_degs("box_tr",{180,45,45});
		scn.set_rotation_degs("box_partecipating",{0,0,0});

		scn.set_rotation_degs("box1",{0,45,0});
		scn.set_rotation_degs("box2",{0,45,0});
		scn.set_rotation_degs("box3",{0,60,0});
		scn.set_rotation_degs("box4",{0,80,0});

		scn.set_translation("box_subtr",{0,0,0.1f});
        scn.set_translation("box_subtr_ceil",{0,3.9f,0.1f});
        scn.set_translation("light", { 0.0f,4.00f,0 });
        //scn.set_translation("light", { 0.0f,3.50f,0 });
        scn.set_translation("occlusion",{0,2.9f,0});
        scn.set_translation("glass_ball",{1.5f,0,0.0f});

        scn.set_translation("glass",0,-1.8f,0);
        scn.set_translation("inner",0,0.6f,0);
        scn.set_translation("inner_water",0,-2.0f,0);

        scn.set_translation("obs",{-0.4f,1.1f,0});
        scn.set_rotation_degs("obs",{0,45,20});

        scn.set_translation("sph",{0,-2.0f,0});
        scn.set_rotation_degs("sph",{45,45,45});

        scn.set_translation("mirror",{2.8f,0,-2.8f});
        scn.set_rotation_degs("mirror",{0,-45,0});



        /*
		scn.set_translation("left_wall", { 5.0f,0,0 });
		scn.set_translation("right_wall", { -5.0f,0,0 });
		scn.set_translation("rear_wall", { 0.0f,0,-5.0f });
        scn.set_translation("light", { 0.0f,4.9f,0 });
		scn.set_translation("ceil_light", { 0.0f,5.0f,0 });
		scn.set_translation("floor", { 0,-5.0f,0 });
		*/

        auto ftor = [](const vec3f& p){
            const float f = 6.0f;
            const float fd = f*1;
            return (sin(f*p.x)*sin(f*p.y)*sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
        };
        //scn.set_displacement("inner_water",ftor);


	}

}

#endif
