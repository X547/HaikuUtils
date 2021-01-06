#include "SampleGLView.h"
#include <stdio.h>
#include <GL/glu.h>

GLenum use_stipple_mode;     // GL_TRUE to use dashed lines
GLenum use_smooth_mode;     // GL_TRUE to use anti-aliased lines
GLint linesize;             // Line width
GLint pointsize;            // Point diameter

float pntA[3] = {
	-160.0, 0.0, 0.0
};
float pntB[3] = {
	-130.0, 0.0, 0.0
};

SampleGLView::SampleGLView(BRect frame, uint32 type)
	: BGLView(frame, "SampleGLView", B_FOLLOW_ALL_SIDES, 0, type)
{
	printf("+SampleGLView\n");
	width = frame.right-frame.left;
	height = frame.bottom-frame.top;
}

SampleGLView::~SampleGLView()
{
	printf("-SampleGLView\n");
}

void SampleGLView::AttachedToWindow(void) {
	MakeFocus(true);
	BGLView::AttachedToWindow();
	LockGL();
	gInit();
	gReshape(width, height);
	UnlockGL();
	Render();
}

void SampleGLView::FrameResized(float newWidth, float newHeight) {
	BGLView::FrameResized(newWidth, newHeight);
	LockGL();
	width = newWidth;
	height = newHeight;
	gReshape(width, height);
	UnlockGL();
	Render();
}

extern "C" void glWindowPos2f(GLfloat x, GLfloat y);

static void CopyFlipY(BBitmap &dst, BBitmap &src)
{
	int32 h = dst.Bounds().Height() + 1;
	int32 stride = dst.BytesPerRow();
	uint8 *dstBits = (uint8*)dst.Bits();
	uint8 *srcBits = (uint8*)src.Bits() + stride*(h - 1);
	for (int32 y = 0; y < h; y++) {
		memcpy(dstBits, srcBits, stride);
		dstBits += stride;
		srcBits -= stride;
	}
}

static void WriteBitmap(BGLView *view, const BBitmap &bmp, BPoint pos)
{
	glDisable(GL_BLEND);
	glWindowPos2f(pos.x, (view->Bounds().Height() + 1.0) - pos.y);
	glPixelZoom(1.0, -1.0);
	glDrawPixels((int32)bmp.Bounds().Width() + 1, (int32)bmp.Bounds().Height() + 1, GL_RGBA, GL_UNSIGNED_BYTE, bmp.Bits());
}

static void ReadBitmap(BGLView *view, BBitmap &bmp, BPoint pos)
{
	BBitmap buf(bmp.Bounds(), 0, bmp.ColorSpace(), bmp.BytesPerRow());
	CopyFlipY(buf, bmp);
	glReadPixels(pos.x, (int32)view->Bounds().Height() + 1 - pos.y - ((int32)bmp.Bounds().Height() + 1), (int32)bmp.Bounds().Width() + 1, (int32)bmp.Bounds().Height() + 1, GL_RGBA, GL_UNSIGNED_BYTE, buf.Bits());
	CopyFlipY(bmp, buf);
}

void SampleGLView::KeyDown(const char* bytes, int32 numBytes)
{
	if ((numBytes == 1) && (bytes[0] == 'q')) {
		printf("Draw bitmap\n");
		int32 w = 64, h = 64;
		BRect rect(0, 0, w - 1, h - 1);
		BBitmap *bmp = new BBitmap(rect, B_RGB32);
		int8 *bits = (int8*)bmp->Bits();
		int32 stride = bmp->BytesPerRow();
		for (int32 y = 0; y < h; y++)
			for (int32 x = 0; x < w; x++)
				*(int32*)(bits + y*stride + x*4) = 0xff000000 + (x*0xff/w)%0x100 + ((y*0xff/h)%0x100)*0x100;
		LockGL();
		gDraw();
		BPoint pos;
		GetMouse(&pos, NULL);
		ReadBitmap(this, *bmp, pos);
		WriteBitmap(this, *bmp, B_ORIGIN);
		SwapBuffers();
		UnlockGL();
		delete bmp;
	}
/*
		CopyPixelsOut(B_ORIGIN, bmp);
		CopyPixelsIn(bmp, B_ORIGIN);
*/
}

void SampleGLView::ErrorCallback(GLenum whichError) {
	fprintf(stderr, "Unexpected error occured (%d):\n", whichError);
	fprintf(stderr, " %s\n", gluErrorString(whichError));
}

void SampleGLView::gInit(void) {
	printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	use_stipple_mode = GL_FALSE;
	use_smooth_mode = GL_TRUE;
	linesize = 2;
	pointsize = 4;
}

void SampleGLView::gDraw(void) {
	GLint i;

	glClearColor(0.5, 0.5, 0.5, 0.0);
	glLineStipple(1, 0xF0E0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glClear(GL_COLOR_BUFFER_BIT);
	glLineWidth(linesize);

	if (use_stipple_mode) {
		glEnable(GL_LINE_STIPPLE);
	} else {
		glDisable(GL_LINE_STIPPLE);
	}

	if (use_smooth_mode) {
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
	}

	glPushMatrix();

	for (i = 0; i < 360/6; i += 5) {
		glRotatef(5.0, 0,0,1);     // Rotate right 5 degrees
		glColor3f(1.0, 1.0, 0.0);  // Set color for line
		glBegin(GL_LINE_STRIP);    // And create the line
		glVertex3fv(pntA);
		glVertex3fv(pntB);
		glEnd();

		glPointSize(pointsize);    // Set size for point
		glColor3f(0.0, 1.0, 0.0);  // Set color for point
		glBegin(GL_POINTS);
		glVertex3fv(pntA);         // Draw point at one end
		glVertex3fv(pntB);         // Draw point at other end
		glEnd();
	}

	glPopMatrix();                // Done with matrix
}

void SampleGLView::gReshape(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-175, 175, -175, 175);
	glMatrixMode(GL_MODELVIEW);
}

void SampleGLView::Render(void) {
	LockGL();
	gDraw();
	SwapBuffers();
	UnlockGL();
}
