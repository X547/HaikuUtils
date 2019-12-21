/*
	
	TestWindow.cpp
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>
#include <View.h>
#include <Screen.h>
#include "TestWindow.h"
#include "Test.h"

#include <string.h>
#include <stdlib.h>
#include <algorithm>
using std::min;
using std::max;

#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <Alert.h>
#include <File.h>
#include <Path.h>

#include <Debug.h>

TestWindow::TestWindow(BRect frame, const char *name)
: BDirectWindow(frame, name, B_TITLED_WINDOW, 0)
{
	BView			*view;

	// allocate the semaphore used to synchronise the star animation drawing access.
	drawing_lock = create_sem(0, "star locker");

	// spawn the star animation thread (we have better set the force quit flag to
	// false first).
	kill_my_thread = false;
	my_thread = spawn_thread(TestWindow::DrawingThread, "DrawingThread",
							 B_DISPLAY_PRIORITY, (void*)this);
	resume_thread(my_thread);
	
	// add a view in the background to insure that the content area will
	// be properly erased in black. This erase mechanism is not synchronised
	// with the star animaton, which means that from time to time, some
	// stars will be erreneously erased by the view redraw. But as every
	// single star is erased and redraw every frame, that graphic glitch
	// will last less than a frame, and that just in the area being redraw
	// because of a window resize, move... Which means the glitch won't
	// be noticeable. The other solution for erasing the background would
	// have been to do it on our own (which means some serious region
	// calculation and handling all color_space). Better to use the kit
	// to do it, as it gives us access to hardware acceleration...
	frame.OffsetTo(0.0, 0.0);
	view = new BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW);
	
	// The only think we want from the view mechanism is to
	// erase the background in black. Because of the way the
	// star animation is done, this erasing operation doesn't
	// need to be synchronous with the animation. That the
	// reason why we use both the direct access and the view
	// mechanism to draw in the same area of the TestWindow.
	// Such thing is usualy not recommended as synchronisation
	// is generally an issue (drawing in random order usualy
	// gives remanent incorrect result).	
	// set the view color to be black (nicer update).
	view->SetViewColor(0, 0, 0);
	AddChild(view);
	
	// Add a shortcut to switch in and out of fullscreen mode.
	AddShortcut('f', B_COMMAND_KEY, new BMessage('full'));	
	
	// As we said before, the window shouldn't get wider than 2048 in any
	// direction, so those limits will do.
	SetSizeLimits(40.0, 2000.0, 40.0, 2000.0);
	
	syncMode = vSync;
	
	// If the graphic card/graphic driver we use doesn't support directwindow
	// in window mode, then we need to switch to fullscreen immediatly, or
	// the user won't see anything, as long as it doesn't used the undocumented
	// shortcut. That would be bad behavior...
	if (!BDirectWindow::SupportsWindowMode()) {
		bool		sSwapped;
		char		*buf;
		BAlert		*quit_alert;
		key_map		*map;
		
		get_key_map(&map, &buf);
		sSwapped = (map->left_control_key==0x5d) && (map->left_command_key==0x5c);
		free(map);
		free(buf);
		quit_alert = new BAlert("QuitAlert", sSwapped ?
		                        "This demo runs only in full screen mode.\n"
		                        "While running, press 'Ctrl-Q' to quit.":
		                        "This demo runs only in full screen mode.\n"
		                        "While running, press 'Alt-Q' to quit.",
		                        "Quit", "Start demo", NULL,
		                        B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (quit_alert->Go() == 0)
			((TestApp*)be_app)->abort_required = true;
		else
			SetFullScreen(true);
	}
}

TestWindow::~TestWindow()
{
	status_t		result;

	// force the drawing_thread to quit. This is the easiest way to deal
	// with potential closing problem. When it's not practical, we
	// recommand to use Hide() and Sync() to force the disconnection of
	// the direct access, and use some other flag to guarantee that your
	// drawing thread won't draw anymore. After that, you can pursue the
	// window destructor and kill your drawing thread...
	kill_my_thread = true;
	delete_sem(drawing_lock);
	wait_for_thread(my_thread, &result);
}

bool TestWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

void TestWindow::MessageReceived(BMessage *message)
{
	int8		key_code;

	switch(message->what) {
	// Switch between full-screen mode and windowed mode.
	case 'full' :
		SetFullScreen(!IsFullScreen());
		break;
	case B_KEY_DOWN :
		if (message->FindInt8("byte", &key_code) != B_OK)
			break;
		if (key_code == '1')
			syncMode = noSync;
		else if (key_code == '2')
			syncMode = timerSync;
		else if (key_code == '3')
			syncMode = vSync;
		break;
	default :
		BDirectWindow::MessageReceived(message);
		break;
	}
}

void TestWindow::DirectConnected(direct_buffer_info *info)
{
	// you need to use that mask to read the buffer state.
	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
	// start a direct screen connection.
	case B_DIRECT_START :
		SwitchContext(info);	// update the direct screen infos.
		release_sem(drawing_lock);	// unblock the animation thread.
		break;
	// stop a direct screen connection.
	case B_DIRECT_STOP :
		acquire_sem(drawing_lock);	// block the animation thread.
		break;
	// modify the state of a direct screen connection.
	case B_DIRECT_MODIFY :
		acquire_sem(drawing_lock);	// block the animation thread.
		SwitchContext(info);	// update the direct screen infos.
		release_sem(drawing_lock);	// unblock the animation thread.
		break;
	default :
		break;
	}
}

// This function update the internal graphic context of the TestWindow
// object to reflect the infos send through the DirectConnected API.
// It also update the state of stars (and erase some of them) to
// insure a clean transition during resize. As this function is called
// in DirectConnected, it's a bad idea to do any heavy drawing (long)
// operation. But as we only update the stars (the background will be
// updated a little later by the view system), it's not a big deal.
void TestWindow::SwitchContext(direct_buffer_info *info)
{
	uint32			i;
	
	// update to the new clipping region. The local copy is kept relative
	// to the center of the animation (origin of the star coordinate).
	window_bound = info->window_bounds;
	clipping_bound = info->clip_bounds;
	// the clipping count is bounded (see comment in header file).
	clipping_list_count = min(info->clip_list_count, (uint32)MAX_CLIPPING_RECT_COUNT);
	for (i=0; i<clipping_list_count; i++) {
		clipping_list[i].left   = info->clip_list[i].left   - info->window_bounds.left;
		clipping_list[i].top    = info->clip_list[i].top    - info->window_bounds.top;
		clipping_list[i].right  = info->clip_list[i].right  - info->window_bounds.left;
		clipping_list[i].bottom = info->clip_list[i].bottom - info->window_bounds.top;
	}
	
	stride = info->bytes_per_row/(info->bits_per_pixel/8);
	
	bits = info->bits;
	// update the screen bases (only one of the 3 will be really used).
	draw_ptr8 = ((uint8*)info->bits) + stride*info->window_bounds.top + info->window_bounds.left;
	draw_ptr16 = ((uint16*)info->bits) + stride*info->window_bounds.top + info->window_bounds.left;
	draw_ptr32 = ((uint32*)info->bits) + stride*info->window_bounds.top + info->window_bounds.left;

	if (info->buffer_state & B_BUFFER_RESET) {
		// cancel the erasing of all stars
	} else {
		// in the other case, update the stars that will stay visible.
	}
	
	// set the pixel_depth and the pixel data, from the color_space.
	switch (info->pixel_format) {
	case B_RGBA32 :
	case B_RGB32 :
		pixel_depth = 32;
		break;
	case B_RGB16 :
		pixel_depth = 16;
		break;
	case B_RGB15 :
	case B_RGBA15 :
		pixel_depth = 16;
		break;
	case B_CMAP8 :
		pixel_depth = 8;
		break;
	case B_RGBA32_BIG :
	case B_RGB32_BIG :
		pixel_depth = 32;
		break;
	case B_RGB16_BIG :
		pixel_depth = 16;
		break;
	case B_RGB15_BIG :
	case B_RGBA15_BIG :
		pixel_depth = 16;
		break;
	default:	// unsupported color space?
		fprintf(stderr, "ERROR - unsupported color space!\n");
		exit(1);
		break;
	}
	
	if (pixel_depth != 32) {
		fprintf(stderr, "Only 32 bit supported\n");
		exit(1);
	}
}

void TestWindow::FillRect(int32 l, int32 t, int32 r, int32 b, int32 c)
{
	clipping_rect rect;
	for (uint32 i = 0; i < clipping_list_count; i++) {
		rect.left   = max(clipping_list[i].left,   l);
		rect.top    = max(clipping_list[i].top,    t);
		rect.right  = min(clipping_list[i].right,  r);
		rect.bottom = min(clipping_list[i].bottom, b);
		for (int32 y = rect.top; y <= rect.bottom; y++)
			for (int32 x = rect.left; x <= rect.right; x++)
				draw_ptr32[y*stride + x] = c;
	}
}

// This is the thread doing the star animation itself. It would be easy to
// adapt to do any other sort of pixel animation.
status_t TestWindow::DrawingThread(void *data)
{
	TestWindow		*wnd;
	BScreen screen;
	bigtime_t time;
	
	// receive a pointer to the TestWindow object.
	wnd = (TestWindow*)data;
	
	while (!wnd->kill_my_thread) {
		time = system_time();
		acquire_sem(wnd->drawing_lock);
		if (wnd->kill_my_thread) break;
		wnd->DrawFrame();
		release_sem(wnd->drawing_lock);
		switch (wnd->syncMode) {
		case noSync:
			break;
		case timerSync:
			snooze_until(time + 1000000/60, B_SYSTEM_TIMEBASE);
			break;
		case vSync:
			screen.WaitForRetrace();
			//snooze_until(time + 11000, B_SYSTEM_TIMEBASE);
			break;
		}
	}
	return 0;
}

void TestWindow::DrawFrame()
{
	int32 w, h;
	int32 c;
	w = window_bound.right  - window_bound.left;
	h = window_bound.bottom - window_bound.top;
	FillRect(0, 0, w, h, 0xffffffff);
	for (int32 i = 0; i < 64; i++) {
		switch (i % 3) {
		case 0: c = 0xffff0000; break;
		case 1: c = 0xff00ff00; break;
		case 2: c = 0xff0000ff; break;
		}
		FillRect(16 + 16*i, 16 + 16*i, 16 + 64 + 16*i, 16 + 64 + 16*i, c);
	}
}
