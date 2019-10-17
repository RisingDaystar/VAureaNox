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

#ifndef VSCENE_HPP_INCLUDED
#define VSCENE_HPP_INCLUDED

#include "VAureaNox.hpp"
#include "VCamera.hpp"

namespace vnx {
	struct VScene {
		std::string mID = "";
		std::vector<std::vector<VResult>> mEmissiveHints;
		std::map<std::string, VMaterial> mMaterials;
		std::vector<VNode*> mNodes;

		VScene() :VScene("") {}

		VScene(std::string idv) :mID(idv) {}
		typedef VResult(VScene::* st_algo_ftor)(const VRay&, int, int*, int*) const;
		typedef vec3d(VScene::* normals_algo_ftor)(const VResult& er, double eps) const;
		typedef vec3i(VScene::* eh_precalc_algo_ftor)(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame);

		st_algo_ftor intersect_algo = &VScene::intersect_naive;
		normals_algo_ftor normals_algo = &VScene::eval_normals_tht;
		eh_precalc_algo_ftor eh_precalc_algo = &VScene::precalc_emissive_hints_full;

		VNode* root = nullptr;
		VCamera camera;

		void shut();

		VMaterial* add_material(std::string id);
		VMaterial* add_material(std::string id, VMaterial mtl);

		bool set_translation(const std::string& idv, const vec3d& amount);
		bool set_translation(const std::string& idv, double x, double y, double z);

		bool set_rotation(std::string n, const vec3d& amount);
		bool set_rotation(std::string n, double x, double y, double z);

		bool set_rotation_degs(const std::string& idv, const vec3d& amount);
		bool set_rotation_degs(const std::string& idv, double x, double y, double z);

		bool set_rotation_order(const std::string& idv, VRotationOrder ro);

		bool set_scale(const std::string& idv, const vec3d& amount);
		bool set_scale(const std::string& idv, double x, double y, double z);
		bool set_scale(const std::string& idv, double amount);

		bool set_rounding(const std::string& idv, double r);

		bool set_displacement(const std::string idv, displ_ftor ftor);

		inline VResult eval(const vec3d& p) const {
			VResult res;
			if (root != nullptr) { root->eval(p, res); res.wor_pos = p; if (res.vdist > 0.0) { res.vmtl = nullptr; res.vsur = nullptr; } } //ensure wor_pos is set to world coord
			return res;
		}
		inline void eval(const vec3d& p, VResult& res) const {
			if (root != nullptr) { root->eval(p, res); res.wor_pos = p; if (res.vdist > 0.0) { res.vmtl = nullptr; res.vsur = nullptr; } } // ensure wor_pos is set to world coord
		}


		inline VNode* set_root(VNode* n) { root = n; return root; }
		inline VNode* get_root() { return root; }

		inline VNode* select(const std::string& idv) {
			if (root == nullptr) { return nullptr; }
			if (root->mID == idv) { return root; }
			return root->select(idv);
		}



		inline vec3d eval_normals(const VResult& er, double eps) const {
			return normalize(std::invoke(normals_algo, this, er, eps));
		}
		inline vec3d eval_unnormalized_normals(const VResult& er, double eps) const {
			return std::invoke(normals_algo, this, er, eps);
		}
		inline VResult intersect(const VRay& ray, int nm, int* iters = nullptr, int* overs = nullptr) const {
			return std::invoke(intersect_algo, this, ray, nm, iters, overs);
		}
		inline vec3i precalc_emissive_hints(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame) {
			return std::invoke(eh_precalc_algo, this, rng, emap, ptr, n_em_e, n_max_iters, tmin, tmax, neps, verbose, parent_frame);
		}

		inline vec3d eval_normals_tht(const VResult& vre, double eps) const {
			constexpr vec3d nv1 = { 1.0, -1.0, -1.0 };
			constexpr vec3d nv2 = { -1.0, -1.0, 1.0 };
			constexpr vec3d nv3 = { -1.0, 1.0, -1.0 };
			constexpr vec3d nv4 = { 1.0, 1.0, 1.0 };
			const auto p = vre.wor_pos;
			VResult res;
			root->eval(p + nv1 * eps, res);
			auto n = nv1 * res.dist;
			root->eval(p + nv2 * eps, res);
			n += nv2 * res.dist;
			root->eval(p + nv3 * eps, res);
			n += nv3 * res.dist;
			root->eval(p + nv4 * eps, res);
			n += nv4 * res.dist;
			return n;
		}

