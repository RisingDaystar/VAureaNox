#ifndef VRE_EXPERIMENTAL_PT_HPP_INCLUDED
#define VRE_EXPERIMENTAL_PT_HPP_INCLUDED

////////////////////////////////////////////////////////
////////TTTTTTTT////OOOO//////DDDD///////OOOO///////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT///////OOOO//////DDDD///////OOOO///////////
////////////////////////////////////////////////////////

//<!+>check BDPT MIS
//<!>proper volumetric integration, right now is hackish
//<!>volumetric BDPT MIS
//<!>code cleanup


#include "../VRenderer.hpp"

namespace vnx {
    struct VRE_Experimental_PT : VRenderer {

        /*
        *BDPT MIS documentation and suggested approach from:
        SmallVCM (https://github.com/SmallVCM/SmallVCM/) ,
        Veach's Thesis,
        Vlnas Thesis (https://cescg.org/wp-content/uploads/2018/04/Vlnas-Bidirectional-Path-Tracing-1.pdf)
        */

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
            D_REFRACTED,

            ABSORBED,
            UNSCATTERED,
            SCATTERED
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
            VMaterial mtl;
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

            constexpr bool is_in_participating(){return (emat.sigma_s>thrd && eres.vdist<0.0);}
            constexpr bool is_in_not_participating(){return (emat.sigma_s>-thrd && emat.sigma_s<thrd && eres.vdist<0.0);}
            constexpr bool has_absorption(){return max_element(emat.sigma_a)>thrd;}
            constexpr bool is_in_transmissive(){return (emat.sigma_s>-thrd && eres.vdist<0.0);}
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
        VDebugPrimary i_debug_primary = DBG_NONE;

        bool b_no_scatter = false;
        bool b_gather_eye_direct = true;
        bool b_gather_eye_indirect = true;
        bool b_gather_eye_vc = true;
        bool b_gather_to_camera = true;

        double mSamplesQuot = 1;


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

        void Init(VScene& scn){
            VRenderer::Init(scn);
			i_ray_samples = TryGet("i_ray_samples", i_ray_samples);if (i_ray_samples <= 0) { throw VException("i_ray_samples <= 0"); }
			i_max_march_iterations = TryGet("VAureaNox","i_max_march_iterations", i_max_march_iterations);
			f_ray_tmin = TryGet("VAureaNox","f_ray_tmin", f_ray_tmin);
			f_ray_tmax = TryGet("VAureaNox","f_ray_tmax", f_ray_tmax);
			f_normal_eps = TryGet("VAureaNox","f_normal_eps", f_normal_eps);
			f_min_wl = TryGet("f_min_wl", f_min_wl);if (f_min_wl <= 0.0) { throw VException("f_min_wl <= 0.0"); }
			f_max_wl = TryGet("f_max_wl", f_min_wl);if (f_max_wl <= 0.0) { throw VException("f_max_wl <= 0.0"); }
			i_rendering_mode = TryGet("i_rendering_mode",i_rendering_mode);

			b_no_scatter = TryGet("b_no_scatter",b_no_scatter);

			b_gather_eye_direct = TryGet("b_gather_eye_direct",b_gather_eye_direct);
			b_gather_eye_indirect = TryGet("b_gather_eye_indirect",b_gather_eye_indirect);
			b_gather_eye_vc = TryGet("b_gather_eye_vc",b_gather_eye_vc);
			b_gather_to_camera = TryGet("b_gather_to_camera",b_gather_to_camera);

			i_debug_primary = TryGet("i_debug_primary",i_debug_primary);

        };

        void PostInit(VScene& scn){
            if(useEyetracing()) std::cout<<"**Using Eyetracing\n";
            else if(useLighttracing()) std::cout<<"**Using Lighttracing\n";
            else if(useBidirectional()) std::cout<<"**Using Bidirectional\n";
            std::cout<<"**Using "<<i_ray_samples<<" Spp\n\n";

            mSamplesQuot = i_ray_samples;
        }

        std::string ImgAffix(const VScene& scn) const{
            std::string ss = "";
            if(useEyetracing()) ss+="_eyetracing";
            else if(useLighttracing()) ss+="_lighttracing";
            else if(useBidirectional()) ss+="_bidirectional";
            ss+="_spp"+std::to_string(i_ray_samples);
            return ss;
        }

        constexpr static const char* toString(VMaterialType type){
            if(type==diffuse) return   "diffuse";
            if(type==conductor) return "conductor";
            if(type==dielectric) return "dielectric";
            return "undefined";
        }
        constexpr static const char* toString(VSampleType type){
            if(type==D_REFRACTED) return   "delta_refracted";
            if(type==D_SPECULAR) return "delta_specular";
            if(type==SPECULAR) return "specular";
            if(type==REFRACTED) return "refracted";
            if(type==DIFFUSE) return "diffuse";
            return "undefined";
        }
        constexpr static const char* toString(VVertexType type){
            if(type==SURFACE) return   "surface";
            if(type==VOLUME) return "volume";
            if(type==EMITTER) return "emitter";
            if(type==EYE) return "eye";
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

        inline bool useBidirectional() const{return i_rendering_mode==BIDIRECTIONAL;}
        inline bool useEyetracing() const{return i_rendering_mode==EYETRACING;}
        inline bool useLighttracing() const{return i_rendering_mode==LIGHTTRACING;}

        inline double lightChoosePdf(const VScene& scn){return scn.emissive_hints.size()==0 ? 0.0 : 1.0 / scn.emissive_hints.size();}

        ///BSDFS
        void bsdf_lambertian(const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf) *pdf=0.0;
            if(bsdf) *bsdf=zero3d;

            auto ndi = std::max(0.0,dot(normal,wi.d));
            if(pdf){
                *pdf = ndi/pid;
            }
            if(bsdf) *bsdf = (mtl.kr/pid);
        }

        void bsdf_conductor(const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf) *pdf=0.0;
            if(bsdf) *bsdf=zero3d;

