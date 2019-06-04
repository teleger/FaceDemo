#!/usr/bin python3
# -*- coding: utf-8 -*-
import numpy as np
import cv2
# if py2 Tkinter
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
# if py2 tkFont
import tkinter.font as tkfont
import ctypes

import multiprocessing

import time
from mtcnn import FaceDetector
from mtcnn import Box
from jlxsamples import *
from jlxsubprocess import JlxProcess

# show rect canvas
image_width = 640
image_height = 480


class Gui:
    def __init__(self, detector=None, recognizer=None, num_of_cam=1):
        self.num_of_camera = num_of_cam
        self.mCanvas = None
        self.videoId = None

        self.process_q = multiprocessing.Queue(1)
        # Set up GUI
        self.window = tk.Tk()
        self.window.resizable(width=False, height=False)
        self.text_str_var = tk.StringVar()
        # Makes main window
        self.window.wm_title("Face Recognition")
        self.window.config(background="#FFFFFF")
        self.main_frame = tk.Frame(self.window, width=640, height=480)
        self.main_frame.grid(row=0, column=0, padx=0, pady=0, rowspan=5)
        self.cap = cv2.VideoCapture(0)
        # ..
        self.face_recognition_running = False
        self.face_detection_running = False
        #
        self.frame_list = []
        self.num_of_photos = 0
        self.register_face_num = 5
        self.detector = detector
        self.recognizer = recognizer
        self.camera_width = 0
        self.camera_height = 0
        # ..
        self.register_new_name = ''
        self.cropped_frame_list = []
        # 注册 人脸
        self.face_registering = False
        self.detect_has_face_num = 0
        self.process_register_name_q = multiprocessing.Queue(1)
        # 识别
        self.cropped_frame_recognition_list = []
        self.index_save = 0
        self.predict_q = multiprocessing.Queue(1)
        # work
        self.init_main_other_work()
        pass

    def config_layout(self):
        self.videoId = tk.Label(self.main_frame, width=640, height=480)
        self.videoId.grid(row=0, column=0, padx=0, pady=0, rowspan=5)

        self.sliderframe = tk.Frame(self.window)
        self.sliderframe.grid(row=0, column=1, rowspan=5, sticky='nswe')

        self.predict_name_label = tk.Label(self.sliderframe, textvariable=self.text_str_var, fg='red',
                                           font=("Arial", 60))
        self.predict_name_label.grid(row=0, column=1, sticky='nswe')

        # 输入框 背景字体为 red
        self.m_entry = tk.Entry(self.sliderframe, relief="raised", fg='red', font='helvetica 26')
        self.m_entry.grid(row=1, column=1, sticky='nswe')

        register_bt = ttk.Button(self.sliderframe, text='FaceRec',
                                 style='SunkableButton.TButton', command=self.register_face)
        register_bt.grid(row=2, column=1, padx=0, pady=5, sticky='nswe')

        manage_bt2 = ttk.Button(self.sliderframe, text='FaceManage', style='SunkableButton.TButton')
        manage_bt2.grid(row=3, column=1, padx=0, pady=5, sticky='nswe')

        self.cropframe = tk.Frame(self.sliderframe, width=240, height=240)
        self.cropframe.grid(row=4, column=1, padx=3, pady=2, sticky='nswe')

        self.croplabel = tk.Label(self.cropframe)
        self.croplabel.pack()
        pass

    # 配置camera相关参数，预览大小，fps设置
    def camera_configure(self):
        self.cap.set(cv2.CAP_PROP_FPS, 30)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, image_width)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, image_height)
        self.camera_width = image_width
        self.camera_height = image_height
        pass

    def run(self):
        self.window.mainloop()
        self.cap.release()
        pass

    def init_main_other_work(self):
        if self.init_detect_other_work():
            JlxProcess(self.recognizer, data=self.process_q,
                       name_queue=self.process_register_name_q,
                       predict_queue=self.predict_q)
            pass
        pass

    def init_detect_other_work(self):
        if self.detector:
            self.face_detection_running = True
            self.detector.set_thread_num(2)
            return True
        else:
            return False
        pass

    # 保存 用户 输入数据
    def save_face_data(self):
        # 获取输入框的数据
        person_name = self.m_entry.get()
        if person_name != '':
            self.register_new_name = person_name
            return True
            pass
        return False
        pass

    def detect_face(self, frame):
        data = np.asarray(frame, dtype=np.uint8)
        data = data.ctypes.data_as(ctypes.c_char_p)
        return self.detector.detect(data, self.camera_width, self.camera_height)

    # 显示识别 结果
    def show_predict_name(self):
        if not self.predict_q.empty():
            predict_name = self.predict_q.get_nowait()
            self.text_str_var.set(predict_name)
        pass

    # 单独 显示 人脸
    def show_face_frame(self):
        if self.face_detection_running:
            if len(self.cropped_frame_list) == 0:
                pass
            if len(self.cropped_frame_list) > 0:
                self.show_predict_name()
                face_image = cv2.cvtColor(self.cropped_frame_list[0], cv2.COLOR_BGR2RGBA)
                self.cropped_frame_list.pop(0)
                face_img = Image.fromarray(face_image)
                face_img_tk = ImageTk.PhotoImage(image=face_img)
                self.croplabel.imgtk = face_img_tk
                self.croplabel.configure(image=face_img_tk)
                self.croplabel.after(10, self.show_face_frame)
            else:
                # 没有 人脸清空 label 数据
                self.croplabel.configure(image='')
                self.croplabel.after(10, self.show_face_frame)
        pass

    # 查找人脸 框出人脸
    def crop_face_frame(self, frame):
        if self.detect_has_face_num % 20 == 0:
            has_face = self.detect_face(frame)
            if has_face is None:
                pass
            else:
                if has_face.y1 != 0 and has_face.y2 != 0:
                    cropped_image = frame[has_face.y1:has_face.y2, has_face.x1:has_face.x2]

                    cropped_image_resize = cv2.resize(cropped_image, (224, 224))

                    self.cropped_frame_list.append(cropped_image_resize)
                    if not self.process_q.full():
                        self.process_q.put_nowait(cropped_image_resize)

                    cv2.rectangle(frame, (has_face.x1, has_face.y1), (has_face.x2, has_face.y2), (0, 255, 0), 2)
                    return True
                else:
                    # 无人脸时,清空 显示结果标签
                    self.text_str_var.set(str(''))
                    self.detect_has_face_num += 1
                    pass
        else:
            self.detect_has_face_num += 1
        if self.detect_has_face_num >= 40000:
            self.detect_has_face_num = 0
            pass
        return False

    # 显示预览camera 图像
    def show_frame(self):
        ret, frame = self.cap.read()
        if ret:
            if frame.any():
                frame = cv2.flip(frame, 1)
                if self.face_detection_running:
                    self.crop_face_frame(frame)
                cv2image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGBA)
                img = Image.fromarray(cv2image)
                imgtk = ImageTk.PhotoImage(image=img)
                self.videoId.imgtk = imgtk
                self.videoId.configure(image=imgtk)
                self.videoId.after(10, self.show_frame)
            else:
                exit()
        else:
            exit()

    # 注册 人脸 函数
    def register_face(self):
        self.save_face_data()
        self.m_entry.delete(0, tk.END)
        if self.recognizer is None:
            return None
        # 正在 注册 人脸
        self.face_registering = True
        print('register new name: ', self.register_new_name)
        if not self.process_register_name_q.full():
            self.process_register_name_q.put_nowait(self.register_new_name)
        return True
        pass


# please read me...
main_run = Gui(detector=FaceDetector(), recognizer=FaceRecognizer(0))
main_run.config_layout()
main_run.camera_configure()
main_run.show_frame()
main_run.show_face_frame()
main_run.run()
