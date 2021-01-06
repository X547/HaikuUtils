#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <Rect.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <LayoutBuilder.h>
#include <private/interface/WindowStack.h>
#include <stdio.h>

enum {
	newMsg = 1,
};

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
	TestWindow(BRect frame): BWindow(frame, "TestApp", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		fView = new TestView(frame.OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
		fView->SetExplicitMinSize(BSize(3, 3));

		BMenuBar *menubar, *toolbar;
		BMenu *menu;
		BMenuItem *it;

		BMessage *newTabMsg = new BMessage(newMsg);
		newTabMsg->AddMessenger("window", BMessenger(this));

		menubar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
		menu = new BMenu("File");
			it = new BMenuItem("New window", new BMessage(newMsg), 'N'); it->SetTarget(be_app); menu->AddItem(it);
			it = new BMenuItem("New tab", newTabMsg, 'T'); it->SetTarget(be_app); menu->AddItem(it);
			it = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'); menu->AddItem(it);
			it = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'); it->SetTarget(be_app); menu->AddItem(it);
			menubar->AddItem(menu);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(menubar)
			.Add(fView)
			.End();
	}

	void Quit()
	{
		if (be_app->LockLooper()) {
			if (be_app->CountWindows() <= 1)
				be_app_messenger.SendMessage(B_QUIT_REQUESTED);
			be_app->UnlockLooper();
		}
		BWindow::Quit();
	}

};

class TestApplication: public BApplication
{
public:
	TestApplication(): BApplication("application/x-vnd.test-app")
	{}

	void ReadyToRun()
	{
		PostMessage(newMsg);
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case newMsg: {
			BWindow *fWnd = new TestWindow(BRect(0, 0, 320, 240).OffsetByCopy(64, 64));
			BMessenger obj;
			if (msg->FindMessenger("window", &obj) >= B_OK) {
				BWindow *srcWnd = dynamic_cast<BWindow*>(obj.Target(NULL));
				if (srcWnd != NULL) {
					BWindowStack stack(srcWnd);
					stack.AddWindow(fWnd);
				}
			}
			fWnd->Show();
			break;
		}
		default:
			BApplication::MessageReceived(msg);
		}
	}

};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
