#ifndef _SAMPLEGLVIEW_H_
#define _SAMPLEGLVIEW_H_

#include <GLView.h>

class SampleGLView : public BGLView {
public:
	         SampleGLView(BRect frame);
	virtual ~SampleGLView();
	void     AttachedToWindow(void);
	void     DetachedFromWindow(void);
	void     FrameResized(float newWidth, float newHeight);
	void     KeyDown(const char* bytes, int32 numBytes);
	void     ErrorCallback(GLenum which);

private:
	void            gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width, GLint teeth, GLfloat tooth_depth);
	void            init();
	void            cleanup();
	void            draw();
	void            reshape(int width, int height);
	void            idle();

	static status_t RenderThread(void *arg);

	bool         fRun;
	thread_id    fThread;
	float        width;
	float        height;

	bigtime_t T0;
	GLint Frames;
	GLfloat viewDist;
	GLfloat view_rotx, view_roty, view_rotz;
	GLint gear1, gear2, gear3;
	bigtime_t displayTime;
	GLfloat angle;
};

#endif	// _SAMPLEGLVIEW_H_
