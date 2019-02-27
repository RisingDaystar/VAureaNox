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

#ifndef _VAUREANOX_H
#define _VAUREANOX_H

#include "yocto\ygl.h"
#include <map>
#include <vector>
#include <queue>
#include <fstream>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
#include <stack>
#include <iostream>
#include <math.h>
#include <sstream>

#define INTERSECT_ALGO intersect_rel
#define NORMALS_ALGO eval_normals_tht
#define LIGHT_PRECALC_ALGO precalc_emissive_hints
//precalc_emissive_hints || precalc_emissive_hints_rejection

using namespace ygl;

namespace vnx {

	struct VResult;
	struct VMaterial;

	struct VNode;
	struct VVolume;
	struct VOperator;

	struct VRay;
	struct VScene;


	typedef float (*displ_ftor)(const ygl::vec3f&);
	typedef void (*mtlm_ftor)(ygl::rng_state& rng,const VResult&,const ygl::vec3f&, VMaterial&);

	using image3f = image<vec3f>;


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

    struct VMemory{
        std::vector<void*> memory;

        VMemory(){
            memory.reserve(8);
        }

        template<typename T> T* New(T* ptr){
            memory.push_back(ptr);
            return ptr;
        }

        ~VMemory(){
            //std::cout<<"Dtor called\n";
            for(void* mptr : memory){
                if(mptr) delete mptr;
                mptr = nullptr;
            }
            memory.clear();
        }
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

	inline float rand1f_r(rng_state& rng, float a, float b) {
        return a + (b - a) * get_random_float(rng);
    }
    inline vec2f rand2f_r(rng_state& rng, float a, float b) {
        return {rand1f_r(rng,a,b),rand1f_r(rng,a,b)};
    }
    inline vec3f rand3f_r(rng_state& rng, float a, float b) {
        return {rand1f_r(rng,a,b),rand1f_r(rng,a,b),rand1f_r(rng,a,b)};
    }

    inline vec3f toVec3f(float v){return {v,v,v};}
    inline vec2f toVec2f(float v){return {v,v};}

    enum ro_axis{
        RO_XYZ,
        RO_XZY,
        RO_YXZ,
        RO_YZX,
        RO_ZXY,
        RO_ZYX,
    };

    enum VMaterialType{
        dielectric,
        conductor,
    };

    enum VIorType{
        wl_dependant,
        non_wl_dependant
    };

	const char* version = "0.06s";

	constexpr float minf = mint<float>();

	const float KC = 299792e3;
    const double KH = 6.63e-34;
    const double KB = 1.38066e-23;
    const double KSB = 5.670373e-8;
    const double KWD = 2.8977721e-3;

	const ygl::vec3f one3f = ygl::vec3f{ 1.0f,1.0f,1.0f };
	const ygl::vec2f one2f = ygl::vec2f{ 1.0f,1.0f };
	const ygl::vec3f mone3f = ygl::vec3f{ -1.0f,-1.0f,-1.0f };
	const ygl::vec2f mone2f = ygl::vec2f{ -1.0f,-1.0f };

    const ygl::vec3f nv1 = { 1.0, -1.0, -1.0 };
    const ygl::vec3f nv2 = { -1.0, -1.0, 1.0 };
    const ygl::vec3f nv3 = { -1.0, 1.0, -1.0 };
    const ygl::vec3f nv4 = { 1.0, 1.0, 1.0 };

	inline bool stricmp(std::string str1, std::string str2) {
		return std::equal(str1.begin(), str1.end(), str2.begin(), [](const char& a, const char& b) {
			return (std::tolower(a) == std::tolower(b));
		});
	}

	inline void print_hhmmss(long long seconds) {
		int min = seconds / 60;
		int hours = min / 60;
		std::cout << (hours % 60) << "h : " << (min % 60) << "m : " << (seconds % 60) << "s\n";
	}

	inline ygl::vec3f rgbtof(float r, float g, float b) {
		return { r / 255.0f,g / 255.0f,b / 255.0f };
	}

    inline float modulo(const float& x){return x - std::floor(x);}

	inline float sign(float v) {
		if (v > 0.0f) { return 1.0f; }
		else if (v < 0.0f) { return -1.0f; }
		return 0.0f;
	}

	inline float nz_sign(float v) { //zero is 1.0f
		if (v >= 0.0f) { return 1.0f; }
		return -1.0f;
	}

	inline float nz_nz_sign(float v) { //zero is -1.0f
		if (v >0.0f) { return 1.0f; }
		return -1.0f;
	}

    inline float cotan(float x){
        return 1.0f/(tan(x));
    }

	inline float radians(float in) {
		return (in*ygl::pif) / 180.0f;
	}

	inline float degrees(float in){
        return (in/ygl::pif)*180;
	}

	//SOURCE : Inigo Quilezles http://www.iquilezles.org/ & GLSL DOCS
	inline float gl_fract(float x) {
		return x - std::floor(x);
	}

	inline float gl_mod(float x,float y){
        return x - y * floor(x/y);
	}
	//

	//SOURCE : Inigo Quilezles http://www.iquilezles.org/ & GLSL DOCS
	inline float hash(float seed){
		return gl_fract(sin(seed)*43758.5453f);
	}
	//

	template <typename T> inline ygl::mat<T,4,4> scaling_matrix(const ygl::vec<T,3>& v){
        return mat<T,4,4>{
            vec<T,4>{v.x,T(0),T(0),T(0)},
            vec<T,4>{T(0),v.y,T(0),T(0)},
            vec<T,4>{T(0),T(0),v.z,T(0)},
            vec<T,4>{T(0),T(0),T(0),T(1)}
        };
	}


	template <typename T> inline ygl::mat<T,4,4> translation_matrix(const ygl::vec<T,3>& v){
        return mat<T,4,4>{
            vec<T,4>{T(1),T(0),T(0),v.x},
            vec<T,4>{T(0),T(1),T(0),v.y},
            vec<T,4>{T(0),T(0),T(1),v.z},
            vec<T,4>{T(0),T(0),T(0),T(1)}
        };
	}



    template<typename T> inline ygl::mat<T,4,4> perspective_matrix(T fov,T aspect, T near, T far) {
        fov = clamp(fov,ygl::epsf,180.0f-ygl::epsf);
        near = max(near,0.0f);
        far = max(far,near+ygl::epsf);

        auto scale = tan(fov*0.5f*ygl::pi<T> / 180.0f)* near;
        auto r = aspect * scale;
        auto l = -r;
        auto t = scale;
        auto b = -t;

        mat<T,4,4> matrix = {
            vec<T,4>{(2*near)/(r-l),0, (r+l)/(r-l), 0},
            vec<T,4>{0,(2*near)/(t-b),(t+b)/(t-b),0},
            vec<T,4>{0,0, -((far+near)/(far-near)), -((2*far*near)/(far-near))},
            vec<T,4>{0,0,0,1},
        };
        return matrix;
    }

