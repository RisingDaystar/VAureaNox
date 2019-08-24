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

#include "VSceneParser.hpp"

namespace vnx{

    void VSceneParser::parse(VScene& scn,const std::string& fname){
        std::map<std::string,VEntry> data;
        if (fname == "") { throw VException("Invalid scene filename."); }
        std::ifstream in;
        std::string line;
        in.open(fname.c_str(), std::ifstream::in);
        if(!in.is_open()) {
            std::string str = "Could not open scene file: ";
            str.append(fname.c_str());
            throw VException(str);
        }

        int lid=0;
        std::string scn_name = "";
        std::string root_id = "";
        while (std::getline(in, line)) {
            lid++;
            try {eval_line(line,scn_name,root_id,data);}
            catch (const VException& ex) {
                std::string str = "Scene loading error--> ";
                str+=std::string(ex.what())+" at line : "+std::to_string(lid);
                throw VException(str);
            }
        }
        in.close();

        try{
            link(scn,root_id,data,"!vmaterial"); //alloco in maniera ricorsiva , se non è collegato alla root, non è necessario instanziarlo a meno che non sia un materiale.
            if(data.find("##camera##")==data.end()){throw VException("Camera is undefined");}
            scn.camera.Relate(&data["##camera##"]);
            scn.root = static_cast<VNode*>(data[root_id].ptr);
            scn.id = scn_name;

        }catch(const VException& ex){
            std::string str = "Scene loading error--> ";
            str+=ex.what();
            throw VException(str);
        }
    }

    void VSceneParser::eval_line(std::string line,std::string& scn_name,std::string& root_id,std::map<std::string,VEntry>& data){
        if (line.empty() || line[0] == ';') { return; }
        int mode = 0;
        std::string::size_type i = 0;

        bool closed = false;

        if(line[0] == '#') {mode = -1;i=1;closed = true;}
        else if(line[0] == '@') {mode = -2;i=1;closed = true;}

        std::string type_name = "";
        std::string id = "";
        int args_index = -1;
        bool grouped = false;


        for(;i<line.size();i++){

            char c = line[i];
            if(c==' ' || c=='\t' || c=='"' || c=='[' || c==']') continue;

            if(mode==-1) {
                if(isControlCH(c)) break;
                if(!std::isalnum(static_cast<unsigned char>(c)) && c!='_') continue;
                scn_name += c;
            }else if(mode==-2){
                if(isControlCH(c)) break;
                if(!std::isalnum(static_cast<unsigned char>(c)) && c!='_') continue;
                root_id += c;
            }else if(mode==0){
                if(isControlCH(c)) throw VException("Illegal syntax, unexpected control character");
                if(c==')') {
                    if(grouped) throw VException("Illegal syntax, expected \"}\" before \")\"");
                    if(!nextTokenIsControlCH(line,i)){
                        throw VException("Illegal syntax, \")\" is not last token");
                    }
                    closed = true;
                    break;
                }
                if(c==',' && args_index==-1) throw VException("Illegal syntax. Unexpected \",\"");
                if(c==',' && args_index==0) {
                    if(id.empty()){throw VException("ID is empty, unexpected \",\"");}
                    data[id] = VEntry({type_name,""});
                    args_index++;
                    continue;
                } //creo entry nella map con key=id e creo il vector con [0]=type_name
                if(c==','){
                    if(!grouped){data[id].add_token("");args_index++;continue;}//passo ad index++
                }
                if(args_index==-1) { //index-1 deve essere il tipo
                    if(c=='('){args_index++;continue;} //passo ad index0
                    type_name+=c;
                }else if (args_index==0){ //index0 deve essere l'id
                    if(type_name.empty()) throw VException("\"Type_name\" is empty");
                    if(type_name=="vcamera"){id="##camera##";data[id] = VEntry({type_name,""});args_index++;data[id].at(args_index)+=c;continue;}
                    id += c;
                }else{
                    if(c=='{' && !grouped)grouped = true;
                    else if(c=='{' && grouped) throw VException("Illegal syntax, Unexpected \"{\"");
                    else if(c=='}' && grouped)grouped = false;
                    else if(c=='}' && !grouped) throw VException("Illegal syntax, Unexpected \"}\"");
                    data[id].at(args_index) += c;
                }
            }
        }

        if(!closed) throw VException("Illegal syntax, missing \")\"");

    }

