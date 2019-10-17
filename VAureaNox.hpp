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

#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
//#include <stack>

#include <iostream>
#include <fstream>

#include <math.h>
#include <sstream>

#include <string>

using namespace ygl;

namespace vnx {

	using uint = unsigned int;

    template< typename C > struct is_char : std::integral_constant<bool, std::is_same<C, char>::value> {};
    template< typename C > struct is_wchar : std::integral_constant<bool, std::is_same<C, char16_t>::value || std::is_same<C, char32_t>::value || std::is_same<C, wchar_t>::value> {};

    using vec4d = vec<double,4>;
	using vec3d = vec<double,3>;
	using vec2d = vec<double,2>;
	using frame3d = frame<double,3>;
	using mat4d = mat<double, 4, 4>;
	using mat3d = mat<double, 3, 3>;
	using mat2d = mat<double, 2, 2>;

	using image3f = image<vec3f>;
	using image3d = image<vec3d>;

	template<typename T>
    constexpr T thrt(){return T(0.00001);}

	constexpr const auto epsd = epst<double>();
	constexpr const auto mind = mint<double>();
	constexpr const auto maxd = maxt<double>();
	constexpr const auto pid  = ygl::pi<double>;
    constexpr const auto thrd = thrt<double>();

	constexpr frame3d identity_frame3d = frame3d{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
	constexpr vec4d identity_quat4d = vec4d{0,0,0,1};
	constexpr mat4d identity_mat4d = mat4d{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
	constexpr mat3d identity_mat3d = mat3d{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	constexpr mat2d identity_mat2d = mat2d{{1, 0}, {0, 1}};

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

	constexpr vec4d one4d = one4<double>;
	constexpr vec3d one3d = one3<double>;
	constexpr vec2d one2d = one2<double>;

	constexpr vec4d zero4d = zero4<double>;
	constexpr vec3d zero3d = zero3<double>;
	constexpr vec2d zero2d = zero2<double>;

    template<typename T,int N>
    constexpr vec<T,N> toVec(T v){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
        if constexpr(N<=1) return {v};
        else if constexpr(N==2) return {v,v};
        else if constexpr(N==3) return {v,v,v};
        else if constexpr(N>3) return {v,v,v,v};
    }

	constexpr double KC = 299792e3;
    constexpr double KH = 6.63e-34;
    constexpr double KB = 1.38066e-23;
    constexpr double KSB = 5.670373e-8;
    constexpr double KWD = 2.8977721e-3;


	struct VResult;
	struct VMaterial;
	struct VNode;
	struct VSdf;
	struct VSdfOperator;
	struct VRay;
	struct VScene;
	struct VSceneParser;
	struct VRenderer;
	struct VConfigurable;


	typedef double (*displ_ftor)(const vec3d&);
	typedef void (*mtlm_ftor)(const VResult&,const vec3d&, VMaterial&);

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
        diffuse,
        dielectric,
        conductor,
    };

    enum VIorType{
        wl_dependant,
        non_wl_dependant
    };

    struct VStatus{ //TODO : extend
        VStatus():bDebugMode(false),bPauseMode(false),bFinished(false),bStopped(false){};
        bool bDebugMode = false;
        bool bPauseMode = false;
        bool bFinished = false;
        bool bStopped = false;

        long unsigned int mRaysEvaled = 0;
        long unsigned int mRaysLost = 0;
    };

	class VException : public std::exception{
	    public:
        VException(const std::string& msg) :std::exception(msg.c_str()){}
	};

    //RNG UTILS

    struct VRng_pcg32{
        uint64_t state = 0U;
        uint64_t inc = 0U;

        VRng_pcg32(){}
        VRng_pcg32(uint64_t sd,uint64_t seq=1){
            seed(sd,seq);
        }

        inline void seed(uint64_t sd,uint64_t seq=1){
            state = 0U;
            inc   = (seq << 1u) | 1u;
            next();
            state += sd;
            next();
        }

        inline uint32_t next(){
            uint64_t oldstate = state;
            state = oldstate * 6364136223846793005ULL + (inc|1);

            uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
            uint32_t rot = oldstate >> 59u;
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
        }

        inline uint32_t next_uint(uint32_t ub){
            uint32_t x = next();
            uint64_t m = uint64_t(x) * uint64_t(ub);
            uint32_t l = uint32_t(m);
            if (l < ub) {
                uint32_t t = -ub;
                if (t >= ub) {
                    t -= ub;
                    if (t >= ub)
                        t %= ub;
                }
                while (l < t) {
                    x = next();
                    m = uint64_t(x) * uint64_t(ub);
                    l = uint32_t(m);
                }
            }
            return m >> 32;
        }
        inline double next_double(){
            double y = (double) next();
            return y/((uint32_t(std::numeric_limits<int32_t>::max())+1)*2.0);
        }
        inline float next_float(){
            float y = (float) next();
            return y/((uint32_t(std::numeric_limits<int32_t>::max())+1)*2.0);
        }

        inline double next_double(double ub){
            double y = (double) next();
            return y/(((uint32_t(std::numeric_limits<int32_t>::max())+1)*2.0)/ub);
        }
        inline float next_float(float ub){
            float y = (float) next();
            return y/(((uint32_t(std::numeric_limits<int32_t>::max())+1)*2.0)/ub);
        }

        inline double next_double(double lb,double ub){
            if(lb>ub) std::swap(lb,ub);
            double y = (double) next();
            return lb+(y/(((uint32_t(std::numeric_limits<int32_t>::max())+1)*2.0)/(ub-lb)));
        }
        inline float next_float(float lb,float ub){
            if(lb>ub) std::swap(lb,ub);
            float y = (float) next();
            return lb+(y/(((uint32_t(std::numeric_limits<int32_t>::max())+1)*2.0)/(ub-lb)));
        }

        template<int N>
        inline vec<float,N> next_vecf(){
            static_assert(N>0 && N<5,"N must be in 1 | 4 range");
            if constexpr(N==1) return {next_float()};
            if constexpr(N==2) return {next_float(),next_float()};
            if constexpr(N==3) return {next_float(),next_float(),next_float()};
            if constexpr(N==4) return {next_float(),next_float(),next_float(),next_float()};
        }

        template<int N>
        inline vec<double,N> next_vecd(){
            static_assert(N>0 && N<5,"N must be in 1 | 4 range");
            if constexpr(N==1) return {next_double()};
            if constexpr(N==2) return {next_double(),next_double()};
            if constexpr(N==3) return {next_double(),next_double(),next_double()};
            if constexpr(N==4) return {next_double(),next_double(),next_double(),next_double()};
        }

    };


    struct VRng_pcg32x2{
        VRng_pcg32 gen[2];

        VRng_pcg32x2(){}
        VRng_pcg32x2(uint64_t sd,uint64_t seq=1){
            seed(sd,sd,seq,seq+1);
        }
        VRng_pcg32x2(uint64_t sd1,uint64_t sd2,uint64_t seq1=1,uint64_t seq2=2){
            seed(sd1,sd2,seq1,seq2);
        }

        inline void seed(uint64_t sd1,uint64_t sd2,uint64_t seq1=1,uint64_t seq2=2){
            uint64_t mask = ~0ull >> 1;
            if ((seq1 & mask) == (seq2 & mask))
                seq2 = ~seq2;
            gen[0].seed(sd1,seq1);
            gen[1].seed(sd2,seq2);
        }

        inline uint64_t next(){
            return ((uint64_t)(gen[0].next()) << 32) | gen[1].next();
        }

        inline uint32_t next_uint(uint32_t ub){//TRUNCATES TO 32bit
            uint32_t x = next();
            uint64_t m = uint64_t(x) * uint64_t(ub);
            uint32_t l = uint32_t(m);
            if (l < ub) {
                uint32_t t = -ub;
                if (t >= ub) {
                    t -= ub;
                    if (t >= ub)
                        t %= ub;
                }
                while (l < t) {
                    x = next();
                    m = uint64_t(x) * uint64_t(ub);
                    l = uint32_t(m);
                }
            }
            return m >> 32;
        }
        inline double next_double(){
            double y = (double) next();
            return y/((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0);
        }
        inline float next_float(){
            float y = (float) next();
            return y/((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0);
        }

        inline double next_double(double ub){
            double y = (double) next();
            return y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/ub);
        }
        inline float next_float(float ub){
            float y = (float) next();
            return y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/ub);
        }

        inline double next_double(double lb,double ub){
            if(lb>ub) std::swap(lb,ub);
            double y = (double) next();
            return lb+(y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/(ub-lb)));
        }
        inline float next_float(float lb,float ub){
            if(lb>ub) std::swap(lb,ub);
            float y = (float) next();
            return lb+(y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/(ub-lb)));
        }

        template<int N>
        inline vec<float,N> next_vecf(){
            static_assert(N>0 && N<5,"N must be in 1 | 4 range");
            if constexpr(N==1) return {next_float()};
            if constexpr(N==2) return {next_float(),next_float()};
            if constexpr(N==3) return {next_float(),next_float(),next_float()};
            if constexpr(N==4) return {next_float(),next_float(),next_float(),next_float()};
        }

        template<int N>
        inline vec<double,N> next_vecd(){
            static_assert(N>0 && N<5,"N must be in 1 | 4 range");
            if constexpr(N==1) return {next_double()};
            if constexpr(N==2) return {next_double(),next_double()};
            if constexpr(N==3) return {next_double(),next_double(),next_double()};
            if constexpr(N==4) return {next_double(),next_double(),next_double(),next_double()};
        }

    };


    struct VRng_splmix64{
        //implements splitmix64 algorithm
        //http://prng.di.unimi.it/
        uint64_t state;

        VRng_splmix64(){}
        VRng_splmix64(uint64_t sd){
            seed(sd);
        }

        void seed(uint64_t sd){
            state = sd;
        }

        uint64_t next() {
            uint64_t z = (state += 0x9e3779b97f4a7c15);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
            z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
            return z ^ (z >> 31);
        }

