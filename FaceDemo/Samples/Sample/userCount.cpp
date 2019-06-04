#include "Samples.hpp"

int Samples::update_userinfo_num(string name) {
        std::vector<string>::iterator it;
        if(it == userinfo_nums.end()){
            userinfo_nums.push_back(name);
        }
        return 0;
    }

int Samples::FromVectorCountbyName(std::map<string, int>& vector_count_by_name,
        const  char* name,const char* flag){
        int    count = 0;
        std::map<string, int>::iterator it;
        it = vector_count_by_name.find(std::string(name));

        if (strcmp(flag,"get") == 0){
            if(it != vector_count_by_name.end()){
                count = it->second;
            }else{
                count = 0;//该名字首次注册返回0
            }
        }
        if (strcmp(flag,"update") == 0){
            if(it != vector_count_by_name.end()){
                vector_count_by_name[it->first]++;
            }else{
                vector_count_by_name.insert(std::pair<string, int>(std::string(name), 0));
            }
        }
        return  count;
    }

    /***
     * 读取配置configure file 到 devicename
     */
int Samples::read_userinput_configfile(const std::string& configfile,string& devicename){
        int checkfile = access(configfile.c_str(),F_OK|R_OK);
        if(checkfile == 0){
            FILE *file = fopen(configfile.c_str(), "r");
            if ( file != NULL )
            {
                char      line[64]={0}; /* or other suitable maximum line size */
                while ( (fgets(line, sizeof line, file) != NULL)) /* read a line */
                {
                    char    *p_index = strstr(line,"node");
                    if(p_index != NULL){
                        devicename = p_index+6;
                        devicename = devicename.substr(devicename.find("\""),devicename.rfind("\""));
                        devicename = devicename.substr(devicename.find("/"),devicename.rfind("\"")-1);
                        break;
                    }
                }
                fclose(file);
                return 0;
            }else{
#ifdef ARM64_DEBUG
	std::cout  << "config   file open ptr is null" << std::endl;
#endif
                //fclose(file);
                return -991;
            }
        }else{
      #ifdef LINUX_C_DEBUG
      	     printf("%s File :%s, config   file access fail %s  %s \n",font.red,__FILE__,configfile.c_str(),font.end);
      #endif
            return -990;
        }
    }
