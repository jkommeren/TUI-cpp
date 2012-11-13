#include <boost/thread/thread.hpp>
#include <iostream>
#include <stdio.h> 
#include <cv.h> 
#include <highgui.h> 
////using namespace std;
//
//
void thread_function()
{
CvCapture *capture = 0; 
 IplImage *frame = 0; 
 int key = 0;
 capture = cvCaptureFromCAM(0); // use GUVCVIEW to determine videonumber (listed as video#) 

 

 if(!capture) 

 { 
 fprintf(stderr, "Cannot open initialize webcam!\n"); 
 return; 
 } 

 cvNamedWindow("result"); 
 while(key != 'q') 
 { 

 frame = cvQueryFrame(capture); 
 if(!frame) break; 

 

 cvShowImage("result", frame); 

 

 key = cvWaitKey(1); // leave this one out and OpenCV will not refresh it's window! 

 } 

 

 cvDestroyWindow("result"); 
 cvReleaseCapture(&capture); 
 return; 
}
//int main (int argc, char *argv[])
//{
//	std::cout << "Hello world! V2" << std::endl;
//	boost::thread threadone(&thread_function);
//	threadone.join();
//	return 0;
//}
//
//////////////////////////////////////////////////////////////////////////  


 

 int main(int argc, char **argv) 
 {
 boost::thread threadone(&thread_function);
threadone.join();
return 0;
 } 

 //////////////////////////////////////////////////////////////////////////  