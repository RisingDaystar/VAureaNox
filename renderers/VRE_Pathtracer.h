
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


#ifndef _VRE_Pathtracer_
#define _VRE_Pathtracer_

#include "../VAureaNox.h"

namespace vnx {

	struct VRE_Pathtracer : VRenderer {
		enum v_light_mode {
			direct,
			mis
		};

		enum v_aa_mode {
			aa_montecarlo,
			aa_ssaa
		};

		enum v_sample_type{
            s_camera,
		    s_diffuse,
		    s_specular,
		    s_glossy,
		    s_transmissive,
		};

		struct VSample{
            VRay ray;
            v_sample_type type = s_camera;
		};

		struct VVolumePoll{
            VResult eres;
            VMaterial emat;

            bool inline is_inside(){return eres.vdist<=ygl::epsf;}
            bool inline is_on_surface(const VRay& ray){return eres.vdist>-ray.tmin && eres.vdist<ray.tmin;}

            bool inline is_on_inside_surface(const VRay& ray){return is_inside() && eres.vdist>-ray.tmin;}
            bool inline is_on_outside_surface(const VRay& ray){return !is_inside() && eres.vdist<ray.tmin;}

            bool inline is_inside_transmissive(){return is_inside() && emat.is_transmissive();}

            bool inline is_on_inside_surface_transmissive(const VRay& ray){return is_on_inside_surface(ray) && emat.is_transmissive();}
            bool inline is_on_outside_surface_transmissive(const VRay& ray){return is_on_outside_surface(ray) && emat.is_transmissive();}
		};

		bool b_do_gi = true;
		bool b_do_spectral = true;
		bool b_do_thin_shadows = false;
		bool b_do_branched_dielectric = true;
		bool b_do_double_direct_sampling = true;

		int n_ray_samples = 1;
		int n_ssaa_samples = 1;
		float f_ray_tmin = 0.001f;
		float f_ray_tmax = 1000.0f;
		float f_normal_eps = 0.0001f;
		float f_refracted_ray_eps_mult = 2.0f;
		float f_min_wl = 400;
		float f_max_wl = 700;
		bool b_debug_normals = false;
		bool b_debug_iterations = false;
		bool b_use_ssaa = false;
		int n_max_march_iterations = 512;
		v_aa_mode e_aa_mode = aa_montecarlo;


		VRE_Pathtracer(std::string cf) : VRenderer(cf) {} ///TODO inizializzazione stile c++

		inline std::string type() const {return "PathTracer";}

		using VRenderer::try_get; //permette di vedere gli altri overload

		v_aa_mode try_get(std::string k, v_aa_mode dVal) const {
			auto match = params.find(k);
			if (match == params.end()) { return dVal; }

			if (stricmp(match->second, "montecarlo")) { return aa_montecarlo; }
			if (stricmp(match->second, "ssaa")) { return aa_ssaa; }
			return aa_ssaa;
		};



		inline void init(){
            parse();

            b_do_gi = try_get("b_do_gi", b_do_gi);
			b_do_spectral = try_get("b_do_spectral", b_do_spectral);
			b_do_thin_shadows = try_get("b_do_thin_shadows", b_do_thin_shadows);
			b_do_branched_dielectric = try_get("b_do_branched_dielectric", b_do_branched_dielectric);
			b_do_double_direct_sampling = try_get("b_do_double_direct_sampling", b_do_double_direct_sampling);


			n_ray_samples = try_get("n_ray_samples", n_ray_samples);
			if (n_ray_samples <= 0) { throw VException("n_ray_samples <= 0"); }

			n_ssaa_samples = try_get("n_ssaa_samples", n_ssaa_samples);
			if (n_ssaa_samples <= 0) { throw VException("n_ssaa_samples <= 0"); }

			n_max_march_iterations = try_get("n_max_march_iterations", n_max_march_iterations);
			if (n_max_march_iterations <= 0) { throw VException("n_max_march_iterations < 0"); }

			f_ray_tmin = try_get("f_ray_tmin", f_ray_tmin);
			if (f_ray_tmin <= 0.0f) { throw VException("f_ray_tmin <= 0.0f"); }

			f_ray_tmax = try_get("f_ray_tmax", f_ray_tmax);
			if (f_ray_tmax <= 0.0f) { throw VException("f_ray_tmax <= 0.0f"); }

			f_normal_eps = try_get("f_normal_eps", f_normal_eps);
			if (f_normal_eps <= 0.0f) { throw VException("f_normal_eps <= 0.0f"); }

			f_refracted_ray_eps_mult = try_get("f_refracted_ray_eps_mult", f_refracted_ray_eps_mult);
			if (f_refracted_ray_eps_mult <= 0.0f) { throw VException("f_refracted_ray_eps_mult <= 0.0f"); }

			f_min_wl = try_get("f_min_wl", f_min_wl);
			if (f_min_wl <= 0.0f) { throw VException("f_min_wl <= 0.0f"); }

			f_max_wl = try_get("f_max_wl", f_min_wl);
			if (f_max_wl <= 0.0f) { throw VException("f_max_wl <= 0.0f"); }

			b_debug_normals = try_get("b_debug_normals", b_debug_normals);

			b_debug_iterations = try_get("b_debug_iterations", b_debug_iterations);

			e_aa_mode = try_get("e_aa_mode",e_aa_mode);

		};

