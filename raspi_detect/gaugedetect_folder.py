#this program loops through saved gaugepics, detects bounding boxes, and informs readGauge.C of ROI
# $ python gaugedetect_folder.py
import cv2
import os
import sys

#update basepath to correct folder **************************************
basepath = '/home/pi/Desktop/gaugepics/'

for filename in os.listdir(basepath):
        if filename.startswith("gauge"):
            print filename
            image = cv2.imread(basepath + filename, 0)

            # update path to gauge2_cascade.xml **************************
            detector = cv2.CascadeClassifier("cascades/gauge2_cascade.xml")
            rects = detector.detectMultiScale(image, scaleFactor=1.1, minNeighbors=5, minSize=(50,50))

            for(i, (x,y,w,h)) in enumerate(rects):
                    cv2.rectangle(image, (x,y), (x+w, y+h), (0, 50*i,255),2)
                    ROI = [x,y,w,h]
                    os.system('./readGauge -r ' + ':'.join(str(s) for s in list(ROI))  + ' gaugepics/' + filename)

            # this allows user to examine detection and progress to next picture with keystroke
            cv2.imshow("gaugepics", image)
            cv2.waitKey()
