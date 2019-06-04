/***************************************************************
 * Sample.cpp
 * GTI2801 Sample code main(). It includes the samples for
 * picture, video, and web camera.
 *
 *                Copyrights, Gyrfalcon technology Inc. 2017
 ***************************************************************/
#include "stdafx.h"
#include "Samples.hpp"
#ifdef LINUX
#ifndef ANDROID
#ifndef ARM
//#include <X11/Xlib.h>
#endif
#endif
#endif

int DemoFlag = 0;          // 1: pFace, 0: Normal samples

#define CHIP_NUM    2

// FTDI device name
char *ftdDeviceName1 = (char *)"0";
char *ftdDeviceName2 = (char *)"1";
char *ftdDeviceName3 = (char *)"2";
char *ftdDeviceName4 = (char *)"3";

char *emmcDeviceName1 =  (char *)"/dev/sg2";
char *emmcDeviceName2 =  (char *)"/dev/sg3";

typedef struct {
    char *VideoName;
    char *DispName;
    int x;
    int y;
    int w;
    int h;
} VIDEO_ARG, *pVIDEO_ARG;

typedef struct {
    int GnetType;
    int TestType;
    char *DeviceName;
    VIDEO_ARG VideoInfo;
} INPUT_ARG, *pINPUT_ARG;

char *device_name[] =
{
    (char *)"0", (char *)"1", (char *)"2", (char *)"3", (char *)"4", (char *)"5", (char *)"6", (char *)"7",
};

int test_type[] = {8, 8, 8, 8, 3, 3, 3, 3};

VIDEO_ARG demoVideo[] =
{
    {(char *)"All.mp4", (char *)"Video1", 0, 0, 700, 460},
    {(char *)"turtle1.mp4", (char *)"Video2", 710, 0, 700, 460},
    {(char *)"ski.mp4", (char *)"Video3", 0, 520, 700, 460},
    {(char *)"Beer.mp4", (char *)"Video4", 710, 520, 700, 460}
};

void *run_test_thread(void* arg);
void singleChip(int gnetType, int testType, char *dev);
void twoChips(int gnetType, int type1, int type2, char *dev1, char *dev2);
void multiChips(int gnetType, int num, int *type, char **dev);
int input_menu(int& gnetType);
int demo_menu(int& gnetType);

 
int MMMmammmmmmain(int argc, _TCHAR* argv[])
{
    int index = 0;
    int gnetType = 0;

#ifdef LINUX
#ifndef ANDROID    
    //XInitThreads();
#endif    
#endif

    if (DemoFlag)
    {
        index = demo_menu(gnetType);
    }
    else if (argc == 1)
    {
        index = input_menu(gnetType);
    }
    else if (argc == 3)
    {
        gnetType = argv[1][0] - '0';
        if (gnetType > 1 || gnetType < 0)
        {
            gnetType = 1;
        }
        index = argv[2][0] - '0';
        if (index > 5)
        {
            index = -1;
        }
    }
    else
    {
        printf("Usage: %s GnetType TestType.\n", argv[0]);
        return 0;
    }

    switch (index)
    {
    case -1:        // Exit
        break;
    case -2:        // Two chips
#ifdef USE_EMMC
        twoChips(gnetType, 3, 4, emmcDeviceName1, emmcDeviceName2);
#else
        twoChips(gnetType, 3, 4, ftdDeviceName1, ftdDeviceName2);
#endif
        break;
    case -3:        // Multi chips
        multiChips(gnetType, CHIP_NUM, test_type, device_name);
        break;
    default:        // Single chip
#ifdef USE_EMMC
        singleChip(gnetType, index, emmcDeviceName1);
#else
        singleChip(gnetType, index, ftdDeviceName1);
#endif        
        break;
    }

    return 0;
}

int input_menu(int& gnetType)
{
    int testIndex[] = {0, 1, 2, 3, -2, 5, -1};
    int index, gType = 0;

    // Input Gnet type.
    printf("\nPlease choose Gnet type (0:Gnet1, 1:Gnet3) - ");
    scanf_s("%d", &gType);
    if (gType >= 0 && gType <= 1)
    {
        gnetType = gType;
    }
    else
    {
        gnetType = 1;
    }

    printf("\n====================================\n");
    printf("|         GTI SDK samples          |\n");
    printf("|    ------------------------      |\n");
    printf("|  0. Single picture.              |\n");
    printf("|  1. Single video.                |\n");
    printf("|  2. Web camera.                  |\n");
    printf("|  3. Picture slide show.          |\n");
    printf("|  4. Multiple chips.              |\n");
    printf("|  5. Test CNN speed.              |\n");
    printf("|  6. Exit.                        |\n");
    printf("====================================\n");
    printf("Please input the number - ");
    scanf_s("%d", &index);
    if (index > 6)
    {
        index = 6;
    }
    return testIndex[index];
}

