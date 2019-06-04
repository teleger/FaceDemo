/***************************************************************
 * Samples.hpp
 * GTI2801 Sample code header file.
 *
 *                Copyrights, Gyrfalcon technology Inc. 2017
 ***************************************************************/
#pragma once

#include <iomanip>
#include <opencv2/opencv.hpp>
#include <opencv2/opencv_modules.hpp>

#include <stdio.h>
#include <stdlib.h>
#include "HandleFile.hpp"

#ifdef WIN32
#include <windows.h>
#endif


#include <time.h>
#include <cstring>
#include <functional>
#include "Classify.hpp"
#include "GTILib.h"
#include "JLXsqlite3.hpp"
#include "JLXDefine.hpp"
#include "Threadpool.hpp"
#include "ThreadQueue.hpp"
#include "jsonconfig.hpp"

#define scanf_s scanf
#define fscanf_s fscanf
#define sprintf_s sprintf

typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef char TCHAR, _TCHAR;
typedef int  BOOL;
typedef unsigned long ULONGLONG;

#ifdef ANDROID
#include <logging.h>
#endif


#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define Sleep(ms) usleep(ms * 1000)

//#define min(a, b) (((a)<(b)) ? (a) : (b))

using std::string;

#define FTDI                0
#define EMMC                1
#define PCIE                2

#define MAX_FILENAME_SIZE   256
#define IMAGE_OUT_SIZE      0x08000
#define GTI_IMAGE_WIDTH     224
#define GTI_IMAGE_HEIGHT    224
#define GTI_IMAGE_CHANNEL   3

#define GTI_PICTURE         0
#define GTI_VIDEO           1
#define GTI_WEBCAM          2

#define PEOPLE_TYPE         7
#define PETS_TYPE           4
#define OFFICE_TYPE         0
#define KITCHEN_TYPE        1
#define LIVINGROOM_TYPE     2


//控制c/c++输出宏
#define OUTINFO_SPEND   0
#define DEBUG_OUT_INFO  0
#define DEBUG_OUT_ERROR 0
#define ARM64_DEBUG

//自定义变量宏--------------
#define STATUS_OK       0
#define JLX_GTI_ENDSTATUS_OK  1
#define JLX_GTI_CON_ERROR_CODE 0
#define JLX_SDK_INIT_ERROR 10001
//-----
#ifdef WIN32
typedef struct dirent {
    string d_name;
} Dirent, *pDirent;

typedef HANDLE cnn_mutex_t;
#define cnn_mutex_init		WIN_mutex_init
#define cnn_mutex_lock		WIN_mutex_lock
#define cnn_mutex_unlock	WIN_mutex_unlock
#define cnn_mutex_destroy	WIN_mutex_destroy
#endif

#ifdef LINUX
typedef pthread_mutex_t cnn_mutex_t;

#define CNN_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define cnn_mutex_init		pthread_mutex_init
#define cnn_mutex_lock		pthread_mutex_lock
#define cnn_mutex_unlock	pthread_mutex_unlock
#define cnn_mutex_destroy	pthread_mutex_destroy
#endif


typedef struct jlxpersoninfo
{
    char  name[64];
    float similar;
}jlxPersonInfo;

typedef struct maxSimilar
{
    int   max_idx;
    float max_similar;
}MaxSimilar;


#ifdef ANDROID
typedef struct userinfo
{
    char name[64];
    int  index;
}UserInfo;
#endif


class Samples
{
private:
    const std::string TAG = "Samples";
public:
    Samples();
    Samples(int);
    Samples(int ,float);
    ~Samples();

    inline float custom_sqrt(float x) {
        if(x == 0) return 0;
         float result = x;
         float xhalf = 0.5f*result;
         int i = *(int*)&result;
         i = 0x5f375a86- (i>>1);
         result = *(float*)&i;
         result = result*(1.5f-xhalf*result*result); // Newton step, repeating increases accuracy
         result = result*(1.5f-xhalf*result*result);
     return 1.0f/result;
    }


    inline int CheckFilePermiss(const char* FileName){
        return access(FileName,R_OK|F_OK);
    }
    inline MaxSimilar GetMaxSimilar(MaxSimilar &in1,MaxSimilar &in2){
        return in1.max_similar > in2.max_similar ? in1:in2;
    }