		inline vec3d eval_normals_cnt(const VResult& vre, double eps) const {
			const auto p = vre.wor_pos;
			VResult res;

			vec3d norm = zero3d;
			root->eval(p + vec3d{ eps,0,0 }, res);
			norm.x = res.dist;
			root->eval(p - vec3d{ eps,0,0 }, res);
			norm.x -= res.dist;
			root->eval(p + vec3d{ 0,eps,0 }, res);
			norm.y = res.dist;
			root->eval(p - vec3d{ 0,eps,0 }, res);
			norm.y -= res.dist;
			root->eval(p + vec3d{ 0,0,eps }, res);
			norm.z = res.dist;
			root->eval(p - vec3d{ 0,0,eps }, res);
			norm.z -= res.dist;

			return norm;
		}


		///original naive sphere tracing algorithm
		inline VResult intersect_naive(const VRay& ray, int miters, int* iters = nullptr, int* overs = nullptr) const {
			double t = 0.0;
			VResult vre;
			int i = 0;
			for (i = 0; i < miters; i++) {
				eval(ray.o + (ray.d * t), vre);
				auto d = std::abs(vre.dist);

				if (d < ray.tmin) { vre._found = true; if (iters) { *iters = i; }if (overs) { *overs = 0; }return vre; }
				t += d;
				if (t > ray.tmax) { break; }
			}
			vre._found = false;
			if (iters) { *iters = i; }
			if (overs) { *overs = 0; }
			return vre;
		}

		///Hints about auto omega overrelaxing & fixed omega overrelaxing from (in order) :
		///http://www.fractalforums.com/3d-fractal-generation/enhanced-sphere-tracing-paper/
		///http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf
		///

		///Experimental Algos (rel, enh):
		///Can trace inside volumes using a double distance feedback ( "dist" & "vdist" ) , with dist being the absolute distance for primary marching/tracing
		///and vdist the propagated signed distance trough operators , to determine the "starting" sign of the tracing ray.
		///dist cannot be used directly for that purpose , because , in the operator, it takes (and then discards) the sign (and value) from the "absolute nearest" isosurface (uses std::abs),
		///this IS a problem for the purpose of both autofixing lipschitz and over relaxing the ray. Especially, when in need to trace inside arbitrary volumes in a scene,
		///it simply cannot determine wether it is an "error" or a legit inside trace.
		///TODO ,over rel & lipschitz fixer for "started inside" tracings



		inline VResult intersect_rel(const VRay& ray, int miters, int* iters = nullptr, int* overs = nullptr) const {
			//const double hmaxf = maxd/2.0; ///consider using this to avoid over/underflows (depending on sign of the leading expression, it might happen, needs testing) .
			constexpr double maxf_m1 = maxd - 1.0;
			double t = 0.0;
			double pd = maxf_m1;
			double os = 0.0;
			double s = 1.0;
			VResult vre;
			int i = 0;
			int no = 0;
			for (; i < miters; i++) {
				eval(ray.o + ray.d * t, vre);
				auto d = vre.dist;
				auto absd = std::abs(d);

				if (i == 0) { s = nz_sign(vre.vdist); }
				if (s < 0) { //started inside
					if (absd < ray.tmin) { vre._found = true; if (iters) { *iters = i; }if (overs) { *overs = no; }return vre; }
					t += absd;
					pd = d;
				}
				else {  //started outside
					if (absd > os) {
						if (d < ray.tmin && d >= 0.0) { vre._found = true; if (iters) { *iters = i; }if (overs) { *overs = no; }return vre; }
						if (absd < ray.tmin) d -= ray.tmin - absd; //precision fix
						os = d * ygl::clamp(0.5 * std::max(d / pd, os / pd), -1.0, 1.0);
						t += d + os;
						pd = d;
					}
					else {
						const auto aos = std::abs(os);
						no++;
						t -= aos;
						pd = maxf_m1; //hmaxf
						os = -aos;
					}
				}
				if (std::abs(t) > ray.tmax) break;
			}
			vre._found = false;
			if (iters) { *iters = i; }
			if (overs) { *overs = no; }
			return vre;
		}

