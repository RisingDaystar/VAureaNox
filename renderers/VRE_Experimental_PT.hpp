#ifndef VRE_EXPERIMENTAL_PT_HPP_INCLUDED
#define VRE_EXPERIMENTAL_PT_HPP_INCLUDED

#include "../VRenderer.hpp"
#include <stack>

namespace vnx {
    struct VRE_Experimental_PT : VRenderer {

        enum VDebugPrimary{
            DBG_NONE = 0,
            DBG_EYELIGHT,
            DBG_ITERATIONS,
            DBG_OVERSTEPS,
            DBG_NORMALS
        };

        enum VRenderingMode{
            EYETRACING,
            LIGHTTRACING,
            BIDIRECTIONAL,
        };

        enum VSampleType{
            INVALID,
            DIFFUSE,
            SPECULAR,
            REFRACTED,
            D_SPECULAR,
            D_REFRACTED
        };
        struct VSample{
            VSampleType type = INVALID;
            VRay ray = {};
        };

        enum VVertexType{
            SURFACE,
            EMITTER,
            EYE,
            VOLUME
        };

        enum VTransportMode{
            RADIANCE,
            IMPORTANCE,
        };

        struct VVertex{
            VResult hit;
            vec3d normal;
            VRay wi = {};
            VRay wo = {};
            vec3d weight = one3d;
            VVertexType type = SURFACE;

            double vc = 0.0;
            double vcm = 0.0;
        };

		struct VSdfPoll{
            VResult eres;
            VMaterial emat;

            constexpr bool is_in_participating(){return (emat.k_sca>thrd && eres.vdist<0.0);}
            constexpr bool is_in_not_participating(){return (emat.k_sca>-thrd && emat.k_sca<thrd && eres.vdist<0.0);}
            constexpr bool has_absorption(){return max_element(emat.ka)>thrd;}
            constexpr bool is_in_transmissive(){return (emat.k_sca>-thrd && eres.vdist<0.0);}
            constexpr bool is_stuck(){return (eres.vdist<0.0 || eres.dist<0.0) && !is_in_transmissive();}
		};


        double f_min_wl = 400;
        double f_max_wl = 700;
        double f_ray_tmin = 0.001;
        double f_ray_tmax = 1000.0;
        double f_normal_eps = 0.0001;

        int i_max_march_iterations = 512;
        int i_ray_samples = 1;
        VRenderingMode i_rendering_mode = BIDIRECTIONAL;

        bool b_debug_direct_only = false;
        VDebugPrimary i_debug_primary = DBG_NONE;
        bool b_gather_direct_light = true;
        bool b_gather_indirect_light = true;

        VRE_Experimental_PT(const VFileConfigs& cfgs,std::string section) : VRenderer(cfgs,section) {}
        std::string Type() const {return "ExperimentalPT";}

        using VRenderer::TryGet; //permette di vedere gli altri overload
		inline VRenderingMode TryGet(const std::string& k, VRenderingMode dVal) const {
            auto ms = mConfigsRef.getSection(mSection);
            if(!ms) return dVal;
            auto match = ms->find(k);
            if (match == ms->end()) { return dVal; }

			if(stricmp(match->second,std::string("eyetracing"))) return EYETRACING;
			if(stricmp(match->second,std::string("bidirectional"))) return BIDIRECTIONAL;
			if(stricmp(match->second,std::string("lighttracing"))) return LIGHTTRACING;
			return dVal;
		}

		inline VDebugPrimary TryGet(const std::string& k, VDebugPrimary dVal) const {
            auto ms = mConfigsRef.getSection(mSection);
            if(!ms) return dVal;
            auto match = ms->find(k);
            if (match == ms->end()) { return dVal; }

			if(stricmp(match->second,std::string("none"))) return DBG_NONE;
			if(stricmp(match->second,std::string("eyelight"))) return DBG_EYELIGHT;
			if(stricmp(match->second,std::string("iterations"))) return DBG_ITERATIONS;
			if(stricmp(match->second,std::string("oversteps"))) return DBG_OVERSTEPS;
			if(stricmp(match->second,std::string("normals"))) return DBG_NORMALS;
			return dVal;
		}

        void Init(){
            VRenderer::Init();
			i_ray_samples = TryGet("i_ray_samples", i_ray_samples);if (i_ray_samples <= 0) { throw VException("i_ray_samples <= 0"); }
			i_max_march_iterations = TryGet("VAureaNox","i_max_march_iterations", i_max_march_iterations);
			f_ray_tmin = TryGet("VAureaNox","f_ray_tmin", f_ray_tmin);
			f_ray_tmax = TryGet("VAureaNox","f_ray_tmax", f_ray_tmax);
			f_normal_eps = TryGet("VAureaNox","f_normal_eps", f_normal_eps);
			f_min_wl = TryGet("f_min_wl", f_min_wl);if (f_min_wl <= 0.0) { throw VException("f_min_wl <= 0.0"); }
			f_max_wl = TryGet("f_max_wl", f_min_wl);if (f_max_wl <= 0.0) { throw VException("f_max_wl <= 0.0"); }
			i_rendering_mode = TryGet("i_rendering_mode",i_rendering_mode);
			b_debug_direct_only = TryGet("b_debug_direct_only",b_debug_direct_only);
			i_debug_primary = TryGet("i_debug_primary",i_debug_primary);
			b_gather_direct_light = TryGet("b_gather_direct_light",b_gather_direct_light);
			b_gather_indirect_light = TryGet("b_gather_indirect_light",b_gather_indirect_light);
        };

        void PostInit(VScene& scn){
            if(i_rendering_mode==EYETRACING) std::cout<<"**Using Eyetracing\n";
            else if(i_rendering_mode==LIGHTTRACING) std::cout<<"**Using Lighttracing\n";
            else if(i_rendering_mode==BIDIRECTIONAL) std::cout<<"**Using Bidirectional\n";
            std::cout<<"**Using "<<i_ray_samples<<" Spp\n\n";
        }

        std::string ImgAffix(const VScene& scn) const{
            std::string ss = "";
            if(i_rendering_mode == EYETRACING) ss+="_eyetracing";
            else if(i_rendering_mode == LIGHTTRACING) ss+="_lighttracing";
            else if(i_rendering_mode == BIDIRECTIONAL) ss+="_bidirectional";
            ss+="_spp"+std::to_string(i_ray_samples);
            return ss;
        }

