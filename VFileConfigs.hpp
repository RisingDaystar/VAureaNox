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

        struct VFCException : public VException{
            VFCException(std::string msg):VException(msg),bFatal(true){}
            VFCException(bool fatal,std::string msg):VException(msg),bFatal(fatal){}
            bool bFatal = true;
        };

        std::map<std::string,std::map<std::string,std::string>> mMappings;
        std::string mFilename;

        VFileConfigs(std::string fn){
            mFilename = fn;
        }

        void parse();
        void eval(const std::string& line,int lid,std::string& cur_section);

        inline void ExceptionAtLine(const std::string& msg,int lid,bool fatal = true){
            throw VFCException(fatal,msg+" | line : "+std::to_string(lid));
        }
    };

};

#endif // VFILECONFIGS_HPP_INCLUDED
