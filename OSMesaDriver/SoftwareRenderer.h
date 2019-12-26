#ifndef SOFTWARERENDERER_H
#define SOFTWARERENDERER_H


#include <kernel/image.h>
#include <GLRenderer.h>
#include <GL/osmesa.h>

class SoftwareRenderer: public BGLRenderer {
public:
	SoftwareRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher);
	virtual ~SoftwareRenderer();

	void LockGL();
	void UnlockGL();

	void SwapBuffers(bool vsync = false);
	void Draw(BRect updateRect);
	status_t CopyPixelsOut(BPoint source, BBitmap *dest);
	status_t CopyPixelsIn(BBitmap *source, BPoint dest);
	void FrameResized(float width, float height);

	void EnableDirectMode(bool enabled);
	void DirectConnected(direct_buffer_info *info);

private:
	bool _AllocBackBuf();
	void _UpdateContext();
	
	BLocker fLocker;
	OSMesaContext fCtx;
	BBitmap *fBackBuf;
	bool fDirectModeEnabled;
	direct_buffer_info* fInfo;
	int32 fWidth, fHeight;
	int32 fSwapBuffersLevel;
};

#endif	// SOFTPIPERENDERER_H
