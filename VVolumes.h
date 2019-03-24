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
	std::function<float(const ygl::vec3f& p, ygl::vec3f& ep, const VNode* tref) > mShader = nullptr;
	vvo_shadered(std::string ids, VMaterial* mtl, std::function<float(const ygl::vec3f& p, ygl::vec3f& ep, const VNode* tref) > ftor): VVolume(ids,mtl),mShader(ftor) { }

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_shadered";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = eval_ep(p);


		float dv = ygl::maxf;
		if (mShader != nullptr) { dv = mShader(p, ep, this); }
		dv+=eval_displacement(ep);
		//dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

///

struct vvo_sd_plane : public VVolume {
    const vec3f mDispo = {0,1,0};
    float mOffset = 1.0f;
	vvo_sd_plane(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mOffset(1.0f){}
	vvo_sd_plane(std::string ids, VMaterial* mtl,float offset): VVolume(ids,mtl),mOffset(offset){}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_plane";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		float dv = ygl::dot(ep,mDispo) + (1.0f / mOffset);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_sphere : public VVolume {
	float mRadius = 1.0f;
	vvo_sd_sphere(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mRadius(1.0f) {}
	vvo_sd_sphere(std::string ids, VMaterial* mtl, float radius): VVolume(ids,mtl),mRadius(radius) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_sphere";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		float dv = ygl::length(ep) - mRadius;
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_box : public VVolume {
	ygl::vec3f mDims = one3f;
	vvo_sd_box(std::string ids, VMaterial* mtl): VVolume(ids,mtl) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, ygl::vec3f dims): VVolume(ids,mtl),mDims(dims) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, float dims): VVolume(ids,mtl),mDims({dims,dims,dims}) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_box";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

        vec3f d = abs(ep) - mDims;
        float dv = length(vnx::vgt(d,zero3f)) + std::min(std::max(mDims.x,std::max(mDims.y,mDims.z)),0.0f);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_ellipsoid : public VVolume {
	ygl::vec3f mSizes = one3f;
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mSizes(one3f) {}
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl, ygl::vec3f sizes): VVolume(ids,mtl),mSizes(sizes) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_ellipsoid";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		float dv = (length(ep / mSizes) - 1.0f) * std::min(std::min(mSizes.x, mSizes.y), mSizes.z);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_cylinder : public VVolume {
	ygl::vec2f mDims = one2f;
	vvo_sd_cylinder(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one2f) {}
	vvo_sd_cylinder(std::string ids, VMaterial* mtl, ygl::vec2f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_cylinder";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		ygl::vec2f d = vabs(ygl::vec2f{length(ygl::vec2f{ ep.x,ep.z }), ep.y}) - mDims;
		float dv = std::min(std::max(d.x, d.y), 0.0f) + length(vnx::vgt(d, ygl::zero2f));
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capsule : public VVolume {
	float mBaseRadius = 1.0f;
	ygl::vec3f mA = one3f;
	ygl::vec3f mB = one3f;
	vvo_sd_capsule(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mBaseRadius(1.0f),mA(one3f),mB(one3f) {}
	vvo_sd_capsule(std::string ids, VMaterial* mtl, float baseRadius, ygl::vec3f a, ygl::vec3f b): VVolume(ids,mtl),mBaseRadius(baseRadius),mA(a),mB(b) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_capsule";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		ygl::vec3f pa = ep - mA, ba = mB - mA;
		float h = ygl::clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
		float dv = length(pa - ba*h) - mBaseRadius;
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_hex_prism : public VVolume {
	ygl::vec2f mDims = one2f;
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one2f) {}
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl, ygl::vec2f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_hex_prism";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		ygl::vec3f q = vabs(ep);
		auto dv = std::max(q.z - mDims.y, std::max((q.x*0.866025f + q.y*0.5f), q.y) - mDims.x);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_tri_prism : public VVolume {
	ygl::vec2f mDims = one2f;
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one2f) {}
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl, ygl::vec2f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_tri_prism";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		ygl::vec3f q = vabs(ep);
		auto dv = std::max(q.z - mDims.y, std::max(q.x*0.866025f + ep.y*0.5f, -ep.y) - mDims.x*0.5f);
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capped_cone : public VVolume {
	ygl::vec3f mDims = one3f;
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one3f) { }
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl, ygl::vec3f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_capped_cone";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

		ygl::vec2f q = ygl::vec2f{ygl::length(ygl::vec2f{ ep.x,ep.z }), ep.y};
		ygl::vec2f v = ygl::vec2f{mDims.z*mDims.y / mDims.x, -mDims.z};
		ygl::vec2f w = v - q;
		ygl::vec2f vv = ygl::vec2f{dot(v, v), v.x*v.x};
		ygl::vec2f qv = ygl::vec2f{dot(v, w), v.x*w.x};
		ygl::vec2f d = vgt(qv, ygl::zero2f)*qv / vv;
		auto dv = sqrt(dot(w, w) - std::max(d.x, d.y)) * sign(std::max(q.y*v.x - q.x*v.y, w.y));
		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};


struct vvo_sd_pyramid4 : public VVolume {
    ygl::vec3f mDims = one3f;
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl): VVolume(ids,mtl),mDims(one3f) {}
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl, ygl::vec3f dims): VVolume(ids,mtl),mDims(dims) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(2),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_pyramid4";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);

        ygl::vec3f d = vabs(ep - vec3f{0,-2.0*mDims.z,0}) - vec3f{2.0*mDims.z,2.0*mDims.z,2.0*mDims.z};
		auto bdv = std::min(vnx::max_element<float>(d), 0.0f) + ygl::length(vgt(d, ygl::zero3f));

        float dv = 0.0;
        dv = std::max( dv, std::abs( dot(ep, vec3f{ -mDims.x, mDims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3f{  mDims.x, mDims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3f{  0, mDims.y, mDims.x }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3f{  0, mDims.y,-mDims.x }) ));
        float octa = dv - mDims.z;
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
        set_rotation_degs(try_strToVec3f(entry->try_at(3),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(4),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),rotation_order));
	}

	inline const char* type(){return "vvo_sd_diamond";}

	inline mat3f rot_y(float a) {
		float c = cos(a);
		float s = sin(a);
		return mat3f{
		{ c, 0.0, s },
		{ 0.0, 1.0, 0.0 },
		{ -s, 0.0, c }
		};
	}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        ygl::vec3f ep = eval_ep(p);


		float dv = minf;
		vec3f point_0 = rot_y(ygl::pif / 4.0)*ep;
		vec3f point_1 = rot_y(ygl::pif / 8.0 + ygl::pif / 16.0)*ep;
		vec3f point_2 = rot_y(ygl::pif / 8.0 - ygl::pif / 16.0)*ep;
		vec3f point_3 = rot_y(-ygl::pif / 8.0 + ygl::pif / 16.0)*ep;
		vec3f point_4 = rot_y(-ygl::pif / 8.0 - ygl::pif / 16.0)*ep;
		vec3f temp;
		point_1 = vec3f{std::abs(point_1.x), -point_1.y, std::abs(point_1.z)};
		point_2 = vec3f{std::abs(point_2.x), -point_2.y, std::abs(point_2.z)};
		point_3 = vec3f{std::abs(point_3.x), -point_3.y, std::abs(point_3.z)};
		point_4 = vec3f{std::abs(point_4.x), -point_4.y, std::abs(point_4.z)};
		temp = normalize(vec3f{1.0, 1.5f, 1.0});
		dv = std::max(dot(vabs(ep), temp) - 0.5f*1.5f, dv);
		dv = std::max(dot(vabs(point_0), temp) - 0.5f*1.5f, dv);
		dv = std::max(ep.y - 0.5f, dv);
		temp = normalize(vec3f{1.0, 1.4f, 1.0});
		dv = std::max(dot(point_1, temp) - 0.762f, dv);
		dv = std::max(dot(point_2, temp) - 0.762f, dv);
		dv = std::max(dot(point_3, temp) - 0.762f, dv);
		dv = std::max(dot(point_4, temp) - 0.762f, dv);
		point_1 = vec3f{point_1.x, -point_1.y, point_1.z};
		point_2 = vec3f{point_2.x, -point_2.y, point_2.z};
		point_3 = vec3f{point_3.x, -point_3.y, point_3.z};
		point_4 = vec3f{point_4.x, -point_4.y, point_4.z};
		temp = normalize(vec3f{1.0, 1.25, 1.0});
		dv = std::max(dot(point_1, temp) - 0.804f, dv);
		dv = std::max(dot(point_2, temp) - 0.804f, dv);
		dv = std::max(dot(point_3, temp) - 0.804f, dv);
		dv = std::max(dot(point_4, temp) - 0.804f, dv);
		point_1 = rot_y(ygl::pif / 8.0)*ep;
		point_2 = rot_y(-ygl::pif / 8.0)*ep;
		point_1 = vec3f{std::abs(point_1.x), point_1.y, std::abs(point_1.z)};
		point_2 = vec3f{std::abs(point_2.x), point_2.y, std::abs(point_2.z)};
		temp = normalize(vec3f{1.0f, 2.5f, 1.0f});
		dv = std::max(dot(point_1, temp) - 0.69f, dv);
		dv = std::max(dot(point_2, temp) - 0.69f, dv);

		dv+=eval_displacement(ep);
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

#endif
