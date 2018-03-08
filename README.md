# fall17
Programs for both learning openCV framework and developing image-recognition software

raspi_detect/

- the cascade file used for detection is in fall17/gaugedetect2/data/gauge2_cascade.xml

***

gaugedetect_folder: this program loops through saved images and performs recognition/needle-reading

- good for debugging
- right now the parameters in 'detectMultiScale' are doing well with the images I have.
If detection is not working I would change these first.

***

gaugedetect_camera: this program uses raspi camera to produce images for recognition/needle-reading

- need to set up interval to delete old pictures, or delete them right away once needle-reading is done
- set up an output file of high/low pressures and timestamp?
- still needs method to weed out a false hit

***

readGauge: identical to original version, except char opt; is now int opt;