#ifndef _SAMPLEGLAPP_CPP_
#define _SAMPLEGLAPP_CPP_

#include <Application.h>

class SampleGLWindow;

class SampleGLApp : public BApplication {
public:
	SampleGLApp();
	SampleGLWindow *NewWindow();
private:
	BPoint newPos;
};

#endif	// _SAMPLEGLAPP_CPP_
