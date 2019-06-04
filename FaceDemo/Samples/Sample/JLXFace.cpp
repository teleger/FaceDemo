/***************************************************************
 * JLXClassify.cpp
 * JLX2801 output result classify sample code.
 *
 *                Copyrights, JLX technology Inc. 2018
 ***************************************************************/
#include "stdafx.h"
#include "Classify.hpp"
#include "Samples.hpp"
#include "Threadpool.hpp"
#include "GtiClassify.h"
#include <math.h>
#include <algorithm>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <stdint.h>
#include <assert.h>
#include <linux/fs.h>
#include <time.h>
#include <sys/time.h>

#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <linux/mmc/ioctl.h>


#ifdef WIN32
#include "strsafe.h"
#endif


#ifdef LINUX
#include <dirent.h>
#include <fcntl.h>
#endif


#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#include <logging.h>
#include <sys/capability.h>
#endif


using std::string;

//add
#ifdef WIN32
cnn_mutex_t CNN_MUTEX_INITIALIZER()
{
    return CreateMutex(NULL, FALSE, NULL);
}

int WIN_mutex_init(cnn_mutex_t *mutex, void *attr)
{
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return 0;
}

int WIN_mutex_lock(cnn_mutex_t * hMutex)
{
    WaitForSingleObject(*hMutex, INFINITE); /* == WAIT_TIMEOUT; */
    return 0;
}

int WIN_mutex_unlock(cnn_mutex_t * hMutex)
{
    ReleaseMutex(*hMutex);
    return 0;
}

int WIN_mutex_destroy(cnn_mutex_t * hMutex)
{
    CloseHandle(*hMutex);
    return 0;
}
#endif


cnn_mutex_t mOpenCvMutex = CNN_MUTEX_INITIALIZER;

string glGtiPath = "GTISDKPATH";


string dataPath = "/sdcard/gti/";
string glImageFolder = dataPath + "Image_bmp";
string glVideoFolder = dataPath + "Image_mp4";
// Model files root
string glGnet1Root   = dataPath + "Models/gti2801/gnet1/";


// coef file name
string glGtiCoefFileName = glGnet1Root + "cnn/gnet1_coef_vgg16.dat";
string glGnet1UserInput = glGnet1Root + "cnn/userinput.txt";
// Gnet32 coef file name for 512 output
//string glGnet32Coef512 = glGnet2_3Root + "cnn_3/gnet32_coef_512.dat";
//string glGnet32UserInput = glGnet2_3Root + "cnn_3/userinput.txt";


int  get_num_of_files(char* directory, string s) {
    int num_of_files = 0;
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(directory)) != NULL){
        /* retrieve all .csv files within directory */
        while((ent = readdir (dir)) != NULL){
            string s1 = ent->d_name;
            string s2 = s;
            if(s1.find(s2) != std::string::npos){
                num_of_files++;
            }
        }
        closedir(dir);
    }else{
#ifdef ANDROID
        LOG(ERROR) << "get_num_of_files Failed to open directory!";
#else
        printf("get_num_of_files Failed to open directory!");
#endif
    }
    return num_of_files;
}

bool GtiResize(unsigned char* imgIn,
               int inHeight,
               int inWidth,
               unsigned char* imgOut,
               int outHeight,
               int outWidth) {
    if(imgIn == NULL) return false;

    double scaleWidth =  (double)outWidth / (double)inWidth;
    double scaleHeight = (double)outHeight / (double)inHeight;

    for(int cy = 0; cy < outHeight; cy++)
    {
        for(int cx = 0; cx < outWidth; cx++)
        {
            int pixel = (cy * (outWidth *3)) + (cx*3);
            int nearestMatch =  (((int)(cy / scaleHeight) * (inWidth *3)) + ((int)(cx / scaleWidth) *3) );

            imgOut[pixel    ] =  imgIn[nearestMatch    ];
            imgOut[pixel + 1] =  imgIn[nearestMatch + 1];
            imgOut[pixel + 2] =  imgIn[nearestMatch + 2];
        }
    }

    return true;
}

bool GtiCvtColor(unsigned char* imgIn,
                 int imgHeight,
                 int imgWidth,
                 unsigned char* imgOut) {
    if(imgIn == NULL) return false;

    for(int cy = 0; cy < imgHeight; cy++)
    {
        for(int cx = 0; cx < imgWidth; cx++)
        {
            int pixel = (cy * (imgWidth *3)) + (cx*3);
            imgOut[cy*imgWidth + cx ] =  imgIn[pixel + 0]; // R
            imgOut[1*imgHeight*imgWidth + cy*imgWidth + cx ] =  imgIn[pixel + 1]; // G
            imgOut[2*imgHeight*imgWidth + cy*imgWidth + cx ] =  imgIn[pixel + 2]; // B
        }
    }

    return true;
}

void get_filenames(char* directory,
                   char** filename,
                   int num_of_files,
                   string s) {
    char dir_vector[256];
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL) {
        int i = 0;
        while (((ent = readdir (dir)) != NULL) && (i < num_of_files)) {
            string s1 = ent->d_name;
            string s2 = s;
            if (s1.find(s2) != std::string::npos) {
                strcpy(filename[i], ent->d_name);
                strcpy(dir_vector, directory);
                strcat(dir_vector, ent->d_name);
                strcpy(filename[i], dir_vector);
                i++;
            }
        }
        closedir(dir);
    } else {
        printf("Failed to open directory!");
    }
    return;
}



//---------------------------------------------------------------------------------------------------------------------------

//====================================================================
// Function name: Samples()
// Class Samples construction.
// This function calls JLX library APIs to set device type, open device,
// initialize JLX SDK library. It also allocates memory for the sample
// code.
//====================================================================
Samples::Samples(){
cnn_mutex_init(&mOpenCvMutex, NULL);
}

