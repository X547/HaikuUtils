#ifndef _TEAMWINDOW_H_
#define _TEAMWINDOW_H_

#include <Window.h>
#include <OS.h>
#include <MessageRunner.h>

class BTabView;
class BColumnListView;

enum {
	teamWindowShowImageMsg = 1,
	teamWindowShowThreadMsg,
	teamWindowShowAreaMsg,
	teamWindowShowPortMsg,
	teamWindowShowSemMsg,
	teamWindowShowFileMsg,

	teamWindowPrivateMsgBase
};

class TeamWindow: public BWindow
{
private:	
	BMessageRunner fListUpdater;
	BTabView *fTabView;
	BMenuBar *fMenuBar;
	BMenuItem *fCurMenu;
	BMenuItem *fInfoMenu;
	BMenuItem *fImagesMenu;
	BMenuItem *fThreadsMenu;
	BMenuItem *fSemsMenu;
	BColumnListView *fInfoView;
	BColumnListView *fImagesView;
	BColumnListView *fThreadsView;
	BColumnListView *fAreasView;
	BColumnListView *fPortsView;
	BColumnListView *fSemsView;
	BColumnListView *fFilesView;

public:
	team_id fId;

	TeamWindow(team_id id);
	~TeamWindow();

	void TabChanged();
	void SetMenu(BMenuItem *menu);
	void MessageReceived(BMessage *msg);
};

TeamWindow *OpenTeamWindow(team_id id, BPoint center);

#endif	// _TEAMWINDOW_H_
