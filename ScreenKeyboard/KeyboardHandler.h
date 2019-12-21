#ifndef _KEYBOARDHANDLER_H_
#define _KEYBOARDHANDLER_H_

#include <OS.h>
#include <InputServerDevice.h>
#include <InterfaceDefs.h>


class KeyboardNotifier
{
private:
	friend class KeyboardHandler;
	KeyboardNotifier *next;
	
public:
	virtual void KeymapChanged() = 0;
/*
	virtual void KeyStringChanged(uint32 scanCode)                  = 0;
	virtual void KeyStateChanged (uint32 scanCode, bool isDown)     = 0;
	virtual void ModifierChanged (uint32 modifier, bool isOn)       = 0;
*/
};

class KeyboardHandler
{
private:
	BInputServerDevice *dev;
	
	key_map *keyMap;
	char *chars;

	uint8 state[16];
	uint32 modifiers;
	
	bigtime_t repeatDelay;
	int32 repeatRate;
	BMessage repeatMsg;
	thread_id repeatThread;
	
	KeyboardNotifier *notifiers;
	
	void StartRepeating(BMessage *msg);
	void StopRepeating();
	static status_t RepeatThread(void *arg);
	
public:
	KeyboardHandler(BInputServerDevice *dev, key_map *keyMap, char *chars, bigtime_t repeatDelay, int32 repeatRate);
	~KeyboardHandler();
	
	void SetKeyMap(key_map *keyMap, char *chars);
	void SetRepeat(bigtime_t delay, int32 rate);
	
	void InstallNotifier  (KeyboardNotifier *notifier);
	void UninstallNotifier(KeyboardNotifier *notifier);
	
	void   State(uint *state);
	uint32 Modifiers();
	void   KeyString(uint32 code, char *str, size_t len);
	
	void KeyChanged(uint32 code, bool isDown);
	void CodelessKeyChanged(const char *str, bool isDown, bool doRepeat);
	void StateChanged(uint8 state[16]);
	void LocksChanged(uint32 locks);
};


#endif	// _KEYBOARDHANDLER_H_
