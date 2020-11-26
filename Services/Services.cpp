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
#include <private/shared/AutoDeleter.h>

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
	nameCol,
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


class IconStringField: public BStringField
{
private:
	ObjectDeleter<BBitmap> fIcon;

public:
	IconStringField(BBitmap *icon, const char *string): BStringField(string), fIcon(icon) {}
	BBitmap *Icon() {return fIcon.Get();}
};

class IconStringColumn: public BStringColumn
{
public:
	IconStringColumn(
		const char* title, float width,
		float minWidth, float maxWidth, uint32 truncate,
		alignment align = B_ALIGN_LEFT
	): BStringColumn(title, width, minWidth, maxWidth, truncate, align) {}

	void DrawField(BField* _field, BRect rect, BView* parent)
	{
		IconStringField *field = (IconStringField*)_field;

		parent->PushState();
		parent->SetDrawingMode(B_OP_ALPHA);
		parent->DrawBitmap(field->Icon(), rect.LeftTop() + BPoint(4, 0));
		parent->PopState();
		rect.left += field->Icon()->Bounds().Width() + 1;

		BStringColumn::DrawField(field, rect, parent);
	}

	float GetPreferredWidth(BField* field, BView* parent) const
	{
		return BStringColumn::GetPreferredWidth(field, parent) + dynamic_cast<IconStringField*>(field)->Icon()->Bounds().Width() + 1;
	}

	bool AcceptsField(const BField* field) const
	{
		return dynamic_cast<const IconStringField*>(field) != NULL;
	}
};


static BRow *FindStringRow(BColumnListView *view, BRow *parent, const char *str)
{
	BRow *row;
	BString name;
	for (int32 i = 0; i < view->CountRows(parent); i++) {
		row = view->RowAt(i, parent);
		name = ((BStringField*)row->GetField(nameCol))->String();
		if (name == BString(str))
			return row;
	}
	return NULL;
}

