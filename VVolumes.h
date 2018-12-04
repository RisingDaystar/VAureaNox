
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
	std::function<float(const ygl::vec3f& p, ygl::vec3f& ep, const VNode* tref) > shader = nullptr;
	vvo_shadered(std::string ids, VMaterial* mat, std::function<float(const ygl::vec3f& p, ygl::vec3f& ep, const VNode* tref) > ftor) { id = ids; material = mat; shader = ftor; }

	inline const char* type(){return "vvo_shadered";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		float dv = ygl::maxf;
		if (shader != nullptr) { dv = shader(p, ep, this); }
		dv+=eval_displacement(ep);
		dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
///

struct vvo_sd_plane : public VVolume {
	ygl::vec4f n = ygl::vec4f{ 0,1,0,1 };
	vvo_sd_plane(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_plane(std::string ids, VMaterial* mat, ygl::vec4f nv) { id = ids; material = mat; n = nv; }

	inline const char* type(){return "vvo_sd_plane";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;

        //DUAL NUMBERS
        /*
        auto da_dom = dual3Dom<float>(ep);
        auto da = (dot(da_dom,dual3Dom<float>({n.x,n.y,n.z})) - dual3<float>(1.0f/n.w))*dual3<float>(std::min(scale.x, std::min(scale.y, scale.z)));
        return VResult{this,material,da.real,da.dual,p,ep};
        */

		float dv = ygl::dot(ep, ygl::normalize(ygl::vec3f{ n.x,n.y,n.z })) + (1.0 / n.w);
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };

	}
};
struct vvo_sd_sphere : public VVolume {
	float r = 1.0f;
	vvo_sd_sphere(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_sphere(std::string ids, VMaterial* mat, float rv) { id = ids; material = mat; r = rv; }

	inline const char* type(){return "vvo_sd_sphere";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		float dv = ygl::length(ep) - r;
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
struct vvo_sd_box : public VVolume {
	ygl::vec3f b = one3f;
	vvo_sd_box(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_box(std::string ids, VMaterial* mat, ygl::vec3f bv) { id = ids; material = mat; b = bv; }
	vvo_sd_box(std::string ids, VMaterial* mat, float bv) { id = ids; material = mat; b = {bv,bv,bv}; }

	inline const char* type(){return "vvo_sd_box";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
        vec3f d = abs(ep) - b;
        float dv = length(vnx::vgt(d,zero3f)) + std::min(std::max(d.x,std::max(d.y,d.z)),0.0f);
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };

	}
};
struct vvo_sd_ellipsoid : public VVolume {
	ygl::vec3f r = one3f;
	vvo_sd_ellipsoid(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_ellipsoid(std::string ids, VMaterial* mat, ygl::vec3f rv) { id = ids; material = mat; r = rv; }

	inline const char* type(){return "vvo_sd_ellipsoid";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		float dv = (length(ep / r) - 1.0) * std::min(std::min(r.x, r.y), r.z);
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
struct vvo_sd_cylinder : public VVolume {
	ygl::vec2f h = one2f;
	vvo_sd_cylinder(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_cylinder(std::string ids, VMaterial* mat, ygl::vec2f hv) { id = ids; material = mat; h = hv; }

	inline const char* type(){return "vvo_sd_cylinder";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		ygl::vec2f d = vabs(ygl::vec2f{length(ygl::vec2f{ ep.x,ep.z }), ep.y}) - h;
		float dv = std::min(std::max(d.x, d.y), 0.0f) + length(vnx::vgt(d, ygl::zero2f));
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
struct vvo_sd_capsule : public VVolume {
	float r = 1.0f;
	ygl::vec3f a = one3f;
	ygl::vec3f b = one3f;
	vvo_sd_capsule(std::string ids, VMaterial* mat) { id = ids; r = 1.0f; a = one3f; b = one3f; }
	vvo_sd_capsule(std::string ids, VMaterial* mat, float rv, ygl::vec3f av, ygl::vec3f bv) { id = ids; r = rv; a = av; b = bv; }

	inline const char* type(){return "vvo_sd_capsule";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		ygl::vec3f pa = ep - a, ba = b - a;
		float h = ygl::clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
		float dv = length(pa - ba*h) - r;
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
struct vvo_sd_hex_prism : public VVolume {
	ygl::vec2f h = one2f;
	vvo_sd_hex_prism(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_hex_prism(std::string ids, VMaterial* mat, ygl::vec2f hv) { id = ids; material = mat; h = hv; }

	inline const char* type(){return "vvo_sd_hex_prism";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		ygl::vec3f q = vabs(ep);
		auto dv = std::max(q.z - h.y, std::max((q.x*0.866025f + q.y*0.5f), q.y) - h.x);
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
struct vvo_sd_tri_prism : public VVolume {
	ygl::vec2f h = one2f;
	vvo_sd_tri_prism(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_tri_prism(std::string ids, VMaterial* mat, ygl::vec2f hv) { id = ids; material = mat; h = hv; }

	inline const char* type(){return "vvo_sd_tri_prism";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;
		ygl::vec3f q = vabs(ep);
		auto dv = std::max(q.z - h.y, std::max(q.x*0.866025f + ep.y*0.5f, -ep.y) - h.x*0.5f);
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};
struct vvo_sd_capped_cone : public VVolume {
	ygl::vec3f c = one3f;
	vvo_sd_capped_cone(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_capped_cone(std::string ids, VMaterial* mat, ygl::vec3f cv) { id = ids; material = mat; c = cv; }

	inline const char* type(){return "vvo_sd_capped_cone";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;

		ygl::vec2f q = ygl::vec2f{ygl::length(ygl::vec2f{ ep.x,ep.z }), ep.y};
		ygl::vec2f v = ygl::vec2f{c.z*c.y / c.x, -c.z};
		ygl::vec2f w = v - q;
		ygl::vec2f vv = ygl::vec2f{dot(v, v), v.x*v.x};
		ygl::vec2f qv = ygl::vec2f{dot(v, w), v.x*w.x};
		ygl::vec2f d = vgt(qv, ygl::zero2f)*qv / vv;
		auto dv = sqrt(dot(w, w) - std::max(d.x, d.y)) * sign(std::max(q.y*v.x - q.x*v.y, w.y));
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};


struct vvo_sd_pyramid4 : public VVolume {
    ygl::vec3f h = {1,1,1};
	vvo_sd_pyramid4(std::string ids, VMaterial* mat) { id = ids; material = mat; }
	vvo_sd_pyramid4(std::string ids, VMaterial* mat, ygl::vec3f hv) { id = ids; material = mat; h = hv; }

	inline const char* type(){return "vvo_sd_pyramid4";}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;

        ygl::vec3f d = vabs(ep - vec3f{0,-2.0*h.z,0}) - vec3f{2.0*h.z,2.0*h.z,2.0*h.z};
		auto bdv = std::min(vnx::max_element<float>(d), 0.0f) + ygl::length(vgt(d, ygl::zero3f));

        float dv = 0.0;
        dv = std::max( dv, std::abs( dot(ep, vec3f{ -h.x, h.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3f{  h.x, h.y, 0 }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3f{  0, h.y, h.x }) ));
        dv = std::max( dv, std::abs( dot(ep, vec3f{  0, h.y,-h.x }) ));
        float octa = dv - h.z;
        dv = std::max(-bdv,octa);
		dv+=eval_displacement(ep);
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}

};

struct vvo_sd_diamond : public VVolume {

	vvo_sd_diamond(std::string ids, VMaterial* mat) { id = ids; material = mat; }

	inline const char* type(){return "vvo_sd_diamond";}

	mat3f rot_y(float a) {
		float c = cos(a);
		float s = sin(a);
		return mat3f{
		{ c, 0.0, s },
		{ 0.0, 1.0, 0.0 },
		{ -s, 0.0, c }
		};
	}

	virtual inline void eval(const ygl::vec3f& p,VResult& res) {
		ygl::vec3f ep = ygl::transform_point_inverse(_frame, p) / scale;

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
        dv*=min_element(scale);
		res = VResult{ this,this,material,material,dv,dv,zero3f,p,ep };
	}
};



#endif
