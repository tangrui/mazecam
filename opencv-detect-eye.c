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

using namespace std;
using namespace cv;


void detectEyeAndFace( Mat frame );
//将下面两个文件复制到当前工程下。
//当前文件路径应该是OpenCV安装路径下的sources\data\haarcascades目录下
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

RNG rng(12345);

int main( int argc, const char** argv )
{
  Mat oneFrame;
  /*  
  Mat test;
  test=imread("a.jpg");
  imshow("",test);
  waitKey(0);    
  */
  //判断face_cascade_name、eye_cascade_name能够顺利加载
  if( !face_cascade.load( face_cascade_name ) ){ printf("face_cascade_name加载失败\n"); return -1; };
  if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("eye_cascade_name加载失败\n"); return -1; };

  
   
  VideoCapture vCp("Sample.avi");

  if( vCp.isOpened()) 
  {
    while( true )
    {
      
      vCp>>oneFrame;

      //-- 3. Apply the classifier to the frame
      if( !oneFrame.empty() )
      { detectEyeAndFace( oneFrame ); }
      else
      { printf(" 当前视频文件为空!"); break; }

      int c = waitKey(10);
      if( (char)c == 'b' ) { break; } 

    }
  }
  return 0;
}


void detectEyeAndFace( Mat oneFrame )
{
  std::vector<Rect> faces;  //脸部标注框
  Mat grayFrame;

  cvtColor( oneFrame, grayFrame, CV_BGR2GRAY );
  equalizeHist( grayFrame, grayFrame );

  face_cascade.detectMultiScale( grayFrame, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

  for( size_t i = 0; i < faces.size(); i++ )
  {
    Point center( int(faces[i].x + faces[i].width*0.5), int(faces[i].y + faces[i].height*0.5) );
    ellipse( oneFrame, center, Size( int(faces[i].width*0.5), int(faces[i].height*0.5)), 0, 0, 360, Scalar( 255, 0, 255 ), 2, 8, 0 );

    Mat faceROI = grayFrame( faces[i] );  //得到当前标注的脸部区域
    std::vector<Rect> eyes;//眼睛标注


    eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );

    for( size_t j = 0; j < eyes.size(); j++ )
    {
      Point center( int(faces[i].x + eyes[j].x + eyes[j].width*0.5), int(faces[i].y + eyes[j].y + eyes[j].height*0.5) ); 
      int radius = cvRound( (eyes[j].width + eyes[i].height)*0.25 );
      circle( oneFrame, center, radius, Scalar( 255, 0, 0 ), 3, 8, 0 );
    }
  } 

  imshow( "眼镜和脸部跟踪检测", oneFrame );
}