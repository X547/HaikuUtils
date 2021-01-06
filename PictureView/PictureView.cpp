#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Picture.h>
#include <Entry.h>
#include <File.h>
#include <stdio.h>
#include <math.h>

#include <private/shared/AutoDeleter.h>

enum {
	appWindowClosedMsg = 1,
};

class StateGroup
{
private:
	BView *fView;
public:
	StateGroup(BView *view): fView(view) {if (fView != NULL) fView->PushState();}
	~StateGroup() {if (fView != NULL) fView->PopState();}
};


class TestView: public BView
{
private:
	ObjectDeleter<BPicture> fPict;
	BPoint fOffset;
	float fScale;
	float fRotation;
	bool fTrack;
	BPoint fOldPos;

public:
	TestView(BRect frame, const char *name):
		BView(frame, name, B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_SUBPIXEL_PRECISE),
		fOffset(0, 0),
		fScale(1),
		fRotation(0),
		fTrack(false)
	{
	}

	void SetPicture(BPicture *pict)
	{
		fPict.SetTo(pict);
		Invalidate();
	}

	void Draw(BRect dirty)
	{
		if (fPict.Get() != NULL) {
			ScaleBy(fScale, fScale);
			RotateBy(fRotation);
			TranslateBy(fOffset.x, fOffset.y);
			DrawPicture(fPict.Get());
		}
	}

	void MouseDown(BPoint where)
	{
		if (!fTrack) {
			SetMouseEventMask(B_POINTER_EVENTS, 0);
			fTrack = true;
			fOldPos = where;
		}
	}

	void Translate(BPoint offset)
	{
		fOffset += BPoint(offset.x/fScale, offset.y/fScale);
		Invalidate();
	}

	void MouseUp(BPoint where)
	{
		fTrack = false;
	}

	void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
	{
		if (fTrack) {
			Translate(where - fOldPos);
		}
		fOldPos = where;
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case B_MOUSE_WHEEL_CHANGED: {
			float deltaY = 0;
			if (msg->FindFloat("be:wheel_delta_y", &deltaY) != B_OK || deltaY == 0)
				return;

			Translate(-fOldPos);
			fScale *= powf(sqrt(2), -deltaY);
			Translate(fOldPos);
			Invalidate();
			break;
		}
		default:
			BView::MessageReceived(msg);
		}
	}

};

class TestWindow: public BWindow
{
private:
	TestView *fView;

public:
	TestWindow(BRect frame): BWindow(frame, "TestApp", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new TestView(frame.OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
		AddChild(fView, NULL);
		fView->MakeFocus(true);
	}

	status_t Load(BEntry &entry)
	{
		status_t res;
		BFile file(&entry, B_READ_ONLY);
		res = file.InitCheck(); if (res < B_OK) return res;
		ObjectDeleter<BPicture> pict(new BPicture());
		pict->Unflatten(&file);
		fView->SetPicture(pict.Detach());
		SetTitle(entry.Name());
		return B_OK;
	}

	void Quit()
	{
		be_app_messenger.SendMessage(appWindowClosedMsg);
		BWindow::Quit();
	}

};


class TestApplication: public BApplication
{
private:
	int32 fWndCnt;

public:
	TestApplication(): BApplication("application/x-vnd.Test-PictureView"),
		fWndCnt(0)
	{
	}

	void ReadyToRun() {
		if (fWndCnt <= 0)
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	}

	void RefsReceived(BMessage *refsMsg)
	{
		entry_ref ref;
		if (refsMsg->FindRef("refs", &ref) < B_OK) return;
		BEntry entry(&ref);

		BRect rect(0, 0, 640, 480);
		rect.OffsetBy(64, 64);
		TestWindow *wnd = new TestWindow(rect);
		if (wnd->Load(entry) < B_OK) return;
		fWndCnt++;
		wnd->Show();
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case appWindowClosedMsg: {
			fWndCnt--;
			if (fWndCnt <= 0)
				be_app_messenger.SendMessage(B_QUIT_REQUESTED);
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