    int update_userinfo_num(string name) ;

    int FromVectorCountbyName(std::map<string, int>& vector_count_by_name,
        const  char* name,const char* flag);

    int read_userinput_configfile(const std::string& configfile,string& devicename);

    int  SelfMkdir(const char* nameDir);

    void CheckDirPathExist();

    bool  CheckExists(char* name){
        return jlxsq.CheckNameExist((const char*)name);
    }

    int  SetPackageResourcePath(const char* packageResourcePath);

    int  DelName(const char* name);

    //float get_cos_sim(float* test_vector1,float* test_vector2,int feat_vector_len,float abs_of_test_vector1,float abs_of_test_vector2);


    void load_vectors(char* directory, std::vector<string> &filename,
            std::vector<std::vector<float> > &target_vectors,int feat_vector_len,
            std::map<string, int>& vector_count_by_name);

    int   InitDB(const char* dbPath);

    int   GetSQUserNum();

    void  SetSQUserList(std::vector<std::string>& usernameList);

    int   DeleteFromVector(const char *name);

    template<typename Ta,typename Tb,typename Tc,typename Td>
    void  LoadVectorsFromDB(Ta &filename,
                            Tb &target_vectors,
                            Tc &feat_vector_len,
                            Td &vector_count_by_name);

    int   read_fc7_vector(char* filename, std::vector<float>& vector, int len);

    template <typename Ta,typename Tb>
    float  BaseAndPredict(Ta &base_vectors,Tb &input_vectors);

    template <typename Ta,typename Tb,typename Tc>
    static float inner_product(Ta &a,Tb &b,Tc &c){
        float  r = 0.0;
        for (int i = 0; i < c; i++) {
            r += a[i]*b[i];
        }
        return r;
    }

    template <typename Ta,typename Tb>
    static float abs_of_vector(Ta &a,Tb &b) {
        float r = sqrt(inner_product(a, a, b));
        return r;
    }
    int     JlxSDKStep();
    int     read_modelconfigurefile(const std::string& file);
    char*   gtiHome = NULL;

    int     oneFramePredictInit(int type);
    //int     faceIdentify(unsigned char *imgData1,unsigned char *imgData2);
    //int     faceIdentify(const cv::Mat& imgData1,const cv::Mat& imgData2);
    void    oneFramePredict(unsigned char *imgData, string* predictions);
    void    facePredict(const cv::Mat& img,std::vector<std::string>& predictions);
    void    facePredict2(unsigned char* img,std::vector<std::string>& predictions);
    float*  getFaceFeature(const cv::Mat& img);
    int     getfeatvectorLen();
    int     face_register(unsigned char *imgData,const char *name);
    int     face_register(const  cv::Mat& imgData,const char *name);
    void    getPredictresult(std::vector<std::string>& predictions);


template <typename Ta,typename Tb,typename Tc, typename Td>
MaxSimilar get_max_cos_sim_idx(Ta& target_vectors,
        Tb& test_vector,
        Tc& abs_of_target_vectors,
        Td& abs_of_test_vector);


template <typename Ta,typename Tb,typename Tc, typename Td>
MaxSimilar get_TOP5_idx(Ta& target_vectors,
                        Tb& test_vector,
                        Tc& abs_of_target_vectors,
                        Td& abs_of_test_vector);

float TwoSimilar(float* target,float* test);

//---
 template <typename testVector,typename absVector>
    MaxSimilar FromThreadPoolGetMaxSilimar(testVector& test_vector4,absVector& abs_of_test_vector);



protected:
    int videoFramerate;//视频帧率
    int videoTime;//视频长度 /秒

protected:
    string      pred_name;
    string      out_similStr;
    string      pred_name_idex;
    float       m_usersimilar;

    string      m_DBFilePath;

    Mysqlite3   jlxsq;

    //ThreadPool  mpool;

    bool        m_initResult;
    Jsonconfig  m_jsonconfig;

protected:
    char    *m_deviceName = NULL;
    string   m_coefName;
    cv::Size m_imgSize;
    std::string jlxdir;

