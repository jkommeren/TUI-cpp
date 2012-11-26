#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>
#include <stdlib.h>
#include <stdio.h> 
#include <cv.h> 
#include <highgui.h> 
// timer thread
#include <sys/time.h>
#include <signal.h>
// thread wait
#include <pthread.h>
// priority:
#include <sched.h>
#include <unistd.h>



class IdentifiedObject
{
	int _x, _y;
	double  _size;
	bool _circle, _square;
	//_border;
	// add orientation here for Module 5
	// add lastSeen for Module 3
	// struct timeval tim;
	// gettimeofday(&tim, NULL);
	 // double lasttime = tim.tv_sec +(tim.tv_usec/1000000.0);
	
	public:
	IdentifiedObject() {
		_x = _y = _size = 0;
		_circle, _square = false;
	}
	IdentifiedObject(const int x, const int y, const double size, bool isCircle, bool isSquare) {
		_x = x;
		_y = y;
		_size = size;
		_circle = isCircle;
		_square = isSquare;
		//_border = isBorder;
	}
	~IdentifiedObject() { }
	void updatePosition(const int x, const int y)
	{
		_x = x;
		_y = y;
	}
	std::string getShape()
	{
		if (_circle == true) return "circle";
		else if (_square ==true ) return "square";
		//else if (_border == true) return "border";
		else {return "different kind of object";}
	}
};


bool showWindow = true;
static volatile sig_atomic_t gotAlarm = 0;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void
sigalrmHandler(int sig)
{
    gotAlarm = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void SecondThread()
{
bool backwards = false;
int xfCirc = 0;
int yfCirc = 0;
int width = 200;
int height = 200;
cvNamedWindow("Animation"); 
IplImage* frameX;
IplImage* blank;
CvPoint circleCenter;
blank = cvCreateImage(cvSize(width,height),8,1);
frameX =  cvCloneImage(blank);
struct timeval tim;
gettimeofday(&tim, NULL);


struct itimerval itv;
clock_t prevClock;
//int maxSigs;                /* Number of signals to catch before exiting */
//int sigCnt;                 /* Number of signals so far caught */
struct sigaction sa;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sa.sa_handler = sigalrmHandler;
if (sigaction(SIGALRM, &sa, NULL) == -1) std::cout<<"oops"<<std::endl;//errExit("sigaction");
itv.it_value.tv_sec = 2; // init time
itv.it_value.tv_usec = 0;
itv.it_interval.tv_sec = 0;
itv.it_interval.tv_usec = 20000; // amount of microseconds for interval
double lasttime = tim.tv_sec +(tim.tv_usec/1000000.0);


if (setitimer(ITIMER_REAL, &itv, NULL) == -1) std::cout<<"oops2"<<std::endl;
int animcounter = 0; 
for (;;) {
	pthread_mutex_lock(&mutex);
	if (gotAlarm) {
	//std::cout << "go!" << std::endl;
                gotAlarm = 0;
                if (showWindow)
                {
                animcounter ++;
                if (animcounter > 50)
					{
						animcounter = 0;
						gettimeofday(&tim, NULL);
						double curtime = tim.tv_sec +(tim.tv_usec/1000000.0);
						//std::cout << "animation framerate: ";
						double animFramerate = 50 / (curtime - lasttime);
						std::cout << animFramerate << std::endl;
						lasttime = curtime;
					}
			}
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
		frameX = cvCloneImage(blank);
		} else if (xfCirc < 10) {
		// reached the top!;
		backwards = false;
		frameX = cvCloneImage(blank);
		}
		
		circleCenter = cvPoint( xfCirc, yfCirc );
		cvCircle(frameX, circleCenter, 20,cvScalar(255));
		cvShowImage ("Animation", frameX);
		cvWaitKey (1);
		//cvDestroyWindow("Animation");
		pthread_cond_wait(&cond, &mutex);
		}
		else {
			std::cout << "waiting ..." << std::endl;
		pthread_cond_wait(&cond, &mutex);
		}
	}
}
 
CvCapture *capture = 0; 
int captureNo = 1;

bool calibrateSwitch = false;
CvRect projectionArea = cvRect(0,0,320,240);


