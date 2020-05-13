#include "SampleGLView.h"
#include <stdio.h>
#include <GL/glu.h>
#include <math.h>
#include <OS.h>

SampleGLView::SampleGLView(BRect frame)
	: BGLView(frame, "SampleGLView", B_FOLLOW_ALL_SIDES, 0, BGL_DOUBLE|BGL_DEPTH)
{
	printf("+SampleGLView\n");
	width = frame.right-frame.left + 1;
	height = frame.bottom-frame.top + 1;
}

SampleGLView::~SampleGLView()
{
	printf("-SampleGLView\n");
}

void SampleGLView::AttachedToWindow(void) {
	printf("AttachedToWindow\n");
	BGLView::AttachedToWindow();
	MakeFocus(true);
	LockGL();
	init();
	reshape(width, height);
	UnlockGL();
	fRun = true;
	fThread = spawn_thread(RenderThread, "render thread", B_NORMAL_PRIORITY, this);
	resume_thread(fThread);
}

void SampleGLView::DetachedFromWindow(void) {
	printf("DetachedFromWindow\n");
	status_t res;
	suspend_thread(fThread);
	fRun = false;
	wait_for_thread(fThread, &res);
	BGLView::AttachedToWindow();
}

void SampleGLView::FrameResized(float newWidth, float newHeight) {
	BGLView::FrameResized(newWidth, newHeight);
	LockGL();
	width = newWidth + 1;
	height = newHeight + 1;
	reshape(width, height);
	UnlockGL();
}

void SampleGLView::KeyDown(const char* bytes, int32 numBytes)
{
	if ((numBytes == 1) && (bytes[0] == B_SPACE)) {
		BRect rect = this->Bounds().OffsetToCopy(B_ORIGIN);
		//rect = BRect(0, 0, 3, 3);
		BBitmap *bmp = new BBitmap(rect, B_RGB32);
		int8 *bits = (int8*)bmp->Bits();
		int32 stride = bmp->BytesPerRow();
		for (int32 y = 0; y <= (int32)rect.Height(); y++)
			for (int32 x = 0; x <= (int32)rect.Width(); x++)
				*(int32*)(bits + y*stride + x*4) = 0xff000000 + x%0x100 + y%0x100*0x100;
		CopyPixelsIn(bmp, B_ORIGIN);
		//Invalidate();
		SwapBuffers();
		delete bmp;
	}
}

void SampleGLView::ErrorCallback(GLenum whichError) {
	fprintf(stderr, "Unexpected error occured (%d):\n", whichError);
	fprintf(stderr, " %s\n", gluErrorString(whichError));
}


void SampleGLView::gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width, GLint teeth, GLfloat tooth_depth)
{
  GLint i;
  GLfloat r0, r1, r2;
  GLfloat angle, da;
  GLfloat u, v, len;

  r0 = inner_radius;
  r1 = outer_radius - tooth_depth / 2.0;
  r2 = outer_radius + tooth_depth / 2.0;

  da = 2.0 * M_PI / teeth / 4.0;

  glShadeModel(GL_FLAT);

  glNormal3f(0.0, 0.0, 1.0);

  /* draw front face */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;
    glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
    glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
    if (i < teeth) {
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
    }
  }
  glEnd();

  /* draw front sides of teeth */
  glBegin(GL_QUADS);
  da = 2.0 * M_PI / teeth / 4.0;
  for (i = 0; i < teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;

    glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
  }
  glEnd();

  glNormal3f(0.0, 0.0, -1.0);

  /* draw back face */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;
    glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
    if (i < teeth) {
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
    }
  }
  glEnd();

  /* draw back sides of teeth */
  glBegin(GL_QUADS);
  da = 2.0 * M_PI / teeth / 4.0;
  for (i = 0; i < teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;

    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
    glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
  }
  glEnd();

  /* draw outward faces of teeth */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i < teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;

    glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
    glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    u = r2 * cos(angle + da) - r1 * cos(angle);
    v = r2 * sin(angle + da) - r1 * sin(angle);
    len = sqrt(u * u + v * v);
    u /= len;
    v /= len;
    glNormal3f(v, -u, 0.0);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
    glNormal3f(cos(angle), sin(angle), 0.0);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
    u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
    v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
    glNormal3f(v, -u, 0.0);
    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
    glNormal3f(cos(angle), sin(angle), 0.0);
  }

  glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
  glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

  glEnd();

  glShadeModel(GL_SMOOTH);

  /* draw inside radius cylinder */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;
    glNormal3f(-cos(angle), -sin(angle), 0.0);
    glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
    glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
  }
  glEnd();

}

