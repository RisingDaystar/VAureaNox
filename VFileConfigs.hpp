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

#ifndef VFILECONFIGS_HPP_INCLUDED
#define VFILECONFIGS_HPP_INCLUDED

#include "VAureaNox.hpp"

namespace vnx {
	struct VFileConfigs {
		struct VFCException : public VException {
			VFCException(const std::string& msg) :VException(msg), bFatal(true) {}
			VFCException(bool fatal, std::string msg) :VException(msg), bFatal(fatal) {}
			bool bFatal = true;
		};

		std::map<std::string, std::map<std::string, std::string>> mMappings;
		std::string mFilename;

		VFileConfigs(std::string fn) {
			mFilename = fn;
		}

		void parse();
		void eval(const std::string& line, uint lid, std::string& cur_section);

		inline const std::map<std::string, std::string>* getSection(const std::string& s) const {
			auto ms = mMappings.find(s);
			if (ms == mMappings.end()) return nullptr;
			return &ms->second;
		}

		inline void ExceptionAtLine(const std::string& msg, uint lid, bool fatal = true) {
			throw VFCException(fatal, msg + " | line : " + std::to_string(lid));
		}

		inline std::string TryGet(const std::string& section, const std::string& k, std::string dVal) const {
			auto ms = getSection(section);
			if (!ms) return dVal;
			auto match = ms->find(k);
			if (match == ms->end()) { return dVal; }
			return match->second;
		}
		inline double TryGet(const std::string& section, const std::string& k, double dVal) const {
			auto ms = getSection(section);
			if (!ms) return dVal;
			auto match = ms->find(k);
			if (match == ms->end()) { return dVal; }
			return strto_d(match->second);
		}
		inline float TryGet(const std::string& section, const std::string& k, float dVal) const {
			auto ms = getSection(section);
			if (!ms) return dVal;
			auto match = ms->find(k);
			if (match == ms->end()) { return dVal; }
			return strto_f(match->second);
		}
		inline int TryGet(const std::string& section, const std::string& k, int dVal) const { //also bool
			auto ms = getSection(section);
			if (!ms) return dVal;
			auto match = ms->find(k);
			if (match == ms->end()) { return dVal; }

			if (stricmp(match->second, std::string("true"))) { return 1; }
			if (stricmp(match->second, std::string("false"))) { return 0; }

			return strto_i(match->second);
		}
	};
};

#endif // VFILECONFIGS_HPP_INCLUDED