int hue =0;
int sat =0;
int val =0;
int maxHue =255;
int maxSat =255;
int maxVal =255;
int minAreaSize = 2000;
int maxAreaSize = 20000;

void StartCapture()
{
//cvNamedWindow("result"); 
try {
	// warning: the catch thingy doesn't work
	// video 1 == device 11.10
	
capture = cvCaptureFromCAM(captureNo); 
}
catch (std::exception& excpt) {
 fprintf(stderr, "Cannot open initialize webcam!\n"); 
// capture = cvCaptureFromCAM(1);
 }
 
 
 }
 
IdentifiedObject IdentifyObject (CvSeq* detectedObject)
 {
	 bool isCircle,isSquare;
	 isCircle = isSquare = false;
	 CvRect boundingRect = cvBoundingRect(detectedObject);
	 // two options; cvMinAreaRect && ...2 . The second one requires only the points and storage as input, but returns
	 // a cvbox2d
	 double boundingRectArea = double(boundingRect.width * boundingRect.height);
	CvBox2D minBox = cvMinAreaRect2(detectedObject);
	//CvRect minBoxRect = CvRect(minBox.size);
	CvSize2D32f minBoxSquare = minBox.size;
	CvRect minBoxRect = cvRect(0,0,int(minBoxSquare.width),int(minBoxSquare.height));
	double minBoxArea = double(minBoxRect.width * minBoxRect.height);
	CvPoint2D32f center = minBox.center;
	int centerX = int(center.x + 0.5);
	int centerY = int(center.y + 0.5);
	
	if (minBoxArea > 1.05 * boundingRectArea)
	{
		isSquare = true;
	}
	
	else {
		double ratio = double(minBoxSquare.height / minBoxSquare.width);
		if (ratio > 0.95 && ratio < 1.05)
	{
		//std::cout << ratio;
		isCircle = true;
	}
	}

	
	
	IdentifiedObject object = IdentifiedObject(centerX,centerY,minBoxArea,isCircle,isSquare);
	return object;
 }
 