Samples::Samples(int gnetType):
        m_GnetType(gnetType),
        m_initResult(false),
        videoFramerate(30),
        videoTime(5),
        m_Device(nullptr),
        m_FileNameBuffer(nullptr),
        m_Buffer32FC3(nullptr),
        m_Img224x224x3Buffer(nullptr),
        m_ImageOutPtr(nullptr){
	std::cout << " Samples --- ---1 --- -- " << std::endl;
       cnn_mutex_init(&mOpenCvMutex, NULL);
#ifdef LINUX
	#ifdef ANDROID
		jlxdir = "/mnt/sdcard/Jlx";
	#else
		gtiHome = getenv("HOME");
	       if (gtiHome == NULL)
	        {
	           printf("GTISDK home path is not set! Please see GTI SDK user guide.\n");
	           exit(0);
	        }
#ifdef LINUX_C_DEBUG
    printf(" Samples --- ---4 --- -- \n");
#else
    std::cout << " Samples --- ---4 --- -- " << std::endl;
#endif

		jlxdir = gtiHome;
		jlxdir = jlxdir + "/Jlx";
		if( 0 != access(jlxdir.c_str(),F_OK)){
#ifdef LINUX_C_DEBUG
      printf("Jlx SDK  path is not set! Please see  SDK user guide \n");
#else
      std::cout << "Jlx SDK  path is not set! Please see  SDK user guide " << std::endl;
#endif
    }

	#endif
#endif
}

Samples::Samples(int gnetType,float similar):
        m_GnetType(gnetType),
        m_initResult(false),
        videoFramerate(30),
        m_usersimilar(similar),
        videoTime(5),
        m_Device(nullptr),
        m_FileNameBuffer(nullptr),
        m_Buffer32FC3(nullptr),
        m_Img224x224x3Buffer(nullptr),
        m_ImageOutPtr(nullptr){

    		cnn_mutex_init(&mOpenCvMutex, NULL);
#ifdef LINUX
	#ifdef ANDROID
		jlxdir = "/mnt/sdcard/Jlx";
	#else
		gtiHome = getenv("HOME");
	        if (gtiHome == NULL)
	        {
	             printf("GTISDK home path is not set! Please see GTI SDK user guide.\n");
	             exit(0);
	        }
		jlxdir = gtiHome;
		jlxdir = jlxdir + "/Jlx";
		if( 0 != access(jlxdir.c_str(),F_OK)){
			std::cout << "Jlx SDK  path is not set! Please see  SDK user guide " << std::endl;
		}
	#endif
#endif

}
//====================================================================
// Function name: ~Samples()
// Class Samples destruction.
// This function releases the memory allcated in Samples(),
// it also calls JLX library API to close device.
//====================================================================
Samples::~Samples() {

#ifdef LINUX
	#ifdef ARM64_DEBUG
    	printf("file :%s  ,line : %d ,  dis structure 1\n" ,__FILE__,__LINE__);
	#endif
#endif
    cnn_mutex_destroy(&mOpenCvMutex);
    safeRelease(m_ImageOutPtr);
    safeRelease(m_Img224x224x3Buffer);
    safeRelease(m_Buffer32FC3);
    safeRelease(m_FileNameBuffer);
    safeRelease(m_deviceName);

    if(m_Device != NULL){
#ifdef LINUX
	#ifdef ARM64_DEBUG
    	printf("file :%s  ,line : %d ,  m_Device\n" ,__FILE__,__LINE__);
	#endif
#endif
        GtiCloseDevice(m_Device);
        GtiDeviceRelease(m_Device);
    }


    if(gClassify != NULL){
        GtiClassifyRelease(gClassify);
#ifdef LINUX
	#ifdef ARM64_DEBUG
      printf("file :%s  ,line : %d ,   release gClassify ok \n" ,__FILE__,__LINE__);
	#endif
#endif
    }

}


int Samples::read_fc7_vector(char* filename, std::vector<float>& vector, int len) {
    FILE *file = fopen(filename, "r");
    if ( file != NULL )
    {
        int       i = 0;
        char      line[128]={0}; /* or other suitable maximum line size */
        while ( (fgets(line, sizeof line, file) != NULL) && (i < len)) /* read a line */
        {
            //fputs (line, stdout); /* write the line */
            vector[i] = atof(line);
            i++;
        }
        fclose(file);
    }
    else {
        printf("Failed to open vector file!");
#ifdef ANDROID
        LOG(ERROR) << "read_fc7_vector Failed to open vector file!";
#endif
        return -70;
    }
    return 0;
}

int Samples::InitArcSoftDB(const char* dbPath,const std::string& table){
    SelfMkdir(dbPath);
    std::string dbFile(dbPath);
    dbFile = dbFile + "/" + "face.db";
#ifdef ANDROID
    LOG(INFO) << TAG << " DBFile----> " << dbFile;
#else
    std::cout << " DBFile----> " << dbFile;
#endif
    try {
        //1.step sqlite open, db file ...
        int ret1 = jlxsq.open(dbFile.c_str());
        if(!ret1)
        {
#if DEBUG_OUT_INFO
            LOG(INFO) << "Open DB OK";
#endif
        }else{
#if DEBUG_OUT_INFO
            LOG(ERROR) << "Open DB Failed";
#endif
        }
        //2.step should do : sqlite created table..
        const std::string CREATE_TABLE_PRE  ="CREATE TABLE IF NOT EXISTS ";;
        int ret2 = jlxsq.CreateArcTable(table,CREATE_TABLE_PRE);
        if(!ret2)
        {
#if DEBUG_OUT_INFO
            LOG(INFO)<< " Sqlite3_exec  Ok";
#endif
        }

    }
    catch(int ret){
#ifdef ANDROID
        LOG(ERROR) << TAG << " " << "InitDB " << ret;
#endif
    }
    return  0;
}

//初始化db file
int Samples::InitDB(const char* dbPath){
    SelfMkdir(dbPath);
    std::string dbFile(dbPath);
    dbFile = dbFile + "/" + "face.db";

#ifdef ANDROID
	#if DEBUG_OUT_INFO
	    LOG(INFO) << TAG << " DBFile PATH :  " << dbFile << std::endl;
	#endif
#else
	#ifdef ARM64_DEBUG
      printf(" class : %s , DBFile PATH : %s \n",TAG.c_str(),dbFile.c_str());
	#endif
#endif

    try
    {
        //1.step sqlite open, db file ...
        int ret1 = jlxsq.open(dbFile.c_str());
        if(!ret1)
        {
#ifdef ANDROID
	#if DEBUG_OUT_INFO
	            LOG(INFO) << "Open DB OK"<< std::endl;
	#endif
#else
	#ifdef ARM64_DEBUG
      printf("%s class : %s ,Open DB OK %s\n",font.green,TAG.c_str(),font.end);
	    //std::cout << TAG << " Open DB OK" << std::endl;
	#endif
#endif
        }else{
#ifdef ANDROID
	#if DEBUG_OUT_INFO
            LOG(ERROR) << "Open DB Failed" << std::endl;
	#endif
#else
	#ifdef ARM64_DEBUG
    	std::cout << TAG << "Open DB Failed" << std::endl;
	#endif
#endif
        }
        //2.step should do : sqlite created table..
        int ret2 = jlxsq.created();
        if(!ret2)
        {
#ifdef ANDROID
	#if DEBUG_OUT_INFO
	            LOG(INFO)<< " Sqlite3_exec  Ok"<< std::endl;
	#endif
#else
	#ifdef ARM64_DEBUG
	    std::cout << TAG << " Sqlite3_exec  Ok"<< std::endl;
	#endif
#endif
        }
    }
    catch(int ret){
#ifdef ANDROID
        LOG(ERROR) << TAG << " " << "InitDB catch" << ret << std::endl;
#else
	std::cout << TAG << " " << "InitDB catch " << ret << std::endl;
#endif
    }
    return  0;
}

