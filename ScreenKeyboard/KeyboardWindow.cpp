#include "KeyboardWindow.h"

#include "KeyboardDevice.h"
#include "KeyboardHandler.h"

#include <ControlLook.h>
#include <Application.h>
#include <Screen.h>
#include <GroupLayoutBuilder.h>


typedef uint32 Pushed[4];

const uint32 linesCount = 5;

const uint32 keyboardLines[] = {
	15, 14, 13, 14, 9
};

const float keyboardDims[] = {
	1,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   1,   1,
	1.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   1.5,
	1.75,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.25,
	2,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   1,
	1,   1, 1, 7, 1, 1, 1, 1, 1
};

const uint32 keyboardCodes[] = {
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x6a, 0x1e,
	0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
	0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x6b, 0x57, 0x56,
	0x5c, 0x66, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63
};

const char *keyboardLabels[] = {
	"漢字", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "\t⌫",
	"⮐", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	"⇪", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "⏎",
	"⇧\t", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "↑", "\t⇧",
	"◆", "⌥", "⌘", 0, "⌘", "◆", "←", "↓", "→"
};


class KeyboardLayout
{
public:
	uint32 Count();
	BRect  Box (int32 key);
	uint32 Code(int32 key);
};

class KeyboardViewNotifier: public KeyboardNotifier
{
private:
	BView *view;
	
public:
	void SetTo(BView *view);
	void KeymapChanged();
};

class KeyboardView: public BView {
public:
	KeyboardView(const char *name, KeyboardHandler *handler);
	~KeyboardView();
	
	int32 KeyAt(BPoint pos);
	
	void MouseDown(BPoint where);
	void MouseUp(BPoint where);
	void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage);
	
	BRect ScaleBox(BRect box);
	void SetPushed();
	
	void Draw(BRect dirty);

private:
	friend class KeyboardViewNotifier;

	KeyboardHandler *handler;
	KeyboardLayout layout;
	font_height height;
	uint32 pushed[4], oldPushed[4];
	int32 track[4];
	
	KeyboardViewNotifier notifier;
};


#define LEN(a) sizeof(a)/sizeof((a)[0])

inline void Assert(bool cond, const char *msg = "assertion failed")
{
	if (!cond) debugger(msg);
}


uint32 KeyboardLayout::Count()
{
	return LEN(keyboardCodes);
}

BRect KeyboardLayout::Box(int32 key)
{
	uint32 i = 0, j = 0;
	uint32 offs = key;
	while ((i < linesCount) && (offs >= keyboardLines[i])) {offs -= keyboardLines[i]; ++i;}
	Assert(i < linesCount, "no such key in layout");
	float pos = 0, sum = 0;
	while (j < offs)             {sum += keyboardDims[key-offs+j]; ++j;} pos = sum;
	while (j < keyboardLines[i]) {sum += keyboardDims[key-offs+j]; ++j;}
	
	return BRect(pos/sum, i/float(linesCount), (pos + keyboardDims[key])/sum, (i + 1)/float(linesCount));
}

uint32 KeyboardLayout::Code(int32 key)
{
	Assert((key >= 0) && ((uint32)key < LEN(keyboardCodes)), "no such key in layout");
	return keyboardCodes[key];
}


void KeyboardViewNotifier::SetTo(BView *view)
{
	this->view = view;
}

void KeyboardViewNotifier::KeymapChanged()
{
	if (view->Window()->LockLooper()) {
		view->Invalidate();
		view->Window()->UnlockLooper();
	}
}


KeyboardView::KeyboardView(const char *name, KeyboardHandler *handler)
	: BView(name, B_WILL_DRAW | B_FRAME_EVENTS | B_SUBPIXEL_PRECISE), handler(handler)
{
	uint32 i;
	SetViewColor(B_TRANSPARENT_COLOR);
	SetDrawingMode(B_OP_OVER);
	
	for (i = 0; i < LEN(pushed); ++i) {
		pushed   [i] = 0;
		oldPushed[i] = 0;
	}

	for (i = 0; i < LEN(track); ++i)
		track[i] = -1;
	
	SetFontSize(20);
	GetFontHeight(&height);
	
	notifier.SetTo(this);
	handler->InstallNotifier(&notifier);
}

KeyboardView::~KeyboardView()
{
	handler->UninstallNotifier(&notifier);
}


