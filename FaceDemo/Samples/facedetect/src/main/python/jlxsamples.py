#!/usr/bin python3
# -*- coding: utf-8 -*-

import ctypes
from ctypes import cdll


class SamplesContext(ctypes.Structure):
    pass


SamplesContextPtr = ctypes.POINTER(SamplesContext)

c_char_array64 = ctypes.c_char*64


class PersonInfo(ctypes.Structure):
    _fields_ = [('name', c_char_array64),
                ('similar', ctypes.c_float)
                ]


class FaceRecognizer:
    def __init__(self, net_type):
        self.load_sqlite_lib_path = str("libsqlite3.so")
        self.load_samples_lib_path = str("libjlx_classify.so")
        cdll.LoadLibrary(self.load_sqlite_lib_path)
        self.lib_ptr = cdll.LoadLibrary(self.load_samples_lib_path)

        # new Samples object
        self.c_samples_object = self.lib_ptr.samples_new
        self.c_samples_object.argtypes = [ctypes.c_int]
        self.c_samples_object.restype = SamplesContextPtr

        self.c_samples_object_ptr = self.c_samples_object(int(net_type))

        #  c function table
        self.c_samples_object_function_sdk_step = self.lib_ptr.sdk_step
        self.c_samples_object_function_set_pack_path = self.lib_ptr.set_pack_path
        self.c_samples_object_function_prediction = self.lib_ptr.prediction
        self.c_samples_object_function_get_init_result = self.lib_ptr.get_init_Result
        self.c_samples_object_function_face_predict = self.lib_ptr.face_predict
        self.c_samples_object_function_face_register = self.lib_ptr.face_regist
        #  arg types and restype

        self.c_samples_object_function_sdk_step.argtypes = [SamplesContextPtr]
        self.c_samples_object_function_sdk_step.restype = ctypes.c_int

        self.c_samples_object_function_set_pack_path.argtypes = [SamplesContextPtr, ctypes.c_char_p]
        self.c_samples_object_function_set_pack_path.restype = ctypes.c_int

        self.c_samples_object_function_prediction.argtypes = [SamplesContextPtr, ctypes.c_int]
        self.c_samples_object_function_prediction.restype = ctypes.c_int

        self.c_samples_object_function_get_init_result.argtypes = [SamplesContextPtr]
        self.c_samples_object_function_get_init_result.restype = ctypes.c_bool

        self.c_samples_object_function_face_predict.argtypes = [SamplesContextPtr, ctypes.c_char_p]
        self.c_samples_object_function_face_predict.restype = PersonInfo

        self.c_samples_object_function_face_register.argtypes = [SamplesContextPtr, ctypes.c_char_p, ctypes.c_char_p]
        self.c_samples_object_function_face_register.restype = ctypes.c_int
        pass

    def init_sdk_step(self):
        return self.c_samples_object_function_sdk_step(self.c_samples_object_ptr)
        pass

    def set_pack_path(self):
        return self.c_samples_object_function_set_pack_path(self.c_samples_object_ptr, None)
        pass

    def prediction(self, network_type):
        return self.c_samples_object_function_prediction(self.c_samples_object_ptr, int(network_type))
        pass

    def get_init_result(self):
        return self.c_samples_object_function_get_init_result(self.c_samples_object_ptr)
        pass

    def face_predict(self, face):
        return self.c_samples_object_function_face_predict(self.c_samples_object_ptr, face)
        pass

    def face_register(self, face, name):
        reg = ctypes.c_char_p(name.encode('utf-8'))
        return self.c_samples_object_function_face_register(self.c_samples_object_ptr, face, reg)
        pass


