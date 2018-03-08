# fall17
Programs for both learning openCV framework and developing image-recognition software

raspi_detect/

gaugedetect_folder: this program loops through saved images and performs recognition/needle-reading
*** good for debugging

gaugedetect_camera: this program uses raspi camera to produce images for recognition/needle-reading
*** need to set up interval to delete old pictures, or delete them right away once needle-reading is done
*** set up an output file of high/low pressures and timestamp?

readGauge: identical to original version, except char opt; is now int opt;