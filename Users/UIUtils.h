#ifndef _UIUTILS_H_
#define _UIUTILS_H_


#include <MenuItem.h>
#include <private/shared/AutoDeleter.h>

class BBitmap;


class IconMenuItem: public BMenuItem
{
public:
	IconMenuItem(BBitmap *bitmap, BMessage* message, char shortcut = 0, uint32 modifiers = 0);
	IconMenuItem(BBitmap *bitmap, BMenu* menu, BMessage* message);
	void GetContentSize(float* width, float* height);
	void DrawContent();

private:
	ObjectDeleter<BBitmap> fBitmap;
};


BBitmap *LoadIcon(int32 id, int32 width, int32 height);


#endif	// _UIUTILS_H_
