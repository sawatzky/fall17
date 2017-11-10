# import the necessary packages
# $ python catdetect.py --image <image>.jpg
import argparse
import cv2

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-i", "--image", required=True,
	help="path to the input image")
ap.add_argument("-c", "--cascade",
	default="data/car_cascade.xml",
	help="path to cat detector haar cascade")
args = vars(ap.parse_args())

# load the input image and convert it to grayscale
image = cv2.imread(args["image"])
gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
 
# load the cat detector Haar cascade, then detect cat faces
# in the input image
detector = cv2.CascadeClassifier(args["cascade"])
rects = detector.detectMultiScale(gray, scaleFactor=1.3,
	minNeighbors=5, minSize=(20, 20))
	
# loop over the cat faces and draw a rectangle surrounding each
for (i, (x, y, w, h)) in enumerate(rects):
	cv2.rectangle(image, (x, y), (x + w, y + h), (0, 50*i, 255), 2)
	cv2.putText(image, "Car #{}".format(i + 1), (x, y - 10),
		cv2.FONT_HERSHEY_PLAIN, 0.55, (0, 0, 255), 2)
 
# show the detected cat faces
cv2.imshow("Car Profiles", image)
cv2.waitKey(0)

# for cat3.jpeg scaleFactor=1.1, minNeighbors=5, minSize=(10,10)
# works for cat4.jpeg as well