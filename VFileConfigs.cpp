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

#include "VFileConfigs.hpp"

namespace vnx{

    void VFileConfigs::eval(const std::string& line){
        std::string value = "";
        std::string name = "";
        std::string section = "";
        int state = 0;
        for(std::string::size_type i=0;i<line.size();i++){
            if(line[i]==':' && state==0){state = 1;continue;}
            if(line[i]==':' && state!=0){break;}
            if(line[i] == ';') break;
            if(line[i] == ' ' || line[i]=='\n' || line[i]=='\r'){continue;}
            if(line[i]=='#' && state==0){state = -1;continue;}

            if(state==-1){section+=line[i];}
            else if(state==0){name+=line[i];}
            else if(state==1){value+=line[i];}
        }

        if(mappings.find(section)!=mappings.end()) return; // sezione duplicata

        if(!section.size() || !name.size() || !value.size()) throw new VException("Badly formatted config file.");
        mappings[section][name] = value;
    }

};
