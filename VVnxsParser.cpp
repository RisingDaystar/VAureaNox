
#include "VVnxsParser.hpp"
#include "VScene.hpp"
#include "VSdfOperators.hpp"
#include "VSdfs.hpp"


////////////////////////////////////////////////////////
////////TTTTTTTT////OOOO//////DDDD///////OOOO///////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT/////OO////OO////DD//DD///OO////OO/////////
///////////TT///////OOOO//////DDDD///////OOOO///////////
////////////////////////////////////////////////////////

//<!>BETTER SYNTAX ERROR DETECTION AND HANDLING

namespace vnx{
    void VVnxsParser::parse(VScene& scn,const std::string& scn_id){
        if (scn_id == "") { throw VException("Vnxs Loader -> Exception -> Invalid .Vnxs filename."); }
        std::ifstream in;
        std::string line;
        in.open(scn_id.c_str(), std::ifstream::in);
        if(!in.is_open()) {
            std::string str = "Vnxs Loader -> Exception -> Could not open .vnxs file: ";
            str.append(scn_id.c_str());
            throw VException(str);
        }

        uint lid=0;
        std::string cur_id = "";
        while (std::getline(in, line)) {
            lid++;
            try {eval(line,lid,cur_id);}
            catch (VVNException& ex) {
                if(ex.bFatal){
                    in.close();
                    std::string str = "Vnxs Loader -> Parser Exception -> ";
                    str+=ex.what();
                    throw VException(str);
                }
                std::cout<<"Vnxs Loader -> Parser Warning -> "<<scn_id.c_str()<<": "<<ex.what()<<"\n";
            }
        }
        in.close();

        try{
            auto root_id = mMappings["#_ROOT"].try_get("id");
            link(scn,root_id,"!vmaterial"); //alloco in maniera ricorsiva , se non è collegato alla root, non è necessario instanziarlo a meno che non sia un materiale.
            if(mMappings.find("#_CAMERA")==mMappings.end()){throw VException("Camera is undefined");}
            scn.camera.Relate(&mMappings["#_CAMERA"]);
            scn.root = static_cast<VNode*>(mMappings[root_id].ptr);
            scn.mID = mMappings["#_SCENE"].try_get("id");

        }catch(const VException& ex){
            std::string str = "Vnxs Loader -> Linker Exception -> ";
            str+=ex.what();
            throw VException(str);
        }
    }

    void VVnxsParser::eval(const std::string& line,uint lid,std::string& cur_id){

        std::string arg_name = "";
        std::string arg_value = "";
        bool fve = false;
        int IDMode = 1;
        bool hasId = false;
        std::string tag_type = "";

        bool group = false;
        bool parsing_name = true;

        for(std::string::size_type i=0;i<line.size();i++){
            if(line[i]=='/'){
                if(i+1>=line.size() || line[i+1]!='/') ExceptionAtLine("Illegal character "+std::string(1,line[i])+"",lid,true);
                break;
            }
            if(line[i]=='#') ExceptionAtLine("Illegal character "+std::string(1,line[i])+"",lid,true);
            if((isspace(line[i]) || line[i]== '>')){
                if(!group){
                    if(!arg_value.empty()){
                        mMappings[cur_id].add_token(arg_name,arg_value);
                        arg_value.clear();
                        arg_name.clear();
                        parsing_name = true;
                    }
                    if(line[i]=='>'){cur_id.clear();break;}
                }
                continue;
            }

            if(!fve && cur_id.empty()){
                if(line[i]=='<'){
                    for(i++;i<line.size();i++){
                        auto ch = line[i];
                        if(ch=='#') ExceptionAtLine("Illegal character "+std::string(1,ch)+"",lid,true);
                        if(isspace(ch)){
                            if(tag_type.empty()) continue;
                            break;
                        }
                        if(ch==':') break;
                        if(ch=='<') ExceptionAtLine("Illegal character "+std::string(1,ch)+"",lid,true);
                        if(ch=='>') break;
                        tag_type += ch;
                    }

                    if(line[i]==':') hasId = true;

                    if(stricmp(tag_type,std::string("vroot"))){
                        IDMode = 0;
                        cur_id = "#_ROOT";
                        mMappings[cur_id] = VMappedEntry();
                    }else if(stricmp(tag_type,std::string("vscene"))){
                        IDMode = 0;
                        cur_id = "#_SCENE";
                        mMappings[cur_id] = VMappedEntry();
                    }else if(stricmp(tag_type,std::string("vmaterial"))){
                        IDMode = 2;
                        cur_id = "#_mtl_";
                    }else if(stricmp(tag_type,std::string("vcamera"))){
                        IDMode = 0;
                        cur_id = "#_CAMERA";
                        mMappings[cur_id] = VMappedEntry();
                    }

                    if(IDMode!=0 && !hasId) break;

                    if(hasId){
                        std::string id = "";
                        for(i++;i<line.size();i++){

                            auto ch = line[i];
                            if(ch=='#') ExceptionAtLine("Illegal character "+std::string(1,ch)+"",lid,true);
                            if(isspace(ch)){
                                if(id.empty()) continue;
                                break;
                            }
                            if(ch=='<') ExceptionAtLine("Illegal character "+std::string(1,ch)+"",lid,true);
                            if(ch=='>') {break;}
                            id+=ch;
                        }
                        if(IDMode==0){
                            if(id.empty()) ExceptionAtLine("Undefined id for type "+tag_type+" requiring it",lid,true);
                            mMappings[cur_id].add_token("id",id);
                        }if(IDMode==1){
                            if(id.empty()) ExceptionAtLine("Undefined id for type "+tag_type+" requiring it",lid,true);
                            cur_id = id;
                            mMappings[cur_id] = VMappedEntry();
                            mMappings[cur_id].add_token("id",cur_id); //Not necessary, but might be usefull
                        }else if(IDMode==2){
                            if(id.empty()) ExceptionAtLine("Undefined id for type "+tag_type+" requiring it",lid,true);
                            cur_id += id;
                            mMappings[cur_id] = VMappedEntry();
                            mMappings[cur_id].add_token("id",cur_id); //Not necessary, but might be usefull
                        }


                    }

                    mMappings[cur_id].add_token("#_type",tag_type);
                    if(line[i]=='>') cur_id.clear();
                }else{
                    ExceptionAtLine("Illegal character before tag definition",lid,true);
                }
                fve = true;
                continue;
            }

            if(line[i]=='{'){
                if(group) ExceptionAtLine("Illegal character {, group already open in args specification",lid,true);
                group = true;
                //character gathered, needed for group evaluation at link time
            }else if(line[i]=='}'){
                if(!group) ExceptionAtLine("Illegal character }, group not open in args specification",lid,true);
                group = false;
                //character gathered, needed for group evaluation at link time
            }else if(line[i]=='='){
                if(group) ExceptionAtLine("Illegal character =, within a group",lid,true);
                if(!parsing_name) ExceptionAtLine("Illegal character = in args specification",lid,true);
                if(!arg_name.empty()) parsing_name = false;
                else ExceptionAtLine("Illegal character = in args specification",lid,true);

                continue;
            }

            if(parsing_name) arg_name+=line[i];
            else arg_value+=line[i];
        }
        if(group) ExceptionAtLine("Illegal state , group still open at EOL",lid,true);
        if(!arg_value.empty()){
            mMappings[cur_id].add_token(arg_name,arg_value);
        }
    }

