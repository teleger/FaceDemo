#include "jsonconfig.hpp"

    //1.打开
bool Jsonconfig::open(const std::string& file){
        //预打开操作,验证文件权限问题
        int check = access(file.c_str(),R_OK|F_OK);
        if(check == 0){
#ifdef ARM64_DEBUG
	    std::cout << "file persionn check ok" << std::endl;
#endif
		ifs.open(file.c_str());
            assert(ifs.is_open());
#ifdef ARM64_DEBUG
	    std::cout << "file assert check ok" << std::endl;
#endif
	   return true;
        }
        return false;
    }

//2.解析操作
void Jsonconfig::read(){
        if (!reader.parse(ifs, root, false))
        {
#ifdef ARM64_DEBUG
	    std::cout << "reader.parse failed" << std::endl;
#endif
            std::cerr << "parse failed \n";
            return;
        }
}

void Jsonconfig::parse_all(int m_GnetType){
#ifdef ARM64_DEBUG
	    std::cout << "parse  all" << std::endl;
#endif
        std::string sdkpath = root["dataPath"].asString();
        similar = root["similar"].asString();
        if(m_GnetType == 0){
             std::string        Gnet1Root = root["JlxGnet1Root"].asString();
            m_coefName  = sdkpath + Gnet1Root +  root["JlxCoefFile"].asString();
            userConfig  = sdkpath + Gnet1Root +  root["JlxGnet1UserInput"].asString();

            m_faceFcname    = sdkpath + Gnet1Root + root["JlxWebCoefFcFile"].asString();
            m_faceLabelname = sdkpath + Gnet1Root + root["JlxWebLabelFileFace"].asString();

            m_picFcname     = sdkpath + Gnet1Root + root["JlxPicCoefDataFcFileName"].asString();
            m_picLabelname  =  sdkpath + Gnet1Root + root["JlxPicLabelFileName"].asString();

            m_videoFcname    = sdkpath + Gnet1Root + root["JlxVideoCoefDataFcFileName"].asString();
            m_videoLabelname = sdkpath + Gnet1Root + root["JlxVideoLabelFileName"].asString();

            m_camFcname    = sdkpath + Gnet1Root + root["JlxWebCamCoefDataFcFileName"].asString();
            m_camLabelname = sdkpath + Gnet1Root + root["JlxWebCamLabelFileName"].asString();

        }else{
             std::string Gnet2_3Root = root["JlxGnet2_3Root"].asString();

            m_coefName = sdkpath + Gnet2_3Root + root["JlxGnet32Coef512"].asString();
            userConfig = sdkpath + Gnet2_3Root + root["JlxGnet32UserInput"].asString();


            m_picFcname     = sdkpath + Gnet2_3Root + root["JlxPicCoefDataFcGnet32"].asString();
            m_picLabelname  =  sdkpath + Gnet2_3Root + root["JlxPicLabelGnet32"].asString();

            m_camFcname    = sdkpath + Gnet2_3Root + root["JlxWebCamCoefDataFcGnet32"].asString();
            m_camLabelname = sdkpath + Gnet2_3Root + root["JlxWebCamLabelGnet32"].asString();


            m_videoFcname = sdkpath    + Gnet2_3Root + root["JlxVideoCoefDataFcGnet32"].asString();
            m_videoLabelname = sdkpath + Gnet2_3Root + root["JlxVideoLabelGnet32"].asString();
        }
#ifdef ARM64_DEBUG
	    std::cout <<"File :" << __FILE__ <<  __LINE__ << " parse  all  end ..  OK  " << std::endl;
#endif
    }
