#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <MessageRunner.h>
#include <MenuBar.h>
#include <LayoutBuilder.h>
#include <Rect.h>
#include <OS.h>
#include <StringList.h>
#include <private/app/LaunchRoster.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>

#include <Entry.h>
#include <NodeInfo.h>

enum {
	invokeMsg = 1,
	selectMsg,

	updateMsg,

	startMsg,
	stopMsg,
	restartMsg
};

enum {
	iconCol,
	nameCol,
	targetCol,
	enabledCol,
	runningCol,
	launchCol,
	pidCol
};


// BBitmapField don't delete bitmap in destructor, fix that

class BitmapField: public BBitmapField
{
public:
	BitmapField(BBitmap* bitmap): BBitmapField(bitmap) {}
	~BitmapField()
	{
		if (Bitmap() != NULL) {
			delete Bitmap(); SetBitmap(NULL);
		}
	}
};


BRow *FindStringRow(BColumnListView *view, const char *str)
{
	BRow *row;
	BString name;
	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		name = ((BStringField*)row->GetField(nameCol))->String();
		if (name == BString(str))
			return row;
	}
	return NULL;
}

void ListServices(BColumnListView *view) {
	BLaunchRoster roster;
	BRow *row;
	BStringList jobs;
	BStringList prevNames;
	BString name;
	BMessage info;
	BString strVal, path;
	bool boolVal;
	int32 intVal;
	team_id team;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevNames.Add(((BStringField*)row->GetField(nameCol))->String());
	}

	status_t status = roster.GetJobs(NULL, jobs);

	for (int32 i = 0; i < jobs.CountStrings(); i++) {
		name = jobs.StringAt(i).String();
		prevNames.Remove(name);
		if ((status = roster.GetJobInfo(name, info)) != B_OK) {
			printf("%s: can't get info (%d)\n", name.String(), status);
		}

		row = FindStringRow(view, name);

		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);

		BEntry entry;
		entry_ref ref;

		status = info.FindString("launch", &path);

		if (status == B_OK) {
			entry.SetTo(path);
			status = entry.GetRef(&ref);
		}

		if (status == B_OK)
			status = BNodeInfo::GetTrackerIcon(&ref, icon, B_MINI_ICON);
		if (status != B_OK) {
			BMimeType genericAppType(B_APP_MIME_TYPE);
			status = genericAppType.GetIcon(icon, B_MINI_ICON);
		}

		row->SetField(new BitmapField(icon), iconCol);

		row->SetField(new BStringField(name), nameCol);
		if (info.FindString("target", &strVal) == B_OK)
			row->SetField(new BStringField(strVal), targetCol);
		else
			row->SetField(new BStringField("-"), targetCol);
		if (info.FindBool("enabled", &boolVal) == B_OK)
			row->SetField(new BStringField(boolVal? "yes": "no"), enabledCol);
		else
			row->SetField(new BStringField("-"), enabledCol);
		if (info.FindBool("running", &boolVal) == B_OK)
			row->SetField(new BStringField(boolVal? "yes": "no"), runningCol);
		else
			row->SetField(new BStringField("-"), runningCol);
		if (info.FindString("launch", &strVal) == B_OK)
			row->SetField(new BStringField(strVal), launchCol);
		else {
			row->SetField(new BStringField("-"), launchCol);
		}
		if (info.FindInt32("team", &team) != B_OK)
			team = -1;
		row->SetField(new BIntegerField(team), pidCol);
	}

	for (int32 i = 0; i < prevNames.CountStrings(); i++) {
		view->RemoveRow(FindStringRow(view, prevNames.StringAt(i)));
	}
}

void ListTeams(BColumnListView *view) {
	team_info info;
	int32 cookie;
	BRow *row, *row2;
	cookie = 0;
	view->AddColumn(new BBitmapColumn("Icon", 16, 16, 16, B_ALIGN_CENTER), iconCol);
	view->AddColumn(new BStringColumn("Name", 192, 50, 512, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BStringColumn("Target", 96, 50, 128, B_TRUNCATE_MIDDLE), targetCol);
	view->AddColumn(new BStringColumn("Enabled", 64, 32, 128, B_TRUNCATE_MIDDLE), enabledCol);
	view->AddColumn(new BStringColumn("Running", 64, 32, 128, B_TRUNCATE_MIDDLE), runningCol);
	view->AddColumn(new BStringColumn("Launch", 256, 32, 512, B_TRUNCATE_MIDDLE), launchCol);
	view->AddColumn(new BIntegerColumn("PID", 64, 32, 128, B_ALIGN_LEFT), pidCol);

	ListServices(view);
}

class ServicesWindow: public BWindow
{
private:
	BColumnListView *view;
	BMessageRunner listUpdater;
public:
	ServicesWindow(BRect frame): BWindow(frame, "Services", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
		listUpdater(BMessenger(this), BMessage(updateMsg), 500000)
	{
		BMenu *menu, *menu2;
		BMenuItem *it;

		menu = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
		menu2 = new BMenu("Action");
			menu2->AddItem(new BMenuItem("Start", new BMessage(startMsg)));
			menu2->AddItem(new BMenuItem("Stop", new BMessage(stopMsg)));
			menu2->AddItem(new BMenuItem("Restart", new BMessage(restartMsg)));
			menu->AddItem(menu2);

		this->view = new BColumnListView("view", 0);
		this->view->SetInvocationMessage(new BMessage(invokeMsg));
		this->view->SetSelectionMessage(new BMessage(selectMsg));
		ListTeams(this->view);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			//.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(menu)
			.Add(this->view)
			//.SetInsets(-1)
			//.AddGlue()
		;
	}

	void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
		case invokeMsg:
		case selectMsg:
			break;
		case updateMsg:
			ListServices(this->view);
			break;
		case startMsg:
		case stopMsg:
		case restartMsg: {
			BLaunchRoster roster;
			BRow *row;
			BString name;
			row = this->view->CurrentSelection(NULL);
			if (row == NULL) {return;}
			name = ((BStringField*)row->GetField(nameCol))->String();
			switch (msg->what) {
				case startMsg: roster.Start(name); break;
				case stopMsg: roster.Stop(name); break;
				case restartMsg: roster.Stop(name); roster.Start(name); break;
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
		}
	}
};

class ServicesApplication: public BApplication
{
public:
	ServicesApplication(): BApplication("application/x-vnd.Test-Services")
	{
	}

	void ReadyToRun()
	{
		BRect rect(0, 0, 640, 480);
		rect.OffsetBy(64, 64);
		BWindow *wnd = new ServicesWindow(rect);
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->Show();
	}
};


int main()
{
	ServicesApplication app;
	app.Run();
	return 0;
}

/*
gcc -g -lbe ListArea.cpp /boot/system/develop/lib/libcolumnlistview.a -o ListArea
ListArea
*/
