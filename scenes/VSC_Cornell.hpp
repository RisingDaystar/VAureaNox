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

#ifndef _VSC_CORNELL_
#define _VSC_CORNELL_

#include "../VSdfs.hpp"
#include "../VSdfOperators.hpp"
#include "../VScene.hpp"

using namespace vnx;

namespace vscndef {

	void init_cornell_scene(vnx::VScene& scn) {
        scn.camera.mYfov = radians(45.0);
        scn.camera.mOrigin = {0,0.0,14.0};
        scn.camera.mTarget = {0,0,0};
        scn.camera.mUp = {0,1.0,0};
        scn.camera.mAperture = 0.0;
        scn.camera.mFocus = length(scn.camera.mTarget-scn.camera.mOrigin);
        scn.camera.mAutoFocus = true;
		scn.id = "cornell";


		auto material_emissive = scn.add_material("material_emissive");
		material_emissive->e_temp = 6500.0;
		material_emissive->e_power = 300.0;

        auto water_material = scn.add_material("water_material",get_material_archetype("water"));
        auto water_material_tinted = scn.add_material("water_material_tinted",get_material_archetype("water"));
        water_material_tinted->ka = {0.01,0.3,1.0};
        auto dispersive_material = scn.add_material("dispersive_material",get_material_archetype("baf10"));
        auto panel_material = scn.add_material("panel_material",get_material_archetype("carbon_diamond"));

        auto dielectric_complex_material = scn.add_material("dielectric_complex_material");
        dielectric_complex_material->type = dielectric;
        dielectric_complex_material->kr = {0.7,0.7,0.7};
        dielectric_complex_material->ior = 1.55;
        dielectric_complex_material->rs = 0.05;
        dielectric_complex_material->mutator = [](ygl::rng_state& rng, const VResult& hit,const vec3d& n, VMaterial& mat) {
            if(!on_pattern_gradient_oblique(hit.loc_pos,-45,4)){
                mat.kr = {0.0,0.0,0.3};
            }
		};;

		auto participating_mat = scn.add_material("participating_mat");
		participating_mat->type = dielectric;
		participating_mat->ka = {0.2,0.2,0.2};
		participating_mat->k_sca = 0.01;


        auto non_dispersive_glass = scn.add_material("non_dispersive_glass");
        non_dispersive_glass->type = dielectric;
        non_dispersive_glass->ior = 1.570;
        non_dispersive_glass->ior_type = non_wl_dependant;
        non_dispersive_glass->ka = zero3d;//{0.1,0.1,0.01};
        non_dispersive_glass->k_sca = 0.0;


        auto diff_white = scn.add_material("diff_white");
        diff_white->type = diffuse;
        diff_white->kr = {0.8,0.8,0.8};

        auto sp_mat = scn.add_material("sp_mat");
        sp_mat->type = conductor;
        sp_mat->kr = {1.0,1.0,1.0};
        sp_mat->rs = 0.15;
        sp_mat->ior = 2.3;
        //sp_mat->rs = 0.1f;

        auto cornell_composite_mat = scn.add_material("cornell_composite_mat");
        cornell_composite_mat->kr = {0.8,0.8,0.8};
		auto mtor_e = [](ygl::rng_state& rng, const VResult& hit,const vec3d& n, VMaterial& mat) {
           if(hit.loc_pos.x<=-3.995){mat.kr = {0.0,0.4,0.0};}
           else if(hit.loc_pos.x>=3.995){mat.kr = {0.4,0.0,0.0};}
           else if(hit.loc_pos.z<-3.995){
                if(!on_pattern_gradient_oblique(hit.loc_pos,45,1)){
                    mat.kr = one3d;
                    mat.rs = 0.03;
                    mat.ior = 2.2;
                    mat.type = conductor;
                }
           }
		};
        cornell_composite_mat->mutator = mtor_e;

		auto root = new vop_union("root", {
            new vop_union("cornell_union",0.0,{
                new vop_subtraction("cornell",0.00,{
                    new vop_cut("cornell_cu",{0,0,1},{0,0,-3.95},0.0,new vop_onion("box",0.1,true,new vvo_sd_box("box_e",cornell_composite_mat,4.2))),
                    new vvo_sd_box("box_subtr_ceil",cornell_composite_mat,{1.0,0.5,1.0}),
                }),
                //new vvo_sd_box("panel",panel_material,{4.0,4.0,0.5}),
                new vvo_sd_box("light",material_emissive,{ 0.1,0.1,0.1 }),

                new vvo_sd_box("fog",participating_mat,{200.0,200.0,200.0}),

                new vvo_sd_sphere("ball_1",diff_white,1.5f),
                new vvo_sd_sphere("ball_2",non_dispersive_glass,1.5),
                new vvo_sd_sphere("ball_3",dispersive_material,1.5),
                //new vvo_sd_sphere("ball_4",dispersive_material,1.5),
                //new vvo_blended("btest",diff_white,new vvo_sd_box("bb1",diff_white),new vvo_sd_sphere("sb1",diff_white),0.5),

                //new vop_twist("distorted_obj",X,0.8,new vvo_sd_ellipsoid("glass_ball3_t",dispersive_material,{0.3,3.0,5.2})),

                /*
                new vop_union("glass",{
                    new vop_union("glass_water_group",{
                        new vop_cut("glass_cylinder",{0,1,0},{0,-2.1,0},0.05,new vop_onion("_gc_eo",0.1,false,new vvo_sd_cylinder("_gc_e",dispersive_material,{1.3,2.3}))),
                        new vvo_sd_cylinder("inner_water",water_material_tinted,{1.18,1.0}),
                    }),
                    new vop_twist("obs",Z,0.8,new vvo_sd_cylinder("obs_t",dielectric_complex_material,{0.1,3.0})),
                    new vvo_sd_box("ice_cube",water_material,0.3),
                }),


                new vop_cut("mirror",{0,1,1},{0,0,0},0.1,new vop_onion("mirror_o",0.06,new vvo_sd_sphere("mirror_i",sp_mat))),


                new vop_union("box_blend",0.2,{
                    new vvo_sd_box("box2",diff_white,0.5),
                    new vvo_sd_box("box3",diff_white,0.45),
                    new vvo_sd_box("box4",diff_white,0.4),
                }),*/


            }),

		});

		scn.set_root(root);

        //VARIOUS INSIDE
        scn.set_translation("ball_1",{1.8,-1.5,-1.5});
        scn.set_translation("ball_2",{-1.8,-1.5,1.5});
        scn.set_translation("ball_3",{2.1,2.2,1.5});
        scn.set_translation("ball_4",{0.0,1.0,0.0});

        scn.set_rotation_degs("ball_4",{45,45,45});

        scn.set_translation("mirror",{2.8,0,-2.8});
        scn.set_rotation_degs("mirror",{0,-45,0});


        //FRONT GLASS PANEL
		scn.set_translation("panel",{0,0.0,4.0});
		scn.set_rounding("panel",0.03);
		//scn.set_rotation_degs("panel",{0,45,0});

        //BOX-ISH OBJECT
		scn.set_translation("box_blend",{2.0,-3.5,0.7});
		scn.set_translation("box2",{0,0.0,0});
		scn.set_translation("box3",{0,0.6,0});
		scn.set_translation("box4",{0,1.0,0});
		scn.set_rotation_degs("box1",{0,45,0});
		scn.set_rotation_degs("box2",{0,45,0});
		scn.set_rotation_degs("box3",{0,60,0});
		scn.set_rotation_degs("box4",{0,80,0});

		//CORNELL
		scn.set_translation("box_subtr",{0,0,0.1});
        scn.set_translation("box_subtr_ceil",{0,3.9,0});
        scn.set_translation("light", { 0.0,4.1,0 });


        //GLASS
        scn.set_translation("glass",0,-1.8,0);
        scn.set_translation("inner",0,0.6,0);
        scn.set_translation("inner_water",0,-0.9,0);
        scn.set_rotation_degs("ice_cube",{45,45,45});
        scn.set_translation("ice_cube",{0.6,0,0.5});
        scn.set_rounding("ice_cube",0.1);
        scn.set_translation("obs",{-0.4,1.1f,0});
        scn.set_rotation_degs("obs",{0,45,20});




	}

}

#endif
