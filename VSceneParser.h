#ifndef _VSCENEPARSER_H_
#define _VSCENEPARSER_H_

#include "VOperators.h"
#include "VVolumes.h"

namespace vnx{
    struct VSceneParser{

        inline void parse(VScene& scn,const std::string& fname){
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
                catch (VException& ex) { printf("Scene parser {%s}: %s\n", fname.c_str(), ex.what()); continue; }
            }
            in.close();

            try{
                link(scn,root_id,data,"!vmaterial"); //alloco in maniera ricorsiva , se non è collegato alla root, non è necessario instanziarlo a meno che non sia un materiale.
                if(data.find("##camera##")==data.end()){throw VException("Camera is undefined");}
                scn.camera.Relate(&data["##camera##"]);
                scn.root = static_cast<VNode*>(data[root_id].ptr);
                scn.id = scn_name;

            }catch(VException& ex){
                for(auto& e : data){
                    auto entry = e.second;
                    if(entry.ptr!=nullptr) delete entry.ptr;
                }
                std::string str = "Scene loading error: ";
                str+=ex.what();
                throw VException(str);
            }
        }

        inline void eval_line(std::string line,std::string& scn_name,std::string& root_id,std::map<std::string,VEntry>& data){
            if (line.empty() || line[0] == ';') { return; }
            int mode = 0;
            int i = 0;
            if(line[0] == '#') {mode = -1;i=1;}
            else if(line[0] == '@') {mode = -2;i=1;}

            std::string type_name = "";
            std::string id = "";
            int args_index = -1;
            bool grouped = false;

            for(;i<line.size();i++){

                char c = line[i];
                if(c=='\n' || c=='\r' || c=='\0') break;
                if(c==' ' || c=='\t') continue;
                if(c=='"') continue;

                if(mode==-1) {
                    scn_name += c;
                }else if(mode==-2){
                    root_id += c;
                }else if(mode==0){
                    if(c==')') break;
                    if(c==',' && args_index==-1) throw VException("Illegal syntax");
                    if(c==',' && args_index==0) {data[id] = VEntry({type_name,""});args_index++;continue;} //creo entry nella map con key=id e creo il vector con [0]=type_name
                    if(c==','){
                        if(!grouped){data[id].add_token("");args_index++;continue;}//passo ad index++
                    }
                    if(args_index==-1) { //index-1 deve essere il tipo
                        if(c=='('){args_index++;continue;} //passo ad index0
                        type_name+=c;
                    }else if (args_index==0){ //index0 deve essere l'id
                        if(type_name.empty()) throw VException("Type_name is empty");
                        if(type_name=="vcamera"){id="##camera##";data[id] = VEntry({type_name,""});args_index++;data[id].at(args_index)+=c;continue;}
                        id += c;
                    }else{
                        if(c=='{' && !grouped)grouped = true;
                        else if(c=='{' && grouped) throw VException("Illegal syntax");
                        else if(c=='}' && grouped)grouped = false;
                        else if(c=='}' && !grouped) throw VException("Illegal syntax");
                        data[id].at(args_index) += c;
                    }
                }
            }

        }

        inline void link(VScene& scn,std::string id,std::map<std::string,VEntry>& data,std::string enforceType=""){
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

            if(type=="vmaterial"){
                auto e = new VMaterial(id);
                e->Relate(entry);
                //TODO add to scn.materials
            }else if(stricmp(type,"vop_union") ||
                     stricmp(type,"vop_intersection") ||
                     stricmp(type,"vop_subtraction") ||
                     stricmp(type,"vop_cut") ||
                     stricmp(type,"vop_twist") ||
                     stricmp(type,"vop_invert") ||
                     stricmp(type,"vop_repeat") ||
                     stricmp(type,"vop_onion")
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

                if(stricmp(type,"vop_union")) e = new vop_union(id,nodes);
                else if(stricmp(type,"vop_intersection")) e = new vop_intersection(id,nodes);
                else if(stricmp(type,"vop_subtraction")) e = new vop_subtraction(id,nodes);
                else if(stricmp(type,"vop_cut")) e = new vop_cut(id,nodes.empty() ? nullptr : nodes[0]);
                else if(stricmp(type,"vop_twist")) e = new vop_twist(id,nodes.empty() ? nullptr : nodes[0]);
                else if(stricmp(type,"vop_invert")) e = new vop_invert(id,nodes.empty() ? nullptr : nodes[0]);
                else if(stricmp(type,"vop_repeat")) e = new vop_repeat(id,nodes.empty() ? nullptr : nodes[0]);
                else if(stricmp(type,"vop_onion")) e = new vop_onion(id,nodes.empty() ? nullptr : nodes[0]);

                //if(!e) throw VException("Unerecognized Operator \""+type+"\""); //NON NECESSARIO

                e->Relate(entry);
            }else{ //VOLUMI
                auto mtl_id = entry->try_at(1);
                link(scn,mtl_id,data,"vmaterial");
                auto mtl = static_cast<VMaterial*>(data[mtl_id].ptr);

                VNode* e = nullptr;

                if(stricmp(type,"vvo_sd_sphere")) e = new vvo_sd_sphere(id,mtl);
                else if(stricmp(type,"vvo_sd_box")) e = new vvo_sd_box(id,mtl);
                //else if(stricmp(type,"vvo_shadered")) e = new vvo_shadered(id,mtl);
                else if(stricmp(type,"vvo_sd_plane")) e = new vvo_sd_plane(id,mtl);
                else if(stricmp(type,"vvo_sd_ellipsoid")) e = new vvo_sd_ellipsoid(id,mtl);
                else if(stricmp(type,"vvo_sd_cylinder")) e = new vvo_sd_cylinder(id,mtl);
                else if(stricmp(type,"vvo_sd_capsule")) e = new vvo_sd_capsule(id,mtl);
                else if(stricmp(type,"vvo_sd_hex_prism")) e = new vvo_sd_hex_prism(id,mtl);
                else if(stricmp(type,"vvo_sd_tri_prism")) e = new vvo_sd_tri_prism(id,mtl);
                else if(stricmp(type,"vvo_sd_capped_cone")) e = new vvo_sd_capped_cone(id,mtl);
                else if(stricmp(type,"vvo_sd_pyramid4")) e = new vvo_sd_pyramid4(id,mtl);
                else if(stricmp(type,"vvo_sd_diamond")) e = new vvo_sd_diamond(id,mtl);

                if(!e) throw VException("Unerecognized Entity \""+type+"\"");
                e->Relate(entry);
            }

        }

    };
};
#endif
