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
        scn.camera.mOrigin = {0,5,20.0f};
        scn.camera.mTarget = {0,0,0};
        scn.camera.mUp = {0,1.0f,0};
		scn.id = "gi_test";

        auto emissive = scn.add_material("emissive");
        emissive->e_temp = 6500; //orig : 15
        emissive->e_power = 18800; //orig : 15
        auto emissive_dim = scn.add_material("emissive_dim");
        emissive_dim->e_temp = 6500; //orig : 15
        emissive_dim->e_power = 880; //orig : 15

        auto diffuse_red_mat = scn.add_material("diffuse_red_mat");
        diffuse_red_mat->kr = {0.4f,0.0f,0.0f};

		auto carbon_diamond_material = scn.add_material("diamond_material");
        carbon_diamond_material->ka = vec3f{0.01f,0.01f,0.01f};
        carbon_diamond_material->sm_b1 = 0.3306f;
        carbon_diamond_material->sm_c1 = 0.1750f;
        carbon_diamond_material->sm_b2 = 4.3356f;
        carbon_diamond_material->sm_c2 = 0.1060f;
        carbon_diamond_material->k_sca = 0.0f;

		auto partecipating_test = scn.add_material("partecipating_test");
        partecipating_test->ka = {0.81f,0.81f,0.51f};
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
        diffuse_exp->ior = 1.4f;
        diffuse_exp->kr = {0.6f,0.0f,0.0f};
        diffuse_exp->rs = 0.15f;
		auto mtor_e = [](ygl::rng_state& rng, const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
           if(dot(hit.wor_pos,hit.loc_pos)>0){mat.kr = {0.1f,0.1f,0.1f};}
		};
        diffuse_exp->mutator = mtor_e;

        auto diffuse_mat = scn.add_material("diffuse");
        diffuse_mat->ior_type = non_wl_dependant;
        diffuse_mat->ior = 1.8f;
        diffuse_mat->kr = {0.8f,0.8f,0.8f};
        diffuse_mat->rs = 0.15f;
		auto mtor = [](ygl::rng_state& rng, const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
			if (std::abs(sin(hit.loc_pos.x)) < 0.03f || std::abs(sin(hit.loc_pos.z)) < 0.03f) {
                    mat.type = conductor;
                    mat.kr = {0.4f,0.4f,0.5f};
                    mat.rs = 0.9f;
            }
		};
		diffuse_mat->mutator = mtor;

		auto diffuse_pure = scn.add_material("diffuse_pure");
		diffuse_pure->kr = {0,1.0f,0};
		diffuse_pure->rs = 0.9f;

		auto diffuse_sand = scn.add_material("diffuse_sand");
		diffuse_sand->kr = {0.7f,0.6f,0.4f};
		diffuse_sand->ior_type = non_wl_dependant;
		diffuse_sand->ior = 1.7f;
		diffuse_sand->rs = 0.5f;

        auto steel = scn.add_material("steel");
        steel->type = conductor;
        steel->ior_type = non_wl_dependant;
        steel->kr = {0.8f,0.8f,0.9f};
        steel->rs = 0.05f;
        steel->ior = 2.6f;

        auto sp_mat = scn.add_material("sp_mat");
        sp_mat->type = dielectric;
        sp_mat->kr = {0.2f,0.5f,0.8f};
        sp_mat->ior_type = non_wl_dependant;
        sp_mat->ior = 2.2f;

		auto ring = new vop_twist("ring", X, 0.9f, new vop_subtraction("ring_s",1.0f,{
                new vvo_sd_cylinder("ring_e_ex",diffuse_exp,{4.0f,0.5f}),
                new vvo_sd_cylinder("ring_e_in",diffuse_exp,{3.0f,1.0f}),
              })
        );

		scn.root = new vop_union("root",{
            new vop_union("p_b_blend",0.0f,{ //0.8f
                new vop_subtraction("plane_sub",0.5f,{
                    new vvo_sd_plane("plane",diffuse_mat),
                    new vop_twist("plane_sub_t",Y,1.9f,
                        new vvo_sd_ellipsoid("plane_sub_e",diffuse_sand,{7.5f,3.5f,4.5f})
                    ),
                }),
                ring,
                new vop_union("sph_group",0.0f,{
                    //new vvo_sd_sphere("coat",partecipating_test,1.5f),
                    new vvo_sd_sphere("sph",steel,0.5f),
                }),
            }),

           new vop_cut("sph2_group",{1,1,1},zero3f,0.1f,{
              //new vvo_sd_sphere("coat2",partecipating_test,1.8f),
              new vop_onion("sph2_o",0.2f,new vvo_sd_sphere("sph2",steel,3.5f)),
           }),
           new vvo_sd_sphere("light",emissive,80.5f),
        });

        scn.set_translation("sph2_group",{2,2,10});
        scn.set_translation("light",{0,180,0});

        auto ftor = [](const vec3f& p){
            const float f = 6.0f;
            const float fd = f*1;
            return (sin(f*p.x)*sin(f*p.y)*sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
        };
        scn.set_displacement("ring",ftor);

        auto ftor3 = [](const vec3f& p){
            const float f = 1;
            const float fd = f*1;
            return (sin(f*p.x)*sin(f*p.y)*sin(f*p.z))/fd; //18= 1/20 + 1/20 + 1/20 ///limiti della funzione sin (e cos...)
        };
        scn.set_displacement("plane_sub_t",ftor3);
        scn.set_scale("ring",{2.0f,1.0f,1.0f});

        //scn.set_scale("coat",{3.0f,3.0f,3.0f});
        scn.set_rotation_degs("sph2_group",{25,25,25});

	}

};

#endif // VSC_GI_TEST_H_INCLUDED
