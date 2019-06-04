#include "Classify.hpp"
#include "Samples.hpp"
#include "Threadpool.hpp"
#include "GtiClassify.h"
#include <math.h>
#include <unistd.h>

//====================================================================
// Function name: void convolution(const signed char* img, const int imgLength)
// This function calls GTISendImage() to send image to GTI FPGA to do
// convolution.
//
// Input: img - 224 pixel x 224 pixel x 3 channels image data.
// return: none.
//====================================================================
int Samples::convolution(BYTE* img, const int imgLength) {
    int Ret = 0;

    int gtiResult = GtiHandleOneFrameFloat(m_Device, img, imgLength, m_ImageOutPtr, GtiGetOutputLength(m_Device));
    if (gtiResult == JLX_GTI_CON_ERROR_CODE)
    {
#if DEBUG_OUT_ERROR
        LOG(ERROR) << "GTI_JNI: convolution error.";
#endif
        GtiCloseDevice(m_Device);
        GtiOpenDevice(m_Device, m_deviceName);
        Ret = -95;
    }
#if DEBUG_OUT_INFO
    LOG(INFO) << "GTI_JNI: convolution    is end";
#endif
    return Ret;
}

//====================================================================
// Function name: void convolution(const cv::Mat& img)
// This function calls GTISendImage() to send image to GTI FPGA to do
// convolution.
//
// Input: Mat& img - 224 pixel x 224 pixel x 3 channels image data.
// return: int.
//====================================================================
int Samples::convolution(const cv::Mat& img) {
    int bRet = 0;
    int ConvIn_Channels = 3;
    int ConvIn_ImgSize = 224;
    int imageInLength = (ConvIn_ImgSize * ConvIn_ImgSize) * ConvIn_Channels;
    std::vector<cv::Mat> input_image;

    //cnnSvicProc8UC3(img, &input_image);
    cnnSvicProc32FC3(img, &input_image);
    cvt32FloatTo8Byte((float *)input_image[0].data, (uchar *)m_Img224x224x3Buffer, ConvIn_ImgSize, ConvIn_ImgSize, ConvIn_Channels);
#if OUTINFO_SPEND
    m_PureCnnStart = GetTickCount64();
#endif
    int gtiResult = GtiHandleOneFrameFloat(m_Device, m_Img224x224x3Buffer, imageInLength, m_ImageOutPtr, GtiGetOutputLength(m_Device));
#if OUTINFO_SPEND
    m_PureCnnEnd = GetTickCount64();
#endif
    if (gtiResult == JLX_GTI_CON_ERROR_CODE)
    {
#if DEBUG_OUT_ERROR
        LOG(ERROR) << "GTI_JNI: convolution error.";
#endif
        GtiCloseDevice(m_Device);
        GtiOpenDevice(m_Device, m_deviceName);
        bRet = -95;
    }
    return bRet;
}

//====================================================================
// Function name: void cvt32FloatTo8Byte(float *InBuffer, uchar *OutBuffer,
//                      int Width, int Height, int Channels)
// This function converts 32 bit float format image to 8 bit integer
// format image.
//
// Input: float *InBuffer - input 32 bits/pixel data buffer.
//        uchar *OutBuffer - output buffer to store 8 bits/pixel image data.
//        int Width - image width in pixel
//        int Height - image height in pixel
//        int Channels - image channels
// return: none.
//====================================================================
void Samples::cvt32FloatTo8Byte(float *InBuffer, uchar *OutBuffer,
                  int Width, int Height, int Channels) {
    uchar *pOut = OutBuffer;
    float *pIn = InBuffer;
    if (pIn == NULL || pOut == NULL)
    {
        std::cout << "cvt32FloatTo8Byte: null pointer!" << std::endl;
        return;
    }

    for (int i = 0; i < Channels; i++)
    {
        for (int j = 0; j < Height; j++)
        {
            for (int k = 0; k < Width; k++)
            {
                *pOut++ = (uchar)*pIn++;
            }
        }
    }
}

