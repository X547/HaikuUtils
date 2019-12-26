#include "SoftwareRenderer.h"
#include <GL/osmesa.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <DirectWindow.h>
#include <private/interface/DirectWindowPrivate.h>
#include <Screen.h>
#include <stdio.h>

#undef _EXPORT
#define _EXPORT __attribute__((__visibility__("default")))

extern "C" _EXPORT BGLRenderer*
instantiate_gl_renderer(BGLView *view, ulong opts, BGLDispatcher *dispatcher)
{
	return new SoftwareRenderer(view, opts, dispatcher);
}

struct RasBuf32
{
	int32 width, height, stride;
	int32 orgX, orgY;
	int32 *colors;
	
	RasBuf32(int32 width, int32 height, int32 stride, int32 orgX, int32 orgY, int32 *colors):
		width(width), height(height), stride(stride), orgX(orgX), orgY(orgY), colors(colors)
	{}
	
	RasBuf32(BBitmap *bmp)
	{
		width  = bmp->Bounds().IntegerWidth()  + 1;
		height = bmp->Bounds().IntegerHeight() + 1;
		stride = bmp->BytesPerRow()/4;
		orgX   = 0;
		orgY   = 0;
		colors = (int32*)bmp->Bits();
	}
	
	RasBuf32(direct_buffer_info *info)
	{
		width  = 0x7fffffff;
		height = 0x7fffffff;
		stride = info->bytes_per_row/4;
		orgX   = 0;
		orgY   = 0;
		colors = (int32*)info->bits;
	}
	
	void ClipSize(int32 x, int32 y, int32 w, int32 h)
	{
		if (x < 0) {w += x; x = 0;}
		if (y < 0) {h += y; y = 0;}
		if (x + w >  width) {w = width  - x;}
		if (y + h > height) {h = height - y;}
		if ((w > 0) && (h > 0)) {
			colors += y*stride + x;
			width  = w;
			height = h;
		} else {
			width = 0; height = 0; colors = NULL;
		}
		if (x + orgX > 0) {orgX += x;} else {orgX = 0;}
		if (y + orgY > 0) {orgY += y;} else {orgY = 0;}
	}
	
	void ClipRect(int32 l, int32 t, int32 r, int32 b)
	{
		ClipSize(l, t, r - l, b - t);
	}
	
	void Shift(int32 dx, int32 dy)
	{
		orgX += dx;
		orgY += dy;
	}
	
	void Clear(int32 color)
	{
		RasBuf32 dst = *this;
		dst.stride -= dst.width;
		for (; dst.height > 0; dst.height--) {
			for (int32 i = dst.width; i > 0; i--)
				*dst.colors++ = color;
			dst.colors += dst.stride;
		}
	}
	
	void Blit(RasBuf32 src)
	{
		RasBuf32 dst = *this;
		int32 x, y;
		x = src.orgX - orgX;
		y = src.orgY - orgY;
		dst.ClipSize(x, y, src.width, src.height);
		src.ClipSize(-x, -y, width, height);
		for (; dst.height > 0; dst.height--) {
			memcpy(dst.colors, src.colors, 4*dst.width);
			dst.colors += dst.stride;
			src.colors += src.stride;
		}
	}
};

SoftwareRenderer::SoftwareRenderer(BGLView *view, ulong options, BGLDispatcher* dispatcher)
	: BGLRenderer(view, options, dispatcher), fBackBuf(NULL), fWidth(1), fHeight(1), fDirectModeEnabled(false), fInfo(NULL), fSwapBuffersLevel(0)
{
	printf("+SoftwareRenderer\n");
	int attribList[] = {
		OSMESA_FORMAT,                OSMESA_BGRA,
		OSMESA_DEPTH_BITS,            16,
		OSMESA_PROFILE,               OSMESA_COMPAT_PROFILE,
		OSMESA_CONTEXT_MAJOR_VERSION, 2,
		0, 0
	};
	fCtx = OSMesaCreateContextAttribs(attribList, NULL);
	if (fCtx == NULL) {
		printf("[!] can't create context\n");
	}
	_AllocBackBuf(); //avoid NULL backBuf
}


SoftwareRenderer::~SoftwareRenderer()
{
	printf("-SoftwareRenderer\n");
	OSMesaDestroyContext(fCtx); fCtx = NULL;
	if (fBackBuf != NULL) {
		delete fBackBuf; fBackBuf = NULL;
	}
	if (fInfo != NULL) {
		free(fInfo); fInfo = NULL;
	}
}

void SoftwareRenderer::LockGL()
{
	BGLRenderer::LockGL();
	BAutolock lock(fLocker);
	_UpdateContext();
	
	if ((fSwapBuffersLevel <= 0) && ((fBackBuf == NULL) || (fBackBuf->Bounds().Size() != BSize(fWidth, fHeight)))) {
		glFinish();
		_AllocBackBuf();
		_UpdateContext();
	}
}

void SoftwareRenderer::UnlockGL()
{
	OSMesaMakeCurrent(NULL, NULL, GL_UNSIGNED_BYTE, 0, 0);
	BGLRenderer::UnlockGL();
}

