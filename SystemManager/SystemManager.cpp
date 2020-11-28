#include <stdio.h>

#include <OS.h>
#include <image.h>
#include <private/kernel/util/KMessage.h>
#include <private/system/extended_system_info_defs.h>
#include <private/libroot/extended_system_info.h>
#include <private/system/syscall_process_info.h>
#include <private/system/syscalls.h>
#include <Application.h>
#include <Window.h>
#include <Alert.h>
#include <View.h>
#include <TabView.h>
#include <Rect.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/shared/AutoDeleter.h>

#include <Entry.h>
#include <Path.h>
#include <NodeInfo.h>

#include "TeamWindow.h"
#include "Errors.h"
#include "Utils.h"

enum {
	invokeMsg = 1,
	selectMsg,
	updateMsg,

	setLayoutMsg,

	terminateMsg,
	suspendMsg,
	resumeMsg,
	sendSignalMsg,

	showLocationMsg,
};

enum {
	nameCol = 0,
	idCol,
	parentIdCol,
	sidCol,
	gidCol,
	uidCol,
	pathCol,
};

enum {
	statNameCol = 0,
	statValueCol,
};

enum ViewLayout {
	flatLayout = 0,
	treeLayout = 1,
	sessionsLayout = 2,
};

struct SignalRec
{
	const char *name;
	int val;
};

