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

#include "VConfigurable.hpp"

namespace vnx{

    VConfigurable::VConfigurable(const std::string& f) {mFilename = f;}

    void VConfigurable::print() {
        printf("*%s parsed : \n", mFilename.c_str());
        for (auto const& pms : mParams) {
            printf("\t%s : %s\n", pms.first.c_str(), pms.second.c_str());
        }
        printf("\n");
    }

    void VConfigurable::parse() {
        if (mFilename == "") { throw VException("Invalid config filename."); }
        std::ifstream in;
        std::string line;
        in.open(mFilename.c_str(), std::ifstream::in);
        if(!in.is_open()) {
            std::string str = "Could not open config file: ";
            str.append(mFilename.c_str());
            throw VException(str);
        }

        int lid=0;
        while (std::getline(in, line)) {
            lid++;
            try {eval(line,lid,mParams);}
            catch (VException& ex) { printf("Config parser {%s}: %s\n", mFilename.c_str(), ex.what()); continue; }
        }
        in.close();
    }

    void VConfigurable::eval(const std::string& line,int lid,std::map<std::string, std::string>& params) {
        if (line.empty() || line[0] == ';') { return; }
        int stage = 0;
        std::string name = "", value = "";
        for (std::string::size_type i = 0; i < line.length(); i++) {
            if (line[i] == ';') { break; }
            if (line[i] == ':') { stage = 1; continue; }
            if (line[i] == ' ' || line[i] == '\t') { continue; }
            if (stage == 0) { name += line[i]; }
            else if (stage == 1) { value += line[i]; }
        }
        if (stage != 1) {
            std::string msg("Syntax error at line ");
            msg+=std::to_string(lid);
            throw VException(msg);
        }

        params.insert(std::pair<std::string, std::string>(name, value));
    }

}
