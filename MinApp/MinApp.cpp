#include <stdio.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>


class TestView: public BView
{
public:
	TestView(BRect frame, const char *name): BView(frame, name, B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_SUBPIXEL_PRECISE)
	{
		SetViewColor(0xff - 0x44, 0xff - 0x44, 0xff - 0x44);
	}

	void Draw(BRect dirty)
	{
		BRect rect = Frame().OffsetToCopy(B_ORIGIN);
		rect.left += 1; rect.top += 1;
		SetHighColor(0x44, 0x44, 0x44);
		SetPenSize(2);
		StrokeRect(rect, B_SOLID_HIGH);
		SetPenSize(1);
		StrokeLine(rect.LeftTop(), rect.RightBottom());
		StrokeLine(rect.RightTop(), rect.LeftBottom());
	}
	
};

class TestWindow: public BWindow
{
private:
	TestView *fView;
public:
	TestWindow(BRect frame): BWindow(frame, "TestApp", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new TestView(frame.OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
		AddChild(fView, NULL);
	}

	void Quit()
	{
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		BWindow::Quit();
	}

};

class TestApplication: public BApplication
{
private:
	TestWindow *fWnd;
public:
	TestApplication(): BApplication("application/x-vnd.Test-App")
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 256, 256).OffsetByCopy(64, 64));
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