void SampleGLView::init() {
	T0 = system_time();
	displayTime = system_time();
	Frames = 0;
	viewDist = 40.0;
	view_rotx = 20.0; view_roty = 30.0; view_rotz = 0.0;
	angle = 0.0;

	static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};
	static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
	static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
	static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
	GLint i;

	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	/* make the gears */
	gear1 = glGenLists(1);
	glNewList(gear1, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
	gear(1.0, 4.0, 1.0, 20, 0.7);
	glEndList();

	gear2 = glGenLists(1);
	glNewList(gear2, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
	gear(0.5, 2.0, 2.0, 10, 0.7);
	glEndList();

	gear3 = glGenLists(1);
	glNewList(gear3, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
	gear(1.3, 2.0, 0.5, 10, 0.7);
	glEndList();

	glEnable(GL_NORMALIZE);

	printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	// printf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

	glClearColor(1.0, 1.0, 1.0, 0.0);
}

void SampleGLView::cleanup()
{
	glDeleteLists(gear1, 1);
	glDeleteLists(gear2, 1);
	glDeleteLists(gear3, 1);
}

void SampleGLView::draw()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

    glTranslatef(0.0, 0.0, -viewDist);

    glRotatef(view_rotx, 1.0, 0.0, 0.0);
    glRotatef(view_roty, 0.0, 1.0, 0.0);
    glRotatef(view_rotz, 0.0, 0.0, 1.0);

    glPushMatrix();
      glTranslatef(-3.0, -2.0, 0.0);
      glRotatef(angle, 0.0, 0.0, 1.0);
      glCallList(gear1);
    glPopMatrix();

    glPushMatrix();
      glTranslatef(3.1, -2.0, 0.0);
      glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
      glCallList(gear2);
    glPopMatrix();

    glPushMatrix();
      glTranslatef(-3.1, 4.2, 0.0);
      glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
      glCallList(gear3);
    glPopMatrix();

  glPopMatrix();

  Frames++;

  {
    bigtime_t t = system_time();
    if (t - T0 >= 5000000) {
      GLfloat seconds = (t - T0) / 1000000.0;
      GLfloat fps = Frames / seconds;
      printf("%d frames in %6.3f seconds = %6.3f FPS\n", Frames, seconds, fps);
      fflush(stdout);
      T0 = t;
      Frames = 0;
    }
  }
}

void SampleGLView::reshape(int width, int height)
{
  GLfloat h = (GLfloat) height / (GLfloat) width;

  glViewport(0, 0, (GLint) width, (GLint) height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -h, h, 5.0, 200.0);
  glMatrixMode(GL_MODELVIEW);
}

void SampleGLView::idle()
{
  bigtime_t t = system_time();

  double dt = (t - displayTime)/1000000.0;
  displayTime = t;

  angle += 70.0 * dt;  /* 70 degrees per second */
  angle = fmod(angle, 360.0); /* prevents eventual overflow */
}


status_t SampleGLView::RenderThread(void *arg)
{
	printf("+RenderThread\n");
	SampleGLView &v = *(SampleGLView*)arg;
	while (v.fRun) {
		v.LockGL();
		v.draw();
		v.SwapBuffers();
		v.UnlockGL();
		v.idle();
	}
	printf("-RenderThread\n");
	return 0;
}
