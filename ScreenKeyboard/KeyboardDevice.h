#ifndef _KEYBOARDDEVICE_H_
#define _KEYBOARDDEVICE_H_

#include <InputServerDevice.h>
#include "KeyboardHandler.h"

class KeyboardWindow;


class KeyboardDevice: public BInputServerDevice
{
public:
	KeyboardDevice();
	~KeyboardDevice();
	
	status_t InitCheck();
	
	status_t Start(const char* name, void* cookie);
	status_t Stop(const char* name, void* cookie);
	
	status_t Control(const char* name, void* cookie, uint32 command, BMessage* message);

private:
	KeyboardHandler handler;
	KeyboardWindow *wnd;
};


extern KeyboardDevice *kbdDev;

extern "C" _EXPORT BInputServerDevice*	instantiate_input_device();

#endif	// _KEYBOARDDEVICE_H_