    template<typename T> inline ygl::mat<T,4,4> perspective_matrix_rtrace(T fov,T aspect) {
        fov = clamp(fov,ygl::epsf,180.0f-ygl::epsf);
        const T n = 0.01f;
        const T f = 1000.0f;
        T yScale = 1.0f / tan( fov * 0.5f );
        T xScale = yScale / aspect;
        mat<T,4,4> m{
                vec<T,4>{xScale , 0.0f , 0.0f , 0.0f} ,
                vec<T,4>{0.0f , yScale , 0.0f , 0.0f} ,
                vec<T,4>{0.0f , 0.0f , f/(f-n) , -f*n/(f-n)},
                vec<T,4>{0.0f , 0.0f , 1.0f , 0.0f}
                };
        return m;
    }


    template <typename T> inline mat<T,4,4> lookAt(const vec<T,3>& from, const vec<T,3>& to, const vec<T,3>& tmp = vec<T,3>{0,1,0}){
        vec<T,3> xaxis = normalize(from - to);
        vec<T,3> yaxis = cross(normalize(tmp), xaxis);
        vec<T,3> zaxis = cross(xaxis, yaxis);


        mat<T,4,4> viewMatrix = {
            vec<T,4>{      xaxis.x,            yaxis.x,            zaxis.x,       0 },
            vec<T,4>{      xaxis.y,            yaxis.y,            zaxis.y,       0 },
            vec<T,4>{      xaxis.z,            yaxis.z,            zaxis.z,       0 },
            vec<T,4>{-dot( xaxis, from ), -dot( yaxis, from ), -dot( zaxis, from ),  1 }
        };



        return viewMatrix;
    }

	template <typename T> inline T max_element(const ygl::vec<T, 3>& v) { return std::max(v.x, std::max(v.y, v.z)); }
	template <typename T> inline T max_element(const ygl::vec<T, 2>& v) { return std::max(v.x, v.y); }
	template <typename T> inline T min_element(const ygl::vec<T, 3>& v) { return std::min(v.x, std::min(v.y, v.z)); }
	template <typename T> inline T min_element(const ygl::vec<T, 2>& v) { return std::min(v.x, v.y); }
	template <typename T> inline ygl::vec<T, 3> vabs(const ygl::vec<T, 3>& v) { return { std::abs(v.x),std::abs(v.y),std::abs(v.z) }; }
	template <typename T> inline ygl::vec<T, 2> vabs(const ygl::vec<T, 2>& v) { return { std::abs(v.x),std::abs(v.y) }; }
	template <typename T> inline ygl::vec<T, 3> vgt(const ygl::vec<T, 3>& v1, const ygl::vec<T, 3>& v2) { return { std::max(v1.x,v2.x),std::max(v1.y,v2.y),std::max(v1.z,v2.z) }; }
	template <typename T> inline ygl::vec<T, 2> vgt(const ygl::vec<T, 2>& v1, const ygl::vec<T, 2>& v2) { return { std::max(v1.x,v2.x),std::max(v1.y,v2.y) }; }

    using std::exp;
    template <typename T> inline ygl::vec<T,3> exp(const ygl::vec<T,3>& v){return ygl::vec<T,3>{exp(v.x),exp(v.y),exp(v.z)};}
    template <typename T> inline ygl::vec<T,3> gl_mod(const ygl::vec<T,3>& v1,const ygl::vec<T,3>& v2){return ygl::vec<T,3>{gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y),gl_mod(v1.z,v2.z)};}
    template <typename T> inline ygl::vec<T,3> gl_fract(const ygl::vec<T,3>& v){return ygl::vec<T,3>{gl_fract(v.x),gl_fract(v.y),gl_fract(v.z)};}

    using std::log;
    template <typename T> inline ygl::vec<T,3> log(const ygl::vec<T,3>& v){return ygl::vec<T,3>{log(v.x),log(v.y),log(v.z)};}

    using std::max;
    template <typename T> inline ygl::vec<T,3> max(const ygl::vec<T,3>& v1,const ygl::vec<T,3>& v2){return ygl::vec<T,3>{max(v1.x,v2.x),max(v1.y,v2.y),max(v1.z,v2.z)};}
    template <typename T> inline ygl::vec<T,2> max(const ygl::vec<T,2>& v1,const ygl::vec<T,2>& v2){return ygl::vec<T,2>{max(v1.x,v2.x),max(v1.y,v2.y)};}

    using std::min;
    template <typename T> inline ygl::vec<T,3> min(const ygl::vec<T,3>& v1,const ygl::vec<T,3>& v2){return ygl::vec<T,3>{min(v1.x,v2.x),min(v1.y,v2.y),min(v1.z,v2.z)};}
    template <typename T> inline ygl::vec<T,2> min(const ygl::vec<T,2>& v1,const ygl::vec<T,2>& v2){return ygl::vec<T,2>{min(v1.x,v2.x),min(v1.y,v2.y)};}

    using std::abs;
    template <typename T> inline ygl::vec<T,3> abs(const ygl::vec<T,3>& v){return ygl::vec<T,3>{abs(v.x),abs(v.y),abs(v.z)};}
    template <typename T> inline ygl::vec<T,2> abs(const ygl::vec<T,2>& v){return ygl::vec<T,2>{abs(v.x),abs(v.y)};}

    using std::sqrt;
    template <typename T> inline ygl::vec<T,3> sqrt(const ygl::vec<T,3>& v){return ygl::vec<T,3>{sqrt(v.x),sqrt(v.y),sqrt(v.z)};}
    template <typename T> inline ygl::vec<T,2> sqrt(const ygl::vec<T,2>& v){return ygl::vec<T,2>{sqrt(v.x),sqrt(v.y)};}

    template <typename T> inline T adot(const ygl::vec<T,2>& v1,const ygl::vec<T,2>& v2){ return abs(dot(v1,v2));}
    template <typename T> inline T adot(const ygl::vec<T,3>& v1,const ygl::vec<T,3>& v2){ return abs(dot(v1,v2));}
    template <typename T> inline T adot(const ygl::vec<T,4>& v1,const ygl::vec<T,4>& v2){ return abs(dot(v1,v2));}

