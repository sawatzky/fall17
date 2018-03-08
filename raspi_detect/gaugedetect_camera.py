# this program activates raspicam in intervals determined by time.sleep() in seconds
# uncomment cv2.imshow and cv2.waitkey to see detected region, keystroke to continue
# ctrl-c at terminal to end process
# pictures saved in gaugepics/
# $ python gaugedetect_camera.py
import cv2
import os
import time

while True:
        timestamp = time.strftime("gaugepics/%Y%m%d_%H%M.jpg")
        os.system("raspistill -vf -w 648 -h 486 -o " + timestamp)
        image = cv2.imread(timestamp)
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

        # update path to gauge2_cascade.xml **********************************
        detector = cv2.CascadeClassifier("cascades/gauge2_cascade.xml")
        rects = detector.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=5, minSize=(50,50))

        for(i, (x,y,w,h)) in enumerate(rects):
               cv2.rectangle(image, (x,y), (x+w, y+h), (0, 50*i,255),2)
               ROI = [x,y,w,h]
               print timestamp
               os.system('./readGauge -r ' + ':'.join(str(s) for s in list(ROI))  + ' gaugepics/' + filename)

        #cv2.imshow("Gauges", image)
        #cv2.waitKey(0)

        time.sleep(60)
