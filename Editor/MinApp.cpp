#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Archivable.h>
#include <MessageFilter.h>
#include <Cursor.h>
#include <Dragger.h>
#include <stdio.h>
#include <algorithm>

static const char *appSignature = "application/x-vnd.Test-FormEditor";

enum {
	markerSize = 6
};

enum PartId {
	noPart,
	bgPart,
	selPart,
	markerPart,
	objPart = markerPart + 4
};

struct Part
{
	PartId id;
	BView *obj;
};

struct ViewList
{
	ViewList *next;
	BView *view;

	ViewList(BView *view): next(NULL), view(view) {}
};


void Assert(bool cond)
{
	if (!cond) {
		fprintf(stderr, "assert failed\n");
		debugger("assert failed");
		exit(1);
	}
}


BView *CopyView(BView *src)
{
	BMessage archive(B_ARCHIVED_OBJECT);
	src->Archive(&archive);
	BArchivable *arch = instantiate_object(&archive);
	Assert(arch != NULL);
	BView *view = dynamic_cast<BView*>(arch);
	Assert(view != NULL);
	return view;
}

BView *GetSubViewContains(BView *container, BView *view)
{
	BView *upView = NULL;
	if (view == NULL) return NULL;
	if (view == container) return NULL;
	upView = view->Parent();
	while ((upView != NULL) && (upView != container)) {view = upView; upView = upView->Parent();}
	if (upView == NULL) return NULL;
	return view;
}

BView *ViewAtRecursive(BView *base, BPoint pos)
{
	BView *res = base;
	BView *view = base->ChildAt(0);
	while (view != NULL) {
		if (view->Frame().Contains(pos)) {
			res = ViewAtRecursive(view, pos - view->Frame().LeftTop());
		}
		view = view->NextSibling();
	}
	return res;
}

class InputCatcher: public BView
{
private:
	BView *fBase;

public:
	InputCatcher(BRect bounds, BView *base): BView(bounds, "input catcher", B_FOLLOW_ALL, B_FRAME_EVENTS | B_TRANSPARENT_BACKGROUND)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	void MouseDown(BPoint pos)
	{
		fBase->MouseDown(pos);
	}

	void MouseUp(BPoint pos)
	{
		fBase->MouseUp(pos);
	}

	void MouseMoved(BPoint pos, uint32 transit, const BMessage* dragMsg)
	{
		fBase->MouseMoved(pos, transit, dragMsg);
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case B_ARCHIVED_OBJECT:
			if (msg->WasDropped()) {
				fBase->MessageReceived(msg);
				return;
			}
			break;
		}
		BView::MessageReceived(msg);
	}

};

class TestView: public BView
{
private:
	ViewList *fSelected;
	Part fPart, fOldPart;
	bool fTrack;
	BPoint fPos, fPos0;
	InputCatcher *fInputCatcher;
	bool fInputCatcherEnabled;

