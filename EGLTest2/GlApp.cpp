#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <private/shared/AutoDeleter.h>
#include "MinApp.h"
#include "GlApp.h"

#include <EGL/egl.h>
#include <GL/gl.h>


static void
gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
    glOrtho(left, right, bottom, top, -1, 1);
}


static void Draw(int width, int height, uint32 step)
{
	GLenum use_stipple_mode = GL_FALSE;
	GLenum use_smooth_mode = GL_TRUE;
	GLint linesize = 2;             // Line width
	GLint pointsize = 4;            // Point diameter

	float pntA[3] = {
		-160.0, 0.0, 0.0
	};
	float pntB[3] = {
		-130.0, 0.0, 0.0
	};

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-175, 175, -175, 175);
	glMatrixMode(GL_MODELVIEW);


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

	glRotatef(0.1*step, 0, 0, 1);

	for (int i = 0; i < 360; i += 5) {
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


class GlAppImpl: public GlApp {
private:
	BitmapHook *fBitmapHook{};
	EGLDisplay eglDpy{};
	EGLConfig eglCfg{};
	EGLContext eglCtx{};
	EGLSurface eglSurf{};
	int pbufferWidth = 512;
	int pbufferHeight = 512;
	
	uint32 step = 0;
	
public:
	GlAppImpl(BitmapHook *bitmapHook);
	virtual ~GlAppImpl();

	status_t Init();
	status_t Draw();
};

GlAppImpl::GlAppImpl(BitmapHook *bitmapHook):
	fBitmapHook(bitmapHook)
{}

GlAppImpl::~GlAppImpl()
{
	eglTerminate(eglDpy);
}

status_t GlAppImpl::Init()
{
	eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint major, minor;
	eglInitialize(eglDpy, &major, &minor);

	EGLint numConfigs;

	EGLint configAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_NONE
	};
	eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
	
	eglBindAPI(EGL_OPENGL_API);
	
	eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, NULL);
	eglSurf = eglCreateWindowSurface(eglDpy, eglCfg, (EGLNativeWindowType)fBitmapHook, NULL);

	return B_OK;
}

status_t GlAppImpl::Draw()
{
	eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
	uint32 newWidth, newHeight;
	fBitmapHook->GetSize(newWidth, newHeight);
	::Draw(newWidth, newHeight, step++);
	eglSwapBuffers(eglDpy, eglSurf);
	return B_OK;
}

GlApp *CreateGlApp(BitmapHook *bitmapHook)
{
	ObjectDeleter<GlAppImpl> app(new GlAppImpl(bitmapHook));
	if (!app.IsSet()) return NULL;
	if (app->Init() < B_OK) return NULL;
	return app.Detach();
}
