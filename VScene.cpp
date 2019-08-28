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

#include "VScene.hpp"

namespace vnx{

    void VScene::shut() {
        if (root) delete root;
        materials.clear();
    }

    VMaterial* VScene::add_material(std::string id) {
        if (materials.find(id) == materials.end()) {
            materials[id] = VMaterial();
        }
        return &materials[id];
    }
    VMaterial* VScene::add_material(std::string id,VMaterial mtl) {
        if (materials.find(id) == materials.end()) {
            materials[id] = mtl;
        }
        return &materials[id];
    }

    bool VScene::set_translation(const std::string& idv, const vec3d& amount) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_translation(amount);
        return true;
    }
    bool VScene::set_translation(const std::string& idv,double x,double y,double z) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_translation(x,y,z);
        return true;
    }

    bool VScene::set_rotation(std::string n, const vec3d& amount) {
        auto node = select(n);
        if (node == nullptr) { return false; }
        node->set_rotation(amount);
        return true;
    }
    bool VScene::set_rotation(std::string n,double x,double y,double z) {
        auto node = select(n);
        if (node == nullptr) { return false; }
        node->set_rotation(x,y,z);
        return true;
    }

    bool VScene::set_rotation_degs(const std::string& idv, const vec3d& amount) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_rotation_degs(amount);
        return true;
    }
    bool VScene::set_rotation_degs(const std::string& idv,double x,double y,double z) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_rotation_degs(x,y,z);
        return true;
    }

    bool VScene::set_rotation_order(const std::string& idv, VRotationOrder ro){
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_rotation_order(ro);
        return true;
    }

    bool VScene::set_scale(const std::string& idv, const vec3d& amount) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_scale(amount);
        return true;
    }
    bool VScene::set_scale(const std::string& idv,double x,double y,double z) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_scale(x,y,z);
        return true;
    }
    bool VScene::set_scale(const std::string& idv, double amount) {
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_scale(vec3d{amount,amount,amount});
        return true;
    }

    bool VScene::set_rounding(const std::string& idv,double r){
        auto node = select(idv);
        if (node == nullptr) { return false; }
        node->set_rounding(r);
        return true;
    }

    bool VScene::set_displacement(const std::string idv,displ_ftor ftor){
        auto node = select(idv);
        if (!node) { return false; }
        node->mDisplacement = ftor;
        return true;
    }

    vec3i VScene::precalc_emissive_hints_full(ygl::rng_state& rng, std::map<std::string, std::vector<VResult>>& emap,VNode* ptr,int n_em_e,int n_max_iters,double tmin,double tmax,double neps,bool verbose,frame3d parent_frame) {
        if (!ptr) { return {0,0,0}; }
        vec3i stats = {0,0,0};

        auto fr = calculate_frame(ptr,parent_frame);

        if(!ptr->isOperator()){
            if(verbose)std::cout<<"::Testing Volume "<<ptr->id;
            VResult vre;
            eval(transform_point(fr, zero3d),vre);

            //OPTIMIZATION , if assigned material is not emissive and does not have a mutator , ignore.
            //WARNING, might need a check towards casted ptr, better than having to rely on isOperator();
            {
              const auto sdf_ptr = static_cast<VSdf*>(ptr);
              if(!sdf_ptr || !sdf_ptr->mMaterial ||
               (!sdf_ptr->mMaterial->is_emissive()  && !sdf_ptr->mMaterial->mutator)){
                std::cout<<" ->Ignored\n";
                goto ignore;
                }
            }


            //N.B: tests any possible volume , searching for emissive materials (which can happen even if assigned material is not
            //homogeneously emissive, because of procedural mutated materials)

            if(vre.sur && vre.mtl ){ //allows for starting outside
                vre._found = true;
                vec3d norm = eval_normals(vre, neps);

                VMaterial vre_material;
                vre.getMaterial(vre_material);
                if(vre_material.mutator!=nullptr){
                    vre_material.eval_mutator(rng, vre, norm, vre_material);
                };
                VMaterial vre_vmaterial;
                vre.getVMaterial(vre_vmaterial);
                if(vre_vmaterial.mutator!=nullptr){
                    vre_vmaterial.eval_mutator(rng, vre, norm, vre_vmaterial);
                };

                    if(vre_vmaterial.is_emissive() && vre.vsur){
                        std::vector<VResult>* epoints = &emap[vre.vsur->id];
                        vre._found = true;
                        epoints->push_back(vre); stats.x++;
                        //if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                    }else if(vre_material.is_emissive() && vre.sur){
                        std::vector<VResult>* epoints = &emap[vre.sur->id];
                        vre._found = true;
                        epoints->push_back(vre); stats.x++;
                        //if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                    }

                    auto dir = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                    auto ray = VRay{vre.wor_pos,dir,tmin,tmax};
                    VResult fvre = vre;
                    for (int i=0; i < n_em_e; i++) {
                        if(i%(n_em_e/10)==0 && verbose) std::cout<<".";
                        vre = intersect(ray,n_max_iters);
                        //if(vre.vdist>0.0) std::cout<<"OLO: "<<i<<"\n";

                        if(!vre.isFound() || !vre.isValid()){
                            dir = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                            //auto rn_nor = ygl::get_random_vec3f(rng);
                            //vec3d rn_nor_off = {rn_nor.x,rn_nor.y,rn_nor.z};
                            ray = offsetted_ray(fvre.wor_pos,{},dir,tmin,tmax,dir,tmin);
                            vre = intersect(ray,n_max_iters);
                            stats.y++;
                            if(!vre.isValid())continue;
                        }else if(ygl::get_random_float(rng)>0.95){
                            dir = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                            //auto rn_nor = ygl::get_random_vec3f(rng);
                            //vec3d rn_nor_off = {rn_nor.x,rn_nor.y,rn_nor.z};
                            ray = offsetted_ray(fvre.wor_pos,{},dir,tmin,tmax,dir,tmin);
                            vre = intersect(ray,n_max_iters);
                            n_em_e++; //reduce math bias
                            stats.z++;
                        }

                        auto norm = eval_normals(vre, neps);
                        VMaterial er_material;
                        vre.getMaterial(er_material);
                        if(er_material.mutator!=nullptr){
                            er_material.eval_mutator(rng, vre, norm, er_material);
                        }
                        VMaterial er_vmaterial;
                        vre.getVMaterial(er_vmaterial);
                        if(er_vmaterial.mutator!=nullptr){
                            er_vmaterial.eval_mutator(rng, vre, norm, er_vmaterial);
                        }

                        frame3d fp;
                        vec3d offalong;
                        bool straight = false;
                        if(dot(ray.d,norm)>0.0){
                            fp = ygl::make_frame_fromz(zero3d, -norm);
                            offalong = -norm;
                        }else{
                            if(vre.vdist<0.0) {
                                if(er_material.is_emissive()) {
                                    fp = ygl::make_frame_fromz(zero3d, -norm);
                                    offalong = -norm;
                                }else if(vre.dist<0.0){
                                    if(er_material.mutator && ygl::get_random_float(rng)<0.95){
                                        fp = ygl::make_frame_fromz(zero3d, -norm);
                                        offalong = -norm;
                                    }else{
                                        //fp = ygl::make_frame_fromz(zero3d, norm);
                                        straight = true;
                                        offalong = norm;
                                    }
                                }else {
                                    fp = ygl::make_frame_fromz(zero3d, norm);
                                    offalong = norm;
                                }
                            }else{
                                fp = ygl::make_frame_fromz(zero3d, -norm);
                                offalong = -norm;
                            }
                        }
                        //auto fp = dot(ray.d,norm) > 0.0 ? ygl::make_frame_fromz(zero3d, -norm) : ygl::make_frame_fromz(zero3d, norm); // TEST -norm && norm
                        if(!straight)dir = ygl::transform_direction(fp,sample_hemisphere_direction<double>(ygl::get_random_vec2f(rng)));
                        else dir = ray.d;
                        //if(ygl::get_random_float(rng)>0.5)dir = ygl::reflect(-ray.d,dir);
                        ray = offsetted_ray(vre.wor_pos,{},dir,tmin,tmax,offalong,vre.dist); //TEST -norm && norm


                        //if(vre.dist<0.0f && vre.vdist<0.0f){
                        //AVOID EXTERNAL EM POINTS (vdist>0.0)
                            /*if (er_material.is_emissive()) {
                                std::vector<VResult>* epoints = &emap[vre.sur->id];
                                vre._found = true;
                                epoints->push_back(vre); stats.x++;
                                //if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                            }else */
                            if(er_vmaterial.is_emissive() && vre.vsur){
                                std::vector<VResult>* epoints = &emap[vre.vsur->id];
                                vre._found = true;
                                epoints->push_back(vre); stats.x++;
                                //if(verbose) std::cout<<"EM ( light: "<<vre.vsur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                            }else if(er_material.is_emissive() && vre.sur){
                                std::vector<VResult>* epoints = &emap[vre.sur->id];
                                vre._found = true;
                                epoints->push_back(vre); stats.x++;
                                //if(verbose) std::cout<<"EM ( light: "<<vre.vsur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                            }
                        //}
                    }
                //}
            }
            if(!stats.x){stats.y=0;stats.z=0;} //if no emissive found, ignore other stats
            if(verbose)std::cout<<"->Found: "<<stats.x<<"/("<<stats.y<<"/"<<stats.z<<")\n";
        }else{
            if(verbose)std::cout<<"::Parsing Operator "<<ptr->id<<"\n";
        }

        ignore:
        auto chs = ptr->get_childs();
        if (chs.empty()) { return stats; }
        for (auto node : chs) {
            stats+=precalc_emissive_hints(rng, emap, node, n_em_e, n_max_iters, tmin, tmax, neps, verbose,fr);
        }

        return stats;
    }



    vec3i VScene::precalc_emissive_hints_strict(ygl::rng_state& rng, std::map<std::string, std::vector<VResult>>& emap,VNode* ptr,int n_em_e,int n_max_iters,double tmin,double tmax,double neps,bool verbose,frame3d parent_frame) {
       if (!ptr) { return {0,0,0}; }
        vec3i stats = {0,0,0};

        auto fr = calculate_frame(ptr,parent_frame);

        if(!ptr->isOperator()){
            if(verbose)std::cout<<"::Testing Volume "<<ptr->id;
            VResult vre;
            eval(transform_point(fr, zero3d),vre);

            //N.B: Searchs only inside "emissive material declared" volumes

            if(vre.vdist<0.0 && vre.sur && vre.vsur){
                vre._found = true;
                VMaterial vre_material;
                vre.getMaterial(vre_material);
                vec3d norm = eval_normals(vre, neps);
                if(vre_material.mutator!=nullptr){
                    vre_material.eval_mutator(rng, vre, norm, vre_material);
                };
                VMaterial vre_vmaterial;
                vre.getVMaterial(vre_vmaterial);
                if(vre_vmaterial.mutator!=nullptr){
                    vre_vmaterial.eval_mutator(rng, vre, norm, vre_vmaterial);
                };

                if(vre_vmaterial.is_emissive() || vre_material.is_emissive()){
                    std::vector<VResult>* epoints = &emap[vre.sur->id];
                    vre._found = true;
                    epoints->push_back(vre); stats.x++;
                        //if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";


                    auto dir = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                    auto ray = VRay{vre.wor_pos,dir,tmin,tmax};
                    VResult fvre = vre;
                    for (int i=0; i < n_em_e; i++) {
                        if(i%(n_em_e/10)==0 && verbose) std::cout<<".";
                        vre = intersect(ray,n_max_iters);
                        //if(vre.vdist>0.0) std::cout<<"OLO: "<<i<<"\n";

                        if(!vre.isFound()){
                            dir = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                            ray = offsetted_ray(fvre.wor_pos,{},dir,tmin,tmax,zero3d,0.0);
                            vre = intersect(ray,n_max_iters);
                            stats.y++;
                        }else if(ygl::get_random_float(rng)>0.95){
                            dir = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                            ray = offsetted_ray(fvre.wor_pos,{},dir,tmin,tmax,zero3d,0.0);
                            vre = intersect(ray,n_max_iters);
                            n_em_e++; //reduce math bias
                            stats.z++;
                        }

                        auto norm = eval_normals(vre, neps);
                        VMaterial er_material;
                        vre.getMaterial(er_material);
                        if(er_material.mutator!=nullptr){
                            er_material.eval_mutator(rng, vre, norm, er_material);
                        }
                        VMaterial er_vmaterial;
                        vre.getVMaterial(er_vmaterial);
                        if(er_vmaterial.mutator!=nullptr){
                            er_vmaterial.eval_mutator(rng, vre, norm, er_vmaterial);
                        }

                        frame3d fp;
                        vec3d offalong;
                        if(dot(ray.d,norm)>0.0){
                            fp = ygl::make_frame_fromz(zero3d, -norm);
                            offalong = -norm;
                        }else{
                            if(vre.vdist<0.0) {
                                if(er_material.is_emissive() || vre.dist<0.0) {
                                    fp = ygl::make_frame_fromz(zero3d, -norm);
                                    offalong = -norm;
                                }else {
                                    fp = ygl::make_frame_fromz(zero3d, norm);
                                    offalong = norm;
                                }
                            }else{
                                fp = ygl::make_frame_fromz(zero3d, -norm);
                                offalong = -norm;
                            }
                        }
                        //auto fp = dot(ray.d,norm) > 0.0 ? ygl::make_frame_fromz(zero3d, -norm) : ygl::make_frame_fromz(zero3d, norm); // TEST -norm && norm
                        dir = ygl::transform_direction(fp,sample_hemisphere_direction<double>(ygl::get_random_vec2f(rng)));

                        //if(ygl::get_random_float(rng)>0.5)dir = ygl::reflect(-ray.d,dir);
                        ray = offsetted_ray(vre.wor_pos,{},dir,tmin,tmax,offalong,vre.dist); //TEST -norm && norm


                        //if(vre.dist<0.0f && vre.vdist<0.0f){
                        //AVOID EXTERNAL EM POINTS (vdist>0.0)
                            /*if (er_material.is_emissive()) {
                                std::vector<VResult>* epoints = &emap[vre.sur->id];
                                vre._found = true;
                                epoints->push_back(vre); stats.x++;
                                if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                            }else */
                            if(er_vmaterial.is_emissive()){
                                std::vector<VResult>* epoints = &emap[vre.vsur->id];
                                vre._found = true;
                                epoints->push_back(vre); stats.x++;
                                //if(verbose) std::cout<<"EM ( light: "<<vre.vsur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                            }
                        //}
                    }
                }
            }
            if(!stats.x){stats.y=0;stats.z=0;} //if no emissive found, ignore other stats
            if(verbose)std::cout<<"->Found: "<<stats.x<<"/("<<stats.y<<"/"<<stats.z<<")\n";
        }else{
            if(verbose)std::cout<<"::Parsing Operator "<<ptr->id<<"\n";
        }
        auto chs = ptr->get_childs();
        if (chs.empty()) { return stats; }
        for (auto node : chs) {
            stats+=precalc_emissive_hints(rng, emap, node, n_em_e, n_max_iters, tmin, tmax, neps, verbose,fr);
        }

        return stats;
    }

    vec3i VScene::populate_emissive_hints(int i_em_evals,int i_max_march_iterations,double f_ray_tmin,double f_ray_tmax,double f_normal_eps,bool verbose) {
        auto rng = ygl::make_rng(ygl::get_time());
        std::map<std::string, std::vector<VResult>> tmpMap;
        vec3i stats = precalc_emissive_hints(rng, tmpMap, root, i_em_evals, i_max_march_iterations, f_ray_tmin, f_ray_tmax, f_normal_eps, verbose, identity_frame3d);
        for (auto ceh : tmpMap) {
            emissive_hints.push_back(ceh.second);
        }
        return stats;
    }

};