        inline void post_init(VScene& scn){
        }


		inline bool russian_roulette(ygl::rng_state& rng,ygl::vec3f& w){
            auto rrprob = 1.0f - min(max_element(w), 0.95f);
            if (get_random_float(rng) < rrprob) return false;
            w *= 1.0f / (1.0f - rrprob);
            return true;
		}

		inline ygl::vec3f calc_offBy(const vec3f& d, const ygl::vec3f& norm) {
			if (!(dot(d, norm) < 0.0f)) { return -norm; }
			return norm;
		}

		//////Hints from
		//https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		//////
        inline vec3f reflect(const vec3f &I, const vec3f &N){
            return I - 2 * dot(I,N) * N;
        }
		//////Hints from
		//https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		//////
        inline vec3f refract(const vec3f &I, const vec3f &N,float etai,float etat){
            float cosi = ygl::clamp(dot(I,N),-1.0f, 1.0f);
            vec3f n = N;
            if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
            float eta = etai / etat;
            float k = 1 - eta * eta * (1 - cosi * cosi);
            return k < 0 ? zero3f : eta * I + (eta * cosi - sqrtf(k)) * n;
        }


		inline vec3f brdf_diffuse_ashikhmin(const VMaterial& m,const float Rs,const VRay& wi,const VRay& wo,float ndi,float ndo){
            auto rfc = ((28.0f*m.kr)/(23.0f*ygl::pif))*(1.0f-Rs);
            auto rfc2 = (1.0f-std::pow(1.0f-(abs(ndi)/2.0f),5.0f))*(1.0f-std::pow(1.0f-(abs(ndo)/2.0f),5.0f));
            return rfc*rfc2;
		}

		inline float brdf_cook_torrance_G(float ndi,float ndo,float ndh,float odh){
            return std::min(1.0f,std::min( (2.0f*abs(ndh)*abs(ndo))/abs(odh), (2.0f*abs(ndh)*abs(ndi))/abs(odh)));
		}
		inline float brdf_cook_torrance_D(float rs,float ndi,float ndo,float ndh,float odh){
            auto a2 = rs*rs;
            a2 = max(a2,0.00001f);
            auto ndh2 = ndh*ndh;
            auto andh4 = ndh2*ndh2;
            return (1.0f/(ygl::pif*a2*andh4))*exp((ndh2-1.0f)/(a2*ndh2));
        }
        inline float brdf_cook_torrance_DG(float rs,float ndi,float ndo,float ndh,float odh){
            auto D = brdf_cook_torrance_G(ndi,ndo,ndh,odh);
            auto G = brdf_cook_torrance_D(rs,ndi,ndo,ndh,odh);
            return D*G;
        }

		inline float brdf_ggx_G(float rs,float ndi,float ndo,float ndh){
			auto a2 = rs * rs;
			a2 = max(a2,0.00001f);
            auto Go = (2 * abs(ndo)) / (abs(ndo) + std::sqrt(a2 + (1 - a2) * ndo * ndo));
            auto Gi = (2 * abs(ndi)) / (abs(ndi) + std::sqrt(a2 + (1 - a2) * ndi * ndi));
            return Go*Gi;
		}

        inline float brdf_ggx_D(float rs,float ndi,float ndo,float ndh){
            auto a2 = rs*rs;
            a2 = max(a2,0.00001f);
            auto ndh2 = ndh*ndh;
            auto dn = ((ndh2*a2)+(1.0f-(ndh2)));
            dn*=dn;
            return a2 / (ygl::pif*dn);
        }

		inline float brdf_ggx_DG(float rs, float ndi, float ndo, float ndh) {
		    return brdf_ggx_D(rs,ndi,ndo,ndh)*brdf_ggx_G(rs,ndi,ndo,ndh);
		}

        inline float brdf_ggx_pdf(float rs,float ndi,float ndo,float ndh){
            return brdf_ggx_D(rs,ndi,ndo,ndh)*ndh;
        }

		inline ygl::vec3f eval_fresnel_schlick(const ygl::vec3f& ks, float dcos) {
            return ks +(1 - ks) *std::pow(clamp(1 - abs(dcos), 0.0f, 1.0f), 5.0f);
		}