void Samples::load_vectors(char* directory,
                           std::vector<string> &filename,
                           std::vector<std::vector<float> > &target_vectors,
                           int feat_vector_len,
                           std::map<string,
                                   int>& vector_count_by_name) {
    char dir_vector[256]={0};
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL)
    {
        int   i = 0;
        while (((ent = readdir (dir)) != NULL) && (i < num_of_targets))
        {
            string s1 = ent->d_name;
            string s2 = ".csv";
            if (s1.find(s2) != std::string::npos)
            {
                //strcpy(filename[i].c_str(), ent->d_name);
                filename.at(i) =  string(ent->d_name);//up one line
                strcpy(dir_vector, directory);
                strcat(dir_vector, ent->d_name);
                read_fc7_vector(dir_vector, target_vectors[i], feat_vector_len);
                i++;

                //int nameindex = atoi(Findout(s1.c_str()).c_str());

                s1.erase(s1.length()-9, 9); // remove _xxxx.csv

                std::map<string, int>::iterator it = vector_count_by_name.find(s1);
                if( it != vector_count_by_name.end())
                {
                    vector_count_by_name[it->first]++;
                }
                else
                {
                    vector_count_by_name.insert(std::pair<string, int>(s1, 0));
                    userinfo_nums.push_back(s1); //add
                }
            }
        }
        closedir(dir);
    }
    else
    {
#ifdef ANDROID
        LOG(ERROR) <<  "Failed to open directory!";
#else
        printf("Failed to open directory!");
#endif
    }

    std::map<string, int>::iterator it;
    for( it = vector_count_by_name.begin(); it != vector_count_by_name.end(); it++){
#ifdef ANDROID
        LOG(INFO) << "name: " << it->first.c_str() << " count :" << it->second;
#else
        printf("name: %s, count: %d\n", it->first.c_str(), it->second);
#endif
    }
    return;
}



int Samples::read_modelconfigurefile(const std::string& file){
    try {
        if(m_jsonconfig.open(file))
        {
#ifdef ARM64_DEBUG
  printf("json config open \n");
	//std::cout << "json config open "<< std::endl;
#endif
            m_jsonconfig.read();
            m_jsonconfig.parse_all(m_GnetType);
        }
        else
        {
            //打开配置文件出错..
#if DEBUG_OUT_ERROR
            LOG(ERROR) << "open configure file failed";
#endif
		std::cout << "open configure file failed";
            return -991;
        }
    }catch (...){
	std::cout <<"read model config catch throw " << std::endl;
    }
#ifdef ARM64_DEBUG
      printf("read_modelconfigurefile     end ..  OK  \n");
	    //std::cout << "read_modelconfigurefile     end ..  OK  " << std::endl;
#endif
    //LOG(INFO) <<"GTI_JNI: CoefName = " << m_jsonconfig.m_coefName <<  " userConfig = "<< m_jsonconfig.userConfig;
    return 0;
}