int32 KeyboardView::KeyAt(BPoint pos)
{
	uint32 i = 0;
	while ((i < layout.Count()) && !ScaleBox(layout.Box(i)).Contains(pos)) ++i;
	if (i == layout.Count())
		return -1;
	else
		return i;
}


void KeyboardView::MouseDown(BPoint where)
{
	BMessage *msg = Looper()->CurrentMessage();
	int32 contactId = 0;
	msg->FindInt32("be:tablet_contact_id", &contactId);

	SetMouseEventMask(B_POINTER_EVENTS);
	
	int32 key = KeyAt(where);
	if (key != -1) {
		uint32 i = 0;
		while (i < 4 && track[i] != key) ++i;
		if(i == 4) {
			track[contactId] = key;
			pushed[key/32] |= 1 << key%32;
			SetPushed();
		}
	}
}

void KeyboardView::MouseUp(BPoint where)
{
	BMessage *msg = Looper()->CurrentMessage();
	int32 contactId = 0;
	msg->FindInt32("be:tablet_contact_id", &contactId);

	if (track[contactId] != -1) {
		pushed[track[contactId]/32] &= ~(1 << track[contactId]%32);
		track[contactId] = -1;
		SetPushed();
	}
}

void KeyboardView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	BMessage *msg = Looper()->CurrentMessage();
	int32 contactId = 0;
	msg->FindInt32("be:tablet_contact_id", &contactId);

	if (track[contactId] != -1) {
		if (ScaleBox(layout.Box(track[contactId])).Contains(where))
			pushed[track[contactId]/32] |=   1 << track[contactId]%32;
		else
			pushed[track[contactId]/32] &= ~(1 << track[contactId]%32);
		SetPushed();
	}
}


BRect KeyboardView::ScaleBox(BRect box)
{
	float padding = 0;
	box.left   *= Frame().Width()  - padding; box.left   = (int)box.left;
	box.top    *= Frame().Height() - padding; box.top    = (int)box.top;
	box.right  *= Frame().Width()  - padding; box.right  = (int)box.right;
	box.bottom *= Frame().Height() - padding; box.bottom = (int)box.bottom;
	box.OffsetBy(padding/2, padding/2);
	box.InsetBy (padding,   padding);
	
	return box;
}


void KeyboardView::SetPushed()
{
	Pushed diff;
	uint32 i;
	
	for (i = 0; i < LEN(diff); ++i) diff[i] = pushed[i] ^ oldPushed[i];
	
	for (i = 0; i < layout.Count(); ++i) {
		if (diff[i/32] & (1 << i%32)) {
			handler->KeyChanged(layout.Code(i), pushed[i/32] & (1 << i%32));
		}
	}
	
	for (i = 0; i < LEN(oldPushed); ++i) oldPushed[i] = pushed[i];

	Invalidate();
}


void KeyboardView::Draw(BRect dirty)
{
	BRect box;
	char str[16];
	rgb_color base;
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	
	SetLowColor(background);
	FillRect(dirty, B_SOLID_LOW);
	
	for (uint32 i = 0; i < layout.Count(); ++i) {
		uint32 flags = 0;
		if (layout.Code(i) == 0x3b && (handler->Modifiers() & B_CAPS_LOCK) != 0)
			base = (rgb_color){0, 128, 255, 255};
		else
			base = (rgb_color){230, 230, 230, 255};
			
		if (pushed[i/32] & (1 << i%32))
			flags |= BControlLook::B_ACTIVATED;
		
		box = ScaleBox(layout.Box(i));
		if (keyboardLabels[i] != NULL)
			strcpy(str, keyboardLabels[i]);
		else
			handler->KeyString(layout.Code(i), str, sizeof(str));

		be_control_look->DrawButtonFrame     (this, box, dirty, base, background, flags);
		be_control_look->DrawButtonBackground(this, box, dirty, base,             flags);
		DrawString(str, BPoint((box.left+box.right)/2 - StringWidth(str)/2, (box.top+box.bottom)/2 + (height.ascent-height.descent)/2 + 1));
	}
}


KeyboardWindow::KeyboardWindow(KeyboardHandler *handler): BWindow(BRect(0, 0, 0, 0), "Keyboard", B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_AVOID_FOCUS)
{
	BScreen screen(this);

	SetLayout(new BGroupLayout(B_VERTICAL));
	ResizeTo(screen.Frame().Width(), 256);
	MoveTo(0, screen.Frame().bottom - 256);
	
	AddChild(new KeyboardView("keyboard", handler));
}
