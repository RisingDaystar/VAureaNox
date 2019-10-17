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

    void VFileConfigs::parse(){
        if (mFilename == "") { throw VFCException("Invalid config filename."); }
        std::ifstream in;
        std::string line;
        in.open(mFilename.c_str(), std::ifstream::in);
        if(!in.is_open()) {
            std::string str = "Could not open config file: ";
            str.append(mFilename.c_str());
            throw VFCException(str);
        }

        uint lid=0;
        std::string cur_section = "";
        while (std::getline(in, line)) {
            lid++;
            try {eval(line,lid,cur_section);}
            catch (VFCException& ex) {
                std::cout<<"Config parser {"<<mFilename.c_str()<<"}: "<<ex.what()<<"\n";
                if(ex.bFatal){cur_section.clear();break;}
            }
        }
        if(!cur_section.empty()) std::cout<<"Config parser {"<<mFilename.c_str()<<"}: Warning, section still open upon reaching EOF\n";
        in.close();
    }

    void VFileConfigs::eval(const std::string& line,uint lid,std::string& cur_section){
        std::string value = "";
        std::string name = "";
        int state = 0;
        for(std::string::size_type i=0;i<line.size();i++){
            if(line[i]==':'){
                if(cur_section.empty())ExceptionAtLine("Syntax Error-> Cannot define key/value while a tag is not opened",lid,false);
                if(state!=0)ExceptionAtLine("Syntax Error-> Unexpected \":\"",lid,false);
                if(state==0){state=1;continue;}
            }

            if(line[i] == ';') break;
            if(line[i] == ' ' || line[i]=='\n' || line[i]=='\r' || line[i]=='\t'){continue;}

            if(line[i]=='<' && cur_section.empty() && state == 0){
                if(line[i] == ' ' || line[i]=='\n' || line[i]=='\r' || line[i]=='\t'){continue;}
                if((i+1<line.size() && line[i+1]=='/'))ExceptionAtLine("Parse Error-> Illegal Section character \"\\",lid);
                bool valid = false;
                for(i++;i<line.size();i++){
                    if(line[i]=='>'){valid=true;break;}
                    cur_section += line[i];
                }
                if(!valid) ExceptionAtLine("Syntax Error-> Unclosed section opening tag",lid);
                if(mMappings.find(cur_section)!= mMappings.end()) ExceptionAtLine("Parse Error-> Duplicated section",lid);
                state = 0;
                continue;
            }

            if(line[i]=='<' && !cur_section.empty() && state == 0){
                if(line[i] == ' ' || line[i]=='\n' || line[i]=='\r' || line[i]=='\t'){continue;}
                if(!(i+1<line.size() && line[i+1]=='/'))ExceptionAtLine("Parse Error-> Expected closing tag",lid);
                std::string end_section = "";
                bool valid = false;
                for(i+=2;i<line.size();i++){
                    if(line[i]=='>'){valid=true;break;}
                    end_section += line[i];
                }
                if(!valid) ExceptionAtLine("Syntax Error-> Unclosed section closing tag",lid);
                if(end_section!=cur_section) ExceptionAtLine("Parse Error-> Unmatching closing tag",lid);
                cur_section.clear();
                state = 0;
                continue;
            }

            if(state==0){name+=line[i];}
            else if(state==1){value+=line[i];}
        }

        if(!cur_section.empty()){
            if(!name.empty() && !value.empty()) mMappings[cur_section][name] = value;
        }
    }

};