//cnn file network,and user configure file
int Samples::JlxSDKStep(){
#ifdef ARM64_DEBUG
  printf("sdk begin step  \n");
	//std::cout <<" sdk begin step " << std::endl;
#endif

#if OUTINFO_SPEND
    ULONGLONG  m_initTimeStart = GetTickCount64();
#endif
    //读取json配置文件

#ifdef ANDROID
	std::string jsonpath = jlxdir + "/Jlx_parms_configurefile.json";
        if(STATUS_OK != read_modelconfigurefile(jsonpath)){
       		 return JLX_SDK_INIT_ERROR;
    	}
#else
	std::string jsonpath = jlxdir + "/Jlx_parms_configurefile.json";
	if(STATUS_OK != read_modelconfigurefile(jsonpath)){
	        return JLX_SDK_INIT_ERROR;
    	}
#endif
#if DEBUG_OUT_INFO
    LOG(INFO) << TAG << "Read model configure file end";
#endif

    //从配置文件 设置相似值
    std::string& similar_value = m_jsonconfig.similar;
    if(!similar_value.empty()){
        m_usersimilar = (float)atof(similar_value.c_str());
        if(m_usersimilar <= 0){
            m_usersimilar = 0.75f;
#if DEBUG_OUT_ERROR
            LOG(ERROR) << "Config similar failed,Default Setting ";
#endif
        }
    } else{
        m_usersimilar = 0.75f;
#if DEBUG_OUT_ERROR
        LOG(ERROR) << "Config similar failed,Because similar is empty, then  Setting Default";
#endif
    }


    string          devicenode;
    //读取设备节点
    if(STATUS_OK != read_userinput_configfile(m_jsonconfig.userConfig,devicenode)) {
        //找不到 相关文件,提前退出,防止程序异常崩溃.
#if DEBUG_OUT_ERROR
        LOG(ERROR) << TAG << " Not Found gti File";
#endif

#ifdef LINUX_C_DEBUG
        //std::cout   <<  __LINE__ << " usr config path: " << m_jsonconfig.userConfig << std::endl;
        //std::cout   <<  __LINE__ << " device node : "   << devicenode << std::endl;
        //std::cout   <<  __LINE__ << " Not Found gti File" << std::endl;
        printf("%s LINE : %d, usr config path: %s %s\n",font.red,__LINE__,m_jsonconfig.userConfig.c_str(),font.end);
        printf("%s LINE : %d, device node : %s %s\n",font.red,__LINE__,devicenode.c_str(),font.end);
        printf("%s LINE : %d, Not Found gti File %s\n",font.red,__LINE__,font.end);
#endif
        return JLX_SDK_INIT_ERROR + 1;
    }


    //检测node 是否存在
    //-----
    if(access(devicenode.c_str(),R_OK|F_OK) == 0){
#if DEBUG_OUT_INFO
        LOG(INFO) << TAG << " access  "<< " Device Node: " << devicenode;
#endif
    }else{
#if DEBUG_OUT_ERROR
        LOG(ERROR) << TAG << " Device Node: " << devicenode;
#endif


#ifdef ARM64_DEBUG
        //std::cout << "File : " << __FILE__ << " line :" <<   __LINE__ << " " << TAG << " Device Node: " << devicenode<< std::endl;
        printf("File :   %s ,line : %d , TAG : %s , Device Node: %s \n", __FILE__,__LINE__,TAG.c_str(),devicenode.c_str());
#endif
    }

    size_t  nodelength  = devicenode.length();
    nodelength   = nodelength + 1;
    m_deviceName = new char[nodelength];
    memset(m_deviceName,0,nodelength);
    memcpy(m_deviceName,devicenode.c_str(),nodelength);
    //cnn 相关文件指定路径
    //device node name
    // Create  device with eMMC interface
    if((strstr(m_deviceName,"sg0") != NULL)|| (strstr(m_deviceName,"mmc") != NULL))
    {
        //LOG(INFO)<< "Samples create eMMC device, "<< (char *)m_jsonconfig.m_coefName.c_str() << " , user: " << (char *)m_jsonconfig.userConfig.c_str();
        if(access(m_jsonconfig.m_coefName.c_str(),R_OK|F_OK) == 0)
        {
            try
            {
                m_Device = GtiDeviceCreate(EMMC, (char *)m_jsonconfig.m_coefName.c_str(), (char *)m_jsonconfig.userConfig.c_str());
#if DEBUG_OUT_INFO
                LOG(ERROR) << TAG << " Jlx Device Create EMMC ok "<< "use mDevice ..  = " << m_Device;
#endif
                if(m_Device == nullptr){
#if DEBUG_OUT_ERROR
                    LOG(ERROR) << "m_Device is nullptr ---1 ";
#endif

#ifdef ARM64_DEBUG
        printf(" %s LINE : %d ,TAG : %s , Device Node: %s %s\n",font.red,__LINE__,TAG.c_str(),devicenode.c_str(),font.end);
        printf("%s coeffile : %s ,userconfig file : %s %s\n",font.red,m_jsonconfig.m_coefName.c_str(),m_jsonconfig.userConfig.c_str(),font.end);
#endif
                }
            }
            catch(...)
            {
#ifdef ANDROID
                LOG(ERROR) << TAG << " Jlx Device Create Failed";
#endif

#ifdef ARM64_DEBUG
        std::cout << TAG << " Device Node: " << devicenode;
#endif
                throw ;
            }
        }
        else
        {
#if DEBUG_OUT_ERROR
            LOG(ERROR) << " Coef File is Not Found ";
#endif

#ifdef ARM64_DEBUG
        //std::cout << "File : " << __FILE__  <<  " line : "<< __LINE__ << TAG <<  " Coef File is Not Found " << "coef file name : "  << m_jsonconfig.m_coefName << std::endl;
        printf(" File : %s  , line :%d , TAG: %s , Coef File is Not Found , coef file name : %s \n",__FILE__,__LINE__,TAG.c_str(),m_jsonconfig.m_coefName.c_str());
#endif
            return JLX_SDK_INIT_ERROR + 2;
        }
    }
    else      // Create GTI device with FTDI interface    network_2048.dat   userinput.txt";
    {
        //LOG(INFO) << " Samples Create FTDI device "  << m_jsonconfig.m_coefName  << " , " << m_jsonconfig.userConfig;
        if(access(m_jsonconfig.m_coefName.c_str(),R_OK|F_OK) == 0)
        {
            try
            {
                m_Device = GtiDeviceCreate(FTDI, (char *)m_jsonconfig.m_coefName.c_str(), (char *)m_jsonconfig.userConfig.c_str());
#if DEBUG_OUT_ERROR
    LOG(ERROR) << TAG << " Jlx Device Create EMMC ok "<<"use m_Device2 = "<<m_Device;
#endif
            }
            catch(...){
#ifdef ANDROID
    LOG(ERROR) << TAG << " Jlx Device Create Failed";
#endif
                throw ;
            }
            if (m_Device == NULL){
#ifdef ANDROID
    LOG(ERROR) << TAG << " Jlx Device is null "<< "m_Device3 = ";
#endif

#ifdef LINUX_C_DEBUG
  printf("%sFile :%s , line : %d ,  Jlx Device is null %s\n",font.red,__FILE__, __LINE__,font.end);
#endif
                return JLX_SDK_INIT_ERROR + 3;
            }else {
#if DEBUG_OUT_INFO
  LOG(INFO) << TAG << " Jlx Device Create OK";
#endif

#ifdef ARM64_DEBUG
    printf("%sFile :%s , line : %d , TAG : %s , Jlx Device Create OK %s\n",font.green,__FILE__, __LINE__,TAG.c_str(),font.end);
//std::cout << "File : " << __FILE__  <<  " line : "<< __LINE__ << TAG << " Jlx Device Create OK" << std::endl;
#endif
            }
        }
        else
        {
#if DEBUG_OUT_ERROR
            LOG(ERROR) << " Coef File is Not Found " ;
#endif
            return JLX_SDK_INIT_ERROR + 3;
        }
    }
#if OUTINFO_SPEND
    ULONGLONG  m_initTimeEnd = GetTickCount64();
    //LOG(INFO) << "Face_JNI: Samples create complete,spend time :"<< (float)(m_initTimeEnd - m_initTimeStart) <<" ms" ;
#endif

#if OUTINFO_SPEND
    ULONGLONG  m_initSdkTimeStart = GetTickCount64();
#endif

    if (m_Device == nullptr || m_deviceName == nullptr) {
#ifdef ANDROID
    LOG(ERROR) << "Device is  nullptr" << "m_Device4 = ";
#endif
        return JLX_SDK_INIT_ERROR + 4;
    }
    //if return value is 1,then ok
    if(m_Device != nullptr)
    {
        try
        {
            if(GtiOpenDevice(m_Device, m_deviceName) == JLX_GTI_ENDSTATUS_OK)
            {
#if DEBUG_OUT_INFO
                LOG(INFO) << TAG <<  " Open Device : " << m_deviceName << " OK ";
#endif

#ifdef ARM64_DEBUG
        //std::cout << "File : " << __FILE__  <<  " line : "<< __LINE__ << TAG <<  " Open Device : " << m_deviceName << " OK " << std::endl;
        printf("File : %s , line : %d ,TAG  : %s,  Open Device : %s ,  OK \n" ,__FILE__,__LINE__,TAG.c_str(),m_deviceName);
#endif
            }
        }catch(std::exception& e)
        {
#ifdef ANDROID
            LOG(ERROR) << TAG << " Open Device , Failed" << e.what();
#endif
        }catch(...)
        {
#ifdef ANDROID
            LOG(ERROR) << TAG << " Open Device , Failed";
#endif
            throw ;
        }

        unsigned int GTI_VERSION = GtiGetSDKVersion(m_Device);
#if DEBUG_OUT_INFO
        LOG(INFO) << TAG << "Version Code: " << GTI_VERSION;
#endif

#ifdef ARM64_DEBUG
        //std::cout << "File : " << __FILE__  <<  " line : "<< __LINE__ << " " << TAG << " Version Code: " << GTI_VERSION << std::endl;
        printf("File :  %s ,line : %d ,TAG : %s ,Version Code: %d \n",__FILE__,__LINE__,TAG.c_str(),GTI_VERSION);
#endif

        int GTI_INIT_CODE = GtiInitialization(m_Device);
        if(GTI_INIT_CODE == JLX_GTI_ENDSTATUS_OK)// Initialize GTI SDK
        {
#if DEBUG_OUT_INFO
            LOG(INFO) << TAG << " Device initialization OK ";
#endif

#ifdef ARM64_DEBUG
        //std::cout << "File : " << __FILE__  <<  " line : "<< __LINE__ << " "<< TAG << "  Device initialization OK "  << std::endl;
        printf("File : %s ,line : %d, TAG : %s ,Device initialization OK\n",__FILE__,__LINE__,TAG.c_str());
#endif
        }else
        {
#if DEBUG_OUT_ERROR
            LOG(ERROR) << TAG << "  Device initialization Error code:  " << GTI_INIT_CODE;
#endif

#ifdef ARM64_DEBUG
        //std::cout << "File : " << __FILE__  <<  " line : "<< __LINE__ << " " << TAG << "  Device initialization Error code:  "  << std::endl;
        printf("File : %s,line : %d , Device initialization Error code\n",__FILE__,__LINE__);
#endif

            GtiCloseDevice(m_Device);
            GtiDeviceRelease(m_Device);
            m_Device = NULL;
            return JLX_SDK_INIT_ERROR + 5;
        }
    }
    else
    {
#if DEBUG_OUT_ERROR
        LOG(ERROR) << TAG << " Open Device , Failed" << "m_Device5 = ";
#endif
        return JLX_SDK_INIT_ERROR + 6;
    }

    //LOG(INFO) << TAG <<" SDKStep Allocate memory for sample code use,begin ";
    // Allocate memory for sample code use
    try {
        m_ImageOutPtr = new float[GtiGetOutputLength(m_Device)];

        if (m_ImageOutPtr == nullptr)
        {
#ifdef ANDROID
            LOG(ERROR) << "\n\n\n------- GTI allocation (m_ImageOutPtr) failed.";
#endif
            return JLX_SDK_INIT_ERROR + 9;
        }
        m_Buffer32FC3 = new float[GTI_IMAGE_WIDTH * GTI_IMAGE_HEIGHT * GTI_IMAGE_CHANNEL];
        if (!m_Buffer32FC3)
        {
#ifdef ANDROID
            LOG(ERROR) << "\n\n\n------- GTI allocation (m_Buffer32FC3) failed.";
#endif
            return JLX_SDK_INIT_ERROR + 10;
        }
        m_Img224x224x3Buffer = new BYTE[GTI_IMAGE_WIDTH * GTI_IMAGE_HEIGHT * GTI_IMAGE_CHANNEL];
        if (!m_Img224x224x3Buffer)
        {
#ifdef ANDROID
            LOG(ERROR) <<"\n\n\n------- GTI allocation (m_Img224x224x3Buffer) failed.";
#endif
            return JLX_SDK_INIT_ERROR + 11;
        }
        m_FileNameBuffer    = new TCHAR[MAX_FILENAME_SIZE];
        if (!m_FileNameBuffer)
        {
#ifdef ANDROID
            LOG(ERROR) <<"\n\n\n------- GTI allocation (m_FileNameBuffer) failed.";
#endif
            return JLX_SDK_INIT_ERROR + 12;
        }
    }
    catch(std::bad_alloc& e){
#ifdef ANDROID
        LOG(ERROR) << TAG << "  " << "GtiSDKStep new space failed  " << e.what();
#else
        std::cout  << "  " << "GtiSDKStep new space failed  " << e.what();
        #endif
    }

#if OUTINFO_SPEND
    ULONGLONG  m_initSdkTimeEnd = GetTickCount64();
    float      sdkspendTime = (float)(m_initSdkTimeEnd - m_initSdkTimeStart);
    LOG(INFO) << " Sdk complete.spend time :" << sdkspendTime;
#endif

#ifdef ARM64_DEBUG
    //std::cout <<  "File : " << __FILE__  <<  " line : "<< __LINE__ << " Sdk complete  end " << std::endl;
    printf("File : %s , line : %d  , Sdk complete  end \n",__FILE__,__LINE__);
#endif
    return 0;
}


