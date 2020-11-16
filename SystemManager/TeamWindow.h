#ifndef _TEAMWINDOW_H_
#define _TEAMWINDOW_H_

#include <Window.h>
#include <OS.h>

class BColumnListView;

class TeamWindow: public BWindow
{
private:
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
