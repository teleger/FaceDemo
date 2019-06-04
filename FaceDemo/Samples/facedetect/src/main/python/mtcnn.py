#!/usr/bin python3
# -*- coding: utf-8 -*-
import sys

import ctypes
from ctypes import cdll
import cv2
import numpy as np


class Context(ctypes.Structure):
    pass


ContextPtr = ctypes.POINTER(Context)

c_float_array10 = ctypes.c_float*10

c_float_array4 = ctypes.c_float*4


class Box(ctypes.Structure):
    _fields_ = [('score', ctypes.c_float),
                ('x1', ctypes.c_int),
                ('y1', ctypes.c_int),
                ('x2', ctypes.c_int),
                ('y2', ctypes.c_int),
                ('area', ctypes.c_float),
                ('ppoint', c_float_array10),
                ('regreCoord', c_float_array4),
                ]


class FaceDetector:
    def __init__(self):
        self.load_lib_path = str("libmtcnn.so")
        self.lib_ptr = cdll.LoadLibrary(self.load_lib_path)
        # 创建字符串
        self.path = ctypes.create_string_buffer(b"/home/work/Jlx/mtcnn/models")
        self.c_object = self.lib_ptr.MTCNN_new
        # 指定必需的参数类型（函数原型）
        self.c_object.argtypes = [ctypes.c_char_p]
        self.c_object.restype = ContextPtr
        self.c_object_ptr = self.c_object(self.path)
        # 探测函数
        self.c_object_function_detect = self.lib_ptr.c_detect_2
        # 设置线程
        self.c_object_function_setThread = self.lib_ptr.c_setNumber
        # 人脸
        self.c_object_function_setMinFace = self.lib_ptr.c_setMinface

        # print(dir(self.c_object))
        # 调用构造函数,传参
        self.c_object_function_detect.argtypes = [ContextPtr,
                                                  ctypes.c_char_p,
                                                  ctypes.c_int,
                                                  ctypes.c_int
                                                  ]
        self.c_object_function_detect.restype = Box
        self.c_object_function_setThread.argtypes = [ContextPtr, ctypes.c_int]
        self.c_object_function_setThread.restype = ctypes.c_bool
        self.c_object_function_setMinFace.argtypes = [ContextPtr, ctypes.c_int]
        self.c_object_function_setMinFace.restype = ctypes.c_bool
        pass

    def detect(self, face, width, height):
        return self.c_object_function_detect(self.c_object_ptr, face, width, height)
        pass

    def set_thread_num(self, thread_num):
        return self.c_object_function_setThread(self.c_object_ptr, thread_num)
        pass

    def set_min_face(self, min_face_size):
        return self.c_object_function_setMinFace(self.c_object_ptr, min_face_size)
        pass


# m = FaceDetector()
#
# frame = cv2.imread("time.jpeg")
#
# data = np.asarray(frame, dtype=np.uint8)
# data = data.ctypes.data_as(ctypes.c_char_p)
#
# m.detect(data, frame.shape[1], frame.shape[0])
#
# m.set_thread_num(2)
