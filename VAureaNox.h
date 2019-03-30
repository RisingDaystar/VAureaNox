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

#ifndef _VAUREANOX_H
#define _VAUREANOX_H

#include "yocto\ygl.h"
#include <map>
#include <vector>
//#include <queue>
#include <fstream>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
//#include <stack>
#include <iostream>
#include <math.h>
#include <sstream>

#define INTERSECT_ALGO intersect_rel
#define NORMALS_ALGO eval_normals_tht
#define LIGHT_PRECALC_ALGO precalc_emissive_hints

using namespace ygl;

namespace vnx {

	struct VResult;
	struct VMaterial;

	struct VNode;
	struct VVolume;
	struct VOperator;

	struct VRay;
	struct VScene;

	struct VSceneParser;

	const char* version = "0.07";

	using vfloat = double;
    using vec4vf = vec<vfloat,4>;
	using vec3vf = vec<vfloat,3>;
	using vec2vf = vec<vfloat,2>;
	using frame3vf = frame<vfloat,3>;
	using mat4vf = mat<vfloat, 4, 4>;
	using mat3vf = mat<vfloat, 3, 3>;
	using mat2vf = mat<vfloat, 2, 2>;

	using image3f = image<vec3f>;
	using image3vf = image<vec3vf>;

	constexpr const auto epsvf = epst<vfloat>();
	constexpr const auto minvf = mint<vfloat>();
	constexpr const auto maxvf = maxt<vfloat>();
	constexpr const auto pivf  = ygl::pi<vfloat>;

	constexpr frame3vf identity_frame3vf = frame3vf{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
	constexpr vec4vf identity_quat4vf = vec4vf{0,0,0,1};
	constexpr mat4vf identity_mat4vf = mat4vf{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
	constexpr mat3vf identity_mat3vf = mat3vf{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	constexpr mat2vf identity_mat2vf = mat2vf{{1, 0}, {0, 1}};

	template<typename T>
	constexpr vec<T,4> zero4 = vec<T,4>{0,0,0,0};
	template<typename T>
	constexpr vec<T,3> zero3 = vec<T,3>{0,0,0};
	template<typename T>
	constexpr vec<T,2> zero2 = vec<T,2>{0,0};

	template<typename T>
	constexpr vec<T,4> one4 = vec<T,4>{1,1,1,1};
	template<typename T>
	constexpr vec<T,3> one3 = vec<T,3>{1,1,1};
	template<typename T>
	constexpr vec<T,2> one2 = vec<T,2>{1,1};

	constexpr vec4vf one4vf = one4<vfloat>;
	constexpr vec3vf one3vf = one3<vfloat>;
	constexpr vec2vf one2vf = one2<vfloat>;

	constexpr vec4vf zero4vf = zero4<vfloat>;
	constexpr vec3vf zero3vf = zero3<vfloat>;
	constexpr vec2vf zero2vf = zero2<vfloat>;

	constexpr vfloat KC = 299792e3;
    constexpr vfloat KH = 6.63e-34;
    constexpr vfloat KB = 1.38066e-23;
    constexpr vfloat KSB = 5.670373e-8;
    constexpr vfloat KWD = 2.8977721e-3;

    constexpr vec3vf nv1 = { 1.0, -1.0, -1.0 };
    constexpr vec3vf nv2 = { -1.0, -1.0, 1.0 };
    constexpr vec3vf nv3 = { -1.0, 1.0, -1.0 };
    constexpr vec3vf nv4 = { 1.0, 1.0, 1.0 };


	typedef vfloat (*displ_ftor)(const vec3vf&);
	typedef void (*mtlm_ftor)(ygl::rng_state& rng,const VResult&,const vec3vf&, VMaterial&);

	enum VAxis { X, Y, Z };

    enum VRotationOrder{
        VRO_XYZ,
        VRO_XZY,
        VRO_YXZ,
        VRO_YZX,
        VRO_ZXY,
        VRO_ZYX,
    };

    enum VMaterialType{
        dielectric,
        conductor,
    };

    enum VIorType{
        wl_dependant,
        non_wl_dependant
    };

    struct VStatus{ //TODO : extend
        bool bDebugMode = false;
        bool bFinished = false;
        bool bStopped = false;
    };

	class VException : public std::exception{
	    std::string wh;
	    public:
        VException(std::string msg) :wh(msg){}
        const char* what() const noexcept{
            return wh.c_str();
        }
	};

    struct VEntry{
        VEntry(){}
        VEntry(std::vector<std::string> tks){for(auto tk : tks)tokens.push_back(tk);}
        void add_token(std::string tk){tokens.push_back(tk);}
        std::string try_at(int idx) const{if(idx>=tokens.size())return std::string(); return tokens[idx];}
        std::string& at(int idx){return tokens[idx];}
        bool empty(){return tokens.empty();}
        int size(){return tokens.size();}
        bool allocated(){return ptr!=nullptr;}


        std::vector<std::string> tokens;
        void* ptr = nullptr;
    };

    template<class T> struct VScopedPtr{
        bool _adr = false;
        T* _ptr = nullptr;

        VScopedPtr(T* ptr,bool adr):_ptr(ptr),_adr(adr){}
        ~VScopedPtr(){
            if(_adr==true){
                if(_ptr!=nullptr) delete _ptr;
                _ptr = nullptr;
            }
        }
        inline void setADR(bool v){_adr = v;}
    };

    inline vec3vf sample_sphere_direction_vf(const vec2f& ruv) {
        auto z   = 2 * ruv.y - 1;
        auto r   = std::sqrt(1 - z * z);
        auto phi = 2 * pivf * ruv.x;
        return {r * std::cos(phi), r * std::sin(phi), z};
    }

    inline vec3vf sample_hemisphere_direction_vf(const vec2f& ruv) {
        auto z   = ruv.y;
        auto r   = std::sqrt(1 - z * z);
        auto phi = 2 * pivf * ruv.x;
        return {r * std::cos(phi), r * std::sin(phi), z};
    }

    inline vec3vf sample_hemisphere_direction_cosine_vf(const vec2f& ruv) {
        auto z   = std::sqrt(ruv.y);
        auto r   = std::sqrt(1 - z * z);
        auto phi = 2 * pivf * ruv.x;
        return {r * std::cos(phi), r * std::sin(phi), z};
    }

    template<typename T,int N>
    inline vec<T,N> toVec(T v){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
        if constexpr(N<=1) return {v};
        else if constexpr(N==2) return {v,v};
        else if constexpr(N==3) return {v,v,v};
        else if constexpr(N>3) return {v,v,v,v};
    }

	inline float rand1f_r(rng_state& rng, float a, float b) {
        return a + (b - a) * get_random_float(rng);
    }
    inline vec2f rand2f_r(rng_state& rng, float a, float b) {
        return {rand1f_r(rng,a,b),rand1f_r(rng,a,b)};
    }
    inline vec3f rand3f_r(rng_state& rng, float a, float b) {
        return {rand1f_r(rng,a,b),rand1f_r(rng,a,b),rand1f_r(rng,a,b)};
    }
    inline vec4f rand4f_r(rng_state& rng, float a, float b) {
        return {rand1f_r(rng,a,b),rand1f_r(rng,a,b),rand1f_r(rng,a,b),rand1f_r(rng,a,b)};
    }

	inline bool stricmp(const std::string& str1,const std::string& str2) {
	    if(str1.size()!=str2.size()) return false;
		return std::equal(str1.begin(), str1.end(), str2.begin(), [](const char& a, const char& b) {
			return (std::tolower(a) == std::tolower(b));
		});
	}

    inline bool strIsGroup(const std::string& ss){
        return ss[0]=='{' && ss[ss.length()-1]=='}';
    }

    inline std::vector<std::string> strDeGroup(const std::string& ss){
        if(!strIsGroup(ss)) return {ss};
        int idx = 0;
        std::vector<std::string> tokens = {""};
        for(auto i=1;i<ss.length()-1;i++){
            char c = ss[i];
            if(c==','){tokens.push_back("");idx++;continue;}
            tokens[idx]+=c;
        }
        return tokens;
    }

    inline VAxis try_strToAxis(const std::string& ss,VAxis def){
        if(ss.empty()) return def;
        if(stricmp(ss,"x")) return VAxis::X;
        if(stricmp(ss,"y")) return VAxis::Y;
        if(stricmp(ss,"z")) return VAxis::Z;
        return def;
    }

    inline VMaterialType try_strToMaterialType(const std::string& ss,VMaterialType def){
        if(ss.empty()) return def;
        if(stricmp(ss,"d")) return VMaterialType::dielectric;
        if(stricmp(ss,"c")) return VMaterialType::conductor;
        return def;
    }

    inline VIorType try_strToIorType(const std::string& ss,VIorType def){
        if(ss.empty()) return def;
        if(stricmp(ss,"wl")) return VIorType::wl_dependant;
        if(stricmp(ss,"nwl")) return VIorType::non_wl_dependant;
        return def;
    }

    inline VRotationOrder try_strToRotationOrder(const std::string& ss,VRotationOrder def){
        if(ss.empty()) return def;
        if(stricmp(ss,"xyz")) return VRO_XYZ;
        if(stricmp(ss,"xzy")) return VRO_XZY;
        if(stricmp(ss,"yxz")) return VRO_YXZ;
        if(stricmp(ss,"yzx")) return VRO_YZX;
        if(stricmp(ss,"zxy")) return VRO_ZXY;
        if(stricmp(ss,"zyx")) return VRO_ZYX;
        return def;
    }

    inline vfloat try_strtovf(const std::string& ss,vfloat def){
        if(ss.empty()) return def;
        return std::atof(ss.c_str());
    }

    inline float try_strtof(const std::string& ss,float def){
        if(ss.empty()) return def;
        return std::atof(ss.c_str());
    }

    inline int try_strtoi(const std::string& ss,int def){
        if(ss.empty()) return def;
        return std::atoi(ss.c_str());
    }

    inline bool try_strtob(const std::string& ss,bool def){
        if(ss.empty()) return def;
        if(ss.length()>1){
            if(stricmp(ss,"true")) return true;
            if(stricmp(ss,"false")) return false;
        }
        return (bool)std::atoi(ss.c_str());
    }

    template<int N>
    inline vec<int,N> try_strToVec_i(const std::string& ss,const vec<int,N>& def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<int,N> v;
            for(auto i=0;i<cpnts.size() && i<N;i++){
                if constexpr(N>=1){if(i==0){v.x = try_strtoi(cpnts[0],0);continue;}}
                if constexpr(N>=2){if(i==1){v.y = try_strtoi(cpnts[1],0);continue;}}
                if constexpr(N>=3){if(i==2){v.z = try_strtoi(cpnts[2],0);continue;}}
                if constexpr(N>=4){if(i==3){v.w = try_strtoi(cpnts[3],0);}}
            }
            return v;
        }else{
            auto cpnt = try_strtoi(ss,0);
            return toVec<int,N>(cpnt);
        }
    }

    template<int N>
    inline vec<bool,N> try_strToVec_b(const std::string& ss,const vec<bool,N>& def){
        static_assert(N>0 && N<5, "N must be in range 1 | 4");
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<bool,N> v;
            for(auto i=0;i<cpnts.size() && i<N;i++){
                if constexpr(N>=1){if(i==0){v.x = try_strtoi(cpnts[0],0);continue;}}
                if constexpr(N>=2){if(i==1){v.y = try_strtoi(cpnts[1],0);continue;}}
                if constexpr(N>=3){if(i==2){v.z = try_strtoi(cpnts[2],0);continue;}}
                if constexpr(N>=4){if(i==3){v.w = try_strtoi(cpnts[3],0);}}
            }
            return v;
        }else{
            auto cpnt = try_strtoi(ss,0);
            return toVec<bool,N>(cpnt);
        }
    }

    template<int N>
    inline vec<float,N> try_strToVec_f(const std::string& ss,const vec<float,N>& def){
        static_assert(N>0 && N<5, "N must be in range 1 | 4");
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<float,N> v;
            for(auto i=0;i<cpnts.size() && i<N;i++){
                if constexpr(N>=1){if(i==0){v.x = try_strtof(cpnts[0],0);continue;}}
                if constexpr(N>=2){if(i==1){v.y = try_strtof(cpnts[1],0);continue;}}
                if constexpr(N>=3){if(i==2){v.z = try_strtof(cpnts[2],0);continue;}}
                if constexpr(N>=4){if(i==3){v.w = try_strtof(cpnts[3],0);}}
            }
            return v;
        }else{
            auto cpnt = try_strtof(ss,0);
            return toVec<float,N>(cpnt);
        }
    }

