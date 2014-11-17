/*

功能：实现对眼睛、脸部的跟踪。
版本：1.0
时间：2014-4-27
*/
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>

#include <iostream>
#include <stdio.h>
#include <time.h>

using namespace std;
using namespace cv;


void detectEyeAndFace( Mat frame );
//将下面两个文件复制到当前工程下。
//当前文件路径应该是OpenCV安装路径下的sources\data\haarcascades目录下
String face_cascade_name = "haarcascade_frontalface_alt2.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

RNG rng(12345);

int main( int argc, char *argv[] )
{
    Mat oneFrame;
    //判断face_cascade_name、eye_cascade_name能够顺利加载
    if( !face_cascade.load( face_cascade_name ) ){ printf("face_cascade_name加载失败\n"); return -1; };

    IplImage* img = cvLoadImage("picture_134.jpg");
    Mat mtx(img);
    detectEyeAndFace( img );
    cvReleaseImage( &img );
    return 0;
}


void detectEyeAndFace( Mat oneFrame )
{
    std::vector<Rect> faces;  //脸部标注框
    Mat grayFrame;

    clock_t start, finish;
    double duration;
    start = clock();

    cvtColor( oneFrame, grayFrame, CV_BGR2GRAY );
    equalizeHist( grayFrame, grayFrame );

    face_cascade.detectMultiScale( grayFrame, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf( "detection time : %f seconds\n", duration );
}