//====================================================================
// Function name: oneFramePredictInit()
// This function sets FC and label name for image, video or camera classifer.
//
// Input: none.
// return none.
//====================================================================
int Samples::oneFramePredictInit(int type) {
    //fc file 指定路径
#if OUTINFO_SPEND
    ULONGLONG  m_initTimeStart = GetTickCount64();
#endif
    if(type == 0) //Image
    {
        checkfcfilefuntion(m_jsonconfig.m_picFcname,m_jsonconfig.m_picLabelname);
    }
    else if(type == 1) //Video
    {
        checkfcfilefuntion(m_jsonconfig.m_videoFcname,m_jsonconfig.m_videoLabelname);
    }
    else if(type == 2) //Camera
    {
        checkfcfilefuntion(m_jsonconfig.m_camFcname,m_jsonconfig.m_camLabelname);
    }
    else if(type == 3) //Face
    {
        int ret = checkfcfilefuntion(m_jsonconfig.m_faceFcname,m_jsonconfig.m_faceLabelname);
        if(ret == 0){
		        std::cout <<" LINE : " << __LINE__ << "check file ok" << std::endl;
	}else{
		std::cout << "Failed  " << ret << std::endl;
	}
#if OUTINFO_SPEND
            ULONGLONG  m_initTimeEnd = GetTickCount64();
            float      PredictInitspendTime = (float)(m_initTimeEnd - m_initTimeStart);
            LOG(INFO)<< "GtiClassifyCreate: " << PredictInitspendTime ;
#endif

	FaceRecognitionInit(gClassify);
    } else{
#if DEBUG_OUT_ERROR
        LOG(ERROR) << "Samples::oneFramePredictInit type not supported. Type =" << type;
#endif
        return JLX_SDK_INIT_ERROR + 19;
    }
    //(image, video, camera,face)
    if(type == 3){
#ifdef LINUX
	#ifdef ANDROID
		#if DEBUG_OUT_INFO
		        LOG(INFO) << TAG << "  cnn_path:"  << m_jsonconfig.m_coefName << std::endl;
		        LOG(INFO) << TAG << "  fc_path : " << m_jsonconfig.m_faceFcname << std::endl;
		#endif
	#else
		#ifdef ARM64_DEBUG
		        std::cout << TAG << "  cnn_path:"  << m_jsonconfig.m_coefName << std::endl;
		        std::cout << TAG << "  fc_path : " << m_jsonconfig.m_faceFcname << std::endl;
		#endif
	#endif
#endif
    }
#ifdef ANDROID
	#if DEBUG_OUT_INFO
		LOG(INFO)<< "Samples::oneFramePredictInit. Type = " << type <<" face"<< std::endl;
	#endif
#else
	#ifdef ARM64_DEBUG
	    std::cout <<  TAG <<  " oneFramePredictInit. Type = " << type <<" face"<< std::endl;
	#endif
#endif

	//gti初始化(classify)完成标志
    setinitResult(true);
    return 0;
}
//====================================================================
// Function name: oneFramePredict(unsigned char *imgData, string* predictions)
// This function inputs picture image, sends the image to GTI FPGA,
// gets output from the FPGA, calls Classify functions to get the image
// class name and index.
//
// Input: none.
// return none.
//====================================================================
int Samples::checkfcfilefuntion(const std::string& fcfilepath,const std::string& labelfilepath){
#ifdef ARM64_DEBUG
  		std::cout << "oneFramePredictInit  File Path : "<< fcfilepath.c_str() << " label :" << labelfilepath.c_str() << std::endl;
#endif
if((0 == access(fcfilepath.c_str(),F_OK|R_OK))&& (0 == access(labelfilepath.c_str(),F_OK|R_OK))){
        try{
	    std::cout << "access file  ok " << std::endl;
            gClassify = GtiClassifyCreate(fcfilepath.c_str(), labelfilepath.c_str());
            if(gClassify == NULL){
		 #ifdef ARM64_DEBUG
		        std::cout << "GtiClassifyCreate  failed"<< std::endl;
		#endif
                return JLX_SDK_INIT_ERROR + 29;
            }

        }
        catch(std::exception& e){
		#ifdef ANDROID
		            LOG(ERROR) << " Classify Create  thrown "<< e.what();
		#else
			  std::cout <<       " Classify Create  thrown "<< e.what();
		#endif
	}
 }else {
		#ifdef ANDROID
		        LOG(ERROR) << "oneFramePredictInit  File Path"<< fcfilepath.c_str() << " label :" << labelfilepath.c_str();
		#else
			#ifdef ARM64_DEBUG
				        std::cout << "oneFramePredictInit  File Path  error  "<< fcfilepath.c_str() << " label :" << labelfilepath.c_str();
			#endif
		#endif
		return JLX_SDK_INIT_ERROR + 30;
    }
	std::cout << "oneFramePredictInit  File Path ------------------------- end"<< std::endl;
    	return 0;
}

