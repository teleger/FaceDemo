/***************************************************************
 * GtiConfig.h
 * GTI2801 configuration header file.
 *
 *                Copyrights, Gyrfalcon technology Inc. 2018
 ***************************************************************/
#pragma once

class GtiConfig;

extern "C"
{
    GtiConfig *GtiConfigCreate();
    void GtiGenerateFilter(GtiConfig *config, char *networkFile, char *outFile);
    void GtiGenerateFilterFromMem(GtiConfig *config, int Blocks, unsigned int *Filter_In, char *networkFile, char *outFile);
    void GtiConfigRelease(GtiConfig *config);
}
