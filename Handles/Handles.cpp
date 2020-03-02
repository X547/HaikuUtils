#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <LayoutBuilder.h>
#include <Rect.h>
#include <OS.h>
#include <sys/stat.h>
#include <fs_info.h>
#include <syscalls.h>
#include <vfs_defs.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>

#include <Entry.h>
#include <Node.h>

enum {
	invokeMsg = 1,
	selectMsg = 2,
};

enum {
	nameCol,
	idCol,
	modeCol,
	devCol,
	nodeCol,
	devNameCol,
	volNameCol,
	fsNameCol
};

void ListHandles(BColumnListView *view, BRow *teamRow, team_id team) {
	BString buf;
	char path[B_OS_NAME_LENGTH];
	fd_info info;
	fs_info fsInfo;
	uint32 cookie;
	BRow *row;
	cookie = 0;
	int i;
	while (_kern_get_next_fd_info(team, &cookie, &info, sizeof(fd_info)) == B_OK) {
		row = new BRow();
		view->AddRow(row, teamRow);

		if (_kern_entry_ref_to_path(info.device, info.node, NULL, path, B_OS_NAME_LENGTH) == B_OK)
			row->SetField(new BStringField(path), nameCol);
		else
			row->SetField(new BStringField(""), nameCol);

		row->SetField(new BIntegerField(info.number), idCol);

		if ((info.open_mode & O_RWMASK) == O_RDONLY) {buf = "R";}
		if ((info.open_mode & O_RWMASK) == O_WRONLY) {buf = "W";}
		if ((info.open_mode & O_RWMASK) == O_RDWR  ) {buf = "RW";}
		row->SetField(new BStringField(buf), modeCol);
		row->SetField(new BIntegerField(info.device), devCol);
		row->SetField(new BIntegerField(info.node), nodeCol);

		fs_stat_dev(info.device, &fsInfo);
		row->SetField(new BStringField(fsInfo.device_name), devNameCol);
		row->SetField(new BStringField(fsInfo.volume_name), volNameCol);
		row->SetField(new BStringField(fsInfo.fsh_name), fsNameCol);
	}
}

void ListTeams(BColumnListView *view) {
	team_info info;
	int32 cookie;
	BRow *row, *row2;
	cookie = 0;
	view->AddColumn(new BStringColumn("Name", 250, 50, 512, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), idCol);
	view->AddColumn(new BStringColumn("Mode", 48, 32, 128, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), modeCol);
	view->AddColumn(new BIntegerColumn("Dev", 64, 32, 128, B_ALIGN_RIGHT), devCol);
	view->AddColumn(new BIntegerColumn("Node", 64, 32, 128, B_ALIGN_RIGHT), nodeCol);
	view->AddColumn(new BStringColumn("Device", 96, 32, 512, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), devNameCol);
	view->AddColumn(new BStringColumn("Volume", 96, 32, 512, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), volNameCol);
	view->AddColumn(new BStringColumn("FS", 96, 32, 512, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), fsNameCol);
	while (get_next_team_info(&cookie, &info) == B_OK) {
		row = new BRow();
		row->SetField(new BStringField(info.args), nameCol);
		row->SetField(new BIntegerField(info.team), idCol);
		view->AddRow(row);
		ListHandles(view, row, info.team);
	}
}

class HandlesWindow: public BWindow
{
private:
	BColumnListView *view;
public:
	HandlesWindow(BRect frame): BWindow(frame, "Handles", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		BRow *row1, *row2;
		this->view = new BColumnListView("view", 0);
		this->view->SetInvocationMessage(new BMessage(invokeMsg));
		this->view->SetSelectionMessage(new BMessage(selectMsg));
		ListTeams(this->view);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.AddGroup(B_HORIZONTAL)
				.Add(this->view)
				.SetInsets(-1)
			.End();
	}

	void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
		default:
			BWindow::MessageReceived(msg);
		}
	}
};

class HandlesApplication: public BApplication
{
public:
	HandlesApplication(): BApplication("application/x-vnd.Test-Handles")
	{
	}

	void ReadyToRun()
	{
		BRect rect(0, 0, 832, 480);
		rect.OffsetBy(64, 64);
		BWindow *wnd = new HandlesWindow(rect);
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->Show();
	}
};


int main()
{
	HandlesApplication app;
	app.Run();
	return 0;
}
