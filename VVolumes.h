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


#ifndef _VVOLUMES_H
#define _VVOLUMES_H

#include "VAureaNox.h"

using namespace vnx;

//Personalizzabile

struct vvo_shadered : public VVolume {
	std::function<float(const vvec3f& p, vvec3f& ep, const VNode* tref) > mShader = nullptr;
	vvo_shadered(std::string ids, VMaterial* mtl, std::function<float(const vvec3f& p, vvec3f& ep, const VNode* tref) > ftor): VVolume(ids,mtl),mShader(ftor) { }

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_shadered";}

	inline void eval(const vvec3f& p,VResult& res) {
		vvec3f ep = eval_ep(p);


		float dv = ygl::maxf;
		if (mShader != nullptr) { dv = mShader(p, ep, this); }
		dv+=eval_displacement(ep);
		//dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

///

struct vvo_sd_plane : public VVolume {
    const vvec3f mDispo = {0,1,0};
    vfloat mOffset = 1.0;
	vvo_sd_plane(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mOffset(1.0f){}
	vvo_sd_plane(std::string ids, VMaterial* mtl,vfloat offset): VVolume(ids,mtl),mOffset(offset){}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_plane";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vfloat dv = ygl::dot(ep,mDispo) + (1.0 / mOffset);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_sphere : public VVolume {
	vfloat mRadius = 1.0f;
	vvo_sd_sphere(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mRadius(1.0f) {}
	vvo_sd_sphere(std::string ids, VMaterial* mtl, vfloat radius): VVolume(ids,mtl),mRadius(radius) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
        mRadius = try_strToVFloat(entry->try_at(6),mRadius);
	}

	inline const char* type(){return "vvo_sd_sphere";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vfloat dv = ygl::length(ep) - mRadius;
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_box : public VVolume {
	vvec3f mDims = vone3f;
	vvo_sd_box(std::string ids, VMaterial* mtl): VVolume(ids,mtl) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, vvec3f dims): VVolume(ids,mtl),mDims(dims) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, vfloat dims): VVolume(ids,mtl),mDims(vvec3f{dims,dims,dims}) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_box";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

        vvec3f d = abs(ep) - mDims;
        vfloat dv = length(vnx::vgt(d,vzero3f)) + std::min(std::max(d.x,std::max(d.y,d.z)),0.0);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_ellipsoid : public VVolume {
	vvec3f mSizes = vone3f;
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mSizes(vone3f) {}
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl, vvec3f sizes): VVolume(ids,mtl),mSizes(sizes) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_ellipsoid";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vfloat dv = (length(ep / mSizes) - 1.0) * std::min(std::min(mSizes.x, mSizes.y), mSizes.z);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_cylinder : public VVolume {
	vvec2f mDims = vone2f;
	vvo_sd_cylinder(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(vone2f) {}
	vvo_sd_cylinder(std::string ids, VMaterial* mtl, vvec2f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_cylinder";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vvec2f d = vabs(vvec2f{length(vvec2f{ ep.x,ep.z }), ep.y}) - mDims;
		vfloat dv = std::min(std::max(d.x, d.y), 0.0) + length(vnx::vgt(d, vzero2f));
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capsule : public VVolume {
	vfloat mBaseRadius = 1.0;
	vvec3f mA = vone3f;
	vvec3f mB = vone3f;
	vvo_sd_capsule(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mBaseRadius(1.0),mA(vone3f),mB(vone3f) {}
	vvo_sd_capsule(std::string ids, VMaterial* mtl, vfloat baseRadius, vvec3f a, vvec3f b): VVolume(ids,mtl),mBaseRadius(baseRadius),mA(a),mB(b) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_capsule";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vvec3f pa = ep - mA, ba = mB - mA;
		vfloat h = ygl::clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
		vfloat dv = length(pa - ba*h) - mBaseRadius;
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_hex_prism : public VVolume {
	vvec2f mDims = vone2f;
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(vone2f) {}
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl, vvec2f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_hex_prism";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vvec3f q = vabs(ep);
		auto dv = std::max(q.z - mDims.y, std::max((q.x*0.866025 + q.y*0.5), q.y) - mDims.x);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_tri_prism : public VVolume {
	vvec2f mDims = vone2f;
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(vone2f) {}
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl, vvec2f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_tri_prism";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vvec3f q = vabs(ep);
		auto dv = std::max(q.z - mDims.y, std::max(q.x*0.866025 + ep.y*0.5, -ep.y) - mDims.x*0.5);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capped_cone : public VVolume {
	vvec3f mDims = vone3f;
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(vone3f) { }
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl, vvec3f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_capped_cone";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vvec2f q = vvec2f{ygl::length(vvec2f{ ep.x,ep.z }), ep.y};
		vvec2f v = vvec2f{mDims.z*mDims.y / mDims.x, -mDims.z};
		vvec2f w = v - q;
		vvec2f vv = vvec2f{dot(v, v), v.x*v.x};
		vvec2f qv = vvec2f{dot(v, w), v.x*w.x};
		vvec2f d = vgt(qv, vzero2f)*qv / vv;
		auto dv = sqrt(dot(w, w) - std::max(d.x, d.y)) * sign(std::max(q.y*v.x - q.x*v.y, w.y));
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};


struct vvo_sd_pyramid4 : public VVolume {
    vvec3f mDims = vone3f;
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(vone3f) {}
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl, vvec3f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_pyramid4";}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

        vvec3f d = vabs(ep - vvec3f{0,-2.0*mDims.z,0}) - vvec3f{2.0*mDims.z,2.0*mDims.z,2.0*mDims.z};
		auto bdv = std::min(vnx::max_element<vfloat>(d), 0.0) + ygl::length(vgt(d, vzero3f));

        vfloat dv = 0.0;
        dv = std::max( dv, std::abs( dot(ep, vvec3f{ -mDims.x, mDims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vvec3f{  mDims.x, mDims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vvec3f{  0, mDims.y, mDims.x }) ));
        dv = std::max( dv, std::abs( dot(ep, vvec3f{  0, mDims.y,-mDims.x }) ));
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
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vvec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_diamond";}

	inline vmat3f rot_y(vfloat a) {
		vfloat c = cos(a);
		vfloat s = sin(a);
		return vmat3f{
		{ c, 0.0, s },
		{ 0.0, 1.0, 0.0 },
		{ -s, 0.0, c }
		};
	}

	inline void eval(const vvec3f& p,VResult& res) {
        vvec3f ep = eval_ep(p);

		vfloat dv = minvf;
		vvec3f point_0 = rot_y(ygl::pi<vfloat> / 4.0)*ep;
		vvec3f point_1 = rot_y(ygl::pi<vfloat> / 8.0 + ygl::pi<vfloat> / 16.0)*ep;
		vvec3f point_2 = rot_y(ygl::pi<vfloat> / 8.0 - ygl::pi<vfloat> / 16.0)*ep;
		vvec3f point_3 = rot_y(-ygl::pi<vfloat> / 8.0 + ygl::pi<vfloat> / 16.0)*ep;
		vvec3f point_4 = rot_y(-ygl::pi<vfloat> / 8.0 - ygl::pi<vfloat> / 16.0)*ep;
		vvec3f temp;
		point_1 = vvec3f{std::abs(point_1.x), -point_1.y, std::abs(point_1.z)};
		point_2 = vvec3f{std::abs(point_2.x), -point_2.y, std::abs(point_2.z)};
		point_3 = vvec3f{std::abs(point_3.x), -point_3.y, std::abs(point_3.z)};
		point_4 = vvec3f{std::abs(point_4.x), -point_4.y, std::abs(point_4.z)};
		temp = normalize(vvec3f{1.0, 1.5, 1.0});
		dv = std::max(dot(vabs(ep), temp) - 0.5*1.5, dv);
		dv = std::max(dot(vabs(point_0), temp) - 0.5*1.5, dv);
		dv = std::max(ep.y - 0.5, dv);
		temp = normalize(vvec3f{1.0, 1.4, 1.0});
		dv = std::max(dot(point_1, temp) - 0.762f, dv);
		dv = std::max(dot(point_2, temp) - 0.762f, dv);
		dv = std::max(dot(point_3, temp) - 0.762f, dv);
		dv = std::max(dot(point_4, temp) - 0.762f, dv);
		point_1 = vvec3f{point_1.x, -point_1.y, point_1.z};
		point_2 = vvec3f{point_2.x, -point_2.y, point_2.z};
		point_3 = vvec3f{point_3.x, -point_3.y, point_3.z};
		point_4 = vvec3f{point_4.x, -point_4.y, point_4.z};
		temp = normalize(vvec3f{1.0, 1.25, 1.0});
		dv = std::max(dot(point_1, temp) - 0.804, dv);
		dv = std::max(dot(point_2, temp) - 0.804, dv);
		dv = std::max(dot(point_3, temp) - 0.804, dv);
		dv = std::max(dot(point_4, temp) - 0.804, dv);
		point_1 = rot_y(ygl::pi<vfloat> / 8.0)*ep;
		point_2 = rot_y(-ygl::pi<vfloat> / 8.0)*ep;
		point_1 = vvec3f{std::abs(point_1.x), point_1.y, std::abs(point_1.z)};
		point_2 = vvec3f{std::abs(point_2.x), point_2.y, std::abs(point_2.z)};
		temp = normalize(vvec3f{1.0, 2.5, 1.0});
		dv = std::max(dot(point_1, temp) - 0.69, dv);
		dv = std::max(dot(point_2, temp) - 0.69, dv);

		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

#endif