//设置db文件存放路径 /data/usr/0/apK_path/files/db.file
int Samples::SetPackageResourcePath(const char* packageResourcePath){
#ifdef LINUX
	#ifdef ANDROID
		    const std::string DBFileName = "/files";
		    std::string str_path(packageResourcePath);
		    str_path    += DBFileName;
		    m_DBFilePath = str_path;
		#if DEBUG_OUT_INFO
		    LOG(INFO) << TAG << " ResourcePath : " << m_DBFilePath;
		#endif
	#else
		m_DBFilePath = jlxdir;//Jlx directory
	#endif
#endif
    return 0;
}

/*float Samples::get_cos_sim(float* test_vector1,float* test_vector2,
                           int feat_vector_len,
                           float abs_of_test_vector1,
                           float abs_of_test_vector2) {

			float cos_similarity = -1e10;

    			cos_similarity  = inner_product(test_vector1, test_vector2, feat_vector_len);
    			cos_similarity /= abs_of_test_vector1;
    			cos_similarity /= abs_of_test_vector2;


    			printf("--- two pic cos similarity: %f\n", cos_similarity);

    			return cos_similarity;
}*/

int Samples::face_register(unsigned char *imgData,const char *name) {
    return face_recognition_register(gClassify, imgData, name);;
}

int Samples::face_register(const cv::Mat& imgData,const char *name) {
    return face_recognition_register(gClassify, imgData, name);
}


#ifdef FACE_RECOGNITION
int Samples::GetSQUserNum(){//This function: Output UserNum
    int  outNum = 0;
    int  ret    = -1;
    if(!jlxsq.OpenDBStatus)
    {
        InitDB(m_DBFilePath.c_str());
    }

    try {
        ret = jlxsq.GetUserName_FromTable();//操作数据前,确保打开数据库
        if (ret == 0)//操作数据库成功
        {
            outNum = jlxsq.getnameNum();//IF from table get ok,do it;
            if(outNum == 0){
                LOG(ERROR) << TAG << " getSQ user OK,,,";
                return 0;
            }
        }
        else
        {
            LOG(ERROR) << TAG << "getSQ user num error";
            return 0;
        }
    }
    catch(int result){
        LOG(ERROR) << TAG << "  " << result;
    }

    return outNum;
}

void Samples::SetSQUserList(std::vector<std::string>& usernameList){
    try {
        jlxsq.GetNamelist(usernameList);
    }
    catch (std::exception e){
        LOG(ERROR) << TAG << "" << e.what();
    }
}
#endif


