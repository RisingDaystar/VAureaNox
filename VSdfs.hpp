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


#ifndef _VSdfs_H
#define _VSdfs_H

using namespace vnx;

    inline double sdf3d_sphere(const vec3d& ep,const double radius){
        return ygl::length(ep) - radius;
    };
    inline double sdf3d_box(const vec3d& ep,const vec3d& dims){
        const vec3d d = abs(ep) - dims;
        return length(max(d,zero3d)) + std::min(std::max(d.x,std::max(d.y,d.z)),0.0);
    };
    inline double sdf3d_plane(const vec3d& ep,const double yoffset){
        constexpr vec3d dispo = {0,1,0};
        return ygl::dot(ep,dispo) + (1.0 / yoffset);
    };
    inline double sdf3d_triprism(const vec3d& ep,const vec2d& dims){
		const vec3d q = abs(ep);
		return std::max(q.z - dims.y, std::max(q.x*0.866025 + ep.y*0.5, -ep.y) - dims.x*0.5);
    };
    inline double sdf3d_hexprism(const vec3d& ep,const vec2d& dims){
		const vec3d q = abs(ep);
		return std::max(q.z - dims.y, std::max((q.x*0.866025 + q.y*0.5), q.y) - dims.x);
    };
    inline double sdf3d_torus(const vec3d& ep,const vec2d& radii){
        return 0.0; //TODO
    };
    inline double sdf3d_capsule(const vec3d& ep,const double BR,const vec3d& A,const vec3d& B){
		const vec3d pa = ep - A, ba = B - A;
		const double h = ygl::clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
		return length(pa - ba*h) - BR;
    };
    inline double sdf3d_cappedcone(const vec3d& ep,const vec3d& dims){
		const vec2d q = vec2d{ygl::length(vec2d{ ep.x,ep.z }), ep.y};
		const vec2d v = vec2d{dims.z*dims.y / dims.x, -dims.z};
		const vec2d w = v - q;
		const vec2d vv = vec2d{dot(v, v), v.x*v.x};
		const vec2d qv = vec2d{dot(v, w), v.x*w.x};
		const vec2d d = max(qv, zero2d)*qv / vv;
		return sqrt(dot(w, w) - std::max(d.x, d.y)) * sign(std::max(q.y*v.x - q.x*v.y, w.y));
    };
    inline double sdf3d_pyramid4(const vec3d& ep,const vec3d& dims){
        const vec3d d = abs(ep - vec3d{0,-2.0*dims.z,0}) - vec3d{2.0*dims.z,2.0*dims.z,2.0*dims.z};
		const double bdv = std::min(vnx::max_element<double>(d), 0.0) + ygl::length(max(d, zero3d));

        double dv = std::max( 0.0, std::abs( dot(ep, vec3d{ -dims.x, dims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3d{  dims.x, dims.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3d{  0, dims.y, dims.x }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3d{  0, dims.y,-dims.x }) ));
        const double octa = dv - dims.z;
        return std::max(-bdv,octa);
    };
    inline double sdf3d_cylinder(const vec3d& ep,const vec2d& dims){
		const vec2d d = abs(vec2d{length(vec2d{ ep.x,ep.z }), ep.y}) - dims;
		return std::min(std::max(d.x, d.y), 0.0) + length(max(d, zero2d));
    };
    inline double sdf3d_ellipsoid(const vec3d& ep,const vec3d& dims){
        const double k1 = length(ep/dims);
        const double k2 = length(ep/fsipow(dims,2));
        return k1*(k1-1.0)/k2;
    };
    inline double sdf3d_diamond(const vec3d& ep){
        return 0.0; //TODO
    };

    inline double sdf2d_triangle(const vec2d& ep);
    inline double sdf2d_pentagon(const vec2d& ep);
    inline double sdf2d_hexagon(const vec2d& ep);
    inline double sdf2d_octagon(const vec2d& ep);
    inline double sdf2d_hexagram(const vec2d& ep);
    inline double sdf2d_star(const vec2d& ep);
    inline double sdf2d_cross(const vec2d& ep,const vec2d& b,double r){
        vec2d p = abs(ep);
        p = (p.y>p.x) ? vec2d{p.y,p.x} : p;
        const vec2d q = p-b;
        const double k = max(q.y,q.x);
        const vec2d w = (k>0.0) ? q : vec2d{b.y-p.x,-k};
        return sign(k)*length(vnx::max(w,zero2d))+r;
    };
    inline double sdf2d_quad(const vec2d& ep);
    inline double sdf2d_polygon(const vec2d& ep,const std::vector<vec2d>& vertices);
    inline double sdf2d_quad(const vec2d& ep);
    inline double sdf2d_circle(const vec2d& ep);
    inline double sdf2d_line(const vec2d& ep);
    inline double sdf2d_rhombus(const vec2d& ep);


//Personalizzabile

struct vvo_sdf2d_cross : public VSdf {
    vec2d mB = one2d;
    double mR = 1.0;
	vvo_sdf2d_cross(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mB(one2d),mR(1.0){}
	vvo_sdf2d_cross(std::string ids, VMaterial* mtl,const vec2d& b,double r): VSdf(ids,mtl),mB(b),mR(r){}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        //TODO
	}

	inline const char* Type(){return "vvo_sdf2d_cross";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        double dv = sdf2d_cross(vec2d{ep.x,ep.z},mB,mR)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}

};

struct vvo_blended : public VSdf {
    VNode* mSdf_1 = nullptr;
    VNode* mSdf_2 = nullptr;
    double mAFactor;

    vvo_blended(std::string ids, VMaterial* mtl,VSdf* s1,VSdf* s2,double A): VSdf(ids,mtl),mSdf_1(s1),mSdf_2(s2),mAFactor(A){
    }

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        //TODO
	}

    inline const char* Type(){return "vvo_blended";}

	inline void eval(const vec3d& p,VResult& res) {
		vec3d ep = eval_ep(p);
		if(!mSdf_1 || !mSdf_2){
            res = VResult{ this,this,mMaterial,mMaterial,maxd,maxd,p,ep };
		}

        VResult vre_d1;
        mSdf_1->eval(ep,vre_d1);
        VResult vre_d2;
        mSdf_2->eval(ep,vre_d2);

		double dist = mAFactor * vre_d1.dist + (1.0 - mAFactor) * vre_d2.dist;
		double vdist = mAFactor * vre_d1.vdist + (1.0 - mAFactor) * vre_d2.vdist;
		auto dspr = eval_displacement(ep)-mRounding;
		dist+=dspr;
		vdist+=dspr;

		res = VResult{ this,this,mMaterial,mMaterial,dist,vdist,p,ep };
	}
};

struct vvo_shadered : public VSdf {
	std::function<float(const vec3d& p, vec3d& ep, const VNode* tref) > mShader = nullptr;
	vvo_shadered(std::string ids, VMaterial* mtl, std::function<float(const vec3d& p, vec3d& ep, const VNode* tref) > ftor): VSdf(ids,mtl),mShader(ftor) { }

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
	}

	inline const char* Type(){return "vvo_shadered";}

	inline void eval(const vec3d& p,VResult& res) {
		vec3d ep = eval_ep(p);


		float dv = maxd;
		if (mShader != nullptr) { dv = mShader(p, ep, this); }
		dv+=eval_displacement(ep)-mRounding;
		//dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

///

struct vvo_sd_plane : public VSdf {
    double mOffset = 1.0;
	vvo_sd_plane(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mOffset(1.0){}
	vvo_sd_plane(std::string ids, VMaterial* mtl,double offset): VSdf(ids,mtl),mOffset(offset){}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
	}

	inline const char* Type(){return "vvo_sd_plane";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        double dv = sdf3d_plane(ep,mOffset)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_sphere : public VSdf {
	double mRadius = 1.0;
	vvo_sd_sphere(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mRadius(1.0) {}
	vvo_sd_sphere(std::string ids, VMaterial* mtl, double radius): VSdf(ids,mtl),mRadius(radius) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mRadius = try_strtod(entry->try_at(6),mRadius);
	}

	inline const char* Type(){return "vvo_sd_sphere";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
		double dv = sdf3d_sphere(ep,mRadius)+eval_displacement(ep)-mRounding;//ygl::length(ep) - mRadius;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_box : public VSdf {
	vec3d mDims = one3d;
	vvo_sd_box(std::string ids, VMaterial* mtl): VSdf(ids,mtl) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, vec3d dims): VSdf(ids,mtl),mDims(dims) {}
	vvo_sd_box(std::string ids, VMaterial* mtl, double dims): VSdf(ids,mtl),mDims(vec3d{dims,dims,dims}) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mDims = try_strToVec_d(entry->try_at(6),one3d);
	}

	inline const char* Type(){return "vvo_sd_box";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
		double dv = sdf3d_box(ep,mDims)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_ellipsoid : public VSdf {
	vec3d mSizes = one3d;
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mSizes(one3d) {}
	vvo_sd_ellipsoid(std::string ids, VMaterial* mtl, vec3d sizes): VSdf(ids,mtl),mSizes(sizes) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mSizes = try_strToVec_d(entry->try_at(6),one3d);
	}

	inline const char* Type(){return "vvo_sd_ellipsoid";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        double dv = sdf3d_ellipsoid(ep,mSizes)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_cylinder : public VSdf {
	vec2d mDims = one2d;
	vvo_sd_cylinder(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mDims(one2d) {}
	vvo_sd_cylinder(std::string ids, VMaterial* mtl, vec2d dims): VSdf(ids,mtl),mDims(dims) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mDims = try_strToVec_d(entry->try_at(6),one2d);
	}

	inline const char* Type(){return "vvo_sd_cylinder";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        double dv = sdf3d_cylinder(ep,mDims)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capsule : public VSdf {
	double mBaseRadius = 1.0;
	vec3d mA = one3d;
	vec3d mB = one3d;
	vvo_sd_capsule(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mBaseRadius(1.0),mA(one3d),mB(one3d) {}
	vvo_sd_capsule(std::string ids, VMaterial* mtl, double baseRadius, vec3d a, vec3d b): VSdf(ids,mtl),mBaseRadius(baseRadius),mA(a),mB(b) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
	}

	inline const char* Type(){return "vvo_sd_capsule";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        const double dv = sdf3d_capsule(ep,mBaseRadius,mA,mB)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_hex_prism : public VSdf {
	vec2d mDims = one2d;
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mDims(one2d) {}
	vvo_sd_hex_prism(std::string ids, VMaterial* mtl, vec2d dims): VSdf(ids,mtl),mDims(dims) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mDims = try_strToVec_d(entry->try_at(6),one2d);
	}

	inline const char* Type(){return "vvo_sd_hex_prism";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        double dv = sdf3d_hexprism(ep,mDims)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_tri_prism : public VSdf {
	vec2d mDims = one2d;
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mDims(one2d) {}
	vvo_sd_tri_prism(std::string ids, VMaterial* mtl, vec2d dims): VSdf(ids,mtl),mDims(dims) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mDims = try_strToVec_d(entry->try_at(6),one2d);
	}

	inline const char* Type(){return "vvo_sd_tri_prism";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        double dv = sdf3d_triprism(ep,mDims)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};
struct vvo_sd_capped_cone : public VSdf {
	vec3d mDims = one3d;
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mDims(one3d) { }
	vvo_sd_capped_cone(std::string ids, VMaterial* mtl, vec3d dims): VSdf(ids,mtl),mDims(dims) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mDims = try_strToVec_d(entry->try_at(6),one3d);
	}

	inline const char* Type(){return "vvo_sd_capped_cone";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        const double dv = sdf3d_cappedcone(ep,mDims)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};


struct vvo_sd_pyramid4 : public VSdf {
    vec3d mDims = one3d;
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl): VSdf(ids,mtl),mDims(one3d) {}
	vvo_sd_pyramid4(std::string ids, VMaterial* mtl, vec3d dims): VSdf(ids,mtl),mDims(dims) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
        mDims = try_strToVec_d(entry->try_at(6),one3d);
	}

	inline const char* Type(){return "vvo_sd_pyramid4";}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);
        const double dv = sdf3d_pyramid4(ep,mDims)+eval_displacement(ep)-mRounding;
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}

};

struct vvo_sd_diamond : public VSdf {

	vvo_sd_diamond(std::string ids, VMaterial* mtl): VSdf(ids,mtl) {}

	inline void DoRelate(const VEntry* entry){
        set_translation(try_strToVec_d(entry->try_at(2),mTranslation));
        set_rotation_degs(try_strToVec_d(entry->try_at(3),vec3d{mRotation.x,mRotation.y,mRotation.z}));
        set_scale(try_strToVec_d(entry->try_at(4),mScale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(5),mRotationOrder));
	}

	inline const char* Type(){return "vvo_sd_diamond";}

	inline mat3d rot_y(double a) {
		double c = cos(a);
		double s = sin(a);
		return mat3d{
		{ c, 0.0, s },
		{ 0.0, 1.0, 0.0 },
		{ -s, 0.0, c }
		};
	}

	inline void eval(const vec3d& p,VResult& res) {
        vec3d ep = eval_ep(p);

		double dv = mind;
		vec3d point_0 = rot_y(pid / 4.0)*ep;
		vec3d point_1 = rot_y(pid / 8.0 + pid / 16.0)*ep;
		vec3d point_2 = rot_y(pid / 8.0 - pid / 16.0)*ep;
		vec3d point_3 = rot_y(-pid / 8.0 + pid / 16.0)*ep;
		vec3d point_4 = rot_y(-pid / 8.0 - pid / 16.0)*ep;
		vec3d temp;
		point_1 = vec3d{std::abs(point_1.x), -point_1.y, std::abs(point_1.z)};
		point_2 = vec3d{std::abs(point_2.x), -point_2.y, std::abs(point_2.z)};
		point_3 = vec3d{std::abs(point_3.x), -point_3.y, std::abs(point_3.z)};
		point_4 = vec3d{std::abs(point_4.x), -point_4.y, std::abs(point_4.z)};
		temp = normalize(vec3d{1.0, 1.5, 1.0});
		dv = std::max(dot(abs(ep), temp) - 0.5*1.5, dv);
		dv = std::max(dot(abs(point_0), temp) - 0.5*1.5, dv);
		dv = std::max(ep.y - 0.5, dv);
		temp = normalize(vec3d{1.0, 1.4, 1.0});
		dv = std::max(dot(point_1, temp) - 0.762f, dv);
		dv = std::max(dot(point_2, temp) - 0.762f, dv);
		dv = std::max(dot(point_3, temp) - 0.762f, dv);
		dv = std::max(dot(point_4, temp) - 0.762f, dv);
		point_1 = vec3d{point_1.x, -point_1.y, point_1.z};
		point_2 = vec3d{point_2.x, -point_2.y, point_2.z};
		point_3 = vec3d{point_3.x, -point_3.y, point_3.z};
		point_4 = vec3d{point_4.x, -point_4.y, point_4.z};
		temp = normalize(vec3d{1.0, 1.25, 1.0});
		dv = std::max(dot(point_1, temp) - 0.804, dv);
		dv = std::max(dot(point_2, temp) - 0.804, dv);
		dv = std::max(dot(point_3, temp) - 0.804, dv);
		dv = std::max(dot(point_4, temp) - 0.804, dv);
		point_1 = rot_y(pid / 8.0)*ep;
		point_2 = rot_y(-pid / 8.0)*ep;
		point_1 = vec3d{std::abs(point_1.x), point_1.y, std::abs(point_1.z)};
		point_2 = vec3d{std::abs(point_2.x), point_2.y, std::abs(point_2.z)};
		temp = normalize(vec3d{1.0, 2.5, 1.0});
		dv = std::max(dot(point_1, temp) - 0.69, dv);
		dv = std::max(dot(point_2, temp) - 0.69, dv);

		dv+=eval_displacement(ep)-mRounding;
        //dv*=min_element(scale);
		res = VResult{ this,this,mMaterial,mMaterial,dv,dv,p,ep };
	}
};

#endif
