#include "KeyboardHandler.h"
#include <Message.h>
#include <malloc.h>
#include <string.h>


void PressKey(uint8 *state, uint32 code)
{
	state[code/8] |= 1 << (code%8);
}

void ReleaseKey(uint8 *state, uint32 code)
{
	state[code/8] &= ~(uint8)(1 << (code%8));
}

bool IsKeyPressed(uint8 *state, uint32 code)
{
	return state[code/8] & (1 << (code%8));
}


void KeyboardHandler::StartRepeating(BMessage *msg)
{
	if (repeatThread > 0) StopRepeating();
	
	repeatMsg = *msg;
	repeatThread = spawn_thread(RepeatThread, "repeat thread", B_REAL_TIME_PRIORITY, this);
	if (repeatThread > 0)
		resume_thread(repeatThread);
}

void KeyboardHandler::StopRepeating()
{
	if (repeatThread > 0) {
		kill_thread(repeatThread);
		repeatThread = 0;
	}
}

status_t KeyboardHandler::RepeatThread(void *arg)
{
	KeyboardHandler *h = (KeyboardHandler*)arg;
	int32 count;
	
	snooze(h->repeatDelay);
	while (true) {
		h->repeatMsg.ReplaceInt64("when", system_time());
		h->repeatMsg.FindInt32("be:key_repeat", &count);
		h->repeatMsg.ReplaceInt32("be:key_repeat", count + 1);
		
		BMessage *msg = new BMessage(h->repeatMsg);
		if (msg != NULL)
			if (h->dev->EnqueueMessage(msg) != B_OK)
				delete msg;
		
		snooze(/* 1000000 / h->repeatRate */ 50000);
	}
}


KeyboardHandler::KeyboardHandler(BInputServerDevice *dev, key_map *keyMap, char *chars, bigtime_t repeatDelay, int32 repeatRate)
	: dev(dev), keyMap(keyMap), chars(chars), repeatDelay(repeatDelay), repeatRate(repeatRate)
{
	repeatThread = 0;
	memset(state, 0, sizeof(state));
	notifiers = NULL;
}

KeyboardHandler::~KeyboardHandler()
{
	StopRepeating();
	if (keyMap != NULL) free((void*)keyMap);
	if (chars  != NULL) free((void*)chars);
}


void KeyboardHandler::SetKeyMap(key_map *keyMap, char *chars)
{
	if (this->keyMap != NULL) free((void*)this->keyMap);
	if (this->chars  != NULL) free((void*)this->chars);
	
	this->keyMap = keyMap;
	this->chars  = chars;
	
	LocksChanged(keyMap->lock_settings);
	
	for (KeyboardNotifier *i = notifiers; i != NULL; i = i->next) i->KeymapChanged();
}

void KeyboardHandler::SetRepeat(bigtime_t delay, int32 rate)
{
	repeatDelay = delay;
	repeatRate  = rate;
}


void KeyboardHandler::InstallNotifier(KeyboardNotifier *notifier)
{
	notifier->next = notifiers;
	notifiers = notifier;
}

void KeyboardHandler::UninstallNotifier(KeyboardNotifier *notifier)
{
	if (notifier == notifiers) {
		notifiers = notifiers->next;
	} else {
		KeyboardNotifier *prev = notifiers;
		while (prev->next != notifier)
			prev = prev->next;
		prev->next = notifier->next;
	}
}


void KeyboardHandler::State(uint *state)
{
	memcpy(state, this->state, sizeof(state));
}

uint32 KeyboardHandler::Modifiers()
{
	return modifiers;
}

void KeyboardHandler::KeyString(uint32 code, char *str, size_t len)
{
	uint32 i;
	char *ch;
	switch (modifiers & (B_SHIFT_KEY | B_CONTROL_KEY | B_OPTION_KEY | B_CAPS_LOCK)) {
		case B_OPTION_KEY | B_CAPS_LOCK | B_SHIFT_KEY: ch = chars + keyMap->option_caps_shift_map[code]; break;
		case B_OPTION_KEY | B_CAPS_LOCK:               ch = chars + keyMap->option_caps_map[code];       break;
		case B_OPTION_KEY | B_SHIFT_KEY:               ch = chars + keyMap->option_shift_map[code];      break;
		case B_OPTION_KEY:                             ch = chars + keyMap->option_map[code];            break;
		case B_CAPS_LOCK  | B_SHIFT_KEY:               ch = chars + keyMap->caps_shift_map[code];        break;
		case B_CAPS_LOCK:                              ch = chars + keyMap->caps_map[code];              break;
		case B_SHIFT_KEY:                              ch = chars + keyMap->shift_map[code];             break;
		default:
			if (modifiers & B_CONTROL_KEY)             ch = chars + keyMap->control_map[code];
			else                                       ch = chars + keyMap->normal_map[code];
	}
	if (len > 0) {
		for (i = 0; (i < (uint32)ch[0]) && (i < len-1); ++i)
			str[i] = ch[i+1];
		str[i] = '\0';
	}
}


void KeyboardHandler::KeyChanged(uint32 code, bool isDown)
{
	uint8 state[16];
	memcpy(state, this->state, sizeof(state));
	if (isDown)
		state[code/8] |= 1 << (code%8);
	else
		state[code/8] &= ~(uint8)(1 << (code%8));
	StateChanged(state);
}

