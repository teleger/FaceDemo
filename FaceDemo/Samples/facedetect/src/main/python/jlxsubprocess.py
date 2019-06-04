#!/usr/bin python3
# -*- coding: utf-8 -*-

import queue
import multiprocessing
import ctypes
import numpy as np
import time
import threading


class JlxProcess:
    # data 224*224 图像队列, name_queue 名字队列 , predict_queue 识别结果 队列
    def __init__(self,  recognizer, data=None, name_queue=None, predict_queue=None):
        self.recognizer = recognizer
        # 数据队列为 none (进程间)
        self.data_queue = data
        self.register_thread = None
        self.register_name_queue = name_queue
        self.predict_queue = predict_queue
        # 线程 队列
        self.q = queue.Queue(1)

        self.process = multiprocessing.Process(target=self.process_work)
        self.process.daemon = True
        self.process.start()
        pass

    # 进程运行,入口
    def process_work(self):
        if self.recognizer:
            if self.recognizer.init_sdk_step() == 0:
                if self.recognizer.set_pack_path() == 0:
                    if self.recognizer.prediction(3) == 0:
                        # 开启 人脸注册 线程
                        self.register_thread = threading.Thread(target=self.face_register_thread)
                        self.register_thread.setDaemon(True)
                        self.register_thread.start()
                        # 人脸 识别
                        self.recognition_face()
                    else:
                        print('prediction failed')
                        exit(1)
                else:
                    print('set_pack_path failed')
                    exit(1)
            else:
                exit(1)
        pass

    # 人脸 识别
    def face_recognition(self, frame):
        data = np.asarray(frame, dtype=np.uint8)
        data = data.ctypes.data_as(ctypes.c_char_p)
        # 如果可以识别
        return self.recognizer.face_predict(data)

    # 子进程 循环 （识别）
    def recognition_face(self):
        if self.data_queue is None:
            print('data_queue is None')
            exit(1)
        while True:
            if not self.data_queue.empty():
                queue_frame = self.data_queue.get()
                if not self.q.full():
                    self.q.put_nowait(queue_frame)
                predict_name = self.face_recognition(queue_frame)
                name = predict_name.name.decode("utf-8")
                print("predict name ", name)
                if self.predict_queue is None:
                    pass
                else:
                    if name != '' and not self.predict_queue.full():
                        self.predict_queue.put_nowait(name)
            else:
                time.sleep(0.02)
                pass
        pass

    # 将 注册人脸数据类型 准换为 c 类型 （调用so 库的c/c++ 函数）
    def face_register(self, face, name):
        data = np.asarray(face, dtype=np.uint8)
        data = data.ctypes.data_as(ctypes.c_char_p)
        # 如果可以识别
        return self.recognizer.face_register(data, name)
        pass

    # 注册 人脸
    def face_register_thread(self):
        if self.register_name_queue is None:
            return None
        while True:
            if not self.register_name_queue.empty() and not self.q.empty():
                cropped_frame = self.q.get_nowait()
                register_new_name = self.register_name_queue.get_nowait()
                result = self.face_register(cropped_frame, register_new_name)
                print('register result :', result)
                pass
            else:
                time.sleep(0.2)
            pass
        pass

