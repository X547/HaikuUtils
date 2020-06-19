#include "HighlightRect.h"

#include <Window.h>
#include <View.h>

enum {
	borderSize = 2
};

static BWindow *CreateHighlightWindow()
{
	BWindow *wnd = new BWindow(BRect(0, 0, 0, 0), "highlightWindow", B_NO_BORDER_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL, B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_AVOID_FOCUS);
	BView *view = new BView(BRect(0, 0, 0, 0), "highlightView", B_FOLLOW_ALL, 0);
	view->SetViewColor(0xff, 0x00, 0x00);
	wnd->AddChild(view);
	return wnd;
};

HighlightRect::HighlightRect()
{
		for (int32 i = 0; i < 4; i++)
			fWnds[i] = NULL;
}

HighlightRect::~HighlightRect()
{
	Hide();
}

void HighlightRect::Show(BRect rect)
{
	if (fWnds[0] == NULL) {
		for (int32 i = 0; i < 4; i++)
			fWnds[i] = CreateHighlightWindow();
	} else {
		if (fRect == rect)
			return;
		for (int32 i = 0; i < 4; i++)
			fWnds[i]->Hide();
	}
	fRect = rect;
	rect.InsetBy(-1, -1);
	fWnds[0]->MoveTo(rect.left - borderSize, rect.top);
	fWnds[0]->ResizeTo(borderSize, rect.Height());
	fWnds[1]->MoveTo(rect.left - borderSize, rect.top - borderSize);
	fWnds[1]->ResizeTo(rect.Width() + 2*borderSize, borderSize);
	fWnds[2]->MoveTo(rect.right, rect.top);
	fWnds[2]->ResizeTo(borderSize, rect.Height());
	fWnds[3]->MoveTo(rect.left - borderSize, rect.bottom);
	fWnds[3]->ResizeTo(rect.Width() + 2*borderSize, borderSize);
		for (int32 i = 0; i < 4; i++)
			fWnds[i]->Show();
}

void HighlightRect::Hide()
{
	for (int32 i = 0; i < 4; i++) {
		if (fWnds[i] != NULL) {
			fWnds[i]->LockLooper();
			fWnds[i]->Quit();
			fWnds[i] = NULL;
		}
	}
}
