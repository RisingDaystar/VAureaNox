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

#ifndef _VOPERATORS_H
#define _VOPERATORS_H

#include "VAureaNox.h"

using namespace vnx;

struct vop_union : public VOperator {
	float blend_factor = 0.0f;
	vop_union(std::string idv, float bfactor, std::vector<VNode*> chs) : VOperator(idv, chs), blend_factor(bfactor){}
	vop_union(std::string idv, std::vector<VNode*> chs) : VOperator(idv, chs), blend_factor(0.0f) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}

	inline const char* type(){return "vop_union";}

	inline void eval(const vec3f& p,VResult& res) {
		if (childs.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		VResult vre;
		childs[0]->eval(p,res);
		auto vdist = res.vdist;
		auto vmat = res.vmat;
		auto vsur = res.vsur;
		if (blend_factor < ygl::epsf) {
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);


                if(abs(vre.dist)<abs(res.dist)){res = vre;} //per abilitare i nested : usare std::abs nel confronto
				if (vre.vdist<vdist) {vdist = vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}

			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);
				sdist = smin(sdist, vre.dist, blend_factor);
				vsdist = smin(vsdist,vre.vdist, blend_factor);

				if (vre.dist < res.dist) { res = vre; } ////per abilitare i nested : usare std::abs nel confronto
				if (vre.vdist<vdist) {vdist = vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}

			}
            res.dist = sdist;
            vdist = vsdist;
		}

		res.vdist = vdist;
		res.vmat = vmat;
		res.vsur = vsur;
		res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;


		res.vdist += dspm;
		//res.vdist *= sfct;
	};
};


struct vop_intersection : public VOperator {
	float blend_factor = 0.0f;
	bool preserve_material = false;

	vop_intersection(std::string idv, float bfactor, bool pmaterial, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(bfactor), preserve_material(pmaterial) {}
	vop_intersection(std::string idv, float bfactor, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(bfactor), preserve_material(false) {}
	vop_intersection(std::string idv, bool pmaterial, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(0.0f), preserve_material(pmaterial) {}
	vop_intersection(std::string idv, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(0.0f), preserve_material(false) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}


	inline const char* type(){return "vop_intersection";}

	inline void eval(const vec3f& p,VResult& res) {
		if (childs.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		VResult vre;
		VMaterial* mtl = nullptr;
		VMaterial* vmtl = nullptr;
        childs[0]->eval(p,res);
        mtl = res.mat;
        vmtl = res.vmat;
        auto vdist = res.vdist;
        auto vmat = res.vmat;
        auto vsur = res.vsur;
		if (blend_factor < ygl::epsf) {
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);
				if (vre.dist > res.dist) { res = vre; }
				if(vre.vdist>vdist) {vdist = vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}
			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);
				sdist = smax(sdist, vre.dist, blend_factor);
				vsdist = smax(vsdist, vre.vdist, blend_factor);

				if (vre.dist > res.dist) { res = vre; }
				if(vre.vdist>vdist) {vdist = vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}
			}
			res.dist = sdist;
            vdist = vsdist;
		}
        res.vdist = vdist;
        res.vsur = vsur;
		res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
		if (preserve_material) { res.mat = mtl;res.vmat = vmtl; }
		else{res.vmat = vmat;}
	};
};

struct vop_subtraction : public VOperator {
	float blend_factor = 0.0f;
	bool preserve_material = false;

