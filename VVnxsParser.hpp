#ifndef VVNXSPARSER_HPP_INCLUDED
#define VVNXSPARSER_HPP_INCLUDED

#include "VAureaNox.hpp"
namespace vnx{
    class VVnxsParser{
        struct VVNException : public VException{
            VVNException(std::string msg):VException(msg),bFatal(true){}
            VVNException(bool fatal,std::string msg):VException(msg),bFatal(fatal){}
            bool bFatal = true;
        };

        std::map<std::string,VMappedEntry> mMappings;

        public:
        VVnxsParser(){};


        void parse(VScene& scn,const std::string& scn_id);
        void eval(const std::string& line,uint lid,std::string& cur_id);
        void link(VScene& scn,const std::string& id,std::string enforceType = "");

        inline void ExceptionAtLine(const std::string& msg,uint lid,bool fatal = true){
            throw VVNException(fatal,msg+" | line : "+std::to_string(lid));
        }

        inline const VMappedEntry* getSection(const std::string& s) const{
            auto ms = mMappings.find(s);
            if(ms == mMappings.end()) return nullptr;
            return &ms->second;
        }

    };
}

#endif // VVNXSPARSER_HPP_INCLUDED
