/*#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"*/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libconfig.h>
#include <sys/stat.h>

#define MAXLEN 255
#define RADDEG 180./CV_PI
#define DEGRAD CV_PI/180

  using namespace cv;
  using namespace std;

  vector<double> ang2psi(vector<double> ang, double *params) {
  double minAng  = params[0];
  double maxAng  = params[1];
  double angZero = params[2];
  double conv    = params[3];

  vector<double> pressure;
  for(unsigned int i=0; i<ang.size(); i++) {
    double psi=-1;
    if(ang[i] > minAng && ang[i] < maxAng) {
      psi = (angZero - ang[i])*conv;
      psi = psi <  0 ? 0 : psi;
      if(psi > 0)
        pressure.push_back(psi);
    }
  }
  return(pressure);
}

double median(vector<double> arr) {
  if(arr.size() == 0) return 0;
  if(arr.size() == 1) return arr[0];

  double median = -1;
  std::sort(arr.begin(), arr.end());
  if(arr.size()%2 != 0) {
    median = arr[(arr.size()-1)/2];
  } else {
    median = 0.5*(arr[arr.size()/2] + arr[arr.size()/2 - 1]);
  }
  return(median);
}

void usage(char *argv0) {
  fprintf(stderr, "Usage: %s <Options> image1.jpg [...]\n", argv0);
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "\t-h               This help text\n");
  fprintf(stderr, "\t-d               Enable debug output (repeat for increased verbosity)\n");
  fprintf(stderr, "\t-l logfile       Send debug data to file\n");
  fprintf(stderr, "\t-r x:y:w:h       Set ROI in image\n");
  fprintf(stderr, "\t-v /dev/videoX   Read from v4l video device\n");
  fprintf(stderr, "\t-f N             Output psi values averaged over N frames/image_files\n");
  fprintf(stderr, "\t-c config_file   Use configuration file\n");
  fprintf(stderr, "\n");
}

