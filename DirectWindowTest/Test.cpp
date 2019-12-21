/*
	
	Test.cpp
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "TestWindow.h"
#include "Test.h"

int main()
{	
	TestApp myApplication;

	if (!myApplication.abort_required)
		myApplication.Run();	
	return 0;
}

TestApp::TestApp() : BApplication("application/x-vnd.Test-DirectWindow")
{
	abort_required = false;
	aWindow = new TestWindow(BRect(120, 150, 120 + 512, 150 + 512), "DirectWindow Test");
	// showing the window will also start the direct connection. If you
	// Sync() after the show, the direct connection will be established
	// when the Sync() return (as far as any part of the content area of
	// the window is visible after the show).
	if (!abort_required)
		aWindow->Show();
}
