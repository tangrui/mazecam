#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <opencv2/core/core_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <ctype.h>

using namespace std;  
using namespace cv;

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;

void detect_detectMultiScale( IplImage* image );
void detect_cvHaarDetectObjects(IplImage* image);

const char* cascade_name = "haarcascade_frontalface_alt2.xml";

int main()
{
    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );

    if( !cascade )
    {
        fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
        return -1;
    }
    storage = cvCreateMemStorage(0);

    clock_t start, finish;
    double duration;
    start = clock();
    for(size_t i = 0; i < 3; i++){
        char filename[20];
        snprintf(filename, sizeof(filename), "picture_%d.jpg", i);
        IplImage* image = cvLoadImage( filename, 1 );
        if( image )
        {
            // detect_cvHaarDetectObjects( image );
            detect_detectMultiScale(image);
            cvReleaseImage( &image );
        }
    }
    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf( "use : %f seconds\n", duration );

    return 0;
}


void detect_cvHaarDetectObjects(IplImage* img )
{
    double scale = 1.2;
    //Image Preparation
    IplImage* gray = cvCreateImage(cvSize(640, 480),8,1);
    IplImage* small_img = cvCreateImage(cvSize(cvRound(640/scale),cvRound(480/scale)),8,1);
    cvCvtColor(img, gray, CV_BGR2GRAY);
    cvResize(gray, small_img, CV_INTER_LINEAR);

    cvEqualizeHist(small_img, small_img); //直方图均衡

    //Detect objects if any
    cvClearMemStorage(storage);
    clock_t start, finish;
    double duration;
    start = clock();
    CvSeq* objects = cvHaarDetectObjects(small_img,
                                            cascade,
                                            storage,
                                            1.2,
                                            2,
                                            0/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(30, 30),
                                            cvSize(0, 0));

    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf( "detection time : %f seconds\n", duration );

    cvReleaseImage(&gray);
    cvReleaseImage(&small_img);
}

void detect_detectMultiScale(IplImage* img )
{
    Mat mtx(img);
    Mat mtx_gray;

    std::vector<Rect> faces;

    cvtColor( mtx, mtx_gray, CV_BGR2GRAY );

    clock_t start, finish;

    double duration;
    start = clock();

    equalizeHist( mtx_gray, mtx_gray );
    cascade.detectMultiScale( mtx_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf( "detection time : %f seconds\n", duration );

    cvReleaseImage(&gray);
    cvReleaseImage(&small_img);
}
