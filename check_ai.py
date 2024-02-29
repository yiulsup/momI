from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtCore import QTimer, QThread
from PyQt5 import uic
import sys
import serial
import numpy as np 
import os
import time
import threading
import signal
import time
import binascii
import queue
import cv2


class radar(QThread):
    def __init__(self, radar_string):
        super().__init__()
        self.radar_string = radar_string
        

    def run(self):
        while True:
            try:
                self.fp1 = open("breathing.txt", "r")
                data_string = self.fp1.read()
                self.radar_string.setText(data_string)
                time.sleep(0.01)
            except:
                continue
            time.sleep(1)

class thermal(QThread):
    def __init__(self, vision_widget):
        super().__init__()
        self.vision_widget = vision_widget
        self.raw = np.zeros((80*60), np.int32)
        for i in range(0, 4800):
            self.raw[i] = 1650

        
        self.YOLO_net = cv2.dnn.readNet("thermal_yolov2-tiny.weights", "thermal_yolov2-tiny.cfg")

        self.classes = []
        with open("thermal.names", "r") as f:
            self.classes = [line.strip() for line in f.readlines()]
        layer_names = self.YOLO_net.getLayerNames()
        self.output_layers = [layer_names[i - 1] for i in self.YOLO_net.getUnconnectedOutLayers()]

    def run(self):
        
        while True:
            try:
                with open('raw1.txt', 'r') as file:
                    data_string = file.read().strip()
            # Split data based on spaces and convert to integers

                numbers = data_string.split()

                raw_dat = [int(num) for num in numbers]      

                if len(raw_dat) < 4770:
                    continue
                for i in range(0, 4770):
                    try:
                        self.raw[i] = raw_dat[i]
                    except:
                        continue

                image = self.raw.reshape(60, 80)
                image1 = image[2:58, :60]
                image2 = image1.reshape(-1)

                max = np.max(image2)
                min = np.min(image2)
                nfactor = 255 / (max - min)
                pTemp = image2 - min
                nTemp = pTemp * nfactor 
                frame = nTemp
                image2 = frame.reshape(56, 60)        
                
                uint_img = np.array(image2).astype('uint8')
                grayImage = cv2.cvtColor(uint_img, cv2.COLOR_GRAY2BGR)
                grayImage = cv2.resize(grayImage, (320, 320))

                h, w, c = grayImage.shape
                blob = cv2.dnn.blobFromImage(grayImage, 0.00392, (160, 160), (0, 0, 0), True, crop=False)
                img = cv2.resize(uint_img, dsize=(160, 160))

                self.YOLO_net.setInput(blob)
                outs = self.YOLO_net.forward(self.output_layers)

                class_ids = []
                confidences = []
                boxes = []
                x = 0
                y = 0
                dw = 0
                dh = 0
                label = "None"
                
                for out in outs:
                    for detection in out:
                        scores = detection[5:]
                        class_id = np.argmax(scores)
                        confidence = scores[class_id]
                        if confidence > 0.8:
                            # Object detected
                            center_x = int(detection[0] * w)
                            center_y = int(detection[1] * h)
                            dw = int(detection[2] * w)
                            dh = int(detection[3] * h)
                            # Rectangle coordinate
                            x = int(center_x - dw / 2)
                            y = int(center_y - dh / 2)
                            boxes.append([x, y, dw, dh])
                            confidences.append(float(confidence))
                            class_ids.append(class_id)

                indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.19, 0.4)

                if len(class_ids) != 0:
                    for i in range(len(boxes)):
                        if i in indexes:
                            x, y, w, h = boxes[i]
                            label = str(self.classes[class_ids[i]])
                            score = confidences[i]

                            #print("{} : temp : {}: {}, {}, {}, {}".format(label, high, x, y, w, h))
                            # 경계상자와 클래스 정보 이미지에 입력
                            grayImage = cv2.rectangle(grayImage, (x, y), (x + w, y + h), (0, 0, 255), 1)
                            grayImage = cv2.rectangle(grayImage, (x, y), (x + 50, y - 13), (0, 0, 255), -1)
                            grayImage = cv2.putText(grayImage, label, (x, y - 5), cv2.FONT_ITALIC, 0.3, (255, 0, 0), 1)

                else:
                    pass
                    #errint1 = int(frame1[0])
                    #errint2 = int(frame1[1])
                    #errint3 = int(frame1[2])
                    #if errint1 == errint2:
                    #    continue
                    #print("{} : {}, {}, {}, {}".format(label, x, y, dw, dh))

                frame1 = cv2.resize(grayImage, (320, 320))
                h, w, ch = frame1.shape
                bytes_per_line = ch * w
                convert_to_Qt_format = QImage(frame1.data, w, h, bytes_per_line, QImage.Format_RGB888)
                qpixmap = QPixmap.fromImage(convert_to_Qt_format)
                self.vision_widget.setPixmap(qpixmap)
                
                self.vision_widget.show()
                time.sleep(0.01)
            except:
                continue    

