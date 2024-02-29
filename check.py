import os
import cv2
import time
import numpy as np
import threading
import time

def vision():
    cap = cv2.VideoCapture("rtsp://192.168.30.20:554/main")

    while True:
        ret, frame1 = cap.read()
        frame1 = cv2.resize(frame1, (320, 640))
        cv2.imshow('d1', frame1)
        cv2.waitKey(1)


def radar():
    while True:
        with open('breathing.txt', 'r') as file:
            data_string = file.read()
            if "measure".find(data_string) == -1:
                pass
            else:
                print(data_string)
            time.sleep(4)

def thermal():
    raw = np.zeros((80*60), np.int16)
    m_raw = np.zeros((80*70), np.int16)

    for i in range(0, 4800):
        raw[i] = 1650

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
                raw[i] = raw_dat[i]

            image = raw.reshape(60, 80)
            image1 = image[:59, :60]
            image2 = image1.reshape(-1)

            max = np.max(image2)
            min = np.min(image2)
            nfactor = 255 / (max - min)
            pTemp = image2 - min
            nTemp = pTemp * nfactor 
            frame = nTemp
            image2 = frame.reshape(59, 60)        

            """
            max = np.max(raw)
            min = np.min(raw)

            nfactor = 255 / (max - min)
            pTemp = raw - min
            nTemp = pTemp * nfactor 
            frame = nTemp
            raw = frame
            image = raw.reshape(60, 80)
            image1 = image[:59, :60]
            image2 = image1.reshape(-1)

            max1 = np.max(image2)
            min1 = np.min(image2)
            nfactor1 = 255 / (max1 - min1)
            pTemp1 = image2 - min1
            nTemp1 = pTemp1 * nfactor1 
            frame1 = nTemp1
            frame2 = frame1.reshape(59, 60)
        """
            
            uint_img = np.array(image2).astype('uint8')
            grayImage = cv2.cvtColor(uint_img, cv2.COLOR_GRAY2BGR)
            grayImage = cv2.resize(grayImage, (320, 320))
            cv2.imshow('d2', grayImage)
            cv2.waitKey(1) 
        except:
            continue

t1 = threading.Thread(target=thermal, args=())
t1.start()
t2 = threading.Thread(target=radar, args=())
t2.start()
#t3 = threading.Thread(target=vision, args=())
#t3.start()

while True:
    pass

