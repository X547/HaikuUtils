#ifndef _SUITEEDITOR_H_
#define _SUITEEDITOR_H_

#include <Window.h>
#include <Messenger.h>

class SuiteEditor: public BWindow
{
private:
	BMessenger fHandle;
	status_t fStatus;

public:
	SuiteEditor(BMessenger handle);
	status_t InitCheck();
};

#endif	// _SUITEEDITOR_H_
