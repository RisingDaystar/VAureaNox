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

#ifndef _VSCENEPARSER_H_
#define _VSCENEPARSER_H_

#include "VAureaNox.hpp"
#include "VScene.hpp"
#include "VSdfOperators.hpp"
#include "VSdfs.hpp"

namespace vnx{
    struct VSceneParser{

        static constexpr char mTchars[3] = {'\n','\t','\0'};

        inline bool isControlCH(char c){
            return c==mTchars[0] || c==mTchars[1] || c==mTchars[2];
        }

        inline bool nextTokenIsControlCH(const std::string& line, std::string::size_type i){
            if(i+1>line.size())return true;

            for(std::string::size_type n=i+1;n<line.size();n++){
                auto c = line[n];
                if(isControlCH(c) || c==';') break;
                if(c==' ') continue;
                return false;
            }
            return true;
        }

        void parse(VScene& scn,const std::string& fname);
        void eval_line(std::string line,std::string& scn_name,std::string& root_id,std::map<std::string,VEntry>& data);
        void link(VScene& scn,std::string id,std::map<std::string,VEntry>& data,std::string enforceType="");

    };
};
#endif