static void ListJobs(BColumnListView *view, BRow *parent, const char *target, BStringList &jobs)
{
	BLaunchRoster roster;
	BRow *row;
	BStringList prevNames;
	BMessage info;
	bool boolVal;
	BString strVal;
	team_id team;
	status_t status;

	for (int32 i = 0; i < view->CountRows(parent); i++) {
		row = view->RowAt(i, parent);
		prevNames.Add(((BStringField*)row->GetField(nameCol))->String());
	}

	for (int32 i = 0; i < jobs.CountStrings(); i++) {
		const char *name = jobs.StringAt(i).String();
		prevNames.Remove(name);
		if ((status = roster.GetJobInfo(name, info)) != B_OK) {
			printf("%s: can't get info (%d)\n", name, status);
			continue;
		}

		const char *curTarget;
		if (info.FindString("target", &curTarget) < B_OK) curTarget = "";

		if (!(
			((target == NULL) && (curTarget == NULL)) ||
			((target != NULL) && (curTarget != NULL) && (strcmp(target, curTarget) == 0))
		)) continue;

		row = FindStringRow(view, parent, name);

		if (row == NULL) {
			row = new BRow();
			view->AddRow(row, parent);
		}

		BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);

		const char *path;
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

		row->SetField(new IconStringField(icon, name), nameCol);
		if (info.FindBool("enabled", &boolVal) == B_OK)
			row->SetField(new BStringField(boolVal? "yes": "no"), enabledCol);
		else
			row->SetField(new BStringField("-"), enabledCol);
		if (info.FindBool("running", &boolVal) == B_OK)
			row->SetField(new BStringField(boolVal? "yes": "no"), runningCol);
		else
			row->SetField(new BStringField("-"), runningCol);

		const char *arg;
		strVal = "";
		for (int32 j = 0; info.FindString("launch", j, &arg) >= B_OK; j++) {
			if (j > 0) strVal += " ";
			size_t k = 0; while (arg[k] != '\0' && !(arg[k] == ' ')) k++;
			bool needQuotes = arg[k] != '\0';
			if (needQuotes) strVal += '"';
			strVal += arg;
			if (needQuotes) strVal += '"';
		}
		row->SetField(new BStringField(strVal), launchCol);
		if (info.FindInt32("team", &team) != B_OK)
			team = -1;
		row->SetField(new BIntegerField(team), pidCol);
	}

	for (int32 i = 0; i < prevNames.CountStrings(); i++) {
		row = FindStringRow(view, parent, prevNames.StringAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static void ListServices(BColumnListView *view) {
	BLaunchRoster roster;
	BRow *row;
	BStringList names, jobs;
	BStringList prevNames, prevJobs;
	BString name;
	BMessage info;
	BString strVal, path;
	status_t status;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevNames.Add(((BStringField*)row->GetField(nameCol))->String());
	}

	status = roster.GetJobs(NULL, jobs);

	status = roster.GetTargets(names);
	names.Add("<no target>", 0);

	for (int32 i = 0; i < names.CountStrings(); i++) {
		name = names.StringAt(i).String();
		prevNames.Remove(name);

		row = FindStringRow(view, NULL, name);

		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
			view->ExpandOrCollapse(row, true);
		}

		BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);
		BMimeType genericAppType(B_APP_MIME_TYPE);
		status = genericAppType.GetIcon(icon, B_MINI_ICON);

		row->SetField(new IconStringField(icon, name), nameCol);
/*
		row->SetField(new BStringField("-"), enabledCol);
		row->SetField(new BStringField("-"), runningCol);
		row->SetField(new BStringField("-"), launchCol);
		row->SetField(new BIntegerField(-1), pidCol);
*/
		ListJobs(view, row, (i == 0)? "": name, jobs);
	}

	for (int32 i = 0; i < prevNames.CountStrings(); i++) {
		row = FindStringRow(view, NULL, prevNames.StringAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static void InitList(BColumnListView *view) {
	view->AddColumn(new IconStringColumn("Name", 256, 50, 512, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BStringColumn("Enabled", 64, 32, 128, B_TRUNCATE_MIDDLE), enabledCol);
	view->AddColumn(new BStringColumn("Running", 64, 32, 128, B_TRUNCATE_MIDDLE), runningCol);
	view->AddColumn(new BStringColumn("Launch", 256, 32, 4096, B_TRUNCATE_MIDDLE), launchCol);
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

	void GetContentSize(float* width, float* height)
	{
		if (fBitmap.Get() == NULL) {
			*width = 0;
			*height = 0;
			return;
		}
		*width = fBitmap->Bounds().Width() + 1;
		*height = fBitmap->Bounds().Height() + 1;
	};

	void DrawContent()
	{
		if (fBitmap.Get() == NULL)
			return;

		BRect frame = Frame();
		BPoint center = BPoint((frame.left + frame.right)/2, (frame.top + frame.bottom)/2);
		Menu()->PushState();
		Menu()->SetDrawingMode(B_OP_ALPHA);
		Menu()->DrawBitmap(fBitmap.Get(), center - BPoint(fBitmap->Bounds().Width()/2, fBitmap->Bounds().Height()/2));
		Menu()->PopState();
	}

private:
	ObjectDeleter<BBitmap> fBitmap;
};


static BBitmap *LoadIcon(int32 id, int32 width, int32 height)
{
	size_t dataSize;
	const void* data = BApplication::AppResources()->FindResource(B_VECTOR_ICON_TYPE, id, &dataSize);

	if (data == NULL) return NULL;

	ObjectDeleter<BBitmap> bitmap(new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32));

	if (bitmap.Get() == NULL) return NULL;

	if (BIconUtils::GetVectorIcon((const uint8*)data, dataSize, bitmap.Get()) != B_OK)
		return NULL;

	return bitmap.Detach();
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
			BRow *row, *parent;
			BString name;
			row = fView->CurrentSelection(NULL);
			if (row == NULL) {return;}
			name = ((BStringField*)row->GetField(nameCol))->String();
			fView->FindParent(row, &parent, NULL);
			bool isTarget = parent == NULL;
			switch (msg->what) {
				case startMsg: if (isTarget) roster.Target(name); else roster.Start(name); break;
				case stopMsg: if (isTarget) roster.StopTarget(name); else roster.Stop(name); break;
				case restartMsg:
					if (isTarget) {
						roster.StopTarget(name); roster.Target(name);
					} else {
						roster.Stop(name); roster.Start(name);
					}
					break;
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
		BWindow *wnd = new ServicesWindow(BRect(0, 0, 512 + 256, 480));
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