int demo_menu(int& gnetType)
{
    int demoIndex[] = {6,7,-3,2,-1};
    int index;

    gnetType = 1;

    printf("\n====================================\n");
    printf("|          GTI2801 Demo            |\n");
    printf("|    ------------------------      |\n");
    printf("|  0. Video Hierarchy.             |\n");
    printf("|  1. Test CNN speed.              |\n");
    printf("|  2. Two Chips.                   |\n");
    printf("|  3. Web camera.                  |\n");
    printf("|  4. Exit.                        |\n");
    printf("====================================\n");
    printf("Please input the number - ");
    scanf_s("%d", &index);
    if (index > 4)
    {
        index = 4;
    }
    return demoIndex[index];
}

void singleChip(int gnetType, int testType, char *dev)
{
    pthread_t tid;
    INPUT_ARG tArg;

    tArg.GnetType = gnetType;
    tArg.TestType = testType;
    tArg.DeviceName = dev;

    // Create thread
    pthread_create(&tid, NULL, run_test_thread, (void *)&tArg);

    pthread_join(tid, NULL);
}

void twoChips(int gnetType, int type1, int type2, char *dev1, char *dev2)
{
    pthread_t tid1, tid2;
    INPUT_ARG tArg1, tArg2;

    tArg1.GnetType = gnetType;
    tArg1.TestType = type1;
    tArg1.DeviceName = dev1;
    tArg2.GnetType = gnetType;
    tArg2.TestType = type2;
    tArg2.DeviceName = dev2;

    // Create thread
    pthread_create(&tid1, NULL, run_test_thread, (void *)&tArg1);
    pthread_create(&tid2, NULL, run_test_thread, (void *)&tArg2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
}

void multiChips(int gnetType, int num, int *type, char **dev)
{
    pthread_t tid[16];
    INPUT_ARG tArg[16];

    if (num > 16 || num < 1)
    {
        return;
    }
    for (int i = 0; i < num; i++)
    {
        tArg[i].GnetType = gnetType;
        tArg[i].TestType = type[i];
        tArg[i].DeviceName = dev[i];
        tArg[i].VideoInfo = demoVideo[i];
    }

    // Create thread
    for (int i = 0; i < num; i++)
    {
        pthread_create(&tid[i], NULL, run_test_thread, (void *)&tArg[i]);
    }

    for (int i = 0; i < num; i++)
    {
        pthread_join(tid[i], NULL);
    }
}

void *run_test_thread(void* arg)
{
    // pINPUT_ARG pInArg = (pINPUT_ARG)arg;
    // int gnetType = pInArg->GnetType;
    // int testType = pInArg->TestType;
    // char *deviceName = pInArg->DeviceName;
    // Samples test(gnetType, deviceName);

    // printf("Start test type %d.\n", testType);

    // switch (testType)
    // {
    // case 0:
    //     test.Test_picture();
    //     break;
    // case 1:
    //     test.Test_video();
    //     break;
    // case 2:
    //     test.Test_webcam();
    //     break;
    // case 3:
    //     test.Slideshow_picture(0);
    //     break;
    // case 4:
    //     test.Slideshow_video();
    //     break;
    // case 5:
    //     test.Test_CnnSpeed(DemoFlag);
    //     break;
    // case 6:
    //     test.Video_Hierarchy(DemoFlag);
    //     break;
    // case 7:
    //     test.Test_CnnSpeed_video();
    //     break;
    // case 8:
    //     test.Demo_video(pInArg->VideoInfo.VideoName, pInArg->VideoInfo.DispName,
    //          pInArg->VideoInfo.x, pInArg->VideoInfo.y, pInArg->VideoInfo.w, pInArg->VideoInfo.h);
    //     break;
    // case 9:
    //     test.Demo_picture(pInArg->VideoInfo.DispName,
    //          pInArg->VideoInfo.x, pInArg->VideoInfo.y, pInArg->VideoInfo.w, pInArg->VideoInfo.h);
    //     break;
    // default:
    //     break;
    // }
    // printf("End test type %d.\n", testType);

    return arg;
}

