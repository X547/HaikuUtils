#include "KeyboardDevice.h"
#include "KeyboardWindow.h"
#include <malloc.h>

KeyboardDevice *kbdDev;


KeyboardDevice::KeyboardDevice(): handler(this, NULL, NULL, 0, 0)
{
	key_map  *keyMap;
	char     *chars;
	bigtime_t delay;
	int32     rate;
	
	get_key_map(&keyMap, &chars);
	get_key_repeat_delay(&delay);
	get_key_repeat_rate (&rate);
	
	handler.SetKeyMap(keyMap, chars);
	handler.LocksChanged(keyMap->lock_settings);
	handler.SetRepeat(delay, rate);
}

KeyboardDevice::~KeyboardDevice()
{
}


status_t KeyboardDevice::InitCheck()
{
	kbdDev = this;
	
	static input_device_ref keyboard = {"Screen keyboard", B_KEYBOARD_DEVICE, (void*)this};
	static input_device_ref *devices[2] = {&keyboard, NULL};
	
	RegisterDevices(devices);
	return B_OK;
}


status_t KeyboardDevice::Start(const char* name, void* cookie)
{
	wnd = new KeyboardWindow(&handler);
	wnd->Show();
	return B_OK;
}
	
status_t KeyboardDevice::Stop(const char* name, void* cookie)
{
	if (wnd->Lock()) {
		wnd->Quit();
		wnd = NULL;
	}
	return B_OK;
}


status_t KeyboardDevice::Control(const char* name, void* cookie, uint32 command, BMessage* message)
{
	switch (command) {
	case B_KEY_LOCKS_CHANGED: {
		key_map  *keyMap;
		char     *chars;	
		get_key_map(&keyMap, &chars);
		handler.LocksChanged(keyMap->lock_settings);
		delete keyMap;
		delete chars;
	}
	case B_KEY_MAP_CHANGED: {
		key_map  *keyMap;
		char     *chars;	
		get_key_map(&keyMap, &chars);
		handler.SetKeyMap(keyMap, chars);
	}
	case B_KEY_REPEAT_DELAY_CHANGED:
	case B_KEY_REPEAT_RATE_CHANGED: {
		bigtime_t delay;
		int32     rate;
		get_key_repeat_delay(&delay);
		get_key_repeat_rate (&rate);
		handler.SetRepeat(delay, rate);
	}
	}
	return B_OK;
}


extern "C" BInputServerDevice *instantiate_input_device()
{
	return new(std::nothrow) KeyboardDevice();
}