    inline bool cmpf(float a, float b){
        const float absA = abs(a);
		const float absB = abs(b);
		const float diff = abs(a - b);
		if (a == b) {
			return true;
		} else if (a == 0 || b == 0 || diff < minf) {
			return diff < (ygl::epsf * minf);
		} else {
			return diff / min((absA + absB), maxf) < ygl::epsf;
		}
    }

    inline bool cmpf(const vec3f& a,const vec3f& b){
        return cmpf(a.x,b.x) == cmpf(a.y,b.y) == cmpf(a.z,b.z);
    }

    inline bool has_nan(const vec3f& a){
        return std::isnan(a.x) || std::isnan(a.y) || std::isnan(a.z);
    }

    inline bool has_nnormal(const vec3f& a){
        return !(std::isnormal(a.x) && std::isnormal(a.y) && std::isnormal(a.z));
    }

    inline bool has_inf(const vec3f& a){
        return !(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z));
    }

    inline bool is_zero_or_has_ltz(const vec3f& a){
        return cmpf(a,zero3f) || a.x<-ygl::epsf || a.y<-ygl::epsf || a.z<-ygl::epsf;
    }

    inline std::string _p(const vec3f& v){
        std::ostringstream oss;
        oss << v.x << "," << v.y << "," << v.z;
        return oss.str();
    }
    inline std::string _p(const vec2f& v){
        std::ostringstream oss;
        oss << v.x << "," << v.y;
        return oss.str();
    }
	/////
	/////
	//DUAL NUMBERS AUTOMATIC DERIVATIVES
	/////
	/////

	template<typename T> struct dual3{
        dual3<T>(){
            real = T(0);
            dual.x = real;
            dual.y = real;
            dual.z = real;
        }
	    dual3<T>(const T& v){
            real = v;
            dual.x = 0.0f;
            dual.y = 0.0f;
            dual.z = 0.0f;
	    }

	    dual3<T>(const T& r,const vec<T,3>& d){
            real = r;
            dual = d;
	    }

	    inline T Real(){return real;}
	    inline vec<T,3> Dual(){return dual;}

        T real;
        vec<T,3> dual;
	};

	template <typename T> inline dual3<T> operator +(const dual3<T>& a, const dual3<T>& b){
        return dual3<T>(a.real+b.real,a.dual+b.dual);
	}
	template <typename T> inline dual3<T> operator -(const dual3<T>& a, const dual3<T>& b){
        return dual3<T>(a.real-b.real,a.dual-b.dual);
	}
	template <typename T> inline dual3<T> operator *(const dual3<T>& a, const dual3<T>& b){
        return dual3<T>(
            a.real * b.real,
            a.real * b.dual + a.dual * b.real
        );
	}
	template <typename T> inline dual3<T> operator /(const dual3<T>& a, const dual3<T>& b){
        return dual3<T>(
            a.real / b.real,
            (a.dual * b.real - a.real * b.dual) / (b.real * b.real)
        );
	}

	template <typename T> inline dual3<T> pow(const dual3<T>& a, float y){
        return dual3<T>(
            std::pow(a.real,y),
            (y * a.dual) * std::pow(a.real,y - 1.0f)
        );
	}

	template <typename T> inline dual3<T> sqrt(const dual3<T>& a){
        return dual3<T>(
            std::sqrt(a.real),
            a.dual / (2.0f*std::sqrt(a.real))
        );
	}

	template <typename T> inline dual3<T> sin(const dual3<T>& a){
        return dual3<T>(
            std::sin(a.real),
            a.dual * std::cos(a.real)
        );
	}

	template <typename T> inline dual3<T> cos(const dual3<T>& a){
        return dual3<T>(
            std::cos(a.real),
            -a.dual * std::sin(a.real)
        );
	}

	template <typename T> inline dual3<T> tan(const dual3<T>& a){
	    auto sq_cos = std::cos(a.real);
        return dual3<T>(
            std::tan(a.real),
            a.dual / (sq_cos * sq_cos)
        );
	}

	template <typename T> inline dual3<T> atan(const dual3<T>& a){
        return dual3<T>(
            std::atan(a.real),
            a.dual / (1.0f +(a.real * a.real))
        );
	}

	template <typename T> inline dual3<T> abs(const dual3<T>& a){
        return dual3<T>(
            std::abs(a.real),
            a.dual*sign(a.real)
        );
	}

	template <typename T> inline dual3<T> max(const dual3<T>& a,const dual3<T>& b){
	    return (a.real>=b.real)?a:b;
	}
	template <typename T> inline dual3<T> min(const dual3<T>& a,const dual3<T>& b){
	    return (a.real<b.real)?a:b;
	}

	template<typename T> inline dual3<T> length(const dual3<T>& x,const dual3<T>& y,const dual3<T>& z){
	    auto r = length(vec<T,3>{x.real,y.real,z.real});
	    auto inv = (cmpf(r,0.0f)) ? r: 1.0f/r;
        auto d = (x.dual*x.real + y.dual*y.real + z.dual*z.real) * inv;
        return dual3<T>(r, d);
        /*return dual3<T>(
            length(vec<T,3>{x.real,y.real,z.real}),
            (x.dual*y.dual*z.dual)* ((2.0f*y.real*(y.dual/x.dual)) + (2.0f*z.real*(z.dual/x.dual)) + (2.0f*x.real)) / sqrt(pow(x,2.0f)+pow(y,2.0f)+pow(z,2.0f))
        );*/
        //return sqrt(dot(x,dot(y,z)));
	}

	template<typename T> struct dual3Dom{

	    dual3Dom<T>(const vec3f& p){
            x = dual3<T>(p.x,{1.0f,0.0f,0.0f});
            y = dual3<T>(p.y,{0.0f,1.0f,0.0f});
            z = dual3<T>(p.z,{0.0f,0.0f,1.0f});
	    }

        dual3<T> x;
        dual3<T> y;
        dual3<T> z;
	};


	template<typename T> inline dual3<T> dot(const dual3Dom<T>& a, const dual3Dom<T>& b){

	    dual3<T> t0 = a.x*b.x;
	    dual3<T> t1 = a.y*b.y;
	    dual3<T> t2 = a.z*b.z;
	    return t0+t1+t2;
	}

	/////
	/////
	//DUAL NUMBERS AUTOMATIC DERIVATIVES
	/////
	/////
	//SOURCE : Inigo Quilezles http://www.iquilezles.org/
	inline float smin(float a, float b, float k)
	{
		float h = ygl::clamp(0.5f + 0.5f*(b - a) / k, 0.0f, 1.0f);
		return ygl::lerp(b, a, h) - k*h*(1.0 - h);
	}
	//SOURCE : Inigo Quilezles http://www.iquilezles.org/
	inline float smax(float a, float b, float k)
	{
		float h = ygl::clamp(0.5f + 0.5f*(b - a) / k, 0.0f, 1.0f);
		return ygl::lerp(a, b, h) + k*h*(1.0 - h);
	}


    /*
	inline ygl::vec3f spectral_color(float l) // RGB <0,1> <- lambda l <400,700> [nm]
	{
		double t; double r = 0.0; double g = 0.0; double b = 0.0;
		if ((l >= 400.0) && (l<410.0)) { t = (l - 400.0) / (410.0 - 400.0); r = +(0.33*t) - (0.20*t*t); }
		else if ((l >= 410.0) && (l<475.0)) { t = (l - 410.0) / (475.0 - 410.0); r = 0.14 - (0.13*t*t); }
		else if ((l >= 545.0) && (l<595.0)) { t = (l - 545.0) / (595.0 - 545.0); r = +(1.98*t) - (t*t); }
		else if ((l >= 595.0) && (l<650.0)) { t = (l - 595.0) / (650.0 - 595.0); r = 0.98 + (0.06*t) - (0.40*t*t); }
		else if ((l >= 650.0) && (l<700.0)) { t = (l - 650.0) / (700.0 - 650.0); r = 0.65 - (0.84*t) + (0.20*t*t); }
		if ((l >= 415.0) && (l<475.0)) { t = (l - 415.0) / (475.0 - 415.0); g = +(0.80*t*t); }
		else if ((l >= 475.0) && (l<590.0)) { t = (l - 475.0) / (590.0 - 475.0); g = 0.8 + (0.76*t) - (0.80*t*t); }
		else if ((l >= 585.0) && (l<639.0)) { t = (l - 585.0) / (639.0 - 585.0); g = 0.84 - (0.84*t); }
		if ((l >= 400.0) && (l<475.0)) { t = (l - 400.0) / (475.0 - 400.0); b = +(2.20*t) - (1.50*t*t); }
		else if ((l >= 475.0) && (l<560.0)) { t = (l - 475.0) / (560.0 - 475.0); b = 0.7 - (t)+(0.30*t*t); }
		return {r,g,b};
	}
	*/

    ///Credits : http://bl.ocks.org/benjaminabel/4355926 ///
	inline ygl::vec3f spectral_to_rgb(float w) // RGB <0,1> <- lambda w <380,780> [nm]
	{
		float l = 0.0f; float R = 0.0f; float G = 0.0f; float B = 0.0f;
        if (w >= 380.0f && w < 440.0f) {
            R = -(w - 440.0f) / (440.0f - 350.0f);
            G = 0.0f;
            B = 1.0f;
        } else if (w >= 440.0f && w < 490.0f) {
            R = 0.0f;
            G = (w - 440.0f) / (490.0f - 440.0f);
            B = 1.0f;
        } else if (w >= 490.0f && w < 510.0f) {
            R = 0.0f;
            G = 1.0f;
            B = -(w - 510.0f) / (510.0f - 490.0f);
        } else if (w >= 510.0f && w < 580.0f) {
            R = (w - 510.0f) / (580.0f - 510.0f);
            G = 1.0f;
            B = 0.0f;
        } else if (w >= 580.0f && w < 645.0f) {
            R = 1.0f;
            G = -(w - 645.0f) / (645.0f - 580.0f);
            B = 0.0;
        } else if (w >= 645.0f && w <= 780.0f) {
            R = 1.0f;
            G = 0.0f;
            B = 0.0f;
        } else {
            R = 0.0f;
            G = 0.0f;
            B = 0.0f;
        }

        if (w >= 380.0f && w < 420.0f) {
            l = 0.3f + 0.7f * (w - 350.0f) / (420.0f - 350.0f);
        } else if (w >= 420.0f && w <= 700.0f) {
            l = 1.0f;
        } else if (w > 700.0f && w <= 780.0f) {
            l = 0.3f + 0.7f * (780.0f - w) / (780.0f - 700.0f);
        } else {
            l = 0.0f;
        }
		return {l*R,l*G,l*B};
	}

	inline float blackbody_planks_law_wl(double t,double c,double wl) {
	    wl = wl*1e-9;
	    auto wl5 = wl*wl*wl*wl*wl;
        return (2*KH*c*c) / (wl5*(exp((KH*c)/(wl*KB*t)) -1));
	}

	inline float blackbody_planks_law_wl_normalized(double t,double c,double wl) {
	    auto le = blackbody_planks_law_wl(t,c,wl);
	    float lmax = KWD / t * 1e9; //wien
	    auto lem = blackbody_planks_law_wl(t,c,lmax);
	    return le / lem;
	}

	inline float stefan_boltzman_law(double t){
        return (KSB / ygl::pif)*(t*t*t*t);
	}

	inline float wien_displacement_law(double t){
        return KWD * t;
	}

	inline float wl_from_frquency(double c,double f){
        return c/f;
	}

	inline float f_from_wl(double c,double wl){
        return c/wl;
	}

	inline double thz_to_ghz(double f){
	    return f*1e3;
	}

    //.,.,.,nm^2,nm^2,nm^2,nm
	inline float sellmeier_law(float b1, float b2, float b3, float c1, float c2, float c3, float wl) {
		wl *= 1e-2;
		auto wl2 = wl*wl;
		return std::sqrt(1 +
			((b1*wl2) / (wl2 - c1)) +
			((b2*wl2) / (wl2 - c2)) +
			((b3*wl2) / (wl2 - c3))
		);
	}

	inline float F0_from_ior(float ior){
	    return powf((ior-1.0f)/(ior+1.0f),2.0f);
	}

	struct VRay {
		vec3f o = zero3f;
		vec3f d = zero3f;
		float tmin = 0.001f;
		float tmax = 1000.0f;
		float ior = 1.0f;

		//float owl = 0.0f;
		float wl = 0.0f;
		float frequency = 0.0f;
		float velocity = 0.0f;


        VRay operator-() const {
          VRay rr;
          rr.o = o;
          rr.d = -d;
          rr.tmin = tmin;
          rr.tmax = tmax;
          rr.ior = ior;
          //rr.owl = owl;
          rr.wl = wl;
          rr.frequency = frequency;
          rr.velocity = velocity;
          return rr;
        }
	};

	enum e_axis { X, Y, Z };

	struct VMaterial {
	    VMaterial(): id(""){}
	    VMaterial(std::string idv){id=idv;}
		std::string id = "";
		mtlm_ftor mutator = nullptr;

        VMaterialType type = dielectric;
        VIorType ior_type = wl_dependant;

		ygl::vec3f kr = ygl::zero3f;
		ygl::vec3f ka = ygl::zero3f;

		float k_sca = -1.0f;

		float e_temp = 0.0f;
		float e_power = 0.0f;

		float ior = 1.0f;
		float rs = 0.0f;

		float sm_b1 = 0.0f;
		float sm_b2 = 0.0f;
		float sm_b3 = 0.0f;

		float sm_c1 = 0.0f;
		float sm_c2 = 0.0f;
		float sm_c3 = 0.0f;

		inline bool is_dielectric() const {return type==dielectric;}
		inline bool is_conductor()const {return type==conductor;}

		inline bool is_emissive()  const { return e_temp > ygl::epsf && e_power > ygl::epsf; }
		inline bool is_transmissive() const { return k_sca>=-ygl::epsf;}
		inline bool is_refractive() const { return is_transmissive() && (sm_b1>ygl::epsf || sm_b2>ygl::epsf || sm_b3>ygl::epsf || sm_c1>ygl::epsf || sm_c2>ygl::epsf || sm_c3>ygl::epsf);}

		inline bool is_delta() const{return ((is_conductor() && rs<=ygl::epsf) || (is_transmissive() && rs<=ygl::epsf) || (is_dielectric() && !is_transmissive() && cmpf(kr,zero3f)));}

		inline bool has_kr(){return kr!=zero3f;}

		inline float eval_ior(float wl,float min_wl,float max_wl,bool do_wl_depend = true) const{
		    if(ior_type==non_wl_dependant) return ior;
		    if(!is_refractive()) return 1.0f;

            if(do_wl_depend) return sellmeier_law(sm_b1,sm_b2,sm_b3,sm_c1,sm_c2,sm_c3,wl);

            auto ior_min = sellmeier_law(sm_b1,sm_b2,sm_b3,sm_c1,sm_c2,sm_c3,min_wl);
            auto ior_max = sellmeier_law(sm_b1,sm_b2,sm_b3,sm_c1,sm_c2,sm_c3,max_wl);
            return (ior_min+ior_max) / 2.0f;

		}

		inline void eval_mutator(ygl::rng_state& rng,const VResult& hit,const ygl::vec3f& n, VMaterial& mat) {
			if (mutator != nullptr){
                mutator(rng, hit, n, mat);
			}
		}
	};

	struct VNode {
		std::string id = "";
		frame3f _frame = identity_frame3f;
		vec3f translation = zero3f;
		vec4f rotation = identity_quat4f;
		vec3f scale = one3f;

		ro_axis rotation_order = RO_XYZ;

		displ_ftor displacement = nullptr;

		virtual ~VNode() {};

		virtual std::vector<VNode*> get_childs() = 0;
		virtual VNode* add_child(VNode* child) = 0;
		virtual void add_childs(std::initializer_list<VNode*> chs) = 0;

		virtual inline const char* type() = 0;

		inline float eval_displacement(const vec3f& p){
		    if(displacement!=nullptr){
                return displacement(p);
		    }
		    return 0.0f;
		}

		inline void set_translation(const ygl::vec3f& a) { translation = a; }
		inline void set_translation(float x, float y, float z) { translation = { x,y,z }; }

		inline void set_rotation(const ygl::vec3f& a) { rotation = { a.x,a.y,a.z,1 }; }
		inline void set_rotation(float x, float y, float z) { rotation = { x,y,z,1 }; }
		inline void set_rotation_degs(const ygl::vec3f& a) { rotation = { vnx::radians(a.x),vnx::radians(a.y),vnx::radians(a.z),1 }; }
		inline void set_rotation_degs(float x, float y, float z) { rotation = { vnx::radians(x),vnx::radians(y),vnx::radians(z),1 }; }
		inline void mod_rotation_degs(const ygl::vec3f& a) { rotation.x += vnx::radians(a.x), rotation.y += vnx::radians(a.y); rotation.z += vnx::radians(a.z); }
		inline void mod_rotation_degs(float x, float y, float z) { rotation.x += vnx::radians(x), rotation.y += vnx::radians(y); rotation.z += vnx::radians(z); }

		inline void set_rotation_order(ro_axis ro){rotation_order = ro;}

        inline void set_scale(float s){ if (s <= ygl::epsf) { scale = one3f; return; }  scale = {s,s,s}; }
		inline void set_scale(const ygl::vec3f& a) { if (min_element(a) <= ygl::epsf) { scale = one3f; return; }  scale = a; }
		inline void set_scale(float x, float y, float z) { if (min_element(ygl::vec3f{ x,y,z }) <= ygl::epsf) { scale = one3f; return; } scale = { x,y,z }; }

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

		inline virtual void eval(const vec3f& p,VResult& res) = 0;
	};

	struct VVolume : public VNode {
		VMaterial* material;

		~VVolume() {
			auto chs = get_childs();
			if (chs.empty()) { return; }
			for (auto child : chs) {
				delete child;
			}
		};

		virtual std::vector<VNode*> get_childs() {
			auto child = std::vector<VNode*>();
			child.resize(0);
			return child;
		}
		virtual VNode* add_child(VNode* child) { return nullptr; };
		virtual void add_childs(std::initializer_list<VNode*> chs) {};
	};

	struct VOperator : public VNode {
		std::vector<VNode*> childs;

		~VOperator() {
			auto chs = get_childs();
			if (chs.empty()) { return; }
			for (auto child : chs) {
				delete child;
			}
		};

		VOperator(std::string idv) {
			id = idv;
			childs = std::vector<VNode*>();
			childs.resize(0);
		}
		VOperator(std::string idv,std::initializer_list<VNode*> chs) :VOperator(idv){
			add_childs(chs);
		}

		virtual std::vector<VNode*> get_childs() {
			return childs;
		}
		virtual VNode* add_child(VNode* child) {
			childs.push_back(child);
			return child;
		};
		virtual void add_childs(std::initializer_list<VNode*> chs) {
			if (chs.size() == 0) { return; }
			for (auto child : chs) {
				childs.push_back(child);
			}
		}
	};

	struct VResult {
		VVolume* sur = nullptr;
		VVolume* vsur = nullptr;

		VMaterial* mat = nullptr;
		VMaterial* vmat = nullptr;

		float dist = maxf;
		float vdist = maxf;

		vec3f norm = zero3f; //still unused
		vec3f wor_pos = zero3f;
		vec3f loc_pos = zero3f;

		bool _found = false;

		inline bool valid() {return sur != nullptr && mat!=nullptr;}
		inline bool found() { return _found; }
	};

	struct VCamera{
        //frame3f _frame = identity_frame3f;
        //float aspect = 1.0f;
        float focus = 1.0f;
        float aperture = 0.0f;
        float yfov = 1.0f;

        vec3f mOrigin = zero3f;
        vec3f mTarget = zero3f;
        vec3f mUp = {0,1,0};

        vec2f mResolution;
        float mAspect;

        mat4f mWorldToCamera = identity_mat4f;
        mat4f mWorldToRaster = identity_mat4f;
        mat4f mCameraToRaster = identity_mat4f;
        mat4f mCameraToClip = identity_mat4f;
        mat4f mClipToRaster = identity_mat4f;


        mat4f mCameraToWorld = identity_mat4f;
        mat4f mRasterToWorld = identity_mat4f;
        mat4f mRasterToCamera = identity_mat4f;

        inline void Setup(const vec2f& resolution){
            mResolution = resolution;
            mAspect = float(mResolution.x) / float(mResolution.y);

            mClipToRaster =
                frame_to_mat(scaling_frame(vec3f{mResolution.x,mResolution.y,1.0f}))*
                frame_to_mat(scaling_frame(vec3f{0.5f,-0.5f,1.0f}))*
                frame_to_mat(translation_frame(vec3f{1.0f,-1.0f,0.0f}));

            mCameraToClip = perspective_mat(yfov,mAspect,0.1f,1.0f);
            mCameraToRaster = mClipToRaster*mCameraToClip;
            mWorldToCamera = inverse(frame_to_mat(lookat_frame(mOrigin,mTarget,mUp)));
            mWorldToRaster = mCameraToRaster*mWorldToCamera;


            //Inverse
            mCameraToWorld = inverse(mWorldToCamera);
            mRasterToCamera = inverse(mCameraToRaster);
            mRasterToWorld = inverse(mWorldToRaster);
        }

        inline bool inBounds(const vec3f& raster,vec2i& pid) const {
            pid = vec2i{int(std::round(raster.x)),int(std::round(raster.y))};
            return (pid.x>=0&&pid.x<int(mResolution.x) && pid.y>=0 && pid.y<int(mResolution.y));
        }

        inline VRay Cast(float x,float y,const vec2f& uv) const{
            vec3f raster_point = {x+uv.x,y+uv.y,0.0f};
            vec3f view_dir = transform_point(mRasterToWorld,raster_point);

            VRay rr;
            rr.o = transform_point(mCameraToWorld,zero3f);
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
			for (auto m : materials) {
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

		bool set_translation(const std::string& idv, const ygl::vec3f& amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_translation(amount);
			return true;
		}
		bool set_translation(const std::string& idv,float x,float y,float z) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_translation(x,y,z);
			return true;
		}

		bool set_rotation(std::string n, const ygl::vec3f& amount) {
			auto node = select(n);
			if (node == nullptr) { return false; }
			node->set_rotation(amount);
			return true;
		}
		bool set_rotation(std::string n,float x,float y,float z) {
			auto node = select(n);
			if (node == nullptr) { return false; }
			node->set_rotation(x,y,z);
			return true;
		}

		bool set_rotation_degs(const std::string& idv, const ygl::vec3f& amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_rotation_degs(amount);
			return true;
		}
		bool set_rotation_degs(const std::string& idv,float x,float y,float z) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_rotation_degs(x,y,z);
			return true;
		}

		bool set_rotation_order(const std::string& idv, ro_axis ro){
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_rotation_order(ro);
			return true;
		}

		bool set_scale(const std::string& idv, const ygl::vec3f& amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_scale(amount);
			return true;
		}
		bool set_scale(const std::string& idv,float x,float y,float z) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_scale(x,y,z);
			return true;
		}
		bool set_scale(const std::string& idv, float amount) {
			auto node = select(idv);
			if (node == nullptr) { return false; }
			node->set_scale(vec3f{amount,amount,amount});
			return true;
		}

		bool set_displacement(const std::string idv,displ_ftor ftor){
            auto node = select(idv);
			if (node == nullptr) { return false; }
			node->displacement = ftor;
			return true;
		}

		inline VResult eval(const vec3f& p) const {
		    VResult res;
			if (root != nullptr) {root->eval(p,res);res.wor_pos = p;} //ensure wor_pos is set to world coord
			return res;
		}
		inline void eval(const vec3f& p,VResult& res) const {
			if (root != nullptr) {root->eval(p,res);res.wor_pos = p;} // ensure wor_pos i set to worl coord
		}

		inline virtual ygl::vec3f eval_normals_tht(const VResult& vre,float eps) const {
            if(!cmpf(vre.norm,zero3f)){return vre.norm;}
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

		inline virtual ygl::vec3f eval_normals_cnt(const VResult& vre,float eps) const {
            if(!cmpf(vre.norm,zero3f)){return vre.norm;}
			const auto p = vre.wor_pos;
			VResult res;

			vec3f norm = zero3f;
			root->eval(p + vec3f{eps,0,0},res);
			norm.x = res.dist;
			root->eval(p - vec3f{eps,0,0},res);
			norm.x -= res.dist;
			root->eval(p + vec3f{0,eps,0},res);
			norm.y = res.dist;
			root->eval(p - vec3f{0,eps,0},res);
			norm.y -= res.dist;
			root->eval(p + vec3f{0,0,eps},res);
			norm.z = res.dist;
			root->eval(p - vec3f{0,0,eps},res);
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
		    const float hmaxf = maxf/2.0f; ///consider using this to avoid over/underflows (depending on sign of the leading expression, it might happen, needs testing) .
		    ///P.S: DON'T use "maxf" , it WILL UNDERFlOW (of course...)
		    const float maxf_m1 = maxf-1.0f;
            float t=0.0;
            float pd=maxf_m1;
            float os=0.0;
            float s = 1.0f;
            VResult vre;
            for(i=0;i<miters;i++){
                eval(ray.o+ray.d*t,vre);
                auto d = vre.dist;
                auto absd = abs(d);

                if(i==0){s = nz_sign(vre.vdist);pd = maxf_m1;}
                if(s<0){ //started inside
                    if(absd<ray.tmin){vre._found = true;/*vre.wor_pos+=(abs(d)*ray.d);*/ return vre;}
                    t+=absd;
                    pd = d;
                }else{  //started outside
                    if(absd>=abs(os)){
                        if(absd<ray.tmin){vre._found = true;/*vre.wor_pos+=(abs(d)*ray.d);*/ return vre;}
                        os=d*min(1.0f,0.5f*d/pd);
                        t+=d+os;
                        pd=d;
                    }else{
                        t-=abs(os); // os
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
			float t = 0.0f;
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
			for (auto pms : params) {
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
			for (int i = 0; i < line.size(); i++) {
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
		VRenderer(std::string cf) : VConfigurable(cf){}

		VStatus status;

		virtual ~VRenderer() {
			shut();
		}
		inline virtual void shut() {}
		inline virtual void init() = 0;
		inline virtual void post_init(VScene& scn) = 0;
		inline virtual std::string type() const = 0;
		inline virtual std::string img_affix(const VScene& scn) const {return std::string("");};
		inline virtual void eval_image(const VScene& scn, ygl::rng_state& rng, image3f& img, int width,int height, int j) = 0;
	};

	inline void save_ppm(std::string fname, const image3f& img) {
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

	inline ygl::vec3f rand_in_hemisphere_cos(const vec3f& pos, const ygl::vec3f& norm,const vec2f& rn) {
		auto fp = ygl::make_frame_fromz(pos, norm);
		auto rz = sqrtf(rn.x), rr = sqrtf(1 - rz * rz), rphi = 2 * ygl::pif * rn.y;
		auto wi_local = ygl::vec3f{ rr * cosf(rphi), rr * sinf(rphi), rz };
		return ygl::transform_direction(fp, wi_local);
	}

	inline VRay offsetted_ray(const ygl::vec3f& o,VRay rr, const ygl::vec3f& d, const float tmin, const float tmax, const ygl::vec3f& offalong, const float dist, float fmult = 2.0f) {
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

	inline bool in_range_eps(float v,float vref,float eps){
        return (v>=vref-eps) && (v<=vref+eps);
	}

	void apply_transforms(VNode* node, const ygl::frame3f& parent = ygl::identity_frame3f) {
		if (node == nullptr) { return; }
		auto fr = parent *ygl::translation_frame(node->translation);
            /* *ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x)*
			ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y)*
			ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z);
			*/
			//ni don't apply scaling here, i consider it as a deformation in nodes evals
			switch(node->rotation_order){
            case RO_XYZ:
                fr=fr*ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z);
                break;
            case RO_XZY:
                fr=fr*ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y);
                break;
            case RO_YXZ:
                fr=fr*ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z);
                break;
            case RO_YZX:
                fr=fr*ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x);
                break;
            case RO_ZXY:
                fr=fr*ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x)*
                    ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y);
                break;
            case RO_ZYX:
                fr=fr*ygl::rotation_frame(ygl::vec3f{ 0,0,1 }, node->rotation.z)*
                    ygl::rotation_frame(ygl::vec3f{ 0,1,0 }, node->rotation.y)*
                    ygl::rotation_frame(ygl::vec3f{ 1,0,0 }, node->rotation.x);
                break;
			}

		node->_frame = fr;
		auto chs = node->get_childs();
		if (chs.empty()) { return; }
		for (auto child : chs) { apply_transforms(child, fr); }
	}

    //TODO : FIX
    inline precalc_emissive_hints_rejection(VScene& scn,ygl::rng_state& rng, std::map<std::string, std::vector<VResult>>& emap,VNode* ptr, std::string p_prefix,int n_em_e,float ieps,float neps,bool verbose){
		if (ptr == nullptr) { return 0; }
		int n = 0;

		std::string id = p_prefix+ptr->id;
		VResult vre;
        ptr->eval(transform_point(ptr->_frame,zero3f),vre);
        vre._found = true;
        if(vre.vsur!=nullptr && ptr->get_childs().empty() && vre.vsur->id == ptr->id){
            if(emap.find(id)==emap.end()){
                auto vre_material = *vre.vmat;
                if(vre_material.mutator!=nullptr){
                    ygl::vec3f norm = scn.NORMALS_ALGO(vre, neps);
                    vre_material.eval_mutator(rng, vre, norm, vre_material);
                };
                if (vre.vdist<0.0f && vre_material.is_emissive()) {
                    std::vector<VResult>* epoints = &emap[id];
                    epoints->push_back(vre); n++;

                    float r = 1.0f;
                    float step = 0.1f;
                    std::vector<VResult> old_points;
                    while(true){
                        std::vector<VResult> points;
                        float min_dist = maxf;
                        for(int s=0;s<ceil(n_em_e*(r/4.0f));s++){
                            auto p = rand3f_r(rng,-r,r);
                            VResult er;
                            scn.eval(transform_point(ptr->_frame,p),er);
                            if(er.vdist<0.0f && er.vsur->id == vre.vsur->id){
                                auto er_material = *er.vmat;
                                if(er_material.mutator!=nullptr){
                                    ygl::vec3f norm = scn.NORMALS_ALGO(er, neps);
                                    er_material.eval_mutator(rng, er, norm, er_material);
                                };
                                if(er_material.is_emissive()){
                                    if(cmpf(r,1.0f) && (p.x<=r-step && p.x>=-r+step)&& (p.y<=r-step && p.y>=-r+step)&& (p.z<=r-step && p.z>=-r+step)) continue;
                                    min_dist = min(min_dist,distance(er.wor_pos,vre.wor_pos));
                                    er._found = true;
                                    points.push_back(er);
                                    if(verbose) std::cout<<"EM ( light: "<<id<<" ) : {"<<er.wor_pos.x<<","<<er.wor_pos.y<<","<<er.wor_pos.z<<"} with r = "<<r<<"\n";
                                }
                            }
                        }
                        if(points.empty()){break; }
                        old_points = points;
                        for(auto point : old_points){epoints->push_back(point);n++;}
                        step+=step;
                        //step = min_dist;
                        r+=step;
                        if(verbose) std::cout<<"#increasing r to : "<<r<<" ---> sampling in range : "<<r-step <<" | "<<r<<'\n';
                    }
                }
            }
        }
		auto chs = ptr->get_childs();
		if (chs.empty()) { return n; }
		for (auto node : chs) {
			n+=precalc_emissive_hints_rejection(scn, rng, emap, node,id, n_em_e, ieps, neps, verbose);
		}
        return n;
    }

    inline int precalc_emissive_hints(VScene& scn,ygl::rng_state& rng, std::map<std::string, std::vector<VResult>>& emap,VNode* ptr, std::string p_prefix,int n_em_e,float ieps,float neps,bool verbose) {
		if (ptr == nullptr) { return 0; }
		int n = 0;

		//std::string id = p_prefix+ptr->id;
        VResult vre;
        scn.eval(transform_point(ptr->_frame,zero3f),vre);
        vre._found = true;
        auto vre_material = *vre.mat;
        ygl::vec3f norm = scn.NORMALS_ALGO(vre, neps);
        if(vre_material.mutator!=nullptr){
            vre_material.eval_mutator(rng, vre, norm, vre_material);
        };
        auto vre_vmaterial = *vre.vmat;
        if(vre_vmaterial.mutator!=nullptr){
            vre_vmaterial.eval_mutator(rng, vre, norm, vre_vmaterial);
        };
        if(vre_vmaterial.is_emissive() || vre_material.is_emissive()){
            /*
            vec3f dir = normalize(rand3f_r(rng,-1.0f,1.0f));
            VResult er = vre;
            VRay ray = offsetted_ray(er.wor_pos,{},dir,ieps,1000.0f,zero3f,0.0f);

            if(er.dist<0.0f || er.vdist<0.0f){
                if (vre_material.is_emissive()) {
                    std::vector<VResult>* epoints = &emap[er.sur->id];
                    er._found = true;
                    epoints->push_back(er); n++;
                    if(verbose) std::cout<<"EM ( light: "<<er.sur->id<<" ) : {"<<er.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                }else if(vre_vmaterial.is_emissive()){
                    std::vector<VResult>* epoints = &emap[er.vsur->id];
                    er._found = true;
                    epoints->push_back(er); n++;
                    if(verbose) std::cout<<"EM ( light: "<<er.vsur->id<<" ) : {"<<er.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                }
            }*/
            auto dir = ygl::sample_sphere_direction(ygl::get_random_vec2f(rng));
            auto ray = VRay{vre.wor_pos,dir,ieps,1000.0f};
            VResult fvre = vre;
            for (int i=0; i < n_em_e; i++) {
                int n_iters = 0;
                vre = scn.INTERSECT_ALGO(ray,512,n_iters);

                //int jj = 0;
                /*auto new_er = scn.INTERSECT_ALGO(ray, 512,jj);
                if(!new_er.found()){
                    dir = normalize(rand3f_r(rng,-1.0f,1.0f));
                    ray = offsetted_ray(er.wor_pos,{},dir,ieps,1000.0f,zero3f,0.0f);
                    if(verbose) std::cout<<"Er not found.\n";
                    continue;
                }
                auto new_norm = scn.NORMALS_ALGO(new_er, neps);
                */

                if(!vre.found()){
                    dir = normalize(rand3f_r(rng,-1.0f,1.0f));
                    ray = offsetted_ray(fvre.wor_pos,{},dir,ieps,1000.0f,zero3f,0.0f);
                    continue;
                }

                auto norm = scn.NORMALS_ALGO(vre, neps);

                ///ggx,  for internal emissive bounces
                auto rn = ygl::get_random_vec2f(rng);
                auto fp = dot(ray.d,norm) >= 0.0f ? ygl::make_frame_fromz(zero3f, norm) : ygl::make_frame_fromz(zero3f, -norm);
                auto tan2 = 0.1f * 0.1f * rn.y / (1 - rn.y);
                auto rz = std::sqrt(1 / (tan2 + 1)), rr = std::sqrt(1 - rz * rz), rphi = 2 * ygl::pif * rn.x;
                auto wh_local = ygl::vec3f{ rr * std::cos(rphi), rr * std::sin(rphi), rz };
                dir = ygl::transform_direction(fp, wh_local);
                ///

                dir = -ygl::reflect(ray.d,dir);
                ray = offsetted_ray(vre.wor_pos,{},dir,ieps,1000.0f,dot(ray.d,norm)>=0 ? norm : -norm,vre.dist);

                auto er_material = *vre.mat;
                if(er_material.mutator!=nullptr){
                    er_material.eval_mutator(rng, vre, norm, er_material);
                }
                auto er_vmaterial = *vre.vmat;
                if(er_vmaterial.mutator!=nullptr){
                    er_vmaterial.eval_mutator(rng, vre, norm, er_vmaterial);
                }
                if(vre.dist<0.0f && vre.vdist<0.0f){
                    if (er_material.is_emissive()) {
                        std::vector<VResult>* epoints = &emap[vre.sur->id];
                        vre._found = true;
                        epoints->push_back(vre); n++;
                        if(verbose) std::cout<<"EM ( light: "<<vre.sur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                    }else if(er_vmaterial.is_emissive()){
                        std::vector<VResult>* epoints = &emap[vre.vsur->id];
                        vre._found = true;
                        epoints->push_back(vre); n++;
                        if(verbose) std::cout<<"EM ( light: "<<vre.vsur->id<<" ) : {"<<vre.wor_pos.x<<","<<vre.wor_pos.y<<","<<vre.wor_pos.z<<"}\n";
                    }
                }
            }
        }
		auto chs = ptr->get_childs();
		if (chs.empty()) { return n; }
		for (auto node : chs) {
			n+=precalc_emissive_hints(scn, rng, emap, node,ptr->id, n_em_e, ieps, neps, verbose);
		}

		return n;
	}

	inline int populate_emissive_hints(VScene& scn,int n_em_e,float ieps,float neps,bool verbose = false) {
	    auto rng = ygl::make_rng(ygl::get_time());
		std::map<std::string, std::vector<VResult>> tmpMap;
		int n = LIGHT_PRECALC_ALGO(scn, rng, tmpMap, scn.root, "", n_em_e, ieps, neps, verbose);
		for (auto ceh : tmpMap) {
            scn.emissive_hints.push_back(ceh.second);
		}
		return n;
	}

	///Procedural patterns utilities
    inline bool on_pattern_gradient_oblique(const vec3f& loc_pos,float angle,float sx){
        angle = radians(angle);
        const float cangle = std::cos(angle);
        const float sangle = std::sin(angle);

        float s = loc_pos.x * cangle - loc_pos.y * sangle;
        float t = loc_pos.y * cangle + loc_pos.x * sangle;
        if((modulo(s * sx) < 0.5f)){
            return true;
        }
        return false;
    }

    inline bool on_pattern_gradient_checker(const vec3f& loc_pos,float angle,const vec2f& sc){
        angle = radians(angle);
        const float cangle = std::cos(angle);
        const float sangle = std::sin(angle);

        float s = loc_pos.x * cangle - loc_pos.y * sangle;
        float t = loc_pos.y * cangle + loc_pos.x * sangle;
        if((modulo(s * sc.x) < 0.5f) || (modulo(t * sc.y) < 0.5f)){
            return true;
        }
        return false;
    }
};


#endif
