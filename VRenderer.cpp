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

#include "VRenderer.hpp"
#include "VScene.hpp"

namespace vnx {
	void VRenderer::_cpu_task(uint id, const VScene& scn, VRenderer* renderer, std::mutex& mtx, VRng& rng, std::atomic<uint>& lrow, std::atomic<uint>& rowCounter) {
		const uint width = scn.mCamera.mResolution.x;
		const uint height = scn.mCamera.mResolution.y;

		while (!renderer->mStatus.bStopped) {
			//sincronizzazione per evitare data race...insignificante overhead
			mtx.lock();
			uint j = lrow;
			lrow++;
			if (j >= height) {
				mtx.unlock(); break;
			}
			mtx.unlock();

			renderer->EvalImageRow(scn, rng, width, height, j);

			mtx.lock();
			std::cout << "Current Row:" << j << " -> Total:{" << (++rowCounter) << " / " << height << "} -> Worker:{" << id << "}\n";
			mtx.unlock();
		}
		mtx.lock();
		std::cout << "\t#Worker " << id << " finished\n";
		mtx.unlock();
	}
};
