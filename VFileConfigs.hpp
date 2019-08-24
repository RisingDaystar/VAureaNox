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

namespace vnx{

    struct VFileConfigs{
        std::map<std::string,std::map<std::string,std::string>> mappings;

        void eval(const std::string& line);
    };

};

#endif // VFILECONFIGS_HPP_INCLUDED
