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
    GtiDevice *GtiDeviceCreate(int DeviceType, int GnetType, int CnnMode);
    void GtiDeviceRelease(GtiDevice *Device);
    int GtiOpenDevice(GtiDevice *Device, char *DeviceName);
    void GtiCloseDevice(GtiDevice *Device);
    void GtiSetPool5(GtiDevice *Device, int PoolingOn);
    int GtiInitialization(GtiDevice *Device, char *GtiCoefFileName);
    int GtiSendImage(GtiDevice *Device, unsigned char *Image224Buffer, unsigned int BufferLen);
    int GtiSendImageFloat(GtiDevice *Device, float *Image224Buffer, unsigned int BufferLen);
    int GtiGetOutputData(GtiDevice *Device, float *OutputBuffer, unsigned int BufferLen);
    int GtiHandleOneFrame(GtiDevice *Device, unsigned char *InputBuffer, unsigned int InputLen,
                            float *OutputBuffer, unsigned int OutLen);
    int GtiHandleOneFrameFloat(GtiDevice *Device, float *InputBuffer, unsigned int InputLen,
                            float *OutputBuffer, unsigned int OutLen);
    unsigned int GtiGetOutputLength(GtiDevice *Device);
}
