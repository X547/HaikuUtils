#include "SampleGLApp.h"
#include "SampleGLWindow.h"
#include <GLView.h>
#include <Screen.h>

SampleGLApp::SampleGLApp()
	: BApplication("application/x-vnd.Be-GLSample")
{
	newPos = BPoint(32, 32);
	NewWindow();
}

SampleGLWindow *SampleGLApp::NewWindow()
{
	BRect windowRect;
	uint32 type;
	SampleGLWindow *wnd = NULL;
	if (Lock()) {
		// Set type to the appropriate value for the
		// sample program you're working with.

		type = BGL_RGB/*|BGL_DOUBLE*/;

		windowRect.Set(0, 0, 255, 255);
		wnd = new SampleGLWindow(windowRect, type);
		BScreen screen(wnd);
		BRect screenRect = screen.Frame();
		windowRect.OffsetTo(newPos);
		newPos.x += 32; newPos.y += 32;
		if (windowRect.right > screenRect.right) {
			newPos.x += windowRect.Width() - screenRect.Width();
			windowRect.OffsetBy(windowRect.Width() - screenRect.Width(), 0);
		}
		if (windowRect.bottom > screenRect.bottom) {
			newPos.y += windowRect.Height() - screenRect.Height();
			windowRect.OffsetBy(0, windowRect.Height() - screenRect.Height());
		}
		wnd->MoveTo(windowRect.LeftTop());
		Unlock();
	}
	if (wnd != NULL) wnd->Show();
	return wnd;
}


int main()
{
	SampleGLApp app;
	app.Run();
	return 0;
}