/** @function main */
int main(int argc, char** argv)
{
  Mat raw, orig, src, src_gray;
  FILE *logf = NULL;
  Rect myROI(420,580,200,75);    // (X,Y,width,height)

  double par_ang2psi_lo[4] = {-999, 999, 0., -1.}; // conv function will return angle back
  double par_ang2psi_hi[4] = {-999, 999, 0., -1.}; // conv function will return angle back

  /* Handle commandline options */
  /* libconfig < 1.4 has no macros and requires different int types */
#ifndef LIBCONFIG_VER_MAJOR
  long int maxFrames, debug;
#else
  int debug, maxFrames;
#endif
  maxFrames  = -1;
  debug = 0;
  int frameCount = 0;
  int useVideo = 0;
  char config_file[MAXLEN]; config_file[0] = '\0';
  char video_dev[MAXLEN];   video_dev[0]  = '\0';
  int opt;
  int  HoughCircleParam[5] = {30, 50, 10, 20, 40};
  int  HoughLineParam[2]   = {25, 7};
  while ((opt = getopt(argc, argv, "dhl:r:c:v:f:")) != -1) {
    switch (opt) {
      case 'd':
        debug++;
        break;

      case 'v':
        useVideo = 1;
        strncpy(video_dev, optarg, MAXLEN-1);
        video_dev[MAXLEN-1]='\0';
        break;

      case 'f':
        maxFrames=atoi(optarg);
        break;

      case 'c':
        strncpy(config_file, optarg, MAXLEN-1);
        config_file[MAXLEN-1]='\0';
        break;

      case 'l':
        if((logf = fopen(optarg, "a")) == NULL ) {
          fprintf(stderr,"Can't open logfile: %s\n", optarg);
          exit(EXIT_FAILURE);
        }
        break;

      case 'r':
        int tmp[4];
        if( sscanf(optarg, "%d:%d:%d:%d", tmp, tmp+1, tmp+2, tmp+3) == 4 ) {
          myROI = Rect(tmp[0], tmp[1], tmp[2], tmp[3]);
        } else {
          fprintf(stderr,
            "\tFatal: Invalid ROI (%s) specified.  Format should be x:y:width:height\n",
            optarg);
          exit(EXIT_FAILURE);
        }
        break;

      case 'h':
      default:
        usage(argv[0]);
        exit(EXIT_FAILURE);
      }
  }

  /* Load config file */
  config_t cfg;
  config_setting_t *setting;
  const char *tmpstr;
  config_init(&cfg);
  struct stat buffer;
  char tmpfn[MAXLEN];
  strncpy(tmpfn,argv[0],MAXLEN-1);
  if( (config_file[0] == '\0') &&
      (stat( strncat(tmpfn, ".cfg", MAXLEN-5), &buffer ) == 0 ) ) {
    strncpy(config_file, tmpfn, MAXLEN-1);
  }
  if(config_file[0] != '\0') {
    if(debug) fprintf(stderr, "Using config file: %s\n", config_file);

    if(!config_read_file(&cfg, config_file)) {
      fprintf(stderr, "Config file error %d - %s\n",
          config_error_line(&cfg),
          config_error_text(&cfg));
      config_destroy(&cfg);
      exit(EXIT_FAILURE);
    }

    if(!debug) config_lookup_int(&cfg, "debug", &debug);

    if(maxFrames<0) config_lookup_int(&cfg, "max_frames", &maxFrames);

    if( (video_dev[0] == '\0') &&
            config_lookup_string(&cfg, "video_device", &tmpstr)) {
      useVideo = 1;
      strncpy(video_dev, tmpstr, MAXLEN-1);
      video_dev[MAXLEN-1]='\0';
    }

    if(config_lookup_string(&cfg, "logfile", &tmpstr)) {
      if((logf = fopen(tmpstr, "a")) == NULL ) {
        fprintf(stderr,"Can't open logfile: %s\n", tmpstr);
        exit(EXIT_FAILURE);
      }
    }

    if( (setting = config_lookup(&cfg, "ROI")) ) {
      int count = config_setting_length(setting);
      int tmp[4];
      if(count != 4) {
        fprintf(stderr, "\tInvalid ROI specified in '%s'.\n", config_file);
        exit(EXIT_FAILURE);
      }
      for(int i=0; i<count; i++)
        tmp[i] = config_setting_get_int_elem(setting, i);
      myROI = Rect(tmp[0], tmp[1], tmp[2], tmp[3]);
    }

    if( (setting = config_lookup(&cfg, "HoughCircleParam")) ) {
      int count = config_setting_length(setting);
      if(count != 5) {
        fprintf(stderr, "\tInvalid HoughCircleParam specified in '%s'.", config_file);
        exit(EXIT_FAILURE);
      }
      for(int i=0; i<count; i++)
        HoughCircleParam[i] = config_setting_get_int_elem(setting, i);
    }

    if( (setting = config_lookup(&cfg, "HoughLineParam")) ) {
      int count = config_setting_length(setting);
      if(count != 2) {
        fprintf(stderr, "\tInvalid HoughLineParam specified in '%s'.", config_file);
        exit(EXIT_FAILURE);
      }
      for(int i=0; i<count; i++)
        HoughLineParam[i] = config_setting_get_int_elem(setting, i);
    }

    if( (setting = config_lookup(&cfg, "ang2psi_hi")) ) {
      config_lookup_float(&cfg, "ang2psi_hi.angMin",  par_ang2psi_hi+0 );
      config_lookup_float(&cfg, "ang2psi_hi.angMax",  par_ang2psi_hi+1 );
      config_lookup_float(&cfg, "ang2psi_hi.angZero", par_ang2psi_hi+2 );
      config_lookup_float(&cfg, "ang2psi_hi.conv",    par_ang2psi_hi+3 );
    }

    if( (setting = config_lookup(&cfg, "ang2psi_lo")) ) {
      config_lookup_float(&cfg, "ang2psi_lo.angMin",  par_ang2psi_lo+0 );
      config_lookup_float(&cfg, "ang2psi_lo.angMax",  par_ang2psi_lo+1 );
      config_lookup_float(&cfg, "ang2psi_lo.angZero", par_ang2psi_lo+2 );
      config_lookup_float(&cfg, "ang2psi_lo.conv",    par_ang2psi_lo+3 );
    }

  } // Does config file exist?
  if(logf == NULL) logf = stderr;
  if(maxFrames < 1) maxFrames = 1;

  if( !useVideo && optind >= argc) {
    fprintf(stderr, "\tMissing image filename argument after options list.\n");
    exit(EXIT_FAILURE);
  }

  VideoCapture cap;
  if( useVideo ) {
    VideoCapture cap(video_dev); // open the video device
    if (!cap.isOpened()) {
      fprintf(stderr,"Can't open the video cam at '%s'\n",video_dev);
      exit(EXIT_FAILURE);
    }
  }

  /* Loop over input files, or open /dev/video frames */
  vector<double> angLeft;
  vector<double> angRight;
  while(useVideo || (optind < argc)) {

    if(!useVideo) {
      if(debug) fprintf(logf,"-----------------------\nOpening file: '%s'\n", argv[optind]);
      raw = imread( argv[optind], 1 );
      if( !raw.data ) {
        fprintf(stderr,"Can't open file: %s\n", argv[optind]);
        exit(EXIT_FAILURE);
      }
    } else {
      if(! cap.read(raw) ) {
        fprintf(logf,"Failed reading frame\n");
        exit(EXIT_FAILURE);
      }
    }

    orig = raw(myROI);
    orig.copyTo(src);
    /// Convert it to gray
    cvtColor( src, src_gray, CV_BGR2GRAY );

    /// Reduce the noise so we avoid false circle detection
    GaussianBlur( src_gray, src_gray, Size(9, 9), 2, 2 );

    vector<Vec3f> circles;
    vector<Vec3f> goodCircles;

    /// Apply the Hough Transform to find the circles
    //void cv::HoughCircles   (   InputArray    image,
    //  OutputArray   circles,
    //  int   method,
    //  double    dp,
    //  double    minDist,
    //  double    param1 = 100,
    //  double    param2 = 100,
    //  int   minRadius = 0,
    //  int   maxRadius = 0 
    //);
    //HoughCircles( src_gray, circles, CV_HOUGH_GRADIENT, 1, src_gray.rows/8, 200, 100, 0, 0 );
    //HoughCircles( src_gray, circles, CV_HOUGH_GRADIENT, 1, 30, 100, 50, 20, 100 );

    HoughCircles( src_gray, circles, CV_HOUGH_GRADIENT, 1, HoughCircleParam[0],
        HoughCircleParam[1], HoughCircleParam[2], HoughCircleParam[3], HoughCircleParam[4]);
    if(debug) fprintf(logf,"\nFound %ld circles.\n", (unsigned long int)circles.size());

    /// Draw the circles detected
    double aveRadius=0;
    for( unsigned int i = 0; i < circles.size(); i++ ) {
      int radius = cvRound(circles[i][2]);
      Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));

      if(debug) fprintf(logf,"\t %3d : %3d   %3d : %3d", i, 
          cvRound(circles[i][0]), cvRound(circles[i][1]), radius);

      int col = (int) 255./circles.size()*(i+1);
      if(
          (abs(circles[i][1] - 0.5*myROI.height) < myROI.height/5)// should be on H. mid-line
       && (abs(circles[i][0] - 0.5*myROI.width)  > myROI.width/7) // reject near image center
       ) {
        if(debug) fprintf(logf," : good");
        goodCircles.push_back( circles[i] );
        aveRadius+=radius;
        circle( src, center, 3, Scalar(0,col,0), -1, 8, 0 );      // circle center
        circle( src, center, radius, Scalar(0,col,0), 3, 8, 0 );  // circle outline
      } else {
        circle( src, center, 3, Scalar(0,0,col), -1, 8, 0 );      // circle center
        circle( src, center, radius, Scalar(0,0,col), 3, 8, 0 );  // circle outline
      }
      if(debug) fprintf(logf,"\n");
    }
    if(goodCircles.size() > 0) {
      aveRadius /= goodCircles.size();
      aveRadius *= 0.7;
    } else {
      aveRadius = 25.;
    }

    if(debug) fprintf(logf,"Max line length (from aveRadius*0.7):  %g\n", aveRadius);

    /// Show your results
    if(debug) {
      namedWindow( "Hough Circle Transform Demo", CV_WINDOW_AUTOSIZE );
      imshow( "Hough Circle Transform Demo", src );
    }

    Mat dst, color_dst;
    orig.copyTo(src);
    Canny( src, dst, 50, 200, 3 );
    cvtColor( dst, color_dst, COLOR_GRAY2BGR );
    vector<Vec4i> lines;


    /* HoughLinesP(dst, lines, 1, CV_PI/180, 50, 50, 10 );
     *  dst: Output of the edge detector. It should be a grayscale image
     *        (although in fact it is a binary one)
     *  lines: A vector that will store the parameters (x_{start}, y_{start},
     *        x_{end}, y_{end}) of the detected lines
     *  rho : The resolution of the parameter r in pixels. We use 1 pixel.
     *  theta: The resolution of the parameter \theta in radians. We use 1
     *        degree (CV_PI/180)
     *  threshold: The minimum number of intersections to "detect" a line
     *  minLinLength: The minimum number of points that can form a line. Lines
     *        with less than this number of points are disregarded.
     *  maxLineGap: The maximum gap between two points to be considered in the
     *        same line.
     */
    HoughLinesP( dst, lines, 1, CV_PI/180, HoughLineParam[0], aveRadius, HoughLineParam[1]);
    if(debug) fprintf(logf,"\nFound %ld lines.\n", (unsigned long int)lines.size());
    for( unsigned int i = 0; i < lines.size(); i++ ) {
      int col = (int) 255./lines.size()*(i+1);
      if(debug) fprintf(logf,"\t %3d : %3d   %3d :  %3d   %3d", i,
          cvRound(lines[i][0]), cvRound(lines[i][1]),
          cvRound(lines[i][2]), cvRound(lines[i][3])
          );
      if(debug > 1) {
        line( color_dst, Point(lines[i][0], lines[i][1]),
          Point(lines[i][2], lines[i][3]), Scalar(0,0,col), 3, 8 );
      }

      int p1_x = (lines[i][0]);
      int p1_y = (lines[i][1]);
      int p2_x = (lines[i][2]);
      int p2_y = (lines[i][3]);

      double len = sqrt(p1_x*p1_x + p1_y*p1_y);

      for( unsigned int j = 0; j < goodCircles.size(); j++ ) {
        int c_x = cvRound(goodCircles[j][0]);
        int c_y = cvRound(goodCircles[j][1]);
        int c_r = cvRound(goodCircles[j][2]);

        /* look for lines roughly inside the circles */
        double scale_radius = 1.3;
        double p1_d = sqrt( pow(p1_x - c_x,2) + pow(p1_y - c_y,2) ) - scale_radius*c_r;
        double p2_d = sqrt( pow(p2_x - c_x,2) + pow(p2_y - c_y,2) ) - scale_radius*c_r;
        double d = abs((p2_x-p1_x)*(p1_y-c_y) - (p1_x-c_x)*(p2_y-p1_y))/
          sqrt(pow(p2_x-p1_x,2) + pow(p2_y-p1_y,2));
        if( (p1_d < 0) && (p2_d < 0) ) {
            if(debug) fprintf(logf," :  len %6.2f  :  c %d : d %6.3f", len, j, d);

          /* good line(s) should be close to the circle center */
          double d = abs((p2_x-p1_x)*(p1_y-c_y) - (p1_x-c_x)*(p2_y-p1_y))/
            sqrt(pow(p2_x-p1_x,2) + pow(p2_y-p1_y,2));
          if(d < 0.2*c_r) {
            line( color_dst, Point(lines[i][0], lines[i][1]),
              Point(lines[i][2], lines[i][3]), Scalar(0,col,0), 2, 4 );

            double p3_x, p3_y;
            /* select line end-point closest to circumference */
            if(abs(p1_d) < abs(p2_d)) {
              p3_x = p1_x;    p3_y = p1_y;
            } else {
              p3_x = p2_x;    p3_y = p2_y;
            }
            circle( color_dst, Point(p3_x, p3_y), 3, Scalar(col,0,0), -1, 8, 0 );
            p3_x -= c_x;
            p3_y -= c_y;

            double ang=-9999;

            ang = RADDEG*atan2(-p3_y, p3_x);
            if(ang < 0) ang+=360;

            if(c_x < 0.5*myROI.width) {
              angLeft.push_back(ang);
            } else {
              angRight.push_back(ang);
            }

            if(debug) fprintf(logf," : a %g deg",ang);
          }
        }
      }
      if(debug) fprintf(logf,"\n");
    }

    if(debug) {
      fprintf(logf,"\n");
      imshow( "Edges", dst );
      namedWindow( "Detected Lines", CV_WINDOW_AUTOSIZE );
      imshow( "Detected Lines", color_dst );
    }

    printf("Filename: %s\n", argv[optind]);
    printf("  Left_angle(s): ");
    for(unsigned int i=0; i<angLeft.size(); i++) {
      printf("  %8.2f", angLeft[i]);
    }
    printf("\n");
    printf("  Right_angle(s):");
    for(unsigned int i=0; i<angRight.size(); i++) {
      printf("  %8.2f", angRight[i]);
    }
    printf("\n\n");

    /* Average results over maxFrames */
    if(maxFrames > 0) {
      frameCount++;

      if(frameCount >= maxFrames) {
        printf("  Pressure low / high: %8.2f  %8.2f\n\n", 
            median(ang2psi(angLeft,  par_ang2psi_lo)),
            median(ang2psi(angRight, par_ang2psi_hi))
            );
        frameCount=0;
        angLeft.clear();
        angRight.clear();
      }
    }

    if(useVideo) {
      sleep(1);
    } else {
      optind++;
      if(debug) waitKey(0);
    }

  }

  exit(0);
}