SignalRec signals[] = {
	{"SIGHUP", 1},
	{"SIGINT", 2},
	{"SIGQUIT", 3},
	{"SIGILL", 4},
	{"SIGCHLD", 5},
	{"SIGABRT", 6},
	{"SIGPIPE", 7},
	{"SIGFPE", 8},
	{"SIGKILL", 9},
	{"SIGSTOP", 10},
	{"SIGSEGV", 11},
	{"SIGCONT", 12},
	{"SIGTSTP", 13},
	{"SIGALRM", 14},
	{"SIGTERM", 15},
	{"SIGTTIN", 16},
	{"SIGTTOU", 17},
	{"SIGUSR1", 18},
	{"SIGUSR2", 19},
	{"SIGWINCH", 20},
	{"SIGKILLTHR", 21},
	{"SIGTRAP", 22},
	{"SIGPOLL", 23},
	{"SIGPROF", 24},
	{"SIGSYS", 25},
	{"SIGURG", 26},
	{"SIGVTALRM", 27},
	{"SIGXCPU", 28},
	{"SIGXFSZ", 29},
	{"SIGBUS", 30},
	{"SIGRESERVED1", 31},
	{"SIGRESERVED2", 32},
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


struct RowTree
{
	ObjectDeleter<RowTree> next, down;
	ObjectDeleter<BRow> row;
};


static void BuildRowTree(ObjectDeleter<RowTree> &tree, BColumnListView *view, BRow *row)
{
	tree.SetTo(new RowTree());
	tree->row.SetTo(row);
	RowTree *last = NULL;
	ObjectDeleter<RowTree> newNode;
	int32 count = view->CountRows(row);
	for (int32 i = 0; i < count; i++) {
		BuildRowTree(newNode, view, view->RowAt(i, row));
		if (last == NULL) {
			tree->down.SetTo(newNode.Detach());
			last = tree->down.Get();
		} else {
			last->next.SetTo(newNode.Detach());
			last = last->next.Get();
		}
	}
}

static void RemoveRow(BColumnListView *view, BRow *row, ObjectDeleter<RowTree> &tree)
{
	BuildRowTree(tree, view, row);
	view->RemoveRow(row);
}

static void InsertRow(BColumnListView *view, BRow *parent, ObjectDeleter<RowTree> &tree)
{
	if (tree.Get() == NULL) return;

	BRow *row = tree->row.Detach();
	view->AddRow(row, parent);
	if (parent != NULL)
		view->ExpandOrCollapse(parent, true);
	ObjectDeleter<RowTree> list(tree->down.Detach());
	while (list.Get() != NULL) {
		ObjectDeleter<RowTree> next(list->next.Detach());
		InsertRow(view, row, list);
		list.SetTo(next.Detach());
	}
	tree.Unset();
}

static void SetRowParent(BColumnListView *view, BRow *row, BRow *newParent)
{
	BRow *oldParent;
	view->FindParent(row, &oldParent, NULL);
	if (newParent != oldParent) {
		ObjectDeleter<RowTree> tree;
		RemoveRow(view, row, tree);
		InsertRow(view, newParent, tree);
	}
}

static BRow *FindIntRow(BColumnListView *view, BRow *parent, int32 val)
{
	BRow *row, *row2;
	BString name;
	for (int32 i = 0; i < view->CountRows(parent); i++) {
		row = view->RowAt(i, parent);
		if (((BIntegerField*)row->GetField(idCol))->Value() == val)
			return row;
		row2 = FindIntRow(view, row, val);
		if (row2 != NULL)
			return row2;
	}
	return NULL;
}

static BRow *FindIntRowList(BList &list, int32 val)
{
	for (int32 i = 0; i < list.CountItems(); i++) {
		BRow *row = (BRow*)list.ItemAt(i);
		if (((BIntegerField*)row->GetField(idCol))->Value() == val)
			return row;
	}
	return NULL;
}

static void CollectRowList(BList &list, BColumnListView *view, BRow *parent = NULL)
{
	for (int32 i = 0; i < view->CountRows(parent); i++) {
		BRow *row = view->RowAt(i, parent);
		list.AddItem(row);
		CollectRowList(list, view, row);
	}
}

static void RelayoutTeams(BColumnListView *view, ViewLayout layout)
{
	BList list;
	CollectRowList(list, view);

	switch (layout) {
	case flatLayout: {
		for (int32 i = 0; i < list.CountItems(); i++) {
			BRow *row = (BRow*)list.ItemAt(i);
			SetRowParent(view, row, NULL);
		}
		break;
	}
	case treeLayout: {
		for (int32 i = 0; i < list.CountItems(); i++) {
			BRow *row = (BRow*)list.ItemAt(i);
			BRow *parent = FindIntRowList(list, ((BIntegerField*)row->GetField(parentIdCol))->Value());
			SetRowParent(view, row, parent);
		}
		break;
	}
	case sessionsLayout: {
		break;
	}
	}

}

static void ListTeams(BColumnListView *view, ViewLayout layout) {
	status_t status;
	team_info info;
	int32 cookie;
	BRow *row;
	cookie = 0;
	BList prevRows;
	BString str;

	CollectRowList(prevRows, view);

	while (get_next_team_info(&cookie, &info) == B_OK) {
		int32 uid = -1, gid = -1;
		KMessage extInfo;
		if (get_extended_team_info(info.team, B_TEAM_INFO_BASIC, extInfo) >= B_OK) {
			if (extInfo.FindInt32("uid", &uid) < B_OK) uid = -1;
			if (extInfo.FindInt32("gid", &gid) < B_OK) gid = -1;
		}

		int32 imageCookie = 0;
		image_info imageInfo;
		status = get_next_image_info(info.team, &imageCookie, &imageInfo);
		if (status < B_OK) strcpy(imageInfo.name, "");
		BPath path(imageInfo.name);

		BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);
		BEntry entry;
		entry_ref ref;
		if (status == B_OK) {
			entry.SetTo(imageInfo.name);
			status = entry.GetRef(&ref);
		}
		if (status == B_OK)
			status = BNodeInfo::GetTrackerIcon(&ref, icon, B_MINI_ICON);
		if (status != B_OK) {
			BMimeType genericAppType(B_APP_MIME_TYPE);
			status = genericAppType.GetIcon(icon, B_MINI_ICON);
		}

		row = FindIntRowList(prevRows, info.team);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		} else {
			prevRows.RemoveItem(row);
		}
		row->SetField(new IconStringField(icon, path.Leaf()), nameCol);
		row->SetField(new BIntegerField(info.team), idCol);
		row->SetField(new BIntegerField(_kern_process_info(info.team, PARENT_ID)), parentIdCol);
		row->SetField(new BIntegerField(_kern_process_info(info.team, SESSION_ID)), sidCol);
		row->SetField(new BIntegerField(_kern_process_info(info.team, GROUP_ID)), gidCol);
		GetUserGroupString(str, uid, gid);
		row->SetField(new BStringField(str), uidCol);
		row->SetField(new BStringField(imageInfo.name), pathCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = (BRow*)prevRows.ItemAt(i);
		view->RemoveRow(row);
		delete row;
	}

	RelayoutTeams(view, layout);

#if 0
	switch (treeLayout) {
	case flatLayout:
		break;
	case treeLayout: {
		int32 count = view->CountRows(), i = 0;
		while (i < count) {
			row = view->RowAt(i);
			BRow *parent = FindIntRow(view, NULL, ((BIntegerField*)row->GetField(parentIdCol))->Value());
			if (parent != NULL) {
				SetRowParent(view, row, parent);
				i = 0; count = view->CountRows();
			} else {
				i++;
			}
		}
		break;
	}
	case sessionsLayout: {
		{
			int32 count = view->CountRows(), i = 0;
			while (i < count) {
				row = view->RowAt(i);
				int32 gid = ((BIntegerField*)row->GetField(gidCol))->Value();
				BRow *parent = FindIntRow(view, NULL, gid);
				if ((parent != NULL) && (parent != row)) {
					SetRowParent(view, row, parent);
					i = 0; count = view->CountRows();
				} else {
					i++;
				}
			}
		}
		{
			int32 count = view->CountRows(), i = 0;
			while (i < count) {
				row = view->RowAt(i);
				int32 sid = ((BIntegerField*)row->GetField(sidCol))->Value();
				BRow *parent = FindIntRow(view, NULL, sid);
				if ((parent != NULL) && (parent != row)) {
					SetRowParent(view, row, parent);
					i = 0; count = view->CountRows();
				} else {
					i++;
				}
			}
		}
		break;
	}
	}
#endif
}

