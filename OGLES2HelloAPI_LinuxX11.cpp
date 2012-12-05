/******************************************************************************

@File         OGLES2HelloAPI_LinuxX11.cpp

@Title        OpenGL ES 2.0 HelloAPI Tutorial

@Version      

@Copyright    Copyright (c) Imagination Technologies Limited.

@Platform     .

@Description  Basic Tutorial that shows step-by-step how to initialize OpenGL ES
2.0, use it for drawing a triangle and terminate it.

******************************************************************************/
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>
#include <stdlib.h>
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



#include <stdio.h>
#include "X11/Xlib.h"
#include "X11/Xutil.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>




/******************************************************************************
Defines
******************************************************************************/

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0

// Max width and height of the window
#define WINDOW_WIDTH	640
#define WINDOW_HEIGHT	480

/*!****************************************************************************
@Function		TestEGLError
@Input			pszLocation		location in the program where the error took
place. ie: function name
@Return		bool			true if no EGL error was detected
@Description	Tests for an EGL error and prints it
******************************************************************************/
bool TestEGLError(const char* pszLocation)
{
	/*
	eglGetError returns the last error that has happened using egl,
	not the status of the last called function. The user has to
	check after every single egl call or at least once every frame.
	*/
	EGLint iErr = eglGetError();
	if (iErr != EGL_SUCCESS)
	{
		printf("%s failed (%d).\n", pszLocation, iErr);
		return false;
	}

	return true;
}

/*!****************************************************************************
@Function		main
@Input			argc		Number of arguments
@Input			argv		Command line arguments
@Return		int			result code to OS
@Description	Main function of the program
******************************************************************************/

bool showWindow = true;

class IdentifiedObject
{
	int _x, _y, _ID;
	double  _size, sizeAcc;
	bool _circle, _square, isnew;
	double lastSeen, timeRenewed;
	CvScalar color;
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
	IdentifiedObject(const int x, const int y, const double size, bool isCircle, bool isSquare, int ID, double curTime) {
		color = cvScalar(rand() % 255, rand() % 255, rand() % 255);
		_x = x;
		_y = y;
		_size = size;
		_circle = isCircle;
		_square = isSquare;
		_ID = ID;
		//std::cout << curTime << std::endl;
		lastSeen = curTime;
		timeRenewed = curTime;
		isnew = true;
		sizeAcc = 0.80; // used to discern objects from eachother
		//_border = isBorder;
	}
	~IdentifiedObject() { 
		//	delete &color;
	}


	bool isNew()
	{
		return isnew;
	}


	bool liesWithin(int x, int y, bool recentlyChanged, int accuracy, double curTime, double curSize)
	{
		if (recentlyChanged || isnew)
		{
			if (_x - (2 * accuracy) < x && _x + (accuracy * 2) > x)
			{
				if (_y - (2 * accuracy) < y && _y + (accuracy * 2) > y) {
					_x = x;
					_y = y;


					lastSeen = curTime;
					_size = curSize;
					if (!isnew) {
						// recentlyChanged was triggered! This means we should renew the object
						isnew = true;
						timeRenewed = curTime;
					}


					return true;




				}
				return false;
			}
			else {
				// object not anywhere close 
				return false;
			}
		}
		else if (_x - accuracy < x && _x + accuracy > x) {
			if (_y - accuracy < y && _y + accuracy > y) {
				if (curSize > sizeAcc * _size && curSize < _size / sizeAcc)
				{
					_x = x;
					_y = y;
					lastSeen = curTime;
					return true;
				}
			}
			return false;
		} 
		else {
			return false;
		}
	}
	int getID()
	{
		return _ID;
	}


	void updatePosition(const int x, const int y)
	{
		_x = x;
		_y = y;
	}
	CvScalar getColor()
	{
		return color;
	}


	CvPoint getPosition(double curTime)
	{
		if (curTime == -1.0)
			// GUI thread
		{
			return cvPoint((float)_x, (float)_y);
		}
		if (curTime - timeRenewed > 1.0)
		{
			isnew = false;
		}
		if (curTime - lastSeen < 1.0)
		{
			//std::cout << curTime-lastSeen << " " <<curTime << " last" << lastSeen << std::endl;
			return cvPoint((float)_x, (float)_y);
		}
		else {
			if (showWindow) std::cout << "deleting ID " << _ID << std::endl;
			// too long ago! Object marked for removal
			return cvPoint((float)-1,(float)-1);
		}
	}