        constexpr static const char* MtlTypeToString(VMaterialType type){
            if(type==diffuse) return   "diffuse";
            if(type==conductor) return "conductor";
            if(type==dielectric) return "dielectric";
            return "undefined";
        }

        constexpr static const char* SampleTypeToString(VSampleType type){
            if(type==D_REFRACTED) return   "delta_refracted";
            if(type==D_SPECULAR) return "delta_specular";
            if(type==SPECULAR) return "specular";
            if(type==REFRACTED) return "refracted";
            if(type==DIFFUSE) return "diffuse";
            return "undefined";
        }

        constexpr static bool isDeltaSample(const VSample& sample){
            const auto st = sample.type;
            return st == D_SPECULAR || st == D_REFRACTED;
        }

        template<typename T,int N>
        constexpr static bool isTracingInvalid(const vec<T,N>& v){
            return (is_zero_or_has_ltz(v) || has_nan(v) || has_inf(v));
        }

        template<typename T>
        constexpr static  bool isTracingInvalid(const T& v){
            return (v<=0.0 || std::isnan(v) || std::isinf(v));
        }

        constexpr static bool isTracingInvalid(const VRay& v){
            return (cmpf(v.d,zero3d) || isTracingInvalid(v.wl));
        }

        constexpr static bool isTracingInvalid(const VSample& v){
            return v.type==INVALID || isTracingInvalid(v.ray);
        }



        ///BSDFS
        void bsdfcos_lambertian(const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf) *pdf=0.0;
            if(bsdf) *bsdf=zero3d;

            auto ndi = std::max(0.0,dot(normal,wi.d));
            if(pdf){
                *pdf = ndi/pid;
            }
            if(bsdf) *bsdf = (mtl.kr/pid)*ndi;
        }
        void bsdfcos_conductor(const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf) *pdf=0.0;
            if(bsdf) *bsdf=zero3d;

