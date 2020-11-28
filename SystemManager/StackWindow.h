#ifndef _STACKWINDOW_H_
#define _STACKWINDOW_H_

#include <Window.h>
#include <OS.h>
#include <private/debug/debug_support.h>
#include <private/shared/AutoDeleter.h>

class BColumnListView;

class StackWindow: public BWindow
{
public:
	BColumnListView *fView;


	thread_id fId;
	team_id fTeam;
	port_id fDebuggerPort, fNubPort;
	debug_context fDebugContext;

	StackWindow(thread_id id);
	~StackWindow();
};

void OpenStackWindow(thread_id id, BPoint center);

#endif	// _STACKWINDOW_H_
