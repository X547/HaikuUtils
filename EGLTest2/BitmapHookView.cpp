#include "BitmapHookView.h"
#include <Messenger.h>
#include <Region.h>
#include <Bitmap.h>
#include <private/shared/PthreadMutexLocker.h>

#include <stdio.h>


void BitmapHookView::ViewBitmapHook::GetSize(uint32_t &width, uint32_t &height)
{
	PthreadMutexLocker lock(&View()->fLock);
	width = (uint32_t)View()->fSize.Width() + 1;
	height = (uint32_t)View()->fSize.Height() + 1;
}

BBitmap *BitmapHookView::ViewBitmapHook::SetBitmap(BBitmap *bmp)
{
	//printf("ViewBitmapHook::SetBitmap(%p)\n", bmp);
	PthreadMutexLocker lock(&View()->fLock);
	BBitmap* oldBmp = View()->fBmp.Detach();
	View()->fBmp.SetTo(bmp);
	BMessenger(View()).SendMessage(B_INVALIDATE);
	return oldBmp;
}


void BitmapHookView::Init()
{
	fSize = Frame().Size();
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0, 0, 0);
}

BitmapHookView::BitmapHookView(const char *name, uint32 flags, BLayout *layout):
	BView(name, flags | B_WILL_DRAW | B_FRAME_EVENTS, layout)
{
	Init();
}

BitmapHookView::BitmapHookView(BRect frame, const char *name, uint32 resizingMode, uint32 flags):
	BView(frame, name, resizingMode, flags | B_WILL_DRAW | B_FRAME_EVENTS)
{
	Init();
}

void BitmapHookView::FrameResized(float newWidth, float newHeight)
{
	PthreadMutexLocker lock(&fLock);
	fSize.Set(newWidth, newHeight);
}

void BitmapHookView::Draw(BRect dirty)
{
	BRegion region(dirty);
	PthreadMutexLocker lock(&fLock);
	if (fBmp.IsSet()) {
		DrawBitmap(fBmp.Get(), B_ORIGIN);
		region.Exclude(fBmp->Bounds());
	}
	FillRegion(&region, B_SOLID_LOW);
}