            if(mtl.is_delta()){
                if(pdf) *pdf = 1.0;
                if(bsdf) *bsdf = mtl.kr;//eval_fresnel_conductor(wo.d,normal,wo.ior,mtl.kr); //TODO: FIXME
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
                    auto F = mtl.kr;//eval_fresnel_conductor(wo.d,h,wo.ior,mtl.kr); //TODO: FIXME
                    *bsdf = (F*(brdf_ggx_DG(mtl.rs,ndi,ndo,ndh)) / (4*std::abs(ndi)*std::abs(ndo)));
                }
            }
        }

        void bsdf_dielectric(const VMaterial& mtl,const vec3d& normal,const VRay& wi,const VRay& wo,double* pdf = nullptr,vec3d* bsdf = nullptr){
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
                auto F = eval_fresnel_dielectric(-wo.d,normal,wo.ior,ior);
                auto ndo = dot(normal,wo.d);
                auto hdi = dot(h,wi.d);


                auto fd90 = (0.5+(2*hdi*hdi))*mtl.rs;
                auto m1_ndi5 = std::pow((1.0-std::abs(ndi)),5.0);
                auto m1_ndo5 = std::pow((1.0-std::abs(ndo)),5.0);
                *bsdf = (mtl.kr/pid)*(1.0+(fd90-1.0)*m1_ndi5)*(1.0+(fd90-1.0)*m1_ndo5)*(one3d-F);

                if(cmpf(ndh,0.0)){*bsdf=zero3d;return;}
                *bsdf+=brdf_ggx_D(mtl.rs,ndh) / (4.0 * std::abs(dot(wi.d, h)) * std::max(std::abs(ndi), std::abs(ndo))) * F;
            }
        }

        ///

        /////////RAY INITS
        inline void InitRay(VRng& rng,VRay& ray){
            init_ray_physics(rng,ray,f_min_wl,f_max_wl);
            ray.tmin = f_ray_tmin;
            ray.tmax = f_ray_tmax;
        }

        inline void InitEyeRay(const VScene& scn,VRng& rng,VRay& ray,int i, int j){
            ray = scn.camera.RayCast(i,j,rng,!i_debug_primary ? rng.next_vecd<2>() : zero2d);
            InitRay(rng,ray);
        }

        inline void InitEyePath(const VScene& scn,const VCamera& camera,const VRay& ray,VVertex& evert){


            evert.wi = ray;
            evert.wo = -ray;

            if(useBidirectional()){
                const double cosAtCam = dot(camera.mForward,-ray.d);
                const double iptcd = camera.mImagePlaneDist /  cosAtCam;
                const double itsaf = fsipow(iptcd,2)  / cosAtCam;
                evert.vc = 0.0;
                evert.vcm = 1.0  / itsaf;
            }

            evert.weight = one3d;
        }

        inline bool InitLightPath(const VScene& scn,VRng& rng,VVertex& lvert,VMaterial& mtl){
            uint32_t idl;
            VResult emr = scn.sample_emissive(rng,idl);
            if(!emr.isFound()) return false;

            if(emr.vdist<0.0){
                InitRay(rng,lvert.wo);
                emr.getVMaterial(mtl);
                lvert.normal = scn.eval_normals(emr,f_normal_eps);
                mtl.eval_mutator(emr,lvert.normal,mtl);
                if(!mtl.is_emissive()){
                    emr.getMaterial(mtl);
                    mtl.eval_mutator(emr,lvert.normal,mtl);
                    if(!mtl.is_emissive()){return false;}
                }

                if(emr.dist>-f_ray_tmin && emr.dist<0.0){
                    double lpdf;
                    auto dir = sample_diffuse<double>(rng.next_vecd<2>(),lvert.normal,lvert.normal,&lpdf);
                    lpdf *= lightChoosePdf(scn);
                    lvert.wo = lvert.wo.newOffsetted(emr.wor_pos,dir,lvert.normal,emr.dist); //offsetted_ray(emr.wor_pos,lvert.wo,dir,f_ray_tmin,f_ray_tmax,lvert.normal,emr.dist);
                    lvert.wi = -lvert.wo;
                    lvert.hit = emr;
                    lvert.type = EMITTER;
                    lvert.weight = (one3d*EvalLe(lvert.wo,mtl.e_power,mtl.e_temp))*adot(lvert.normal,dir)/((lpdf)); //TODO

                    if(useBidirectional()){
                        lvert.vcm = lightChoosePdf(scn) / lpdf;
                        lvert.vc = adot(lvert.normal,dir) / lpdf;
                    }

                    return true;
                }else if(emr.dist<0.0){
                    lvert.wo.o = emr.wor_pos;
                    lvert.wo.d = sample_sphere_direction<double>(rng.next_vecd<2>());
                    auto hit = scn.intersect(lvert.wo,i_max_march_iterations);
                    if(hit.isFound()){
                        lvert.normal = scn.eval_normals(hit,f_normal_eps);
                        double lpdf;

                        auto dir = sample_diffuse<double>(rng.next_vecd<2>(),lvert.normal,lvert.normal,&lpdf);
                        lpdf *= lightChoosePdf(scn);

                        lvert.wo = lvert.wo.newOffsetted(hit.wor_pos,dir,lvert.normal,hit.dist);//offsetted_ray(hit.wor_pos,lvert.wo,dir,f_ray_tmin,f_ray_tmax,lvert.normal,hit.dist);
                        lvert.wi = -lvert.wo;
                        lvert.hit = hit;
                        lvert.type = EMITTER;
                        lvert.weight = (one3d*EvalLe(lvert.wo,mtl.e_power,mtl.e_temp))*adot(lvert.normal,dir)/((lpdf)); //TODO

                        if(useBidirectional()){
                            lvert.vcm = lightChoosePdf(scn) / lpdf;
                            lvert.vc = adot(lvert.normal,dir) / lpdf;
                        }

                        return true;
                    }
                    return false;
                }
            }else if(emr.dist<=f_ray_tmin){
                InitRay(rng,lvert.wo);
                emr.getMaterial(mtl);
                lvert.normal = scn.eval_normals(emr,f_normal_eps);
                mtl.eval_mutator(emr,lvert.normal,mtl);
                if(!mtl.is_emissive()){
                    emr.getVMaterial(mtl);
                    mtl.eval_mutator(emr,lvert.normal,mtl);
                    if(!mtl.is_emissive()){return false;}
                }


                double lpdf;
                auto dir = sample_diffuse<double>(rng.next_vecd<2>(),lvert.normal,lvert.normal,&lpdf);
                lpdf *= lightChoosePdf(scn);

                lvert.wo = lvert.wo.newOffsetted(emr.wor_pos,dir,lvert.normal,emr.dist);////offsetted_ray(emr.wor_pos,lvert.wo,dir,f_ray_tmin,f_ray_tmax,lvert.normal,emr.dist);
                lvert.wi = -lvert.wo;
                lvert.hit = emr;
                lvert.type = EMITTER;
                lvert.weight = (one3d*EvalLe(lvert.wo,mtl.e_power,mtl.e_temp))*adot(lvert.normal,dir) /((lpdf)); //TODO

                if(useBidirectional()){
                    lvert.vcm = lightChoosePdf(scn) / lpdf;
                    lvert.vc = adot(lvert.normal,dir) / lpdf;
                }


                return true;
            }
            return false;
        }
        ////////

        template<bool upray = true>
		inline VSdfPoll PollVolume(const VScene& scn,VRay& ray){
			auto ev = scn.eval(ray.o);


            VMaterial evmat;
            //fix for nested dielectrics
            if(ev.vdist<0.0 && ev.dist<0.0){ev.getMaterial(evmat);}
            else if(ev.vdist<0.0){ev.getVMaterial(evmat);}
            //TODO : pre-emptive stuck ray handling (else)

            vec3d evnorm = scn.eval_normals(ev,f_normal_eps);
            evmat.eval_mutator(ev, evnorm, evmat);

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

        inline VSample SampleBsdf_transmissive(const VScene& scn,VRng& rng,const VResult& hit,const VMaterial& mtl,const vec3d& normal,const VRay& wo,VSdfPoll& poll,VTransportMode transport_mode,double& cosOut,double* pdf = nullptr,vec3d* bsdf = nullptr){
            bool outside = dot(wo.d,normal) >= 0;
            vec3d offAlong = outside ? -normal : normal;

            auto poll_ray = wo.newOffsetted(hit.wor_pos,normal,normal,hit.dist);//offsetted_ray(hit.wor_pos,wo,normal,wo.tmin,wo.tmax,normal,hit.dist);
            auto vpoll = PollVolume(scn,poll_ray);
            if(vpoll.is_stuck())return {};
            auto etai = poll_ray.ior;

            poll_ray =  wo.newOffsetted(hit.wor_pos,-normal,-normal,hit.dist);//offsetted_ray(hit.wor_pos,wo,-normal,wo.tmin,wo.tmax,-normal,hit.dist);
            vpoll = PollVolume(scn,poll_ray);
            if(vpoll.is_stuck())return {};
            auto etat = poll_ray.ior;




            double F = eval_fresnel_dielectric(-wo.d,normal,etai,etat);


            if(cmpf(F,0.0)){
                auto refr_dir = vnx::refract<true>(-wo.d,normal,etai,etat);
                auto wi = wo.newOffsetted(hit.wor_pos,refr_dir,offAlong,hit.dist);//offsetted_ray(hit.wor_pos,wo,refr_dir,wo.tmin,wo.tmax,offAlong,hit.dist);
                if(same_hemisphere(normal,wo,wi)) return VSample{D_REFRACTED,{}};

                poll = PollVolume(scn,wi);
                if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<toString(poll.emat.type)<<" after event : "<<toString(D_REFRACTED)<<" for "<<toString(mtl.type)<<" material \n";} return {};}

                if(pdf) *pdf = 1.0;
                if(bsdf) {
                    *bsdf = one3d;
                    if(transport_mode==RADIANCE) {
                        if(!outside) std::swap(etai,etat);
                        *bsdf *= (etai*etai) / (etat*etat);
                    }
                }

                if(transport_mode==RADIANCE) cosOut = adot(normal,wi.d);
                else cosOut = adot(normal,wo.d);
                return VSample{D_REFRACTED,wi};
            }else if(cmpf(F,1.0)){
                auto refl_dir = vnx::reflect(-wo.d,normal); //outside ? normal : -normal
                auto wi = wo.newOffsetted(hit.wor_pos,refl_dir,-offAlong,hit.dist);//offsetted_ray(hit.wor_pos,wo,refl_dir,wo.tmin,wo.tmax,-offAlong,hit.dist);
                if(!same_hemisphere(normal,wo,wi)) return VSample{D_SPECULAR,{}};

                poll = PollVolume(scn,wi);
                if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<toString(poll.emat.type)<<" after event : "<<toString(D_SPECULAR)<<" for "<<toString(mtl.type)<<" material \n";} return {};}

                if(pdf) *pdf = 1.0;
                if(bsdf) *bsdf = toVec<double,3>(F);

                if(transport_mode==RADIANCE) cosOut = adot(normal,wi.d);
                else cosOut = adot(normal,wo.d);
                return VSample{D_SPECULAR,wi};
            }else{
                const double rv = F;
                if(rng.next_double()>rv){
                    auto refr_dir = vnx::refract<true>(-wo.d,normal,etai,etat);
                    auto wi = wo.newOffsetted(hit.wor_pos,refr_dir,offAlong,hit.dist);//offsetted_ray(hit.wor_pos,wo,refr_dir,wo.tmin,wo.tmax,offAlong,hit.dist);
                    if(same_hemisphere(normal,wo,wi)) return VSample{D_REFRACTED,{}};

                    poll = PollVolume(scn,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<toString(poll.emat.type)<<" after event : "<<toString(D_REFRACTED)<<" for "<<toString(mtl.type)<<" material \n";} return {};}

                    if(pdf) *pdf = (1.0-rv);
                    if(bsdf) {
                        *bsdf = (one3d-F);
                        if(transport_mode==RADIANCE) {
                            if(!outside) std::swap(etai,etat);
                            *bsdf *= (etai*etai) / (etat*etat);
                        }
                    }

                    if(transport_mode==RADIANCE) cosOut = adot(normal,wi.d);
                    else cosOut = adot(normal,wo.d);
                    return VSample{D_REFRACTED,wi};
                }else{
                    auto refl_dir = vnx::reflect(-wo.d,normal); //outside ? normal : -normal
                    auto wi = wo.newOffsetted(hit.wor_pos,refl_dir,-offAlong,hit.dist);//offsetted_ray(hit.wor_pos,wo,refl_dir,wo.tmin,wo.tmax,-offAlong,hit.dist);
                    if(!same_hemisphere(normal,wo,wi)) return VSample{D_SPECULAR,{}};

                    poll = PollVolume(scn,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck in "<<toString(poll.emat.type)<<" after event : "<<toString(D_SPECULAR)<<" for "<<toString(mtl.type)<<" material \n";} return {};}

                    if(pdf) *pdf = rv;
                    if(bsdf) *bsdf = toVec<double,3>(F);

                    if(transport_mode==RADIANCE) cosOut = adot(normal,wi.d);
                    else cosOut = adot(normal,wo.d);
                    return VSample{D_SPECULAR,wi};
                }
            }
            return {};
        }

        inline VSample SampleBsdf(const VScene& scn,VRng& rng,VVertex& vertex,const VRay& wo,VSdfPoll& poll,VTransportMode transport_mode,double& cosOut,double* pdf = nullptr,vec3d* bsdf = nullptr){
            if(pdf)*pdf=0.0;
            if(bsdf)*bsdf=zero3d;


            const VResult& hit = vertex.hit;
            const VMaterial& mtl = vertex.mtl;
            const vec3d& normal = vertex.normal;

            if(mtl.is_transmissive()){
                if(mtl.is_delta()){
                    return SampleBsdf_transmissive(scn,rng,hit,mtl,normal,wo,poll,transport_mode,cosOut,pdf,bsdf);
                }else{
                    //ROUGH TRANSMISSION
                }
            }else if(mtl.is_conductor()){
                if(mtl.is_delta()){
                    VRay wi = wo.newOffsetted(hit.wor_pos,vnx::reflect(-wo.d,normal),normal,hit.dist);//offsetted_ray(hit.wor_pos,wo,ygl::reflect(wo.d,normal),wo.tmin,wo.tmax,normal,hit.dist);
                    if(!same_hemisphere(normal,wo,wi)) return VSample{D_SPECULAR,{}};

                    poll = PollVolume(scn,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<toString(D_SPECULAR)<<" - "<<toString(mtl.type)<<"\n";} return {};}

                    if (transport_mode==RADIANCE) {
                        bsdf_conductor(mtl,normal,wi,wo,pdf,bsdf);
                        cosOut = adot(normal,wi.d);
                    }else {
                        bsdf_conductor(mtl,normal,wo,wi,pdf,bsdf);
                        cosOut = adot(normal,wo.d);
                    }

                    return VSample{D_SPECULAR,wi}; //PERFECT SPECULAR
                }else{
                    VRay wi = wo.newOffsetted(hit.wor_pos,vnx::reflect(-wo.d,sample_ggx(rng.next_vecd<2>(),wo.d,normal,mtl.rs)),normal,hit.dist);//offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(ygl::get_random_vec2f(rng),wo.d,normal,mtl.rs)), wo.tmin, wo.tmax, normal, hit.dist);
                    if(!same_hemisphere(normal,wo,wi)) return VSample{SPECULAR,{}};

                    poll = PollVolume(scn,wi);
                    if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<toString(SPECULAR)<<" - "<<toString(mtl.type)<<"\n";} return {};}

                    if (transport_mode==RADIANCE) {
                        bsdf_conductor(mtl,normal,wi,wo,pdf,bsdf);
                        cosOut = adot(normal,wi.d);
                    }else {
                        bsdf_conductor(mtl,normal,wo,wi,pdf,bsdf);
                        cosOut = adot(normal,wo.d);
                    }

                    return VSample{SPECULAR,wi}; //ROUGH SPECULAR
                }
            }else if(mtl.is_dielectric()){
                    auto rr = rng.next_double();
                    VRay wi;
                    VSample sample;
                    if(rr<0.5){
                        auto rn = rng.next_vecd<2>();
                        auto offAlong = normal;//dot(wo.d,n)>=0 ? n : -n;
                        wi = wo.newOffsetted(hit.wor_pos,sample_diffuse_cos(rn,wo.d,normal),offAlong,hit.dist);//offsetted_ray(hit.wor_pos, wo, sample_diffuse_cos(rn,wo.d,normal), wo.tmin, wo.tmax, offAlong, hit.dist);
                        if(!same_hemisphere(normal,wo,wi)) return VSample{DIFFUSE,{}};
                        poll = PollVolume(scn,wi);
                        if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<toString(DIFFUSE)<<" - "<<toString(mtl.type)<<"\n";} return {};}

                        sample = VSample{DIFFUSE,wi};
                    }else{
                        auto rn = rng.next_vecd<2>();
                        auto offAlong = normal;//dot(wo.d,n)>=0 ? n : -n;
                        wi = wo.newOffsetted(hit.wor_pos,vnx::reflect(-wo.d,sample_ggx(rn,wo.d,normal,mtl.rs)),offAlong,hit.dist);//offsetted_ray(hit.wor_pos, wo, ygl::reflect(wo.d,sample_ggx(rn,wo.d,normal,mtl.rs)), wo.tmin, wo.tmax, offAlong, hit.dist);
                        if(!same_hemisphere(normal,wo,wi)) return VSample{SPECULAR,{}};

                        poll = PollVolume(scn,wi);
                        if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<toString(SPECULAR)<<" - "<<toString(mtl.type)<<"\n";} return {};}

                        sample = VSample{SPECULAR,wi};
                    }

                    if (transport_mode==RADIANCE) {
                        bsdf_dielectric(mtl,normal,wi,wo,pdf,bsdf);
                        cosOut = adot(normal,wi.d);
                    }else {
                        bsdf_dielectric(mtl,normal,wo,wi,pdf,bsdf);
                        cosOut = adot(normal,wo.d);
                    }

                    return sample;
            }else if(mtl.is_diffuse()){
                VRay wi = wo.newOffsetted(hit.wor_pos,sample_diffuse_cos(rng.next_vecd<2>(),wo.d,normal),normal,hit.dist);//offsetted_ray(hit.wor_pos,wo,sample_diffuse_cos(ygl::get_random_vec2f(rng),wo.d,normal),wo.tmin,wo.tmax,normal,hit.dist);
                if(!same_hemisphere(normal,wo,wi)) return VSample{DIFFUSE,{}};
                poll = PollVolume(scn,wi);
                if(poll.is_stuck()){if(mStatus.bDebugMode){std::cout<<"SampleBsdf: Ray is stuck after : "<<toString(DIFFUSE)<<" - "<<toString(mtl.type)<<"\n";} return {};}

                if (transport_mode==RADIANCE) {
                    bsdf_lambertian(mtl,normal,wi,wo,pdf,bsdf);
                    cosOut = adot(normal,wi.d);
                }else {
                    bsdf_lambertian(mtl,normal,wo,wi,pdf,bsdf);
                    cosOut = adot(normal,wo.d);
                }

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
                bsdf_conductor(mtl,normal,wi,wo,nullptr,&c);
                return c;
            }else if(mtl.is_dielectric()){
                if(!same_hemisphere(normal,wo,wi)) return zero3d;
                vec3d c = zero3d;
                bsdf_dielectric(mtl,normal,wi,wo,nullptr,&c);
                return c;
            }else if(mtl.is_diffuse()){
                if(!same_hemisphere(normal,wo,wi)) return zero3d;
                vec3d c = zero3d;
                bsdf_lambertian(mtl,normal,wi,wo,nullptr,&c);
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
                bsdf_conductor(mtl,normal,wi,wo,&pdf);
                return pdf;
            }else if(mtl.is_dielectric()){
                if(!same_hemisphere(normal,wo,wi)) return 0.0;
                double pdf = 0.0;
                bsdf_dielectric(mtl,normal,wi,wo,&pdf);
                return pdf;
            }else if(mtl.is_diffuse()){
                if(!same_hemisphere(normal,wo,wi)) return 0.0;
                double pdf = 0.0;
                bsdf_lambertian(mtl,normal,wi,wo,&pdf);
                return pdf;
            }
            return 0.0;
        }

        inline vec3d EvalOcclusion_ToCamera(const VScene& scn,const VResult& hit,const VCamera& camera,VRay& to_cam,bool& intersected,vec2i& pixel_id){
            vec3d tr_w = one3d;
            VMaterial occ_mtl;
            VResult occ_hit;
            vec3d occ_normal;
            intersected = false;

            auto max_dist = distance(hit.wor_pos,camera.mOrigin);

            while(max_element(tr_w)>0.0){
                VSdfPoll poll = PollVolume(scn,to_cam);
                occ_hit = scn.intersect(to_cam,i_max_march_iterations);

                if(!occ_hit.isFound()){
                    intersected = camera.WorldToPixel(hit.wor_pos,pixel_id);
                    if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                        tr_w*=exp(-poll.emat.sigma_t_vec()*distance(poll.eres.wor_pos,camera.mOrigin));
                    }
                    break;
                }else{

                    auto occ_dist = distance(hit.wor_pos,occ_hit.wor_pos);
                    if(occ_dist>max_dist-(f_ray_tmin*2.0)){
                        intersected = camera.WorldToPixel(hit.wor_pos,pixel_id);
                        if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                            tr_w*=exp(-poll.emat.sigma_t_vec()*distance(poll.eres.wor_pos,camera.mOrigin));
                        }
                        break;
                    }
                }

                if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                    tr_w*=exp(-poll.emat.sigma_t_vec()*ygl::distance(to_cam.o,occ_hit.wor_pos));
                }

                occ_hit.getMaterial(occ_mtl);
                occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                occ_mtl.eval_mutator( occ_hit, occ_normal, occ_mtl);
                if(!occ_mtl.is_transmissive() || occ_mtl.is_refractive()) break;

                bool outside = dot(occ_normal,to_cam.d)<0.0;
                //if(outside) break; //AVOID bypassing other volumes properties [Needs Testing]
                to_cam = to_cam.newOffsetted(occ_hit.wor_pos,to_cam.d,outside ? -occ_normal : occ_normal,occ_hit.dist);
            }
            if(!pixel_id.x || !pixel_id.y) return zero3d;
            return tr_w;
        }

        inline vec3d EvalOcclusion_ToEmissive(const VScene& scn,const VResult& hit,VRay& to_light,VResult& occ_hit,VMaterial& occ_mtl,vec3d& occ_normal,bool& found){
            vec3d tr_w = one3d;
            found = false;
            VSdfPoll poll = PollVolume(scn,to_light);
            while(max_element(tr_w)>0.0){
                if(poll.is_stuck()) return zero3d;
                occ_hit = scn.intersect(to_light,i_max_march_iterations);
                if(!occ_hit.isFound()){return zero3d;}

                if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                    tr_w*=exp(-poll.emat.sigma_t_vec()*ygl::distance(to_light.o,occ_hit.wor_pos));
                }
                occ_hit.getMaterial(occ_mtl);
                occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                occ_mtl.eval_mutator(occ_hit, occ_normal, occ_mtl);
                if(occ_mtl.is_emissive()) {found = true;break;}
                if(!occ_mtl.is_transmissive() || occ_mtl.is_refractive()) return zero3d;

                bool outside = dot(occ_normal,to_light.d)<0.0;
                if(outside) return zero3d; //AVOID bypassing other volumes properties
                //auto offAlong = outside ? -occ_normal : occ_normal;
                to_light = to_light.newOffsetted(occ_hit.wor_pos,to_light.d,occ_normal,occ_hit.dist);
                poll = PollVolume(scn,to_light);
            }
            if(!found) return zero3d;
            return tr_w;
        }

        inline vec3d EvalOcclusion_ToVolVertex(const VScene& scn,const VResult& hit,const VResult& vhit,VRay to_vertex,bool& found){ //TODO
            vec3d tr_w = one3d;
            found = false;
            auto dist = distance(hit.wor_pos,vhit.wor_pos);
            VResult occ_hit;
            VMaterial occ_mtl;
            vec3d occ_normal;

            while(max_element(tr_w)>0.0){
                VSdfPoll poll = PollVolume(scn,to_vertex);
                auto occ_hit = scn.intersect(to_vertex,i_max_march_iterations);
                if(!occ_hit.isFound()){ //VERTEX NOT OCCLUDED
                    found = true;
                    if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                        tr_w*=exp(-poll.emat.sigma_t_vec()*ygl::distance(to_vertex.o,vhit.wor_pos));
                    }
                    break;
                }

                auto occ_d = distance(hit.wor_pos,occ_hit.wor_pos);

                if(occ_d>dist){//VERTEX NOT OCCLUDED
                  if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                      tr_w*=exp(-poll.emat.sigma_t_vec()*ygl::distance(to_vertex.o,vhit.wor_pos));
                  }
                  found = true;
                  break;
                }

                //CONTINUE (IF HITTED A TRANSMISSIVE NOT REFRACTIVE)
                if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                    tr_w*=exp(-poll.emat.sigma_t_vec()*ygl::distance(to_vertex.o,occ_hit.wor_pos));
                }
                occ_hit.getMaterial(occ_mtl);
                occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                occ_mtl.eval_mutator(occ_hit, occ_normal, occ_mtl);
                if(!occ_mtl.is_transmissive() || occ_mtl.is_refractive()) return zero3d;

                bool outside = dot(occ_normal,to_vertex.d)<0.0; //DON'T TEST FOR OUTSIDE | INSIDE , there is no chance for indirect bounce in connectVertices (TODO: TEST)
                //if(outside) return zero3d;
                to_vertex = to_vertex.newOffsetted(occ_hit.wor_pos,to_vertex.d,outside ? -occ_normal : occ_normal,occ_hit.dist);
            }
            return tr_w;
        }

        inline vec3d EvalOcclusion_ToSurfaceVertex(const VScene& scn,const VResult& hit,const VResult& vhit,VRay to_vertex,bool& found){ //TODO
            vec3d tr_w = one3d;
            found = false;
            auto dist = distance(hit.wor_pos,vhit.wor_pos);
            VResult occ_hit;
            VMaterial occ_mtl;
            vec3d occ_normal;

            while(max_element(tr_w)>0.0){
                VSdfPoll poll = PollVolume(scn,to_vertex);
                auto occ_hit = scn.intersect(to_vertex,i_max_march_iterations);
                if(!occ_hit.isFound()){return zero3d;}
                if(poll.is_in_transmissive() && poll.emat.sigma_t()>0.0){
                    tr_w*=exp(-poll.emat.sigma_t_vec()*ygl::distance(to_vertex.o,occ_hit.wor_pos));
                }
                auto occ_d = distance(hit.wor_pos,occ_hit.wor_pos);

                auto theresold = std::abs(occ_hit.dist)+(to_vertex.tmin*2.0);
                if(occ_d>dist+theresold) return zero3d;
                else{
                    if(occ_d>dist-theresold){ //VERTEX NOT OCCLUDED
                        found = true;
                        break;
                    }
                }

                occ_hit.getMaterial(occ_mtl);
                occ_normal = scn.eval_normals(occ_hit, f_normal_eps);
                occ_mtl.eval_mutator(occ_hit, occ_normal, occ_mtl);
                if(!occ_mtl.is_transmissive() || occ_mtl.is_refractive()) return zero3d;

                bool outside = dot(occ_normal,to_vertex.d)<0.0; //DON'T TEST FOR OUTSIDE | INSIDE , there is no chance for indirect bounce in connectVertices (TODO: TEST)
                //if(outside) return zero3d;
                to_vertex = to_vertex.newOffsetted(occ_hit.wor_pos,to_vertex.d,outside ? -occ_normal : occ_normal,occ_hit.dist);
            }
            return tr_w;
        }


        ///TODO
        inline vec3d ConnectVertices(const VScene& scn,
                                     VRng& rng,
                                        const VVertex& ep_vertex,
                                        const VRay& out,
                                        const VVertex& lp_vertex){

            const VResult& hit = ep_vertex.hit;
            const VMaterial& mtl = ep_vertex.mtl;
            const vec3d& normal = ep_vertex.normal;

            auto to_lp_vertex_dir = normalize(lp_vertex.hit.wor_pos-hit.wor_pos);
            const auto to_lp_offAlong = (ep_vertex.type==SURFACE)? normal : to_lp_vertex_dir;
            const auto to_lp_offBy = (ep_vertex.type==SURFACE) ? hit.dist : out.tmin;
            auto to_lp_vertex = out.newOffsetted(hit.wor_pos,to_lp_vertex_dir,to_lp_offAlong,to_lp_offBy);//offsetted_ray(hit.wor_pos,out,to_lp_vertex_dir,out.tmin,out.tmax,to_lp_offAlong,to_lp_offBy);

            auto to_ep_vertex_dir = -to_lp_vertex_dir;
            const auto to_ep_offAlong = (lp_vertex.type==SURFACE)? lp_vertex.normal : to_ep_vertex_dir;
            const auto to_ep_offBy = (lp_vertex.type==SURFACE) ? lp_vertex.hit.dist : out.tmin;
            auto to_ep_vertex = lp_vertex.wi.newOffsetted(hit.wor_pos,to_ep_vertex_dir,to_ep_offAlong,to_ep_offBy);//offsetted_ray(lp_vertex.hit.wor_pos,lp_vertex.wi,to_eye_vertex_dir,out.tmin,out.tmax,to_ep_offAlong,to_ep_offBy);

            auto dist = distance(hit.wor_pos,lp_vertex.hit.wor_pos);


            vec3d ep_bsdf;
            double ep_pdfW;
            double ep_rev_pdfW;
            double cosAtEp;


            vec3d lp_bsdf;
            double lp_pdfW;
            double lp_rev_pdfW;
            double cosAtLp;

            if(lp_vertex.type == SURFACE){
                lp_bsdf = EvalBsdf_F(scn,lp_vertex.hit,lp_vertex.mtl,lp_vertex.normal,lp_vertex.wi,to_ep_vertex);
                if(isTracingInvalid(lp_bsdf)) return zero3d;
                lp_pdfW = EvalBsdf_Pdf(scn,lp_vertex.hit,lp_vertex.mtl,lp_vertex.normal,lp_vertex.wi,to_ep_vertex);
                if(isTracingInvalid(lp_pdfW)) return zero3d;
                lp_rev_pdfW = EvalBsdf_Pdf(scn,lp_vertex.hit,lp_vertex.mtl,lp_vertex.normal,to_ep_vertex,lp_vertex.wi);
                if(isTracingInvalid(lp_rev_pdfW)) return zero3d;
                cosAtLp = dot(lp_vertex.normal,to_ep_vertex_dir);
            }else if(lp_vertex.type == EMITTER){
                lp_bsdf = one3d;
                lp_pdfW = 1.0 / (2.0*pid);
                lp_rev_pdfW = 1.0 / (2.0*pid);
                cosAtLp = dot(lp_vertex.normal,to_ep_vertex_dir);
            }else{
                //TODO, VOLUME
                lp_bsdf = one3d;
                lp_pdfW = 4.0*pid;
                cosAtLp = 1.0;
            }

            VResult occ_hit;
            vec3d tr_w = one3d;
            if(ep_vertex.type==SURFACE){
                ep_bsdf = EvalBsdf_F(scn,hit,mtl,normal,to_lp_vertex,out);
                if(isTracingInvalid(ep_bsdf)) return zero3d;
                ep_pdfW = EvalBsdf_Pdf(scn,hit,mtl,normal,to_lp_vertex,out);
                if(isTracingInvalid(ep_pdfW)) return zero3d;
                ep_rev_pdfW = EvalBsdf_Pdf(scn,hit,mtl,normal,out,to_lp_vertex);
                if(isTracingInvalid(ep_rev_pdfW)) return zero3d;
                cosAtEp = dot(normal,to_lp_vertex_dir);

                //if(!russian_roulette(rng,ep_pdfW))return zero3d;
                //if(!russian_roulette(rng,lp_pdfW))return zero3d;

                VResult occ_hit;
                if(lp_vertex.type!=VOLUME){
                    bool found = false;
                    tr_w = EvalOcclusion_ToSurfaceVertex(scn,hit,lp_vertex.hit,to_lp_vertex,found);
                    if(!found) return zero3d;
                }else{ //TODO
                    bool found = false;
                    tr_w = EvalOcclusion_ToVolVertex(scn,hit,lp_vertex.hit,to_lp_vertex,found);
                    if(!found) return zero3d;
                }
            }else{
                ep_bsdf = one3d;
                ep_pdfW = 1.0 / (4.0*pid);
                ep_rev_pdfW = ep_pdfW;
                cosAtEp = 1.0;

                //if(!russian_roulette(rng,ep_pdfW))return zero3d;
                //if(!russian_roulette(rng,lp_pdfW))return zero3d;

                if(lp_vertex.type!=VOLUME){
                    bool found = false;
                    tr_w = EvalOcclusion_ToSurfaceVertex(scn,hit,lp_vertex.hit,to_lp_vertex,found);
                    if(!found) return zero3d;
                }else{ //TODO
                    bool found = false;
                    tr_w = EvalOcclusion_ToVolVertex(scn,hit,lp_vertex.hit,to_lp_vertex,found);
                    if(!found) return zero3d;
                }
            }



            auto dsqr = dist*dist;
            auto G = cosAtLp * cosAtEp / dsqr;
            if(G<0.0) return zero3d;


            auto ep_pdfA = PdfWtoA(ep_pdfW,dist,cosAtLp);
            auto lp_pdfA = PdfWtoA(lp_pdfW,dist,cosAtEp);


            auto w_lp = ep_pdfA * ((lp_vertex.vcm+lp_vertex.vc)*lp_rev_pdfW);
            auto w_ep = lp_pdfA * ((ep_vertex.vcm+ep_vertex.vc)*ep_rev_pdfW);


            auto mw = 1.0/(w_lp+1.0+w_ep);


            auto c = tr_w*(mw * G) * ep_bsdf * lp_bsdf*spectral_to_rgb(to_ep_vertex.wl);

            return ep_vertex.weight*lp_vertex.weight*c;

        }


        inline vec3d ConnectToEmissive(const VScene& scn,VRng& rng,const VVertex& vertex,const VRay& out){
            uint32_t idl;
            auto emissive = scn.sample_emissive(rng,idl);
            if(!emissive.isFound()) return zero3d;

            const VResult& hit = vertex.hit;
            const VMaterial& mtl = vertex.mtl;
            const vec3d& normal = vertex.normal;
            const VVertexType& vtype = vertex.type;

            auto dir = normalize(emissive.wor_pos-hit.wor_pos);
            auto to_light = out.newOffsetted(hit.wor_pos,dir,(vtype==SURFACE) ? normal : dir,(vtype==SURFACE) ? hit.dist : out.tmin);//offsetted_ray(hit.wor_pos,out,dir,out.tmin,out.tmax,normal,(vtype==SURFACE) ? hit.dist : out.tmin);

            VResult occ_hit;
            VMaterial occ_mtl;
            vec3d occ_normal;
            vec3d bsdf;
            double bsdf_pdfW;


            if(vtype==SURFACE){
                bsdf = EvalBsdf_F(scn,hit,mtl,normal,to_light,out);
                if(isTracingInvalid(bsdf)) return zero3d;
                bsdf_pdfW = EvalBsdf_Pdf(scn,hit,mtl,normal,to_light,out);
                if(isTracingInvalid(bsdf_pdfW)) return zero3d;
            }else if(vtype==VOLUME){
                bsdf_pdfW = 1.0/(4.0*pid);
                bsdf = one3d;
            }else{
                return zero3d;
            }

            bool found = false;
            auto tr_w = EvalOcclusion_ToEmissive(scn,hit,to_light,occ_hit,occ_mtl,occ_normal,found);
            if(!found) return zero3d;

            auto lc = EvalLe(to_light,occ_mtl.e_power,occ_mtl.e_temp);

            auto cosAtLight = adot(occ_normal,-to_light.d);
            auto light_pdfW = PdfAtoW(1.0,distance(hit.wor_pos,occ_hit.wor_pos),cosAtLight);
            if(isTracingInvalid(light_pdfW)) return zero3d;

            if(useBidirectional()){
                const auto bsdf_rev_pdfW = EvalBsdf_Pdf(scn,hit,mtl,normal,out,to_light);
                if(isTracingInvalid(bsdf_rev_pdfW)) return zero3d;

                const auto cosToLight = vtype == VOLUME ? 1.0 : adot(normal,to_light.d);

                const auto emitted_pdfW = 1.0 / (2.0 * pid);//1.0 *cosAtLight*(1.0/pid);

                const double w_light = bsdf_pdfW / (lightChoosePdf(scn)*light_pdfW);
                const double w_camera = (emitted_pdfW * cosToLight / (light_pdfW * cosAtLight)) * ((vertex.vcm + vertex.vc)*bsdf_rev_pdfW);

                const double mw = 1.0 / (w_light + 1.0 + w_camera);


                const vec3d li = (mw*cosToLight/(lightChoosePdf(scn)*light_pdfW))*(lc*bsdf);

                return tr_w*vertex.weight*li;
            }else{


                const auto cosToLight = (vtype == VOLUME) ? 1.0 : adot(normal,to_light.d);
                auto li = tr_w*lc*bsdf*(cosToLight / light_pdfW);
                auto mw = balance_heuristic(light_pdfW*lightChoosePdf(scn),bsdf_pdfW);

                return vertex.weight*mw*li;
            }

            return zero3d;
        }

        inline void ConnectToCamera(const VScene& scn,
                                    image3d& img,
                                    VCamera& camera,
                                    const VVertex& vertex,
                                    const VRay& in
                                    ){

            const VResult& hit = vertex.hit;
            const VMaterial& mtl = vertex.mtl;
            const vec3d& normal = vertex.normal;
            const VVertexType& vtype = vertex.type;


            auto dir = normalize(camera.mOrigin-hit.wor_pos);
            const double cosAtCam = dot(-camera.mForward,-dir);
            if(cosAtCam<=0.0) return;



            VRay to_cam;
            vec3d* px = nullptr;
            vec3d bsdf;
            double bsdf_rev_pdfW = 0.0;
            VResult occ_hit;
            VMaterial occ_mtl;
            vec3d tr_w = one3d;
            vec2i pixel_id = zero2<int>;
            bool intersected = false;

            if(vtype==VOLUME){
                to_cam = in.newOffsetted(hit.wor_pos,dir,dir,in.tmin);//offsetted_ray(hit.wor_pos,in,dir,in.tmin,in.tmax,dir,in.tmin);
                VRay ray = to_cam;
                tr_w = EvalOcclusion_ToCamera(scn,hit,camera,ray,intersected,pixel_id);
                if(!intersected) return;

                bsdf = one3d;
                bsdf_rev_pdfW = 1.0/(4.0*pid);
            }else{
                to_cam = in.newOffsetted(hit.wor_pos,dir,normal,hit.dist);//offsetted_ray(hit.wor_pos,in,dir,in.tmin,in.tmax,normal,hit.dist);


                if(vtype != EMITTER){
                    bsdf = EvalBsdf_F(scn,hit,mtl,normal,in,to_cam);
                    if(isTracingInvalid(bsdf)) return;
                    if(useBidirectional()) {
                        bsdf_rev_pdfW = EvalBsdf_Pdf(scn,hit,mtl,normal,to_cam,in);
                        if(isTracingInvalid(bsdf_rev_pdfW)) return;
                    }
                }else{
                    bsdf = toVec<double,3>(EvalLe(in,mtl.e_power,mtl.e_temp));
                    bsdf_rev_pdfW = 1.0;
                }

                VRay ray = to_cam;
                tr_w = EvalOcclusion_ToCamera(scn,hit,camera,ray,intersected,pixel_id);
                if(!intersected) return;
            }



            double cosToCam = vtype==VOLUME ? 1.0 : dot(normal,dir);
            if(cosToCam<=0.0) return;

            const double iptcd = camera.mImagePlaneDist /  cosAtCam;
            const double itsaf = fsipow(iptcd,2)  / cosAtCam;
            const double itsf = itsaf * std::abs(cosToCam) / distance_squared(camera.mOrigin,hit.wor_pos);

            const double stif = 1.0 / itsf;

            double mw = 1.0;
            double sc_f = mSamplesQuot*stif;

            if(useBidirectional()){
                double w_light = (itsf) * ((vertex.vcm + vertex.vc) * bsdf_rev_pdfW);
                mw = (1.0 / (w_light+1.0));
            }


            auto rd = tr_w*mw*(vertex.weight*bsdf) / sc_f;
            if(isTracingInvalid(rd)) return;
            camera.mFrameBuffer.add(pixel_id,rd*spectral_to_rgb(to_cam.wl));
        }


        inline VSampleType SampleVolumeDist(const VScene& scn,VRng& rng,const VResult& hit,VRay& in,VSdfPoll& poll,double& pdf,double& dist){ //TODO
            double tFar = (!hit.isFound()) ?  in.tmax : ygl::distance(in.o,hit.wor_pos);
            double omega = poll.emat.sigma_t_maj();
            if(isTracingInvalid(omega)) return INVALID;
            dist = 0.0;
            pdf = 1.0/std::exp(-omega*tFar);

            VRay o_in = in;
            VSdfPoll o_poll = poll;
            while(true){
                dist += -std::log(1.0-rng.next_double()) / (omega);
                if(dist>tFar){
                    in = o_in;
                    poll = o_poll;
                    return UNSCATTERED;
                }
                in.o = o_in.o+(in.d*dist);
                poll = PollVolume(scn,in);
                if(poll.is_in_not_participating() || poll.is_stuck()){ //HACK : HANDLES DEGENERATE CASES and nested dielectrics
                    in = o_in;
                    poll = o_poll;
                    return UNSCATTERED;
                }
                omega = poll.emat.sigma_t_maj();
                if(isTracingInvalid(omega)){
                    in = o_in;
                    poll = o_poll;
                    return UNSCATTERED;
                }
                pdf += 1.0/std::exp(-omega*tFar);
                if(rng.next_double()<poll.emat.sigma_s / omega) break;
            }
            if(rng.next_double()<max_element(poll.emat.sigma_a) / omega) return ABSORBED;
            return SCATTERED;
        }

        inline vec3d Radiance(const VScene& scn, VRng& rng,image3d& img,VCamera& camera, const VRay& ray) {
            if(i_debug_primary!=DBG_NONE){
                if(i_debug_primary==DBG_EYELIGHT){
                    VResult hit = scn.intersect(ray,i_max_march_iterations);
                    mStatus.mRaysEvaled++;
                    if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;return zero3d;}
                    VMaterial mtl;
                    vec3d normal = scn.eval_normals(hit,f_normal_eps);
                    hit.getMaterial(mtl);
                    mtl.eval_mutator(hit,normal,mtl);
                    if(!mtl.is_emissive()) return EvalBsdf_F(scn,hit,mtl,normal,-ray,-ray);
                    return one3d*EvalLe(ray,mtl.e_power,mtl.e_temp);
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
            if(!useEyetracing()){
                if(useBidirectional()) light_path.reserve(20);
                VVertex vertex;
                if(InitLightPath(scn,rng,vertex,vertex.mtl)){
                    //if(useBidirectional()) {light_path.push_back(vertex);}

                    if(b_gather_to_camera && !useBidirectional()) ConnectToCamera(scn,img,camera,vertex,vertex.wo);

                    vec3d bsdf;
                    double pdf;
                    VSample sampled;

                    VMaterial& mtl = vertex.mtl;
                    VResult& hit = vertex.hit;
                    vec3d& weight = vertex.weight;
                    VRay& wi = vertex.wi;
                    VRay& wo = vertex.wo;
                    VVertexType& type = vertex.type;
                    vec3d& normal = vertex.normal;
                    auto cosOut = 1.0;

                    VSdfPoll poll = PollVolume(scn,vertex.wo);
                    if(poll.is_stuck()) {return zero3d;}
                    int nd_b = 0;
                    for(int b=0;;b++){ //Trace from light and store path vertices
                        camera.mFrameBuffer.HintScale(nd_b); //TAKES THE MAX between mScale and b parameter, needed to divide framebuffer contribution
                        //camera.mFrameBuffer.HintScale(useLighttracing() ? nd_b : 0); //nd_b;
                        hit = scn.intersect(wo,i_max_march_iterations);
                        mStatus.mRaysEvaled++;
                        if(poll.is_in_transmissive()){
                            if(poll.is_in_participating()){
                                double dist;
                                VSampleType volEvent = SampleVolumeDist(scn,rng,hit,wo,poll,pdf,dist);
                                bsdf = one3d;

                                if(volEvent==SCATTERED){
                                    nd_b++;
                                    //wo.o = wo.o+(wo.d*dist);
                                    //poll = PollVolume<true>(scn, wo);

                                    mtl = poll.emat;
                                    hit = poll.eres;
                                    normal = zero3d;
                                    type = VOLUME;
                                    wi = -wo;


                                    if(useBidirectional()){ //TODO MIS
                                        vertex.vcm *= dist*dist;
                                        //vertex.vcm /= adot(normal,vertex.wo.d);
                                        //vertex.vc /= adot(normal,vertex.wo.d);
                                    }

                                    if(b_gather_to_camera) ConnectToCamera(scn,img,camera,vertex,wi);
                                    wo.d =  sample_sphere_direction<double>(rng.next_vecd<2>());
                                    cosOut = 1.0;

                                    weight *= 1.0 / pdf;
                                    if(b>3 && !russian_roulette(rng,weight)) break;

                                    if(useBidirectional()){//TODO MIS
                                        light_path.push_back(vertex);
                                        vertex.vc = (1.0 / pdf)*((vertex.vc*pdf) + vertex.vcm);
                                        vertex.vcm = 1.0 / pdf;
                                    }

                                    continue;
                                }else if(volEvent==UNSCATTERED){
                                    if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                                    vertex.type = SURFACE;
                                }else{break;}
                            }else if(poll.is_in_not_participating()){
                                if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                                if(poll.has_absorption()) weight*= exp(-poll.emat.sigma_a*ygl::distance(poll.eres.wor_pos,hit.wor_pos));
                                vertex.type = SURFACE;
                            }
                        }else{
                            if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                            vertex.type = SURFACE;
                        }
                        hit.getMaterial(mtl);
                        normal = scn.eval_normals(hit, f_normal_eps);
                        mtl.eval_mutator( hit, normal, mtl);

                        wi = -wo;

                        //SURFACE VC && VCM ---->MIS completion
                        if(useBidirectional()){
                            vertex.vcm *= distance_squared(poll.eres.wor_pos,hit.wor_pos);
                            vertex.vcm /= adot(normal,vertex.wi.d);
                            vertex.vc /= adot(normal,vertex.wi.d);
                        }
                        ////////////////////////

                        if(mtl.is_emissive()){
                            type = EMITTER;
                            if(!useBidirectional() && b_gather_to_camera) ConnectToCamera(scn,img,camera,vertex,wi);
                            break;
                        }
                        if(!mtl.is_delta()){
                            nd_b++;
                            if(b_gather_to_camera) ConnectToCamera(scn,img,camera,vertex,wi);
                            if(useBidirectional()) {
                                light_path.push_back(vertex);
                            }
                        }
                        if(b_no_scatter) break;

                        sampled = SampleBsdf(scn,rng,vertex,wi,poll,VTransportMode::IMPORTANCE,cosOut,&pdf,&bsdf);//Sample
                        if(isTracingInvalid(sampled)){if(mStatus.bDebugMode){std::cout<<"Primary LT Loop: Invalid Sample ->mtl: "<<toString(mtl.type)<<" for sample : "<<toString(sampled.type)<<"\n";} break;}
                        if(isTracingInvalid(pdf)){if(mStatus.bDebugMode){std::cout<<"Primary LT Loop: Invalid PDF: "<<pdf<<" ->mtl: "<<toString(mtl.type)<<" for sample : "<<toString(sampled.type)<<"\n";} break;}
                        if(isTracingInvalid(bsdf)){if(mStatus.bDebugMode){std::cout<<"Primary LT Loop: Invalid BSDF: "<<bsdf<<" ->mtl: "<<toString(mtl.type)<<" for sample : "<<toString(sampled.type)<<"\n";} break;}

                         wo = sampled.ray;
                        //SURFACE VC && VCM ---->MIS Definition
                        if(useBidirectional()){
                            if(isDeltaSample(sampled)){
                                vertex.vc *= cosOut;
                                vertex.vcm = 0.0;
                            }else{
                                double rev_pdf = EvalBsdf_Pdf(scn,hit,mtl,normal,vertex.wi,vertex.wo);
                                if(isTracingInvalid(rev_pdf)) rev_pdf = 0.0;
                                vertex.vc = (cosOut / pdf) *(vertex.vcm + (vertex.vc*rev_pdf));
                                vertex.vcm = 1.0 / pdf;
                            }
                        }

                        ////////////////////////

                        if(isDeltaSample(sampled)) weight = weight*(bsdf / pdf);
                        else weight = weight*(bsdf*cosOut / pdf);
                        if(b>3){if(!russian_roulette(rng,weight)) break;}



                    }
                }
            } //IF !UNIDIRECTIONAL
            if(useLighttracing()){return zero3d;}


            ///EYE PATH

            bool lastDelta = true;
            bool allDelta = true;
            vec3d bsdf = one3d;
            double pdf = 1.0;


            VSample sampled;

            VVertex vertex;
            InitEyePath(scn,camera,ray,vertex);

            VMaterial& mtl = vertex.mtl;
            VResult& hit = vertex.hit;
            vec3d& normal = vertex.normal;
            vec3d& weight = vertex.weight;
            VRay& wi = vertex.wi;
            VRay& wo = vertex.wo;
            double& vc = vertex.vc;
            double& vcm = vertex.vcm;

            auto cosOut = 1.0;

            VSdfPoll poll = PollVolume(scn,wi);
            if(poll.is_stuck()) return zero3d;
            auto gathered_wl = wi.wl;

            auto weight_no_rr = weight;

            for(int b=0;;b++){ //Trace from eye
                hit = scn.intersect(wi,i_max_march_iterations);
                mStatus.mRaysEvaled++;

                if(poll.is_in_transmissive()){
                    if(poll.is_in_participating()){
                        double dist;
                        VSampleType volEvent = SampleVolumeDist(scn,rng,hit,wi,poll,pdf,dist);
                        bsdf = one3d;

                        if(volEvent == SCATTERED){
                            lastDelta = false;
                            allDelta = false;
                            //wi.o = wi.o+(wi.d*dist);
                            //poll = PollVolume<true>(scn, wi);
                            wo = -wi;

                            vertex.type = VOLUME;
                            normal = zero3d;
                            mtl = poll.emat;
                            hit = poll.eres;

                            if(useBidirectional()){//TODO MIS
                                vcm *= dist*dist;
                                //vertex.vcm /= adot(normal,vertex.wo.d);
                                //vertex.vc /= adot(normal,vertex.wo.d);
                            }

                            if(b_gather_eye_direct){
                                auto li = ConnectToEmissive(scn,rng,vertex,wo);
                                if(!isTracingInvalid(li)) output+=li;
                            }
                            if(useBidirectional() && b_gather_eye_vc){
                                for(auto vi=0;vi<light_path.size();vi++){
                                    auto li = ConnectVertices(scn,rng,vertex,wo,light_path[vi]);
                                    if(!isTracingInvalid(li)) output += li;
                                }
                            }

                            wi.d =  sample_sphere_direction<double>(rng.next_vecd<2>());
                            cosOut = 1.0;

                            weight *= 1.0 / pdf;
                            weight_no_rr = weight;
                            if(b>3 && !russian_roulette(rng,weight)) break;

                            if(useBidirectional()){//TODO MIS
                                vc = (1.0 / pdf)*((vc*pdf) + vcm);
                                vcm = 1.0 / pdf;
                            }
                            continue;
                        }else if(volEvent == UNSCATTERED){
                            if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                            vertex.type = SURFACE;
                        }else{break;}
                    }else if(poll.is_in_not_participating()){
                        if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                        if(poll.has_absorption()) weight*= exp(-poll.emat.sigma_a*ygl::distance(poll.eres.wor_pos,hit.wor_pos));
                        vertex.type = SURFACE;
                    }
                }else{
                    if(!hit.isFound() || !hit.isValid()){mStatus.mRaysLost++;break;}
                    vertex.type = SURFACE;
                }

                hit.getMaterial(mtl);
                normal = scn.eval_normals(hit, f_normal_eps);
                mtl.eval_mutator( hit, normal, mtl);
                wo = -wi;

                //SURFACE VC && VCM ---->MIS Completion
                if(useBidirectional()){
                    vcm *= distance_squared(poll.eres.wor_pos,hit.wor_pos);
                    vcm /= adot(normal,wo.d);
                    vc /= adot(normal,wo.d);
                }
                ////////////////////////

                if(mtl.is_emissive()){
                    if(!b_gather_eye_indirect) break;

                    if((lastDelta && !useBidirectional()) || allDelta || b==0){
                        output +=  weight*EvalLe(wi,mtl.e_power,mtl.e_temp);
                        break;
                    }

                    double mw;
                    if(useBidirectional()){
                        auto cosAtLight = adot(normal,wo.d);
                        auto light_pdfW = lightChoosePdf(scn);//PdfAtoW(1.0,distance(hit.wor_pos,poll.eres.wor_pos),cosAtLight);
                        const double emitted_pdfW = lightChoosePdf(scn) / (2.0*pid);
                        const double w_camera = ((light_pdfW*cosAtLight) / (emitted_pdfW*cosOut)) * ((vertex.vcm + vertex.vc)*emitted_pdfW);
                        mw = 1.0 / (1.0 + w_camera);
                    }else{
                        auto cosAtLight = dot(normal,wo.d);
                        auto light_pdfW = PdfAtoW(1.0,distance(hit.wor_pos,poll.eres.wor_pos),cosAtLight);
                        mw = balance_heuristic(pdf,light_pdfW*lightChoosePdf(scn));
                    }


                    auto lc = EvalLe(wi,mtl.e_power,mtl.e_temp);

                    auto rd = (weight_no_rr)*(mw*lc);

                    if(!isTracingInvalid(rd)) output+=rd;

                    break;
                }



                if(!mtl.is_delta()){
                    //Direct
                    if(b_gather_eye_direct){
                        auto li = ConnectToEmissive(scn,rng,vertex,wo);
                        if(!isTracingInvalid(li)) output += li;
                    }
                    //VertexConnect
                    if(b_gather_eye_vc && useBidirectional()){
                        for(auto vi=0;vi<light_path.size();vi++){
                            auto li = ConnectVertices(scn,rng,vertex,wo,light_path[vi]);
                            if(!isTracingInvalid(li)) output += li;
                        }
                    }
                }

                if(b_no_scatter) break;

                sampled = SampleBsdf(scn,rng,vertex,wo,poll,VTransportMode::RADIANCE,cosOut,&pdf,&bsdf);//Sample
                if(isTracingInvalid(sampled)){if(mStatus.bDebugMode){std::cout<<"Primary EYE Loop: Invalid Sample ->mtl: "<<toString(mtl.type)<<" for sample : "<<toString(sampled.type)<<"\n";} break;}
                if(isTracingInvalid(pdf)){if(mStatus.bDebugMode){std::cout<<"Primary EYE Loop: Invalid PDF: "<<pdf<<" ->mtl: "<<toString(mtl.type)<<" for sample : "<<toString(sampled.type)<<"\n";} break;}
                if(isTracingInvalid(bsdf)){if(mStatus.bDebugMode){std::cout<<"Primary EYE Loop: Invalid BSDF: "<<bsdf<<" ->mtl: "<<toString(mtl.type)<<" for sample : "<<toString(sampled.type)<<"\n";} break;}
                wi = sampled.ray;

                if(isDeltaSample(sampled)) weight = weight*(bsdf / pdf);
                else weight = weight*(bsdf*cosOut / pdf);

                weight_no_rr = weight;
                if(b>3){if(!russian_roulette(rng,weight)) break;}

                //SURFACE VC && VCM ---->MIS Definition
                if(useBidirectional()){
                    if(isDeltaSample(sampled)){
                        lastDelta = true;
                        vertex.vc *= cosOut;
                        vertex.vcm = 0.0;
                    }else{
                        lastDelta = false;
                        allDelta = false;
                        double rev_pdf = EvalBsdf_Pdf(scn,hit,mtl,normal,wo,wi);
                        if(isTracingInvalid(rev_pdf)) break;
                        vertex.vc = (cosOut / pdf) *(vertex.vcm + (vertex.vc*rev_pdf));
                        vertex.vcm = 1.0 / pdf;
                    }
                }else{
                    lastDelta = isDeltaSample(sampled);
                }
                ////////////////////////
            }

            return output*spectral_to_rgb(gathered_wl);
        }

		void EvalImageRow(const VScene& scn, VRng& rng, image3d& img, int width, int height, int j) {
		    const double f_ray_samples = (double)i_ray_samples;
		    const double f_ray_samples_2 = f_ray_samples*f_ray_samples;
		    const auto f_img_size = vec2f{img.size.x,img.size.y};

			for (int i = 0; i < width && !mStatus.bStopped; i++) {
				vec3d color = zero3d;

                //JITTERED

                for (int s = 0; s < i_ray_samples; s++) {
                    auto px_uv = rng.next_vecd<2>();
                    VRay ray = scn.camera.RayCast(i,j,rng,!i_debug_primary ? (px_uv) : zero2d);
                    InitRay(rng,ray);
                    color += (Radiance(scn, rng, img, const_cast<VCamera&>(scn.camera), ray)/ f_ray_samples);
                }

                //STRATIFIED
                /*
                for(int s=0;s<i_ray_samples;s++){
                    for(int t=0;t<i_ray_samples;t++){
                        VRay ray = scn.camera.RayCast(i,j,rng,!i_debug_primary ? (vec2d{s,t}+rng.next_vecd<2>())/f_ray_samples : zero2d);
                        InitRay(rng,ray);
                        color += (Radiance(scn, rng, img, const_cast<VCamera&>(scn.camera), ray)/ f_ray_samples_2);
                    }
                }*/

				auto& px = at(img,{i, j});
				px += color;
			}
		}

    };
};

#endif // VRE_EXPERIMENTAL_PT_HPP_INCLUDED
