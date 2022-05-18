#pragma once

#include <stdint.h>


class BBitmap;

class BitmapHook {
public:
	virtual ~BitmapHook() {};
	virtual void GetSize(uint32_t &width, uint32_t &height) = 0;
	virtual BBitmap *SetBitmap(BBitmap *bmp) = 0;
};
