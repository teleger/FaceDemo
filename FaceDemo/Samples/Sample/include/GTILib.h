/***************************************************************
 * GTILib.h
 * GTI header file, it includes the public functions in GTI SDK
 * library.
 *
 *                Copyrights, Gyrfalcon technology Inc. 2017
 ***************************************************************/
#pragma once

class GtiDevice;

extern "C"
{
    GtiDevice *GtiDeviceCreate(int DeviceType, char *FilterFileName, char *ConfigFileName);
    void GtiDeviceRelease(GtiDevice *Device);
    unsigned int GtiGetSDKVersion(GtiDevice *Device);
    int GtiOpenDevice(GtiDevice *Device, char *DeviceName);
    void GtiCloseDevice(GtiDevice *Device);
    void GtiSelectNetwork(GtiDevice *Device, int NetworkId);
    int GtiInitialization(GtiDevice *Device);
    int GtiSendImage(GtiDevice *Device, unsigned char *Image224Buffer, unsigned int BufferLen);
    int GtiSendImageFloat(GtiDevice *Device, float *Image224Buffer, unsigned int BufferLen);
    int GtiGetOutputData(GtiDevice *Device, unsigned char *OutputBuffer, unsigned int BufferLen);
    int GtiGetOutputDataFloat(GtiDevice *Device, float *OutputBuffer, unsigned int BufferLen);
    int GtiHandleOneFrame(GtiDevice *Device, unsigned char *InputBuffer, unsigned int InputLen,
                            unsigned char *OutputBuffer, unsigned int OutLen);
    int GtiHandleOneFrameFloat(GtiDevice *Device, unsigned char *InputBuffer, unsigned int InputLen,
                            float *OutputBuffer, unsigned int OutLen);
    unsigned int GtiGetOutputLength(GtiDevice *Device);
}
