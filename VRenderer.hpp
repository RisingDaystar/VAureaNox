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

#ifndef VRENDERER_HPP_INCLUDED
#define VRENDERER_HPP_INCLUDED

#include "VConfigurable.hpp"

namespace vnx {
	struct VRenderer : VConfigurable {
		VStatus mStatus;
	private:
		std::mutex mMutex;
		std::atomic<uint> mLastRow;
		std::atomic<uint> mRowCounter;

		std::vector<std::thread*> mThreads;
		std::vector<VRng> mRngs;
	protected:
		bool mOneShotRender = false;
	public:

		static void _cpu_task(uint id, const VScene& scn, VRenderer* renderer, std::mutex& mtx, VRng& rng, std::atomic<uint>& lrow, std::atomic<uint>& rowCounter);

		inline std::mutex& get_mutex() { return mMutex; }

		inline void Process(const VScene& scn) {
			if (mOneShotRender) {
				mRngs.push_back(VRng());
				const auto tt = get_time();
				mRngs[0].seed(tt, tt, 0, 0);
				EvalImage_OS(scn, mRngs[0]);
			} else {
				unsigned int i_max_threads = mConfigsRef.TryGet(mSection, "i_max_threads", 0);
				const unsigned int nThreads = i_max_threads == 0 ? std::thread::hardware_concurrency() : std::min(std::thread::hardware_concurrency(), i_max_threads);
				if (nThreads > 1) {
					mThreads.resize(nThreads, nullptr);
					mRngs.resize(nThreads, VRng());
					const auto tt = get_time();
					for (uint n = 0; n < nThreads; n++) {
						mRngs[n].seed(tt, tt, n + n, n + n + 1);
						//mRngs[n].seed(tt,n+1); //pcg32 | splitmix
						//mRngs[n].seed(tt); //xoroshiro
						//mRngs[n].long_jump(); //xoroshiro
					}
					for (uint i = 0; i < nThreads; i++) {
						mThreads[i] = new std::thread(VRenderer::_cpu_task, i, std::ref(scn), this, std::ref(mMutex), std::ref(mRngs[i]), std::ref(mLastRow), std::ref(mRowCounter));
					}
					for (uint i = 0; i < nThreads; i++) { if (mThreads[i] && mThreads[i]->joinable()) { mThreads[i]->join(); } }
					for (uint i = 0; i < nThreads; i++) { if (mThreads[i]) { delete mThreads[i]; } mThreads[i] = nullptr; }
				} else {
					mRngs.push_back(VRng());
					const auto tt = get_time();
					mRngs[0].seed(tt, tt, 0, 0);
					_cpu_task(0, scn, this, mMutex, mRngs[0], mLastRow, mRowCounter);
				}
			}
			mThreads.clear();
			mRngs.clear();
		}

		VRenderer(const VFileConfigs& cfgs, std::string section) : VConfigurable(cfgs, section), mOneShotRender(false) {
			mLastRow.store(0);
			mRowCounter.store(0);
		}

		virtual ~VRenderer() { Shut(); }
		virtual void Shut() {}
		virtual void Init(VScene& scn) {};
		virtual void PostInit(VScene& scn) = 0;
		virtual std::string Identifier() const = 0;
		virtual std::string ImgAffix(const VScene& scn) const { return std::string(""); };

		virtual void EvalImage_OS(const VScene& scn, VRng& rng) {}
		virtual void EvalImageRow(const VScene& scn, VRng& rng, uint width, uint height, uint j) = 0;
	};
};

#endif // VRENDERER_HPP_INCLUDED
