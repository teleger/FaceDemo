//
// Created by menethis on 18-9-19.
//

#ifndef FACERECOGNITIONSDKPROJECT_JLXDEFINE_HPP
#define FACERECOGNITIONSDKPROJECT_JLXDEFINE_HPP

#include <array>


#ifdef LINUX_C_DEBUG
typedef struct FontColor{
    const char yellow[16]="\033[1;33m";
    const char red[16]="\033[0;31m";
    const char green[16]="\033[0;32m";
    const char end[8]="\033[0m";
}JlxfontColor;
#endif

typedef  std::array<float,4096> MyJLXArray;
#endif //FACERECOGNITIONSDKPROJECT_JLXDEFINE_HPP
