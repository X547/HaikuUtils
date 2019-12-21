#ifndef _KEYBOARDWINDOW_H_
#define _KEYBOARDWINDOW_H_

#include <Window.h>


class KeyboardHandler;


class KeyboardWindow: public BWindow
{
public:
	KeyboardWindow(KeyboardHandler *handler);
};

#endif	// _KEYBOARDWINDOW_H_