            if(mtl.is_delta()){
                if(pdf) *pdf = 1.0;
                if(bsdf) *bsdf = eval_fresnel_conductor(wo.d,normal,wo.ior,mtl.kr);
            }else{
                auto h = normalize(wi.d+wo.d);
                auto ndh = dot(h,normal);
                if(pdf){
                    if(cmpf(ndh,0.0))return;
                    auto odh = dot(wo.d,h);
                    if(cmpf(odh,0.0))return;
                    *pdf = brdf_ggx_pdf(mtl.rs,ndh)/ (4*std::abs(odh));
                }

                if(bsdf){
                    auto ndi = dot(normal,wi.d);
                    auto ndo = dot(normal,wo.d);

                    if(ndo*ndi<epsd)return;
                    auto h = normalize(wi.d+wo.d);
                    if(cmpf(h,zero3d))return;
                    auto ndh = dot(normal,h);
                    auto F = eval_fresnel_conductor(wo.d,h,wo.ior,mtl.kr);
                    *bsdf = (F*(brdf_ggx_DG(mtl.rs,ndi,ndo,ndh)) / (4*std::abs(ndi)*std::abs(ndo)))*std::abs(ndi);
                }
            }
        }
        void bsdfcos_dielectric(const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf) *pdf=0.0;
            if(bsdf) *bsdf=zero3d;

            auto ndi = dot(normal,wi.d);
            auto h = normalize(wi.d+wo.d);
            auto ndh = dot(h,normal);
            if(pdf){
                *pdf =0.5*(std::abs(ndi)/pid);
                if(cmpf(ndh,0.0)){*pdf=0.0;return;}
                auto odh = dot(wo.d,h);
                if(cmpf(odh,0.0)){*pdf=0.0;return;}

                auto d = (brdf_ggx_pdf(mtl.rs,ndh));
                *pdf += 0.5 * (d / (4*std::abs(odh)));
            }

            if(bsdf){
                auto ior = mtl.eval_ior(wo.owl,f_min_wl,f_max_wl,true);
                auto F = eval_fresnel_dielectric(wo.d,normal,wo.ior,ior);
                auto ndo = dot(normal,wo.d);
                auto hdi = dot(h,wi.d);


                auto fd90 = (0.5+(2*hdi*hdi))*mtl.rs;
                auto m1_ndi5 = std::pow((1.0-std::abs(ndi)),5.0);
                auto m1_ndo5 = std::pow((1.0-std::abs(ndo)),5.0);
                *bsdf = (mtl.kr/pid)*(1.0+(fd90-1.0)*m1_ndi5)*(1.0+(fd90-1.0)*m1_ndo5)*(one3d-F);

                if(cmpf(ndh,0.0)){*bsdf=zero3d;return;}
                *bsdf+=brdf_ggx_D(mtl.rs,ndh) / (4.0 * std::abs(dot(wi.d, h)) * std::max(std::abs(ndi), std::abs(ndo))) * F * std::abs(ndi);
            }
        }

        ///

        /////////RAY INITS
        inline void InitRay(ygl::rng_state& rng,VRay& ray){
            init_ray_physics(rng,ray,f_min_wl,f_max_wl);
            ray.tmin = f_ray_tmin;
            ray.tmax = f_ray_tmax;
        }

        inline void InitEyeRay(const VScene& scn,ygl::rng_state& rng,VRay& ray,int i, int j){
            ray = scn.camera.RayCast(i,j,rng,!i_debug_primary ? get_random_vec2f(rng) : zero2f);
            InitRay(rng,ray);
        }
        inline bool InitLightPath(const VScene& scn,ygl::rng_state& rng,VVertex& lvert,VMaterial& mtl){
            int idl;
            VResult emr = scn.sample_emissive(rng,idl);
            if(!emr.isFound()) return false;

            if(emr.vdist<0.0){
                InitRay(rng,lvert.wo);
                emr.getVMaterial(mtl);
                auto normal = scn.eval_normals(emr,f_normal_eps);
                mtl.eval_mutator(rng,emr,normal,mtl);
                if(!mtl.is_emissive()){
                    emr.getMaterial(mtl);
                    mtl.eval_mutator(rng,emr,normal,mtl);
                    if(!mtl.is_emissive()){return false;}
                }

                if(emr.dist>-f_ray_tmin && emr.dist<0.0){
                    double lpdf;
                    auto dir = sample_diffuse<double>(ygl::get_random_vec2f(rng),normal,normal,&lpdf);
                    lvert.wo = offsetted_ray(emr.wor_pos,lvert.wo,dir,f_ray_tmin,f_ray_tmax,normal,emr.dist);
                    lvert.hit = emr;
                    lvert.normal = normal;
                    lvert.type = EMITTER;
                    lvert.weight = (one3d*EvalLe(lvert.wo,mtl.e_power,mtl.e_temp))*adot(lvert.normal,dir)/((lpdf)); //TODO
                    return true;
                }else if(emr.dist<0.0){
                    lvert.wo.o = emr.wor_pos;
                    lvert.wo.d = sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                    auto hit = scn.intersect(lvert.wo,i_max_march_iterations);
                    if(hit.isFound()){ //TODO, valutare se necessario il nuovo materiale...(forse)
                        lvert.normal = scn.eval_normals(hit,f_normal_eps);
                        double lpdf;

                        auto dir = sample_diffuse<double>(ygl::get_random_vec2f(rng),lvert.normal,lvert.normal,&lpdf);
                        lvert.wo = offsetted_ray(hit.wor_pos,lvert.wo,dir,f_ray_tmin,f_ray_tmax,lvert.normal,hit.dist);
                        lvert.hit = hit;
                        lvert.type = EMITTER;
                        lvert.weight = (one3d*EvalLe(lvert.wo,mtl.e_power,mtl.e_temp))*adot(lvert.normal,dir)/((lpdf)); //TODO

                        return true;
                    }
                    return false;
                }
            }else if(emr.dist<=f_ray_tmin){
                InitRay(rng,lvert.wo);
                emr.getMaterial(mtl);
                lvert.normal = scn.eval_normals(emr,f_normal_eps);
                mtl.eval_mutator(rng,emr,lvert.normal,mtl);
                if(!mtl.is_emissive()){
                    emr.getVMaterial(mtl);
                    mtl.eval_mutator(rng,emr,lvert.normal,mtl);
                    if(!mtl.is_emissive()){return false;}
                }


                double lpdf;
                auto dir = sample_diffuse<double>(ygl::get_random_vec2f(rng),lvert.normal,lvert.normal,&lpdf);
                lvert.wo = offsetted_ray(emr.wor_pos,lvert.wo,dir,f_ray_tmin,f_ray_tmax,lvert.normal,emr.dist);
                lvert.hit = emr;
                lvert.type = EMITTER;
                lvert.weight = (one3d*EvalLe(lvert.wo,mtl.e_power,mtl.e_temp))*adot(lvert.normal,dir) /((lpdf)); //TODO
                return true;
            }
            return false;
        }
        ////////

        template<bool upray = true>
		inline VSdfPoll PollVolume(const VScene& scn, ygl::rng_state& rng, VRay& ray){
			auto ev = scn.eval(ray.o);


            VMaterial evmat;
            //fix for nested dielectrics
            if(ev.vdist<0.0 && ev.dist<0.0){ev.getMaterial(evmat);}
            else if(ev.vdist<0.0){ev.getVMaterial(evmat);}
            //TODO : pre-emptive stuck ray handling (else)

            vec3d evnorm = scn.eval_normals(ev,f_normal_eps);
            evmat.eval_mutator(rng, ev, evnorm, evmat);

            if constexpr(upray){
                if(ev.vdist<0.0) {
                    update_ray_physics(ray,evmat.eval_ior(ray.owl,f_min_wl,f_max_wl,true));
                    //SELLMEIER NEEDS VACUUM Wavelength !! : ray.owl;
                }
                else update_ray_physics(ray,1.0);
            }
            return {ev,evmat};
		}

		inline double EvalLe(const VRay& ray,double pwr,double t){
            return blackbody_planks_law_wl_normalized(t,KC / ray.ior,ray.owl)*pwr;
		}

        inline VSample SampleBsdf_transmissive(const VScene& scn,ygl::rng_state& rng,const VResult& hit,const VMaterial& mtl,const vec3d& normal,const VRay& wo,VSdfPoll& poll,VTransportMode transport_mode,double* pdf = nullptr,vec3d* bsdf = nullptr){
            bool outside = dot(wo.d,normal) >= 0;
            vec3d offBy = outside ? -normal : normal;


            auto poll_ray = offsetted_ray(hit.wor_pos,wo,-normal,wo.tmin,wo.tmax,-normal,hit.dist);
            auto vpoll = PollVolume(scn,rng,poll_ray);
            if(vpoll.is_stuck())return {};
            auto etat = poll_ray.ior;

            poll_ray = offsetted_ray(hit.wor_pos,wo,normal,wo.tmin,wo.tmax,normal,hit.dist);
            vpoll = PollVolume(scn,rng,poll_ray);
            if(vpoll.is_stuck())return {};
            auto etai = poll_ray.ior;

            double F = eval_fresnel_dielectric(wo.d,normal,etai,etat);


            if(cmpf(F,0.0)){
                auto refr_dir = vnx::refract<true>(-wo.d,normal,etai,etat);
                auto wi = offsetted_ray(hit.wor_pos,wo,refr_dir,wo.tmin,wo.tmax,offBy,hit.dist);
                if(same_hemisphere(normal,wo,wi)) return VSample{D_REFRACTED,{}};

                poll = PollVolume(scn,rng,wi);
                if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<MtlTypeToString(poll.emat.type)<<" after event : "<<SampleTypeToString(D_REFRACTED)<<" for "<<MtlTypeToString(mtl.type)<<" material \n";} return {};}

                if(pdf) *pdf = 1.0;
                if(bsdf) {
                    *bsdf = one3d;
                    if(transport_mode==RADIANCE) {
                        if(outside) std::swap(etai,etat);
                        *bsdf *= (etai*etai) / (etat*etat);
                    }
                }
                return VSample{D_REFRACTED,wi};
            }else if(cmpf(F,1.0)){
                auto refl_dir = vnx::reflect(-wo.d,outside ? normal : -normal); //
                auto wi = offsetted_ray(hit.wor_pos,wo,refl_dir,wo.tmin,wo.tmax,-offBy,hit.dist);
                if(!same_hemisphere(normal,wo,wi)) return VSample{D_SPECULAR,{}};

                poll = PollVolume(scn,rng,wi);
                if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<MtlTypeToString(poll.emat.type)<<" after event : "<<SampleTypeToString(D_SPECULAR)<<" for "<<MtlTypeToString(mtl.type)<<" material \n";} return {};}

                if(pdf) *pdf = 1.0;
                if(bsdf) *bsdf = toVec<double,3>(F);
                return VSample{D_SPECULAR,wi};
            }else{
                const double rv = F;
                if(get_random_float(rng)>rv){
                    auto refr_dir = vnx::refract<true>(-wo.d,normal,etai,etat);
                    auto wi = offsetted_ray(hit.wor_pos,wo,refr_dir,wo.tmin,wo.tmax,offBy,hit.dist);
                    if(same_hemisphere(normal,wo,wi)) return VSample{D_REFRACTED,{}};

                    poll = PollVolume(scn,rng,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<MtlTypeToString(poll.emat.type)<<" after event : "<<SampleTypeToString(D_REFRACTED)<<" for "<<MtlTypeToString(mtl.type)<<" material \n";} return {};}

                    if(pdf) *pdf = (1.0-rv);
                    if(bsdf) {
                        *bsdf = (one3d-F);
                        if(transport_mode==RADIANCE) {
                            if(outside) std::swap(etai,etat);
                            *bsdf *= (etai*etai) / (etat*etat);
                        }
                    }
                    return VSample{D_REFRACTED,wi};
                }else{
                    auto refl_dir = vnx::reflect(-wo.d,outside ? normal : -normal); //outside ? normal : -normal
                    auto wi = offsetted_ray(hit.wor_pos,wo,refl_dir,wo.tmin,wo.tmax,-offBy,hit.dist);
                    if(!same_hemisphere(normal,wo,wi)) return VSample{D_SPECULAR,{}};

                    poll = PollVolume(scn,rng,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<MtlTypeToString(poll.emat.type)<<" after event : "<<SampleTypeToString(D_SPECULAR)<<" for "<<MtlTypeToString(mtl.type)<<" material \n";} return {};}

                    if(pdf) *pdf = rv;
                    if(bsdf) *bsdf = toVec<double,3>(F);
                    return VSample{D_SPECULAR,wi};
                }
            }
            return {};
        }

        inline VSample SampleBsdf(const VScene& scn,ygl::rng_state& rng,const VResult& hit,const VMaterial& mtl,const vec3d& normal,const VRay& wo,VSdfPoll& poll,VTransportMode transport_mode,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf)*pdf=0.0;
            if(bsdf)*bsdf=zero3d;

            if(mtl.is_transmissive()){
                if(mtl.is_delta()){
                    return SampleBsdf_transmissive(scn,rng,hit,mtl,normal,wo,poll,transport_mode,pdf,bsdf);
                }else{
                    //ROUGH TRANSMISSION
                }
            }else if(mtl.is_conductor()){
                if(mtl.is_delta()){
                    VRay wi = offsetted_ray(hit.wor_pos,wo,ygl::reflect(wo.d,normal),wo.tmin,wo.tmax,normal,hit.dist);
                    if(!same_hemisphere(normal,wo,wi)) return VSample{D_SPECULAR,{}};

                    poll = PollVolume(scn,rng,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<SampleTypeToString(D_SPECULAR)<<" - "<<MtlTypeToString(mtl.type)<<"\n";} return {};}

                    if (transport_mode==RADIANCE) bsdfcos_conductor(mtl,normal,wi,wo,pdf,bsdf);
                    else bsdfcos_conductor(mtl,normal,wo,wi,pdf,bsdf);
                    return VSample{D_SPECULAR,wi}; //PERFECT SPECULAR
                }else{
                    VRay wi = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(ygl::get_random_vec2f(rng),wo.d,normal,mtl.rs)), wo.tmin, wo.tmax, normal, hit.dist);
                    if(!same_hemisphere(normal,wo,wi)) return VSample{SPECULAR,{}};

                    poll = PollVolume(scn,rng,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<SampleTypeToString(SPECULAR)<<" - "<<MtlTypeToString(mtl.type)<<"\n";} return {};}

                    if (transport_mode==RADIANCE) bsdfcos_conductor(mtl,normal,wi,wo,pdf,bsdf);
                    else bsdfcos_conductor(mtl,normal,wo,wi,pdf,bsdf);
                    return VSample{SPECULAR,wi}; //ROUGH SPECULAR
                }
            }else if(mtl.is_dielectric()){
                    auto rr = ygl::get_random_float(rng);
                    VSample sample;
                    VRay wi;
                    if(rr<0.5){
                        auto rn = ygl::get_random_vec2f(rng);
                        auto offby = normal;//dot(wo.d,n)>=0 ? n : -n;
                        wi = offsetted_ray(hit.wor_pos, wo, sample_diffuse_cos(rn,wo.d,normal), wo.tmin, wo.tmax, offby, hit.dist);
                        if(!same_hemisphere(normal,wo,wi)) return VSample{DIFFUSE,{}};
                        poll = PollVolume(scn,rng,wi);
                        if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<SampleTypeToString(DIFFUSE)<<" - "<<MtlTypeToString(mtl.type)<<"\n";} return {};}

                        sample = VSample{DIFFUSE,wi};
                    }else{
                        auto rn = ygl::get_random_vec2f(rng);
                        auto offby = normal;//dot(wo.d,n)>=0 ? n : -n;
                        wi = offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(rn,wo.d,normal,mtl.rs)), wo.tmin, wo.tmax, offby, hit.dist);
                        if(!same_hemisphere(normal,wo,wi)) return VSample{SPECULAR,{}};

                        poll = PollVolume(scn,rng,wi);
                        if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<SampleTypeToString(SPECULAR)<<" - "<<MtlTypeToString(mtl.type)<<"\n";} return {};}

                        sample = VSample{SPECULAR,wi};
                    }

                    if (transport_mode==RADIANCE) bsdfcos_dielectric(mtl,normal,wi,wo,pdf,bsdf);
                    else bsdfcos_dielectric(mtl,normal,wo,wi,pdf,bsdf);
                    return sample;
            }else if(mtl.is_diffuse()){
                VRay wi = offsetted_ray(hit.wor_pos,wo,sample_diffuse_cos(ygl::get_random_vec2f(rng),wo.d,normal),wo.tmin,wo.tmax,normal,hit.dist);
                if(!same_hemisphere(normal,wo,wi)) return VSample{DIFFUSE,{}};
                poll = PollVolume(scn,rng,wi);
                if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<SampleTypeToString(DIFFUSE)<<" - "<<MtlTypeToString(mtl.type)<<"\n";} return {};}

                if (transport_mode==RADIANCE) bsdfcos_lambertian(mtl,normal,wi,wo,pdf,bsdf);
                else bsdfcos_lambertian(mtl,normal,wo,wi,pdf,bsdf);
                return VSample{DIFFUSE,wi};
            }
            return {};
        }

        inline vec3d EvalBsdf_F(const VScene& scn,const VResult& hit,const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo){
            if(mtl.is_delta()) return zero3d;

            if(mtl.is_transmissive()){
                //TODO
            }else if(mtl.is_conductor()){
                if(!same_hemisphere(normal,wo,wi)) return zero3d;
                vec3d c = zero3d;
                bsdfcos_conductor(mtl,normal,wi,wo,nullptr,&c);
                return c;
            }else if(mtl.is_dielectric()){
                if(!same_hemisphere(normal,wo,wi)) return zero3d;
                vec3d c = zero3d;
                bsdfcos_dielectric(mtl,normal,wi,wo,nullptr,&c);
                return c;
            }else if(mtl.is_diffuse()){
                if(!same_hemisphere(normal,wo,wi)) return zero3d;
                vec3d c = zero3d;
                bsdfcos_lambertian(mtl,normal,wi,wo,nullptr,&c);
                return c;
            }
            return zero3d;
        }

        inline double EvalBsdf_Pdf(const VScene& scn,const VResult& hit,const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo){
            if(mtl.is_delta()) return 0.0;

            if(mtl.is_transmissive()){
                    //TODO
            }else if(mtl.is_conductor()){
                if(!same_hemisphere(normal,wo,wi)) return 0.0;
                double pdf = 0.0;
                bsdfcos_conductor(mtl,normal,wi,wo,&pdf);
                return pdf;
            }else if(mtl.is_dielectric()){
                if(!same_hemisphere(normal,wo,wi)) return 0.0;
                double pdf = 0.0;
                bsdfcos_dielectric(mtl,normal,wi,wo,&pdf);
                return pdf;
            }else if(mtl.is_diffuse()){
                if(!same_hemisphere(normal,wo,wi)) return 0.0;
                double pdf = 0.0;
                bsdfcos_lambertian(mtl,normal,wi,wo,&pdf);
                return pdf;
            }
            return 0.0;
        }


        inline vec3d ConnectVertex(const VScene& scn,
                                        ygl::rng_state& rng,
                                        const VResult& hit,
                                        const vec3d& normal,
                                        const VMaterial& mtl,
                                        VVertexType vtype,
                                        const VRay& out,
                                        const VVertex& lp_vertex){
            return zero3d;

        }


        inline vec3d ConnectToEmissive(const VScene& scn,
                                        ygl::rng_state& rng,
                                        const VResult& hit,
                                        const vec3d& normal,
                                        const VMaterial& mtl,
                                        VVertexType vtype,
                                        const VRay& out){
            int idl;
            auto emissive = scn.sample_emissive(rng,idl);
            if(!emissive.isFound()) return zero3d;

            auto dir = normalize(emissive.wor_pos-hit.wor_pos);
            auto to_light = offsetted_ray(hit.wor_pos,out,dir,out.tmin,out.tmax,normal,(vtype==SURFACE) ? hit.dist : out.tmin);

            VResult occ_hit;
            VMaterial occ_mtl;
            vec3d occ_normal;
            vec3d bsdf;
            double pdf;
            auto w = one3d;

            if(vtype==SURFACE){
                occ_hit = scn.intersect(to_light,i_max_march_iterations);
                if(!occ_hit.isFound()) return zero3d;
                occ_hit.getMaterial(occ_mtl);
                occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                occ_mtl.eval_mutator(rng, occ_hit, occ_normal, occ_mtl);
                if(!occ_mtl.is_emissive()) return zero3d;

                bsdf = EvalBsdf_F(scn,hit,mtl,normal,to_light,out);
                if(isTracingInvalid(bsdf)) return zero3d;
                pdf = EvalBsdf_Pdf(scn,hit,mtl,normal,to_light,out);
                if(isTracingInvalid(pdf)) return zero3d;
            }else if(vtype==VOLUME){
                VRay ray = to_light;

                bool found = false;
                while(max(w)>0.0){
                    VSdfPoll poll = PollVolume(scn,rng,ray);
                    if(!poll.is_in_transmissive()) break;
                    occ_hit = scn.intersect(ray,i_max_march_iterations);
                    if(!occ_hit.isFound()) break;
                    if(poll.is_in_transmissive() && poll.has_absorption()){
                        w*=exp(-poll.emat.ka*ygl::distance(ray.o,occ_hit.wor_pos));
                    }
                    occ_hit.getMaterial(occ_mtl);
                    occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                    occ_mtl.eval_mutator(rng, occ_hit, occ_normal, occ_mtl);
                    if(occ_mtl.is_emissive()) {found = true;break;}
                    if(!occ_mtl.is_transmissive() || occ_mtl.is_refractive()) break;
                    auto offby = dot(occ_normal,ray.d)<0.0 ? -occ_normal : occ_normal;
                    ray = offsetted_ray(occ_hit.wor_pos,ray,ray.d,ray.tmin,ray.tmax,offby,occ_hit.dist);
                }
                if(!found) return zero3d;
                pdf = (4.0*pid);
                bsdf = one3d;
                to_light = ray;
            }else{
                return zero3d;
            }



            auto lc = EvalLe(to_light,occ_mtl.e_power,occ_mtl.e_temp); //TODO
            auto cosNDir = dot(occ_normal,-to_light.d);
            //if(cosNDir<epsd) return zero3d;
            auto light_pdf = PdfAtoW(1.0,distance(hit.wor_pos,occ_hit.wor_pos),cosNDir);
            if(light_pdf>0.0){
                auto mw = power_heuristic(light_pdf,pdf);
                auto rd = (mw*bsdf*lc) / light_pdf;
                return w*rd;
            }
            return zero3d;
        }

        inline void ConnectToCamera(const VScene& scn,
                                    ygl::rng_state& rng,
                                    image3d& img,
                                    VCamera& camera,
                                    const VResult& hit,
                                    const vec3d& normal,
                                    const VMaterial& mtl,
                                    VVertexType vtype,
                                    const VRay& in,
                                    const vec3d& w,
                                    int pl
                                    ){

            auto nn_dir = camera.mOrigin-hit.wor_pos;
            auto dir = normalize(nn_dir);
            if(dot(-camera.mForward,-nn_dir)<=0.0) return;

            VRay to_cam;
            vec3d* px = nullptr;
            vec3d bsdf;
            VResult occ_hit;
            VMaterial occ_mtl;
            vec3d tw = one3d;
            vec2i pid = zero2<int>;

            if(vtype==VOLUME){
                    to_cam = offsetted_ray(hit.wor_pos,in,dir,in.tmin,in.tmax,dir,in.tmin);
                    VRay ray = to_cam;

                    while(max(tw)>0.0){
                        VSdfPoll poll = PollVolume(scn,rng,ray);
                        if(!poll.is_in_transmissive()) break;
                        occ_hit = scn.intersect(ray,i_max_march_iterations);

                        if(!occ_hit.isFound()){
                            camera.WorldToPixel(hit.wor_pos,pid);
                            break;
                        }else{
                            auto max_dist = length(hit.wor_pos-camera.mOrigin);
                            auto occ_dist = length(hit.wor_pos-occ_hit.wor_pos);
                            if(occ_dist>max_dist-(f_ray_tmin*2)){
                                camera.WorldToPixel(hit.wor_pos,pid);
                                break;
                            }
                        }

                        if(poll.is_in_transmissive() && poll.has_absorption()){
                            tw*=exp(-poll.emat.ka*ygl::distance(ray.o,occ_hit.wor_pos));
                        }

                        occ_hit.getMaterial(occ_mtl);
                        auto occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                        occ_mtl.eval_mutator(rng, occ_hit, occ_normal, occ_mtl);
                        if(!occ_mtl.is_transmissive() || occ_mtl.is_refractive()) break;
                        auto offby = dot(occ_normal,ray.d)<0.0 ? -occ_normal : occ_normal;
                        ray = offsetted_ray(occ_hit.wor_pos,ray,ray.d,ray.tmin,ray.tmax,offby,occ_hit.dist);
                    }
                    if(!pid.x || !pid.y) return;
                    bsdf = one3d;
                    to_cam = ray;
            }else{
                to_cam = offsetted_ray(hit.wor_pos,in,dir,in.tmin,in.tmax,normal,hit.dist);
                occ_hit = scn.intersect(to_cam,i_max_march_iterations);
                if(!occ_hit.isFound()){
                    camera.WorldToPixel(hit.wor_pos,pid);
                }else{
                    auto max_dist = length(hit.wor_pos-camera.mOrigin);
                    auto occ_dist = length(hit.wor_pos-occ_hit.wor_pos);
                    if(occ_dist>max_dist-(f_ray_tmin*2)){
                        camera.WorldToPixel(hit.wor_pos,pid);
                    }
                }
                if(!pid.x || !pid.y) return;
                if(vtype != EMITTER){
                    bsdf = EvalBsdf_F(scn,hit,mtl,normal,in,to_cam);
                    if(isTracingInvalid(bsdf)) return;
                }
                else{
                    bsdf = toVec<double,3>(EvalLe(in,mtl.e_power,mtl.e_temp));
                }
            }








            const double cosAtCam = dot(-camera.mForward,-dir);
            double cosToCam = dot(normal,dir);
            if(vtype==VOLUME) cosToCam = 1.0;

            const double iptcd = camera.mImagePlaneDist /  cosAtCam;
            const double itsaf = fsipow(iptcd,2)  / cosAtCam;
            const double itsf = itsaf * std::abs(cosToCam) / distance_squared(camera.mOrigin,hit.wor_pos);

            const double stif = 1.0 / itsf;

            auto rd = (w*bsdf) / (double(i_ray_samples)*stif);
            if(isTracingInvalid(rd)) return;
            camera.mFrameBuffer.add(pid,tw*rd*spectral_to_rgb(to_cam.wl),pl);
        }

        inline vec3d Radiance(const VScene& scn, ygl::rng_state& rng,image3d& img,VCamera& camera, const VRay& ray) {
            if(i_debug_primary!=DBG_NONE){
                if(i_debug_primary==DBG_EYELIGHT){
                    VResult hit = scn.intersect(ray,i_max_march_iterations);
                    mStatus.mRaysEvaled++;
                    if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;return zero3d;}
                    VMaterial mtl;
                    vec3d normal = scn.eval_normals(hit,f_normal_eps);
                    hit.getMaterial(mtl);
                    mtl.eval_mutator(rng,hit,normal,mtl);
                    return EvalBsdf_F(scn,hit,mtl,normal,-ray,-ray);
                }else if(i_debug_primary == DBG_ITERATIONS){
                    int iters = 0;
                    VResult hit = scn.intersect(ray,i_max_march_iterations,&iters);
                    mStatus.mRaysEvaled++;
                    if(!hit.isFound()){
                        mStatus.mRaysLost++;
                        if(hit.vdist>0.0)return {0.0,1.0,0.0};
                        if(hit.vdist<0.0 && hit.vdist>-ray.tmin) return {0.0,0.0,1.0};
                        return {1.0,0.0,0.0};
                    }
                    return toVec<double,3>(((float)iters)/((float)i_max_march_iterations));
                }else if(i_debug_primary == DBG_OVERSTEPS){
                    int overs = 0;
                    VResult hit = scn.intersect(ray,i_max_march_iterations,nullptr,&overs);
                    mStatus.mRaysEvaled++;
                    if(!hit.isFound()){
                        mStatus.mRaysLost++;
                        if(hit.vdist>0.0)return {0.0,1.0,0.0};
                        if(hit.vdist<0.0 && hit.vdist>-ray.tmin) return {0.0,0.0,1.0};
                        return {1.0,0.0,0.0};
                    }
                    return toVec<double,3>(((float)overs)/((float)i_max_march_iterations));
                }else if(i_debug_primary == DBG_NORMALS){
                    VResult hit = scn.intersect(ray,i_max_march_iterations);
                    mStatus.mRaysEvaled++;
                    if(!hit.isFound()){
                        mStatus.mRaysLost++;
                        return {0.0,0.0,0.0};
                    }
                    return abs(scn.eval_normals(hit,f_normal_eps));
                }
            }

            std::vector<VVertex> light_path;
            vec3d output = zero3d;

            ///LIGHT PATH
            if(i_rendering_mode!=EYETRACING){
                light_path.reserve(20);
                VVertex vertex;
                VMaterial lmtl;
                if(InitLightPath(scn,rng,vertex,lmtl)){
                    light_path.push_back(vertex);

                    //Connect Light First Vertex to Camera
                    ConnectToCamera(scn,rng,img,camera,vertex.hit,vertex.normal,lmtl,vertex.type,vertex.wo,vertex.weight,0);
                    //

                    vec3d bsdf;
                    double pdf;
                    VSample sampled;
                    VMaterial mtl;
                    VResult hit;
                    vec3d normal;
                    bool lastDelta = true;

                    VSdfPoll poll = PollVolume(scn,rng,vertex.wo);
                    //if(poll.is_stuck()) return zero3d; ///TODO

                    for(int b=0;;b++){ //Trace from light and store path vertices
                        camera.mFrameBuffer.HintScale(b); //TAKES THE MAX between mScale and b parameter, needed to divide framebuffer contribution
                        hit = scn.intersect(vertex.wo,i_max_march_iterations);
                        mStatus.mRaysEvaled++;
                        if(poll.is_in_transmissive()){
                            if(poll.is_in_participating()){
                                double tFar = (!hit.isFound()) ?  vertex.wo.tmax : ygl::distance(vertex.wo.o,hit.wor_pos);
                                double omega = (poll.emat.k_sca);
                                double dist = -std::log(1.0-ygl::get_random_float(rng)) / (omega);

                                pdf = std::exp(-omega*tFar);
                                bsdf = one3d;

                                if(dist<tFar){
                                    auto orig = vertex.wo;
                                    lastDelta = false;
                                    vertex.wo.o = vertex.wo.o+(vertex.wo.d*dist);
                                    poll = PollVolume<true>(scn, rng, vertex.wo);
                                    ConnectToCamera(scn,rng,img,camera,poll.eres,zero3d,poll.emat,VOLUME,-vertex.wo,vertex.weight,b);
                                    vertex.wo.d =  sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                                    vertex.wi = -vertex.wo;
                                    //hit = poll.eres; //REDUNDANT ?
                                    vertex.weight *= (exp(-poll.emat.ka*dist))/(pdf+(4.0*pid));
                                    light_path.push_back(vertex);
                                    //if(poll.emat.is_emissive()) weight *= EvalLe(wi,mtl.e_power,mtl.e_temp); //emitting media : TODO
                                    if(b>3){if(!russian_roulette(rng,vertex.weight)) break;}
                                    //b--;
                                    continue;
                                }else{
                                    if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                                    if(poll.has_absorption()) vertex.weight = vertex.weight*exp(-poll.emat.ka*tFar)/(pdf);
                                }
                            }else if(poll.is_in_not_participating()){
                                if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                                if(poll.has_absorption()) vertex.weight*= exp(-poll.emat.ka*ygl::distance(poll.eres.wor_pos,hit.wor_pos));
                            }
                        }else{
                            if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                        }


                        hit.getMaterial(mtl);
                        normal = scn.eval_normals(hit, f_normal_eps);
                        mtl.eval_mutator(rng, hit, normal, mtl);

                        if(mtl.is_emissive()){ConnectToCamera(scn,rng,img,camera,hit,normal,mtl,EMITTER,-vertex.wo,vertex.weight,0);break;} //TODO : might be able to gather additional path info... needs test
                        if(!mtl.is_delta()){
                            ConnectToCamera(scn,rng,img,camera,hit,normal,mtl,SURFACE,-vertex.wo,vertex.weight,0);
                        }
                        if(b_debug_direct_only)break;

                        sampled = SampleBsdf(scn,rng,hit,mtl,normal,-vertex.wo,poll,VTransportMode::IMPORTANCE,&pdf,&bsdf);//Sample
                        if(isTracingInvalid(sampled)){if(mStatus.bDebugMode){std::cout<<"Primary LT Loop: Invalid Sample ->mtl: "<<MtlTypeToString(mtl.type)<<" for sample : "<<SampleTypeToString(sampled.type)<<"\n";} break;}
                        if(isTracingInvalid(pdf)){if(mStatus.bDebugMode){std::cout<<"Primary LT Loop: Invalid PDF: "<<pdf<<" ->mtl: "<<MtlTypeToString(mtl.type)<<" for sample : "<<SampleTypeToString(sampled.type)<<"\n";} break;}
                        if(isTracingInvalid(bsdf)){if(mStatus.bDebugMode){std::cout<<"Primary LT Loop: Invalid BSDF: "<<bsdf<<" ->mtl: "<<MtlTypeToString(mtl.type)<<" for sample : "<<SampleTypeToString(sampled.type)<<"\n";} break;}


                        if(!isDeltaSample(sampled)){
                            light_path.push_back(vertex);
                            lastDelta = true;
                        }else{lastDelta = false;}
                        vertex.weight = (vertex.weight*bsdf)/pdf;
                        if(b>3){if(!russian_roulette(rng,vertex.weight)) break;}

                        vertex.hit = hit;
                        vertex.normal = normal;
                        vertex.type = SURFACE;
                        vertex.wi = vertex.wo;
                        vertex.wo = sampled.ray;



                    }
                }
            } //IF !UNIDIRECTIONAL
            if(i_rendering_mode==LIGHTTRACING){return zero3d;}


            ///EYE PATH

            bool lastDelta = true;
            vec3d bsdf = one3d;
            double pdf = 1.0;
            vec3d weight = one3d;

            VSample sampled;
            VMaterial mtl;
            VResult hit;
            vec3d normal;
            VRay wi = ray;
            VRay wo = -ray;


            //std::stack<VMaterial> volStack; //TODO


            VSdfPoll poll = PollVolume(scn,rng,wi);
            if(poll.is_stuck()) return zero3d;
            auto o_wl = wi.wl;

            for(int b=0;;b++){ //Trace from eye
                //const auto prev_hit = hit; //could use Poll.eres in indirect calc instead
                hit = scn.intersect(wi,i_max_march_iterations);
                mStatus.mRaysEvaled++;

                if(poll.is_in_transmissive()){
                    if(poll.is_in_participating()){
                        double tFar = (!hit.isFound()) ?  wi.tmax : ygl::distance(wi.o,hit.wor_pos);
                        double omega = (poll.emat.k_sca);
                        double dist = -std::log(1.0-ygl::get_random_float(rng)) / (omega);

                        pdf = std::exp(-omega*tFar);
                        bsdf = one3d;

                        if(dist<tFar){
                            auto orig = wi;
                            lastDelta = false;
                            wi.o = wi.o+(wi.d*dist);
                            poll = PollVolume<true>(scn, rng, wi);
                            output += weight*ConnectToEmissive(scn,rng,poll.eres,zero3d,poll.emat,VOLUME,wo);
                            wi.d =  sample_sphere_direction<double>(ygl::get_random_vec2f(rng));
                            wo = -wi;
                            //hit = poll.eres; //REDUNDANT ?
                            weight *= (exp(-poll.emat.ka*dist))/(pdf+(4.0*pid));
                            //if(poll.emat.is_emissive()) weight *= EvalLe(wi,mtl.e_power,mtl.e_temp); //emitting media : TODO
                            if(b>3){if(!russian_roulette(rng,weight)) break;}
                            b--;
                            continue;
                        }else{
                            if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                            if(poll.has_absorption()) weight = weight*exp(-poll.emat.ka*tFar)/(pdf);
                        }
                    }else if(poll.is_in_not_participating()){
                        if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                        if(poll.has_absorption()) weight*= exp(-poll.emat.ka*ygl::distance(poll.eres.wor_pos,hit.wor_pos));
                    }
                }else{
                    if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                }

                hit.getMaterial(mtl);
                normal = scn.eval_normals(hit, f_normal_eps);
                mtl.eval_mutator(rng, hit, normal, mtl);
                wo = -wi;



                if(mtl.is_emissive()){
                    if(lastDelta){
                        output +=  weight*EvalLe(wi,mtl.e_power,mtl.e_temp);
                        break;
                    }

                    if(!b_gather_indirect_light) break;

                    auto lc = EvalLe(wi,mtl.e_power,mtl.e_temp); //TODO
                    auto cosNDir = dot(normal,wo.d);
                    //if(cosNDir<epsd) break;
                    auto light_pdf = PdfAtoW(1.0,distance(hit.wor_pos,poll.eres.wor_pos),cosNDir); //poll.eres is == prev_hit
                    if(light_pdf>0.0){
                        auto mw = power_heuristic(pdf,light_pdf);

                        auto rd = (weight*mw*lc);
                        output+=rd;
                    }

                    break;
                }


                //DIRECT
                if(!mtl.is_delta() && b_gather_direct_light){
                    auto li = ConnectToEmissive(scn,rng,hit,normal,mtl,SURFACE,wo);
                    if(!isTracingInvalid(li)) output += weight*li;
                }

                //VC ____TODO
                /*
                if(!mtl.is_delta() && i_rendering_mode==BIDIRECTIONAL){
                    for(auto vi=0;vi<light_path;vi++){
                        auto li = ConnectVertex(scn,rng,hit,mtl,normal,wo,light_path[vi]);
                        if(!isTracingInvalid(li)) output += weight*li;
                    }
                }
                */

                if(b_debug_direct_only) break;


                sampled = SampleBsdf(scn,rng,hit,mtl,normal,wo,poll,VTransportMode::RADIANCE,&pdf,&bsdf);//Sample
                if(isTracingInvalid(sampled)){if(mStatus.bDebugMode){std::cout<<"Primary EYE Loop: Invalid Sample ->mtl: "<<MtlTypeToString(mtl.type)<<" for sample : "<<SampleTypeToString(sampled.type)<<"\n";} break;}
                if(isTracingInvalid(pdf)){if(mStatus.bDebugMode){std::cout<<"Primary EYE Loop: Invalid PDF: "<<pdf<<" ->mtl: "<<MtlTypeToString(mtl.type)<<" for sample : "<<SampleTypeToString(sampled.type)<<"\n";} break;}
                if(isTracingInvalid(bsdf)){if(mStatus.bDebugMode){std::cout<<"Primary EYE Loop: Invalid BSDF: "<<bsdf<<" ->mtl: "<<MtlTypeToString(mtl.type)<<" for sample : "<<SampleTypeToString(sampled.type)<<"\n";} break;}


                lastDelta = isDeltaSample(sampled);
                weight = weight*(bsdf / pdf);
                if(b>3){if(!russian_roulette(rng,weight)) break;}

                wi = sampled.ray;
            }

            return output*spectral_to_rgb(o_wl);
        }

		void EvalImageRow(const VScene& scn, ygl::rng_state& rng, image3d& img, int width, int height, int j) {
		    const double f_ray_samples = (double)i_ray_samples;
		    const double f_ray_samples_2 = f_ray_samples*f_ray_samples;
		    const auto f_img_size = vec2f{img.size.x,img.size.y};

			for (int i = 0; i < width && !mStatus.bStopped; i++) {
				vec3d color = zero3d;

                //JITTERED
                for (int s = 0; s < i_ray_samples; s++) {
                    auto px_uv = get_random_vec2f(rng);
                    VRay ray = scn.camera.RayCast(i,j,rng,!i_debug_primary ? (px_uv) : zero2f);
                    InitRay(rng,ray);
                    color += (Radiance(scn, rng, img, const_cast<VCamera&>(scn.camera), ray)/ f_ray_samples);
                }

                //STRATIFIED
                /*
                for(int s=0;s<i_ray_samples;s++){
                    for(int t=0;t<i_ray_samples;t++){
                        VRay ray = scn.camera.RayCast(i,j,rng,!i_debug_primary ? (vec2f{s,t}+get_random_vec2f(rng))/f_ray_samples : zero2f);
                        InitRay(rng,ray);
                        color += (Radiance(scn, rng, img, scn.camera, ray)/ f_ray_samples_2);
                    }
                }*/

				auto& px = at(img,{i, j});
				px += color;
			}
		}

    };
};

#endif // VRE_EXPERIMENTAL_PT_HPP_INCLUDED
