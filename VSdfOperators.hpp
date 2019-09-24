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

#ifndef _VSdfOperators_H
#define _VSdfOperators_H

using namespace vnx;

struct vop_union : public VSdfOperator {
	double mBlendFactor = 0.0;
	vop_union(std::string idv, double bfactor, std::vector<VNode*> chs) : VSdfOperator(idv, chs), mBlendFactor(bfactor){}
	vop_union(std::string idv, std::vector<VNode*> chs) : VSdfOperator(idv, chs), mBlendFactor(0.0) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mBlendFactor = try_strtod(entry->try_get("blend"),mBlendFactor);
	}

	inline const char* Type(){return "vop_union";}

	inline void eval(const vec3d& p,VResult& res) {
		if (mChilds.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		VResult vre;
		mChilds[0]->eval(p,res);
		auto vdist = res.vdist;
		auto vmat = res.vmtl;
		auto vsur = res.vsur;
		if (mBlendFactor < thrd) {
			for (std::vector<VNode*>::size_type i = 1; i < mChilds.size(); i++) {
				mChilds[i]->eval(p,vre);

                if(abs(vre.dist)<abs(res.dist)){res = vre;}
				if (vre.vdist<vdist) {vdist = vre.vdist;vmat = vre.vmtl;vsur = vre.vsur;}
			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (std::vector<VNode*>::size_type i = 1; i < mChilds.size(); i++) {
				mChilds[i]->eval(p,vre);
				sdist = smin(sdist, vre.dist, mBlendFactor);
				vsdist = smin(vsdist,vre.vdist, mBlendFactor);

				//if (vre.dist < res.dist) { res = vre; }
				if(abs(vre.dist)<abs(res.dist)){res = vre;}
				if (vre.vdist<vdist) {vdist = vre.vdist;vmat = vre.vmtl;vsur = vre.vsur;}

			}
            res.dist = sdist;
            vdist = vsdist;
		}

		res.vdist = vdist;
		res.vmtl = vmat;
		res.vsur = vsur;
		res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		//res.dist *= sfct;
		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
	};
};


struct vop_intersection : public VSdfOperator {
	double mBlendFactor = 0.0;
	bool mPreserveMtl = false;

	vop_intersection(std::string idv, double bfactor, bool pMtl, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(pMtl) {}
	vop_intersection(std::string idv, double bfactor, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(false) {}
	vop_intersection(std::string idv, bool pMtl, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(0.0), mPreserveMtl(pMtl) {}
	vop_intersection(std::string idv, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(0.0), mPreserveMtl(false) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mBlendFactor = try_strtod(entry->try_get("blend"),mBlendFactor);
        mPreserveMtl = try_strtob(entry->try_get("preserve"),mPreserveMtl);
	}


	inline const char* Type(){return "vop_intersection";}

	inline void eval(const vec3d& p,VResult& res) {
		if (mChilds.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		VResult vre;
		VMaterial* mtl = nullptr;
		VMaterial* vmtl = nullptr;
        mChilds[0]->eval(p,res);
        mtl = res.mtl;
        vmtl = res.vmtl;
        auto vdist = res.vdist;
        auto vmat = res.vmtl;
        auto vsur = res.vsur;
		if (mBlendFactor < thrd) {
			for (std::vector<VNode*>::size_type i = 1; i < mChilds.size(); i++) {
				mChilds[i]->eval(p,vre);
				if (vre.dist > res.dist) { res = vre; }
				if(vre.vdist>vdist) {vdist = vre.vdist;vmat = vre.vmtl;vsur = vre.vsur;}
			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (std::vector<VNode*>::size_type i = 1; i < mChilds.size(); i++) {
				mChilds[i]->eval(p,vre);
				sdist = smax(sdist, vre.dist, mBlendFactor);
				vsdist = smax(vsdist, vre.vdist, mBlendFactor);

				if (vre.dist > res.dist) { res = vre; }
				if(vre.vdist>vdist) {vdist = vre.vdist;vmat = vre.vmtl;vsur = vre.vsur;}
			}
			res.dist = sdist;
            vdist = vsdist;
		}
        res.vdist = vdist;
        res.vsur = vsur;
		res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
		if (mPreserveMtl) { res.mtl = mtl;res.vmtl = vmtl; }
		else{res.vmtl = vmat;}
	};
};

struct vop_subtraction : public VSdfOperator {
	double mBlendFactor = 0.0;
	bool mPreserveMtl = false;

	vop_subtraction(std::string idv, double bfactor, bool pMtl, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(pMtl) {}
	vop_subtraction(std::string idv, double bfactor, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(bfactor), mPreserveMtl(false) {}
	vop_subtraction(std::string idv, bool pMtl, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(0.0), mPreserveMtl(pMtl) {}
	vop_subtraction(std::string idv, std::vector<VNode*> chs) : VSdfOperator(idv,chs), mBlendFactor(0.0), mPreserveMtl(false) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mBlendFactor = try_strtod(entry->try_get("blend"),mBlendFactor);
        mPreserveMtl = try_strtob(entry->try_get("preserve"),mPreserveMtl);
	}


	inline const char* Type(){return "vop_subtraction";}

	inline void eval(const vec3d& p,VResult& res) {
		if (mChilds.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		VResult vre;
		VMaterial* mtl = nullptr;
		VMaterial* vmtl = nullptr;
        mChilds[0]->eval(p,res);
        mtl = res.mtl;
        vmtl = res.vmtl;
        auto vdist = res.vdist;
        auto vmat = res.vmtl;
        auto vsur = res.vsur;
		if (mBlendFactor < thrd) {
			for (std::vector<VNode*>::size_type i = 1; i < mChilds.size(); i++) {
				mChilds[i]->eval(p,vre);

				if (-vre.dist > res.dist) { res = vre;res.dist = -vre.dist; }
				if(-vre.vdist > vdist) {vdist = -vre.vdist;vmat = vre.vmtl;vsur = vre.vsur;}
			}
		}
		else {
            auto sdist = res.dist;
            auto vsdist = res.vdist;
			for (std::vector<VNode*>::size_type i = 1; i < mChilds.size(); i++) {
				mChilds[i]->eval(p,vre);
				sdist = smax(sdist,-vre.dist, mBlendFactor);
				vsdist = smax(vsdist,-vre.vdist, mBlendFactor);

				if (-vre.dist > res.dist) { res = vre;res.dist = -vre.dist; }
				if(-vre.vdist > vdist) {vdist = -vre.vdist;vmat = vre.vmtl;vsur = vre.vsur;}
			}
            res.dist = sdist;
            vdist = vsdist;
		}
		res.vdist = vdist;
		res.wor_pos = p;
		res.vsur = vsur;
		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
		if (mPreserveMtl) { res.mtl = mtl;res.vmtl = vmtl; }
		else{res.vmtl = vmat;}
	};
};

struct vop_twist : public VSdfOperator {
	VAxis mAxis = X;
	double mAmount = 1.0;

	vop_twist(std::string idv, VNode* chs) : VSdfOperator(idv, { chs }), mAxis(X), mAmount(1.0) {}
	vop_twist(std::string idv, double am, VNode* chs) : VSdfOperator(idv, { chs }), mAxis(X), mAmount(am) {}
	vop_twist(std::string idv, VAxis ax, VNode* chs) : VSdfOperator(idv, { chs }), mAxis(ax), mAmount(1.0) {}
	vop_twist(std::string idv, VAxis ax, double am, VNode* chs) : VSdfOperator(idv, { chs }), mAxis(ax), mAmount(am) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mAxis = try_strToAxis(entry->try_get("axis"),mAxis);
        mAmount = try_strtod(entry->try_get("amount"),mAmount);
	}

	inline const char* Type(){return "vop_twist";}

	inline void eval(const vec3d& p,VResult& res) {
        if (mChilds.empty()) { return; }
        vec3d ep  = transform_point_inverse(mFrame, p);
		//ygl::vec3f ep = transform_point_inverse(_frame, p) / scale;
		//eval_ep(ep);
        //ep = transform_point_inverse(_frame, ep) / scale;

        //ep = transform_point(_frame, ep);

		VResult vre;
		if (mAxis == X) {
			double c = std::cos(mAmount*ep.x);
			double s = std::sin(mAmount*ep.x);
			mat2d  m = mat2d{{ c, -s }, { s, c }};
			auto mres = m*vec2d{ ep.y,ep.z };
			vec3d  q = vec3d{mres.x, mres.y, ep.x};
			q = ygl::transform_point(mFrame, q);
			mChilds[0]->eval(q,res);
		}else if (mAxis == Y) {
			double c = std::cos(mAmount*ep.y);
			double s = std::sin(mAmount*ep.y);
			mat2d  m = mat2d{{ c, -s }, { s, c }};
			auto mres = m*vec2d{ ep.x,ep.z };
			vec3d  q = vec3d{mres.x, mres.y, ep.y};
			q = ygl::transform_point(mFrame, q);
			mChilds[0]->eval(q,res);
		}else if (mAxis == Z) {
			double c = std::cos(mAmount*ep.z);
			double s = std::sin(mAmount*ep.z);
			mat2d  m = mat2d{{ c, -s }, { s, c }};
			auto mres = m*vec2d{ ep.x,ep.y };
			vec3d  q = vec3d{mres.x, mres.y, ep.z};
			q = ygl::transform_point(mFrame, q);
			mChilds[0]->eval(q,res);
		}

		res.wor_pos = p;
		res.dist *= (1.0 / 3.0);
		res.vdist *= (1.0 / 3.0);
		auto dspm = eval_displacement(ep);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
	}
};

struct vop_repeat : public VSdfOperator {
    vec3d mCells = one3d;
    ygl::vec<bool,3> mAxis = {true,true,true};

	vop_repeat(std::string idv, VNode* chs) : VSdfOperator(idv, { chs }), mCells(one3d),mAxis({true,true,true}) {}
	vop_repeat(std::string idv,const vec3d& cells, VNode* chs) : VSdfOperator(idv, { chs }), mCells(cells),mAxis({true,true,true}) {}
	vop_repeat(std::string idv,const vec3d& cells,const vec<bool,3>& axis, VNode* chs) : VSdfOperator(idv, { chs }), mCells(cells),mAxis(axis) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mCells = try_strToVec_d(entry->try_get("cells"),mCells);
        mAxis = try_strToVec_b(entry->try_get("axis"),mAxis);
	}

	inline const char* Type(){return "vop_repeat";}

	inline void eval(const vec3d& p,VResult& res) {
	    if (mChilds.empty()) { return; }
		vec3d ep  = transform_point_inverse(mFrame, p);
		//eval_ep(ep);


		auto hs = (0.5*mCells);
        auto mep = gl_mod(ep+hs,mCells)-hs;

        if(mAxis.x) ep.x = mep.x;
        if(mAxis.y) ep.y = mep.y;
        if(mAxis.z) ep.z = mep.z;

        ep  = transform_point(mFrame, ep);
        mChilds[0]->eval(ep,res);

		res.wor_pos = p;
		auto dspm = eval_displacement(ep);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
	}
};

struct vop_invert : public VSdfOperator {
	vop_invert(std::string idv, VNode* chs) : VSdfOperator(idv, { chs }){}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
	}

	inline const char* Type(){return "vop_invert";}

	inline void eval(const vec3d& p,VResult& res) {
	    if (mChilds.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

		mChilds[0]->eval(p,res);
		res.wor_pos = p;

		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		res.dist *=-1;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		res.vdist *=-1;
		//res.vdist *= sfct;
	}
};



struct vop_onion : public VSdfOperator {
    double mThickness = 0.2;
    bool mPreserveVolume = true;

	vop_onion(std::string idv, VNode* chs) : VSdfOperator(idv, { chs }), mThickness(0.2),mPreserveVolume(true) {}
	vop_onion(std::string idv, double thickness, VNode* chs) : VSdfOperator(idv, { chs }), mThickness(thickness),mPreserveVolume(true){}
	vop_onion(std::string idv, double thickness,bool preserve, VNode* chs) : VSdfOperator(idv, { chs }), mThickness(thickness),mPreserveVolume(preserve){}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mThickness = try_strtod(entry->try_get("thickness"),mThickness);
        mPreserveVolume = try_strtob(entry->try_get("preserve"),mPreserveVolume);
	}

	inline const char* Type(){return "vop_onion";}

	inline void eval(const vec3d& p,VResult& res) {
	    if (mChilds.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

        mChilds[0]->eval(p,res);


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
		res.dist += dspm-mRounding;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
	}
};

struct vop_cut : public VSdfOperator {
    vec3i mAxis = {0,1,0};
    vec3d mOffset = zero3d;
    double mBlendFactor = 0.0;

	vop_cut(std::string idv, VNode* chs) : VSdfOperator(idv, { chs }),mAxis({0,1,0}),mOffset(zero3d),mBlendFactor(0.0) {}
	vop_cut(std::string idv,const vec3i& axis,const vec3d& offset,double bfactor, VNode* chs) : VSdfOperator(idv, { chs }),mAxis(axis),mOffset(offset),mBlendFactor(bfactor) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mAxis = try_strToVec_i(entry->try_get("axis"),mAxis);
        mOffset = try_strToVec_d(entry->try_get("offset"),mOffset);
        mBlendFactor = try_strtod(entry->try_get("blend"),mBlendFactor);
	}

	inline const char* Type(){return "vop_cut";}

	inline void eval(const vec3d& p,VResult& res) {
	    if (mChilds.empty()) { return; }
		//ygl::vec3f ep = p;
		//eval_ep(ep);

        mChilds[0]->eval(p,res);
        auto epe = transform_point_inverse(mFrame, p+mOffset);

        auto dist = res.dist;
        auto vdist = res.vdist;
        if(mBlendFactor<thrd){

            if(mAxis.x!=0){
                double f = mAxis.x*epe.x;
                dist = std::max(dist,f);
                vdist = std::max(vdist,f);
            }
            if(mAxis.y!=0){
                double f = mAxis.y*epe.y;
                dist = std::max(dist,f);
                vdist = std::max(vdist,f);
            }
            if(mAxis.z!=0){
                double f = mAxis.z*epe.z;
                dist = std::max(dist,f);
                vdist = std::max(vdist,f);
            }
        }else{
            if(mAxis.x!=0){
                double f = mAxis.x*epe.x;
                dist = smax(dist,f,mBlendFactor);
                vdist = smax(vdist,f,mBlendFactor);
            }
            if(mAxis.y!=0){
                double f = mAxis.y*epe.y;
                dist = smax(dist,f,mBlendFactor);
                vdist = smax(vdist,f,mBlendFactor);
            }
            if(mAxis.z!=0){
                double f = mAxis.z*epe.z;
                dist = smax(dist,f,mBlendFactor);
                vdist = smax(vdist,f,mBlendFactor);
            }
        }
        res.dist = dist;
        res.vdist = vdist;

		res.wor_pos = p;
		auto dspm = eval_displacement(p);
		//auto sfct = min_element(scale);
		res.dist += dspm-mRounding;
		//res.dist *= sfct;

		res.vdist += dspm-mRounding;
		//res.vdist *= sfct;
	}
};



struct vop_extrude : public VSdfOperator{
    double mH = 1.0;

    vop_extrude(std::string idv, VNode* chs,double h) : VSdfOperator(idv, { chs }),mH(h) {}

	inline void DoRelate(const VMappedEntry* entry){
        VNode::DoRelate(entry);
        mH = try_strtod(entry->try_get("h"),mH);
	}

	inline const char* Type(){return "vop_extrude";}

	inline void eval(const vec3d& p,VResult& res) {
	    if (mChilds.empty()) { return; }
	    const auto mch = mChilds[0];
	    mch->eval(p,res);

        auto dspm = eval_displacement(p);

        vec2d wv = vec2d{res.dist,abs(p.z)-mH};
        res.dist = min(max(wv.x,wv.y),0.0)+ length(max(wv,zero2d))+dspm-mRounding;
        wv.x = res.vdist;
        res.vdist = min(max(wv.x,wv.y),0.0)+ length(max(wv,zero2d))+dspm-mRounding;

        res.wor_pos = p;
	}
};
/*
struct vop_revolve : public VSdfOperator{
    double mO = 1.0;

    vop_revolve(std::string idv, VNode* chs,double o) : VSdfOperator(idv, { chs }),mO(h) {}

	inline void DoRelate(const VMappedEntry* entry){
        set_translation(try_strToVec_d(entry->try_get(1),translation));
        set_rotation_degs(try_strToVec_d(entry->try_get(2),vec3d{rotation.x,rotation.y,rotation.z}));
        set_scale(try_strToVec_d(entry->try_get(3),scale));
        set_rotation_order(try_strToRotationOrder(entry->try_get(4),rotation_order));
        mO = try_strtod(entry->try_get(5),mO);
	}

	inline const char* Type(){return "vop_revolve";}

	inline inline void eval(const vec3d& p,VResult& res) {
	    if (mChilds.empty()) { return; }
	    const auto mch = mChilds[0];
	    const vec2d q = {length(vec2d{p.x,p.z}-mO,p.y};
	    mch->eval({q.x,q.y,p.z},res);

        auto dspm = eval_displacement(p);

        res.dist+=dspm-mRounding;
        res.vdist+=dspm-mRounding;

        res.wor_pos = p;
	}
};
*/



#endif