void KeyboardHandler::CodelessKeyChanged(const char *str, bool isDown, bool doRepeat)
{
	BMessage *msg = new BMessage();
	if (msg == NULL) return;
	
	msg->AddInt64("when", system_time());
	msg->AddInt32("key", 0);
	msg->AddString("bytes", str);
	msg->AddInt32("raw_char", 0xa);

	if (isDown) {
		msg->what = B_KEY_DOWN;

		if (doRepeat) {
			msg->AddInt32("be:key_repeat", 1);
			StartRepeating(msg);
		}
	} else {
		msg->what = B_KEY_UP;
		
		if (doRepeat)
			StopRepeating();
	}

	if (dev->EnqueueMessage(msg) != B_OK)
		delete msg;
}

void KeyboardHandler::StateChanged(uint8 state[16])
{
	uint32 i, j;
	BMessage *msg;
	
	
	uint32 modifiers = this->modifiers & (B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
	if (IsKeyPressed(state, keyMap->left_shift_key))    modifiers |= B_SHIFT_KEY   | B_LEFT_SHIFT_KEY;
	if (IsKeyPressed(state, keyMap->right_shift_key))   modifiers |= B_SHIFT_KEY   | B_RIGHT_SHIFT_KEY;
	if (IsKeyPressed(state, keyMap->left_command_key))  modifiers |= B_COMMAND_KEY | B_LEFT_COMMAND_KEY;
	if (IsKeyPressed(state, keyMap->right_command_key)) modifiers |= B_COMMAND_KEY | B_RIGHT_COMMAND_KEY;
	if (IsKeyPressed(state, keyMap->left_control_key))  modifiers |= B_CONTROL_KEY | B_LEFT_CONTROL_KEY;
	if (IsKeyPressed(state, keyMap->right_control_key)) modifiers |= B_CONTROL_KEY | B_RIGHT_CONTROL_KEY;
	if (IsKeyPressed(state, keyMap->caps_key))          modifiers ^= B_CAPS_LOCK;
	if (IsKeyPressed(state, keyMap->scroll_key))        modifiers ^= B_SCROLL_LOCK;
	if (IsKeyPressed(state, keyMap->num_key))           modifiers ^= B_NUM_LOCK;
	if (IsKeyPressed(state, keyMap->left_option_key))   modifiers |= B_OPTION_KEY  | B_LEFT_OPTION_KEY;
	if (IsKeyPressed(state, keyMap->right_option_key))  modifiers |= B_OPTION_KEY  | B_RIGHT_OPTION_KEY;
	if (IsKeyPressed(state, keyMap->menu_key))          modifiers |= B_MENU_KEY;
	
	if (this->modifiers != modifiers) {
		msg = new BMessage(B_MODIFIERS_CHANGED);
		if (msg != NULL) {
			msg->AddInt64("when", system_time());
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt32("be:old_modifiers", this->modifiers);
			msg->AddData("states", B_UINT8_TYPE, state, 16);
			
			if (dev->EnqueueMessage(msg) == B_OK)
				this->modifiers = modifiers;
			else
				delete msg;
		}
	}
	
	
	uint8 diff[16];
	char rawCh;
	char str[5];
	
	for (i = 0; i < 16; ++i)
		diff[i] = this->state[i] ^ state[i];
	
	for (i = 0; i < 128; ++i) {
		if (diff[i/8] & (1 << (i%8))) {
			msg = new BMessage();
			if (msg) {
				KeyString(i, str, sizeof(str));
				
				msg->AddInt64("when", system_time());
				msg->AddInt32("key", i);
				msg->AddInt32("modifiers", modifiers);
				msg->AddData("states", B_UINT8_TYPE, state, 16);
				
				if (str[0] != '\0') {
					if (chars[keyMap->normal_map[i]] != 0)
						rawCh = chars[keyMap->normal_map[i] + 1];
					else
						rawCh = str[0];
					
					for (j = 0; str[j] != '\0'; ++j)
						msg->AddInt8("byte", str[j]);
					
					msg->AddString("bytes", str);
					msg->AddInt32("raw_char", rawCh);
				}
				
				if (state[i/8] & (1 << (i%8))) {
					if (str[0] != '\0')
						msg->what = B_KEY_DOWN;
					else
						msg->what = B_UNMAPPED_KEY_DOWN;

					msg->AddInt32("be:key_repeat", 1);
					StartRepeating(msg);
				} else {
					if (str[0] != '\0')
						msg->what = B_KEY_UP;
					else
						msg->what = B_UNMAPPED_KEY_UP;
					
					StopRepeating();
				}
				
				if (dev->EnqueueMessage(msg) == B_OK) {
					for (j = 0; j < 16; ++j)
						this->state[j] = state[j];
				} else
					delete msg;
			}
		}
	}
}

void KeyboardHandler::LocksChanged(uint32 locks)
{
	locks &= B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK;
	if (modifiers != locks) {
		BMessage *msg = new BMessage(B_MODIFIERS_CHANGED);
		if (msg != NULL) {
			msg->AddInt64("when", system_time());
			msg->AddInt32("modifiers", locks);
			msg->AddInt32("be:old_modifiers", modifiers);
			msg->AddData("states", B_UINT8_TYPE, state, 16);
			
			if (dev->EnqueueMessage(msg) == B_OK)
				modifiers = locks;
			else
				delete msg;
		}
	}
}