static BColumnListView* NewTeamsView()
{
	BColumnListView *view;
	view = new BColumnListView("Teams", B_NAVIGABLE);
	view->SetInvocationMessage(new BMessage(invokeMsg));
	view->AddColumn(new IconStringColumn("Name", 256 - 32, 50, 500, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), idCol);
	view->AddColumn(new BIntegerColumn("Parent", 64, 32, 128, B_ALIGN_RIGHT), parentIdCol);
	view->AddColumn(new BIntegerColumn("Session", 64, 32, 128, B_ALIGN_RIGHT), sidCol);
	view->AddColumn(new BIntegerColumn("Group", 64, 32, 128, B_ALIGN_RIGHT), gidCol);
	view->AddColumn(new BStringColumn("User", 64 + 16, 32, 128, B_TRUNCATE_END), uidCol);
	view->AddColumn(new BStringColumn("Path", 512, 50, 1024, B_TRUNCATE_MIDDLE), pathCol);
	view->SetColumnVisible(parentIdCol, false);
	return view;
}


static void GetUsedMax(BString &str, uint64 used, uint64 max)
{
	int32 ratio = 100;
	if (max > 0) {
		ratio = int32(double(used)/double(max)*100.0);
	}
	str.SetToFormat("%" B_PRIu64 "/%" B_PRIu64 " (% " B_PRId32 " %%)", used, max, ratio);
}

static void ListStats(BColumnListView *view)
{
	int32 rowId = 0;
	BString str;

	system_info info;

	if (get_system_info(&info) >= B_OK) {
		// TODO: convert to readable date-time format
		str.SetToFormat("%" B_PRIdBIGTIME, info.boot_time);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu32, info.cpu_count);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMax(str, info.used_pages, info.max_pages);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu64, info.cached_pages);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu64, info.block_cache_pages);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu64, info.ignored_pages);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu64, info.needed_memory);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu64, info.free_memory);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMax(str, info.free_swap_pages, info.max_swap_pages);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu32, info.page_faults);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMax(str, info.used_sems, info.max_sems);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMax(str, info.used_ports, info.max_ports);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMax(str, info.used_threads, info.max_threads);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMax(str, info.used_teams, info.max_teams);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		view->RowAt(rowId++)->SetField(new BStringField(info.kernel_name), statValueCol);

		str.SetToFormat("%s %s", info.kernel_build_date, info.kernel_build_time);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);
	}
}

static void NewInfoRow(BColumnListView *view, const char *name)
{
	BRow *row = new BRow();
	row->SetField(new BStringField(name), statNameCol);
	view->AddRow(row);
}