std::vector<CvSeq*> DetectObjects (CvMemStorage* storage,  IplImage* frame_thresh, double ContourAccuracy, double minAreaSize, double maxAreaSize)
{
CvSeq* contour = 0;

std::vector<CvSeq*> objects;
 // = NULL;
//objects.clear();
//return objects;
cvFindContours( frame_thresh, storage, &contour, sizeof(CvContour),CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );

int counter = 0;
           double biggestContourSize = 0.0;
           CvSeq* biggestContour;
  for( ; contour != 0; contour = contour->h_next )

        {
	        //*objects.begin();
        	CvSeq* currentContour = cvApproxPoly(contour,sizeof(CvContour) ,storage, CV_POLY_APPROX_DP,cvArcLength(contour) * ContourAccuracy,0);
          	double contourArea = cvContourArea(currentContour);
          	
          	if (contourArea > double(minAreaSize) && contourArea < double(maxAreaSize))
          	{
          	counter ++;
          	if (contourArea > biggestContourSize)
          	{
	          	biggestContourSize = contourArea;
	          	biggestContour = currentContour;
          	}
          	//CvSeq* ptr = currentContour;
          	objects.insert(objects.end(),currentContour);
            	//objects.insert(currentContou
           // objects.push_back(*currentContour);
            	//break;
            	//objects.insert(currentContour);
            }
        }
  if (calibrateSwitch)
  {
	  calibrateSwitch = false;
	  projectionArea = cvBoundingRect(biggestContour);
	  std::cout << "calibrated to";
	  std::cout << projectionArea.height;
	  std::cout << "x";
	  std::cout << projectionArea.width << std::endl;
	  
	  //projectionContour = BiggestContour;
  }
 
 //std::cout << counter << std::endl;
 //return objects;
 return objects;
 }
 
 void ProcessFrame (
	 CvMemStorage* storage)
 {
 IplImage *frame = NULL; 
 //IplImage *frame_gray = NULL;
 IplImage *frame_HSV = NULL;
 IplImage *frame_thresh = NULL;
 
 int bluerange = 200;
 int redrange = 200;
 int greenrange = 100;
 double ContourAccuracy = 0.0005;
 //double minAreaSize =  2000;
 //double maxAreaSize = 20000;
 
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
cvInRangeS(frame_HSV,cvScalar(hue,sat,val), cvScalar(maxHue,maxSat,maxVal), frame_thresh);

std::vector<CvSeq*> unidentifiedObjects= DetectObjects(storage, frame_thresh, ContourAccuracy,  minAreaSize, maxAreaSize);

int counter2 = 0;
//for (seqX::iterator i = unidentifiedObjects.begin(); i != unidentifiedObjects.end(); ++i)
for (CvSeq* seq : unidentifiedObjects)
{
	IdentifiedObject x = IdentifyObject(seq);
	
//	std::cout << "I'm thinking object ";
//	std::cout << counter2;
//	std::cout << " is a " + x.getShape()<< std::endl;
	CvScalar col; 
	if (x.getShape() == "square")col = cvScalar(255,0,0);
	else if (x.getShape() == "circle") col = cvScalar(0,255,0);
	else { col = cvScalar(255,255,255);}
	counter2++;
	cvDrawContours(frame,seq,col,cvScalar(1,1,1 ),10);
//delete seq;
//seq = NULL;
}
cvRectangle(frame, cvPoint(projectionArea.x,projectionArea.y), cvPoint(projectionArea.x + projectionArea.width, projectionArea.y + projectionArea.height),cvScalar(1,255,255));

if (showWindow) {
cvShowImage("current",frame);
cvCreateTrackbar("min Hue", "current", &hue, 255, 0);
cvCreateTrackbar("max Hue", "current", &maxHue, 255, 0);
cvCreateTrackbar("min Saturation", "current", &sat, 255, 0);
cvCreateTrackbar("max Saturation", "current", &maxSat, 255, 0);
cvCreateTrackbar("min Value", "current", &val, 255, 0);
cvCreateTrackbar("max Value", "current", &maxVal, 255, 0);
cvCreateTrackbar("min Object Size", "current", &minAreaSize, 2000, 0);
cvCreateTrackbar("max Object Size", "current", &maxAreaSize, 50000, 0);
int key = cvWaitKey(10);
//std::cout << key << std::endl;
if (key == 99) 
	{
		calibrateSwitch = true;
}
else if (key == 119)
{
	showWindow = false;
}

//if (counter2 > 0)
//{
// //std::cout << counter2 << std::endl;
// }
}
 
 cvReleaseImage(&frame_thresh);
  cvReleaseImage(&frame_HSV);
unidentifiedObjects.erase(unidentifiedObjects.begin(),unidentifiedObjects.end());
 
  //cvDestroyWindow("current");
}

 
 int main(int argc, char **argv) 
 {
	 // check for argument
	  int i = 0;
	  if ( argc != 1)
	  {
		 
	 	 std::cout << argv[1] << std::endl;
		  std::cout << "arg found!" << std::endl;
		  captureNo = atoi(argv[1]);
	  }
	  
 boost::thread t2(&SecondThread);
 pthread_t threadID = (pthread_t) t2.native_handle();

 //struct sched_param param;
int sched_priority = 90;
pthread_setschedprio(threadID, sched_priority);
//pthread_attr_setschedparam(threadID,SCHED_RR, &param);
 
 StartCapture();
 
	 CvMemStorage* storage = cvCreateMemStorage(0);
	 int framecounter = 0;
	  struct timeval tim;
	  gettimeofday(&tim, NULL);
	  double lasttime = tim.tv_sec +(tim.tv_usec/1000000.0);
	// cvNamedWindow("current"); 
 while (true)
 {
	// if(showWindow) {
	
	 if (framecounter < 30)
	 {
		 framecounter ++;
	 }
	 else {
		 // reset counter and calc the time 30 frames took to process (=framerate)
		 framecounter = 0;
		 gettimeofday(&tim, NULL);
		double curtime = tim.tv_sec+(tim.tv_usec/1000000.0);
		double framerate = 30 / ((curtime-lasttime));
		std::string framestring = boost::lexical_cast<std::string>(framerate);
		std::cout << "framerate:"+framestring << std::endl;
		lasttime = curtime;
	 }
	 //}
	 ProcessFrame(storage);
	 cvClearMemStorage(storage);
 }
   cvReleaseCapture(&capture); 
 return 0;
 }