int  Samples::face_recognition_register(Classify* gtiClassify,const cv::Mat& img_face,const char* name){
    if(!m_initResult)return -101;//在未完成初始化的情况下  去注册  的返回值

    int     Ret = 0;
    // feature vector path
    //char    feat_vector_path[256]={0};
    // CNN
    convolution(img_face);
    // FC
    GtiClassifyFC(gtiClassify, m_ImageOutPtr, 1);
    // get feature vector
    float* v_out = GetFeatureVector(gtiClassify); // gtiClassify->m_Fc7_out

    MyJLXArray  feat_vector;
    for (int i = 0; i < feat_vector_len; ++i)
    {
        feat_vector[i] = v_out[i];
    }

    // insert new feature vector
    target_vectors.push_back(feat_vector);

    // insert abs of new feature vector
    abs_of_target_vectors.push_back(abs_of_vector(feat_vector, feat_vector_len)); // calculate the abs of the newly added vector

    // update number of target feature vectors
    num_of_targets++;

    //update vector count of each person
    FromVectorCountbyName(vector_count_by_name, name,"update");

    update_userinfo_num(string(name));

    // get feature vector filename
    char  fn_vector[128] = {0};

    int   index = FromVectorCountbyName(vector_count_by_name, name,"get");

    sprintf(fn_vector, "%s_%04d", name, index);

    //LOG(INFO) << "insertToDB  "<< " name: "<< fn_vector;
    // insert file name of new feature vector

    facename_target_vectors.push_back(string(name));//在数据库 的名字

    string insert_name = string(name);
    jlxsq.InsertToDB(feat_vector,insert_name,index);

    return  0;
}

int  Samples::face_recognition_register(Classify* gtiClassify,unsigned char *img_face,const char* name){
    if(!m_initResult){
        return 20001;//在未完成初始化的情况下去注册返回值
    }

    int  Ret = 0;
/*
    cv::Mat bgr(224,224,CV_8UC3,(unsigned char*)img_face);
    static int icountimage=0;
    char fname[50] = {0};
    sprintf(fname,"/sdcard/jlx/%d.jpg",icountimage++);
    IplImage ipm = IplImage(bgr);
    cvSaveImage(fname,&ipm,0);
*/

    // CNN
    ULONGLONG  CnnTimerStart = GetTickCount64();

    int conRet = convolution(img_face, GTI_IMAGE_WIDTH * GTI_IMAGE_HEIGHT * 3);

    ULONGLONG  CnnTimerEnd = GetTickCount64();

    if(conRet != 0)
    {
        //-95, convolution error
        #ifdef ANDROID
        LOG(ERROR) << "Face Register  "<< "Error Code:"<< conRet << " Because: convolution error ";
        #endif
        return conRet;
    }
    // FC
    ULONGLONG Fc_TimerStart = GetTickCount64();
    GtiClassifyFC(gtiClassify, m_ImageOutPtr, 1);
    ULONGLONG Fc_TimerEnd = GetTickCount64();
    #ifdef ANDROID
    LOG(INFO) << " FaceRegister  GtiClassifyFC" << " end:"<< Ret;
    #else
    std::cout << " FaceRegister  GtiClassifyFC" << " end:"<< Ret;
    #endif
    float* v_out = GetFeatureVector(gtiClassify); // gtiClassify->m_Fc7_out

    MyJLXArray  feat_vector;
    for (int i = 0; i < feat_vector_len; ++i)
    {
        feat_vector[i] = v_out[i];
    }
    // insert new feature vector
    target_vectors.push_back(feat_vector);

    // insert abs of new feature vector
    abs_of_target_vectors.push_back(abs_of_vector(feat_vector, feat_vector_len)); // calculate the abs of the newly added vector

    // update number of target feature vectors
    num_of_targets++;

    //update vector count of each person
    FromVectorCountbyName(vector_count_by_name, name,"update");

    update_userinfo_num(string(name));

    //LOG(INFO) << "user info num name: "<< name;

    // get feature vector filename
    char  fn_vector[128] = {0};

    int   index = FromVectorCountbyName(vector_count_by_name, name,"get");
    sprintf(fn_vector, "%s_%04d", name, index);

    #ifdef ANDROID
    LOG(INFO) << "insertToDB  "<< "name:"<< fn_vector;
    #else
    std::cout << "insertToDB  "<< "name:"<< fn_vector;
    #endif
    // insert file name of new feature vector
    facename_target_vectors.push_back(string(name));


    string insert_name = string(name);
    jlxsq.InsertToDB(feat_vector,insert_name,index);

//    write face image into .jpg file     //保存.jpg 文件相关去掉
//    bool bwrite = false;
//    if (bwrite)
//    {
//        sprintf(fn_photo, "%s_%04d.bgr", name, FromVectorCountbyName(vector_count_by_name, name,"get"));
//        strcpy(photo_path, dir_target_photos);
//        strcat(photo_path, fn_photo);
//
//        fp = fopen(photo_path, "wb");
//        if(fp)
//        {
//            ALOG("GTI_JNI: Cannot create file %s", photo_path);
//            fwrite(img_face, 1, 224 * 224 * 3, fp);
//            fclose(fp);
//            Ret = -104;
//        }
//    }
    return Ret;
}

int  Samples::DeleteFromVector(const char *name) {
   if(strcmp(name,"deleteall") == 0)
   {//清除全部
       if(facename_target_vectors.size() >= 0)
       {
           facename_target_vectors.clear();
           abs_of_target_vectors.clear();
           target_vectors.clear();
           return 0;
       }
   }

   std::vector<int>   data;
   int   ret = jlxsq.GetUserName_FromTable();
   if(ret == 0) {
       jlxsq.comp_vector(data,name);
       #ifdef ANDROID
       LOG(ERROR) << "delete step = 1,get table name num : "<< data.size() << "name: " << name << " ##";
       #else
        std::cout << "delete step = 1,get table name num : "<< data.size() << "name: " << name << " ##";
        #endif
   }
   else {
       return -30;
   }

   //擦除
//   facename_target_vectors.erase(
//            std::remove(std::begin(facename_target_vectors),
//                        std::end(facename_target_vectors),std::string(name)),
//            std::end(facename_target_vectors));

    std::vector<string>::iterator        iter    = facename_target_vectors.begin();
    std::vector<float>::iterator         iter_1  = abs_of_target_vectors.begin();
    std::vector<MyJLXArray>::iterator    iter_2  = target_vectors.begin();

   for(iter;iter != facename_target_vectors.end();){
       if(strcmp((*iter).c_str(),name) == 0){
           facename_target_vectors.erase(iter);
           abs_of_target_vectors.erase(iter_1);
           target_vectors.erase(iter_2);
           num_of_targets--;
       }else{
           iter++;
           iter_1++;
           iter_2++;
       }
       if(iter == facename_target_vectors.end()){
           break;
       }
   }

   return 0;
}