        inline uint32_t next_uint(uint32_t ub){//TRUNCATES TO 32bit
            uint32_t x = next();
            uint64_t m = uint64_t(x) * uint64_t(ub);
            uint32_t l = uint32_t(m);
            if (l < ub) {
                uint32_t t = -ub;
                if (t >= ub) {
                    t -= ub;
                    if (t >= ub)
                        t %= ub;
                }
                while (l < t) {
                    x = next();
                    m = uint64_t(x) * uint64_t(ub);
                    l = uint32_t(m);
                }
            }
            return m >> 32;
        }
        inline double next_double(){
            double y = (double) next();
            return y/((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0);
        }
        inline float next_float(){
            float y = (float) next();
            return y/((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0);
        }

        inline double next_double(double ub){
            double y = (double) next();
            return y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/ub);
        }
        inline float next_float(float ub){
            float y = (float) next();
            return y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/ub);
        }

        inline double next_double(double lb,double ub){
            if(lb>ub) std::swap(lb,ub);
            double y = (double) next();
            return lb+(y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/(ub-lb)));
        }
        inline float next_float(float lb,float ub){
            if(lb>ub) std::swap(lb,ub);
            float y = (float) next();
            return lb+(y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/(ub-lb)));
        }

        template<int N>
        inline vec<float,N> next_vecf(){
            static_assert(N>0 && N<5,"N must be in 1 | 4 range");
            if constexpr(N==1) return {next_float()};
            if constexpr(N==2) return {next_float(),next_float()};
            if constexpr(N==3) return {next_float(),next_float(),next_float()};
            if constexpr(N==4) return {next_float(),next_float(),next_float(),next_float()};
        }

        template<int N>
        inline vec<double,N> next_vecd(){
            static_assert(N>0 && N<5,"N must be in 1 | 4 range");
            if constexpr(N==1) return {next_double()};
            if constexpr(N==2) return {next_double(),next_double()};
            if constexpr(N==3) return {next_double(),next_double(),next_double()};
            if constexpr(N==4) return {next_double(),next_double(),next_double(),next_double()};
        }

    };

    struct VRng_xrs256ss{ //TODO
        //implements xoroshiro256** algorithm
        //http://prng.di.unimi.it/xoshiro256starstar.c
        //http://prng.di.unimi.it/
        protected:
            static inline uint64_t rotl(const uint64_t x, int k) {
                return (x << k) | (x >> (64 - k));
            }
        public:
            uint64_t state[4];

            VRng_xrs256ss(){}
            VRng_xrs256ss(uint64_t sd){
                seed(sd);
            }

            inline void seed(uint64_t sd){
                VRng_splmix64 smx;
                smx.seed(sd);
                state[0] = smx.next();
                state[1] = smx.next();
                state[2] = smx.next();
                state[3] = smx.next();
            }

            inline uint64_t next() {
                const uint64_t result = rotl(state[1] * 5, 7) * 9;
                const uint64_t t = state[1] << 17;

                state[2] ^= state[0];
                state[3] ^= state[1];
                state[1] ^= state[2];
                state[0] ^= state[3];

                state[2] ^= t;

                state[3] = rotl(state[3], 45);

                return result;
            }

            inline uint32_t next_uint(uint32_t ub){//TRUNCATES TO 32bit
                uint32_t x = next();
                uint64_t m = uint64_t(x) * uint64_t(ub);
                uint32_t l = uint32_t(m);
                if (l < ub) {
                    uint32_t t = -ub;
                    if (t >= ub) {
                        t -= ub;
                        if (t >= ub)
                            t %= ub;
                    }
                    while (l < t) {
                        x = next();
                        m = uint64_t(x) * uint64_t(ub);
                        l = uint32_t(m);
                    }
                }
                return m >> 32;
            }
            inline double next_double(){
                double y = (double) next();
                return y/((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0);
            }
            inline float next_float(){
                float y = (float) next();
                return y/((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0);
            }

            inline double next_double(double ub){
                double y = (double) next();
                return y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/ub);
            }
            inline float next_float(float ub){
                float y = (float) next();
                return y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/ub);
            }

            inline double next_double(double lb,double ub){
                if(lb>ub) std::swap(lb,ub);
                double y = (double) next();
                return lb+(y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/(ub-lb)));
            }
            inline float next_float(float lb,float ub){
                if(lb>ub) std::swap(lb,ub);
                float y = (float) next();
                return lb+(y/(((uint64_t(std::numeric_limits<int64_t>::max())+1)*2.0)/(ub-lb)));
            }

            template<int N>
            inline vec<float,N> next_vecf(){
                static_assert(N>0 && N<5,"N must be in 1 | 4 range");
                if constexpr(N==1) return {next_float()};
                if constexpr(N==2) return {next_float(),next_float()};
                if constexpr(N==3) return {next_float(),next_float(),next_float()};
                if constexpr(N==4) return {next_float(),next_float(),next_float(),next_float()};
            }

            template<int N>
            inline vec<double,N> next_vecd(){
                static_assert(N>0 && N<5,"N must be in 1 | 4 range");
                if constexpr(N==1) return {next_double()};
                if constexpr(N==2) return {next_double(),next_double()};
                if constexpr(N==3) return {next_double(),next_double(),next_double()};
                if constexpr(N==4) return {next_double(),next_double(),next_double(),next_double()};
            }

            inline void jump(void) {
                constexpr uint64_t JUMP[] = { 0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };

                uint64_t s0 = 0;
                uint64_t s1 = 0;
                uint64_t s2 = 0;
                uint64_t s3 = 0;
                for(int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
                    for(int b = 0; b < 64; b++) {
                        if (JUMP[i] & UINT64_C(1) << b) {
                            s0 ^= state[0];
                            s1 ^= state[1];
                            s2 ^= state[2];
                            s3 ^= state[3];
                        }
                        next();
                    }

                state[0] = s0;
                state[1] = s1;
                state[2] = s2;
                state[3] = s3;
            }

            inline void long_jump(void) {
                constexpr uint64_t LONG_JUMP[] = { 0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };

                uint64_t s0 = 0;
                uint64_t s1 = 0;
                uint64_t s2 = 0;
                uint64_t s3 = 0;
                for(int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
                    for(int b = 0; b < 64; b++) {
                        if (LONG_JUMP[i] & UINT64_C(1) << b) {
                            s0 ^= state[0];
                            s1 ^= state[1];
                            s2 ^= state[2];
                            s3 ^= state[3];
                        }
                        next();
                    }

                state[0] = s0;
                state[1] = s1;
                state[2] = s2;
                state[3] = s3;
            }
    };

    using VRng = VRng_pcg32x2; //DEFAULT VRng_pcg32



    template<typename T>
    struct VImg{
        vec2i mResolution;
        std::vector<vec<T,3>> mPixels;
        VImg(){}
        VImg(const vec2i& res){
            mResolution = res;
            mPixels.resize(res.x*res.y);
        }

        inline vec<T,3>& at(const vec2i& pid) {
            return mPixels[pid.y * mResolution.x + pid.x];
        }
        inline vec<T,3>& at(int pidx,int pidy) {
            return mPixels[pidy * mResolution.x + pidx];
        }
        inline const vec<T,3>& at(const vec2i& pid) const{
            return mPixels[pid.y * mResolution.x + pid.x];
        }
        inline const vec<T,3>& at(int pidx,int pidy) const{
            return mPixels[pidy * mResolution.x + pidx];
        }
    };

    struct VFilm{
        struct VPixel{
            vec3d wr = zero3d;
            int num = 0;
        };

        vec2i mResolution = zero2i;
        std::vector<VPixel> mPixels;
        double mScale = 1.0;
        bool mScaleOnSplat = false;

        VFilm(vec2i resolution) : mResolution(resolution){
            mPixels.resize(resolution.x*resolution.y);
            mScale = 1.0;
        }

        inline void Splat(const vec2i& pid,vec3d cc){
            if(!(pid.x>=0&&pid.x<mResolution.x && pid.y>=0 && pid.y<mResolution.y)) return;
            auto& px = mPixels[(pid.y*mResolution.x)+pid.x];
            px.wr += mScaleOnSplat ? (cc*mScale) : cc;
            px.num++;
        }

        inline void SetScale(double v){mScale = v;}

        inline void SetScaleOnSplat(bool v){mScaleOnSplat = v;}

        inline VPixel& at(const vec2i& pid) {
            return mPixels[pid.y * mResolution.x + pid.x];
        }
        inline VPixel& at(int pidx,int pidy) {
            return mPixels[pidy * mResolution.x + pidx];
        }
        inline const VPixel& at(const vec2i& pid) const{
            return mPixels[pid.y * mResolution.x + pid.x];
        }
        inline const VPixel& at(int pidx,int pidy) const{
            return mPixels[pidy * mResolution.x + pidx];
        }
    };

	//////////////////////////
	//String and Parsing Utils/
	//////////////////////////

    struct VEntry{
        VEntry(){}
        VEntry(std::vector<std::string> tks){for(auto tk : tks)tokens.push_back(tk);}
        void add_token(std::string tk){tokens.push_back(tk);}
        std::string try_at(std::vector<std::string>::size_type idx) const{if(idx>=tokens.size())return std::string(); return tokens[idx];}
        std::string& at(std::vector<std::string>::size_type idx){return tokens[idx];}
        bool empty(){return tokens.empty();}
        std::vector<std::string>::size_type size(){return tokens.size();}
        bool allocated(){return ptr!=nullptr;}


        std::vector<std::string> tokens;
        void* ptr = nullptr;
    };

    struct VMappedEntry{
        VMappedEntry(){}
        VMappedEntry(std::map<std::string,std::string> tks){for(auto tk : tks)mMappings[tk.first] = tk.second;}
        void add_token(std::string k,std::string v){mMappings[k] = v;}
        std::string try_get(const std::string& k) const{
            auto e = mMappings.find(k);
            if(e==mMappings.end()) return std::string();
            return e->second;
        }
        std::string& at(const std::string& k){return mMappings[k];}
        bool empty(){return mMappings.empty();}
        bool allocated(){return ptr!=nullptr;}


        std::map<std::string,std::string> mMappings;
        void* ptr = nullptr;
    };


    template<typename T>
    inline float strto_f(const std::basic_string<T>& str,T** eptr=nullptr){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_f argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return wcstof(str.c_str(),eptr);
        else return strtof(str.c_str(),eptr);
    }
    template<typename T>
    inline float strto_f(const T* cstr,T** eptr=nullptr){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_f argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return wcstof(cstr,eptr);
        else return strtof(cstr,eptr);
    }

    template<typename T>
    inline double strto_d(const std::basic_string<T>& str,T** eptr=nullptr){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_d argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return wcstod(str.c_str(),eptr);
        else return strtod(str.c_str(),eptr);
    }
    template<typename T>
    inline double strto_d(const T* cstr,T** eptr=nullptr){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_d argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return wcstod(cstr,eptr);
        else return strtod(cstr,eptr);
    }

    template<typename T>
    inline long strto_l(const std::basic_string<T>& str,T** eptr=nullptr,int radix = 10){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_l argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return wcstol(str.c_str(),eptr,radix);
        else return strtol(str.c_str(),eptr,radix);
    }
    template<typename T>
    inline long strto_l(const T* cstr,T** eptr=nullptr,int radix = 10){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_l argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return wcstol(cstr,eptr,radix);
        else return strtol(cstr,eptr,radix);
    }

    template<typename L,typename T>
    inline long strto_limi(const std::basic_string<T>& str,T** eptr=nullptr,int radix = 10){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_lim argument must be a litteral.");
        static_assert(std::is_integral<L>::value,"strto_lim limit must be an integer fixed floating point type");
        constexpr L _lmax = std::numeric_limits<L>::max();
        constexpr L _lmin = std::numeric_limits<L>::min();

        long v = 0;
        if constexpr(is_wchar<T>::value){v = wcstol(str.c_str(),eptr,radix);}
        else{v = strtol(str.c_str(),eptr,radix);}

        if (v > _lmax) {
            errno = ERANGE;
            return _lmax;
        }else if (v < _lmin) {
            errno = ERANGE;
            return _lmin;
        }
        return v;
    }
    template<typename L,typename T>
    inline long strto_limi(const T* cstr,T** eptr=nullptr,int radix = 10){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_lim argument must be a litteral.");
        static_assert(std::is_integral<L>::value,"strto_lim limit must be an integer fixed floating point type");
        constexpr L _lmax = std::numeric_limits<L>::max();
        constexpr L _lmin = std::numeric_limits<L>::min();

        long v = 0;
        if constexpr(is_wchar<T>::value){v = wcstol(cstr,eptr,radix);}
        else{v = strtol(cstr,eptr,radix);}

        if (v > _lmax) {
            errno = ERANGE;
            return _lmax;
        }else if (v < _lmin) {
            errno = ERANGE;
            return _lmin;
        }
        return v;
    }

    template<typename T>
    inline int strto_i(const std::basic_string<T>& str,T** eptr=nullptr,int radix = 10){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_i argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return strto_limi<int>(str.c_str(),eptr,radix);
        else return strto_limi<int>(str.c_str(),eptr,radix);
    }
    template<typename T>
    inline int strto_i(const T* cstr,T** eptr=nullptr,int radix = 10){
        static_assert(is_wchar<T>::value || is_char<T>::value,"strto_i argument must be a litteral.");
        if constexpr(is_wchar<T>::value) return strto_limi<int>(cstr,eptr,radix);
        else return strto_limi<int>(cstr,eptr,radix);
    }

    template<typename T>
	inline bool stricmp(const std::basic_string<T>& str1,const std::basic_string<T>& str2) {
	    static_assert(is_wchar<T>::value || is_char<T>::value,"stricmp template argument must be a litteral.");
	    if(str1.size()!=str2.size()) return false;
		return std::equal(str1.begin(), str1.end(), str2.begin(), [](const T& a, const T& b) {
			return (std::tolower(a) == std::tolower(b));
		});
	}

    template<typename T>
    inline bool strIsGroup(const std::basic_string<T>& ss){
        return ss[0]==T('{') && ss[ss.length()-1]==T('}');
    }

    template<typename T>
    inline std::vector<std::basic_string<T>> strDeGroup(const std::basic_string<T>& ss){
        if(!strIsGroup(ss)) return {ss};
        int idx = 0;
        std::vector<std::basic_string<T>> tokens = {std::basic_string<T>("")};
        for(typename std::basic_string<T>::size_type i=1;i<ss.length()-1;i++){
            T c = ss[i];
            if(c==T(',')){tokens.push_back(std::basic_string<T>(""));idx++;continue;}
            tokens[idx]+=c;
        }
        return tokens;
    }

    template<typename T>
    inline VAxis try_strToAxis(const std::basic_string<T>& ss,VAxis def){
        if(ss.empty()) return def;
        if(stricmp(ss,std::basic_string<T>("x"))) return VAxis::X;
        if(stricmp(ss,std::basic_string<T>("y"))) return VAxis::Y;
        if(stricmp(ss,std::basic_string<T>("z"))) return VAxis::Z;
        return def;
    }

    template<typename T>
    inline VMaterialType try_strToMaterialType(const std::basic_string<T>& ss,VMaterialType def){
        if(ss.empty()) return def;
        if(stricmp(ss,std::basic_string<T>("diff")) || stricmp(ss,std::basic_string<T>("diffuse"))) return VMaterialType::diffuse;
        if(stricmp(ss,std::basic_string<T>("diel")) || stricmp(ss,std::basic_string<T>("dielectric"))) return VMaterialType::dielectric;
        if(stricmp(ss,std::basic_string<T>("cond")) || stricmp(ss,std::basic_string<T>("conductor"))) return VMaterialType::conductor;
        return def;
    }

    template<typename T>
    inline VIorType try_strToIorType(const std::basic_string<T>& ss,VIorType def){
        if(ss.empty()) return def;
        if(stricmp(ss,std::basic_string<T>("wl"))) return VIorType::wl_dependant;
        if(stricmp(ss,std::basic_string<T>("nwl"))) return VIorType::non_wl_dependant;
        return def;
    }

    template<typename T>
    inline VRotationOrder try_strToRotationOrder(const std::basic_string<T>& ss,VRotationOrder def){
        if(ss.empty()) return def;
        if(stricmp(ss,std::basic_string<T>("xyz"))) return VRO_XYZ;
        if(stricmp(ss,std::basic_string<T>("xzy"))) return VRO_XZY;
        if(stricmp(ss,std::basic_string<T>("yxz"))) return VRO_YXZ;
        if(stricmp(ss,std::basic_string<T>("yzx"))) return VRO_YZX;
        if(stricmp(ss,std::basic_string<T>("zxy"))) return VRO_ZXY;
        if(stricmp(ss,std::basic_string<T>("zyx"))) return VRO_ZYX;
        return def;
    }

    template<typename T>
    inline double try_strtod(const std::basic_string<T>& ss,double def){
        if(ss.empty()) return def;
        return strto_d(ss);
    }

    template<typename T>
    inline float try_strtof(const std::basic_string<T>& ss,float def){
        if(ss.empty()) return def;
        return strto_f(ss);
    }

    template<typename T>
    inline int try_strtoi(const std::basic_string<T>& ss,int def){
        if(ss.empty()) return def;
        return strto_i(ss);
    }

    template<typename T>
    inline bool try_strtob(const std::basic_string<T>& ss,bool def){
        if(ss.empty()) return def;
        if(ss.length()>1){
            if(stricmp(ss,std::basic_string<T>("true"))) return true;
            if(stricmp(ss,std::basic_string<T>("false"))) return false;
        }
        return strto_limi<bool>(ss);
    }

    template<typename T,int N>
    inline vec<int,N> try_strToVec_i(const std::basic_string<T>& ss,const vec<int,N>& def){
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<int,N> v;
            for(typename std::vector<std::basic_string<T>>::size_type i=0;i<cpnts.size() && i<N;i++){
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

    template<typename T,int N>
    inline vec<bool,N> try_strToVec_b(const std::basic_string<T>& ss,const vec<bool,N>& def){
        static_assert(N>0 && N<5, "N must be in range 1 | 4");
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<bool,N> v;
            for(typename std::vector<std::basic_string<T>>::size_type i=0;i<cpnts.size() && i<N;i++){

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

    template<typename T,int N>
    inline vec<float,N> try_strToVec_f(const std::basic_string<T>& ss,const vec<float,N>& def){
        static_assert(N>0 && N<5, "N must be in range 1 | 4");
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<float,N> v;
            for(typename std::vector<std::basic_string<T>>::size_type i=0;i<cpnts.size() && i<N;i++){
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

    template<typename T,int N>
    inline vec<double,N> try_strToVec_d(const std::basic_string<T>& ss,const vec<double,N>& def){
        static_assert(N>0 && N<5, "N must be in range 1 | 4");
        if(ss.empty()) return def;

        if(strIsGroup(ss)){
            auto cpnts = strDeGroup(ss);
            vec<double,N> v;
            for(typename std::vector<std::basic_string<T>>::size_type i=0;i<cpnts.size() && i<N;i++){
                if constexpr(N>=1){if(i==0){v.x = try_strtof(cpnts[0],0);continue;}}
                if constexpr(N>=2){if(i==1){v.y = try_strtof(cpnts[1],0);continue;}}
                if constexpr(N>=3){if(i==2){v.z = try_strtof(cpnts[2],0);continue;}}
                if constexpr(N>=4){if(i==3){v.w = try_strtof(cpnts[3],0);}}
            }
            return v;
        }else{
            auto cpnt = try_strtof(ss,0);
            return toVec<double,N>(cpnt);
        }
    }

	inline void print_hhmmss(long long seconds) {
		int min = seconds / 60;
		int hours = min / 60;
		std::cout << (hours % 60) << "h : " << (min % 60) << "m : " << (seconds % 60) << "s\n";
	}

	//////////////////////////
	//////////////////////////
	//////////////////////////

	template<typename T>
	inline T avg(const vec<T,3>& v){
	    return (v.x+v.y+v.z) / T(3);
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
	inline double degrees(T in){
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

	template <typename T>
	constexpr T min_bv(const T& v1,const T& v2) {
	    return v1 < v2 ? v1 : v2;
    }
	template <typename T>
	constexpr T max_bv(const T& v1,const T& v2) {
	    return v1 > v2 ? v1 : v2;
    }

	template <typename T,int N>
	constexpr T max_element(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return v.x;
	    else if constexpr(N==2) return std::max(v.x, v.y);
	    else if constexpr(N==3) return std::max(v.x, std::max(v.y, v.z));
	    else if constexpr(N==4) return std::max(std::max(v.x,v.y), std::max(v.z, v.w));
    }

	template <typename T,int N>
	constexpr T min_element(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return v.x;
	    else if constexpr(N==2) return std::min(v.x, v.y);
	    else if constexpr(N==3) return std::min(v.x, std::min(v.y, v.z));
	    else if constexpr(N==4) return std::min(std::min(v.x,v.y), std::min(v.z, v.w));
    }

	template <typename T,int N>
	constexpr vec<T,N> abs(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::abs(v.x)};
	    else if constexpr(N==2) return {std::abs(v.x),std::abs(v.y)};
	    else if constexpr(N==3) return {std::abs(v.x),std::abs(v.y),std::abs(v.z)};
	    else if constexpr(N==4) return {std::abs(v.x),std::abs(v.y),std::abs(v.z),std::abs(v.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> exp(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::exp(v.x)};
	    else if constexpr(N==2) return {std::exp(v.x),std::exp(v.y)};
	    else if constexpr(N==3) return {std::exp(v.x),std::exp(v.y),std::exp(v.z)};
	    else if constexpr(N==4) return {std::exp(v.x),std::exp(v.y),std::exp(v.z),std::exp(v.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> pow(const ygl::vec<T, N>& v,T p) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::pow(v.x,p)};
	    else if constexpr(N==2) return {std::pow(v.x,p),std::pow(v.y,p)};
	    else if constexpr(N==3) return {std::pow(v.x,p),std::pow(v.y,p),std::pow(v.z,p)};
	    else if constexpr(N==4) return {std::pow(v.x,p),std::pow(v.y,p),std::pow(v.z,p),std::pow(v.w,p)};
    }
	template <typename T,int N>
	constexpr vec<T,N> pow(const ygl::vec<T, N>& v,unsigned int p) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::pow(v.x,p)};
	    else if constexpr(N==2) return {std::pow(v.x,p),std::pow(v.y,p)};
	    else if constexpr(N==3) return {std::pow(v.x,p),std::pow(v.y,p),std::pow(v.z,p)};
	    else if constexpr(N==4) return {std::pow(v.x,p),std::pow(v.y,p),std::pow(v.z,p),std::pow(v.w,p)};
    }

	template <typename T,int N>
	constexpr vec<T,N> log(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::log(v.x)};
	    else if constexpr(N==2) return {std::log(v.x),std::log(v.y)};
	    else if constexpr(N==3) return {std::log(v.x),std::log(v.y),std::log(v.z)};
	    else if constexpr(N==4) return {std::log(v.x),std::log(v.y),std::log(v.z),std::log(v.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> sqrt(const ygl::vec<T, N>& v) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::sqrt(v.x)};
	    else if constexpr(N==2) return {std::sqrt(v.x),std::sqrt(v.y)};
	    else if constexpr(N==3) return {std::sqrt(v.x),std::sqrt(v.y),std::sqrt(v.z)};
	    else if constexpr(N==4) return {std::sqrt(v.x),std::sqrt(v.y),std::sqrt(v.z),std::sqrt(v.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> min(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::min(v1.x,v2.x)};
	    else if constexpr(N==2) return {std::min(v1.x,v2.x),std::min(v1.y,v2.y)};
	    else if constexpr(N==3) return {std::min(v1.x,v2.x),std::min(v1.y,v2.y),std::min(v1.z,v2.z)};
	    else if constexpr(N==4) return {std::min(v1.x,v2.x),std::min(v1.y,v2.y),std::min(v1.z,v2.z),std::min(v1.w,v2.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> max(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {std::max(v1.x,v2.x)};
	    else if constexpr(N==2) return {std::max(v1.x,v2.x),std::max(v1.y,v2.y)};
	    else if constexpr(N==3) return {std::max(v1.x,v2.x),std::max(v1.y,v2.y),std::max(v1.z,v2.z)};
	    else if constexpr(N==4) return {std::max(v1.x,v2.x),std::max(v1.y,v2.y),std::max(v1.z,v2.z),std::max(v1.w,v2.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> gl_mod(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {gl_mod(v1.x,v2.x)};
	    else if constexpr(N==2) return {gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y)};
	    else if constexpr(N==3) return {gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y),gl_mod(v1.z,v2.z)};
	    else if constexpr(N==4) return {gl_mod(v1.x,v2.x),gl_mod(v1.y,v2.y),gl_mod(v1.z,v2.z),gl_mod(v1.w,v2.w)};
    }

	template <typename T,int N>
	constexpr vec<T,N> gl_fract(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return {gl_fract(v1.x,v2.x)};
	    else if constexpr(N==2) return {gl_fract(v1.x,v2.x),gl_fract(v1.y,v2.y)};
	    else if constexpr(N==3) return {gl_fract(v1.x,v2.x),gl_fract(v1.y,v2.y),gl_fract(v1.z,v2.z)};
	    else if constexpr(N==4) return {gl_fract(v1.x,v2.x),gl_fract(v1.y,v2.y),gl_fract(v1.z,v2.z),gl_fract(v1.w,v2.w)};
    }

	template <typename T,int N>
	constexpr T adot(const ygl::vec<T, N>& v1,const ygl::vec<T, N>& v2) {
	    return std::abs(dot(v1,v2));
    }

    /*
    //Not affected by Aspect Ratio
    template <typename T>
    constexpr inline mat<T, 4, 4> perspective_mat(T fov, T near, T far) {
        fov *= pid/ 360.0;
        const double f = 1.0 / (std::tan(fov));
        const double d = 1.0 / (near-far);
        return {
            {f, 0, 0, 0},
            {0, -f, 0, 0},
            {0, 0, (near+far)*d, -1.0},
            {0, 0, 2.0*near*far*d, 0}
            };
    }

    template <typename T = double>
    constexpr inline mat<T, 4, 4>  perspective_mat(T fov, T aspect, T near) {
        fov *= pid/ 360.0;
        const double f = (std::tan(fov) / 2.0);
        return {
            {1.0/(aspect*f), 0, 0, 0},
            {0, 1.0/f, 0, 0},
            {0, 0, -1.0, -1.0},
            {0, 0, 2.0*near, 0}
            };
    }*/

    template<typename T>
    inline mat<T,4,4> glLookAtMat(const vec<T,3>& eye,const vec<T,3>& target,const vec<T,3>& up = {0,(T)1.0,0},bool inv_xz=true){
        vec<T,3> forward = normalize(eye-target);
        vec<T,3> left = cross(normalize(up),forward);
        vec<T,3> c_up = cross(forward,left);
        mat<T,4,4> lookat = identity_mat4d;

        if (inv_xz) {
            forward = -forward;
            left = -left;
        }

        lookat.x = {left.x,c_up.x,forward.x,0};
        lookat.y = {left.y,c_up.y,forward.y,0};
        lookat.z = {left.z,c_up.z,forward.z,0};

        lookat.w.x = eye.x;
        lookat.w.y = eye.y;
        lookat.w.z = eye.z;

        return lookat;
    }


    constexpr mat4d scaling_mat(const vec3d& sc){
        mat4d m = identity_mat4d;
        m.x.x = sc.x;
        m.y.y = sc.y;
        m.z.z = sc.z;
        m.w.w = 1.0;
        return m;
    }

    constexpr mat4d translation_mat(const vec3d& tr){
        mat4d m = identity_mat4d;
        m.w.x = tr.x;
        m.w.y = tr.y;
        m.w.z = tr.z;
        m.w.w = 1.0;
        return m;
    }

    template<typename T>
    inline constexpr double absmax(T v1,T v2){
        v1 = std::abs(v1);
        v2 = std::abs(v2);
        return (v1>v2) ?  v1 : v2;
    }

    template<typename T>
    inline constexpr double absmin(T v1,T v2){
        v1 = std::abs(v1);
        v2 = std::abs(v2);
        return (v1>v2) ?  v2 : v1;
    }

    template<typename T>
    inline constexpr double signed_absmax(T v1,T v2){
        if(std::abs(v1)>std::abs(v2)) return v1;
        return v2;
    }
    template<typename T>
    inline constexpr double signed_absmin(T v1,T v2){
        if(std::abs(v1)>std::abs(v2)) return v2;
        return v1;
    }

    template<typename T>
    constexpr bool cmpf(T a, T b){
        T min_a = a -(a-std::nextafter(a,std::numeric_limits<T>::lowest()));
        T max_a = a +(std::nextafter(a,std::numeric_limits<T>::max())-a);
        return min_a <= b && max_a >= b;
    }

    template<typename T,int N>
    constexpr bool cmpf(const vec<T,N>& a,const vec<T,N>& b){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return cmpf(a.x,b.x);
	    else if constexpr(N==2) return cmpf(a.x,b.x) && cmpf(a.y,b.y);
	    else if constexpr(N==3) return cmpf(a.x,b.x) && cmpf(a.y,b.y) && cmpf(a.z,b.z);
	    else if constexpr(N==4) return cmpf(a.x,b.x) && cmpf(a.y,b.y) && cmpf(a.z,b.z) && cmpf(a.w,b.w);
    }

    template<typename T,int N>
    constexpr bool has_nan(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return std::isnan(a.x);
	    else if constexpr(N==2) return std::isnan(a.x) || std::isnan(a.y);
	    else if constexpr(N==3) return std::isnan(a.x) || std::isnan(a.y) || std::isnan(a.z);
	    else if constexpr(N==4) return std::isnan(a.x) || std::isnan(a.y) || std::isnan(a.z) || std::isnan(a.w);
    }

    template<typename T,int N>
    constexpr bool has_nnormal(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return !(std::isnormal(a.x));
	    else if constexpr(N==2) return !(std::isnormal(a.x) && std::isnormal(a.y));
	    else if constexpr(N==3) return !(std::isnormal(a.x) && std::isnormal(a.y) && std::isnormal(a.z));
	    else if constexpr(N==4) return !(std::isnormal(a.x) && std::isnormal(a.y) && std::isnormal(a.z) && std::isnormal(a.w));
    }

    template<typename T,int N>
    constexpr bool has_inf(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
	    if constexpr(N==1) return !(std::isfinite(a.x));
	    else if constexpr(N==2) return !(std::isfinite(a.x) && std::isfinite(a.y));
	    else if constexpr(N==3) return !(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z));
	    else if constexpr(N==4) return !(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z) && std::isfinite(a.w));
    }

    template<typename T,int N>
    constexpr bool is_zero_or_has_ltz(const vec<T,N>& a){
        static_assert(N>0 && N<5,"N must be in range 1 | 4");
        if constexpr(N==1) return cmpf(a,{0}) || a.x<-epst<T>();
        else if constexpr(N==2) return cmpf(a,zero2<T>) || a.x<-epst<T>() || a.y<-epst<T>();
        else if constexpr(N==3) return cmpf(a,zero3<T>) || a.x<-epst<T>() || a.y<-epst<T>() || a.z<-epst<T>();
        else if constexpr(N==4) return cmpf(a,zero4<T>) || a.x<-epst<T>() || a.y<-epst<T>() || a.z<-epst<T>() || a.w<-epst<T>();
    }

    template<typename T>
	constexpr bool in_range_eps(T v,T vref,T eps){
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

	template<typename T>
	inline constexpr T fsipow(T x,unsigned int e){
        if(e==0)return 1.0;
        if(e%2 == 0){
            T h = fsipow(x,e/2);
            return h*h;
        }else{
            T h = fsipow(x,e/2);
            return h*h*x;
        }
    }

	template<typename T,int N>
	inline constexpr vec<T,N> fsipow(const vec<T,N>& x,unsigned int e){
        if(e==0)return toVec<T,N>(1.0);
        if(e%2 == 0){
            vec<T,N> h = fsipow(x,e/2);
            return h*h;
        }else{
            vec<T,N> h = fsipow(x,e/2);
            return h*h*x;
        }
    }


	template<typename T,int D=3>
	inline constexpr T smax(T a,T b,T k){
	    constexpr T c = 1.0/(2.0*D);
		const T h = std::max(k-std::abs(a-b),0.0) / k;
		return std::max(a,b) + fsipow(h,D)*k*c;
	}

	template<typename T,int D=3>
	inline constexpr T smin(T a,T b,T k){
	    constexpr T c = 1.0/(2.0*D);
		const T h = std::max(k-std::abs(a-b),0.0) / k;
		return std::min(a,b) - fsipow(h,D)*k*c;
	}



    template<typename T>
    constexpr vec<T,3> gamma_to_linear(const vec<T,3>& srgb, T gamma = 2.2f) {
        return pow(srgb,gamma);//{std::pow(srgb.x, gamma), std::pow(srgb.y, gamma), std::pow(srgb.z, gamma)};
    }
    template<typename T>
    constexpr vec<T,3> linear_to_gamma(const vec<T,3>& lin, T gamma = 2.2f) {
        return pow(lin,1.0 / gamma);
    }
    template<typename T>
    constexpr vec<T,4> gamma_to_linear(const vec<T,4>& srgb, T gamma = 2.2f) {
        return {std::pow(srgb.x, gamma), std::pow(srgb.y, gamma), std::pow(srgb.z, gamma), srgb.w};
    }
    template<typename T>
    constexpr vec<T,4> linear_to_gamma(const vec<T,4>& lin, T gamma = 2.2f) {
        return {std::pow(lin.x, 1.0 / gamma), std::pow(lin.y, 1.0 / gamma), std::pow(lin.z, 1.0 / gamma),
            lin.w};
    }

    ///Credits : http://bl.ocks.org/benjaminabel/4355926 ///
	constexpr vec3d spectral_to_rgb(double w){ // RGB <0,1> <- lambda w <380,780> [nm]
		double l = 0.0; double R = 0.0; double G = 0.0; double B = 0.0;
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

	inline double blackbody_planks_law_wl(double t,double c,double wl) {
	    wl = wl*1e-9;
	    auto wl5 = wl*wl*wl*wl*wl;
        return (2.0*KH*c*c) / (wl5*(std::exp((KH*c)/(wl*KB*t)) -1.0));
	}

	inline double blackbody_planks_law_wl_normalized(double t,double c,double wl) {
	    auto le = blackbody_planks_law_wl(t,c,wl);
	    double lmax = KWD / t * 1e9; //wien
	    auto lem = blackbody_planks_law_wl(t,c,lmax);
	    return le / lem;
	}

	constexpr double stefan_boltzman_law(double t){
        return (KSB / pid)*(t*t*t*t);
	}

	constexpr double wien_displacement_law(double t){
        return KWD * t;
	}

	constexpr double wl_from_frquency(double c,double f){
        return c/f;
	}

	constexpr double f_from_wl(double c,double wl){
        return c/wl;
	}

	constexpr double thz_to_ghz(double f){
	    return f*1e3;
	}

    struct sellmeier_coeff{
        double mB = 0.0;
        double mC = 0.0;
    };
	inline double sellmeier_law(const std::vector<sellmeier_coeff>& smc,double wl){
        wl*=10e-3;
        wl*=wl;
        double n2=1.0;
        const auto s = smc.size();
        std::vector<sellmeier_coeff>::size_type i=0;
        for(;i<s;i++){
            const auto sm = smc[i];
            n2+=((sm.mB*wl) / (wl-sm.mC));
        }
        return i ? std::sqrt( n2 ) : 1.0;
	}

	/////////////////////////
	////PATH TRACING UTILS///
	/////////////////////////

    template<typename T>
	constexpr T F0_from_ior(T ior){
	    return fsipow((ior-1.0)/(ior+1.0),2)/0.08;
	}

    template<typename T>
    constexpr vec<T,3> eval_fresnel_schlick(const vec<T,3>& ks, T dcos) {
        if(cmpf(ks,zero3<T>)) return zero3d;
        return ks +(1 - ks) *fsipow(clamp(1 - std::abs(dcos), 0.0, 1.0), 5);
    }

    template<typename T>
    constexpr vec<T,3> eval_fresnel_conductor(const vec<T,3>& I,const vec<T,3>& N,T etai,vec<T,3> etak){
       T cosi = ygl::clamp(dot(I,N),-1.0, 1.0);

       T cosi2 = cosi * cosi;
       T sini2 = 1 - cosi2;
       vec<T,3> etai2 = toVec<T,3>(etai * etai);
       vec<T,3> etak2 = etak * etak;

       vec<T,3> t0 = etai2 - etak2 - vec<T,3>{sini2,sini2,sini2};
       vec<T,3> a2plusb2 = vnx::sqrt(t0 * t0 + 4 * etai2 * etak2);
       vec<T,3> t1 = a2plusb2 + vec<T,3>{cosi2,cosi2,cosi2};
       vec<T,3> a = vnx::sqrt(0.5 * (a2plusb2 + t0));
       vec<T,3> t2 = 2 * a * cosi;
       vec<T,3> Rs = (t1 - t2) / (t1 + t2);

       vec<T,3> t3 = cosi2 * a2plusb2 + toVec<T,3>(sini2 * sini2);
       vec<T,3> t4 = t2 * sini2;
       vec<T,3> Rp = Rs * (t3 - t4) / (t3 + t4);

       return 0.5 * (Rp + Rs);
    }

    //////Hints from
    //https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
    //////
    template<typename T>
    constexpr T eval_fresnel_dielectric(const vec<T,3>& I, const vec<T,3>& N,T etai,T etat){
        T cosi = ygl::clamp(dot(I,N),-1.0, 1.0);
        if(cosi > 0.0) { std::swap(etai, etat);}

        T sint = (etai / etat) * std::sqrt(std::max(0.0, 1.0 - cosi * cosi));
        if (sint >= 1.0) {
            return 1.0;
        }
        else {
            T cost = std::sqrt(std::max(0.0, 1.0 - sint * sint));
            cosi = std::abs(cosi);
            T Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            T Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            return (Rs * Rs + Rp * Rp) / 2.0;
        }
    }

    template<typename T>
    constexpr vec<T,3> sample_sphere_direction(const vec<T,2>& ruv) {
        auto z   = 2 * ruv.y - 1;
        auto r   = std::sqrt(1 - z * z);
        auto phi = 2 * pi<T> * ruv.x;
        return {r * std::cos(phi), r * std::sin(phi), z};
    }
    template<typename T>
    constexpr vec<T,3> sample_hemisphere_direction(const vec<T,2>& ruv) {
        auto z   = ruv.y;
        auto r   = std::sqrt(1 - z * z);
        auto phi = 2 * pi<T> * ruv.x;
        return {r * std::cos(phi), r * std::sin(phi), z};
    }
    template<typename T>
    constexpr vec<T,3> sample_hemisphere_direction_cos(const vec<T,2>& ruv) {
        auto z   = std::sqrt(ruv.y);
        auto r   = std::sqrt(1 - z * z);
        auto phi = 2 * pi<T> * ruv.x;
        return {r * std::cos(phi), r * std::sin(phi), z};
    }

    template<typename T>
    constexpr vec<T,3> sample_disk_point(const vec<T,2>& ruv) {
        auto r   = std::sqrt(ruv.y);
        auto phi = 2 * pi<T> * ruv.x;
        return {std::cos(phi) * r, std::sin(phi) * r, 0};
    }

    template<typename T>
    constexpr vec<T,3> sample_diffuse_cos(const vec<T,2>& rn,const vec<T,3>& o,const vec<T,3>& n,double* pdf = nullptr){
        auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3<T>, n) : make_frame_fromz(zero3<T>, -n);
        //auto fp = make_frame_fromz(zero3<T>, n);
        auto wh_local = sample_hemisphere_direction_cos<T>(rn);
        if(pdf) *pdf = (wh_local.z <= 0) ? 0 : wh_local.z / pid;
        return ygl::transform_direction(fp, wh_local);
    }

    template<typename T>
    constexpr vec<T,3> sample_diffuse(const vec<T,2>& rn,const vec<T,3>& o,const vec<T,3>& n,double* pdf = nullptr){
        auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3<T>, n) : make_frame_fromz(zero3<T>, -n);
        //auto fp = make_frame_fromz(zero3<T>, n);
        auto wh_local = sample_hemisphere_direction<T>(rn);
        if(pdf) *pdf = (wh_local.z <= 0) ? 0 : 1.0 / (2.0 * pid);
        return ygl::transform_direction(fp, wh_local);
    }
    template<typename T>
    constexpr vec<T,3> sample_ggx(const vec<T,2>& rn,const vec<T,3>& o,const vec<T,3>& n,T rs){
        auto fp = dot(n, o) >= 0 ? make_frame_fromz(zero3<T>, n) : make_frame_fromz(zero3<T>, -n);
        //auto fp = make_frame_fromz(zero3<T>, n);
        auto tan2 = rs * rs * rn.y / (1 - rn.y);
        auto rz = std::sqrt(1 / (tan2 + 1)), rr = std::sqrt(1 - rz * rz), rphi = 2 * pi<T> * rn.x;
        auto wh_local = vec<T,3>{ rr * std::cos(rphi), rr * std::sin(rphi), rz };
        return ygl::transform_direction(fp, wh_local);
    }

    template<typename T>
    constexpr vec<T,3> dir_to(const vec<T,3>& orig,const vec<T,3>& target){
        return normalize(target-orig);
    }


    //////Hints from
    //https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
    //////
    template<typename T>
    constexpr vec<T,3> reflect(const vec<T,3> &I, const vec<T,3> &N){
        return I - 2 * dot(I,N) * N;
    }
    //////Hints from
    //https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
    //////
    template<bool sw_eta = true,typename T = double>
    constexpr vec<T,3> refract(const vec<T,3> &I, const vec<T,3> &N,T etai,T etat){
        T cosi = ygl::clamp(dot(I,N),-1.0, 1.0);
        vec<T,3> n = N;
        if (cosi < 0) { cosi = -cosi; } else { if constexpr(sw_eta){std::swap(etai, etat);} n= -N; }
        T eta = etai / etat;
        T k = 1.0 - eta * eta * (1.0 - cosi * cosi);
        return k < 0 ? zero3<T> : eta * I + (eta * cosi - std::sqrt(k)) * n;

    }

    template<typename T>
    constexpr T balance_heuristic(T p1,T p2){
        return p1 / (p1+p2);
    }

    template<typename T,int E=2>
    constexpr T power_heuristic(T p1,T p2){
        p1 = fsipow(p1,E);
        p2 = fsipow(p2,E);
        return p1 / (p1+p2);
    }

    template<typename T>
    constexpr T Mis2(T spdf,T o1pdf){
        return spdf / (spdf+o1pdf);
    }
    template<typename T>
    constexpr T Mis3(T spdf,T o1pdf,T o2pdf){
        return spdf / (spdf+o1pdf+o2pdf);
    }

    inline bool russian_roulette(VRng& rng,double& w){
        double rrp = std::max(0.05,1.0-w);
        if ( rng.next_float() < rrp) return false;
        w /= 1.0-rrp;
        return true;
    }

    inline bool russian_roulette(VRng& rng,vec3d& w){
        double rrp = std::max(0.05,1.0-max_element(w));
        if ( rng.next_float() < rrp) return false;
        w /= 1.0-rrp;
        return true;
    }

    template<typename T>
    constexpr vec<T,3> calc_beer_law(const vec<T,3>& ka,const T dst){
        if (dst > epst<T>) return exp(-ka*dst);
        return one3<T>;
    }

    struct VRay{
		vec3d o = zero3d;
		vec3d d = zero3d;
		double tmin = 0.001;
		double tmax = 1000.0;
		double ior = 1.0;

		double wl = 0.0;
		double owl = 0.0;

        inline VRay operator-() const {
          return VRay{o,-d,tmin,tmax,ior,wl,owl};
        }

        inline VRay newTo(const vec3d& dest) const{
            return VRay{o,normalize(dest-o),tmin,tmax,ior,wl,owl};
        }

        inline VRay newOffsetted(const vec3d& _o,const vec3d& _d,const vec3d& _offalong,double _dist) const{
            return VRay{
                _o + (_offalong*(2.0 * std::max(tmin, std::abs(_dist)))),
                _d,
                tmin,
                tmax,
                ior,
                wl,
                owl
            };
        }

        inline VRay newOffsettedTo(const vec3d& _o,const vec3d& _dest,const vec3d& _offalong,double _dist) const{
            return newOffsetted(_o,normalize(-_dest-_o),_offalong,_dist);
        }

	};

    constexpr bool same_hemisphere(const vec3d& n,const VRay& r1,const VRay& r2){
        return dot(n, r1.d) * dot(n, r2.d) > 0;
    }
    constexpr bool same_hemisphere(const vec3d& n,const vec3d& r1,const vec3d& r2){
        return dot(n, r1) * dot(n, r2) > 0;
    }

    template<typename T>
    constexpr vec<T,3> normal_forward(const vec<T,3>& n,const vec<T,3>& w){
        return dot(n,w)< 0.0 ? -n : n;
    }

	inline VRay offsetted_ray(const vec3d& o,VRay rr, const vec3d& d, const double tmin, const double tmax, const vec3d& offalong, const double dist, double fmult = 2.0) {
		rr.o = o + (offalong*(fmult * std::max(tmin, std::abs(dist))));
		rr.d = d;
		rr.tmin = tmin;
		rr.tmax = tmax;
		return rr;
	}

    template<typename T>
    constexpr void update_ray_physics(VRay& ray,T ior){
        ray.ior = ior;
        ray.wl = ray.owl / ior;
    }

    template<typename T>
    constexpr void init_ray_physics(VRng& rng,VRay& ray,T minwl, T maxwl){
        ray.wl = rng.next_double(minwl,maxwl);//rand1f_r(rng,minwl,maxwl);
        ray.owl = ray.wl;
        ray.ior = 1.0;
    }


    inline double GTerm(const vec3d& v1,const vec3d& n1,const vec3d& v2,const vec3d& n2){
        const auto v1_to_v2 = normalize(v2-v1);
        double g1;
        if(!cmpf(n1,zero3d))g1 = adot(n1,v1_to_v2);
        else g1 = 1.0;
        double g2;
        if(!cmpf(n2,zero3d))g2 = adot(n2,-v1_to_v2);
        else g2 = 1.0;
        return (g1*g2)/ygl::distance_squared(v1,v2);
    }

    inline double PdfAtoW(const double& pdfA,const double& dist,const double& cosTheta){
        return (pdfA * (dist*dist)) / std::abs(cosTheta);
    }
    inline double PdfWtoA(const double& pdfW,const double& dist,const double& cosTheta){
        return (pdfW *  std::abs(cosTheta)) / (dist*dist);
    }

    inline double BdptMisWi(double vc,double vcm,double pdf){
        return pdf*(vc+vcm);
    }

    ///////////////////////////
    /////BRDF SAMPLING HELPERS
    ///////////////////////////

    struct VMaterial {
	    VMaterial(){}
	    VMaterial(VMaterialType _type,
               VIorType _ior_type,
               vec3d _kr,vec3d _sigma_a,double _sigma_s,double _sigma_u,
               double _e_temp,double _e_power,
               double _ior,
               double _rs,
               const std::vector<sellmeier_coeff>& _smc
               ){
                type = _type;
                ior_type=_ior_type;
                kr = _kr;
                sigma_a = _sigma_a;
                sigma_s = _sigma_s;
                sigma_u = _sigma_u;
                e_temp = _e_temp;
                e_power = _e_power;
                ior = _ior;
                rs = _rs;
                for(std::vector<sellmeier_coeff>::size_type i=0;i<_smc.size();i++){
                    smc.push_back(_smc[i]);
                }
            }
		mtlm_ftor mutator = nullptr;

        VMaterialType type = diffuse;
        VIorType ior_type = wl_dependant;

		vec3d kr = zero3d;


		vec3d sigma_a = zero3d;
		double sigma_s = -1.0;
		double sigma_u = 0.0;

		double e_temp = 0.0;
		double e_power = 0.0;

		double ior = 1.0;
		double rs = 0.0;

		std::vector<sellmeier_coeff> smc;

		inline void Relate(VMappedEntry* entry){
            entry->ptr = this;
            type = try_strToMaterialType(entry->try_get("mat_type"),type);
            ior_type = try_strToIorType(entry->try_get("ior_type"),ior_type);
            kr = try_strToVec_d(entry->try_get("kr"),kr);

            sigma_a = try_strToVec_d(entry->try_get("sigma_a"),sigma_a);
            sigma_s = try_strtof(entry->try_get("sigma_s"),sigma_s);
            sigma_u = try_strtof(entry->try_get("sigma_u"),sigma_u);

            e_temp = try_strtof(entry->try_get("e_temp"),e_temp);
            e_power = try_strtof(entry->try_get("e_power"),e_power);

            ior = try_strtof(entry->try_get("ior"),ior);
            rs = try_strtof(entry->try_get("rs"),rs);


            for(auto i=1;;i++){
                auto sm_B = try_strtof(entry->try_get("s_b"+std::to_string(i)),-1.0);
                if(sm_B<0.0) break;
                auto sm_C = try_strtof(entry->try_get("s_c"+std::to_string(i)),-1.0);
                if(sm_C<0.0) sm_C = 0.0;
                smc.push_back({sm_B,sm_C});
            }
		}

        inline double sigma_t() const{return max(sigma_a)+sigma_s;}
        inline vec3d sigma_t_vec() const{return sigma_a+sigma_s+sigma_u;}
		inline double sigma_t_maj() const{return max(sigma_a)+sigma_s+sigma_u;}

        inline bool is_diffuse() const {return type==diffuse;}
		inline bool is_dielectric() const {return type==dielectric;}
		inline bool is_conductor()const {return type==conductor;}

		inline bool is_emissive()  const { return e_temp > thrd && e_power > thrd; }
		inline bool is_transmissive() const { return is_dielectric() && sigma_s>=-thrd;}
		inline bool is_refractive() const { return is_transmissive() && (is_dispersive() || ior>1.0);}
		inline bool is_dispersive() const{
            for(std::vector<sellmeier_coeff>::size_type i=0;i<smc.size();i++){
                if(smc[i].mB>thrd) return true;
            }
            return false;
		}

		inline bool is_delta() const{return rs<=thrd && (is_conductor() || is_transmissive());}

		inline bool has_kr() const{return !cmpf(kr,zero3d);}

		inline double eval_ior(double wl,double min_wl,double max_wl) const{
		    if(ior_type==non_wl_dependant || !is_dispersive()) return ior;
            if(ior_type==wl_dependant && is_dispersive()) return sellmeier_law(smc,wl);
            if(is_dispersive()){
                auto ior_min = sellmeier_law(smc,min_wl);
                auto ior_max = sellmeier_law(smc,max_wl);
                return (ior_min+ior_max) / 2.0;
            }
            return ior;
		}

		inline void eval_mutator(const VResult& hit,const vec3d& n, VMaterial& mat) {
			if (mutator != nullptr){
                mutator(hit, n, mat);
			}
		}
	};

    template<typename T>
    inline vec<T,3> brdf_diffuse_ashikhmin(const VMaterial& m,const T Rs,const VRay& wi,const VRay& wo,T ndi,T ndo){
        auto rfc = ((28.0)/(23.0*pi<T>))*m.kr*(1.0-Rs);
        auto rfc2 = (1.0-fsipow(1.0-(std::abs(ndi)/2.0),5))*(1.0-fsipow(1.0-(std::abs(ndo)/2.0),5));
        return rfc*rfc2;
    }
    template<typename T>
    inline T brdf_cook_torrance_G(T ndi,T ndo,T ndh,T odh){
        return std::min(1.0,std::min( (2.0*std::abs(ndh)*std::abs(ndo))/std::abs(odh), (2.0*std::abs(ndh)*std::abs(ndi))/std::abs(odh)));
    }
    template<typename T>
    inline T brdf_cook_torrance_D(T rs,T ndi,T ndo,T ndh,T odh){
        auto a2 = rs*rs;
        a2 = std::max(a2,thrt<T>);
        auto ndh2 = ndh*ndh;
        auto andh4 = ndh2*ndh2;
        return (1.0/(pi<T>*a2*andh4))*std::exp((ndh2-1.0)/(a2*ndh2));
    }
    template<typename T>
    inline T brdf_cook_torrance_DG(T rs,T ndi,T ndo,T ndh,T odh){
        auto D = brdf_cook_torrance_G(ndi,ndo,ndh,odh);
        auto G = brdf_cook_torrance_D(rs,ndi,ndo,ndh,odh);
        return D*G;
    }
    template<typename T>
    inline T brdf_ggx_G(T rs,T ndi,T ndo){
        auto a2 = rs * rs;
        a2 = std::max(a2,thrt<T>());
        auto goDN = std::abs(ndo) + std::sqrt(a2 + (1 - a2) * ndo * ndo);
        if(goDN<=0.0) return 0.0;
        auto giDN = std::abs(ndi) + std::sqrt(a2 + (1 - a2) * ndi * ndi);
        if(giDN<=0.0) return 0.0;

        auto Go = (2 * std::abs(ndo)) / goDN;
        auto Gi = (2 * std::abs(ndi)) / giDN;
        return Go*Gi;
    }
    template<typename T>
    inline T brdf_ggx_D(T rs,T ndh){
        auto ndh2 = ndh*ndh;
        auto a2 = rs*rs;
        a2 = std::max(a2,thrt<T>());
        auto dn = pid*fsipow((a2-1)*ndh2+1,2);
        if(dn<=0.0) return 0.0;
        return a2/dn;
    }
    template<typename T>
    inline T brdf_ggx_DG(T rs, T ndi, T ndo, T ndh) {
        return brdf_ggx_D(rs,ndh)*brdf_ggx_G(rs,ndi,ndo);
    }
    template<typename T>
    inline T brdf_ggx_pdf(T rs,T ndh){
        return brdf_ggx_D(rs,ndh)*std::abs(ndh);
    }

	struct VResult {
		VSdf* sur = nullptr;
		VSdf* vsur = nullptr;

		VMaterial* mtl = nullptr;
		VMaterial* vmtl = nullptr;

		double dist = maxd;
		double vdist = maxd;

		vec3d wor_pos = zero3d;
		vec3d loc_pos = zero3d;

		bool _found = false;

		inline bool isValid() const{return (sur && mtl);}
		inline bool isFound() const{ return _found; }
		inline bool isInside() const{return vdist<0.0;}
		inline bool isOutside() const{return !isInside();}

		inline bool hasMaterial() const{return mtl!=nullptr;}
		inline bool hasVMaterial() const{return vmtl!=nullptr;}

		inline void getMaterial(VMaterial& _mtl){_mtl = hasMaterial() ? (*mtl) : VMaterial{};}
		inline void getVMaterial(VMaterial& _vmtl){_vmtl = hasVMaterial() ? (*vmtl) : VMaterial{};}

	};

    //TODO
    struct VIntersectAlgo{
        VIntersectAlgo(){};
        virtual std::string Type() = 0;
        inline virtual VResult operator()(const VScene& scn,const VRay& ray,unsigned int nmi,unsigned int* iters=nullptr,unsigned int* overs=nullptr)=0;
    };
    struct VNormalsAlgo{
        VNormalsAlgo(){};
        virtual std::string Type() = 0;
        VResult operator()(const VScene& scn,const VResult& er,double f_normal_eps){return this->operator()(scn,er.wor_pos,f_normal_eps);}
        virtual VResult operator()(const VScene& scn,const vec3d& ep,double f_normal_eps) = 0;
    };
    struct VEmissivesPrecalcAlgo{
        VEmissivesPrecalcAlgo(){};
        virtual std::string Type() = 0;
        //TODO
    };

	struct VNode {
		std::string mID = "";
		frame3d mFrame = identity_frame3d;
		vec3d mTranslation = zero3d;
		vec4d mRotation = identity_quat4d;
		vec3d mScale = one3d;
		VRotationOrder mRotationOrder = VRO_XYZ;
		double mRounding = 0.0;
		displ_ftor mDisplacement = nullptr;


		VNode(std::string idv): mID(idv) ,
		mFrame(identity_frame3d),
		mTranslation(zero3d),
		mRotation(identity_quat4d),
		mScale(one3d),
		mRotationOrder(VRO_XYZ),
		mRounding(0.0),
		mDisplacement(nullptr){}

		virtual ~VNode() {};

		void Relate(VMappedEntry* entry){entry->ptr = this;DoRelate(entry);}
		virtual void DoRelate(const VMappedEntry* entry){
            set_translation(try_strToVec_d(entry->try_get("pos"),mTranslation));
            set_rotation_degs(try_strToVec_d(entry->try_get("rot"),vec3d{mRotation.x,mRotation.y,mRotation.z}));
            set_scale(try_strToVec_d(entry->try_get("sca"),mScale));
            set_rotation_order(try_strToRotationOrder(entry->try_get("rot_order"),mRotationOrder));
            set_rounding(try_strtod(entry->try_get("rounding"),mRounding));
		};

		virtual std::vector<VNode*> get_childs() = 0;
		virtual VNode* add_child(VNode* child) = 0;
		virtual void add_childs(std::vector<VNode*> chs) = 0;

		inline virtual const char* Type() = 0;
		inline virtual bool isOperator(){return false;}

		inline double eval_displacement(const vec3d& p){
		    if(mDisplacement) return mDisplacement(p);
		    return 0.0;
		}

		inline vec3d eval_ep(const vec3d& p){
		    return transform_point_inverse(mFrame, p);// / scale;
		}

		inline void set_translation(const vec3d& a) { mTranslation = a; }
		inline void set_translation(double x, double y, double z) { mTranslation = { x,y,z }; }

		inline void set_rounding(double r) { mRounding = r; }

		inline void set_rotation(const vec3d& a) { mRotation = { a.x,a.y,a.z,1 }; }
		inline void set_rotation(double x, double y, double z) { mRotation = { x,y,z,1 }; }
		inline void set_rotation_degs(const vec3d& a) { mRotation = { vnx::radians(a.x),vnx::radians(a.y),vnx::radians(a.z),1 }; }
		inline void set_rotation_degs(double x, double y, double z) { mRotation = { vnx::radians(x),vnx::radians(y),vnx::radians(z),1 }; }
		inline void mod_rotation_degs(const vec3d& a) { mRotation.x += vnx::radians(a.x), mRotation.y += vnx::radians(a.y); mRotation.z += vnx::radians(a.z); }
		inline void mod_rotation_degs(double x, double y, double z) { mRotation.x += vnx::radians(x), mRotation.y += vnx::radians(y); mRotation.z += vnx::radians(z); }

		inline void set_rotation_order(VRotationOrder ro){mRotationOrder = ro;}

        inline void set_scale(double s){ if (s <= thrd) { mScale = one3d; return; }  mScale = {s,s,s}; }
		inline void set_scale(const vec3d& a) { if (min_element(a) <= thrd) { mScale = one3d; return; }  mScale = a; }
		inline void set_scale(double x, double y, double z) { if (min_element(vec3d{ x,y,z }) <= thrd) { mScale = one3d; return; } mScale = { x,y,z }; }

		inline void set_displacement(displ_ftor ftor){mDisplacement = ftor;}

		VNode* select(const std::string& n) {
			auto chs = get_childs();
			if (chs.empty()) { return nullptr; }
			for (auto& child : chs) {
				if (child->mID == n) { return child; }
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
				if (child->mID == n) { results.push_back(child); }
				auto res = child->select_all(n);
				if (!res.empty()) {
					for (auto chr : res) {
						results.push_back(chr);
					}
				}
			}
			return results;
		}

		virtual void eval(const vec3d& p,VResult& res) = 0;
	};

	struct VSdf : public VNode {
		VMaterial* mMaterial = nullptr;

		~VSdf() {};

		VSdf(const std::string& idv):VNode(idv),mMaterial(nullptr){}
		VSdf(const std::string& idv,VMaterial* mtl) :VNode(idv),mMaterial(mtl){}

		std::vector<VNode*> get_childs() {
			auto child = std::vector<VNode*>();
			child.resize(0);
			return child;
		}
		VNode* add_child(VNode* child) { return nullptr; };
		void add_childs(std::vector<VNode*> chs) {};
	};

	struct VSdfOperator : public VNode {
		std::vector<VNode*> mChilds;

		~VSdfOperator() {
			auto chs = get_childs();
			if (chs.empty()) { return; }
			for (auto child : chs) {
				if(child)delete child;
			}
		}
		VSdfOperator(const std::string& idv) :VNode(idv){
			mChilds = std::vector<VNode*>();
			mChilds.resize(0);
		}
		VSdfOperator(const std::string& idv,std::vector<VNode*> chs) :VNode(idv){
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

		inline bool isOperator(){return true;}
	};

	template<typename T>
	inline vec<T,3> AcesFilmic(vec<T,3> hdr,bool ltosrgb = true,bool fitOriginal=false){
        // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
        if(fitOriginal) hdr*=0.6;
        auto ldr = (hdr*(2.51*hdr+0.03))/((hdr*(2.43*hdr+0.59))+0.14);
        if(ltosrgb) return vnx::linear_to_gamma(ldr);
        return ldr;
	}

	inline bool save_ppm(const std::string& fname, const VImg<double>& img,bool apply_tonemap,int depth = 255) {
		const int x = img.mResolution.x, y = img.mResolution.y;
		std::ofstream fp;
		fp.open((fname + ".ppm").c_str(),std::ios::binary);
        if(!fp.is_open()) return false;
		fp<<"P6 "<<x<<" "<<y<<" "<<std::to_string(depth)<<"\n";

		for (int j = 0; j < y; ++j){
			for (int i = 0; i < x; ++i){
                const vec3d &pixel = apply_tonemap ? AcesFilmic(img.at(i,j)) : img.at(i,j);
                if (depth<256){

                    auto r = static_cast<byte>(clamp(static_cast<int>(std::round(pixel.x * (depth+1))), 0, depth));
                    auto g = static_cast<byte>(clamp(static_cast<int>(std::round(pixel.y * (depth+1))), 0, depth));
                    auto b = static_cast<byte>(clamp(static_cast<int>(std::round(pixel.z * (depth+1))), 0, depth));
                    fp<<r<<g<<b;
                }else{
                    auto r = static_cast<unsigned int>(clamp(static_cast<int>(std::round(pixel.x * (depth+1))), 0, depth));
                    auto g = static_cast<unsigned int>(clamp(static_cast<int>(std::round(pixel.y * (depth+1))), 0, depth));
                    auto b = static_cast<unsigned int>(clamp(static_cast<int>(std::round(pixel.z * (depth+1))), 0, depth));

                    auto rb1 = static_cast<byte>((r>>8) & 0xff);
                    auto rb2 = static_cast<byte>(r & 0xff);
                    auto gb1 = static_cast<byte>((g>>8) & 0xff);
                    auto gb2 = static_cast<byte>(g & 0xff);
                    auto bb1 = static_cast<byte>((b>>8) & 0xff);
                    auto bb2 = static_cast<byte>(b & 0xff);
                    fp<<rb1<<rb2<<gb1<<gb2<<bb1<<bb2;
                }
			}
		}

		fp.close();
		return true;
	}

	inline bool save_hdr(const std::string& fname, const VImg<double>& img,bool apply_tonemap) {
		const int x = img.mResolution.x, y = img.mResolution.y;
		std::ofstream fp;
		fp.open((fname + ".hdr").c_str(),std::ios::binary);
        if(!fp.is_open()) return false;
        fp <<"#?RADIANCE"<<'\n';
        fp <<"#VAureaNox"<<'\n';
        fp <<"FORMAT=32-bit_rle_rgbe"<<"\n\n";
        fp <<"-Y "<<y<<" +X "<<x<<'\n';

		for (int j = 0; j < y; ++j){
			for (int i = 0; i < x; ++i){
                const vec3d &rgb = apply_tonemap ? AcesFilmic(img.at(i,j)) : img.at(i,j);
                float mx_l = float(max_element(rgb));
                if(mx_l>mint<float>()){
                    int e;
                    mx_l = float(frexp(mx_l,&e)*256.0f/mx_l);
                    fp<<static_cast<byte>(static_cast<float>(rgb.x)*mx_l);
                    fp<<static_cast<byte>(static_cast<float>(rgb.y)*mx_l);
                    fp<<static_cast<byte>(static_cast<float>(rgb.z)*mx_l);
                    fp<<static_cast<byte>(e+128);
                }
			}
		}

		fp.close();
		return true;
	}

	inline bool save_image(std::string fname, const VImg<double>& img,const std::string& ext,bool apply_tonemap = true) {
        if(stricmp(ext,std::string("hdr"))){
            return save_hdr(fname,img,apply_tonemap);
        }else{
            return save_ppm(fname,img,apply_tonemap);
        }
	}


	frame3d calculate_frame(const VNode* node,const frame3d& parent);
	void apply_transforms(VNode* node, const frame3d& parent);

	///Procedural patterns utilities
    inline bool on_pattern_gradient_oblique(const vec3d& loc_pos,double angle,float sx){
        angle = radians(angle);
        const double cangle = std::cos(angle);
        const double sangle = std::sin(angle);

        double s = loc_pos.x * cangle - loc_pos.y * sangle;
        if((modulo(s * sx) < 0.5)){
            return true;
        }
        return false;
    }

    inline bool on_pattern_gradient_checker(const vec3d& loc_pos,double angle,const vec2f& sc){
        angle = radians(angle);
        const double cangle = std::cos(angle);
        const double sangle = std::sin(angle);

        double s = loc_pos.x * cangle - loc_pos.y * sangle;
        double t = loc_pos.y * cangle + loc_pos.x * sangle;
        if((modulo(s * sc.x) < 0.5) || (modulo(t * sc.y) < 0.5)){
            return true;
        }
        return false;
    }

    inline VMaterial get_material_archetype(const std::string& k){
            if(stricmp(k,std::string("bk7"))) return VMaterial(dielectric,wl_dependant,zero3d,zero3d,0.0,0.0,0.0,0.0,1.5168,0.0,{{1.03961212,0.00600069867},{0.231792344,0.0200179144},{1.01046945,103.560653}});
            else if(stricmp(k,std::string("baf10"))) return VMaterial(dielectric,wl_dependant,zero3d,zero3d,0.0,0.0,0.0,0.0,1.6700,0.0,{{1.5851495,0.00926681282},{0.143559385,0.0424489805},{1.08521269,105.613573}});
            else if(stricmp(k,std::string("bak1"))) return VMaterial(dielectric,wl_dependant,zero3d,zero3d,0.0,0.0,0.0,0.0,1.5725,0.0,{{1.12365662,0.00644742752},{0.309276848,0.0222284402},{0.881511957,107.297751}});
            else if(stricmp(k,std::string("lasf9"))) return VMaterial(dielectric,wl_dependant,zero3d,zero3d,0.0,0.0,0.0,0.0,1.8502,0.0,{{2.00029547,0.0121426017},{0.298926886,0.0538736236},{1.80691843,156.530829}});
            else if(stricmp(k,std::string("fused_silica"))) return VMaterial(dielectric,wl_dependant,zero3d,zero3d,0.0,0.0,0.0,0.0,1.4585,0.0,{{0.6961663,fsipow(0.0684043,2)},{0.4079426,fsipow(0.1162414,2)},{0.8974794,fsipow(9.896161,2)}});
            else if(stricmp(k,std::string("carbon_diamond"))) return VMaterial(dielectric,wl_dependant,zero3d,zero3d,0.0,0.0,0.0,0.0,2.4175,0.0,{{0.3306,fsipow(0.1750,2)},{4.3356,fsipow(0.1060,2)}});
            else if(stricmp(k,std::string("water"))) return VMaterial(dielectric,wl_dependant,zero3d,{0.1,0.1,0.01},0.0,0.0,0.0,0.0,1.3218,0.0,{{0.75831,0.01007},{0.08495,8.91377}});
            else if(stricmp(k,std::string("dispersive_plastic"))) return VMaterial(dielectric,wl_dependant,{0.8,0.8,0.8},zero3d,-1.0,0.0,0.0,0.0,1.5168,0.01,{{1.03961212,0.00600069867},{0.231792344,0.0200179144},{1.01046945,103.560653}});
            return VMaterial();
    }

};


#endif
