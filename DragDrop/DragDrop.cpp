#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Cursor.h>
#include <stdio.h>

#include <vector>

enum {
	dragMsg = 'drag'
};

void DrawRect(BView *view, BRect rect)
{
	rect.left += 1; rect.top += 1;
	view->PushState();
	view->SetHighColor(0x44, 0x44, 0x44);
	view->SetLowColor(0xff - 0x44, 0xff - 0x44, 0xff - 0x44);
	view->FillRect(rect, B_SOLID_LOW);
	view->SetPenSize(2);
	view->StrokeRect(rect, B_SOLID_HIGH);
	view->SetPenSize(1);
	view->StrokeLine(rect.LeftTop(), rect.RightBottom());
	view->StrokeLine(rect.RightTop(), rect.LeftBottom());
	view->PopState();
}

class TestView: public BView {
private:
	std::vector<BRect> rects;
	bool inDrag, inDrop;
	int32 dropAction;
	int dragId;

public:
	TestView(BRect frame)
		: BView(frame, "view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_SUBPIXEL_PRECISE | B_FRAME_EVENTS),
		inDrag(false), inDrop(false), dropAction(0)
	{
		BRect rect(B_ORIGIN, BPoint(64, 64));
		rect.OffsetBy(32, 32);
		for (int i = 0; i < 8; i++) {
			rects.push_back(rect);
			rect.OffsetBy(16, 16);
		}
	}

	void Draw(BRect where) {
		for (std::vector<BRect>::iterator it = rects.begin(); it != rects.end(); it++) {
			DrawRect(this, *it);
		}
		if (inDrop) {
			BRect rect = Bounds();
			rect.left += 1; rect.top += 1;
			PushState();
			SetHighColor(0, 0, 0);
			SetPenSize(2);
			StrokeRect(rect);
			PopState();
		}
	}

	void UpdateDropAction(BMessage *msg)
	{
		if (!inDrop) return;
		int32 modifiers;
		msg->FindInt32("modifiers", &modifiers);
		int32 newDropAction;
		newDropAction = B_MOVE_TARGET;
		if ((modifiers & (B_COMMAND_KEY | B_SHIFT_KEY)) == B_COMMAND_KEY) {
			newDropAction = B_COPY_TARGET;
		} else if ((modifiers & (B_COMMAND_KEY | B_SHIFT_KEY)) == B_SHIFT_KEY) {
			newDropAction = B_MOVE_TARGET;
		} else if ((modifiers & (B_COMMAND_KEY | B_SHIFT_KEY)) == (B_COMMAND_KEY | B_SHIFT_KEY)) {
			newDropAction = B_LINK_TARGET;
		}
		if (dropAction != newDropAction) {
			dropAction = newDropAction;
			switch (dropAction) {
			case B_MOVE_TARGET: {SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);} break;
			case B_COPY_TARGET: {BCursor cursor(B_CURSOR_ID_COPY); SetViewCursor(&cursor);} break;
			case B_LINK_TARGET: {BCursor cursor(B_CURSOR_ID_CREATE_LINK); SetViewCursor(&cursor);} break;
			}
		}
	}

	void SetInDrop(bool set)
	{
		if (inDrop != set) {
			inDrop = set;
			Invalidate();
			if (inDrop) {
				UpdateDropAction(Looper()->CurrentMessage());
				//SetEventMask(B_KEYBOARD_EVENTS, 0);
			} else {
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
				dropAction = 0;
				//SetEventMask(0, 0);
			}
		}
	}

	void MouseDown(BPoint where)
	{
		for (std::vector<BRect>::reverse_iterator it = rects.rbegin(); it != rects.rend(); it++) {
			if ((*it).Contains(where)) {
				inDrag = true;
				dragId = std::distance(rects.begin(), it.base()) - 1;
				BMessage* drag = new BMessage(dragMsg);
				drag->AddMessenger("be:originator", BMessenger(this));
				drag->AddInt32("be:actions", B_COPY_TARGET);
				drag->AddInt32("be:actions", B_MOVE_TARGET);
				drag->AddInt32("be:actions", B_TRASH_TARGET);
				drag->AddSize("size", (*it).Size());
				DragMessage(drag, *it, this);
				delete drag;
				SetInDrop(true);
				UpdateDropAction(Looper()->CurrentMessage());
				break;
			}
		}
	};

	void MouseUp(BPoint where)
	{
		printf("MouseUp\n");
		int32 buttons;
		Looper()->CurrentMessage()->FindInt32("buttons", &buttons);
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0) {
			inDrag = false;
		}
	}

	void MouseMoved(BPoint where, uint32 code, const BMessage* dragMsg)
	{
		if ((code == B_ENTERED_VIEW) && (dragMsg != NULL)) {
			SetInDrop(true);
		}
		if ((code == B_EXITED_VIEW) && inDrop) {
			SetInDrop(false);
		}
		UpdateDropAction(Looper()->CurrentMessage());
	}

	void MessageReceived(BMessage *msg) {
		//msg->PrintToStream();
		if (msg->WasDropped()) {
			printf("Dropped\n");
			switch (msg->what) {
			case dragMsg: {
				BMessenger source;
				BPoint whereto, ofs;
				BSize size;
				msg->FindMessenger("be:originator", &source);
				msg->FindPoint("_drop_point_", &whereto);
				msg->FindPoint("_drop_offset_", &ofs);
				msg->FindSize("size", &size);
				whereto = ConvertFromScreen(whereto);
				if ((source == BMessenger(this)) && (dropAction == B_MOVE_TARGET)) {
					rects[dragId] = BRect(whereto - ofs, size);
					Invalidate();
				} else {
					if (dropAction == B_MOVE_TARGET) {
						msg->SendReply(B_TRASH_TARGET);
					}
					rects.push_back(BRect(whereto - ofs, size));
					Invalidate();
				}
				Window()->Activate();
				break;
			}
			default:
				BView::MessageReceived(msg);
			}
			SetInDrop(false);
		} else {
			switch (msg->what) {
			case B_MODIFIERS_CHANGED:
				UpdateDropAction(msg);
				break;
			case B_TRASH_TARGET: {
				if (inDrag) {
					rects.erase(rects.begin() + dragId);
					Invalidate();
					inDrag = false;
				}
				break;
			}
			default:
				BView::MessageReceived(msg);
			}
		}
	}
};

class TestWindow: public BWindow {
private:
	BView* view;

public:
	TestWindow(BRect frame)
		: BWindow(
		frame, "Drag&Drop", B_TITLED_WINDOW, 0
	)
	{
		view = new TestView(this->Bounds());
		AddChild(view);
		view->MakeFocus(true);
	}

	bool QuitRequested() {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
};

class TestApplication : public BApplication {
private:
	TestWindow* window;

public:
	TestApplication(): BApplication("application/x-vnd.Test-DragDrop")
	{
		BRect windowRect;
		windowRect.Set(50, 50, 349, 399);
		TestWindow *window = new TestWindow(windowRect);
		window->Show();
	}
};

int main(void) {
	TestApplication app;
	app.Run();
}