		inline VResult intersect_enh(const VRay& ray, int miters, int* iters = nullptr, int* overs = nullptr) const {
			constexpr double maxf_m1 = maxd - 1.0; //TODO : check all cases for over/underflows
			double t = 0.0;
			double pd = maxf_m1;
			double os = 0.0;
			double s = 1.0;

			VResult vre;
			int i = 0;
			int no = 0;
			for (; i < miters; i++) {
				eval(ray.o + ray.d * t, vre);

				if (i == 0) { s = nz_sign(vre.vdist); }
				if (s < 0) { //started inside
					auto d = vre.dist;  ///MARCH BY DIST INSIDE VOLUMES, NEEDED for nested volumes
					auto absd = std::abs(d);
					if (vre.vdist < 0.0 && absd < ray.tmin) {
						vre._found = true; if (iters) { *iters = i; }if (overs) { *overs = no; }return vre;
					}
					if (vre.vdist > 0.0 && vre.dist > 0.0) { //Precision fix
						t -= absd + (ray.tmin / 2.0);
						continue;
					}
					t += absd;
					//pd = d;
				}
				else {  //started outside
					auto d = vre.vdist; ///MARCH BY VDIST OUTSIDE, NO NEED TO ACCOUNT FOR NESTED
					///VDIST allows for precise back-tracing in case of oversteps and discontinuities
					auto absd = std::abs(d);
					if (absd > os) {
						if (d < ray.tmin && d >= 0.0) { vre._found = true; if (iters) { *iters = i; }if (overs) { *overs = no; }return vre; }
						if (d < 0.0) { //Precision && Discontinuities fix
							const auto cdd = std::min(1.0, std::max(std::abs(os / pd), std::abs(d / pd)));
							const auto step = (absd + (std::abs(os) + ray.tmin)) * cdd / (2.0);
							d += step;
							t -= step;
							os = 0.0;
							pd = d;
							continue;

						}

						//auto cc = std::abs((d-os)/pd)*std::max(std::abs(d/pd),std::abs(os/pd)) ///gives a strange miss in the cornell box scene (right in the middle, where ray is perpendicular to box), might be prone to undetected overstep
						auto cc = std::max(std::abs(d / pd), std::abs(os / pd));
						os = d * std::max(-cc, std::min(cc, 0.5 * (d / pd)));

						//os = d*std::max(-1.0,std::min(1.0,0.5*(d/pd)));
						t += d + os;
						pd = d;

					}
					else {
						no++;
						const auto aos = std::abs(os);
						t -= aos;
						pd = maxf_m1;
						os = -aos;
						//os=0.0;
					}
				}
				if (std::abs(t) > ray.tmax) break;
			}
			vre._found = false;
			if (iters) { *iters = i; }
			if (overs) { *overs = no; }
			return vre;
		}

		inline VResult sample_emissive(VRng& rng, uint32_t& idl) const {
			idl = 0;
			if (mEmissiveHints.empty()) return VResult();
			idl = rng.next_uint(mEmissiveHints.size());
			const std::vector<VResult>* ep = &mEmissiveHints[idl];
			if (ep->empty()) return VResult();
			return (*ep)[rng.next_uint(ep->size())];
		}

		inline VResult sample_emissive_in_light(VRng& rng, uint32_t idl) const {
			if (mEmissiveHints.empty()) return VResult();
			const std::vector<VResult>* ep = &mEmissiveHints[idl];
			if (ep->empty()) return VResult();
			return (*ep)[rng.next_uint(ep->size())];
		}

		vec3i precalc_emissive_hints_full(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame);
		vec3i precalc_emissive_hints_strict(VRng& rng, std::map<std::string, std::vector<VResult>>& emap, VNode* ptr, int n_em_e, int n_max_iters, double tmin, double tmax, double neps, bool verbose, frame3d parent_frame);

		vec3i populate_emissive_hints(int i_em_evals, int i_max_march_iterations, double f_ray_tmin, double f_ray_tmax, double f_normal_eps, bool verbose = false);
	};
};

#endif // VSCENE_HPP_INCLUDED
