#ifndef _SUITEEDITOR_H_
#define _SUITEEDITOR_H_

#include <Window.h>
#include <Messenger.h>

class BColumnListView;

class SuiteEditor: public BWindow
{
private:
	BMessenger fHandle;
	status_t fStatus;

	BColumnListView *fView;

public:
	SuiteEditor(BMessenger handle);
	status_t InitCheck();
};

#endif	// _SUITEEDITOR_H_
