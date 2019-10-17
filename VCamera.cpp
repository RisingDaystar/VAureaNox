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

#include "VCamera.hpp"
#include "VScene.hpp"

namespace vnx{

    void VCamera::Relate(const VMappedEntry* entry){
        mYfov = radians(try_strtof(entry->try_get("yfov"),mYfov));
        mOrigin = try_strToVec_d(entry->try_get("origin"),mOrigin);
        mTarget = try_strToVec_d(entry->try_get("target"),mTarget);
        mUp = try_strToVec_d(entry->try_get("up"),mUp);
        mFocus = try_strtod(entry->try_get("focus"),mFocus);
        mAperture = try_strtod(entry->try_get("aperture"),mAperture);
        mAutoFocus = try_strtob(entry->try_get("autofocus"),mAutoFocus);
    }

    void VCamera::Setup(const vec2f& resolution){
        mResolution = resolution;
        mAspect = double(mResolution.x) / double(mResolution.y);
        mNumPixels = mResolution.x*mResolution.y;

        mClipToRaster =
            scaling_mat(vec3d{mResolution.x*0.5,mResolution.y*0.5,1.0})*
            translation_mat(vec3d{1.0,1.0,0.0});
        mRasterToClip = inverse(mClipToRaster);

        mCameraToClip = ygl::perspective_mat(mYfov,mAspect,0.1);
        mCameraToRaster = mClipToRaster*mCameraToClip;
        mCameraToWorld = glLookAtMat(mOrigin,mTarget,mUp,true);//frame_to_mat(lookat_frame(mOrigin,mTarget,mUp,true));

        mWorldToCamera = inverse(mCameraToWorld);
        mWorldToRaster = mCameraToRaster*mWorldToCamera;

        //Inverse
        mRasterToCamera = inverse(mCameraToRaster);
        mRasterToWorld = inverse(mWorldToRaster);


        mForward = normalize(mTarget-mOrigin);

        mImagePlaneDist =(mResolution.y*0.5) / std::tan(mYfov*0.5);
    }

    void VCamera::EvalAutoFocus(const VScene& scn,double tmin,double tmax,int nm){
        if(mAutoFocus){
            VRay focus_ray = {mOrigin,mForward,tmin,tmax};
            auto hit = scn.intersect(focus_ray,nm,nullptr,nullptr);
            if(hit.isFound()) mFocus = length(focus_ray.o-hit.wor_pos);
            else mFocus = focus_ray.tmax;
            std::cout<<"**Autofocus --> Focal Length : "<<mFocus<<" | hit at {"<<hit.wor_pos<<"}\n";
        }else std::cout<<"**Focal Length : "<<mFocus<<"\n";
    }



};

