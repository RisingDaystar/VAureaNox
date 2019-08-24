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

#include "VAureaNox.hpp"

namespace vnx{

    struct VConfigurable {
        VConfigurable(const std::string& f);

        std::map<std::string, std::string> mParams;
        std::string mFilename;

        void print();
        void parse();
        void eval(const std::string& line,int lid,std::map<std::string, std::string>& params);

        inline std::string TryGet(std::string k, std::string dVal) const {
            auto match = mParams.find(k);
            if (match == mParams.end()) { return dVal; }
            return match->second;
        }
        inline double TryGet(std::string k, double dVal) const {
            auto match = mParams.find(k);
            if (match == mParams.end()) { return dVal; }
            return strto_d(match->second);
        }
        inline float TryGet(std::string k, float dVal) const {
            auto match = mParams.find(k);
            if (match == mParams.end()) { return dVal; }
            return strto_f(match->second);
        }
        inline int TryGet(std::string k, int dVal) const { //also bool
            auto match = mParams.find(k);
            if (match == mParams.end()) { return dVal; }

            if (stricmp(match->second, std::string("true"))) { return 1; }
            if (stricmp(match->second, std::string("false"))) { return 0; }

            return strto_i(match->second);
        }
    };
}

#endif // VCONFIGURABLE_HPP_INCLUDED
