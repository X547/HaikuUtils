#ifndef _SAMPLEGLVIEW_H_
#define _SAMPLEGLVIEW_H_

#include <GLView.h>

class SampleGLView : public BGLView {
public:
	     SampleGLView(BRect frame, uint32 type);
	void AttachedToWindow(void);
	void FrameResized(float newWidth, float newHeight);
	void KeyDown(const char* bytes, int32 numBytes);
	void ErrorCallback(GLenum which);

	void Render(void);

private:
	void         gInit(void);
	void         gDraw(void);
	void         gReshape(int width, int height);
	
	float        width;
	float        height;
};

#endif	// _SAMPLEGLVIEW_H_
