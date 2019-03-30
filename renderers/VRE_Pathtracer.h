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


#ifndef _VRE_Pathtracer_
#define _VRE_Pathtracer_

#include "../VAureaNox.h"

namespace vnx {

	struct VRE_Pathtracer : VRenderer {

		enum v_sample_type{
            s_undefined,
		    s_reflected,
		    s_tr_reflected,
		    s_transmitted,
		};

		struct VSample{
            VRay ray;
            v_sample_type type = s_undefined;
            vec2vf pdf_factor = one2vf;
		};

		struct VSampleEx{
            VSampleEx(): sample({}),w(zero3vf),ge(true),b(0){};
		    VSampleEx(VSample samplev,vec3vf wv,bool gev,int bv): sample(samplev),w(wv),ge(gev),b(bv){};
            VSample sample;
            vec3vf w;
            bool ge;
            int b;
		};

		struct VVolumePoll{
            VResult eres;
            VMaterial emat;

            bool inline is_abs_inside(){return eres.dist<=epsvf;}

            bool inline is_inside(){return eres.vdist<=epsvf;}
            bool inline is_on_surface(const VRay& ray){return eres.vdist>-ray.tmin && eres.vdist<ray.tmin;}

            bool inline is_on_inside_surface(const VRay& ray){return is_inside() && eres.vdist>-ray.tmin;}
            bool inline is_on_outside_surface(const VRay& ray){return !is_inside() && eres.vdist<ray.tmin;}

            bool inline is_inside_transmissive(){return is_inside() && emat.is_transmissive();}

            bool inline is_on_inside_surface_transmissive(const VRay& ray){return is_on_inside_surface(ray) && emat.is_transmissive();}
            bool inline is_on_outside_surface_transmissive(const VRay& ray){return is_on_outside_surface(ray) && emat.is_transmissive();}
		};

		bool b_do_spectral = true;
		bool b_debug_normals = false;
		bool b_debug_iterations = false;

		int n_ray_samples = 1;
		int n_max_march_iterations = 512;

		vfloat f_ray_tmin = 0.001;
		vfloat f_ray_tmax = 1000.0;
		vfloat f_normal_eps = 0.0001;
		vfloat f_refracted_ray_eps_mult = 2.0;
		vfloat f_min_wl = 400;
		vfloat f_max_wl = 700;

		VRE_Pathtracer(VConfigurable& shared_cfg,std::string cf) : VRenderer(shared_cfg,cf) {} ///TODO inizializzazione stile c++

		inline std::string type() const {return "PathTracer";}

		using VRenderer::try_get; //permette di vedere gli altri overload

		inline void init(){
            parse();

			b_do_spectral = try_get("b_do_spectral", b_do_spectral);

			n_ray_samples = try_get("n_ray_samples", n_ray_samples);
			if (n_ray_samples <= 0) { throw VException("n_ray_samples <= 0"); }

			n_max_march_iterations = mSharedCfg->try_get("n_max_march_iterations", n_max_march_iterations);
			//if (n_max_march_iterations <= 0) { throw VException("n_max_march_iterations < 0"); }

			f_ray_tmin = mSharedCfg->try_get("f_ray_tmin", f_ray_tmin);
			//if (f_ray_tmin <= 0.0) { throw VException("f_ray_tmin <= 0.0"); }

			f_ray_tmax = mSharedCfg->try_get("f_ray_tmax", f_ray_tmax);
			//if (f_ray_tmax <= 0.0) { throw VException("f_ray_tmax <= 0.0"); }

			f_normal_eps = mSharedCfg->try_get("f_normal_eps", f_normal_eps);
			//if (f_normal_eps <= 0.0) { throw VException("f_normal_eps <= 0.0"); }

			f_refracted_ray_eps_mult = try_get("f_refracted_ray_eps_mult", f_refracted_ray_eps_mult);
			if (f_refracted_ray_eps_mult <= 0.0) { throw VException("f_refracted_ray_eps_mult <= 0.0"); }

			f_min_wl = try_get("f_min_wl", f_min_wl);
			if (f_min_wl <= 0.0) { throw VException("f_min_wl <= 0.0"); }

			f_max_wl = try_get("f_max_wl", f_min_wl);
			if (f_max_wl <= 0.0) { throw VException("f_max_wl <= 0.0"); }

			b_debug_normals = try_get("b_debug_normals", b_debug_normals);

			b_debug_iterations = try_get("b_debug_iterations", b_debug_iterations);
		};

        inline void post_init(VScene& scn){
            std::cout<<"**Using : "<<n_ray_samples<<" samples per pixel\n";
        }

		/*inline bool russian_roulette(ygl::rng_state& rng,ygl::vec3f& w){
            auto rrprob = 1.0f - min(max_element(w), 0.95f);
            //if (get_random_float(rng) < rrprob) return false; //?
            if (get_random_float(rng) < rrprob) return false;
            w *= 1.0f / (1.0f - rrprob);
            return true;
		}*/

