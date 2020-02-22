#include "SampleGLApp.h"
#include "SampleGLWindow.h"
#include "SampleGLView.h"

SampleGLWindow::SampleGLWindow(BRect frame)
	: BWindow(frame, "OpenGL Test 2", B_TITLED_WINDOW, 0)
{
	AddShortcut('n', B_COMMAND_KEY, new BMessage('neww'));
	AddChild(theView=new SampleGLView(Bounds()));
}

bool SampleGLWindow::QuitRequested()
{
	if (be_app->CountWindows() == 1)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void SampleGLWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case 'neww': {
		((SampleGLApp*)be_app)->NewWindow();
		break;
	}
	default:
		BWindow::MessageReceived(msg);
	}
}