		inline vec3f eval_fresnel_conductor(const vec3f& I,const vec3f& N,float etai,vec3f etak){
           float cosi = ygl::clamp(dot(I,N),-1.0f, 1.0f);

           float cosi2 = cosi * cosi;
           float sini2 = 1 - cosi2;
           vec3f etai2 = toVec3f(etai * etai);
           vec3f etak2 = etak * etak;

           vec3f t0 = etai2 - etak2 - vec3f{sini2,sini2,sini2};
           vec3f a2plusb2 = vnx::sqrt(t0 * t0 + 4 * etai2 * etak2);
           vec3f t1 = a2plusb2 + vec3f{cosi2,cosi2,cosi2};
           vec3f a = vnx::sqrt(0.5f * (a2plusb2 + t0));
           vec3f t2 = 2 * a * cosi;
           vec3f Rs = (t1 - t2) / (t1 + t2);

           vec3f t3 = cosi2 * a2plusb2 + toVec3f(sini2 * sini2);
           vec3f t4 = t2 * sini2;
           vec3f Rp = Rs * (t3 - t4) / (t3 + t4);

           return 0.5 * (Rp + Rs);
		}

		//////Hints from
		//https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		//////
		inline float eval_fresnel_dielectric(const vec3f& I, const vec3f& N,float etai,float etat){
            float cosi = ygl::clamp(dot(I,N),-1.0f, 1.0f);
            if (cosi > ygl::epsf) { std::swap(etai, etat); }

            float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
            if (sint >= 1) {
                return 1.0f;
            }
            else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
                return (Rs * Rs + Rp * Rp) / 2;
            }
		}


		inline ygl::vec3f eval_light(const VScene& scn,ygl::rng_state& rng,const VRay& orr, ygl::vec3f n,ygl::vec3f& ln,VResult& lres) {
			ygl::vec3f w = one3f;
			int i = 0;
			int b = 0;
			VRay rr = orr;
			while (true) {
                //TODO : controllare meccanismo del poll_volume
                auto poll = poll_volume(scn, rng, rr);
                VResult hit = scn.INTERSECT_ALGO( rr, n_max_march_iterations, i);
                if(poll.is_inside() && !poll.emat.is_transmissive()) {break;}
				if (!hit.found() || !hit.valid()) { return zero3f; }

                if(poll.is_inside_transmissive()){
                    w *= calc_beer_law(poll.emat,length(rr.o-hit.wor_pos));
                }
                if(w == zero3f){return zero3f;}

                VMaterial hmat = *hit.mat;
                n = scn.eval_v_normals(hit, f_normal_eps);
				hmat.eval_mutator(rng, hit, n, hmat);

				if (hmat.is_emissive()) {
					ln = n;
                    lres = hit;
                    return w*eval_le(rr,hmat.e_power,hmat.e_temp);
				}

				if (hmat.is_transmissive()) {
                    if(!hmat.is_refractive()){
                        bool outside = dot(rr.d,n) < 0;
                        vec3f offBy = outside ? -n : n;
                        rr = offsetted_ray(hit.wor_pos, rr, rr.d, rr.tmin, rr.tmax, offBy, hit.dist, f_refracted_ray_eps_mult);
                    }else{
                        if(!b_do_thin_shadows) return ygl::zero3f;
                        bool outside = dot(rr.d,n) < 0;
                        vec3f offBy = outside ? -n : n;

                        //TODO : fix del meccanismo del poll_ray / poll_volume
                        //auto poll_ray = offsetted_ray(hit.wor_pos,rr,rr.d,rr.tmin,rr.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                        //auto poll = poll_volume(scn,rng,poll_ray);
                        //auto ior = poll_ray.ior;

                        auto ior = hmat.eval_ior(rr.wl,f_min_wl,f_max_wl,b_do_spectral);
                        auto ks = eval_fresnel_dielectric(-rr.d,n,rr.ior,ior);
                        w*=1.0f-ks;
                        rr = offsetted_ray(hit.wor_pos, rr, rr.d, rr.tmin, rr.tmax, offBy, hit.dist, f_refracted_ray_eps_mult);
                    }
				}
				else{ return ygl::zero3f; }
				if(status.bDebugMode) std::cout<<"*Shadow Loop--> i:"<<i<<"--->"<<w.x<<","<<w.y<<","<<w.z<<"\n";

				b++;
			}
			return zero3f;
		}


