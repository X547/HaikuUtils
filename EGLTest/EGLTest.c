#include <stdlib.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

extern EGLNativeWindowType createNativeWindow(void);

static EGLint const attribute_list[] = {
	EGL_RED_SIZE, 1,
	EGL_GREEN_SIZE, 1,
	EGL_BLUE_SIZE, 1,
	EGL_NONE
};

status_t RenderThread(void *arg)
{
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	NativeWindowType native_window;
	EGLint num_config;

	/* get an EGL display connection */
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	/* initialize the EGL display connection */
	eglInitialize(display, NULL, NULL);

	/* get an appropriate EGL frame buffer configuration */
	eglChooseConfig(display, attribute_list, &config, 1, &num_config);

	/* create an EGL rendering context */
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);

	/* create a native window */
	native_window = createNativeWindow();

	/* create an EGL window surface */
	surface = eglCreateWindowSurface(display, config, native_window, NULL);

	/* connect the context to the surface */
	eglMakeCurrent(display, surface, surface, context);

	/* clear the color buffer */
	glClearColor(1.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	eglSwapBuffers(display, surface);

	sleep(10);

	exit(0);
	return 0;
}