class vision(QThread):
    def __init__(self, cap, vision_widget):
        super().__init__()
        self.cap = cap
        self.vision_widget = vision_widget
        self.YOLO_net = cv2.dnn.readNet("yolov3-tiny.weights", "yolov3-tiny.cfg")

        # YOLO NETWORK 재구성
        self.classes = []
        with open("obj.names", "r") as f:
            self.classes = [line.strip() for line in f.readlines()]
        layer_names = self.YOLO_net.getLayerNames()

        for i in self.YOLO_net.getUnconnectedOutLayers():
            self.output_layers = [layer_names[i-1] for i in self.YOLO_net.getUnconnectedOutLayers()]

        self.cnt = 0

    def run(self):
        while True:
            self.cnt = self.cnt + 1
            if self.cnt != 17:
                continue
            try:
                ret, frame = self.cap.read()
                frame = cv2.resize(frame, (320, 640))
      
            # YOLO 입력
                blob = cv2.dnn.blobFromImage(frame, 0.00392, (416, 416), (0, 0, 0), True, crop=False)
                self.YOLO_net.setInput(blob)
                outs = self.YOLO_net.forward(self.output_layers)

                class_ids = []
                confidences = []
                boxes = []

                for out in outs:
                    for detection in out:
                        scores = detection[5:]
                        class_id = np.argmax(scores)
                        confidence = scores[class_id]

                        if confidence > 0.45:
                            # Object detected
                            center_x = int(detection[0] * w)
                            center_y = int(detection[1] * h)
                            dw = int(detection[2] * w)
                            dh = int(detection[3] * h)
                        # Rectangle coordinate
                            x = int(center_x - dw / 2)
                            y = int(center_y - dh / 2)
                            boxes.append([x, y, dw, dh])
                            confidences.append(float(confidence))
                            class_ids.append(0)

                indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.4, 0.5)
                for i in range(len(boxes)):
                    if i in indexes:
                        x, y, w, h = boxes[i]
                        label = str(self.classes[class_ids[i]])
                        score = confidences[i]

                        # 경계상자와 클래스 정보 이미지에 입력
                        cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 1)
                        cv2.rectangle(frame, (x, y), (x + 50, y - 13), (0, 0, 255), -1)
                        cv2.putText(frame, label, (x, y - 5), cv2.FONT_HERSHEY_COMPLEX, 0.3,
                        (255, 0, 0), 1)

                frame = cv2.resize(frame, dsize=(320, 640))

                h, w, ch = frame.shape
                bytes_per_line = ch * w
                convert_to_Qt_format = QImage(frame.data, w, h, bytes_per_line, QImage.Format_RGB888)
                qpixmap = QPixmap.fromImage(convert_to_Qt_format)
                self.vision_widget.setPixmap(qpixmap)
                self.vision_widget.show()
                time.sleep(0.01)

            except:
                continue
            
class check(QMainWindow):
    def __init__(self):
        super(check, self).__init__()
        uic.loadUi("momI.ui", self)
        self.show()

        self.sWidget.setCurrentIndex(1)
        self.cap = cv2.VideoCapture("rtsp://192.168.30.20:554/main")

        self.tRadar = radar(self.Radar)
        self.tRadar.start()
        self.tThermal = thermal(self.Thermal)
        self.tThermal.start()
        self.tVision = vision(self.cap, self.Vision)
        self.tVision.start()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = check()
    window.show()
    sys.exit(app.exec_())