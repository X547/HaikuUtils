#pragma once

#include <SupportDefs.h>

class BitmapHook;

class GlApp {
public:
	virtual ~GlApp() {};
	virtual status_t Draw() = 0;
};

GlApp *CreateGlApp(BitmapHook *bitmapHook);