	std::string getShape()
	{
		if (_circle == true) return "circle";
		else if (_square ==true ) return "square";
		//else if (_border == true) return "border";
		else {return "different kind of object";}
	}
};


int maxPixelTravel = 20;
int IDcounter = 0;
int hue =0;
int sat =90;
int val =0;
int maxHue =80;
int maxSat =255;
int maxVal =255;
int minAreaSize = 500;
int maxAreaSize = 2500;
std::vector<IdentifiedObject> toRemove;
std::vector<IdentifiedObject> identifiedObjects;


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
static double s_line_length = 0.5;

void SecondThread()
{
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
	//cv::namedWindow("Animation",CV_WINDOW_OPENGL | CV_WINDOW_AUTOSIZE);
	//cv::resizeWindow("Animation", 640, 480);
	//cv::setOpenGlDrawCallback("Animation", on_opengl);
	




	////////////////////END OLD CODE PART 1
	
	
	

	bool				bDemoDone	= false;

	// X11 variables
	Window				x11Window	= 0;
	Display*			x11Display	= 0;
	long				x11Screen	= 0;
	XVisualInfo*		x11Visual	= 0;
	Colormap			x11Colormap	= 0;

	// EGL variables
	EGLDisplay			eglDisplay	= 0;
	EGLConfig			eglConfig	= 0;
	EGLSurface			eglSurface	= 0;
	EGLContext			eglContext	= 0;
	GLuint	ui32Vbo = 0; // Vertex buffer object handle

	/*
	EGL has to create a context for OpenGL ES. Our OpenGL ES resources
	like textures will only be valid inside this context
	(or shared contexts).
	Creation of this context takes place at step 7.
	*/
	EGLint ai32ContextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	// Matrix used for projection model view (PMVMatrix)
	float pfIdentity[] =
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	// Fragment and vertex shaders code
	const char* pszFragShader = "\
								void main (void)\
								{\
								gl_FragColor = vec4(1.0, 1.0, 0.66 ,1.0);\
								}";

	const char* pszVertShader = "\
								attribute highp vec4	myVertex;\
								uniform mediump mat4	myPMVMatrix;\
								void main(void)\
								{\
								gl_Position = myPMVMatrix * myVertex;\
								}";
	/*
	Step 0 - Create a NativeWindowType that we can use it for OpenGL ES output
	*/
	Window					sRootWindow;
	XSetWindowAttributes	sWA;
	unsigned int			ui32Mask;
	int						i32Depth;
	int 					i32Width, i32Height;

	// Initializes the display and screen
	x11Display = XOpenDisplay( 0 );
	if (!x11Display)
	{
		printf("Error: Unable to open X display\n");
		goto cleanup;
	}
	x11Screen = XDefaultScreen( x11Display );

	// Gets the window parameters
	sRootWindow = RootWindow(x11Display, x11Screen);
	i32Depth = DefaultDepth(x11Display, x11Screen);
	x11Visual = new XVisualInfo;
	XMatchVisualInfo( x11Display, x11Screen, i32Depth, TrueColor, x11Visual);
	if (!x11Visual)
	{
		printf("Error: Unable to acquire visual\n");
		goto cleanup;
	}
	x11Colormap = XCreateColormap( x11Display, sRootWindow, x11Visual->visual, AllocNone );
	sWA.colormap = x11Colormap;

	// Add to these for handling other events
	sWA.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	ui32Mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

	i32Width  = WINDOW_WIDTH  < XDisplayWidth(x11Display, x11Screen) ? WINDOW_WIDTH : XDisplayWidth(x11Display, x11Screen);
	i32Height = WINDOW_HEIGHT < XDisplayHeight(x11Display,x11Screen) ? WINDOW_HEIGHT: XDisplayHeight(x11Display,x11Screen);

	// Creates the X11 window
	x11Window = XCreateWindow( x11Display, RootWindow(x11Display, x11Screen), 0, 0, i32Width, i32Height,
		0, CopyFromParent, InputOutput, CopyFromParent, ui32Mask, &sWA);
	XMapWindow(x11Display, x11Window);
	XFlush(x11Display);

	/*
	Step 1 - Get the default display.
	EGL uses the concept of a "display" which in most environments
	corresponds to a single physical screen. Since we usually want
	to draw to the main screen or only have a single screen to begin
	with, we let EGL pick the default display.
	Querying other displays is platform specific.
	*/
	eglDisplay = eglGetDisplay((EGLNativeDisplayType)x11Display);

	/*
	Step 2 - Initialize EGL.
	EGL has to be initialized with the display obtained in the
	previous step. We cannot use other EGL functions except
	eglGetDisplay and eglGetError before eglInitialize has been
	called.
	If we're not interested in the EGL version number we can just
	pass NULL for the second and third parameters.
	*/
	EGLint iMajorVersion, iMinorVersion;
	if (!eglInitialize(eglDisplay, &iMajorVersion, &iMinorVersion))
	{
		printf("Error: eglInitialize() failed.\n");
		goto cleanup;
	}

	/*
	Step 3 - Make OpenGL ES the current API.
	EGL provides ways to set up OpenGL ES and OpenVG contexts
	(and possibly other graphics APIs in the future), so we need
	to specify the "current API".
	*/
	eglBindAPI(EGL_OPENGL_ES_API);

	if (!TestEGLError("eglBindAPI"))
	{
		goto cleanup;
	}

	/*
	Step 4 - Specify the required configuration attributes.
	An EGL "configuration" describes the pixel format and type of
	surfaces that can be used for drawing.
	For now we just want to use a 16 bit RGB surface that is a
	Window surface, i.e. it will be visible on screen. The list
	has to contain key/value pairs, terminated with EGL_NONE.
	*/
	EGLint pi32ConfigAttribs[5];
	pi32ConfigAttribs[0] = EGL_SURFACE_TYPE;
	pi32ConfigAttribs[1] = EGL_WINDOW_BIT;
	pi32ConfigAttribs[2] = EGL_RENDERABLE_TYPE;
	pi32ConfigAttribs[3] = EGL_OPENGL_ES2_BIT;
	pi32ConfigAttribs[4] = EGL_NONE;

	/*
	Step 5 - Find a config that matches all requirements.
	eglChooseConfig provides a list of all available configurations
	that meet or exceed the requirements given as the second
	argument. In most cases we just want the first config that meets
	all criteria, so we can limit the number of configs returned to 1.
	*/
	EGLint iConfigs;
	if (!eglChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &iConfigs) || (iConfigs != 1))
	{
		printf("Error: eglChooseConfig() failed.\n");
		goto cleanup;
	}

	/*
	Step 6 - Create a surface to draw to.
	Use the config picked in the previous step and the native window
	handle when available to create a window surface. A window surface
	is one that will be visible on screen inside the native display (or
	fullscreen if there is no windowing system).
	Pixmaps and pbuffers are surfaces which only exist in off-screen
	memory.
	*/
	eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)x11Window, NULL);

	if (!TestEGLError("eglCreateWindowSurface"))
	{
		goto cleanup;
	}

	/*
	Step 7 - Create a context.
	*/
	eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, ai32ContextAttribs);
	if (!TestEGLError("eglCreateContext"))
	{
		goto cleanup;
	}

	/*
	Step 8 - Bind the context to the current thread and use our
	window surface for drawing and reading.
	Contexts are bound to a thread. This means you don't have to
	worry about other threads and processes interfering with your
	OpenGL ES application.
	We need to specify a surface that will be the target of all
	subsequent drawing operations, and one that will be the source
	of read operations. They can be the same surface.
	*/
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	if (!TestEGLError("eglMakeCurrent"))
	{
		goto cleanup;
	}

	/*
	Step 9 - Draw something with OpenGL ES.
	At this point everything is initialized and we're ready to use
	OpenGL ES to draw something on the screen.
	*/

	GLuint uiFragShader, uiVertShader;		// Used to hold the fragment and vertex shader handles
	GLuint uiProgramObject;					// Used to hold the program handle (made out of the two previous shaders

	// Create the fragment shader object
	uiFragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load the source code into it
	glShaderSource(uiFragShader, 1, (const char**)&pszFragShader, NULL);

	// Compile the source code
	glCompileShader(uiFragShader);

	// Check if compilation succeeded
	GLint bShaderCompiled;
	glGetShaderiv(uiFragShader, GL_COMPILE_STATUS, &bShaderCompiled);

	if (!bShaderCompiled)
	{
		// An error happened, first retrieve the length of the log message
		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(uiFragShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

		// Allocate enough space for the message and retrieve it
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetShaderInfoLog(uiFragShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);

		// Displays the error
		printf("Failed to compile fragment shader: %s\n", pszInfoLog);
		delete [] pszInfoLog;
		goto cleanup;
	}

	// Loads the vertex shader in the same way
	uiVertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(uiVertShader, 1, (const char**)&pszVertShader, NULL);
	glCompileShader(uiVertShader);
	glGetShaderiv(uiVertShader, GL_COMPILE_STATUS, &bShaderCompiled);

	if (!bShaderCompiled)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(uiVertShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetShaderInfoLog(uiVertShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		printf("Failed to compile vertex shader: %s\n", pszInfoLog);
		delete [] pszInfoLog;
		goto cleanup;
	}

	// Create the shader program
	uiProgramObject = glCreateProgram();

	// Attach the fragment and vertex shaders to it
	glAttachShader(uiProgramObject, uiFragShader);
	glAttachShader(uiProgramObject, uiVertShader);

	// Bind the custom vertex attribute "myVertex" to location VERTEX_ARRAY
	glBindAttribLocation(uiProgramObject, VERTEX_ARRAY, "myVertex");

	// Link the program
	glLinkProgram(uiProgramObject);

	// Check if linking succeeded in the same way we checked for compilation success
	GLint bLinked;
	glGetProgramiv(uiProgramObject, GL_LINK_STATUS, &bLinked);

	if (!bLinked)
	{
		int ui32InfoLogLength, ui32CharsWritten;
		glGetProgramiv(uiProgramObject, GL_INFO_LOG_LENGTH, &ui32InfoLogLength);
		char* pszInfoLog = new char[ui32InfoLogLength];
		glGetProgramInfoLog(uiProgramObject, ui32InfoLogLength, &ui32CharsWritten, pszInfoLog);
		printf("Failed to link program: %s\n", pszInfoLog);
		delete [] pszInfoLog;
		goto cleanup;
	}

	// Actually use the created program
	glUseProgram(uiProgramObject);

	// Sets the clear color.
	// The colours are passed per channel (red,green,blue,alpha) as float values from 0.0 to 1.0
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // clear blue

	// We're going to draw a triangle to the screen so create a vertex buffer object for our triangle
	{
		// Interleaved vertex data
		GLfloat afVertices[] = {	-0.8f,-0.4f,0.0f, // Position
			0.8f ,-0.4f,0.0f,
			0.0f ,0.4f ,0.0f};

		// Generate the vertex buffer object (VBO)
		glGenBuffers(1, &ui32Vbo);

		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);

		// Set the buffer's data
		unsigned int uiSize = 3 * (sizeof(GLfloat) * 3); // Calc afVertices size (3 vertices * stride (3 GLfloats per vertex))
		glBufferData(GL_ARRAY_BUFFER, uiSize, afVertices, GL_STATIC_DRAW);
	}

	// Draws a triangle for 800 frames
	for(int i = 0; i < 800; ++i)
	{
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
					std::cout << "animation framerate: ";
					double animFramerate = 50 / (curtime - lasttime);
					std::cout << animFramerate << std::endl;
					lasttime = curtime;
				}
			}
			


		// Check if the message handler finished the demo
		if (bDemoDone) break;

		/*
		Clears the color buffer.
		glClear() can also be used to clear the depth or stencil buffer
		(GL_DEPTH_BUFFER_BIT or GL_STENCIL_BUFFER_BIT)
		*/
		glClear(GL_COLOR_BUFFER_BIT);
		if (!TestEGLError("glClear"))
		{
			goto cleanup;
		}

		/*
		Bind the projection model view matrix (PMVMatrix) to
		the associated uniform variable in the shader
		*/

		// First gets the location of that variable in the shader using its name
		int i32Location = glGetUniformLocation(uiProgramObject, "myPMVMatrix");

		// Then passes the matrix to that variable
		glUniformMatrix4fv( i32Location, 1, GL_FALSE, pfIdentity);

		/*
		Enable the custom vertex attribute at index VERTEX_ARRAY.
		We previously binded that index to the variable in our shader "vec4 MyVertex;"
		*/
		glEnableVertexAttribArray(VERTEX_ARRAY);

		// Sets the vertex data to this attribute index
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 0, 0);

		/*
		Draws a non-indexed triangle array from the pointers previously given.
		This function allows the use of other primitive types : triangle strips, lines, ...
		For indexed geometry, use the function glDrawElements() with an index list.
		*/
		glDrawArrays(GL_TRIANGLES, 0, 3);
		if (!TestEGLError("glDrawArrays"))
		{
			goto cleanup;
		}

		/*
		Swap Buffers.
		Brings to the native display the current render surface.
		*/
		eglSwapBuffers(eglDisplay, eglSurface);

		if (!TestEGLError("eglSwapBuffers"))
		{
			goto cleanup;
		}

		// Managing the X11 messages
		int i32NumMessages = XPending( x11Display );
		for( int i = 0; i < i32NumMessages; i++ )
		{
			XEvent	event;
			XNextEvent( x11Display, &event );

			switch( event.type )
			{
				// Exit on mouse click
			case ButtonPress:
				bDemoDone = true;
				break;
			default:
				break;
			}
		}

		pthread_cond_wait(&cond, &mutex);
		}
		else {
			std::cout << "waiting ..." << std::endl;
			pthread_cond_wait(&cond, &mutex);
		}

	}

	// Frees the OpenGL handles for the program and the 2 shaders
	glDeleteProgram(uiProgramObject);
	glDeleteShader(uiFragShader);
	glDeleteShader(uiVertShader);

	// Delete the VBO as it is no longer needed
	glDeleteBuffers(1, &ui32Vbo);

	/*
	Step 10 - Terminate OpenGL ES and destroy the window (if present).
	eglTerminate takes care of destroying any context or surface created
	with this display, so we don't need to call eglDestroySurface or
	eglDestroyContext here.
	*/
