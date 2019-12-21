/*
	
	TestWindow.h
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include <DirectWindow.h>

#include <OS.h>


class TestWindow : public BDirectWindow {
public:
		// standard constructor and destrcutor
	TestWindow(BRect frame, const char *name); 
	virtual ~TestWindow();

		// standard window member
	virtual	bool QuitRequested();
	virtual	void MessageReceived(BMessage *message);

		// this is the hook controling direct screen connection
	virtual void DirectConnected(direct_buffer_info *info);

private:
	// this function is used to inforce a direct access context
	// modification.
	void	SwitchContext(direct_buffer_info *info);
	
	void FillRect(int32 l, int32 t, int32 r, int32 b, int32 c);

	static status_t DrawingThread(void *data);
	void DrawFrame();
		
		enum {
			noSync = 1,
			timerSync = 2,
			vSync = 3,
		// used for the local copy of the clipping region
			MAX_CLIPPING_RECT_COUNT = 256
		};


		// flag used to force the drawing thread to quit.
		bool kill_my_thread;
		// the drawing thread doing the star animation.
		thread_id my_thread;
		// used to synchronise the star animation drawing.
		sem_id drawing_lock;
		
		int32 syncMode;
		
		// the pixel drawing can be done in 3 depth : 8, 16 or 32.
		int32 pixel_depth;
		
		void *bits;
		// base pointer of the screen, one per pixel_depth
		uint8 *draw_ptr8;
		uint16 *draw_ptr16;
		uint32 *draw_ptr32;
		
		// offset, in bytes, between two lines of the frame buffer.
		int32 stride;
	
		// clipping region, defined as a list of rectangle, including the
		// smaller possible bounding box. This application will draw only
		// in the 64 first rectangles of the region. Region more complex
		// than that won't be completly used. This is a valid approximation
		// with the current clipping architecture. If a region more complex
		// than that is passed to your application, that just means that
		// the user is doing something really, really weird.
		clipping_rect		window_bound;
		clipping_rect		clipping_bound;
		clipping_rect		clipping_list[MAX_CLIPPING_RECT_COUNT];
		uint32				clipping_list_count;
};

#endif