		inline vec3f sample_diffuse(const vec2f& rn,const vec3f& o,const vec3f& n){
            auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3f, n) : make_frame_fromz(zero3f, -n);
            auto rz = sqrtf(rn.y), rr = sqrtf(1 - rz * rz), rphi = 2 * ygl::pif * rn.x;
            auto wh_local = ygl::vec3f{rr * cosf(rphi), rr * sinf(rphi), rz };
            return ygl::transform_direction(fp, wh_local);
		}

		inline vec3f sample_ggx(const vec2f& rn,const vec3f& o,const vec3f& n,float rs){
            auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3f, n) : make_frame_fromz(zero3f, -n);
            auto tan2 = rs * rs * rn.y / (1 - rn.y);
            auto rz = std::sqrt(1 / (tan2 + 1)), rr = std::sqrt(1 - rz * rz), rphi = 2 * ygl::pif * rn.x;
            auto wh_local = ygl::vec3f{ rr * std::cos(rphi), rr * std::sin(rphi), rz };
            return ygl::transform_direction(fp, wh_local);
		}

		inline VResult sample_emissive(const VScene& scn,ygl::rng_state& rng,int& idl){
		    idl = 0;
		    if(scn.emissive_hints.empty()) return VResult();
		    idl = (int)(ygl::get_random_float(rng) * scn.emissive_hints.size());
            const std::vector<VResult>* ep = &scn.emissive_hints[idl];
            if(ep->empty()) return VResult();
            VResult point = (*ep)[(int)(ygl::get_random_float(rng) * ep->size())];
            return point;
		}

		inline VResult sample_emissive_in_light(const VScene& scn,ygl::rng_state& rng,int idl){
		    if(scn.emissive_hints.empty()) return VResult();
            const std::vector<VResult>* ep = &scn.emissive_hints[idl];
            if(ep->empty()) return VResult();
            VResult point = (*ep)[(int)(ygl::get_random_float(rng) * ep->size())];
            return point;
		}

		inline void sample_transmissive(const VScene& scn, ygl::rng_state& rng, const VResult& hit, const VMaterial& material,const VRay& wo, const ygl::vec3f& n, const ygl::vec3f& wh, std::vector<VSample>& samples){
                auto wi = -wo;
                bool outside = dot(wo.d,n) > 0;
                vec3f offBy = outside ? -n : n;
                auto type_refl = s_specular;


                //TODO : fix del meccanismo del poll_ray / poll_volume
                //auto poll_ray = offsetted_ray(hit.wor_pos,wi,wi.d,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                //auto poll = poll_volume(scn,rng,poll_ray);
                //auto ior = poll_ray.ior;

                auto ior = hit.vmat->eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral);

                auto F = eval_fresnel_dielectric(wi.d,wh,wi.ior,ior);

                if(b_do_branched_dielectric){
                    if(F<1.0f){
                        auto refr_dir = refract(wi.d,wh,wi.ior,ior);
                        auto iray = offsetted_ray(hit.wor_pos,wi,refr_dir,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                        samples.push_back(VSample{iray,s_transmissive});
                    }
                    if(F>ygl::epsf){
                        auto refl_dir = ygl::reflect(wo.d,outside ? wh : -wh);
                        auto iray = offsetted_ray(hit.wor_pos,wi,refl_dir,wi.tmin,wi.tmax,-offBy,hit.dist,f_refracted_ray_eps_mult);
                        samples.push_back(VSample{iray,type_refl});
                    }
                }else{
                    if(cmpf(F,0.0f)){
                        auto refr_dir = refract(wi.d,wh,wi.ior,ior);
                        auto iray = offsetted_ray(hit.wor_pos,wi,refr_dir,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                        samples.push_back(VSample{iray,s_transmissive});
                    }else if(cmpf(F,1.0f)){
                        auto refl_dir = ygl::reflect(wo.d,outside ? wh : -wh);
                        auto iray = offsetted_ray(hit.wor_pos,wi,refl_dir,wi.tmin,wi.tmax,-offBy,hit.dist,f_refracted_ray_eps_mult);
                        samples.push_back(VSample{iray,type_refl});
                    }else{
                        auto rn_ks = get_random_float(rng);
                        if(rn_ks>F){
                            auto refr_dir = refract(wi.d,wh,wi.ior,ior);
                            auto iray = offsetted_ray(hit.wor_pos,wi,refr_dir,wi.tmin,wi.tmax,offBy,hit.dist,f_refracted_ray_eps_mult);
                            samples.push_back(VSample{iray,s_transmissive});
                        }else{
                            auto refl_dir = ygl::reflect(wo.d,outside ? wh : -wh);
                            auto iray = offsetted_ray(hit.wor_pos,wi,refl_dir,wi.tmin,wi.tmax,-offBy,hit.dist,f_refracted_ray_eps_mult);
                            samples.push_back(VSample{iray,type_refl});
                        }
                    }
                }
		}

		inline void sample_bsdfcos(const VScene& scn, ygl::rng_state& rng, const VResult& hit, const VMaterial& material,const VRay& wo, const ygl::vec3f& n, std::vector<VSample>& samples) {
            if(material.is_delta()){
                if(material.is_transmissive()){
                    sample_transmissive(scn,rng,hit,material,wo,n,n,samples);
                }else{
                    auto iray = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,dot(n,wo.d) >= 0 ? n : -n), wo.tmin, wo.tmax, n, hit.dist);
                    samples.push_back(VSample{iray,s_specular});
                }
            }else{
                if(material.is_transmissive()){
                    auto rn = ygl::get_random_vec2f(rng);
                    auto wh = sample_ggx(rn,wo.d,n,material.rs);
                    sample_transmissive(scn,rng,hit,material,wo,n,wh,samples);
                }else if(material.is_dielectric()){
                    auto ior = material.eval_ior(wo.ior,f_min_wl,f_max_wl,b_do_spectral);
                    auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,ior);
                    auto weights = vec2f{1.0f-F,F};
                    weights /= weights.x+weights.y;

                    auto rr = ygl::get_random_float(rng);
                    if(rr<weights.x){
                        auto rn = ygl::get_random_vec2f(rng);
                        auto iray = offsetted_ray(hit.wor_pos, wo, sample_diffuse(rn,wo.d,n), wo.tmin, wo.tmax, n, hit.dist);
                        samples.push_back(VSample{iray,s_diffuse});
                    }else{
                        auto rn = ygl::get_random_vec2f(rng);
                        auto iray = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(rn,wo.d,n,material.rs)), wo.tmin, wo.tmax, n, hit.dist, 2.0f);
                        samples.push_back(VSample{iray,s_glossy});
                    }
                }else{
                    auto rn = ygl::get_random_vec2f(rng);
                    auto iray = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(rn,wo.d,n,material.rs)), wo.tmin, wo.tmax, n, hit.dist, 2.0f);
                    samples.push_back(VSample{iray,s_glossy});
                }

            }
		}

		inline float eval_bsdfcos_pdf( const VMaterial& material, const VRay& wi, const VRay& wo, const ygl::vec3f& n) {
		    float pdf = 0.0f;

		    if(material.is_delta()){
                if(material.is_conductor()) pdf = 1;
                else if(material.is_transmissive()) pdf = 1;
		    }else{
		        vec2f weights = zero2f;
                auto ndi = dot(n,wi.d);
                auto ndo = dot(n,wo.d);
                auto h = normalize(wi.d+wo.d);
                auto ndh = dot(h,n);

                if(material.is_transmissive()){
                    //TODO rough transmissive
                }else{
                    if(ndo*ndi<=ygl::epsf) return 0.0f;
                    if(material.is_conductor()){
                        if(ndh<=ygl::epsf) return 0.0f;
                        weights = {0,1};
                    }else{
                        auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral));
                        weights = {1.0f-F,F};
                        weights /= weights.x+weights.y;
                        pdf += ((weights.x)/ygl::pif);
                    }
                    if(ndh>ygl::epsf){
                        auto odh = dot(wo.d,h);
                        auto d = (brdf_ggx_pdf(material.rs,ndi,ndo,ndh));
                        pdf += weights.y * d / (4*abs(odh));
                    }
                }
		    }
		    return pdf;
		}

		inline ygl::vec3f eval_bsdfcos(const VMaterial& material, const VRay& wi, const VRay& wo, const ygl::vec3f& n) {
		    vec3f li = zero3f;
            auto ndi = dot(n,wi.d);
            auto ndo = dot(n,wo.d);
		    if(material.is_delta()){
                if(material.is_conductor()){
                    auto F = eval_fresnel_conductor(wo.d,n,wi.ior,material.kr);
                    li +=F;
                }else if(material.is_transmissive()){
                    auto F = eval_fresnel_dielectric(-wo.d,n,wo.ior,material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral));
                    if(ndo*ndi<=ygl::epsf) li+= toVec3f(1.0f-F);
                    else li+= toVec3f(F);
                }
		    }else{
                if(material.is_transmissive()){
                    //TODO rough transmissive
                }else{
                    if(ndo*ndi<=0) return zero3f;
                    auto h = normalize(wi.d+wo.d);
                    auto ndh = dot(n,h);
                    vec3f F = zero3f;
                    if(material.is_conductor())  F = eval_fresnel_conductor(wo.d,n,wo.ior,material.kr);
                    else{
                        F = toVec3f(eval_fresnel_dielectric(-wo.d,n,wo.ior,material.eval_ior(wo.wl,f_min_wl,f_max_wl,b_do_spectral)));
                        auto hdi = dot(h,wi.d);
                        auto fd90 = (0.5f+(hdi*hdi))*material.rs;
                        auto m1_ndi5 = std::pow((1.0f-abs(ndi)),5.0f);
                        auto m1_ndo5 = std::pow((1.0f-abs(ndo)),5.0f);
                        li += (material.kr/ygl::pif)*(1.0f+fd90*m1_ndi5)*(1.0f+fd90*m1_ndo5)*(one3f-F)*abs(ndi);
                    }
                    if(ndh>ygl::epsf){
                        li += (F*(brdf_ggx_DG(material.rs,ndi,ndo,ndh)) / (4*abs(ndi)*abs(ndo)))*abs(ndi);
                    }
                }
		    }
		    return li;
		}


		inline vec3f calc_beer_law(const VMaterial& evmat,const float dst){
            if (dst > ygl::epsf) return exp(-evmat.ka*dst);
            return one3f;
		}

		inline void update_ray_physics(VRay& ray,float ior){
            ray.ior = ior;
            ray.velocity = KC / ray.ior;
            ray.wl = ray.velocity / ray.frequency;
		}

		inline VVolumePoll poll_volume(const VScene& scn, ygl::rng_state& rng, VRay& ray){
		    VResult ev;
			scn.eval(ray.o,ev);
            auto evmat = *ev.vmat;
            ygl::vec3f evnorm = scn.eval_v_normals(ev,f_normal_eps);
            evmat.eval_mutator(rng, ev, evnorm, evmat);
			if (ev.vdist < ygl::epsf) {
			    update_ray_physics(ray,evmat.eval_ior(ray.wl,f_min_wl,f_max_wl,b_do_spectral));
                return {ev,evmat};
			}
			update_ray_physics(ray,1.0f);
            return {ev,evmat};
		}

		inline float eval_le(const VRay& ray,float pwr,float t){
		    //TODO , find a way to retain emissive power in the case of b_do_spectral,
		    //to brute force it i multiply for a constant to make luminance similar to the non spectral case at 6500k
            if(b_do_spectral) return blackbody_planks_law_wl_normalized(t,ray.velocity,ray.wl)*pwr*2.0f;
            return pwr;
		}


		inline vec3f sample_direct_from_light(const VScene& scn, ygl::rng_state& rng, const VRay& wo,const VResult& hit,const VMaterial& material, const ygl::vec3f& n){
            vec3f output = zero3f;
            const int n_lights = scn.emissive_hints.size();
            for(int idl=0;idl<n_lights;idl++){
                vec3f dir = zero3f;
                VResult point = sample_emissive_in_light(scn, rng, idl);
                if(point.found()) dir = normalize(point.wor_pos - hit.wor_pos);
                if(dir!=zero3f){
                    auto ndl = dot(n,dir);
                    if(ndl<=ygl::epsf){continue;}

                    vec3f ln;
                    VResult lres;
                    VRay wi = offsetted_ray(hit.wor_pos, wo, dir, wo.tmin, wo.tmax, n, hit.dist, 2.0f);
                    auto lc = eval_light(scn, rng, wi, n, ln, lres);
                    if (lc != ygl::zero3f && lres.found()) {
                        auto ndil = dot(ln,-wi.d);
                        if(ndil<=ygl::epsf){continue;}

                        auto sa = ygl::distance_squared(hit.wor_pos,lres.wor_pos)/ abs(ndil); //TODO, ricontrollare divisione per angolo solido
                        auto light_pdf = ygl::sample_uniform_index_pdf(n_lights)*sa;
                        auto brdf_pdf = eval_bsdfcos_pdf(material,wi,wo,n);
                        auto mis_pdf = 0.5f*light_pdf + 0.5f*brdf_pdf;
                        if(mis_pdf>ygl::epsf) output += (lc*eval_bsdfcos(material,wi,wo, n))*(1.0f / mis_pdf);
                    }
                }
            }
            if(n_lights>1) output /= n_lights;
            return output;
		}

		inline vec3f sample_direct_from_bsdf(const VScene& scn, ygl::rng_state& rng, const VRay& wo,const VResult& hit,const VMaterial& material, const ygl::vec3f& n){
            std::vector<VSample> samples(0);
            sample_bsdfcos(scn, rng, hit, material, wo, n, samples);
            vec3f output = zero3f;
            VRay wi;

            if(samples.empty()){return zero3f;}
            else if(samples.size()!=1){
                wi = samples[(int)(ygl::get_random_float(rng) * samples.size())].ray;
            }else{ wi = samples[0].ray;}

            if(wi.d!=zero3f){
                auto ndl = dot(n,wi.d);
                if(ndl<=ygl::epsf){return zero3f;}

                vec3f ln;
                VResult lres;
                auto lc = eval_light(scn, rng, wi, n, ln, lres);
                if (lc != ygl::zero3f && lres.found()) {
                    auto ndil = dot(ln,-wi.d);
                    if(ndil<=ygl::epsf) return zero3f;
                    //TODO, considerare il numero di samples generati da sample_bsdfcos
                    auto sa = ygl::distance_squared(hit.wor_pos,lres.wor_pos) / abs(ndil); //TODO, ricontrollare divisione per angolo solido
                    auto light_pdf = scn.emissive_hints.empty() ? 0.0f : ygl::sample_uniform_index_pdf(scn.emissive_hints.size())*sa;
                    auto brdf_pdf = eval_bsdfcos_pdf(material,wi,wo,n);
                    auto mis_pdf = 0.5f*light_pdf + 0.5f*brdf_pdf;
                    if(mis_pdf>ygl::epsf) return (lc*eval_bsdfcos(material,wi,wo, n))*(1.0f / mis_pdf);
                }
            }
            return output;
		}

		inline vec3f eval_direct_mc(const VScene& scn, ygl::rng_state& rng, const VRay& wo,const VResult& hit,const VMaterial& material, const ygl::vec3f& n){
		    if(material.is_delta()) return zero3f;
            vec3f output = zero3f;
            if(scn.emissive_hints.empty()) return sample_direct_from_bsdf(scn,rng,wo,hit,material,n);
            if(b_do_double_direct_sampling){
                auto rnl = ygl::get_random_float(rng);
                if(rnl>0.5f){ //prova prima light sampling
                    output += sample_direct_from_light(scn,rng,wo,hit,material,n);
                    if(cmpf(output,zero3f)){
                        output += sample_direct_from_bsdf(scn,rng,wo,hit,material,n);
                    }
                }else{ //prova prima bsdf sampling
                    output += sample_direct_from_bsdf(scn,rng,wo,hit,material,n);
                    if(cmpf(output,zero3f)){
                        output += sample_direct_from_light(scn,rng,wo,hit,material,n);
                    }
                }
                output*=0.5f;
            }else{ //MIS classico basato su random 1/2
                auto rnl = ygl::get_random_float(rng);
                if(rnl>0.5f){
                    output += sample_direct_from_light(scn,rng,wo,hit,material,n);
                }else{
                    output += sample_direct_from_bsdf(scn,rng,wo,hit,material,n);
                }
            }

            return output;
		}

		inline bool isGathering(const VMaterial& m){
            return m.is_delta();
		}

		struct VPExSample{
            VPExSample(): sample({}),w(zero3f),ge(true),b(0){};
		    VPExSample(VSample samplev,vec3f wv,bool gev,int bv): sample(samplev),w(wv),ge(gev),b(bv){};
            VSample sample;
            vec3f w;
            bool ge;
            int b;
		};


		inline vec3f eval_pixel(const VScene& scn, ygl::rng_state& rng, const VRay& ray) {

			ygl::vec3f output = ygl::zero3f;
            std::vector<VPExSample> s_queue;
            s_queue.reserve(100);
            s_queue.push_back(VPExSample({ray,s_camera},one3f,true,0));

            while(!s_queue.empty()){
                auto ex_sample = s_queue.back();
                s_queue.pop_back();

                auto sample = ex_sample.sample;
                if(ex_sample.w==zero3f){continue;}
                auto w = ex_sample.w;
                auto gather_ke = ex_sample.ge;
                auto b = ex_sample.b;
                auto i_output = zero3f;

                while(true){
                    int n_iters = 0;
                    //TODO, controllare meccanismo del poll volume
                    auto poll = poll_volume(scn, rng, sample.ray);
                    if(poll.is_inside() && !poll.emat.is_transmissive()) {break;}
                    VResult hit = scn.INTERSECT_ALGO( sample.ray, n_max_march_iterations, n_iters);
                    if (!hit.found() || !hit.valid()) { break; }

                    if (b_debug_iterations) {
                        return one3f*((float)n_iters)/((float)n_max_march_iterations);
                    }

                    if (b_debug_normals) {
                        return scn.eval_normals(hit,f_normal_eps);
                    }

                    if(poll.is_inside_transmissive()){
                        w *= calc_beer_law(poll.emat,length(sample.ray.o-hit.wor_pos));
                    }

                    auto material = *hit.mat;
                    auto n = scn.eval_normals(hit, f_normal_eps);
                    material.eval_mutator(rng, hit, n, material);
                    if(n==zero3f){if(status.bDebugMode){std::cout<<"<!>Normal is zero...\n";}break;}

                    auto wo = -sample.ray;
                    auto wi = sample.ray;

                    if (material.is_emissive()) {
                        if(gather_ke) i_output += w*eval_le(wi,material.e_power,material.e_temp);
                        break;
                    }

                    if(!isGathering(material)){
                        output += w*eval_direct_mc(scn, rng, wo, hit, material, n);
                    }

                    if(!b_do_gi && b>=2) break;

                    std::vector<VSample> samples;
                    sample_bsdfcos(scn, rng, hit,material,wo,n,samples);
                    if(samples.empty()) break;

                    for(int si=1;si<samples.size();si++){ // supporto al branched, se necessario
                        auto ss = samples[si];
                        auto wi = ss.ray;
                        if(wi.d==zero3f) continue;

                        vec3f sw = w;

                        auto pdf = eval_bsdfcos_pdf(material,wi,wo,n);
                        if(pdf<=ygl::epsf) break;
                        sw*= eval_bsdfcos(material,wi,wo,n)/pdf;
                        if(sw==zero3f) break;

                        if (b > 2 && !russian_roulette(rng,sw) && ss.type!=s_transmissive) continue;
                        bool gke = isGathering(material);
                        s_queue.push_back(VPExSample(ss,sw,gke,b+1));
                    }
                    auto prev_sample = sample;
                    sample = samples[0];
                    wi = sample.ray;
                    if(wi.d==zero3f) break;

                    auto pdf = eval_bsdfcos_pdf(material,wi,wo,n);
                    if(pdf<=ygl::epsf) break;
                    w*= eval_bsdfcos(material,wi,wo,n)/pdf;
                    if(w==zero3f) break;


                    if (b > 2 && !russian_roulette(rng,w) && sample.type!=s_transmissive) break;
                    if(status.bDebugMode) std::cout<<"*Main Loop--> b:"<<b<<"--->"<<w.x<<","<<w.y<<","<<w.z<<"\n";
                    gather_ke = isGathering(material);

                    b++;
                };
                output += i_output;
            }

			return output;
		};


		inline VRay eval_camera(const VScene& scn,int i, int j, int wd, int hd,vec2f crn, ygl::rng_state& rng)  {
			auto camera = scn.camera;

			auto lrn = ygl::zero2f;
            vec2f uv;
			if(e_aa_mode==aa_montecarlo){
                crn = ygl::get_random_vec2f(rng);
                uv = ygl::vec2f{ (i + crn.x) / (camera.aspect*hd), 1.0f - (j + crn.y) / hd };
            }else{
                uv = vec2f{
                    (i+(crn.x+0.5f)/n_ssaa_samples)/wd,
                    1.0f-(j+(crn.y+0.5f)/n_ssaa_samples)/hd
                };
            }
			if (camera.aperture>ygl::epsf) {lrn = ygl::get_random_vec2f(rng);}

			auto h = 2 * std::tan(camera.yfov / 2.0f);
			auto w = h * camera.aspect;

			ygl::vec3f o;
			float focus = 1.0f;
			if (camera.aperture <= ygl::epsf) { o = ygl::vec3f{ lrn.x, lrn.y, 0.0f }; }
			else { o = ygl::vec3f{ lrn.x * camera.aperture, lrn.y * camera.aperture, 0.0f };focus = camera.focus; }

			auto q = ygl::vec3f{ w * focus * (uv.x - 0.5f),h * focus * (uv.y - 0.5f), -focus };
			return VRay{ transform_point(camera._frame, o), transform_direction(camera._frame, normalize(q - o)), f_ray_tmin, f_ray_tmax };
		};

		inline void eval_image(const VScene& scn, ygl::rng_state& rng, image3f& img, int width, int height, int j) {
			for (int i = 0; i < width && !status.bStopped; i++) {
				ygl::vec3f color = ygl::zero3f;

				int nssa = n_ssaa_samples;
				if(e_aa_mode!=aa_ssaa){nssa = 1;}

                for(int ssj = 0; ssj<nssa; ssj++){
                    for(int ssi=0;ssi<nssa;ssi++){
                        for (int s = 0; s < n_ray_samples; s++) {
                            vnx::VRay ray = eval_camera(scn, i, j, width, height, vec2f{ssj,ssi}, rng);
                            ray.wl = rand1f_r(rng,f_min_wl,f_max_wl); //380 / 780 (VALORI LIMITE PER LA FUNZIONE DI SPECTRAL SAMPLING)

                            ray.velocity = KC;
                            ray.frequency = ray.velocity / ray.wl;
                            ray.ior = 1.0f;

                            if(b_do_spectral) color += eval_pixel(scn, rng, ray)*spectral_color(ray.wl);
                            else color += eval_pixel(scn, rng, ray);
                        }
                    }
                }
                if(nssa>1)color /= nssa*nssa;
				auto& px = at(img,{i, j});
				px.x += color.x / n_ray_samples;
				px.y += color.y / n_ray_samples;
				px.z += color.z / n_ray_samples;
			}
		}

        inline std::string img_affix(const VScene& scn) const {
		    std::string ss = "";
		    if(b_do_spectral)ss+="_spec";
		    ss += std::string("_spp")+std::to_string(n_ray_samples);
		    if(e_aa_mode==aa_ssaa && n_ssaa_samples>1){
                std::string ssaa_s = std::to_string(n_ssaa_samples);
                ss+=std::string("_ssaa")+ssaa_s+"x"+ssaa_s;
		    }
		    return ss;
		}

	};

};


#endif