static BColumnListView *NewStatsView()
{
	BColumnListView *view;
	view = new BColumnListView("Stats", B_NAVIGABLE);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), statNameCol);
	view->AddColumn(new BStringColumn("Value", 256, 50, 1024, B_TRUNCATE_END, B_ALIGN_RIGHT), statValueCol);

	NewInfoRow(view, "Boot time");
	NewInfoRow(view, "CPU count");
	NewInfoRow(view, "Pages");
	NewInfoRow(view, "Cached pages");
	NewInfoRow(view, "Block cache pages");
	NewInfoRow(view, "Ignored pages");
	NewInfoRow(view, "Needed memory");
	NewInfoRow(view, "Free memory");
	NewInfoRow(view, "Swap pages");
	NewInfoRow(view, "Page faults");
	NewInfoRow(view, "Semaphores");
	NewInfoRow(view, "Ports");
	NewInfoRow(view, "Threads");
	NewInfoRow(view, "Teams");
	NewInfoRow(view, "Kernel name");
	NewInfoRow(view, "Kernel build timestamp");

	ListStats(view);
	return view;
}


class TestWindow: public BWindow
{
private:
	BColumnListView *fTeamsView;
	BColumnListView *fStatsView;
	ViewLayout fLayout;
	BMessageRunner fListUpdater;

public:
	TestWindow(BRect frame): BWindow(frame, "SystemManager", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
		fLayout(treeLayout),
		fListUpdater(BMessenger(this), BMessage(updateMsg), 500000)
	{
		BMenuBar *menuBar;
		BMenu *signalMenu;
		BTabView *tabView;
		BTab *tab;

		menuBar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
		BLayoutBuilder::Menu<>(menuBar)
/*
			.AddMenu(new BMenu("File"))
				.AddItem(new BMenuItem("New", new BMessage('item'), 'N'))
				.AddItem(new BMenuItem("Open...", new BMessage('item'), 'O'))
				.AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'))
				.AddSeparator()
				.AddItem(new BMenuItem("Save", new BMessage('item'), 'S'))
				.AddItem(new BMenuItem("Save as...", new BMessage('item'), 'S', B_SHIFT_KEY))
				.AddSeparator()
				.AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'))
				.End()
*/
			.AddMenu(new BMenu("View"))
				.AddMenu(new BMenu("Layout"))
					.AddItem(new BMenuItem("Flat", []() {
						BMessage *msg = new BMessage(setLayoutMsg);
						msg->SetInt32("val", flatLayout);
						return msg;
					}()))
					.AddItem(new BMenuItem("Tree", []() {
						BMessage *msg = new BMessage(setLayoutMsg);
						msg->SetInt32("val", treeLayout);
						return msg;
					}()))
					.AddItem(new BMenuItem("Sessions and groups", []() {
						BMessage *msg = new BMessage(setLayoutMsg);
						msg->SetInt32("val", sessionsLayout);
						return msg;
					}()))
					.End()
				.End()
			.AddMenu(new BMenu("Action"))
/*
				.AddItem(new BMenuItem("Update", new BMessage(updateMsg), 'R'))
				.AddSeparator()
*/
				.AddItem(new BMenuItem("Terminate", new BMessage(terminateMsg), 'T'))
				.AddItem(new BMenuItem("Suspend", new BMessage(suspendMsg)))
				.AddItem(new BMenuItem("Resume", new BMessage(resumeMsg)))
				.AddMenu(signalMenu = new BMenu("Send signal"))
					.End()
				.AddSeparator()
				.AddItem(new BMenuItem("Show location", new BMessage(showLocationMsg)))
				.End()
			.End()
		;

		for (size_t i = 0; i < sizeof(signals)/sizeof(signals[0]); i++) {
			BMessage *msg = new BMessage(sendSignalMsg);
			msg->AddInt32("val", signals[i].val);
			signalMenu->AddItem(new BMenuItem(signals[i].name, msg));
		}

		tabView = new BTabView("tab_view", B_WIDTH_FROM_LABEL);
		tabView->SetBorder(B_NO_BORDER);

		//tab = new BTab(); tabView->AddTab(NewTeamsView("Apps"), tab);
		tab = new BTab(); tabView->AddTab(fTeamsView = NewTeamsView(), tab);
		//tab = new BTab(); tabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Services", B_FOLLOW_NONE), tab);
		//tab = new BTab(); tabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Sockets", B_FOLLOW_NONE), tab);
		tab = new BTab(); tabView->AddTab(fStatsView = NewStatsView(), tab);

		ListTeams(fTeamsView, fLayout);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(menuBar)
			.AddGroup(B_VERTICAL, 0)
				.Add(tabView)
				.SetInsets(-1, 0, -1, -1)
				.End()
			.End()
		;
	}

	team_id SelectedTeam(const char **name = NULL)
	{
		BRow *row = fTeamsView->CurrentSelection(NULL);
		if (row == NULL) return -1;
		if (name != NULL) *name = ((IconStringField*)row->GetField(nameCol))->String();
		return ((BIntegerField*)row->GetField(idCol))->Value();
	}

	void MessageReceived(BMessage *msg)
	{
		try {
			switch (msg->what) {
			case invokeMsg: {
				team_id team = SelectedTeam();
				if (team >= B_OK) {
					OpenTeamWindow(team,
						BPoint((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2)
					);
				}
				return;
			}
			case updateMsg: {
				ListTeams(fTeamsView, fLayout);
				ListStats(fStatsView);
				return;
			}
			case setLayoutMsg: {
				int32 layout;
				if (msg->FindInt32("val", &layout) >= B_OK) {
					fLayout = (ViewLayout)layout;
					RelayoutTeams(fTeamsView, fLayout);
				}
				return;
			}
			case terminateMsg: {
				team_id team;
				int32 which;
				BInvoker *invoker = NULL;
				if (msg->FindInt32("which", &which) < B_OK) {
					const char *name;
					team = SelectedTeam(&name);
					if (team < B_OK) return;
					Check(msg->AddInt32("team", team));
					BString str;
					str.SetToFormat("Are you sure you want to terminate team \"%s\" (%" B_PRId32 ")?", name, team);
					BAlert *alert = new BAlert("SystemManager", str, "No", "Yes", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					invoker = new BInvoker(new BMessage(*msg), this);
					invoker->Message()->SetPointer("invoker", invoker);
					alert->Go(invoker);
					return;
				} else {
					if (msg->FindPointer("invoker", &(void*&)invoker) >= B_OK) {
						delete invoker; invoker = NULL;
					}
					if (which != 1) return;
					CheckRetVoid(msg->FindInt32("team", &team));
				}
				if (team >= B_OK)
					Check(kill_team(team), "Can't terminate team.");
				return;
			}
			case suspendMsg: {
				team_id team = SelectedTeam();
				if (team >= B_OK)
					CheckErrno(kill(team, SIGSTOP), "Can't suspend team.");
				return;
			}
			case resumeMsg: {
				team_id team = SelectedTeam();
				if (team >= B_OK)
					CheckErrno(kill(team, SIGCONT), "Can't resume team.");
				return;
			}
			case sendSignalMsg: {
				int32 signal;
				if (msg->FindInt32("val", &signal) >= B_OK) {
					team_id team = SelectedTeam();
					if (team >= B_OK)
						CheckErrno(kill(team, signal), "Can't send signal.");
				}
				return;
			}
			case showLocationMsg: {
				BRow *row = fTeamsView->CurrentSelection(NULL);
				if (row != NULL) {
					const char *path = ((BStringField*)row->GetField(pathCol))->String();

					BEntry entry(path);
					node_ref node;
					entry.GetNodeRef(&node);

					BEntry parent;
					entry.GetParent(&parent);
					entry_ref parentRef;
					parent.GetRef(&parentRef);

					BMessage message(B_REFS_RECEIVED);
					message.AddRef("refs", &parentRef);
					message.AddData("nodeRefToSelect", B_RAW_TYPE, &node, sizeof(node_ref));

					BMessenger("application/x-vnd.Be-TRAK").SendMessage(&message);
				}
				return;
			}
			}
		} catch (StatusError &err) {
			ShowError(err);
		}
		BWindow::MessageReceived(msg);
	}
};

class TestApplication: public BApplication
{
public:
	TestApplication(): BApplication("application/x-vnd.Test-SystemManager")
	{
	}

	void ReadyToRun() {
		BWindow *wnd = new TestWindow(BRect(0, 0, 640, 480));
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->CenterOnScreen();
		wnd->Show();
	}
};


int main()
{
	(new TestApplication())->Run();
	return 0;
}
