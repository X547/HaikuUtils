#ifndef _TEAMWINDOW_H_
#define _TEAMWINDOW_H_

#include <Window.h>
#include <OS.h>
#include <MessageRunner.h>

class BTabView;
class BColumnListView;

class TeamWindow: public BWindow
{
private:
	BMessageRunner fListUpdater;
	BTabView *fTabView;
	BColumnListView *fInfoView;
	BColumnListView *fImagesView;
	BColumnListView *fThreadsView;
	BColumnListView *fAreasView;
	BColumnListView *fPortsView;
	BColumnListView *fSemsView;
	BColumnListView *fHandlesView;

public:
	team_id fId;

	TeamWindow(team_id id);
	~TeamWindow();

	void MessageReceived(BMessage *msg);
};

void OpenTeamWindow(team_id id, BPoint center);

#endif	// _TEAMWINDOW_H_