    template<int N>
    inline vec<vfloat,N> try_strToVec_vf(const std::string& ss,const vec<vfloat,N>& def){
        static_assert(N>0 && N<5, "N must be in range 1 | 4");
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<vfloat,N> v;
            for(auto i=0;i<cpnts.size() && i<N;i++){
                if constexpr(N>=1){if(i==0){v.x = try_strtof(cpnts[0],0);continue;}}
                if constexpr(N>=2){if(i==1){v.y = try_strtof(cpnts[1],0);continue;}}
                if constexpr(N>=3){if(i==2){v.z = try_strtof(cpnts[2],0);continue;}}
                if constexpr(N>=4){if(i==3){v.w = try_strtof(cpnts[3],0);}}
            }
            return v;
        }else{
            auto cpnt = try_strtof(ss,0);
            return toVec<vfloat,N>(cpnt);
        }
    }

    /*
    inline vec3i try_strToVec_i(const std::string& ss,vec3i def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec3i v = zero3i;
            for(int i=0;i<cpnts.size() && i<3;i++){
                if(i==0)v.x = try_strtoi(cpnts[i],0);
                else if(i==1)v.y = try_strtoi(cpnts[i],0);
                else if(i==2)v.z = try_strtoi(cpnts[i],0);
            }
            return v;
        }else{
            auto cpnt = try_strtoi(ss,0);
            return vec3i{cpnt,cpnt,cpnt};
        }
    }

    inline vec2i try_strToVec2i(const std::string& ss,vec2i def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec2i v = zero2i;
            for(int i=0;i<cpnts.size() && i<2;i++){
                if(i==0)v.x = try_strtoi(cpnts[i],0);
                else if(i==1)v.y = try_strtoi(cpnts[i],0);
            }
            return v;
        }else{
            auto cpnt = try_strtoi(ss,0);
            return vec2i{cpnt,cpnt};
        }
    }



    inline vec<bool,4> try_strToVec4b(const std::string& ss,vec<bool,4> def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<bool,4> v = {false,false,false,false};
            for(int i=0;i<cpnts.size() && i<4;i++){
                if(i==0)v.x = try_strtob(cpnts[i],false);
                else if(i==1)v.y = try_strtob(cpnts[i],false);
                else if(i==2)v.z = try_strtob(cpnts[i],false);
                else if(i==3)v.w = try_strtob(cpnts[i],false);
            }
            return v;
        }else{
            auto cpnt = try_strtob(ss,false);
            return vec<bool,4>{cpnt,cpnt,cpnt,cpnt};
        }
    }

    inline vec<bool,3> try_strToVec_b(const std::string& ss,vec<bool,3> def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<bool,3> v = {false,false,false};
            for(int i=0;i<cpnts.size() && i<3;i++){
                if(i==0)v.x = try_strtob(cpnts[i],false);
                else if(i==1)v.y = try_strtob(cpnts[i],false);
                else if(i==2)v.z = try_strtob(cpnts[i],false);
            }
            return v;
        }else{
            auto cpnt = try_strtob(ss,false);
            return vec<bool,3>{cpnt,cpnt,cpnt};
        }
    }

    inline vec<bool,2> try_strToVec2b(const std::string& ss,vec<bool,2> def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<bool,2> v = {false,false};
            for(int i=0;i<cpnts.size() && i<2;i++){
                if(i==0)v.x = try_strtob(cpnts[i],false);
                else if(i==1)v.y = try_strtob(cpnts[i],false);
            }
            return v;
        }else{
            auto cpnt = try_strtob(ss,false);
            return vec<bool,2>{cpnt,cpnt};
        }
    }

    inline vec4vf try_strToVec4vf(const std::string& ss,vec4vf def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec4vf v = zero4vf;
            for(int i=0;i<cpnts.size() && i<4;i++){
                if(i==0) v.x = std::atof(cpnts[i].c_str());
                else if(i==1) v.y = std::atof(cpnts[i].c_str());
                else if(i==2) v.z = std::atof(cpnts[i].c_str());
                else if(i==3) v.w = std::atof(cpnts[i].c_str());
            }
            return v;
        }else{
            auto cpnt = std::atof(ss.c_str());
            return vec4vf{cpnt,cpnt,cpnt,cpnt};
        }
    }
    inline vec3vf try_strToVec_vf(const std::string& ss,vec3vf def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec3vf v = zero3vf;
            for(int i=0;i<cpnts.size() && i<3;i++){
                if(i==0) v.x = std::atof(cpnts[i].c_str());
                else if(i==1) v.y = std::atof(cpnts[i].c_str());
                else if(i==2) v.z = std::atof(cpnts[i].c_str());
            }
            return v;
        }else{
            auto cpnt = std::atof(ss.c_str());
            return vec3vf{cpnt,cpnt,cpnt};
        }
    }
    inline vec2vf try_strToVec2vf(const std::string& ss,vec2vf def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec2vf v = zero2vf;
            for(int i=0;i<cpnts.size() && i<2;i++){
                if(i==0) v.x = std::atof(cpnts[i].c_str());
                else if(i==1) v.y = std::atof(cpnts[i].c_str());
            }
            return v;
        }else{
            auto cpnt = std::atof(ss.c_str());
            return vec2vf{cpnt,cpnt};
        }
    }
    */

	inline void print_hhmmss(long long seconds) {
		int min = seconds / 60;
		int hours = min / 60;
		std::cout << (hours % 60) << "h : " << (min % 60) << "m : " << (seconds % 60) << "s\n";
	}

    template <typename T>
	inline vec<T,3> rgbto(T r,T g,T b) {
		return { r / 255.0,g / 255.0,b / 255.0 };
	}
    template <typename T>
	inline vec<T,3> rgbto(const vec<T,3>& rgb) {
		return { rgb.x / 255.0,rgb.y / 255.0,rgb.z / 255.0 };
	}

    template <typename T>
    inline T modulo(const T& x){return x - std::floor(x);}

    template <typename T>
	inline T sign(T v) {
		if (v > 0.0) { return 1.0; }
		else if (v < 0.0) { return -1.0; }
		return 0.0;
	}

    template <typename T>
	inline T nz_sign(T v) { //zero is 1.0f
		if (v >= 0.0) { return 1.0; }
		return -1.0;
	}

    template <typename T>
	inline T nz_nz_sign(T v) { //zero is -1.0f
		if (v >0.0) { return 1.0; }
		return -1.0;
	}

    template <typename T>
    inline T cotan(T x){
        return 1.0/(std::tan(x));
    }

    template <typename T>
	inline T radians(T in) {
		return (in*pi<T>) / 180.0;
	}

    template <typename T>
	inline vfloat degrees(T in){
        return (in/pi<T>) * 180.0;
	}


	//SOURCE : Inigo Quilezles http://www.iquilezles.org/ & GLSL DOCS
	template <typename T>
	inline T gl_fract(T x) {
		return x - std::floor(x);
	}

    //SOURCE : Inigo Quilezles http://www.iquilezles.org/ & GLSL DOCS
    template <typename T>
	inline T gl_mod(T x,T y){
        return x - y * std::floor(x/y);
	}


    //SOURCE : Inigo Quilezles http://www.iquilezles.org/ & GLSL DOCS
    template <typename T>
	inline T hash(T seed){
		return gl_fract(std::sin(seed)*43758.5453);
	}


	template <typename T,int N>
	inline T max_element(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return v.x;
	    else if constexpr(N==2) return std::max(v.x, v.y);
	    else if constexpr(N==3) return std::max(v.x, std::max(v.y, v.z));
	    else if constexpr(N==4) return std::max(std::max(v.x,v.y), std::max(v.z, v.w));
    }

	template <typename T,int N>
	inline T min_element(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return v.x;
	    else if constexpr(N==2) return std::min(v.x, v.y);
	    else if constexpr(N==3) return std::min(v.x, std::min(v.y, v.z));
	    else if constexpr(N==4) return std::min(std::min(v.x,v.y), std::min(v.z, v.w));
    }

	template <typename T,int N>
	vec<T,N> abs(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::abs(v.x)};
	    else if constexpr(N==2) return {std::abs(v.x),std::abs(v.y)};
	    else if constexpr(N==3) return {std::abs(v.x),std::abs(v.y),std::abs(v.z)};
	    else if constexpr(N==4) return {std::abs(v.x),std::abs(v.y),std::abs(v.z),std::abs(v.w)};
    }

	template <typename T,int N>
	inline vec<T,N> exp(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::exp(v.x)};
	    else if constexpr(N==2) return {std::exp(v.x),std::exp(v.y)};
	    else if constexpr(N==3) return {std::exp(v.x),std::exp(v.y),std::exp(v.z)};
	    else if constexpr(N==4) return {std::exp(v.x),std::exp(v.y),std::exp(v.z),std::exp(v.w)};
    }

	template <typename T,int N>
	inline vec<T,N> pow(const ygl::vec<T, N>& v,T p) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::pow(v.x,p)};
	    else if constexpr(N==2) return {std::pow(v.x,p),std::pow(v.y,p)};
	    else if constexpr(N==3) return {std::pow(v.x,p),std::pow(v.y,p),std::pow(v.z,p)};
	    else if constexpr(N==4) return {std::pow(v.x,p),std::pow(v.y,p),std::pow(v.z,p),std::pow(v.w,p)};
    }

	template <typename T,int N>
	vec<T,N> log(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::log(v.x)};
	    else if constexpr(N==2) return {std::log(v.x),std::log(v.y)};
	    else if constexpr(N==3) return {std::log(v.x),std::log(v.y),std::log(v.z)};
	    else if constexpr(N==4) return {std::log(v.x),std::log(v.y),std::log(v.z),std::log(v.w)};
    }

	template <typename T,int N>
	inline vec<T,N> sqrt(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::sqrt(v.x)};
	    else if constexpr(N==2) return {std::sqrt(v.x),std::sqrt(v.y)};
	    else if constexpr(N==3) return {std::sqrt(v.x),std::sqrt(v.y),std::sqrt(v.z)};
	    else if constexpr(N==4) return {std::sqrt(v.x),std::sqrt(v.y),std::sqrt(v.z),std::sqrt(v.w)};
    }

	template <typename T,int N>
	inline vec<T,N> min(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::min(v1.x,v2.x)};
	    else if constexpr(N==2) return {std::min(v1.x,v2.x),std::min(v1.y,v2.y)};
	    else if constexpr(N==3) return {std::min(v1.x,v2.x),std::min(v1.y,v2.y),std::min(v1.z,v2.z)};
	    else if constexpr(N==4) return {std::min(v1.x,v2.x),std::min(v1.y,v2.y),std::min(v1.z,v2.z),std::min(v1.w,v2.w)};
    }

	template <typename T,int N>
	inline vec<T,N> max(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::max(v1.x,v2.x)};
	    else if constexpr(N==2) return {std::max(v1.x,v2.x),std::max(v1.y,v2.y)};
	    else if constexpr(N==3) return {std::max(v1.x,v2.x),std::max(v1.y,v2.y),std::max(v1.z,v2.z)};
	    else if constexpr(N==4) return {std::max(v1.x,v2.x),std::max(v1.y,v2.y),std::max(v1.z,v2.z),std::max(v1.w,v2.w)};
    }

	template <typename T,int N>
	inline vec<T,N> gl_mod(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {gl_mod(v1.x,v2.x)};
	    else if constexpr(N==2) return {gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y)};
	    else if constexpr(N==3) return {gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y),gl_mod(v1.z,v2.z)};
	    else if constexpr(N==4) return {gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y),gl_mod(v1.z,v2.z),gl_mod(v1.w,v2.w)};
    }

	template <typename T,int N>
	inline vec<T,N> gl_fract(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {gl_fract(v1.x,v2.x)};
	    else if constexpr(N==2) return {gl_fract(v1.x,v2.x),gl_fract(v1.y,v2.y)};
	    else if constexpr(N==3) return {gl_fract(v1.x,v2.x),gl_fract(v1.y,v2.y),gl_fract(v1.z,v2.z)};
	    else if constexpr(N==4) return {gl_fract(v1.x,v2.x),gl_fract(v1.y,v2.y),gl_fract(v1.z,v2.z),gl_fract(v1.w,v2.w)};
    }

	template <typename T,int N>
	inline T adot(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    return std::abs(dot(v1,v2));
    }


    template<typename T>
    inline bool cmpf(T a, T b){
        const T absA = std::abs(a);
		const T absB = std::abs(b);
		const T diff = std::abs(a - b);
		if (a == b) {
			return true;
		} else if (a == 0 || b == 0 || diff < mint<T>()) {
			return diff < (epst<T>() * mint<T>());
		} else {
			return diff / std::min((absA + absB), maxt<T>()) < epst<T>();
		}
    }

    template<typename T,int N>
    inline bool cmpf(const vec<T,N>& a,const vec<T,N>& b){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return cmpf(a.x,b.x);
	    else if constexpr(N==2) return cmpf(a.x,b.x) && cmpf(a.y,b.y);
	    else if constexpr(N==3) return cmpf(a.x,b.x) && cmpf(a.y,b.y) && cmpf(a.z,b.z);
	    else if constexpr(N==4) return cmpf(a.x,b.x) && cmpf(a.y,b.y) && cmpf(a.z,b.z) && cmpf(a.w,b.w);
    }

    template<typename T,int N>
    inline bool has_nan(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return std::isnan(a.x);
	    else if constexpr(N==2) return std::isnan(a.x) || std::isnan(a.y);
	    else if constexpr(N==3) return std::isnan(a.x) || std::isnan(a.y) || std::isnan(a.z);
	    else if constexpr(N==4) return std::isnan(a.x) || std::isnan(a.y) || std::isnan(a.z) || std::isnan(a.w);
    }

    template<typename T,int N>
    inline bool has_nnormal(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return !(std::isnormal(a.x));
	    else if constexpr(N==2) return !(std::isnormal(a.x) && std::isnormal(a.y));
	    else if constexpr(N==3) return !(std::isnormal(a.x) && std::isnormal(a.y) && std::isnormal(a.z));
	    else if constexpr(N==4) return !(std::isnormal(a.x) && std::isnormal(a.y) && std::isnormal(a.z) && std::isnormal(a.w));
    }

    template<typename T,int N>
    inline bool has_inf(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return !(std::isfinite(a.x));
	    else if constexpr(N==2) return !(std::isfinite(a.x) && std::isfinite(a.y));
	    else if constexpr(N==3) return !(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z));
	    else if constexpr(N==4) return !(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z) && std::isfinite(a.w));
    }

    template<typename T,int N>
    inline bool is_zero_or_has_ltz(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
        if constexpr(N==1) return cmpf(a,{0}) || a.x<-epst<T>();
        else if constexpr(N==2) return cmpf(a,zero2<T>) || a.x<-epst<T>() || a.y<-epst<T>();
        else if constexpr(N==3) return cmpf(a,zero3<T>) || a.x<-epst<T>() || a.y<-epst<T>() || a.z<-epst<T>();
        else if constexpr(N==4) return cmpf(a,zero4<T>) || a.x<-epst<T>() || a.y<-epst<T>() || a.z<-epst<T>() || a.w<-epst<T>();
    }

    template<typename T>
	inline bool in_range_eps(T v,T vref,T eps){
        return (v>=vref-eps) && (v<=vref+eps);
	}

    template<typename T,int N>
    inline std::ostream& operator<<(std::ostream& os, const vec<T,N>& s){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
        if constexpr(N==1) os<<'{'<<s.x<<'}';
        else if constexpr(N==2) os<<'{'<<s.x<<'}';
        else if constexpr(N==3) os<<'{'<<s.x<<','<<s.y<<','<<s.z<<'}';
        else if constexpr(N==4) os<<'{'<<s.x<<','<<s.y<<','<<s.z<<','<<s.w<<'}';
        return os;
    }

	//SOURCE : Inigo Quilezles http://www.iquilezles.org/
	template<typename T>
	inline T smin(T a, T b, T k){
		T h = ygl::clamp(0.5 + 0.5*(b - a) / k, 0.0, 1.0);
		return ygl::lerp(b, a, h) - k*h*(1.0 - h);
	}
	//SOURCE : Inigo Quilezles http://www.iquilezles.org/
	template<typename T>
	inline T smax(T a, T b, T k){
		T h = ygl::clamp(0.5 + 0.5*(b - a) / k, 0.0, 1.0);
		return ygl::lerp(a, b, h) + k*h*(1.0 - h);
	}

    template<typename T>
    inline vec<T,3> gamma_to_linear(const vec<T,3>& srgb, T gamma = 2.2f) {
        return pow(srgb,gamma);//{std::pow(srgb.x, gamma), std::pow(srgb.y, gamma), std::pow(srgb.z, gamma)};
    }
    template<typename T>
    inline vec<T,3> linear_to_gamma(const vec<T,3>& lin, T gamma = 2.2f) {
        return pow(lin,1.0 / gamma);
    }
    template<typename T>
    inline vec<T,4> gamma_to_linear(const vec<T,4>& srgb, T gamma = 2.2f) {
        return {std::pow(srgb.x, gamma), std::pow(srgb.y, gamma), std::pow(srgb.z, gamma), srgb.w};
    }
    template<typename T>
    inline vec<T,4> linear_to_gamma(const vec<T,4>& lin, T gamma = 2.2f) {
        return {std::pow(lin.x, 1.0 / gamma), std::pow(lin.y, 1.0 / gamma), std::pow(lin.z, 1.0 / gamma),
            lin.w};
    }

    ///Credits : http://bl.ocks.org/benjaminabel/4355926 ///
	inline vec3vf spectral_to_rgb(vfloat w){ // RGB <0,1> <- lambda w <380,780> [nm]
		vfloat l = 0.0; vfloat R = 0.0; vfloat G = 0.0; vfloat B = 0.0;
        if (w >= 380.0 && w < 440.0) {
            R = -(w - 440.0) / (440.0 - 350.0);
            G = 0.0;
            B = 1.0;
        } else if (w >= 440.0 && w < 490.0) {
            R = 0.0;
            G = (w - 440.0) / (490.0 - 440.0);
            B = 1.0;
        } else if (w >= 490.0 && w < 510.0) {
            R = 0.0;
            G = 1.0;
            B = -(w - 510.0) / (510.0 - 490.0);
        } else if (w >= 510.0 && w < 580.0) {
            R = (w - 510.0) / (580.0 - 510.0);
            G = 1.0;
            B = 0.0;
        } else if (w >= 580.0 && w < 645.0) {
            R = 1.0;
            G = -(w - 645.0) / (645.0 - 580.0);
            B = 0.0;
        } else if (w >= 645.0 && w <= 780.0) {
            R = 1.0;
            G = 0.0;
            B = 0.0;
        } else {
            R = 0.0;
            G = 0.0;
            B = 0.0;
        }

        if (w >= 380.0 && w < 420.0) {
            l = 0.3 + 0.7 * (w - 350.0) / (420.0 - 350.0);
        } else if (w >= 420.0 && w <= 700.0) {
            l = 1.0;
        } else if (w > 700.0 && w <= 780.0) {
            l = 0.3 + 0.7 * (780.0 - w) / (780.0 - 700.0);
        } else {
            l = 0.0;
        }
		return {l*R,l*G,l*B};
	}

	inline vfloat blackbody_planks_law_wl(vfloat t,vfloat c,vfloat wl) {
	    wl = wl*1e-9;
	    auto wl5 = wl*wl*wl*wl*wl;
        return (2.0*KH*c*c) / (wl5*(std::exp((KH*c)/(wl*KB*t)) -1.0));
	}

	inline vfloat blackbody_planks_law_wl_normalized(vfloat t,vfloat c,vfloat wl) {
	    auto le = blackbody_planks_law_wl(t,c,wl);
	    vfloat lmax = KWD / t * 1e9; //wien
	    auto lem = blackbody_planks_law_wl(t,c,lmax);
	    return le / lem;
	}

	inline vfloat stefan_boltzman_law(vfloat t){
        return (KSB / pivf)*(t*t*t*t);
	}

	inline vfloat wien_displacement_law(vfloat t){
        return KWD * t;
	}

	inline vfloat wl_from_frquency(vfloat c,vfloat f){
        return c/f;
	}

	inline vfloat f_from_wl(vfloat c,vfloat wl){
        return c/wl;
	}

	inline vfloat thz_to_ghz(vfloat f){
	    return f*1e3;
	}

    //.,.,.,nm^2,nm^2,nm^2,nm
	inline vfloat sellmeier_law(vfloat b1, vfloat b2, vfloat b3, vfloat c1, vfloat c2, vfloat c3, vfloat wl) {
		wl *= 1e-2;
		auto wl2 = wl*wl;
		return std::sqrt(1 +
			((b1*wl2) / (wl2 - c1)) +
			((b2*wl2) / (wl2 - c2)) +
			((b3*wl2) / (wl2 - c3))
		);
	}

	inline vfloat F0_from_ior(vfloat ior){
	    return std::pow((ior-1.0)/(ior+1.0),2.0);
	}

	struct VRay {
		vec3vf o = zero3vf;
		vec3vf d = zero3vf;
		vfloat tmin = 0.001;
		vfloat tmax = 1000.0;
		vfloat ior = 1.0;

		vfloat wl = 0.0;
		vfloat frequency = 0.0;
		vfloat velocity = 0.0;


        VRay operator-() const {
          VRay rr;
          rr.o = o;
          rr.d = -d;
          rr.tmin = tmin;
          rr.tmax = tmax;
          rr.ior = ior;

          rr.wl = wl;
          rr.frequency = frequency;
          rr.velocity = velocity;
          return rr;
        }
	};

	inline VRay offsetted_ray(const vec3vf& o,VRay rr, const vec3vf& d, const vfloat tmin, const vfloat tmax, const vec3vf& offalong, const vfloat dist, vfloat fmult = 2.0) {
		rr.o = o + (offalong*(fmult * std::max(tmin, std::abs(dist))));
		rr.d = d;
		rr.tmin = tmin;
		rr.tmax = tmax;
		return rr;
	}

	inline VRay flip_ray(VRay rr){
        rr.d=-rr.d;
        return rr;
	}

	struct VMaterial {
	    VMaterial(): id(""){}
	    VMaterial(std::string idv){id=idv;}
		std::string id = "";
		mtlm_ftor mutator = nullptr;

        VMaterialType type = dielectric;
        VIorType ior_type = wl_dependant;

		vec3vf kr = zero3vf;
		vec3vf ka = zero3vf;

		vfloat k_sca = -1.0;

		vfloat e_temp = 0.0;
		vfloat e_power = 0.0;

		vfloat ior = 1.0;
		vfloat rs = 0.0;

		vfloat sm_b1 = 0.0;
		vfloat sm_b2 = 0.0;
		vfloat sm_b3 = 0.0;

		vfloat sm_c1 = 0.0;
		vfloat sm_c2 = 0.0;
		vfloat sm_c3 = 0.0;

		inline void Relate(VEntry* entry){
            entry->ptr = this;
            type = try_strToMaterialType(entry->try_at(1),type);
            ior_type = try_strToIorType(entry->try_at(2),ior_type);
            kr = try_strToVec_vf(entry->try_at(3),kr);
            ka = try_strToVec_vf(entry->try_at(4),ka);

            k_sca = try_strtof(entry->try_at(5),k_sca);

            e_temp = try_strtof(entry->try_at(6),e_temp);
            e_power = try_strtof(entry->try_at(7),e_power);

            ior = try_strtof(entry->try_at(8),ior);
            rs = try_strtof(entry->try_at(9),rs);
            //vmaterial("light_mat",d,nwl,0.0,0.0,-1.0,6000,120)
		}

		inline bool is_dielectric() const {return type==dielectric;}
		inline bool is_conductor()const {return type==conductor;}

		inline bool is_emissive()  const { return e_temp > 0.0001 && e_power > 0.0001; }
		inline bool is_transmissive() const { return k_sca>=-0.0001;}
		inline bool is_refractive() const { return is_transmissive() && (sm_b1>0.0001 || sm_b2>0.0001 || sm_b3>0.0001 || sm_c1>0.0001 || sm_c2>0.0001 || sm_c3>0.0001);}

		inline bool is_delta() const{return ((is_conductor() && rs<=0.0001) || (is_transmissive() && rs<=0.0001) || (is_dielectric() && !is_transmissive() && cmpf(kr,zero3vf)));}

		inline bool has_kr(){return kr!=zero3vf;}

		inline vfloat eval_ior(vfloat wl,vfloat min_wl,vfloat max_wl,bool do_wl_depend = true) const{
		    if(ior_type==non_wl_dependant) return ior;
		    if(!is_refractive()) return 1.0;

            if(do_wl_depend) return sellmeier_law(sm_b1,sm_b2,sm_b3,sm_c1,sm_c2,sm_c3,wl);

            auto ior_min = sellmeier_law(sm_b1,sm_b2,sm_b3,sm_c1,sm_c2,sm_c3,min_wl);
            auto ior_max = sellmeier_law(sm_b1,sm_b2,sm_b3,sm_c1,sm_c2,sm_c3,max_wl);
            return (ior_min+ior_max) / 2.0;

		}

		inline void eval_mutator(ygl::rng_state& rng,const VResult& hit,const vec3vf& n, VMaterial& mat) {
			if (mutator != nullptr){
                mutator(rng, hit, n, mat);
			}
		}
	};

	struct VNode {
		std::string id = "";
		frame3vf _frame = identity_frame3vf;
		vec3vf translation = zero3vf;
		vec4vf rotation = identity_quat4vf;
		vec3vf scale = one3vf;
		VRotationOrder rotation_order = VRO_XYZ;
		displ_ftor displacement = nullptr;


		VNode(std::string idv): id(idv) ,
		_frame(identity_frame3vf),
		scale(one3vf),
		rotation(identity_quat4vf),
		translation(zero3vf),
		rotation_order(VRO_XYZ),
		displacement(nullptr){}

		virtual ~VNode() {};

		void Relate(VEntry* entry){entry->ptr = this;DoRelate(entry);}
		virtual void DoRelate(const VEntry* entry) = 0;

		virtual std::vector<VNode*> get_childs() = 0;
		virtual VNode* add_child(VNode* child) = 0;
		virtual void add_childs(std::vector<VNode*> chs) = 0;

		virtual inline const char* type() = 0;

		inline vfloat eval_displacement(const vec3vf& p){
		    if(displacement!=nullptr){
                return displacement(p);
		    }
		    return 0.0;
		}

		inline void set_translation(const vec3vf& a) { translation = a; }
		inline void set_translation(vfloat x, vfloat y, vfloat z) { translation = { x,y,z }; }

		inline void set_rotation(const vec3vf& a) { rotation = { a.x,a.y,a.z,1 }; }
		inline void set_rotation(vfloat x, vfloat y, vfloat z) { rotation = { x,y,z,1 }; }
		inline void set_rotation_degs(const vec3vf& a) { rotation = { vnx::radians(a.x),vnx::radians(a.y),vnx::radians(a.z),1 }; }
		inline void set_rotation_degs(vfloat x, vfloat y, vfloat z) { rotation = { vnx::radians(x),vnx::radians(y),vnx::radians(z),1 }; }
		inline void mod_rotation_degs(const vec3vf& a) { rotation.x += vnx::radians(a.x), rotation.y += vnx::radians(a.y); rotation.z += vnx::radians(a.z); }
		inline void mod_rotation_degs(vfloat x, vfloat y, vfloat z) { rotation.x += vnx::radians(x), rotation.y += vnx::radians(y); rotation.z += vnx::radians(z); }

		inline void set_rotation_order(VRotationOrder ro){rotation_order = ro;}

        inline void set_scale(vfloat s){ if (s <= 0.0001) { scale = one3vf; return; }  scale = {s,s,s}; }
		inline void set_scale(const vec3vf& a) { if (min_element(a) <= 0.0001) { scale = one3vf; return; }  scale = a; }
		inline void set_scale(vfloat x, vfloat y, vfloat z) { if (min_element(vec3vf{ x,y,z }) <= 0.0001) { scale = one3vf; return; } scale = { x,y,z }; }

		VNode* select(const std::string& n) {
			auto chs = get_childs();
			if (chs.empty()) { return nullptr; }
			for (auto& child : chs) {
				if (child->id == n) { return child; }
				auto res = child->select(n);
				if (res != nullptr) { return res; }
			}
			return nullptr;
		}

		std::vector<VNode*> select_all(const std::string& n) {
			std::vector<VNode*> results;
			auto chs = get_childs();
			if (chs.empty()) { return {}; }

			for (auto& child : chs) {
				if (child->id == n) { results.push_back(child); }
				auto res = child->select_all(n);
				if (!res.empty()) {
					for (auto chr : res) {
						results.push_back(chr);
					}
				}
			}
			return results;
		}

		inline virtual void eval(const vec3vf& p,VResult& res) = 0;
	};

	struct VVolume : public VNode {
		VMaterial* mMaterial = nullptr;
		~VVolume() {};

		inline vec3vf eval_ep(const vec3vf& p){
		    return transform_point_inverse(_frame, p);// / scale;
		}

		VVolume(std::string idv):VNode(idv),mMaterial(nullptr){}
		VVolume(std::string idv,VMaterial* mtl) :VNode(idv),mMaterial(mtl){}

		std::vector<VNode*> get_childs() {
			auto child = std::vector<VNode*>();
			child.resize(0);
			return child;
		}
		VNode* add_child(VNode* child) { return nullptr; };
		void add_childs(std::vector<VNode*> chs) {};
	};

	struct VOperator : public VNode {
		std::vector<VNode*> mChilds;

		~VOperator() {
			auto chs = get_childs();
			if (chs.empty()) { return; }
			for (auto child : chs) {
				if(child)delete child;
			}
		}
		VOperator(std::string idv) :VNode(idv){
			mChilds = std::vector<VNode*>();
			mChilds.resize(0);
		}
		VOperator(std::string idv,std::vector<VNode*> chs) :VNode(idv){
			add_childs(chs);
		}

		virtual std::vector<VNode*> get_childs() {
			return mChilds;
		}
		virtual VNode* add_child(VNode* child) {
			mChilds.push_back(child);
			return child;
		}
		virtual void add_childs(std::vector<VNode*> chs) {
			if (chs.empty()) { return; }
			for (auto child : chs) {
				mChilds.push_back(child);
			}
		}
	};

	struct VResult {
		VVolume* sur = nullptr;
		VVolume* vsur = nullptr;

		VMaterial* mat = nullptr;
		VMaterial* vmat = nullptr;

		vfloat dist = maxvf;
		vfloat vdist = maxvf;

		//vec3f norm = zero3f; //still unused
		vec3vf wor_pos = zero3vf;
		vec3vf loc_pos = zero3vf;

		bool _found = false;

		inline bool valid() {return sur != nullptr && mat!=nullptr;}
		inline bool found() { return _found; }
	};

	struct VCamera{
        vfloat focus = 1.0;
        vfloat aperture = 0.0;
        vfloat yfov = 45.0;

        vec3vf mOrigin = zero3vf;
        vec3vf mTarget = zero3vf;
        vec3vf mUp = {0,1,0};

        vec2f mResolution;
        vfloat mAspect;

        mat4vf mWorldToCamera = identity_mat4vf;
        mat4vf mWorldToRaster = identity_mat4vf;
        mat4vf mCameraToRaster = identity_mat4vf;
        mat4vf mCameraToClip = identity_mat4vf;
        mat4vf mClipToRaster = identity_mat4vf;


        mat4vf mCameraToWorld = identity_mat4vf;
        mat4vf mRasterToWorld = identity_mat4vf;
        mat4vf mRasterToCamera = identity_mat4vf;

        void Relate(const VEntry* entry){
            yfov = radians(try_strtof(entry->try_at(1),45.0));
            mOrigin = try_strToVec_vf(entry->try_at(2),mOrigin);
            mTarget = try_strToVec_vf(entry->try_at(3),mTarget);
            mUp = try_strToVec_vf(entry->try_at(4),mUp);
        }

        inline void Setup(const vec2f& resolution){
            mResolution = resolution;
            mAspect = vfloat(mResolution.x) / vfloat(mResolution.y);

            mClipToRaster =
                frame_to_mat(scaling_frame(vec3vf{mResolution.x,mResolution.y,1.0}))*
                frame_to_mat(scaling_frame(vec3vf{0.5,-0.5,1.0}))*
                frame_to_mat(translation_frame(vec3vf{1.0,-1.0,0.0}));

            mCameraToClip = perspective_mat(yfov,mAspect,0.1,1.0);
            mCameraToRaster = mClipToRaster*mCameraToClip;
            mWorldToCamera = inverse(frame_to_mat(lookat_frame(mOrigin,mTarget,mUp)));
            mWorldToRaster = mCameraToRaster*mWorldToCamera;


            //Inverse
            mCameraToWorld = inverse(mWorldToCamera);
            mRasterToCamera = inverse(mCameraToRaster);
            mRasterToWorld = inverse(mWorldToRaster);
        }

        inline bool inBounds(const vec3vf& raster,vec2i& pid) const {
            pid = vec2i{int(std::round(raster.x)),int(std::round(raster.y))};
            return (pid.x>=0&&pid.x<int(mResolution.x) && pid.y>=0 && pid.y<int(mResolution.y));
        }

        inline VRay Cast(vfloat x,vfloat y,const vec2f& uv) const{
            vec3vf raster_point = {x+uv.x,y+uv.y,0.0};
            vec3vf view_dir = transform_point(mRasterToWorld,raster_point);

            VRay rr;
            rr.o = transform_point(mCameraToWorld,zero3vf);
            rr.d = normalize(view_dir-rr.o);
            return rr;
        }
	};

	struct VScene {
		std::string id = "";
		std::vector<std::vector<VResult>> emissive_hints;
		std::map<std::string, VMaterial*> materials;
		std::vector<VNode*> nodes;

		VScene():VScene(""){}

		VScene(std::string idv):id(idv){}

		VNode* root = nullptr;
		VCamera camera;

		inline void shut() {
			if (root) delete root;
			for (auto& m : materials) {
				if (m.second != nullptr) { delete m.second; }
			}
			materials.clear();
		}

		inline VMaterial* add_material(std::string id) {
			if (materials.find(id) == materials.end()) {
				auto m = new VMaterial(id);
				materials[id] = m;
				return m;
			}
			return materials[id];
		}

		bool set_translation(const std::string& idv, const vec3vf& amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_translation(amount);
			return true;
		}
		bool set_translation(const std::string& idv,vfloat x,vfloat y,vfloat z) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_translation(x,y,z);
			return true;
		}

		bool set_rotation(std::string n, const vec3vf& amount) {
			auto node = select(n);
			if (node == nullptr) { return false; }
			node->set_rotation(amount);
			return true;
		}
		bool set_rotation(std::string n,vfloat x,vfloat y,vfloat z) {
			auto node = select(n);
			if (node == nullptr) { return false; }
			node->set_rotation(x,y,z);
			return true;
		}

		bool set_rotation_degs(const std::string& idv, const vec3vf& amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_rotation_degs(amount);
			return true;
		}
		bool set_rotation_degs(const std::string& idv,vfloat x,vfloat y,vfloat z) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_rotation_degs(x,y,z);
			return true;
		}

		bool set_rotation_order(const std::string& idv, VRotationOrder ro){
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_rotation_order(ro);
			return true;
		}

		bool set_scale(const std::string& idv, const vec3vf& amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_scale(amount);
			return true;
		}
		bool set_scale(const std::string& idv,vfloat x,vfloat y,vfloat z) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_scale(x,y,z);
			return true;
		}
		bool set_scale(const std::string& idv, vfloat amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_scale(vec3vf{amount,amount,amount});
			return true;
		}

		bool set_displacement(const std::string idv,displ_ftor ftor){
            auto node = select(idv);
			if (node == nullptr) { return false; }
			node->displacement = ftor;
			return true;
		}

		inline VResult eval(const vec3vf& p) const {
		    VResult res;
			if (root != nullptr) {root->eval(p,res);res.wor_pos = p;} //ensure wor_pos is set to world coord
			return res;
		}
		inline void eval(const vec3vf& p,VResult& res) const {
			if (root != nullptr) {root->eval(p,res);res.wor_pos = p;} // ensure wor_pos is set to world coord
		}

		inline vec3vf eval_normals_tht(const VResult& vre,vfloat eps) const {
			const auto p = vre.wor_pos;
                VResult res;
                root->eval(p + nv1*eps,res);
                auto n = nv1*res.dist;
                root->eval(p + nv2*eps,res);
                n += nv2*res.dist;
                root->eval(p + nv3*eps,res);
                n += nv3*res.dist;
                root->eval(p + nv4*eps,res);
                n += nv4*res.dist;
                return normalize(n);
		}

		inline vec3vf eval_normals_cnt(const VResult& vre,vfloat eps) const {
			const auto p = vre.wor_pos;
			VResult res;

			vec3vf norm = zero3vf;
			root->eval(p + vec3vf{eps,0,0},res);
			norm.x = res.dist;
			root->eval(p - vec3vf{eps,0,0},res);
			norm.x -= res.dist;
			root->eval(p + vec3vf{0,eps,0},res);
			norm.y = res.dist;
			root->eval(p - vec3vf{0,eps,0},res);
			norm.y -= res.dist;
			root->eval(p + vec3vf{0,0,eps},res);
			norm.z = res.dist;
			root->eval(p - vec3vf{0,0,eps},res);
			norm.z -= res.dist;

			return normalize(norm);
		}

		inline VNode* set_root(VNode* n) { root = n; return root; }
		inline VNode* get_root() { return root; }

		inline VNode* select(const std::string& idv) {
			if (root == nullptr) { return nullptr; }
			if (root->id == idv) { return root; }
			return root->select(idv);
		}

        ///Hints about auto omega overrelaxing & fixed omega overrelaxing from (in order) :
        ///http://www.fractalforums.com/3d-fractal-generation/enhanced-sphere-tracing-paper/
        ///http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf
        ///

        ///RisingDaystar lipschitz fixer algorithm:
        ///Allows tracing towards C1 discontinuous volumes and lipschitz > 1 functs , reducing artifacts by A LOT (in awfully distorted scenes, allows the scene to be recognizeable at least...).
        ///I'm using a self crafted technique (i'm still experimenting and testing....),
        ///which can trace inside volumes using a double distance feedback ( "dist" & "vdist" ) , with dist being the absolute distance for primary marching/tracing
        ///and vdist the propagated signed distance trough operators , to determine the "starting" sign of the tracing ray.
        ///dist cannot be used directly for that purpose , because , in the operator, it takes (and then discards) the sign (and value) from the "absolute nearest" isosurface (uses std::abs),
        ///this IS a problem for the purpose of both autofixing lipschitz and over relaxing the ray. Especially, when in need to trace inside arbitrary volumes in a scene,
        ///it simply cannot determine wether it is an "error" or a legit inside trace.
        ///TODO ,over rel & lipschitz fixer for "started inside" tracings
		inline VResult intersect_rel(const VRay& ray,int miters,int& i) const{
		    //const vfloat hmaxf = maxvf/2.0; ///consider using this to avoid over/underflows (depending on sign of the leading expression, it might happen, needs testing) .
		    ///P.S: DON'T use "maxf" , it WILL UNDERFlOW (of course...)
		    constexpr vfloat maxf_m1 = maxvf-1.0;
            vfloat t=0.0;
            vfloat pd=maxf_m1;
            vfloat os=0.0;
            vfloat s = 1.0;
            VResult vre;
            for(i=0;i<miters;i++){
                eval(ray.o+ray.d*t,vre);
                auto d = vre.dist;
                auto absd = std::abs(d);

                if(i==0){s = nz_sign(vre.vdist);pd = maxf_m1;}
                if(s<0){ //started inside
                    if(absd<ray.tmin){vre._found = true;/*vre.wor_pos+=(abs(d)*ray.d);*/ return vre;}
                    t+=absd;
                    pd = d;
                }else{  //started outside
                    if(absd>=std::abs(os)){
                        if(absd<ray.tmin){vre._found = true;/*vre.wor_pos+=(abs(d)*ray.d);*/ return vre;}
                        os=d*std::min(1.0,0.5*d/pd);
                        t+=d+os;
                        pd=d;
                    }else{
                        t-=std::abs(os); // os
                        pd=maxf_m1; //hmaxf
                        os=0.0;
                    }
                }
                if(std::abs(t)>ray.tmax) break;
            }
            vre._found = false;
            return vre;
		}

        ///original naive sphere tracing algorithm
		inline VResult intersect(const VRay& ray, int miters,int& i) const {
			vfloat t = 0.0;
			VResult vre;
			for(i=0;i<miters;i++){
                eval(ray.o + (ray.d*t),vre);
				auto d = std::abs(vre.dist);

				if (d < ray.tmin) { vre._found = true;return vre; }
				t += d;
				if (t > ray.tmax) { break; }
			}
			vre._found = false;
			return vre;
		}
	};

	struct VConfigurable {

		VConfigurable(std::string f) {fname = f;}

		std::map<std::string, std::string> params;
		std::string fname;

		void print() {
			printf("*%s parsed : \n", fname.c_str());
			for (auto const& pms : params) {
				printf("\t%s : %s\n", pms.first.c_str(), pms.second.c_str());
			}
			printf("\n");
		}

		void parse() {
			if (fname == "") { throw VException("Invalid config filename."); }
			std::ifstream in;
			std::string line;
			in.open(fname.c_str(), std::ifstream::in);
			if(!in.is_open()) {
				std::string str = "Could not open config file: ";
				str.append(fname.c_str());
				throw VException(str);
			}

            int lid=0;
			while (std::getline(in, line)) {
                lid++;
				try {eval(line,lid,params);}
				catch (VException& ex) { printf("Config parser {%s}: %s\n", fname.c_str(), ex.what()); continue; }
			}
			in.close();
		}

		inline void eval(const std::string& line,int lid,std::map<std::string, std::string>& params) {
			if (line.empty() || line[0] == ';') { return; }
			int stage = 0;
			std::string name = "", value = "";
			for (auto i = 0; i < line.size(); i++) {
				if (line[i] == ';') { break; }
				if (line[i] == ':') { stage = 1; continue; }
				if (line[i] == ' ' || line[i] == '\t') { continue; }
				if (stage == 0) { name += line[i]; }
				else if (stage == 1) { value += line[i]; }
			}
			if (stage != 1) {
                std::string msg("Syntax error at line ");
                msg+=std::to_string(lid);
                throw VException(msg);
            }

			params.insert(std::pair<std::string, std::string>(name, value));
		}

		std::string try_get(std::string k, std::string dVal) const {
			auto match = params.find(k);
			if (match == params.end()) { return dVal; }
			return match->second;
		}
		double try_get(std::string k, double dVal) const {
			auto match = params.find(k);
			if (match == params.end()) { return dVal; }
			return std::stof(match->second);
		}
		float try_get(std::string k, float dVal) const {
			auto match = params.find(k);
			if (match == params.end()) { return dVal; }
			return std::stof(match->second);
		}
		int try_get(std::string k, int dVal) const { //also bool
			auto match = params.find(k);
			if (match == params.end()) { return dVal; }

			if (stricmp(match->second, "true")) { return 1; }
			if (stricmp(match->second, "false")) { return 0; }

			return std::stoi(match->second);
		}
	};

	struct VRenderer : VConfigurable{
	    VConfigurable* mSharedCfg = nullptr;
		VRenderer(VConfigurable& shared_cfg,std::string cf) : VConfigurable(cf), mSharedCfg(&shared_cfg){
		}

		VStatus status;

		virtual ~VRenderer() {
			shut();
		}
		inline virtual void shut() {}
		inline virtual void init() = 0;
		inline virtual void post_init(VScene& scn) = 0;
		inline virtual std::string type() const = 0;
		inline virtual std::string img_affix(const VScene& scn) const {return std::string("");};
		inline virtual void eval_image(const VScene& scn, ygl::rng_state& rng, image3vf& img, int width,int height, int j) = 0;
	};

	inline void save_ppm(std::string fname, const image3vf& img) {
		const int x = img.size.x, y = img.size.y;
		FILE *fp = fopen((fname + ".ppm").c_str(), "wb");
		fprintf(fp, "P3\n%d %d\n255\n", x, y);

		for (int j = 0; j < y; ++j){
			for (int i = 0; i < x; ++i){
                auto pixel = linear_to_gamma(at(img,{i, j}));
                auto r = (byte)clamp(int(pixel.x * 256), 0, 255);
                auto g = (byte)clamp(int(pixel.y * 256), 0, 255);
                auto b = (byte)clamp(int(pixel.z * 256), 0, 255);
				fprintf(fp, "%d %d %d\n", r, g, b);
			}
		}
		fclose(fp);
	}

	frame3vf calculate_frame(const VNode* node,const frame3vf& parent = identity_frame3vf){
		if (node == nullptr) { return identity_frame3vf; }
		auto fr = parent*ygl::translation_frame(node->translation);

			switch(node->rotation_order){
            case VRO_XYZ:
                fr=fr*ygl::rotation_frame(vec3vf{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(vec3vf{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(vec3vf{ 0,0,1 }, node->rotation.z);
                break;
            case VRO_XZY:
                fr=fr*ygl::rotation_frame(vec3vf{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(vec3vf{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(vec3vf{ 0,1,0 }, node->rotation.y);
                break;
            case VRO_YXZ:
                fr=fr*ygl::rotation_frame(vec3vf{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(vec3vf{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(vec3vf{ 0,0,1 }, node->rotation.z);
                break;
            case VRO_YZX:
                fr=fr*ygl::rotation_frame(vec3vf{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(vec3vf{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(vec3vf{ 1,0,0 }, node->rotation.x);
                break;
            case VRO_ZXY:
                fr=fr*ygl::rotation_frame(vec3vf{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(vec3vf{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(vec3vf{ 0,1,0 }, node->rotation.y);
                break;
            case VRO_ZYX:
                fr=fr*ygl::rotation_frame(vec3vf{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(vec3vf{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(vec3vf{ 1,0,0 }, node->rotation.x);
                break;
			}
			auto n_scale = !cmpf(node->scale,zero3vf) ? one3vf/node->scale : one3vf;
			return fr*scaling_frame(n_scale);
	}

	void apply_transforms(VNode* node, const frame3vf& parent = identity_frame3vf) {
		node->_frame = calculate_frame(node,parent);
		auto chs = node->get_childs();
		if (chs.empty()) { return; }
		for (auto& child : chs) { apply_transforms(child, node->_frame); }
	}

    //TODO : FIX
    /*
    inline vec2i precalc_emissive_hints_rejection(VScene& scn,ygl::rng_state& rng, std::map<std::string, std::vector<VResult>>& emap,VNode* ptr, std::string p_prefix,int n_em_e,vfloat ieps,vfloat neps,bool verbose,vframe3f parent_frame = identity_vframe3f){
		if (ptr == nullptr) { return {0,0}; }
		vec2i stats = {0,0};

		std::string id = p_prefix+ptr->id;
		VResult vre;
        auto fr = calculate_frame(ptr,parent_frame); //HACK : risultato esatto , TODO test
        auto epv = transform_point(fr, vzero3f);
        scn.eval(epv,vre);

        vre._found = true;
        if(vre.vsur!=nullptr && ptr->get_childs().empty() && vre.vsur->id == ptr->id){
            if(emap.find(id)==emap.end()){
                auto vre_material = *vre.vmat;
                if(vre_material.mutator!=nullptr){
                    vvec3f norm = scn.NORMALS_ALGO(vre, neps);
                    vre_material.eval_mutator(rng, vre, norm, vre_material);
                };
                if (vre.vdist<0.0 && vre_material.is_emissive()) {
                    std::vector<VResult>* epoints = &emap[id];
                    epoints->push_back(vre); stats.x++;

                    float r = 1.0;
                    float step = 0.1;
                    std::vector<VResult> old_points;
                    while(true){
                        std::vector<VResult> points;
                        float min_dist = maxf;
                        for(int s=0;s<ceil(n_em_e*(r/4.0));s++){
                            auto p = rand3f_r(rng,-r,r);
                            VResult er;
                            scn.eval(transform_point(fr,p),er);
                            if(er.vdist<0.0 && er.vsur->id == vre.vsur->id){
                                auto er_material = *er.vmat;
                                if(er_material.mutator!=nullptr){
                                    ygl::vec3f norm = scn.NORMALS_ALGO(er, neps);
                                    er_material.eval_mutator(rng, er, norm, er_material);
                                };
                                if(er_material.is_emissive()){
                                    if(cmpf(r,1.0) && (p.x<=r-step && p.x>=-r+step)&& (p.y<=r-step && p.y>=-r+step)&& (p.z<=r-step && p.z>=-r+step)) continue;
                                    min_dist = min(min_dist,distance(er.wor_pos,vre.wor_pos));
                                    er._found = true;
                                    points.push_back(er);
                                    if(verbose) std::cout<<"EM ( light: "<<id<<" ) : {"<<er.wor_pos.x<<","<<er.wor_pos.y<<","<<er.wor_pos.z<<"} with r = "<<r<<"\n";
                                }
                            }
                        }
                        if(points.empty()){break; }
                        old_points = points;
                        for(auto point : old_points){epoints->push_back(point);stats.x++;}
                        step+=step;
                        //step = min_dist;
                        r+=step;
                        if(verbose) std::cout<<"#increasing r to : "<<r<<" ---> sampling in range : "<<r-step <<" | "<<r<<'\n';
                    }
                }
            }
        }
		auto chs = ptr->get_childs();
		if (chs.empty()) { return stats; }
		for (auto node : chs) {
			stats+=precalc_emissive_hints_rejection(scn, rng, emap, node,id, n_em_e, ieps, neps, verbose, fr);
		}
        return stats;
    }*/


    inline vec2i precalc_emissive_hints(VScene& scn,ygl::rng_state& rng, std::map<std::string, std::vector<VResult>>& emap,VNode* ptr,int n_em_e,int n_max_iters,vfloat tmin,vfloat tmax,vfloat neps,bool verbose,frame3vf parent_frame = identity_frame3vf) {
		if (ptr == nullptr) { return {0,0}; }
		vec2i stats = {0,0};

		//std::string id = p_prefix+ptr->id;
        VResult vre;
        auto fr = calculate_frame(ptr,parent_frame); //HACK : risultato esatto , TODO test
        auto epv = transform_point(fr, zero3vf);
        scn.eval(epv,vre);

        vre._found = true;
        auto vre_material = *vre.mat;
        vec3vf norm = scn.NORMALS_ALGO(vre, neps);
        if(vre_material.mutator!=nullptr){
            vre_material.eval_mutator(rng, vre, norm, vre_material);
        };
        auto vre_vmaterial = *vre.vmat;
        if(vre_vmaterial.mutator!=nullptr){
            vre_vmaterial.eval_mutator(rng, vre, norm, vre_vmaterial);
        };
        if(vre_vmaterial.is_emissive() || vre_material.is_emissive()){
            auto dir = sample_sphere_direction_vf(ygl::get_random_vec2f(rng));
            auto ray = VRay{epv,dir,tmin,tmax};
            VResult fvre = vre;
            for (int i=0; i < n_em_e; i++) {
                int n_iters = 0;
                vre = scn.INTERSECT_ALGO(ray,n_max_iters,n_iters);

                if(!vre.found()){
                    dir = sample_sphere_direction_vf(ygl::get_random_vec2f(rng));
                    ray = offsetted_ray(fvre.wor_pos,{},dir,tmin,tmax,zero3vf,0.0);
                    stats.y++;
                    continue;
                }

                auto norm = scn.NORMALS_ALGO(vre, neps);

                //ggx,  for internal emissive bounces
                /*auto rn = ygl::get_random_vec2f(rng);
                auto fp = dot(ray.d,norm) > 0.0f ? ygl::make_frame_fromz(zero3f, -norm) : ygl::make_frame_fromz(zero3f, norm); // TEST -norm && norm
                auto tan2 = 0.1f * 0.1f * rn.y / (1 - rn.y);
                auto rz = std::sqrt(1 / (tan2 + 1)), rr = std::sqrt(1 - rz * rz), rphi = 2 * ygl::pif * rn.x;
                auto wh_local = ygl::vec3f{ rr * std::cos(rphi), rr * std::sin(rphi), rz };
                dir = ygl::transform_direction(fp, wh_local);*/
                //
                auto rn = ygl::get_random_vec2f(rng);
                auto fp = dot(ray.d,norm) > 0.0 ? ygl::make_frame_fromz(zero3vf, -norm) : ygl::make_frame_fromz(zero3vf, norm); // TEST -norm && norm
                dir = ygl::transform_direction(fp,sample_hemisphere_direction_vf(rn));

                dir = -ygl::reflect(ray.d,dir);
                ray = offsetted_ray(vre.wor_pos,{},dir,tmin,tmax,dot(ray.d,norm)>0 ? -norm : norm,vre.dist); //TEST -norm && norm

                auto er_material = *vre.mat;
                if(er_material.mutator!=nullptr){
                    er_material.eval_mutator(rng, vre, norm, er_material);
                }
                auto er_vmaterial = *vre.vmat;
                if(er_vmaterial.mutator!=nullptr){
                    er_vmaterial.eval_mutator(rng, vre, norm, er_vmaterial);
                }
                //if(vre.dist<0.0f && vre.vdist<0.0f){
                    if (er_material.is_emissive()) {
                        std::vector<VResult>* epoints = &emap[vre.sur->id];
                        vre._found = true;
                        epoints->push_back(vre); stats.x++;
                        if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                    }else if(er_vmaterial.is_emissive()){
                        std::vector<VResult>* epoints = &emap[vre.vsur->id];
                        vre._found = true;
                        epoints->push_back(vre); stats.x++;
                        if(verbose) std::cout<<"EM ( light: "<<vre.vsur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                    }
                //}
            }
        }
		auto chs = ptr->get_childs();
		if (chs.empty()) { return stats; }
		for (auto node : chs) {
			stats+=precalc_emissive_hints(scn, rng, emap, node, n_em_e, n_max_iters, tmin, tmax, neps, verbose,fr);
		}

		return stats;
	}

	inline vec2i populate_emissive_hints(VScene& scn,int n_em_evals,int n_max_march_iterations,vfloat f_ray_tmin,vfloat f_ray_tmax,vfloat f_normal_eps,bool verbose = false) {
	    auto rng = ygl::make_rng(ygl::get_time());
		std::map<std::string, std::vector<VResult>> tmpMap;
		vec2i stats = LIGHT_PRECALC_ALGO(scn, rng, tmpMap, scn.root, n_em_evals, n_max_march_iterations, f_ray_tmin, f_ray_tmax, f_normal_eps, verbose);
		for (auto ceh : tmpMap) {
            scn.emissive_hints.push_back(ceh.second);
		}
		return stats;
	}

	///Procedural patterns utilities
    inline bool on_pattern_gradient_oblique(const vec3vf& loc_pos,vfloat angle,float sx){
        angle = radians(angle);
        const vfloat cangle = std::cos(angle);
        const vfloat sangle = std::sin(angle);

        vfloat s = loc_pos.x * cangle - loc_pos.y * sangle;
        if((modulo(s * sx) < 0.5)){
            return true;
        }
        return false;
    }

    inline bool on_pattern_gradient_checker(const vec3vf& loc_pos,vfloat angle,const vec2f& sc){
        angle = radians(angle);
        const vfloat cangle = std::cos(angle);
        const vfloat sangle = std::sin(angle);

        vfloat s = loc_pos.x * cangle - loc_pos.y * sangle;
        vfloat t = loc_pos.y * cangle + loc_pos.x * sangle;
        if((modulo(s * sc.x) < 0.5) || (modulo(t * sc.y) < 0.5)){
            return true;
        }
        return false;
    }
};


#endif
