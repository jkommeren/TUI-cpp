#include <boost/thread/thread.hpp>
#include <iostream>
#include <stdio.h> 
#include <cv.h> 
#include <highgui.h> 
#include<sys/time.h>

////using namespace std;
//
//
void SecondThread()
{
bool backwards = false;
int xfCirc = 0;
int yfCirc = 0;
cvNamedWindow("Animation"); 
while (true) {
usleep(20000);
IplImage* frameX = cvCreateImage(cvSize(200,200),8,3);
if (!backwards) {
xfCirc = xfCirc + 1.0;
yfCirc = yfCirc + 1.0;
}
else {
xfCirc -= 1.0;
yfCirc -= 1.0;
}
if (xfCirc > 190) {
backwards = true;
} else if (xfCirc < 10) {
// reached the top!;
backwards = false;
}

CvPoint circleCenter = cvPoint( xfCirc, yfCirc );
cvCircle(frameX, circleCenter, 20,cvScalar(255,255,255));
cvShowImage ("Animation", frameX);
cvWaitKey (1);
//cvDestroyWindow("Animation");

}
}
 
CvCapture *capture = 0; 
void StartCapture()
{
//cvNamedWindow("result"); 
try {
	// warning: the catch thingy doesn't work
capture = cvCaptureFromCAM(1); 
}
catch (std::exception& excpt) {
 fprintf(stderr, "Cannot open initialize webcam!\n"); 
// capture = cvCaptureFromCAM(1);
 }
 
 
 }
 
std::vector<CvSeq*> DetectObjects (CvMemStorage* storage,  IplImage* frame_thresh, double ContourAccuracy, double minAreaSize, double maxAreaSize)
{
CvSeq* contour = 0;
CvSeq* currentContour = 0;
std::vector<CvSeq*> objects;
objects.clear();
cvFindContours( frame_thresh, storage, &contour, sizeof(CvContour),CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );
int counter = 0;
           
  for( ; contour != 0; contour = contour->h_next )
        {
        	currentContour = cvApproxPoly(contour,sizeof(CvContour) ,storage, CV_POLY_APPROX_DP,cvArcLength(contour) * ContourAccuracy,0);
          	double contourArea = cvContourArea(currentContour);
          	
          	if (contourArea > minAreaSize && contourArea < maxAreaSize)
          	{
          	counter ++;
          	objects.insert(objects.begin(), currentContour);
            	
            	}
        } 
 std::cout << counter << std::endl;
 return objects;
 }
 
 void ProcessFrame ()
 {
 IplImage *frame = 0; 
 //IplImage *frame_gray = NULL;
 IplImage *frame_HSV = NULL;
 IplImage *frame_thresh = NULL;
 
 int bluerange = 200;
 int redrange = 200;
 int greenrange = 100;
 double ContourAccuracy = 0.0005;
 double minAreaSize =  2000;
 double maxAreaSize = 20000;
 CvMemStorage* storage = cvCreateMemStorage(0);
 
 cvGrabFrame(capture);
 try {
  frame = cvRetrieveFrame(capture);
 }
 catch (std::exception& x)
 {
 fprintf(stderr, "oops");

return;
 }
 
frame_HSV = cvCreateImage(cvGetSize(frame),8,3);
frame_thresh = cvCreateImage(cvGetSize(frame),8,1);
cvCvtColor(frame, frame_HSV, CV_BGR2HSV);
// range in HSV colors
cvInRangeS(frame_HSV,cvScalar(20,20,100), cvScalar(30,255,255), frame_thresh);
std::vector<CvSeq*> unidentifiedObjects = DetectObjects(storage, frame_thresh, ContourAccuracy,  minAreaSize, maxAreaSize);
cvShowImage("current",frame_thresh);
int key = cvWaitKey(1);
int counter2 = 0;
for (CvSeq* seq : unidentifiedObjects)
{
counter2++;
}
if (counter2 > 0)
{
 std::cout << counter2 << std::endl;
 }
 cvReleaseImage(&frame_thresh);
  cvReleaseImage(&frame_HSV);
  //delete storage;
  //cvReleaseImage(&frame);
unidentifiedObjects.clear();
  cvClearMemStorage(storage);
  //cvDestroyWindow("current");
}

 
 
 int main(int argc, char **argv) 
 {
 boost::thread t2(&SecondThread);
 StartCapture();
 while (true)
 {
 ProcessFrame();
 }
   cvReleaseCapture(&capture); 
 return 0;
 }