    void VSceneParser::link(VScene& scn,std::string id,std::map<std::string,VEntry>& data,std::string enforceType){
        if(data.find(id)==data.end()){throw VException("Undefined reference to "+id);}

        VEntry* entry = &data[id];

        if(enforceType!=""){
            if(enforceType[0]=='!'){
                enforceType.erase(0,1);
                if(enforceType!="" && stricmp(entry->at(0),enforceType)){throw VException("Reference \""+id+"\" is of type \""+enforceType+"\"");}
            }else{
                if(!stricmp(entry->at(0),enforceType)){throw VException("Reference \""+id+"\" is not of type \""+enforceType+"\"");}
            }
        }

        if(entry->allocated()) return;
        auto type = entry->at(0);

        //TODO VBsdf
        if(type=="vmaterial"){
            scn.materials[id] = VMaterial();
            scn.materials[id].Relate(entry);
        }else if(stricmp(type,std::string("vop_union")) ||
                 stricmp(type,std::string("vop_intersection")) ||
                 stricmp(type,std::string("vop_subtraction")) ||
                 stricmp(type,std::string("vop_cut")) ||
                 stricmp(type,std::string("vop_twist")) ||
                 stricmp(type,std::string("vop_invert")) ||
                 stricmp(type,std::string("vop_repeat")) ||
                 stricmp(type,std::string("vop_onion"))
                 ){ //OPERATORI
            auto chs = entry->at(entry->size()-1);
            if(chs=="") return;
            auto chs_vec = strDeGroup(chs);
            std::vector<VNode*> nodes;
            for(auto ch : chs_vec){
                link(scn,ch,data);
                nodes.push_back(static_cast<VNode*>(data[ch].ptr));
            }
            VNode* e = nullptr;

            if(stricmp(type,std::string("vop_union"))) e = new vop_union(id,nodes);
            else if(stricmp(type,std::string("vop_intersection"))) e = new vop_intersection(id,nodes);
            else if(stricmp(type,std::string("vop_subtraction"))) e = new vop_subtraction(id,nodes);
            else if(stricmp(type,std::string("vop_cut"))) e = new vop_cut(id,nodes.empty() ? nullptr : nodes[0]);
            else if(stricmp(type,std::string("vop_twist"))) e = new vop_twist(id,nodes.empty() ? nullptr : nodes[0]);
            else if(stricmp(type,std::string("vop_invert"))) e = new vop_invert(id,nodes.empty() ? nullptr : nodes[0]);
            else if(stricmp(type,std::string("vop_repeat"))) e = new vop_repeat(id,nodes.empty() ? nullptr : nodes[0]);
            else if(stricmp(type,std::string("vop_onion"))) e = new vop_onion(id,nodes.empty() ? nullptr : nodes[0]);

            //if(!e) throw VException("Unerecognized Operator \""+type+"\""); //NON NECESSARIO

            e->Relate(entry);
            scn.nodes.push_back(e);
        }else{ //VOLUMI
            auto mtl_id = entry->try_at(1);
            link(scn,mtl_id,data,"vmaterial");

            auto mtl = static_cast<VMaterial*>(data[mtl_id].ptr);

            VNode* e = nullptr;

            if(stricmp(type,std::string("vvo_sd_sphere"))) e = new vvo_sd_sphere(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_box"))) e = new vvo_sd_box(id,mtl);
            //else if(stricmp(type,"vvo_shadered")) e = new vvo_shadered(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_plane"))) e = new vvo_sd_plane(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_ellipsoid"))) e = new vvo_sd_ellipsoid(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_cylinder"))) e = new vvo_sd_cylinder(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_capsule"))) e = new vvo_sd_capsule(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_hex_prism"))) e = new vvo_sd_hex_prism(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_tri_prism"))) e = new vvo_sd_tri_prism(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_capped_cone"))) e = new vvo_sd_capped_cone(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_pyramid4"))) e = new vvo_sd_pyramid4(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_diamond"))) e = new vvo_sd_diamond(id,mtl);

            if(e==nullptr) throw VException("Unerecognized Entity \""+type+"\"");

            e->Relate(entry);
            scn.nodes.push_back(e);
        }
    }

};
