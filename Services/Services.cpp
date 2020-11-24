#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <MessageRunner.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <LayoutBuilder.h>
#include <Rect.h>
#include <OS.h>
#include <StringList.h>
#include <private/app/LaunchRoster.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>

#include <Resources.h>
#include <IconUtils.h>

#include <Entry.h>
#include <NodeInfo.h>

#include "Resources.h"

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
		row = FindStringRow(view, prevNames.StringAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

void InitList(BColumnListView *view) {
	view->AddColumn(new BBitmapColumn("Icon", 16, 16, 16, B_ALIGN_CENTER), iconCol);
	view->AddColumn(new BStringColumn("Name", 192, 50, 512, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BStringColumn("Target", 96, 50, 128, B_TRUNCATE_MIDDLE), targetCol);
	view->AddColumn(new BStringColumn("Enabled", 64, 32, 128, B_TRUNCATE_MIDDLE), enabledCol);
	view->AddColumn(new BStringColumn("Running", 64, 32, 128, B_TRUNCATE_MIDDLE), runningCol);
	view->AddColumn(new BStringColumn("Launch", 256, 32, 512, B_TRUNCATE_MIDDLE), launchCol);
	view->AddColumn(new BIntegerColumn("PID", 64, 32, 128, B_ALIGN_LEFT), pidCol);

	ListServices(view);
}

class IconMenuItem: public BMenuItem
{
public:
	IconMenuItem(BBitmap *bitmap, BMessage* message, char shortcut = 0, uint32 modifiers = 0):
		BMenuItem("", message, shortcut, modifiers),
		fBitmap(bitmap)
	{
	}

	~IconMenuItem()
	{
		if (fBitmap != NULL) {
			delete fBitmap; fBitmap = NULL;
		}
	}

	void GetContentSize(float* width, float* height)
	{
		if (fBitmap == NULL) {
			*width = 0;
			*height = 0;
			return;
		}
		*width = fBitmap->Bounds().Width() + 1;
		*height = fBitmap->Bounds().Height() + 1;
	};

	void DrawContent()
	{
		if (fBitmap == NULL)
			return;

		BRect frame = Frame();
		BPoint center = BPoint((frame.left + frame.right)/2, (frame.top + frame.bottom)/2);
		Menu()->PushState();
		Menu()->SetDrawingMode(B_OP_ALPHA);
		Menu()->DrawBitmap(fBitmap, center - BPoint(fBitmap->Bounds().Width()/2, fBitmap->Bounds().Height()/2));
		Menu()->PopState();
	}

private:
	BBitmap *fBitmap;
};


BBitmap *LoadIcon(int32 id, int32 width, int32 height)
{
	size_t dataSize;
	const void* data = BApplication::AppResources()->FindResource(B_VECTOR_ICON_TYPE, id, &dataSize);

	if (data == NULL) return NULL;

	BBitmap *bitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);

	if (bitmap == NULL) return NULL;

	if (BIconUtils::GetVectorIcon((const uint8*)data, dataSize, bitmap) != B_OK) {
		delete bitmap;
		return NULL;
	}

	return bitmap;
}


class ServicesWindow: public BWindow
{
private:
	BColumnListView *fView;
	BMessageRunner fListUpdater;

public:
	ServicesWindow(BRect frame): BWindow(frame, "Services", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
		fListUpdater(BMessenger(this), BMessage(updateMsg), 500000)
	{
		BMenuBar *menubar, *toolbar;

		BLayoutBuilder::Menu<>(menubar = new BMenuBar("menu", B_ITEMS_IN_ROW, true))
			.AddMenu(new BMenu("Action"))
				.AddItem(new BMenuItem("Start", new BMessage(startMsg)))
				.AddItem(new BMenuItem("Stop", new BMessage(stopMsg)))
				.AddItem(new BMenuItem("Restart", new BMessage(restartMsg)))
				.End()
			.End()
		;

		BLayoutBuilder::Menu<>(toolbar = new BMenuBar("toolbar", B_ITEMS_IN_ROW, true))
			.AddItem(new IconMenuItem(LoadIcon(resStartIcon, 16, 16), new BMessage(startMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resStopIcon, 16, 16), new BMessage(stopMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resRestartIcon, 16, 16), new BMessage(restartMsg)))
			.End()
		;

		fView = new BColumnListView("view", 0);
		fView->SetInvocationMessage(new BMessage(invokeMsg));
		fView->SetSelectionMessage(new BMessage(selectMsg));
		InitList(fView);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(menubar)
			.Add(toolbar)
			.AddGroup(B_HORIZONTAL)
				.Add(fView)
				.SetInsets(-1)
			.End()
		;

		SetKeyMenuBar(menubar);
	}

	void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
		case invokeMsg:
		case selectMsg:
			break;
		case updateMsg:
			ListServices(fView);
			break;
		case startMsg:
		case stopMsg:
		case restartMsg: {
			BLaunchRoster roster;
			BRow *row;
			BString name;
			row = fView->CurrentSelection(NULL);
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
		BWindow *wnd = new ServicesWindow(BRect(0, 0, 640, 480));
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->CenterOnScreen();
		wnd->Show();
	}
};


int main()
{
	ServicesApplication app;
	app.Run();
	return 0;
}
