#ifndef _SAMPLEGLWINDOW_H_
#define _SAMPLEGLWINDOW_H_

#include <Window.h>

class SampleGLView;

class SampleGLWindow : public BWindow {
public:
	             SampleGLWindow(BRect frame, uint32 type);
	virtual bool QuitRequested();
	        void MessageReceived(BMessage *msg);

private:
	SampleGLView* theView;
};

#endif	// _SAMPLEGLWINDOW_H_