    void VVnxsParser::link(VScene& scn,const std::string& id,std::string enforceType){
        if(mMappings.find(id)==mMappings.end()){throw VException("Undefined reference to "+id);}

        VMappedEntry* entry = &mMappings[id];

        if(enforceType!=""){
            if(enforceType[0]=='!'){
                enforceType.erase(0,1);
                if(enforceType!="" && stricmp(entry->at("#_type"),enforceType)){throw VException("Reference \""+id+"\" is of type \""+enforceType+"\"");}
            }else{
                if(!stricmp(entry->at("#_type"),enforceType)){throw VException("Reference \""+id+"\" is not of type \""+enforceType+"\"");}
            }
        }

        if(entry->allocated()) return;
        auto type = entry->try_get("#_type");

        //TODO VBsdf
        if(type=="vmaterial"){
            auto mid = entry->try_get("id");
            scn.mMaterials[mid] = VMaterial();
            scn.mMaterials[mid].Relate(entry);
        }else if(stricmp(type,std::string("vop_union")) ||
                 stricmp(type,std::string("vop_intersection")) ||
                 stricmp(type,std::string("vop_subtraction")) ||
                 stricmp(type,std::string("vop_cut")) ||
                 stricmp(type,std::string("vop_twist")) ||
                 stricmp(type,std::string("vop_invert")) ||
                 stricmp(type,std::string("vop_repeat")) ||
                 stricmp(type,std::string("vop_onion"))
                 ){ //OPERATORI
            auto chs = entry->try_get("childs");
            if(chs=="") return;
            auto chs_vec = strDeGroup(chs);
            std::vector<VNode*> nodes;
            for(auto ch : chs_vec){
                link(scn,ch,"!vmaterial");
                nodes.push_back(static_cast<VNode*>(mMappings[ch].ptr));
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

            if(!e) throw VException("Invalid Operator initialization\""+type+"\""); //NON NECESSARIO

            e->Relate(entry);
            scn.mNodes.push_back(e);
        }else{ //VOLUMI
            auto mtl_id = entry->try_get("material");
            VMaterial* mtl = nullptr;
            if(mtl_id!=""){
                link(scn,"#_mtl_"+mtl_id,"vmaterial");
                mtl = static_cast<VMaterial*>(mMappings["#_mtl_"+mtl_id].ptr);
            }

            VNode* e = nullptr;

            if(stricmp(type,std::string("vvo_sd_sphere"))) e = new vvo_sd_sphere(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_box"))) e = new vvo_sd_box(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_plane"))) e = new vvo_sd_plane(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_ellipsoid"))) e = new vvo_sd_ellipsoid(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_cylinder"))) e = new vvo_sd_cylinder(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_capsule"))) e = new vvo_sd_capsule(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_hex_prism"))) e = new vvo_sd_hex_prism(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_tri_prism"))) e = new vvo_sd_tri_prism(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_capped_cone"))) e = new vvo_sd_capped_cone(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_pyramid4"))) e = new vvo_sd_pyramid4(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_diamond"))) e = new vvo_sd_diamond(id,mtl);
            else if(stricmp(type,std::string("vvo_sd_torus"))) e = new vvo_sd_torus(id,mtl);
            else if(stricmp(type,std::string("vvo_blended"))){
                auto v1 = entry->try_get("v1");
                if(v1=="") return;
                auto v2 = entry->try_get("v2");
                if(v2=="") return;

                link(scn,v1,"!vmaterial");
                auto v1_ptr = static_cast<VNode*>(mMappings[v1].ptr);
                if(v1_ptr){
                    link(scn,v2,"!vmaterial");
                    auto v2_ptr = static_cast<VNode*>(mMappings[v2].ptr);
                    if(v2_ptr){
                        e = new vvo_blended(id,mtl,v1_ptr,v2_ptr);
                    }
                }
            }else{
                throw VException("Unrecognized Entity \""+type+"\"");
            }

            if(!e) throw VException("Invalid Entity initialization\""+type+"\"");

            e->Relate(entry);
            scn.mNodes.push_back(e);
        }
    }
}

