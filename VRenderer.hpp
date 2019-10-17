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
	    VStatus mStatus;
		VRenderer(const VFileConfigs& cfgs,std::string section) : VConfigurable(cfgs,section){
		}

		virtual ~VRenderer() {Shut();}
		virtual void Shut() {}
		virtual void Init(VScene& scn) {};
		virtual void PostInit(VScene& scn) = 0;
		virtual std::string Type() const = 0;
		virtual std::string ImgAffix(const VScene& scn) const {return std::string("");};
		virtual void EvalImageRow(const VScene& scn, VRng& rng, uint width, uint height, uint j) = 0;
	};
};

#endif // VRENDERER_HPP_INCLUDED