//====================================================================
// Function name: void cnnSvicProc8UC3(const cv::Mat& img,
//                          std::vector<cv::Mat>* input_channels)
// This function converts input image to the float point 3 channel image.
//
// Input: const cv::Mat& img - input image.
//        std::vector<cv::Mat>* input_channels - output buffer to store
//              float point 3 channel image.
// return: none.
//====================================================================
void Samples::cnnSvicProc8UC3(const cv::Mat& img, std::vector<cv::Mat>*input_channels) {
    int width = 224;
    int height = 224;
    int num_channels_ = 3;
    m_imgSize.width = width;
    m_imgSize.height = height;
    unsigned char *input_data = (unsigned char *)m_Buffer32FC3;

    for (int i = 0; i < num_channels_; ++i)
    {
        cv::Mat channel(height, width, CV_8UC1, input_data);
        input_channels->push_back(channel);
        input_data += width * height;
    }

    /* Convert the input image to the input image format of the network. */
    cv::Mat sample;
    if (img.channels() == 4) // && num_channels_ == 3)
        cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
    else if (img.channels() == 1) // && num_channels_ == 3)
        cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
    else
        sample = img;

    cv::Mat sample_resized;
    if (sample.size() != m_imgSize)
        cv::resize(sample, sample_resized, m_imgSize);
    else
        sample_resized = sample;

    cv::Mat sample_uchar;
    sample_resized.convertTo(sample_uchar, CV_8UC3);

    cv::Mat sample_normalized;
    cv::subtract(sample_uchar, cv::Scalar(0., 0., 0.), sample_normalized); // Zero

    /* This operation will write the separate BGR planes directly to the
    * input layer of the network because it is wrapped by the cv::Mat
    * objects in input_channels. */
    cv::split(sample_normalized, *input_channels);
}
//====================================================================
// Function name: void cnnSvicProc32FC3(const cv::Mat& img,
//                          std::vector<cv::Mat>* input_channels)
// This function converts input image to the float point 3 channel image.
//
// Input: const cv::Mat& img - input image.
//        std::vector<cv::Mat>* input_channels - output buffer to store
//              float point 3 channel image.
// return: none.
//====================================================================
void Samples::cnnSvicProc32FC3(const cv::Mat& img, std::vector<cv::Mat>* input_channels) {
    cv::Mat sample;
    cv::Mat sample_resized;
    cv::Mat sample_byte;
    cv::Mat sample_normalized;

    const int num_channels = GTI_IMAGE_CHANNEL;
    int width = GTI_IMAGE_WIDTH;
    int height = GTI_IMAGE_HEIGHT;
    float *input_data = m_Buffer32FC3;
    if (input_data == NULL)
    {
        std::cout << "Failed allocat memory for input_data!" << std::endl;
        return;
    }

    for (int i = 0; i < num_channels; ++i)
    {
        cv::Mat channel(height, width, CV_32FC1, input_data);
        input_channels->push_back(channel);
        input_data += width * height;
    }

    sample = img;
    switch (img.channels())
    {
    case 1:
        if (num_channels == 3)
        {
            cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
        }
        break;
    case 3:
        if (num_channels == 1)
        {
            cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
        }
        break;
    case 4:
        if (num_channels == 1)
        {
            cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
        }
        else if (num_channels == 3)
        {
            cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
        }
        break;
    default:
        break;
    }

    sample_resized = sample;

    if (num_channels == 3)
    {
        sample_resized.convertTo(sample_byte, CV_32FC3);
    }
    else
    {
        sample_resized.convertTo(sample_byte, CV_32FC1);
    }

    cv::subtract(sample_byte, cv::Scalar(0., 0., 0.), sample_normalized);

    /* This operation will write the separate BGR planes directly to the
     * input layer of the network because it is wrapped by the cv::Mat
     * objects in input_channels. */
    cv::split(sample_normalized, *input_channels);
}