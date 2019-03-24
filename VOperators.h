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
	float mBlendFactor = 0.0f;
	vop_union(std::string idv, float bfactor, std::vector<VNode*> chs) : VOperator(idv, chs), mBlendFactor(bfactor){}
	vop_union(std::string idv, std::vector<VNode*> chs) : VOperator(idv, chs), mBlendFactor(0.0f) {}

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
		if (mBlendFactor < ygl::epsf) {
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);

                if(abs(vre.dist)<abs(res.dist)){res = vre;}
				if (vre.vdist<vdist) {vdist = vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}

			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);
				sdist = smin(sdist, vre.dist, mBlendFactor);
				vsdist = smin(vsdist,vre.vdist, mBlendFactor);

				if (vre.dist < res.dist) { res = vre; }
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
	float mBlendFactor = 0.0f;
	bool mPreserveMtl = false;

	vop_intersection(std::string idv, float bfactor, bool pMtl, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(pMtl) {}
	vop_intersection(std::string idv, float bfactor, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(false) {}
	vop_intersection(std::string idv, bool pMtl, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(0.0f), mPreserveMtl(pMtl) {}
	vop_intersection(std::string idv, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(0.0f), mPreserveMtl(false) {}

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
		if (mBlendFactor < ygl::epsf) {
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
				sdist = smax(sdist, vre.dist, mBlendFactor);
				vsdist = smax(vsdist, vre.vdist, mBlendFactor);

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
		if (mPreserveMtl) { res.mat = mtl;res.vmat = vmtl; }
		else{res.vmat = vmat;}
	};
};

struct vop_subtraction : public VOperator {
	float mBlendFactor = 0.0f;
	bool mPreserveMtl = false;

	vop_subtraction(std::string idv, float bfactor, bool pMtl, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(pMtl) {}
	vop_subtraction(std::string idv, float bfactor, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(false) {}
	vop_subtraction(std::string idv, bool pMtl, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(0.0f), mPreserveMtl(pMtl) {}
	vop_subtraction(std::string idv, std::vector<VNode*> chs) : VOperator(idv,chs), mBlendFactor(0.0f), mPreserveMtl(false) {}

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
		if (mBlendFactor < ygl::epsf) {
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);

				if (-vre.dist > res.dist) { res = vre;res.dist = -vre.dist; }
				if(-vre.vdist > vdist) {vdist = -vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}
			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (int i = 1; i < childs.size(); i++) {
				childs[i]->eval(p,vre);
				sdist = smax(sdist,-vre.dist, mBlendFactor);
				vsdist = smax(vsdist,-vre.vdist, mBlendFactor);

				if (-vre.dist > res.dist) { res = vre;res.dist = -vre.dist; }
				if(-vre.vdist > vdist) {vdist = -vre.vdist;vmat = vre.vmat;vsur = vre.vsur;}
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
		if (mPreserveMtl) { res.mat = mtl;res.vmat = vmtl; }
		else{res.vmat = vmat;}
	};
};

struct vop_twist : public VOperator {
	e_axis mAxis = X;
	float mAmount = 1.0f;

	vop_twist(std::string idv, VNode* chs) : VOperator(idv, { chs }), mAxis(X), mAmount(1.0f) {}
	vop_twist(std::string idv, float am, VNode* chs) : VOperator(idv, { chs }), mAxis(X), mAmount(am) {}
	vop_twist(std::string idv, e_axis ax, VNode* chs) : VOperator(idv, { chs }), mAxis(ax), mAmount(1.0f) {}
	vop_twist(std::string idv, e_axis ax, float am, VNode* chs) : VOperator(idv, { chs }), mAxis(ax), mAmount(am) {}

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
		if (mAxis == X) {
			float c = cos(mAmount*ep.x);
			float s = sin(mAmount*ep.x);
			ygl::mat2f  m = ygl::mat2f{{ c, -s }, { s, c }};
			auto mres = m*ygl::vec2f{ ep.y,ep.z };
			ygl::vec3f  q = ygl::vec3f{mres.x, mres.y, ep.x};
			q = ygl::transform_point(_frame, q);
			childs[0]->eval(q,res);
		}
		else if (mAxis == Y) {
			float c = cos(mAmount*ep.y);
			float s = sin(mAmount*ep.y);
			ygl::mat2f  m = ygl::mat2f{{ c, -s }, { s, c }};
			auto mres = m*ygl::vec2f{ ep.x,ep.z };
			ygl::vec3f  q = ygl::vec3f{mres.x, mres.y, ep.y};
			q = ygl::transform_point(_frame, q);
			childs[0]->eval(q,res);
		}
		else if (mAxis == Z) {
			float c = cos(mAmount*ep.z);
			float s = sin(mAmount*ep.z);
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
    ygl::vec3f mCells = one3f;
    ygl::vec<bool,3> mAxis = {true,true,true};

	vop_repeat(std::string idv, VNode* chs) : VOperator(idv, { chs }), mCells(one3f),mAxis({true,true,true}) {}
	vop_repeat(std::string idv,const vec3f& cells, VNode* chs) : VOperator(idv, { chs }), mCells(cells),mAxis({true,true,true}) {}
	vop_repeat(std::string idv,const vec3f& cells,const vec<bool,3>& axis, VNode* chs) : VOperator(idv, { chs }), mCells(cells),mAxis(axis) {}

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


		auto hs = (0.5f*mCells);
        auto mep = gl_mod(ep+hs,mCells)-hs;

        if(mAxis.x) ep.x = mep.x;
        if(mAxis.y) ep.y = mep.y;
        if(mAxis.z) ep.z = mep.z;

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
	vop_invert(std::string idv, VNode* chs) : VOperator(idv, { chs }){}

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
    float mThickness = 0.2f;
    bool mPreserveVolume = true;

	vop_onion(std::string idv, VNode* chs) : VOperator(idv, { chs }), mThickness(0.2f),mPreserveVolume(true) {}
	vop_onion(std::string idv, float thickness, VNode* chs) : VOperator(idv, { chs }), mThickness(thickness),mPreserveVolume(true){}
	vop_onion(std::string idv, float thickness,bool preserve, VNode* chs) : VOperator(idv, { chs }), mThickness(thickness),mPreserveVolume(preserve){}

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


        if(mPreserveVolume){
            res.dist = abs(res.dist+mThickness)-mThickness;
            res.vdist = abs(res.vdist+mThickness)-mThickness;
        }else{
            res.dist = abs(res.dist)-mThickness;
            res.vdist = abs(res.vdist)-mThickness;
        }

        res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm;
		//res.dist *= sfct;

		res.vdist += dspm;
		//res.vdist *= sfct;
	}
};

struct vop_cut : public VOperator {
    vec<short,3> mAxis = {0,1,0};
    vec3f mOffset = zero3f;
    float mBlendFactor = 0.0f;

	vop_cut(std::string idv, VNode* chs) : VOperator(idv, { chs }),mAxis({0,1,0}),mOffset(zero3f),mBlendFactor(0.0f) {}
	vop_cut(std::string idv,const vec<short,3>& axis,const vec3f& offset,float bfactor, VNode* chs) : VOperator(idv, { chs }),mAxis(axis),mOffset(offset),mBlendFactor(bfactor) {}

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
        auto epe = transform_point_inverse(_frame, p+mOffset);

        auto dist = res.dist;
        auto vdist = res.vdist;
        if(mBlendFactor<ygl::epsf){

            if(mAxis.x!=0){
                auto f = mAxis.x*epe.x;
                dist = std::max(dist,f);
                vdist = std::max(vdist,f);
            }
            if(mAxis.y!=0){
                auto f = mAxis.y*epe.y;
                dist = std::max(dist,f);
                vdist = std::max(vdist,f);
            }
            if(mAxis.z!=0){
                auto f = mAxis.z*epe.z;
                dist = std::max(dist,f);
                vdist = std::max(vdist,f);
            }
        }else{
            if(mAxis.x!=0){
                auto f = mAxis.x*epe.x;
                dist = smax(dist,f,mBlendFactor);
                vdist = smax(vdist,f,mBlendFactor);
            }
            if(mAxis.y!=0){
                auto f = mAxis.y*epe.y;
                dist = smax(dist,f,mBlendFactor);
                vdist = smax(vdist,f,mBlendFactor);
            }
            if(mAxis.z!=0){
                auto f = mAxis.z*epe.z;
                dist = smax(dist,f,mBlendFactor);
                vdist = smax(vdist,f,mBlendFactor);
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