	void _Init()
	{

		fSelected = NULL; fTrack = false;
		fPart.id = noPart; fPart.obj = NULL;
		fOldPart = fPart;

		//AddChild(new BDragger(BRect(0, 0, 7, 7), this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));

		/*
		fInputCatcher = new InputCatcher(Bounds(), this);
		AddChild(fInputCatcher);
		fInputCatcherEnabled = true;
		*/
	}

public:
	TestView(BRect frame, const char *name):
		BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_DRAW_ON_CHILDREN | B_SUBPIXEL_PRECISE)
	{
		_Init();
	}

	TestView(BMessage* archive):
		BView(archive)
	{
		_Init();
	}

	BArchivable *Instantiate(BMessage *archive)
	{
		if (validate_instantiation(archive, "TestView"))
			return new TestView(archive);
		return NULL;
	}

	status_t Archive(BMessage* archive)
	{
		BView::Archive(archive);
		archive->AddString("add_on", appSignature);
		return B_OK;
	}

	void EnableInputCatcher(bool enable)
	{
		if (enable != fInputCatcherEnabled) {
			if (enable) {
				fInputCatcher->Show();
			} else {
				fInputCatcher->Hide();
			}
			fInputCatcherEnabled = enable;
		}
	}

	rgb_color SelColor()
	{
		return Window()->IsActive()? make_color(0x00, 0x99, 0xff): make_color(0xcc, 0xcc, 0xcc);
	}

	BRect MarkerRect(BPoint pos)
	{
		return BRect(pos - BPoint(markerSize/2.0, markerSize/2.0), pos + BPoint(markerSize/2.0, markerSize/2.0));
	}

	bool IsSelected(BView *view)
	{
		ViewList *it = fSelected;
		while (it != NULL && !(it->view == view)) it = it->next;
		return it != NULL;
	}

	bool IsSingleton()
	{
		return (fSelected != NULL) && (fSelected->next == NULL);
	}

	BView *Singleton()
	{
		if (IsSingleton()) {
			return fSelected->view;
		}
		return NULL;
	}

	void ClearSelection()
	{
		if (fSelected != NULL) {
			Invalidate(Bounds());
			while (fSelected != NULL) {
				ViewList *cur = fSelected;
				fSelected = fSelected->next;
				delete cur;
			}
		}
	}

	void SelectObj(BView *obj)
	{
		ViewList *curSel;
		ClearSelection();
		MakeFocus(true);
		Invalidate(Bounds());
		curSel = new ViewList(obj);
		fSelected = curSel;
	}

	void SelectRect(BRect rect)
	{
		ViewList *prevSel = NULL, *lastSel = NULL, *curSel = NULL;
		ClearSelection();
		MakeFocus(true);
		Invalidate(Bounds());
		BView *it = ChildAt(0);
		while (it != NULL) {
			if (rect.Contains(it->Frame())) {
				curSel = new ViewList(it);
				if (fSelected == NULL) {
					fSelected = curSel;
				} else {
					lastSel->next = curSel;
				}
				lastSel = curSel;
			}
			it = it->NextSibling();
		}
	}

	void DeleteSelection()
	{
		if (fSelected != NULL) {
			Invalidate(Bounds());
			while (fSelected != NULL) {
				ViewList *cur = fSelected;
				fSelected = fSelected->next;
				RemoveChild(cur->view); delete cur->view;
				delete cur;
			}
		}
	}

	void MoveSelection(BPoint shift)
	{
		if (fSelected != NULL) {
			Invalidate(Bounds());
			ViewList *sel = fSelected;
			while (sel != NULL) {
				sel->view->MoveBy(shift.x, shift.y);
				sel = sel->next;
			}
		}
	}

	void CopySelectionLocal()
	{
		ViewList *oldSel = NULL, *oldCurSel = NULL;
		ViewList *prevSel = NULL, *lastSel = NULL, *curSel = NULL;
		BView *view = NULL;
		oldSel = fSelected; fSelected = NULL;
		while (oldSel != NULL) {
			oldCurSel = oldSel; oldSel = oldSel->next;
			view = CopyView(oldCurSel->view);
			AddChild(view);
			curSel = new ViewList(view);
			if (fSelected == NULL) {
				fSelected = curSel;
			} else {
				lastSel->next = curSel;
			}
			lastSel = curSel;
			delete oldCurSel;
		}
	}

	void SetPart()
	{
		if ((fOldPart.id != fPart.id) || (fOldPart.obj != fPart.obj)) {
			switch (fPart.id) {
			case bgPart: {
				BCursor cursor(B_CURSOR_ID_CROSS_HAIR); be_app->SetCursor(&cursor);
				break;
			}
			case markerPart + 0:
			case markerPart + 3: {
				BCursor cursor(B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST); be_app->SetCursor(&cursor);
				break;
			}
			case markerPart + 1:
			case markerPart + 2: {
				BCursor cursor(B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST); be_app->SetCursor(&cursor);
				break;
			}
			default:
				be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
			}
			fOldPart = fPart;
		}
	}

	void UpdatePart()
	{
		fPart.id = bgPart; fPart.obj = NULL;
		ViewList *sel = fSelected;
		if (IsSingleton()) {
			BRect singlFrame = Singleton()->Frame();
			for (int i = 0; i < 4; i++) {
				BPoint markerPos;
				markerPos.x = (i % 2 == 0)? singlFrame.left: singlFrame.right;
				markerPos.y = (i / 2 == 0)? singlFrame.top: singlFrame.bottom;
				if (MarkerRect(markerPos).Contains(fPos)) {
					fPart.id = PartId(markerPart + i); fPart.obj = NULL;
				}
			}
		}
		if (fPart.id == bgPart) {
			while ((sel != NULL) && !(sel->view->Frame().Contains(fPos))) sel = sel->next;
			if (sel != NULL) {
				fPart.id = selPart; fPart.obj = NULL;
			}
		}
		if (fPart.id == bgPart) {
			BView *view = ChildAt(0);
			while (view != NULL) {
				if (view->Frame().Contains(fPos)) {
					fPart.id = objPart; fPart.obj = view;
				}
				view = view->NextSibling();
			}
		}
		SetPart();
	}

	void DrawMarker(BPoint pos)
	{
		BRect rect = MarkerRect(pos);
		PushState();
		SetHighColor(SelColor());
		SetLowColor(0xff, 0xff, 0xff);
		FillRect(rect, B_SOLID_LOW);
		StrokeRect(rect, B_SOLID_HIGH);
		PopState();
	}

	void DrawSelBorder(BRect rect, bool singleton)
	{
		rgb_color color = SelColor();
		rect.InsetBy(-1, -1);
		PushState();
		color.alpha = 0x33;
		SetHighColor(color);
		SetDrawingMode(B_OP_ALPHA);
		FillRect(rect);
		SetDrawingMode(B_OP_OVER);
		color.alpha = 0xff;
		SetHighColor(color);
		StrokeRect(rect);
		if (singleton) {
			DrawMarker(rect.LeftTop());
			DrawMarker(rect.RightTop());
			DrawMarker(rect.LeftBottom());
			DrawMarker(rect.RightBottom());
		}
		PopState();
	}

	void DrawFocusBorder(BRect rect)
	{
		rgb_color color = SelColor();
		rect.InsetBy(-1, -1);
		PushState();
		SetHighColor(0, 0, 0);
		StrokeRect(rect);
		rect.left -= 1; rect.top -= 1; rect.right += 2; rect.bottom += 2;
		SetPenSize(2);
		SetDrawingMode(B_OP_ALPHA);
		color.alpha = 0x99;
		SetHighColor(color);
		StrokeRect(rect);
		PopState();
	}

	void DrawAfterChildren(BRect dirty)
	{
		BView *focus = Window()->CurrentFocus();
		BView *subView = GetSubViewContains(this, focus);
		if (subView != NULL) {
			DrawFocusBorder(subView->Frame());
		}
		ViewList *sel = fSelected;
		while (sel != NULL) {
			DrawSelBorder(sel->view->Frame(), IsSingleton());
			sel = sel->next;
		}
		if (fTrack) {
			if (fPart.id == bgPart) {
				BRect rect(fPos0, fPos);
				if (rect.left > rect.right) std::swap(rect.left, rect.right);
				if (rect.top > rect.bottom) std::swap(rect.top, rect.bottom);
				SelectRect(rect);
				DrawSelBorder(rect, false);
			}
		}
	}

	void MouseDown(BPoint pos)
	{
		int32 modifiers = 0;
		int32 clicks = 0;
		Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers);
		Looper()->CurrentMessage()->FindInt32("clicks", &clicks);
		if (fPart.id == objPart) {
			SelectObj(fPart.obj);
			fPart.id = selPart; fPart.obj = NULL;
			SetPart();
		}
		if ((fPart.id == selPart) || (fPart.id == bgPart)) {
			Window()->Activate();
			MakeFocus(true);
		}
		if ((fPart.id == selPart) && IsSingleton() && (clicks == 2)) {
			Singleton()->MakeFocus();
			fPart.id = noPart; fPart.obj = NULL; SetPart();
			ClearSelection();
		} else {
			if ((fPart.id == selPart) && (modifiers & B_COMMAND_KEY)) {
				CopySelectionLocal();
			}
		}
		fTrack = true;
		fPos0 = fPos;
		SetMouseEventMask(B_POINTER_EVENTS);
		Invalidate(Bounds());
	}

	void MouseUp(BPoint pos)
	{
		fTrack = false;
		UpdatePart();
		Invalidate(Bounds());
	}

	void MouseMoved(BPoint pos, uint32 transit, const BMessage* dragMsg)
	{
		if (!fTrack) {
			fPos = pos;
			switch (transit) {
			case B_EXITED_VIEW:
			case B_OUTSIDE_VIEW:
				fPart.id = noPart; SetPart(); break;
			default:
				UpdatePart();
			}
		} else {
			switch (fPart.id) {
			case selPart:
				MoveSelection(pos - fPos);
				break;
			case markerPart + 0 ... markerPart + 3: {
				BPoint shift = pos - fPos;
				BRect frame = Singleton()->Frame();
				Invalidate(Bounds());
				if ((fPart.id - markerPart) % 2 == 0) frame.left += shift.x; else frame.right += shift.x;
				if ((fPart.id - markerPart) / 2 == 0) frame.top += shift.y; else frame.bottom += shift.y;
				if (frame.Width() < 0) {std::swap(frame.left, frame.right); fPart.id = PartId(((fPart.id - markerPart) ^ 1) + markerPart); SetPart();}
				if (frame.Height() < 0) {std::swap(frame.top, frame.bottom); fPart.id = PartId(((fPart.id - markerPart) ^ 2) + markerPart); SetPart();}
				Singleton()->MoveTo(frame.LeftTop());
				Singleton()->ResizeTo(frame.Size());
				break;
			}
			}
			fPos = pos;
		}
		Invalidate(Bounds());
	}

	void KeyDown(const char* bytes, int32 numBytes)
	{
		switch (bytes[0]) {
		case B_DELETE:
		case B_BACKSPACE:
			DeleteSelection();
			break;
		case B_ESCAPE:
			ClearSelection();
			break;
		}
	}

	void WindowActivated(bool active)
	{
		Invalidate(Bounds());
	}

	void MakeFocus(bool focus)
	{
		BView::MakeFocus(focus);
		Invalidate(Bounds());
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case B_ARCHIVED_OBJECT: {
			if (!msg->WasDropped()) return;
			BPoint dropPt, dropOfs;
			msg->FindPoint("_drop_point_", &dropPt);
			msg->FindPoint("_drop_offset_", &dropOfs);
			ConvertFromScreen(&dropPt);
			BArchivable *arch = instantiate_object(msg);
			Assert(arch != NULL);
			BView *view = dynamic_cast<BView*>(arch);
			Assert(view != NULL);
			view->MoveTo(dropPt - dropOfs);
			view->SetResizingMode(B_FOLLOW_NONE);
			AddChild(view);
			ClearSelection();
			SelectObj(view);
			return;
		}
		}
		BView::MessageReceived(msg);
	}

};