	vop_subtraction(std::string idv, float bfactor, bool pmaterial, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(bfactor), preserve_material(pmaterial) {}
	vop_subtraction(std::string idv, float bfactor, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(bfactor), preserve_material(false) {}
	vop_subtraction(std::string idv, bool pmaterial, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(0.0f), preserve_material(pmaterial) {}
	vop_subtraction(std::string idv, std::vector<VNode*> chs) : VOperator(idv,chs), blend_factor(0.0f), preserve_material(false) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}


	inline const char* type(){return "vop_subtraction";}

	inline void eval(const vec3f& p,VResult& res) {
		if (childs.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		VResult vre;
		VMaterial* mtl = nullptr;
		VMaterial* vmtl = nullptr;
        childs[0]->eval(p,res);
        mtl = res.mat;
        vmtl = res.vmat;
        auto vdist = res.vdist;
        auto vmat = res.vmat;
        auto vsur = res.vsur;
		if (blend_factor < ygl::epsf) {
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);

				if (-vre.dist > res.dist) { res = vre;res.dist = -vre.dist; }
				if(-vre.vdist>vdist) {vdist = -vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}
			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);
				sdist = smax(sdist,-vre.dist, blend_factor);
				vsdist = smax(vsdist,-vre.vdist, blend_factor);

				if (-vre.dist > res.dist) { res = vre;res.dist = -vre.dist; }
				if(-vre.vdist>vdist) {vdist = -vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}
			}
            res.dist = sdist;
            vdist = vsdist;
		}
		res.vdist = vdist;
		res.wor_pos = p;
		res.vsur = vsur;
		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
		if (preserve_material) { res.mat = mtl;res.vmat = vmtl; }
		else{res.vmat = vmat;}
	};
};

struct vop_twist : public VOperator {
	e_axis axis = X;
	float amount = 1.0f;

	vop_twist(std::string idv, VNode* chs) : VOperator(idv, { chs }), axis(X), amount(1.0f) {}
	vop_twist(std::string idv, float am, VNode* chs) : VOperator(idv, { chs }), axis(X), amount(am) {}
	vop_twist(std::string idv, e_axis ax, VNode* chs) : VOperator(idv, { chs }), axis(ax), amount(1.0f) {}
	vop_twist(std::string idv, e_axis ax, float am, VNode* chs) : VOperator(idv, { chs }), axis(ax), amount(am) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}

	inline const char* type(){return "vop_twist";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
        if (childs.empty()) { return; }
        ygl::vec3f ep  = transform_point_inverse(_frame, p);
		//ygl::vec3f ep = transform_point_inverse(_frame, p) / scale;
		//eval_ep(ep);
        //ep = transform_point_inverse(_frame, ep) / scale;

        //ep = transform_point(_frame, ep);

		VResult vre;
		if (axis == X) {
			float c = cos(amount*ep.x);
			float s = sin(amount*ep.x);
			ygl::mat2f  m = ygl::mat2f{{ c, -s }, { s, c }};
			auto mres = m*ygl::vec2f{ ep.y,ep.z };
			ygl::vec3f  q = ygl::vec3f{mres.x, mres.y, ep.x};
			q = ygl::transform_point(_frame, q);
			childs[0]->eval(q,res);
		}
		else if (axis == Y) {
			float c = cos(amount*ep.y);
			float s = sin(amount*ep.y);
			ygl::mat2f  m = ygl::mat2f{{ c, -s }, { s, c }};
			auto mres = m*ygl::vec2f{ ep.x,ep.z };
			ygl::vec3f  q = ygl::vec3f{mres.x, mres.y, ep.y};
			q = ygl::transform_point(_frame, q);
			childs[0]->eval(q,res);
		}
		else if (axis == Z) {
			float c = cos(amount*ep.z);
			float s = sin(amount*ep.z);
			ygl::mat2f  m = ygl::mat2f{{ c, -s }, { s, c }};
			auto mres = m*ygl::vec2f{ ep.x,ep.y };
			ygl::vec3f  q = ygl::vec3f{mres.x, mres.y, ep.z};
			q = ygl::transform_point(_frame, q);
			childs[0]->eval(q,res);
		}

		res.wor_pos = p;
		res.dist *= (1.0 / 3.0f);
		res.vdist *= (1.0 / 3.0f);
		auto dspm = eval_displacement(ep);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
	}
};

struct vop_repeat : public VOperator {
    ygl::vec3f cells = {1.0f,1.0f,1.0f};
    ygl::vec<bool,3> axis = {true,true,true};

	vop_repeat(std::string idv, VNode* chs) : VOperator(idv, { chs }), cells(one3f),axis({true,true,true}) {}
	vop_repeat(std::string idv,const vec3f& cs, VNode* chs) : VOperator(idv, { chs }), cells(cs),axis({true,true,true}) {}
	vop_repeat(std::string idv,const vec3f& cs,const vec<bool,3>& ax, VNode* chs) : VOperator(idv, { chs }), cells(cs),axis(ax) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}

	inline const char* type(){return "vop_repeat";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
	    if (childs.empty()) { return; }
		ygl::vec3f ep  = transform_point_inverse(_frame, p);
		//eval_ep(ep);


		auto hs = (0.5f*cells);
        auto mep = gl_mod(ep+hs,cells)-hs;

        if(axis.x) ep.x = mep.x;
        if(axis.y) ep.y = mep.y;
        if(axis.z) ep.z = mep.z;

        ep  = transform_point(_frame, ep);
        childs[0]->eval(ep,res);

		res.wor_pos = p;
		auto dspm = eval_displacement(ep);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
	}
};

