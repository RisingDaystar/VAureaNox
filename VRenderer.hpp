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

namespace vnx{

	struct VRenderer : VConfigurable{
	    VConfigurable* mSharedCfg = nullptr;
	    VStatus mStatus;
		VRenderer(VConfigurable& shared_cfg,std::string cf) : VConfigurable(cf), mSharedCfg(&shared_cfg){
		}

		virtual ~VRenderer() {Shut();}
		virtual void Shut() {}
		virtual void Init();
		virtual void PostInit(VScene& scn) = 0;
		virtual std::string Type() const = 0;
		virtual std::string ImgAffix(const VScene& scn) const {return std::string("");};
		virtual void EvalImageRow(const VScene& scn, ygl::rng_state& rng, image3d& img, int width,int height, int j) = 0;
	};
};

#endif // VRENDERER_HPP_INCLUDED