int  Samples::DelName(const char* name){
    DeleteFromVector(name);//操作（删除数据结构中的内容）
    return jlxsq.DeleteFromTableWhereName(name);
}

std::string Samples::openFileDialog(int Mode) {
#ifdef LINUX
    TCHAR fileName[200];
    std::string retFileName = "Image_bmp/bicycle.bmp";

    std::cout << "************************" << std::endl;
    switch (Mode)
    {
    case GTI_PICTURE:    // picture only
        std::cout << "Please input image file name(*.bmp, *.jpg, *.png): ";
        scanf_s("%s", fileName);
        getchar();
        retFileName = (std::string)&fileName[0];
        break;
    case GTI_VIDEO:      // video only
        std::cout << "Please input video file name(*.mp4): ";
        scanf_s("%s", fileName);
        getchar();
        retFileName = (std::string)&fileName[0];
        break;
    default:             // unknown
        std::cout << "Unknown file mode, open image file - Image_bmp/bicycle.bmp" << std::endl;
        break;
    }
    std::cout << "************************" << std::endl;
    return retFileName;
#else
    // Initialize OPENFILENAME
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = m_FileNameBuffer;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = MAX_FILENAME_SIZE;
    switch (Mode)
    {
    case GTI_PICTURE:     // picture only
        ofn.lpstrInitialDir = (LPCWSTR)glImageFolder.c_str();// L".\\Image_bmp\\";
        ofn.lpstrFilter = L"Image Files(*.BMP;*.JPG;*.PNG;)\0*.BMP;*.JPG;*.PNG;\0All files(*.*)\0*.*\0";
        break;
    case GTI_VIDEO:       // video only
        ofn.lpstrInitialDir = (LPCWSTR)glVideoFolder.c_str();  // L".\\Image_mp4\\";
        ofn.lpstrFilter = L"Video Files(*.mp4)\0*.mp4;\0All files(*.*)\0*.*\0";
        break;
    default:    // picture and video
        ofn.lpstrInitialDir = (LPCWSTR)glImageFolder.c_str(); // L".\\Image_bmp\\";
        ofn.lpstrFilter = L"Image Files(*.BMP;*.JPG;*.PNG;*.mp4)\0*.BMP;*.JPG;*.PNG;*.mp4;\0All files(*.*)\0*.*\0";
        break;
    }

    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = L"Open Image..."; // NULL;
    ofn.nMaxFileTitle = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    // Display the Open dialog box.
    if (GetOpenFileName(&ofn) == TRUE)
    {
        std::wstring ws(ofn.lpstrFile);                 // wchar_t to wstring
        return std::string(ws.begin(), ws.end());       // wstring to string
    }
    else
    {
        return std::string("");
    }
#endif
}

#ifdef WIN32
HANDLE Samples::opendir(const char* dirName)
{
    HANDLE hDir = INVALID_HANDLE_VALUE;
    wchar_t szDir[MAX_PATH];
    size_t ret;

    mbstowcs_s(&ret, szDir, MAX_PATH, dirName, MAX_PATH);
    StringCchCat(szDir, MAX_PATH, TEXT("\\*"));
    hDir = FindFirstFile(szDir, &m_ffd);
    if (INVALID_HANDLE_VALUE == hDir)
    {
        hDir = NULL;
    }
    m_OpenFlag = 1;
    return hDir;
}

pDirent Samples::readdir(HANDLE hDir)
{
    char fileName[200];
    int fsize;

    if (0 == m_OpenFlag)
    {
        if (0 == FindNextFile(hDir, &m_ffd))
        {
            return NULL;
        }
    }
    m_OpenFlag = 0;
    fsize = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, m_ffd.cFileName, -1, fileName, 0, NULL, NULL);
    fsize = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, m_ffd.cFileName, -1, fileName, fsize, NULL, NULL);
    m_ent.d_name = fileName;
    return &m_ent;
}

void Samples::closedir(HANDLE hDir)
{
    FindClose(hDir);
}
#endif


//c wrap To python
#ifdef __cplusplus
extern "C"{
    //构造 对象
    Samples* samples_new(int type){
        if(type != 0){
          return NULL;
        }
        return new Samples(type);
    }

    int sdk_step(Samples* s){
        if(s != NULL){
           return s->JlxSDKStep();
        }
        return -1;
    }
    // 设置 sqlite 的路径
    int set_pack_path(Samples* s,const char* path){
        if(s != NULL){
          return s->SetPackageResourcePath(NULL);
        }else{
#ifdef LINUX_C_DEBUG
    printf("LINE: %d ,Samples is null. please check \n",__LINE__);
#endif
        }
        return -2;
    }

    // 识别 模式.. 默认 3 ，（human face ） and so on
    int prediction(Samples* s,int type){
        if(s != NULL){
           return s->oneFramePredictInit(type);
        }else{
#ifdef LINUX_C_DEBUG
    printf("LINE: %d ,Samples is null. please check \n",__LINE__);
#endif
        }
        return -3;
    }

    bool get_init_Result(Samples* s){
      if(s != NULL){
          return s->getinitResult();
      }else{
#ifdef LINUX_C_DEBUG
    printf("LINE: %d ,Samples is null. please check \n",__LINE__);
#endif
      }
      return false;
    }

    //人脸识别 函数
    jlxpersoninfo face_predict(Samples* s, unsigned char* face_data){
        std::vector<std::string> predictions;
        jlxpersoninfo pred_py_name = {0};
        if(s != NULL){
            s->facePredict2(face_data,predictions);
            if(predictions.size() > 0){
              std::string& pred_name = predictions.at(0);

              if(strlen(pred_name.c_str()) >= 64){
                  strncpy(pred_py_name.name,pred_name.c_str(),64);
              }else{
                  strncpy(pred_py_name.name,pred_name.c_str(),strlen(pred_name.c_str()));
              }
            }else{
                printf("LINE: %d --- predict no face ---\n",__LINE__);
            }
        }else{
#ifdef LINUX_C_DEBUG
    printf("LINE : %d , Samples is null. please check \n",__LINE__);
#endif
        }
        return pred_py_name;
    }

    int  face_regist(Samples* s, unsigned char* face_data,const char* name){
        if(s != NULL){
          return   s->face_register(face_data,name);
        }else{
#ifdef LINUX_C_DEBUG
  printf("LINE : %d , Samples is null. please check \n",__LINE__);
#endif
        }
        return -4;
    }
}
#endif
//end