		inline bool russian_roulette(ygl::rng_state& rng,vec3vf& w){
            auto rrprob = std::min(max_element(w), 0.95);
            if (get_random_float(rng) > rrprob) return false;
            w *= 1.0 / rrprob;
            return true;
		}

		//////Hints from
		//https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		//////
        inline vec3vf reflect(const vec3vf &I, const vec3vf &N){
            return I - 2 * dot(I,N) * N;
        }
		//////Hints from
		//https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		//////
        inline vec3vf refract(const vec3vf &I, const vec3vf &N,vfloat etai,vfloat etat){
            vfloat cosi = ygl::clamp(dot(I,N),-1.0, 1.0);
            vec3vf n = N;
            if (cosi < 0) { cosi = -cosi; } else { /*std::swap(etai, etat);*/ n= -N; }
            vfloat eta = etai / etat;
            vfloat k = 1 - eta * eta * (1 - cosi * cosi);
            return k < 0 ? zero3vf : eta * I + (eta * cosi - std::sqrt(k)) * n;
        }

		inline vec3vf brdf_diffuse_ashikhmin(const VMaterial& m,const vfloat Rs,const VRay& wi,const VRay& wo,vfloat ndi,vfloat ndo){
            auto rfc = ((28.0)/(23.0*pivf))*m.kr*(1.0-Rs);
            auto rfc2 = (1.0-std::pow(1.0-(std::abs(ndi)/2.0),5.0))*(1.0-std::pow(1.0-(std::abs(ndo)/2.0),5.0));
            return rfc*rfc2;
		}

		inline vfloat brdf_cook_torrance_G(vfloat ndi,vfloat ndo,vfloat ndh,vfloat odh){
            return std::min(1.0,std::min( (2.0*std::abs(ndh)*std::abs(ndo))/std::abs(odh), (2.0*std::abs(ndh)*std::abs(ndi))/std::abs(odh)));
		}
		inline vfloat brdf_cook_torrance_D(vfloat rs,vfloat ndi,vfloat ndo,vfloat ndh,vfloat odh){
            auto a2 = rs*rs;
            a2 = std::max(a2,thvf);
            auto ndh2 = ndh*ndh;
            auto andh4 = ndh2*ndh2;
            return (1.0/(pivf*a2*andh4))*std::exp((ndh2-1.0)/(a2*ndh2));
        }
        inline vfloat brdf_cook_torrance_DG(vfloat rs,vfloat ndi,vfloat ndo,vfloat ndh,vfloat odh){
            auto D = brdf_cook_torrance_G(ndi,ndo,ndh,odh);
            auto G = brdf_cook_torrance_D(rs,ndi,ndo,ndh,odh);
            return D*G;
        }

		inline vfloat brdf_ggx_G(vfloat rs,vfloat ndi,vfloat ndo,vfloat ndh){
			auto a2 = rs * rs;
			a2 = std::max(a2,thvf);
			auto goDN = std::abs(ndo) + std::sqrt(a2 + (1 - a2) * ndo * ndo);
			if(cmpf(goDN,0.0)) return 0.0;
			auto giDN = std::abs(ndi) + std::sqrt(a2 + (1 - a2) * ndi * ndi);
			if(cmpf(giDN,0.0)) return 0.0;

            auto Go = (2 * std::abs(ndo)) / goDN;
            auto Gi = (2 * std::abs(ndi)) / giDN;
            return Go*Gi;
		}

        inline vfloat brdf_ggx_D(vfloat rs,vfloat ndi,vfloat ndo,vfloat ndh){
            auto a2 = rs*rs;
            a2 = std::max(a2,thvf);
            auto ndh2 = ndh*ndh;
            auto dn = ((ndh2*a2)+(1.0-(ndh2)));
            dn*=dn;
            if(cmpf(dn,0.0)) return 0.0;
            return a2 / (pivf*dn);
        }

		inline vfloat brdf_ggx_DG(vfloat rs, vfloat ndi, vfloat ndo, vfloat ndh) {
		    return brdf_ggx_D(rs,ndi,ndo,ndh)*brdf_ggx_G(rs,ndi,ndo,ndh);
		}

        inline vfloat brdf_ggx_pdf(vfloat rs,vfloat ndi,vfloat ndo,vfloat ndh){
            return brdf_ggx_D(rs,ndi,ndo,ndh)*ndh;
        }

		inline vec3vf eval_fresnel_schlick(const vec3vf& ks, vfloat dcos) {
		    if(ks==zero3vf) return zero3vf;
            return ks +(1 - ks) *std::pow(clamp(1 - std::abs(dcos), 0.0, 1.0), 5.0);
		}