void SoftwareRenderer::SwapBuffers(bool vsync)
{
	atomic_add(&fSwapBuffersLevel, 1);
	GLView()->LockGL();
	BScreen screen(GLView()->Window());
	glFinish();
	if (!fDirectModeEnabled || (fInfo == NULL)) {
		if (GLView()->LockLooperWithTimeout(1000) == B_OK) {
			GLView()->DrawBitmap(fBackBuf, B_ORIGIN);
			GLView()->UnlockLooper();
			if (vsync)
				screen.WaitForRetrace();
		}
	} else {
		BAutolock lock(fLocker);
		RasBuf32 srcBuf(fBackBuf);
		RasBuf32 dstBuf(fInfo);
		for (uint32 i = 0; i < fInfo->clip_list_count; i++) {
			clipping_rect *clip = &fInfo->clip_list[i];
			RasBuf32 dstClip = dstBuf;
			dstClip.ClipRect(clip->left, clip->top, clip->right + 1, clip->bottom + 1);
			dstClip.Shift(-fInfo->window_bounds.left, -fInfo->window_bounds.top);
			dstClip.Blit(srcBuf);
		}
	}
	{
		BAutolock lock(fLocker);
		if(_AllocBackBuf()) _UpdateContext();
	}
	GLView()->UnlockGL();
	atomic_add(&fSwapBuffersLevel, -1);
}

void SoftwareRenderer::Draw(BRect updateRect)
{
	BAutolock lock(fLocker);
	if (!fDirectModeEnabled || (fInfo == NULL))
		GLView()->DrawBitmap(fBackBuf, B_ORIGIN);
}

status_t SoftwareRenderer::CopyPixelsOut(BPoint location, BBitmap *bitmap)
{
	{
		BAutolock lock(fLocker);
		
		color_space scs = fBackBuf->ColorSpace();
		color_space dcs = bitmap->ColorSpace();
	
		if ((scs != dcs) && (scs != B_RGBA32 || dcs != B_RGB32)) {
			printf("[!] SoftwareRenderer::CopyPixelsOut(): incompatible color space\n");
			return B_BAD_TYPE;
		}
	}

	atomic_add(&fSwapBuffersLevel, 1);
	GLView()->LockGL();
	glFinish();
	RasBuf32 srcBuf(fBackBuf);
	RasBuf32 dstBuf(bitmap);
	srcBuf.Shift(-location.x, -location.y);
	dstBuf.Blit(srcBuf);
	GLView()->UnlockGL();
	atomic_add(&fSwapBuffersLevel, -1);

	return B_OK;
}

status_t SoftwareRenderer::CopyPixelsIn(BBitmap *bitmap, BPoint location)
{
	{
		BAutolock lock(fLocker);
		
		color_space sourceCS = bitmap->ColorSpace();
		color_space destinationCS = fBackBuf->ColorSpace();
	
		if (sourceCS != destinationCS
			&& (sourceCS != B_RGB32 || destinationCS != B_RGBA32)) {
			printf("[!] SoftwareRenderer::CopyPixelsIn(): incompatible color space\n");
			return B_BAD_TYPE;
		}
	}

	atomic_add(&fSwapBuffersLevel, 1);
	GLView()->LockGL();
	glFinish();
	RasBuf32 srcBuf(bitmap);
	RasBuf32 dstBuf(fBackBuf);
	dstBuf.Shift(-location.x, -location.y);
	dstBuf.Blit(srcBuf);
	GLView()->UnlockGL();
	atomic_add(&fSwapBuffersLevel, -1);

	return B_OK;
}

void SoftwareRenderer::EnableDirectMode(bool enabled)
{
	fDirectModeEnabled = enabled;
}

void SoftwareRenderer::DirectConnected(direct_buffer_info *info)
{
	BAutolock lock(fLocker);
	if (info) {
		if (!fInfo) {
			fInfo = (direct_buffer_info *)calloc(1, DIRECT_BUFFER_INFO_AREA_SIZE);
		}
		memcpy(fInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
	} else if (fInfo) {
		free(fInfo); fInfo = NULL;
	}
}

void SoftwareRenderer::FrameResized(float width, float height)
{
	BAutolock lock(fLocker);
	fWidth  = (int32)width;  if (fWidth  < 1) fWidth  = 1;
	fHeight = (int32)height; if (fHeight < 1) fHeight = 1;
}


bool SoftwareRenderer::_AllocBackBuf()
{
	if ((fBackBuf == NULL) || (fBackBuf->Bounds().Size() != BSize(fWidth, fHeight))) {
		if (fBackBuf != NULL) {
			delete fBackBuf; fBackBuf = NULL;
		}
			fBackBuf = new BBitmap(BRect(B_ORIGIN, BSize(fWidth, fHeight)), B_RGB32);
			return true;
	}
	return false;
}

void SoftwareRenderer::_UpdateContext()
{
	OSMesaMakeCurrent(fCtx, fBackBuf->Bits(), GL_UNSIGNED_BYTE, fBackBuf->Bounds().IntegerWidth() + 1, fBackBuf->Bounds().IntegerHeight() + 1);
	OSMesaPixelStore(OSMESA_Y_UP, 0);
}
