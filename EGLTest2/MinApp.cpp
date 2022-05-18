#include "MinApp.h"
#include "GlApp.h"
#include "BitmapHookView.h"

#include <stdio.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Region.h>
#include <Bitmap.h>
#include <Messenger.h>

#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoLocker.h>


class TestView: public BView
{
private:
	enum {
		stepMsg = 1,
	};

	BitmapHookView *fBmpHookView;
	ObjectDeleter<GlApp> fGlApp;

public:
	TestView(BRect frame, const char *name):
		BView(frame, name, B_FOLLOW_NONE, B_FRAME_EVENTS)
	{
		fBmpHookView = new BitmapHookView(frame.OffsetToCopy(B_ORIGIN), "bitmapHookView", B_FOLLOW_ALL, 0);
		AddChild(fBmpHookView);
	}

	void AttachedToWindow() override
	{
		fGlApp.SetTo(CreateGlApp(fBmpHookView->Hook()));
		if (!fGlApp.IsSet()) {
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
			return;
		}
		BMessenger(this).SendMessage(stepMsg);
	}
	
	void FrameResized(float newWidth, float newHeight) override
	{
		//fGlApp->Draw();
	}

	void MessageReceived(BMessage *msg) override
	{
		switch (msg->what) {
			case stepMsg: {
				fGlApp->Draw();
				BMessenger(this).SendMessage(stepMsg);
				return;
			};
		}
		BView::MessageReceived(msg);
	}
	
};


class TestWindow: public BWindow
{
private:
	TestView *fView;

public:
	TestWindow(BRect frame): BWindow(frame, "Minimal EGL App", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new TestView(Frame().OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
		AddChild(fView, NULL);
	}
};

class TestApplication: public BApplication
{
private:
	TestWindow *fWnd;

public:
	TestApplication(): BApplication("application/x-vnd.Test-App")
	{
		fWnd = new TestWindow(BRect(0, 0, 640 - 1, 480 - 1));
		fWnd->SetFlags(fWnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		fWnd->CenterOnScreen();
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
