#include "UIUtils.h"

#include <Application.h>
#include <Resources.h>
#include <IconUtils.h>
#include <Bitmap.h>


IconMenuItem::IconMenuItem(BBitmap *bitmap, BMessage* message, char shortcut, uint32 modifiers):
	BMenuItem("", message, shortcut, modifiers),
	fBitmap(bitmap)
{
}

IconMenuItem::IconMenuItem(BBitmap *bitmap, BMenu* menu, BMessage* message):
	BMenuItem(menu, message),
	fBitmap(bitmap)
{
}

void IconMenuItem::GetContentSize(float* width, float* height)
{
	*width = 0;
	*height = 0;
	if (fBitmap.Get() != NULL) {
		*width += fBitmap->Bounds().Width() + 1;
		*height += fBitmap->Bounds().Height() + 1;
	}
	if (Submenu() != NULL) {
		*width += 2 + 7;
	}
};

void IconMenuItem::DrawContent()
{
	if (fBitmap.Get() == NULL)
		return;

	BRect frame = Frame();
	if (Submenu() != NULL) {
		frame.right -= 2 + 7;
	}
	BPoint center = BPoint((frame.left + frame.right)/2, (frame.top + frame.bottom)/2);
	Menu()->PushState();
	Menu()->SetDrawingMode(B_OP_ALPHA);
	Menu()->DrawBitmap(fBitmap.Get(), center - BPoint(fBitmap->Bounds().Width()/2, fBitmap->Bounds().Height()/2));
	if (Submenu() != NULL) {
		BPoint origin = center + BPoint(fBitmap->Bounds().Width()/2 + 2 + 3, 0);
		Menu()->FillTriangle(
			origin + BPoint(-3, -1.5),
			origin + BPoint(3, -1.5),
			origin + BPoint(0, 1.5)
		);
	}
	Menu()->PopState();
}


BBitmap *LoadIcon(int32 id, int32 width, int32 height)
{
	size_t dataSize;
	const void* data = BApplication::AppResources()->FindResource(B_VECTOR_ICON_TYPE, id, &dataSize);

	if (data == NULL) return NULL;

	ObjectDeleter<BBitmap> bitmap(new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32));

	if (bitmap.Get() == NULL) return NULL;

	if (BIconUtils::GetVectorIcon((const uint8*)data, dataSize, bitmap.Get()) != B_OK)
		return NULL;

	return bitmap.Detach();
}
