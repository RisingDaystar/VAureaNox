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


#ifndef _VVOLUMES_H
#define _VVOLUMES_H

#include "VAureaNox.h"

using namespace vnx;

//Personalizzabile

struct vvo_shadered : public VVolume {
	std::function<float(const vec3vf& p, vec3vf& ep, const VNode* tref) > mShader = nullptr;
	vvo_shadered(std::string ids, VMaterial* mtl, std::function<float(const vec3vf& p, vec3vf& ep, const VNode* tref) > ftor): VVolume(ids,mtl),mShader(ftor) { }

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_shadered";}

	inline void eval(const vec3vf& p,VResult& res) {
		vec3vf ep = eval_ep(p);


		float dv = maxvf;
		if (mShader != nullptr) { dv = mShader(p, ep, this); }
		dv+=eval_displacement(ep);
		//dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

///

struct vvo_sd_plane : public VVolume {
    const vec3vf mDispo = {0,1,0};
    vfloat mOffset = 1.0;
	vvo_sd_plane(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mOffset(1.0){}
	vvo_sd_plane(std::string ids, VMaterial* mtl,vfloat offset): VVolume(ids,mtl),mOffset(offset){}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_plane";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vfloat dv = ygl::dot(ep,mDispo) + (1.0 / mOffset);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_sphere : public VVolume {
	vfloat mRadius = 1.0;
	vvo_sd_sphere(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mRadius(1.0) {}
	vvo_sd_sphere(std::string ids, VMaterial* mtl, vfloat radius): VVolume(ids,mtl),mRadius(radius) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
        mRadius = try_strtovf(entry->try_at(6),mRadius);
	}

	inline const char* type(){return "vvo_sd_sphere";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vfloat dv = ygl::length(ep) - mRadius;
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_box : public VVolume {
	vec3vf mDims = one3vf;
	vvo_sd_box(std::string ids, VMaterial* mtl): VVolume(ids,mtl) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, vec3vf dims): VVolume(ids,mtl),mDims(dims) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, vfloat dims): VVolume(ids,mtl),mDims(vec3vf{dims,dims,dims}) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_box";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

        vec3vf d = abs(ep) - mDims;
        vfloat dv = length(max(d,zero3vf)) + std::min(std::max(d.x,std::max(d.y,d.z)),0.0);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_ellipsoid : public VVolume {
	vec3vf mSizes = one3vf;
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mSizes(one3vf) {}
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl, vec3vf sizes): VVolume(ids,mtl),mSizes(sizes) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_ellipsoid";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vfloat dv = (length(ep / mSizes) - 1.0) * std::min(std::min(mSizes.x, mSizes.y), mSizes.z);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_cylinder : public VVolume {
	vec2vf mDims = one2vf;
	vvo_sd_cylinder(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one2vf) {}
	vvo_sd_cylinder(std::string ids, VMaterial* mtl, vec2vf dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_cylinder";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vec2vf d = abs(vec2vf{length(vec2vf{ ep.x,ep.z }), ep.y}) - mDims;
		vfloat dv = std::min(std::max(d.x, d.y), 0.0) + length(max(d, zero2vf));
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capsule : public VVolume {
	vfloat mBaseRadius = 1.0;
	vec3vf mA = one3vf;
	vec3vf mB = one3vf;
	vvo_sd_capsule(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mBaseRadius(1.0),mA(one3vf),mB(one3vf) {}
	vvo_sd_capsule(std::string ids, VMaterial* mtl, vfloat baseRadius, vec3vf a, vec3vf b): VVolume(ids,mtl),mBaseRadius(baseRadius),mA(a),mB(b) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_capsule";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vec3vf pa = ep - mA, ba = mB - mA;
		vfloat h = ygl::clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
		vfloat dv = length(pa - ba*h) - mBaseRadius;
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_hex_prism : public VVolume {
	vec2vf mDims = one2vf;
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one2vf) {}
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl, vec2vf dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_hex_prism";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vec3vf q = abs(ep);
		auto dv = std::max(q.z - mDims.y, std::max((q.x*0.866025 + q.y*0.5), q.y) - mDims.x);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_tri_prism : public VVolume {
	vec2vf mDims = one2vf;
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one2vf) {}
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl, vec2vf dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_tri_prism";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vec3vf q = abs(ep);
		auto dv = std::max(q.z - mDims.y, std::max(q.x*0.866025 + ep.y*0.5, -ep.y) - mDims.x*0.5);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capped_cone : public VVolume {
	vec3vf mDims = one3vf;
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one3vf) { }
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl, vec3vf dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_capped_cone";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vec2vf q = vec2vf{ygl::length(vec2vf{ ep.x,ep.z }), ep.y};
		vec2vf v = vec2vf{mDims.z*mDims.y / mDims.x, -mDims.z};
		vec2vf w = v - q;
		vec2vf vv = vec2vf{dot(v, v), v.x*v.x};
		vec2vf qv = vec2vf{dot(v, w), v.x*w.x};
		vec2vf d = max(qv, zero2vf)*qv / vv;
		auto dv = sqrt(dot(w, w) - std::max(d.x, d.y)) * sign(std::max(q.y*v.x - q.x*v.y, w.y));
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};


struct vvo_sd_pyramid4 : public VVolume {
    vec3vf mDims = one3vf;
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one3vf) {}
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl, vec3vf dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_pyramid4";}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

        vec3vf d = abs(ep - vec3vf{0,-2.0*mDims.z,0}) - vec3vf{2.0*mDims.z,2.0*mDims.z,2.0*mDims.z};
		auto bdv = std::min(vnx::max_element<vfloat>(d), 0.0) + ygl::length(max(d, zero3vf));

        vfloat dv = 0.0;
        dv = std::max( dv, std::abs( dot(ep, vec3vf{ -mDims.x, mDims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3vf{  mDims.x, mDims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3vf{  0, mDims.y, mDims.x }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3vf{  0, mDims.y,-mDims.x }) ));
        vfloat octa = dv - mDims.z;
        dv = std::max(-bdv,octa);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}

};

struct vvo_sd_diamond : public VVolume {

	vvo_sd_diamond(std::string ids, VMaterial* mtl): VVolume(ids,mtl) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_vf(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec_vf(entry->try_at(3),vec3vf{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_vf(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_diamond";}

	inline mat3vf rot_y(vfloat a) {
		vfloat c = cos(a);
		vfloat s = sin(a);
		return mat3vf{
		{ c, 0.0, s },
		{ 0.0, 1.0, 0.0 },
		{ -s, 0.0, c }
		};
	}

	inline void eval(const vec3vf& p,VResult& res) {
        vec3vf ep = eval_ep(p);

		vfloat dv = minvf;
		vec3vf point_0 = rot_y(pivf / 4.0)*ep;
		vec3vf point_1 = rot_y(pivf / 8.0 + pivf / 16.0)*ep;
		vec3vf point_2 = rot_y(pivf / 8.0 - pivf / 16.0)*ep;
		vec3vf point_3 = rot_y(-pivf / 8.0 + pivf / 16.0)*ep;
		vec3vf point_4 = rot_y(-pivf / 8.0 - pivf / 16.0)*ep;
		vec3vf temp;
		point_1 = vec3vf{std::abs(point_1.x), -point_1.y, std::abs(point_1.z)};
		point_2 = vec3vf{std::abs(point_2.x), -point_2.y, std::abs(point_2.z)};
		point_3 = vec3vf{std::abs(point_3.x), -point_3.y, std::abs(point_3.z)};
		point_4 = vec3vf{std::abs(point_4.x), -point_4.y, std::abs(point_4.z)};
		temp = normalize(vec3vf{1.0, 1.5, 1.0});
		dv = std::max(dot(abs(ep), temp) - 0.5*1.5, dv);
		dv = std::max(dot(abs(point_0), temp) - 0.5*1.5, dv);
		dv = std::max(ep.y - 0.5, dv);
		temp = normalize(vec3vf{1.0, 1.4, 1.0});
		dv = std::max(dot(point_1, temp) - 0.762f, dv);
		dv = std::max(dot(point_2, temp) - 0.762f, dv);
		dv = std::max(dot(point_3, temp) - 0.762f, dv);
		dv = std::max(dot(point_4, temp) - 0.762f, dv);
		point_1 = vec3vf{point_1.x, -point_1.y, point_1.z};
		point_2 = vec3vf{point_2.x, -point_2.y, point_2.z};
		point_3 = vec3vf{point_3.x, -point_3.y, point_3.z};
		point_4 = vec3vf{point_4.x, -point_4.y, point_4.z};
		temp = normalize(vec3vf{1.0, 1.25, 1.0});
		dv = std::max(dot(point_1, temp) - 0.804, dv);
		dv = std::max(dot(point_2, temp) - 0.804, dv);
		dv = std::max(dot(point_3, temp) - 0.804, dv);
		dv = std::max(dot(point_4, temp) - 0.804, dv);
		point_1 = rot_y(pivf / 8.0)*ep;
		point_2 = rot_y(-pivf / 8.0)*ep;
		point_1 = vec3vf{std::abs(point_1.x), point_1.y, std::abs(point_1.z)};
		point_2 = vec3vf{std::abs(point_2.x), point_2.y, std::abs(point_2.z)};
		temp = normalize(vec3vf{1.0, 2.5, 1.0});
		dv = std::max(dot(point_1, temp) - 0.69, dv);
		dv = std::max(dot(point_2, temp) - 0.69, dv);

		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

#endif