class MouseMsgFilter: public BMessageFilter
{
private:
	TestView *fView;

public:
	MouseMsgFilter(TestView *view): BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE), fView(view)
	{
	}

	filter_result Filter(BMessage *msg, BHandler **handler)
	{
		if (*handler == NULL)
			return B_DISPATCH_MESSAGE;

		switch (msg->what) {
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED: {
			BView *view = dynamic_cast<BView*>(*handler);
			BView *targetView = fView;
			if (view == NULL)
				return B_DISPATCH_MESSAGE;
			if (GetSubViewContains(fView, view) != NULL) {
				BPoint screenPos, viewPos;
				msg->FindPoint("screen_where", &screenPos);
				BView *focus = GetSubViewContains(fView, fView->Window()->CurrentFocus());
				if ((focus != NULL) && (focus->Frame().Contains(fView->ConvertFromScreen(screenPos))))
					targetView = ViewAtRecursive(focus, focus->ConvertFromScreen(screenPos));
				*handler = targetView;
				viewPos = targetView->ConvertFromScreen(screenPos);
				msg->ReplacePoint("where", viewPos);
				msg->ReplacePoint("be:view_where", viewPos);
				return B_DISPATCH_MESSAGE;
			}
			break;
		}
		}
		return B_DISPATCH_MESSAGE;
	}
};

class TestWindow: public BWindow
{
private:
	TestView *fView;
public:
	TestWindow(BRect frame): BWindow(frame, "TestApp", B_TITLED_WINDOW, B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new TestView(frame.OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
		AddChild(fView, NULL);
		fView->MakeFocus(true);
		AddCommonFilter(new MouseMsgFilter(fView));
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
	TestApplication(): BApplication(appSignature)
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 255, 255).OffsetByCopy(64, 64));
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
