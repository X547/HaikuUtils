#pragma once

#include "BitmapHook.h"
#include <View.h>
#include <pthread.h>
#include <private/shared/AutoDeleter.h>

class BitmapHookView: public BView
{
private:
	class ViewBitmapHook: public BitmapHook {
	private:
		inline BitmapHookView *View() {return (BitmapHookView*)((char*)this - offsetof(BitmapHookView, fHook));}

	public:
		virtual ~ViewBitmapHook() {};
		void GetSize(uint32_t &width, uint32_t &height) override;
		BBitmap *SetBitmap(BBitmap *bmp) override;
	};

	pthread_mutex_t fLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
	BSize fSize;
	ObjectDeleter<BBitmap> fBmp;
	ViewBitmapHook fHook;
	
	void Init();

public:
	BitmapHookView(const char *name, uint32 flags, BLayout *layout = NULL);
	BitmapHookView(BRect frame, const char* name, uint32 resizingMode, uint32 flags);
	virtual ~BitmapHookView() {}

	BitmapHook *Hook() {return &fHook;}

	void FrameResized(float newWidth, float newHeight) override;
	void Draw(BRect dirty) override;
};