		inline vec3vf eval_fresnel_conductor(const vec3vf& I,const vec3vf& N,vfloat etai,vec3vf etak){
           float cosi = ygl::clamp(dot(I,N),-1.0, 1.0);

           vfloat cosi2 = cosi * cosi;
           vfloat sini2 = 1 - cosi2;
           vec3vf etai2 = toVec<vfloat,3>(etai * etai);
           vec3vf etak2 = etak * etak;

           vec3vf t0 = etai2 - etak2 - vec3vf{sini2,sini2,sini2};
           vec3vf a2plusb2 = vnx::sqrt(t0 * t0 + 4 * etai2 * etak2);
           vec3vf t1 = a2plusb2 + vec3vf{cosi2,cosi2,cosi2};
           vec3vf a = vnx::sqrt(0.5 * (a2plusb2 + t0));
           vec3vf t2 = 2 * a * cosi;
           vec3vf Rs = (t1 - t2) / (t1 + t2);

           vec3vf t3 = cosi2 * a2plusb2 + toVec<vfloat,3>(sini2 * sini2);
           vec3vf t4 = t2 * sini2;
           vec3vf Rp = Rs * (t3 - t4) / (t3 + t4);

           return 0.5 * (Rp + Rs);
		}

		//////Hints from
		//https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		//////
		inline vfloat eval_fresnel_dielectric(const vec3vf& I, const vec3vf& N,vfloat etai,vfloat etat){
            vfloat cosi = ygl::clamp(dot(I,N),-1.0, 1.0);
            //if (cosi > ygl::epsf) { std::swap(etai, etat); }

            vfloat sint = etai / etat * std::sqrt(std::max(0.0, 1 - cosi * cosi));
            if (sint >= 1) {
                return 1.0;
            }
            else {
            vfloat cost = std::sqrt(std::max(0.0, 1 - sint * sint));
            cosi = std::abs(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
                return (Rs * Rs + Rp * Rp) / 2;
            }
		}


		inline vfloat pow2(vfloat a){return a*a;}


		inline vec3vf eval_light(const VScene& scn,ygl::rng_state& rng,VRay rr,vec3vf& ln,VResult& lres) {
			vec3vf w = one3vf;
			for(int b=0;;b++){
                int i = 0;
                auto old_rr = rr;
                auto poll = poll_volume(scn, rng, rr);
                lres = scn.INTERSECT_ALGO( rr, n_max_march_iterations, i);
                if (!lres.found() || !lres.valid()) { return zero3vf; }
                if(poll.is_inside_transmissive()){
                    w *= calc_beer_law(poll.emat,length(rr.o-lres.wor_pos));
                }
                if(is_zero_or_has_ltz(w)){return zero3vf;}

                VMaterial hmat = *lres.mat;
                ln = scn.NORMALS_ALGO(lres, f_normal_eps);
                hmat.eval_mutator(rng, lres, ln, hmat);

                if (hmat.is_emissive()) {
                    return w*eval_le(rr,hmat.e_power,hmat.e_temp);
                }
                if(hmat.is_transmissive() && !cmpf(old_rr.ior,rr.ior)){
                    if(b>2){if(!russian_roulette(rng,w)) break;}

                    if(has_inf(w)){std::cout<<"*<!>Eval light loop--> Not finite number detected--> b:"<<b<<"--->"<<w<<"\n";break;}
                    if(status.bDebugMode)std::cout<<"*<!>Eval light Loop--> b:"<<b<<"--->"<<w<<"\n";

                    bool outside = dot(rr.d,ln) > 0;
                    vec3vf offBy = outside ? -ln : ln;
                    rr = offsetted_ray(lres.wor_pos,rr,rr.d,rr.tmin,rr.tmax,offBy,lres.dist);
                    continue;
                }
                return zero3vf;
			}
			return zero3vf;
		}


		inline vec3vf sample_diffuse(const vec2f& rn,const vec3vf& o,const vec3vf& n){
            auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3vf, n) : make_frame_fromz(zero3vf, -n);
            auto wh_local = sample_hemisphere_direction_cosine_vf(rn);
            return ygl::transform_direction(fp, wh_local);
		}


		inline vec3vf sample_ggx(const vec2f& rn,const vec3vf& o,const vec3vf& n,vfloat rs){
            //auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3f, n) : make_frame_fromz(zero3f, -n);
            auto fp = make_frame_fromz(zero3vf,n);
            auto tan2 = rs * rs * rn.y / (1 - rn.y);
            auto rz = std::sqrt(1 / (tan2 + 1)), rr = std::sqrt(1 - rz * rz), rphi = 2 * pivf * rn.x;
            auto wh_local = vec3vf{ rr * std::cos(rphi), rr * std::sin(rphi), rz };
            return ygl::transform_direction(fp, wh_local);
		}