cleanup:
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ;
	eglTerminate(eglDisplay);

	/*
	Step 11 - Destroy the eglWindow.
	Again, this is platform specific and delegated to a separate function
	*/
	if (x11Window) XDestroyWindow(x11Display, x11Window);
	if (x11Colormap) XFreeColormap( x11Display, x11Colormap );
	if (x11Display) XCloseDisplay(x11Display);

	delete x11Visual;

	return;
	
	
	
	
	
	
	
	
	
	
	
}

CvCapture *capture = 0; 
int captureNo = 1;


bool calibrateSwitch = false;
CvRect projectionArea = cvRect(0,0,320,240);


bool recentlyChanged = false;






void StartCapture()
{
	std::cout << "capturing"  << std::endl;

	//cvNamedWindow("result"); 
	try {
		// warning: the catch thingy doesn't work
		// video 1 == device 11.10


		capture = cvCaptureFromCAM(captureNo); 
	}
	catch (std::exception& excpt) {
		std::cout << "error capturing, video input not found" << std::endl;
		fprintf(stderr, "Cannot open initialize webcam!\n"); 
		// capture = cvCaptureFromCAM(1);
	}


}

void trackbarMoved (int id)
{
	id;
	recentlyChanged = true;
}

IdentifiedObject IdentifyObject (CvSeq* detectedObject,double curTime)
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
	double minBoxHeight = (double) minBoxSquare.height;
	double minBoxWidth= (double) minBoxSquare.width;


	// calculate the relative difference in height and width of the item (independent of its orientation => minbox)
	double ratio = minBoxHeight / minBoxWidth;
	if (ratio < 1) ratio = 1 / ratio;


	// values which have worked in the past for circles vs squares, based on a couple of math assumptions
	ratio = ratio * ratio * 1.05;


	double rectHeight = (double) boundingRect.height;
	double rectWidth = (double) boundingRect.width;


	if (minBoxHeight <= rectHeight * ratio && minBoxHeight >= rectHeight / ratio)
	{
		if (minBoxWidth <= rectWidth * ratio && minBoxWidth >= rectWidth / ratio)
		{
			// double check with a non-powered factor to smoothen out results
			if (minBoxWidth * 0.9 < minBoxHeight && minBoxWidth * 1.1 > minBoxHeight)
			{
				isCircle = true;
			}
			else {
				// not square-ish  (elliptical?)
			}
		}
	}


	CvRect minBoxRect = cvRect(0,0,int(minBoxSquare.width),int(minBoxSquare.height));
	double minBoxArea = double(minBoxRect.width * minBoxRect.height);
	CvPoint2D32f center = minBox.center;
	int centerX = int(center.x + 0.5);
	int centerY = int(center.y + 0.5);




	if (isCircle)
	{
		double const PI = 4 * atan(1);
		double avgRadius = (rectHeight + rectWidth) /4;
		double expCircArea = PI * avgRadius * avgRadius;
		double measuredArea = cvContourArea(detectedObject);
		if (measuredArea > expCircArea * 1.02)
		{
			// oops I'm  a square after all, just nearly vertical
			isCircle = false;
			isSquare = true;
		}
	}


	//	if (minBoxArea > 1.05 * boundingRectArea)
	//	{
	//		isSquare = true;
	//	}
	//	
	//	else {
	//		double ratio = double(minBoxSquare.height / minBoxSquare.width);
	//		if (ratio > 0.95 && ratio < 1.05)
	//	{
	//		//std::cout << ratio;
	//		isCircle = true;
	//	}
	//	}




	IDcounter++;
	IdentifiedObject object = IdentifiedObject(centerX,centerY,minBoxArea,isCircle,isSquare, IDcounter, curTime);
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
	bool found = false;
	//for (seqX::iterator i = unidentifiedObjects.begin(); i != unidentifiedObjects.end(); ++i)
	struct timeval tim;


	for (CvSeq* seq : unidentifiedObjects)
	{
		//
		CvRect boundingRect = cvBoundingRect(seq);
		double dx = 0.5 * boundingRect.width + boundingRect.x;
		double dy = 0.5 * boundingRect.height + boundingRect.y;
		//const long double sysTime = time(0);
		//	const long curTime = (long)sysTime*1000;


		gettimeofday(&tim, NULL);
		double curTimeD = tim.tv_sec +(tim.tv_usec/1000000.0);


		//curTimeD = curTimeD * 1000;
		// long curTime = (long)curTimeD;
		int i = 0;
		for (i=0; i<identifiedObjects.size(); i++)
		{
			if (identifiedObjects.at(i).liesWithin((int)dx,(int)dy, recentlyChanged, maxPixelTravel, curTimeD, cvContourArea(seq)))
			{


				found =true;
				break;
			}
		}
		//	for  (IdentifiedObject io : identifiedObjects)
		//{
		//	
		//	// needs to update the item itself too
		//	
		//	if (io.liesWithin((int)dx,(int)dy, recentlyChanged, maxPixelTravel, curTimeD, cvContourArea(seq)))
		//	{
		//		
		//		std::cout << "object recognized, hello!" << std::endl;
		//		found = true;
		//		break;
		//	}
		//	
		//}


		if (!found) {
			if (showWindow) std::cout << "Object added!" << std::endl;
			IdentifiedObject ioX = IdentifyObject(seq,curTimeD);
			identifiedObjects.insert(identifiedObjects.end(),ioX);
		}


		//	std::cout << "I'm thinking object ";
		//	std::cout << counter2;
		//	std::cout << " is a " + x.getShape()<< std::endl;
		//	CvScalar col; 
		//	if (x.getShape() == "square")col = cvScalar(255,0,0);
		//	else if (x.getShape() == "circle") col = cvScalar(0,255,0);
		//	else { col = cvScalar(255,255,255);}
		//	counter2++;
		//	cvDrawContours(frame,seq,col,cvScalar(1,1,1 ),10);
		//delete seq;
		//seq = NULL;
	}
	if (recentlyChanged) recentlyChanged = false;




	// draw projection area
	cvRectangle(frame, cvPoint(projectionArea.x,projectionArea.y), cvPoint(projectionArea.x + projectionArea.width, projectionArea.y + projectionArea.height),cvScalar(1,255,255));


	// any objects that need to be removed?
	for (IdentifiedObject io : identifiedObjects)
	{
		//sysTimeX = time(0);
		//		std::cout << sysTimeX  << std::endl;
		//	curTimeX = (1000 * sysTimeX);
		//	long curTimeZ = (long)curTimeX;
		//	std::cout << curTimeZ << std::endl;
		gettimeofday(&tim, NULL);
		double curTimeD = tim.tv_sec +(tim.tv_usec/1000000.0);
		//curTimeD = curTimeD * 1000;
		// long curTime = (long)curTimeD;
		//std::cout << "proper time?: " << curTime << std::endl;
		CvPoint center = io.getPosition(curTimeD);
		if (center.x < 0)
		{
			//toRemove.Add(io);
			//std::cout << "object marked for removal" << std::endl;
			toRemove.insert(toRemove.end(),io);
		}
		else {
			if (io.getShape() == "circle") cvCircle(frame, center, 5,io.getColor(),5);
			else if (io.getShape() == "square") cvCircle(frame, center, 5,io.getColor(),10);
			else { cvCircle(frame, center, 5,io.getColor(),1);}
		}


	}


	for (IdentifiedObject ioZ : toRemove)
	{
		//identifiedObjects.erase(iox);
		unsigned int i = 0;
		bool localfound = false;
		for (i=0; i<identifiedObjects.size(); i++)
		{
			IdentifiedObject ioz = identifiedObjects.at(i);


			if (ioZ.getID() == ioz.getID())
			{


				localfound =true;
				if (showWindow) std::cout << "deleting object" << ioz.getID() << std::endl; 
				break;
			}
		}
		if (localfound) 
		{
			identifiedObjects.erase(identifiedObjects.begin() + i);
			//std::cout << "deleted?" << std::endl;
		}


	}


	if (showWindow) {
		//cvNamedWindow("current");
		//cvCreateTrackbar("min Hue", "current", &hue, 255, trackbarMoved);
		//cvCreateTrackbar("max Hue", "current", &maxHue, 255, trackbarMoved);
		//cvCreateTrackbar("min Saturation", "current", &sat, 255, trackbarMoved);
		//cvCreateTrackbar("max Saturation", "current", &maxSat, 255, trackbarMoved);
		//cvCreateTrackbar("min Value", "current", &val, 255, trackbarMoved);
		//cvCreateTrackbar("max Value", "current", &maxVal, 255, trackbarMoved);
		//cvCreateTrackbar("min Object Size", "current", &minAreaSize, 2000, trackbarMoved);
		//cvCreateTrackbar("max Object Size", "current", &maxAreaSize, 50000, trackbarMoved);
		//cvCreateTrackbar("Max pixel travel", "current", &maxPixelTravel, 100, trackbarMoved);


		//cvShowImage("current",frame);


		//std::cout << key << std::endl;
		int key = cvWaitKey(10);
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
	toRemove.erase(toRemove.begin(),toRemove.end());
	//cvDestroyWindow("current");
}



int main(int argc, char **argv)
{
	// Variable set in the message handler to finish the demo
	int i = 0;
	  if ( argc != 1)
	  {
		  std::cout << "arg found: " << argv[1] << std::endl;
		  captureNo = atoi(argv[1]);
	  }


 boost::thread t2(&SecondThread);
 //t2.join();
 pthread_t threadID = (pthread_t) t2.native_handle();


 //struct sched_param param;
int sched_priority = 90;
pthread_setschedprio(threadID, sched_priority);
//pthread_attr_setschedparam(threadID,SCHED_RR, &param);
 

if (captureNo == 9)
{
	// 9 = no camera used
	while(true)
	{
		//std::cout << "Shhh! Main thread is sleeping ..." << std::endl;
		//std::cout.flush();
		//int key = cvWaitKey(1000);
	}
}
else
{
	std::cout << "starting capture" << std::endl;
	StartCapture();
}

 ///// END OPENCV CODE




}

/******************************************************************************
End of file (OGLES2HelloAPI_LinuxX11.cpp)
******************************************************************************/

