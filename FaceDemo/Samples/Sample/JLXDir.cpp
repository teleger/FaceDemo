#include "Classify.hpp"
#include "Samples.hpp"
#include <unistd.h>
int  Samples::SelfMkdir(const char* nameDir){
    if((access(nameDir,F_OK))!=-1)
    {
#if DEBUG_OUT_INFO
        LOG(INFO) << "file exists path:" << nameDir ;
#endif
        return 0;
    }else
    {
        try {
            int isCreate = mkdir(nameDir,S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
            if(!isCreate){
#if DEBUG_OUT_INFO
                LOG(INFO) << "create path:" << nameDir ;
#endif
            }
        }
        catch(int create){
#if DEBUG_OUT_ERROR
            LOG(ERROR) << "create path failed! error code:" << create  << " Dir " << nameDir;
#endif
        }
    }
    return 0;
}
void Samples::CheckDirPathExist(){
    SelfMkdir("/sdcard/gti/face");
    SelfMkdir("/sdcard/gti/face/Data");
    SelfMkdir("/sdcard/gti/face/Data/vectors/");
    SelfMkdir("/sdcard/gti/face/Data/photos/");
}