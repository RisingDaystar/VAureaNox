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

#ifndef VCAMERA_HPP_INCLUDED
#define VCAMERA_HPP_INCLUDED

#include "VAureaNox.hpp"

namespace vnx{
	struct VCamera{

	    struct VFrameBuffer{
	        vec2f mResolution;
            int mScale = 1;

            std::vector<vec3d> mPixels;

            void HintScale(int sc){if(sc>mScale) mScale = sc;}

            void add(const vec2i& pid,const vec3d& c,int sc){
                if(!(pid.x>=0&&pid.x<int(mResolution.x) && pid.y>=0 && pid.y<int(mResolution.y))) return;
                auto px = &mPixels[(pid.y*int(mResolution.x))+pid.x];
                *px += c;
                HintScale(sc);
            }

            void Create(const vec2f& res){
                mPixels.resize(res.x*res.y);
                mResolution = res;
            }

            void MergeToImg(image3d& img) const{
                if(img.size.x!=int(mResolution.x) || img.size.y!=int(mResolution.y)) return;
                for(auto i=0;i<mResolution.x*mResolution.y;i++){
                    img.pixels[i] += mPixels[i] / float(mScale);
                }
            }

	    }mFrameBuffer;

        double mFocus = 1.0;
        double mAperture = 0.0;
        double mYfov = 45.0;

        bool mAutoFocus = true;

        double mImagePlaneDist = 0.0;

        vec3d mOrigin = zero3d;
        vec3d mTarget = zero3d;
        vec3d mUp = {0,1,0};

        vec3d mForward = zero3d;
        unsigned int mNumPixels = 0;

        vec2f mResolution;
        double mAspect;

        mat4d mWorldToCamera = identity_mat4d;
        mat4d mWorldToRaster = identity_mat4d;
        mat4d mCameraToRaster = identity_mat4d;
        mat4d mCameraToClip = identity_mat4d;
        mat4d mClipToRaster = identity_mat4d;


        mat4d mCameraToWorld = identity_mat4d;
        mat4d mRasterToWorld = identity_mat4d;
        mat4d mRasterToCamera = identity_mat4d;

        void Relate(const VEntry* entry);

        void Setup(const vec2f& resolution);
        void EvalAutoFocus(const VScene& scn,double tmin,double tmax,int nm);

        inline bool RasterInBounds(const vec3d& raster,vec2i& pid) const {
            pid = vec2i{int(std::round(raster.x)),int(std::round(raster.y))};
            return (pid.x>=0&&pid.x<int(mResolution.x) && pid.y>=0 && pid.y<int(mResolution.y));
        }

        inline vec3d WorldToRaster(const vec3d& world_point) const{
            return transform_point(mWorldToRaster,world_point);
        }

        inline bool WorldToPixel(const vec3d& world_point,vec2i& pid) const{
            auto rp = WorldToRaster(world_point);
            return RasterInBounds(rp,pid);
        }

        inline VRay RayCast(double x,double y,ygl::rng_state& rng,const vec2f& uv) const{
            vec3d raster_point = {x+uv.x,y+uv.y,0.0};
            vec3d camera_point = transform_point(mRasterToCamera,raster_point);

            VRay rr;
            rr.o = zero3d;
            rr.d = normalize(camera_point);

            //DOF
            if(mAperture>thrd){
                double ft = mFocus / normalize(mOrigin).z;
                auto focal_point = rr.o+rr.d*ft;
                auto lens_point = ygl::sample_disk_point(get_random_vec2f(rng))*mAperture;
                rr.o = vec3d{lens_point.x,lens_point.y,0.0};
                rr.d = normalize(focal_point-rr.o);
            }

            rr.o = transform_point(mCameraToWorld, rr.o);
            rr.d = transform_vector(mCameraToWorld, rr.d);

            return rr;
        }
	};
};
#endif // VCAMERA_HPP_INCLUDED
