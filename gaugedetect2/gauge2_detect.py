# import the necessary packages
# $ python gauge_detect.py --image <image>.jpg
import argparse
import cv2

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-i", "--image", required=True,
	help="path to the input image")
ap.add_argument("-c", "--cascade",
	default="data/gauge2_cascade.xml",
	help="path to gauge cascade haar cascade")
args = vars(ap.parse_args())

# load the input image and convert it to grayscale
image = cv2.imread(args["image"])
gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
 
# load the gauge detector Haar cascade, then detect gauges
# in the input image
detector = cv2.CascadeClassifier(args["cascade"])
rects = detector.detectMultiScale(gray, scaleFactor=1.2,
	minNeighbors=5, minSize=(25, 25))
	
# loop over the gauges and draw a rectangle surrounding each
for (i, (x, y, w, h)) in enumerate(rects):
	cv2.rectangle(image, (x, y), (x + w, y + h), (0, 50*i, 255), 2)
	cv2.putText(image, "Gauge {}".format(i + 1), (x, y - 10),
		cv2.FONT_HERSHEY_PLAIN, 1, (255, 255, 255), 2)
 
# show the detected gauges
cv2.imshow("Car Profiles", image)
cv2.waitKey(0)

# for cat3.jpeg scaleFactor=1.1, minNeighbors=5, minSize=(10,10)
# works for cat4.jpeg as well