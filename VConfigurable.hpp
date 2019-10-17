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

#ifndef VCONFIGURABLE_HPP_INCLUDED
#define VCONFIGURABLE_HPP_INCLUDED

#include "VFileConfigs.hpp"

namespace vnx {
	struct VConfigurable {
		VConfigurable(const VFileConfigs& configref, const std::string& section);

		const VFileConfigs& mConfigsRef;
		std::string mSection;

		inline std::string TryGet(const std::string& k, std::string dVal) const {
			return mConfigsRef.TryGet(mSection, k, dVal);
		}
		inline double TryGet(const std::string& k, double dVal) const {
			return mConfigsRef.TryGet(mSection, k, dVal);
		}
		inline float TryGet(const std::string& k, float dVal) const {
			return mConfigsRef.TryGet(mSection, k, dVal);
		}
		inline int TryGet(const std::string& k, int dVal) const { //also bool
			return mConfigsRef.TryGet(mSection, k, dVal);
		}

		inline std::string TryGet(const std::string& s, const std::string& k, std::string dVal) const {
			return mConfigsRef.TryGet(s, k, dVal);
		}
		inline double TryGet(const std::string& s, const std::string& k, double dVal) const {
			return mConfigsRef.TryGet(s, k, dVal);
		}
		inline float TryGet(const std::string& s, const std::string& k, float dVal) const {
			return mConfigsRef.TryGet(s, k, dVal);
		}
		inline int TryGet(const std::string& s, const std::string& k, int dVal) const { //also bool
			return mConfigsRef.TryGet(s, k, dVal);
		}
	};
}

#endif // VCONFIGURABLE_HPP_INCLUDED