    char  name[128]; // name for register
    int   feat_vector_len; // feature vector dim
    int   num_of_targets; // num of persons in pool//csv 文件数
    std::vector<string> facename_target_vectors;
    std::map<string, int> vector_count_by_name;

    std::vector<MyJLXArray > target_vectors;
    std::vector<float> abs_of_target_vectors;
    MyJLXArray feat_vector; // feature vector for inference
    std::vector<string> userinfo_nums;
    //end

    GtiDevice   *m_Device = NULL;
    TCHAR       *m_FileNameBuffer = NULL;
    float       *m_Buffer32FC3 = NULL;
    BYTE        *m_Img224x224x3Buffer = NULL;
    float       *m_ImageOutPtr =  NULL;
    int         m_WtRdDelay = 35;        // Read/Write delay
    int         m_GnetType;

    ULONGLONG m_SvicTimerStart = 0;
    ULONGLONG m_SvicTimerEnd = 0;
    ULONGLONG m_CnnTimerStart = 0;
    ULONGLONG m_CnnTimerEnd = 0;
    ULONGLONG m_FcTimerStart = 0;
    ULONGLONG m_FcTimerEnd = 0;
    ULONGLONG m_PureCnnStart = 0;
    ULONGLONG m_PureCnnEnd = 0;
    ULONGLONG m_IdentifyTimerStart = 0;
    ULONGLONG m_IdentifyTimerEnd = 0;


    Classify *gClassify = NULL;

#ifdef LINUX
    unsigned long GetTickCount64()
    {
        struct timeval tv;
        if(gettimeofday(&tv, NULL) != 0)
                return 0;
        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    }
#endif

#ifdef LINUX_C_DEBUG
    FontColor font;
#endif

    template <class T>
    void safeRelease(T& buffer)
    {
        if (buffer != nullptr)
        {
            delete [] buffer;
            buffer = nullptr;
        }
    }

    void setinitResult(bool result){
        m_initResult = result;
    }

    string openFileDialog(int Mode = 0);
    int    convolution(const cv::Mat& img);
    int    convolution(BYTE* img, const int imgLength);

    void convOutput();
    void setLabel(cv::Mat& im, const std::string label, const cv::Point & orig, double scale);
    void displayImage(cv::Mat &img, Classify* gClassify, int mode, string str="", int resize=1, int demoFlag=0, string ext="");
    void dispDemoImage(cv::Mat &img, Classify* gClassify, char* dispName, string str, int w, int h, int demoFlag=0, string ext="");
    void showImage(cv::Mat &dispImg, Classify* gClassify, string text, string displayName, string ext, int demoFlag);
    void showImage1(cv::Mat &dispImg, Classify* gClassify, string text, string displayName, string ext, int demoFlag);
    void cvt32FloatTo8Byte(float *InBuffer, uchar *OutBuffer, int Width, int Height, int Channels);
    void cnnSvicProc32FC3(const cv::Mat& img, std::vector<cv::Mat>* input_channels);
    void cnnSvicProc8UC3(const cv::Mat& img, std::vector<cv::Mat>*input_channels);
    int sdb2BlobsFcUnmapping(const unsigned char *svbuf_ptr, float* output_blobs_data, int dataLength);
    int gtiWriteToFile(char *filename, unsigned char *dataBuff, int dataLen);


    void convolution(const cv::Mat& img, string *ptxt);
    void convolution_face(const cv::Mat& img);

    int  checkfcfilefuntion(const std::string& fcfilepath,const std::string& labelfilepath);
    int  FaceRecognitionInit(Classify *gtiClassify);
    int  face_recognition_register(Classify* gtiClassify,unsigned char *img_face,const  char* name);
    int  face_recognition_register(Classify* gtiClassify,const cv::Mat& img_face,const  char* name);

    int   InitArcSoftDB(const char* dbPath,const std::string& table);
public:
    bool getinitResult(){
        return m_initResult;
    }
#ifdef WIN32
    int m_OpenFlag = 0;
    WIN32_FIND_DATA m_ffd;
    Dirent m_ent;
    HANDLE opendir(const char* dirName);
    pDirent readdir(HANDLE);
    void closedir(HANDLE);
#endif
};