struct vop_invert : public VOperator {
	e_axis axis = X;
	float amount = 5.0f;

	vop_invert(std::string idv, VNode* chs) : VOperator(idv, { chs }), axis(X), amount(5.0f) {}
	vop_invert(std::string idv, VNode* chs, e_axis ax) : VOperator(idv, { chs }), axis(ax), amount(5.0f) {}
	vop_invert(std::string idv, VNode* chs, e_axis ax, float am) : VOperator(idv, { chs }), axis(ax), amount(am) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}

	inline const char* type(){return "vop_invert";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
	    if (childs.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		childs[0]->eval(p,res);
		res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		res.dist *=-1;
		//res.dist *= sfct;

		res.vdist += dspm;
		res.vdist *=-1;
		//res.vdist *= sfct;
	}
};



struct vop_onion : public VOperator {
    float thickness = 0.2f;
	vop_onion(std::string idv, VNode* chs) : VOperator(idv, { chs }), thickness(0.2f) {}
	vop_onion(std::string idv, float tk, VNode* chs) : VOperator(idv, { chs }), thickness(tk){}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}

	inline const char* type(){return "vop_onion";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
	    if (childs.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

        childs[0]->eval(p,res);
		res.wor_pos = p;

        res.dist = abs(res.dist+thickness)-thickness; //abs(dist+thickness) ->added +thickness to preserve volume
        res.vdist = abs(res.vdist+thickness)-thickness; //abs(dist+thickness) ->added +thickness to preserve volume

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
	}
};

struct vop_cut : public VOperator {
    vec<short,3> axis = {0,1,0};
    vec3f offset = zero3f;
    float blend_factor = 0.0f;
	vop_cut(std::string idv, VNode* chs) : VOperator(idv, { chs }),axis({0,1,0}),offset(zero3f),blend_factor(0.0f) {}
	vop_cut(std::string idv,const vec<short,3>& ax,const vec3f& off,float bf, VNode* chs) : VOperator(idv, { chs }),axis(ax),offset(off),blend_factor(bf) {}

	void DoRelate(const VEntry* entry){
        set_translation(try_strToVec3f(entry->try_at(1),translation));
        set_rotation_degs(try_strToVec3f(entry->try_at(2),vec3f{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec3f(entry->try_at(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_at(4),rotation_order));
	}

	inline const char* type(){return "vop_cut";}

	inline void eval(const ygl::vec3f& p,VResult& res) {
	    if (childs.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

        childs[0]->eval(p,res);
        auto epe = transform_point_inverse(_frame, p+offset);

        auto dist = res.dist;
        auto vdist = res.vdist;
        if(blend_factor<ygl::epsf){
            if(axis.x!=0){
                dist = std::max(dist,axis.x*epe.x);
                vdist = std::max(vdist,axis.x*epe.x);
            }
            if(axis.y!=0){
                dist = std::max(dist,axis.y*epe.y);
                vdist = std::max(vdist,axis.y*epe.y);
            }
            if(axis.z!=0){
                dist = std::max(dist,axis.z*epe.z);
                vdist = std::max(vdist,axis.z*epe.z);
            }
        }else{
            if(axis.x!=0){
                dist = smax(dist,axis.x*epe.x,blend_factor);
                vdist = smax(vdist,axis.x*epe.x,blend_factor);
            }
            if(axis.y!=0){
                dist = smax(dist,axis.y*epe.y,blend_factor);
                vdist = smax(vdist,axis.y*epe.y,blend_factor);
            }
            if(axis.z!=0){
                dist = smax(dist,axis.z*epe.z,blend_factor);
                vdist = smax(vdist,axis.z*epe.z,blend_factor);
            }
        }
        res.dist = dist;
        res.vdist = vdist;

		res.wor_pos = p;
		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
	}
};


#endif
