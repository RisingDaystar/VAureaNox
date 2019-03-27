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


#ifndef VSC_GI_TEST_H_INCLUDED
#define VSC_GI_TEST_H_INCLUDED

#include "../VVolumes.h"
#include "../VOperators.h"
using namespace vnx;

namespace vscndef {

	void init_gi_test_scene(vnx::VScene& scn) {
        scn.camera.yfov = radians(45);
        scn.camera.mOrigin = {0,5,20.0};
        scn.camera.mTarget = {0,0,0};
        scn.camera.mUp = {0,1.0,0};
		scn.id = "gi_test";

        auto emissive = scn.add_material("emissive");
        emissive->e_temp = 6500; //orig : 15
        emissive->e_power = 18800; //orig : 15
        auto emissive_dim = scn.add_material("emissive_dim");
        emissive_dim->e_temp = 6500; //orig : 15
        emissive_dim->e_power = 880; //orig : 15

        auto diffuse_red_mat = scn.add_material("diffuse_red_mat");
        diffuse_red_mat->kr = {0.4,0.0,0.0};

		auto carbon_diamond_material = scn.add_material("diamond_material");
        carbon_diamond_material->ka = vec3vf{0.01,0.01,0.01};
        carbon_diamond_material->sm_b1 = 0.3306;
        carbon_diamond_material->sm_c1 = 0.1750;
        carbon_diamond_material->sm_b2 = 4.3356;
        carbon_diamond_material->sm_c2 = 0.1060;
        carbon_diamond_material->k_sca = 0.0;

		auto partecipating_test = scn.add_material("partecipating_test");
        partecipating_test->ka = {0.81,0.81,0.51};
        //partecipating_test->rs = 0.3f;
        partecipating_test->k_sca = 0.0f;
		partecipating_test->sm_b1 = 1.03961212;
		partecipating_test->sm_b2 = 0.231792344;
		partecipating_test->sm_b3 = 1.01046945;
		partecipating_test->sm_c1 = 6.00069867e-3;
		partecipating_test->sm_c2 = 2.00179144e-2;
		partecipating_test->sm_c3 = 1.03560653e2;

        auto diffuse_exp = scn.add_material("diffuse_exp");
        diffuse_exp->ior_type = non_wl_dependant;
        diffuse_exp->ior = 1.4;
        diffuse_exp->kr = {0.6,0.0f,0.0};
        diffuse_exp->rs = 0.15;
		auto mtor_e = [](ygl::rng_state& rng, const VResult& hit,const vec3vf& n, VMaterial& mat) {
           if(dot(hit.wor_pos,hit.loc_pos)>0){mat.kr = {0.1,0.1,0.1};}
		};
        diffuse_exp->mutator = mtor_e;

        auto diffuse_mat = scn.add_material("diffuse");
        diffuse_mat->ior_type = non_wl_dependant;
        diffuse_mat->ior = 1.8;
        diffuse_mat->kr = {0.8,0.8,0.8};
        diffuse_mat->rs = 0.15;
		auto mtor = [](ygl::rng_state& rng, const VResult& hit,const vec3vf& n, VMaterial& mat) {
			if (std::abs(sin(hit.loc_pos.x)) < 0.03 || std::abs(sin(hit.loc_pos.z)) < 0.03) {
                    mat.type = conductor;
                    mat.kr = {0.4,0.4,0.5};
                    mat.rs = 0.9;
            }
		};
		diffuse_mat->mutator = mtor;

		auto diffuse_pure = scn.add_material("diffuse_pure");
		diffuse_pure->kr = {0,1.0,0};
		diffuse_pure->rs = 0.9;

		auto diffuse_sand = scn.add_material("diffuse_sand");
		diffuse_sand->kr = {0.7,0.6,0.4};
		diffuse_sand->ior_type = non_wl_dependant;
		diffuse_sand->ior = 1.7;
		diffuse_sand->rs = 0.5;

        auto steel = scn.add_material("steel");
        steel->type = conductor;
        steel->ior_type = non_wl_dependant;
        steel->kr = {0.8,0.8,0.9};
        steel->rs = 0.05;
        steel->ior = 2.6;

        auto sp_mat = scn.add_material("sp_mat");
        sp_mat->type = dielectric;
        sp_mat->kr = {0.2,0.5,0.8};
        sp_mat->ior_type = non_wl_dependant;
        sp_mat->ior = 2.2;

		auto ring = new vop_twist("ring", X, 0.9f, new vop_subtraction("ring_s",1.0,{
                new vvo_sd_cylinder("ring_e_ex",diffuse_exp,{4.0,0.5}),
                new vvo_sd_cylinder("ring_e_in",diffuse_exp,{3.0,1.0}),
              })
        );

		scn.root = new vop_union("root",0.0,{
            new vop_union("p_b_blend",0.0,{ //0.8f
                new vop_subtraction("plane_sub",0.5,{
                    new vvo_sd_plane("plane",diffuse_mat),
                    new vop_twist("plane_sub_t",Y,1.9,
                        new vvo_sd_ellipsoid("plane_sub_e",diffuse_sand,{7.5,3.5,4.5})
                    ),
                }),
                ring,
                new vop_union("sph_group",0.0,{
                    //new vvo_sd_sphere("coat",partecipating_test,1.5f),
                    new vvo_sd_sphere("sph",steel,0.5),
                }),
            }),

           new vop_cut("sph2_group",{1,1,1},zero3vf,0.1,{
              //new vvo_sd_sphere("coat2",partecipating_test,1.8f),
              new vop_onion("sph2_o",0.2f,new vvo_sd_sphere("sph2",steel,3.5)),
           }),
           new vvo_sd_sphere("light",emissive,80.5),
        });

        scn.set_translation("sph2_group",{2,2,10});
        scn.set_translation("light",{0,180,0});

        auto ftor = [](const vec3vf& p){
            const vfloat f = 6.0;
            const vfloat fd = f*1;
            return (std::sin(f*p.x)*std::sin(f*p.y)*std::sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
        };
        scn.set_displacement("ring",ftor);

        auto ftor3 = [](const vec3vf& p){
            const vfloat f = 1;
            const vfloat fd = f*1;
            return (std::sin(f*p.x)*std::sin(f*p.y)*std::sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
        };
        scn.set_displacement("plane_sub_t",ftor3);
        scn.set_scale("ring",{1.2,1.0,1.2});

        //scn.set_scale("coat",{3.0f,3.0f,3.0f});
        scn.set_rotation_degs("sph2_group",{25,25,25});

	}

};

#endif // VSC_GI_TEST_H_INCLUDED
