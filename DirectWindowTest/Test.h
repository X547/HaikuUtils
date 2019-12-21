/*
	
	Test.h
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef TEST_H
#define TEST_H

#include <Application.h>
#include "TestWindow.h"

class TestApp : public BApplication {
public:
	TestApp();
	bool abort_required;
private:
	TestWindow *aWindow;
};

#endif