		inline VResult sample_emissive(const VScene& scn,ygl::rng_state& rng,int& idl){
		    idl = 0;
		    if(scn.emissive_hints.empty()) return VResult();
            idl = ygl::get_random_int(rng,scn.emissive_hints.size());
            const std::vector<VResult>* ep = &scn.emissive_hints[idl];
            if(ep->empty()) return VResult();
            VResult point = (*ep)[ygl::get_random_int(rng,ep->size())];
            return point;
		}

		inline VResult sample_emissive_in_light(const VScene& scn,ygl::rng_state& rng,int idl){
		    if(scn.emissive_hints.empty()) return VResult();
            const std::vector<VResult>* ep = &scn.emissive_hints[idl];
            if(ep->empty()) return VResult();
            VResult point = (*ep)[ygl::get_random_int(rng,ep->size())];
            return point;
		}

		inline VSample sample_transmissive( const VScene& scn, ygl::rng_state& rng, const VResult& hit, const VMaterial& material,const VRay& wo, const vec3vf& n, const vec3vf& wh){
            auto wi = -wo;
            bool outside = dot(wo.d,n) > 0;
            vec3vf offBy = outside ? -n : n;

            auto poll_ray = offsetted_ray(hit.wor_pos,wi,wi.d,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
            auto poll = poll_volume(scn,rng,poll_ray);
            vfloat ior = poll_ray.ior;
            auto F = eval_fresnel_dielectric(wi.d,wh,wi.ior,ior);

            if(cmpf(F,0.0)){
                auto refr_dir = refract(wi.d,wh,wi.ior,ior);
                auto iray = offsetted_ray(hit.wor_pos,wi,refr_dir,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                return VSample{iray,s_transmitted,{0,1.0}};
            }else if(cmpf(F,1.0)){
                auto refl_dir = ygl::reflect(wo.d,outside ? wh : -wh);
                auto iray = offsetted_ray(hit.wor_pos,wi,refl_dir,wi.tmin,wi.tmax,-offBy,hit.dist);
                return VSample{iray,s_tr_reflected,{0,1.0}};
            }else{
                //auto rn_ks = get_random_float(rng);
                const vfloat rv = F;
                if(get_random_float(rng)>rv){ //TODO : re-test with 0.5f
                    auto refr_dir = refract(wi.d,wh,wi.ior,ior);
                    auto iray = offsetted_ray(hit.wor_pos,wi,refr_dir,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                    return VSample{iray,s_transmitted,{0,1.0-rv}};
                }else{
                    auto refl_dir = ygl::reflect(wo.d,outside ? wh : -wh);
                    auto iray = offsetted_ray(hit.wor_pos,wi,refl_dir,wi.tmin,wi.tmax,-offBy,hit.dist);
                    return VSample{iray,s_tr_reflected,{0,rv}};
                }
            }
		}

		inline VSample sample_bsdfcos(const VScene& scn, ygl::rng_state& rng, const VResult& hit, const VMaterial& material,const VRay& wo, const vec3vf& n) {
            if(material.is_delta()){
                if(material.is_transmissive()){
                    return sample_transmissive(scn,rng,hit,material,wo,n,n);
                }else{
                    auto offby = n;//dot(wo.d,n)>=0 ? n : -n;
                    auto iray = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,offby), wo.tmin, wo.tmax, offby, hit.dist);
                    return VSample{iray,s_reflected,{0.0,1.0}};
                }
            }else{
                if(material.is_transmissive()){
                    return sample_transmissive(scn,rng,hit,material,wo,n,n); //TODO
                }else if(material.is_dielectric()){
                    auto ior = material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral);
                    auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,ior);

                    auto weights = vec2vf{(1.0-F),F};
                    weights /= weights.x+weights.y;

                    auto rr = ygl::get_random_float(rng);
                    if(rr<weights.x){
                        auto rn = ygl::get_random_vec2f(rng);
                        auto offby = n;//dot(wo.d,n)>=0 ? n : -n;
                        auto iray = offsetted_ray(hit.wor_pos, wo, sample_diffuse(rn,wo.d,n), wo.tmin, wo.tmax, offby, hit.dist);
                        return VSample{iray,s_reflected,weights};
                    }else{
                        auto rn = ygl::get_random_vec2f(rng);
                        auto offby = n;//dot(wo.d,n)>=0 ? n : -n;
                        auto iray = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(rn,wo.d,n,material.rs)), wo.tmin, wo.tmax, offby, hit.dist);
                        return VSample{iray,s_reflected,weights};
                    }
                }else{
                    auto rn = ygl::get_random_vec2f(rng);
                    auto offby = n;//dot(wo.d,n)>=0 ? n : -n;
                    auto iray = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(rn,wo.d,n,material.rs)), wo.tmin, wo.tmax, offby, hit.dist);
                    return VSample{iray,s_reflected,{0.0,1.0}};
                }
            }
		}

		inline vfloat eval_bsdfcos_pdf( const VMaterial& material, const VRay& wi, const VRay& wo, const vec3vf& n,const vec2vf& weights = one2vf) {
		    vfloat pdf = 0.0;

		    if(material.is_delta()){
                if(material.is_conductor()) pdf = 1.0;
                else if(material.is_transmissive())pdf = weights.y;
		    }else{
                auto ndi = dot(n,wi.d);
                auto ndo = dot(n,wo.d);
                auto h = normalize(wi.d+wo.d);
                auto ndh = dot(h,n);

                if(material.is_transmissive()){
                    auto ir = (dot(n, wo.d) >= 0) ? ygl::reflect(-wi.d, n) : ygl::reflect(-wi.d, -n);
                    auto hv = normalize(ir + wo.d);
                    auto d = (brdf_ggx_pdf(material.rs,dot(n,ir),ndo,dot(hv,n)));
                    pdf +=  d / (4*std::abs(dot(wo.d,hv)));
                }else{
                    if(ndo*ndi<epsvf) return 0.0;
                    if(material.is_conductor()){
                        if(ndh==0.0) return 0.0;
                        auto odh = dot(wo.d,h);
                        if(odh==0.0)return 0.0;
                        auto d = (brdf_ggx_pdf(material.rs,ndi,ndo,ndh));
                        pdf +=  d / (4*std::abs(odh));
                    }else{
                        //auto ior = material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral);
                        //auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,ior);
                        //auto weights = vec2f{(1.0f-F),F};
                        //weights /= weights.x+weights.y;

                        pdf += (weights.x)*(std::abs(ndi)/pivf);

                        if(ndh==0.0) return pdf;
                        auto odh = dot(wo.d,h);
                        if(odh==0.0)return pdf;
                        auto d = (brdf_ggx_pdf(material.rs,ndi,ndo,ndh));
                        pdf += weights.y * (d / (4*std::abs(odh)));
                    }
                }
		    }
		    return pdf;
		}

		inline vec3vf eval_bsdfcos(const VScene& scn,ygl::rng_state& rng,const VResult& hit,const VMaterial& material, const VRay& wi, const VRay& wo, const vec3vf& n) {
		    vec3vf li = zero3vf;
            auto ndi = dot(n,wi.d);
            auto ndo = dot(n,wo.d);
		    if(material.is_delta()){
                if(material.is_conductor()){
                    auto F = eval_fresnel_conductor(wo.d,n,wi.ior,material.kr);
                    li +=F;
                }else if(material.is_transmissive()){
                    bool outside = dot(wo.d,n) > 0;
                    vec3vf offBy = outside ? -n : n;
                    auto poll_ray = offsetted_ray(hit.wor_pos,wo,wo.d,wo.tmin,wo.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                    auto poll = poll_volume(scn,rng,poll_ray);
                    vfloat ior = poll_ray.ior;

                    auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,ior);
                    if(ndo*ndi<=epsvf) li+= toVec<vfloat,3>(1.0-F);
                    else li+= toVec<vfloat,3>(F);
                }
		    }else{
                if(material.is_transmissive()){
                    auto ir = (ndo >= 0) ? ygl::reflect(-wi.d, n) : ygl::reflect(-wi.d, -n);
                    auto hv = normalize(ir + wo.d);
                    auto irdn = dot(ir,n);
                    auto F = toVec<vfloat,3>(eval_fresnel_dielectric(-wo.d,hv,wo.ior,material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral)));
                    if(ndo*ndi<epsvf) li+= ((one3vf-F)*(brdf_ggx_DG(material.rs,irdn,ndo,dot(n,hv))) / (4*std::abs(irdn)*std::abs(ndo)));
                    else li+= (F*(brdf_ggx_DG(material.rs,irdn,ndo,dot(n,hv))) / (4*std::abs(irdn)*std::abs(ndo)));
                }else{
                    if(ndo*ndi<epsvf) return zero3vf;
                    auto h = normalize(wi.d+wo.d);
                    auto ndh = dot(n,h);
                    if(material.is_conductor())  {
                        auto F = eval_fresnel_conductor(wo.d,n,wo.ior,material.kr);
                        li += (F*(brdf_ggx_DG(material.rs,ndi,ndo,ndh)) / (4*std::abs(ndi)*std::abs(ndo)));
                    }
                    else{
                        auto ior = material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral);
                        auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,ior);
                        auto ks = F0_from_ior(ior);

                        auto hdi = dot(h,wi.d);
                        auto fd90 = (0.5+(hdi*hdi))*ks;
                        auto m1_ndi5 = std::pow((1.0-std::abs(ndi)),5.0);
                        auto m1_ndo5 = std::pow((1.0-std::abs(ndo)),5.0);
                        li += (material.kr/pivf)*(1.0+fd90*m1_ndi5)*(1.0+fd90*m1_ndo5)*(one3vf-F);

                        if(ndh==0.0) return li*std::abs(ndi);
                        li+=brdf_ggx_D(material.rs,ndi,ndo,ndh) / (4.0 * std::abs(dot(wi.d, h)) * std::max(std::abs(ndi), std::abs(ndo))) * F;
                    }
                }
                li*=std::abs(ndi);
		    }
		    return li;
		}


		inline vec3vf calc_beer_law(const VMaterial& evmat,const vfloat dst){
            if (dst > epsvf) return exp(-evmat.ka*dst);
            return one3vf;
		}

		inline void update_ray_physics(VRay& ray,vfloat ior){
            ray.ior = ior;
            ray.velocity = KC / ray.ior;
            ray.wl = ray.velocity / ray.frequency;
		}

		inline VVolumePoll poll_volume(const VScene& scn, ygl::rng_state& rng, VRay& ray){
			auto ev = scn.eval(ray.o);

            VMaterial evmat;
            //fix for nested dielectrics
            if(ev.vdist<0.0 && ev.dist<0.0 && std::abs(ev.dist)<std::abs(ev.vdist)){
                evmat = *ev.mat;
            }
            else{
                evmat = *ev.vmat;
            }

            vec3vf evnorm = scn.NORMALS_ALGO(ev,f_normal_eps);
            evmat.eval_mutator(rng, ev, evnorm, evmat);

			if(ev.vdist<epsvf || ev.dist<epsvf) update_ray_physics(ray,evmat.eval_ior(ray.wl,f_min_wl,f_max_wl,b_do_spectral));
			else update_ray_physics(ray,1.0);
            return {ev,evmat};
		}

		inline VVolumePoll poll_volume_noray(const VScene& scn, ygl::rng_state& rng,const VRay& ray){
			auto ev = scn.eval(ray.o);
            VMaterial evmat;
            //fix for nested dielectrics
            if(ev.vdist<0.0 && ev.dist<0.0 && std::abs(ev.dist)<std::abs(ev.vdist)){
                evmat = *ev.mat;
            }
            else{
                evmat = *ev.vmat;
            }

            vec3vf evnorm = scn.NORMALS_ALGO(ev,f_normal_eps);
            evmat.eval_mutator(rng, ev, evnorm, evmat);
            return {ev,evmat};
		}

		inline vfloat eval_le(const VRay& ray,vfloat pwr,vfloat t){
		    //TODO , find a way to retain emissive power in the case of b_do_spectral,
		    //to brute force it i multiply for a constant to make luminance similar to the non spectral case at 6500k
            if(b_do_spectral) return blackbody_planks_law_wl_normalized(t,ray.velocity,ray.wl)*pwr*2.0;
            return pwr;
		}

		inline bool isGathering(const VMaterial& m){
            return m.is_delta();
		}

		inline vec3f sample_hg_phase(vfloat& pdf){
            //TODO
		}

		inline vec3f sample_iso_phase(vfloat& pdf){
            //TODO
		}

		inline vfloat Mis2(vfloat spdf,vfloat o1pdf){
            return spdf / (spdf+o1pdf);
		}

		inline vfloat Mis3(vfloat spdf,vfloat o1pdf,vfloat o2pdf){
            return spdf / (spdf+o1pdf+o2pdf);
		}

        enum VertexType{
            SURFACE,
            VOLUME,
            EMISSIVE
        };

		struct VVertex{
		    struct VTroughput{
		        vec3vf w;
                vfloat e_power;
                vfloat e_temp;
		    };
            const VResult& hit;
            const VMaterial& material;
            const VRay& wi;
            const VTroughput& troughput;
            const vec3vf& n;
            VertexType type = SURFACE;
		};

		inline vec3vf ConnectToEmissive(const VScene& scn,ygl::rng_state& rng,const VRay& wo,const VVertex& v1,const VResult& em){
            auto dir = normalize(em.wor_pos-v1.hit.wor_pos);
            auto to_em = offsetted_ray(v1.hit.wor_pos,wo,dir,f_ray_tmin,f_ray_tmax,v1.n,v1.hit.dist);
            int n_iters = 0;
            auto origin = to_em.o;

            vec3vf brdf = zero3vf;
            vfloat pdf = 0.0;

            if(v1.type==VOLUME){
                brdf = one3vf;
                pdf = 4*pivf;
            }else if(v1.type==SURFACE){
                brdf = eval_bsdfcos(scn,rng,v1.hit,v1.material,to_em,wo,v1.n);
                if(brdf==zero3vf) return zero3vf;
                pdf = eval_bsdfcos_pdf(v1.material,to_em,wo,v1.n);
                if(pdf<=0.0) return zero3vf;
            }

            vfloat traveled_dist = 0.0;
            vec3vf trw = one3vf;
            for(int b=0;;++b){
                VVolumePoll poll = poll_volume(scn,rng,to_em);
                VResult hit = scn.INTERSECT_ALGO(to_em,n_max_march_iterations,n_iters);
                if(!hit.found()) return zero3vf;

                VMaterial tmat = *hit.mat;
                auto tn = scn.NORMALS_ALGO(hit,f_normal_eps);
                tmat.eval_mutator(rng,hit,tn,tmat);

                if(poll.is_inside_transmissive() && poll.emat.ka!=zero3vf){
                    trw *= exp(-poll.emat.ka*ygl::distance(to_em.o,hit.wor_pos));
                }
                traveled_dist = ygl::distance(origin,hit.wor_pos);

                if(tmat.is_emissive()){
                    auto lc = eval_le(to_em,tmat.e_power,tmat.e_temp);
                    auto dsqr = traveled_dist*traveled_dist;
                    auto pdf2 = dsqr / std::abs(dot(tn,-to_em.d));

                    vfloat mw = Mis2(pdf2,1.0/pdf);
                    if(mw>0.0) return (mw / pdf2)*(trw*v1.troughput.w*lc*brdf);
                    return zero3vf;
                }

                if(!tmat.is_transmissive() || tmat.is_refractive()){break;}
                if (b>2){if(!russian_roulette(rng,trw)) break;}

                if(has_inf(trw)){std::cout<<"*<!>Connection loop--> Not finite number detected--> b:"<<b<<"--->"<<trw<<"\n";break;}
                if(status.bDebugMode)std::cout<<"*<!>Connection Loop--> b:"<<b<<"--->"<<trw<<"\n";

                //continue ray
                auto offby = dot(to_em.d,tn) > 0 ? -tn : tn;
                to_em = offsetted_ray(hit.wor_pos,to_em,to_em.d,f_ray_tmin,f_ray_tmax,-offby,hit.dist);
            }
            return zero3vf;
		}

		inline void init_ray_physics(rng_state& rng,VRay& ray){
            ray.wl = rand1f_r(rng,f_min_wl,f_max_wl);
            ray.velocity = KC;
            ray.frequency = ray.velocity / ray.wl;
            ray.ior = 1.0;
		}

		inline vec3vf eval_pixel(const VScene& scn, ygl::rng_state& rng,image3vf& img,const VCamera& camera, const VRay& ray) {
			vec3vf output = zero3vf;
			vec3vf w = one3vf;
			bool gather_ke = true;
			bool nee_hit = false;
            VSample sample = {ray,s_undefined};
			VResult hit = {};

            for(int b=0;;b++){
                status.mRaysEvaled++;
                int n_iters = 0;
                VVolumePoll poll = poll_volume(scn, rng, sample.ray);

                if(!nee_hit)hit = scn.INTERSECT_ALGO( sample.ray, n_max_march_iterations, n_iters);
                nee_hit = false;

                if (b_debug_iterations) {
                    if (!hit.found() || !hit.valid()) {break; }
                    return one3vf*((vfloat)n_iters)/((vfloat)n_max_march_iterations);
                }

                if (b_debug_normals) {
                    if (!hit.found() || !hit.valid()) {break; }
                    return scn.NORMALS_ALGO(hit,f_normal_eps);
                }

                if(poll.emat.k_sca>epsvf && poll.eres.vdist<0.0){
                    vfloat tFar = 0.0;
                    if (!hit.found() || !hit.valid()) {tFar = sample.ray.tmax;}
                    else {tFar = ygl::distance(sample.ray.o,hit.wor_pos);}
                    auto vmt = poll.emat;

                    vfloat dist = -std::log(1-ygl::get_random_float(rng)) / (vmt.k_sca);
                    vfloat pdf = std::exp(-(vmt.k_sca)*tFar);

                    if(dist<tFar){
                        //auto incoming = sample.ray;
                        sample.ray.o = sample.ray.o+(sample.ray.d*dist);

                        poll = poll_volume_noray(scn, rng, sample.ray);

                        if(!scn.emissive_hints.empty()){
                            int idl = 0;
                            auto emissive = sample_emissive(scn,rng,idl);
                            VVertex ev = {poll.eres,poll.emat,sample.ray,{w,0.0,0.0},zero3vf,VOLUME};
                            output += ConnectToEmissive(scn,rng,sample.ray,ev,emissive);
                            //VVertex lev = {emissive,{},{},{w,0.0f,0.0f},scn.NORMALS_ALGO(emissive,f_normal_eps),EMISSIVE,0.0f,0.0f};
                            //output += ConnectVertex(scn,rng,ev,lev,incoming,pdf,1);
                            //for(int vi = 0;vi<light_path_length; vi++){
                                //output += ConnectVertex(scn,rng,ev,vertices[vi],incoming,pdf,light_path_length);
                            //}
                        }

                        sample.ray.d =  sample_sphere_direction_vf(ygl::get_random_vec2f(rng));

                        w = (w*exp(-vmt.ka*dist))/(pdf+(4*pivf));
                        if(is_zero_or_has_ltz(w) || has_inf(w)) break;

                        if(b>2){if(!russian_roulette(rng,w)){break;}}
                        continue;
                    }else{
                        if (!hit.found() || !hit.valid()) {break; }
                        w = w*exp(-vmt.ka*tFar)/(pdf);
                    }
                }else if(cmpf(poll.emat.k_sca,0.0) && poll.eres.vdist<0.0){
                    if (!hit.found() || !hit.valid()) {break; }
                    w = w*exp(-poll.emat.ka*ygl::distance(sample.ray.o,hit.wor_pos));
                }else{
                    if (!hit.found() || !hit.valid()) {break; }
                }

                auto material = *hit.mat;
                auto n = scn.NORMALS_ALGO(hit, f_normal_eps);
                material.eval_mutator(rng, hit, n, material);

                //if(n==zero3f){if(status.bDebugMode){std::cout<<"*<!>Main Loop--> Normal is zero\n";}break;}

                auto wo = sample.ray;
                wo.d=-wo.d;
                auto wi = sample.ray;

                if (material.is_emissive()) {
                    auto radiance = eval_le(wo,material.e_power,material.e_temp);
                    if(gather_ke) output += w*radiance;
                    break;
                }

                gather_ke = isGathering(material);

                sample = sample_bsdfcos(scn, rng, hit, material, wo, n);
                wi = sample.ray;
                if(wi.d==zero3vf) break;

                auto pdf = eval_bsdfcos_pdf(material,wi,wo,n,sample.pdf_factor);
                if(cmpf(pdf,0.0)) break;
                auto brdf = eval_bsdfcos(scn,rng,hit,material,wi,wo,n);
                if(cmpf(brdf,zero3vf)) break;

                //DIRECT + NEE
                if(!gather_ke){

                    //TODO
                    if(!scn.emissive_hints.empty()){
                        int idl = 0;
                        auto emissive = sample_emissive(scn,rng,idl);
                        VVertex ev = {hit,material,wi,{w,0.0,0.0},n,SURFACE};
                        output += ConnectToEmissive(scn,rng,wo,ev,emissive);
                        //VVertex lev = {emissive,{},{},{w,0.0f,0.0f},scn.NORMALS_ALGO(emissive,f_normal_eps),EMISSIVE,0.0f,0.0f};
                        //output += ConnectVertex(scn,rng,ev,lev,wo,pdf,1);
                        //for(int vi = 0;vi<light_path_length; vi++){
                            //output += ConnectVertex(scn,rng,ev,vertices[vi],wo,pdf,light_path_length);
                        //}
                    }

                    { //SCOPE
                        vec3vf ln;
                        VResult lres;
                        auto lc = eval_light(scn, rng, wi, ln, lres);
                        if(!cmpf(lc,zero3vf)){
                            auto dist = ygl::distance_squared(hit.wor_pos,lres.wor_pos);
                            auto pdf2 = (dist) / std::abs(dot(ln,-wi.d));
                            vfloat mw = Mis2(pdf2,1.0/pdf);
                            if(mw>0.0) output += (mw / pdf2) *(w*lc*brdf);
                            break; // will hit an emissive as next event
                        }else{nee_hit = true;hit = lres;}
                    }

                }

                //

                w*= brdf/pdf;
                if (b>2){if(!russian_roulette(rng,w)) break;}
                if (is_zero_or_has_ltz(w) || has_inf(w)) break;


                if(has_inf(w)){std::cout<<"*<!>Main loop--> Not finite number detected: pdf: "<<pdf<<"; w:"<<w<<"\n";break;}
                if(status.bDebugMode)std::cout<<"*<!>Main Loop--> b:"<<b<<"--->"<<w<<"\n";
            };

			return output;
		};

		inline void eval_image(const VScene& scn, ygl::rng_state& rng, image3vf& img, int width, int height, int j) {
		    const vfloat f_ray_samples = (vfloat)n_ray_samples;
			for (int i = 0; i < width && !status.bStopped; i++) {
				vec3vf color = zero3vf;

                for (int s = 0; s < n_ray_samples; s++) {
                    VRay ray = scn.camera.Cast(i,j,get_random_vec2f(rng));
                    ray.tmin = f_ray_tmin;
                    ray.tmax = f_ray_tmax;
                    init_ray_physics(rng,ray);

                    if(b_do_spectral) color += (eval_pixel(scn, rng, img, scn.camera, ray)/ f_ray_samples)*spectral_to_rgb(ray.wl);
                    else color += eval_pixel(scn, rng, img, scn.camera, ray)/ f_ray_samples;
                }
				auto& px = at(img,{i, j});
				px += color; // / f_ray_samples;
			}
		}

        inline std::string img_affix(const VScene& scn) const {
		    std::string ss = "";
		    if(b_do_spectral)ss+="_spec";
		    ss += std::string("_spp")+std::to_string(n_ray_samples);
		    return ss;
		}

	};

};


#endif
