#include <stdio.h>
#include <time.h>

#include <OS.h>
#include <image.h>
#include <private/kernel/util/KMessage.h>
#include <private/system/extended_system_info_defs.h>
#include <private/libroot/extended_system_info.h>
#include <private/system/syscall_process_info.h>
#include <private/system/syscalls.h>
#include <Application.h>
#include <PropertyInfo.h>
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
#include "UIUtils.h"

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
	memSizeCol,
	memAllocCol,
	userCol,
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

static void GetTeamMemory(size_t &size, size_t &alloc, team_id team)
{
	area_info info;
	ssize_t cookie = 0;

	// TODO: limit max enumerated area count to prevent freezes
	size = 0; alloc = 0;
	while (get_next_area_info(team, &cookie, &info) >= B_OK) {
		size += info.size;
		alloc += info.ram_size;
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
		size_t memSize, memAlloc;
		GetTeamMemory(memSize, memAlloc, info.team);
		GetSizeString(str, memSize); row->SetField(new BStringField(str), memSizeCol);
		GetSizeString(str, memAlloc); row->SetField(new BStringField(str), memAllocCol);
		GetUserGroupString(str, uid, gid);
		row->SetField(new BStringField(str), userCol);
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
	view->AddColumn(new BStringColumn("Mapped", 64 + 16, 32, 256, B_TRUNCATE_END), memSizeCol);
	view->AddColumn(new BStringColumn("Alloc", 64 + 16, 32, 256, B_TRUNCATE_END), memAllocCol);
	view->AddColumn(new BStringColumn("User", 64 + 16, 32, 128, B_TRUNCATE_END), userCol);
	view->AddColumn(new BStringColumn("Path", 512, 50, 1024, B_TRUNCATE_MIDDLE), pathCol);
	view->SetColumnVisible(parentIdCol, false);
	return view;
}


static void ListStats(BColumnListView *view)
{
	int32 rowId = 0;
	BString str;

	system_info info;

	if (get_system_info(&info) >= B_OK) {
		time_t unixTime = info.boot_time/1000000;
		struct tm *tm = localtime(&unixTime);
		str.SetToFormat("%04d.%02d.%02d %02d:%02d:%02d.%06d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(info.boot_time%1000000));
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		str.SetToFormat("%" B_PRIu32, info.cpu_count);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMaxSize(str, info.used_pages * B_PAGE_SIZE, info.max_pages * B_PAGE_SIZE);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetSizeString(str, info.cached_pages * B_PAGE_SIZE);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetSizeString(str, info.block_cache_pages * B_PAGE_SIZE);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetSizeString(str, info.ignored_pages * B_PAGE_SIZE);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetSizeString(str, info.needed_memory);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		uint64 totalMemory = info.max_pages * B_PAGE_SIZE;
		GetUsedMaxSize(str, totalMemory - info.free_memory, totalMemory);
		view->RowAt(rowId++)->SetField(new BStringField(str), statValueCol);

		GetUsedMaxSize(str, (info.max_swap_pages - info.free_swap_pages) * B_PAGE_SIZE, info.max_swap_pages * B_PAGE_SIZE);
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
	NewInfoRow(view, "Memory");
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


static BMessage *NewSetLayoutMsg(int32 layout)
{
	BMessage *msg = new BMessage(setLayoutMsg);
	msg->SetInt32("val", layout);
	return msg;
}

class TestWindow: public BWindow
{
private:
	BTabView *fTabView;
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
					.AddItem(new BMenuItem("Flat", NewSetLayoutMsg(flatLayout)))
					.AddItem(new BMenuItem("Tree", NewSetLayoutMsg(treeLayout)))
					.AddItem(new BMenuItem("Sessions and groups", NewSetLayoutMsg(sessionsLayout)))
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

		fTabView = new BTabView("tab_view", B_WIDTH_FROM_LABEL);
		fTabView->SetBorder(B_NO_BORDER);

		//tab = new BTab(); fTabView->AddTab(NewTeamsView("Apps"), tab);
		tab = new BTab(); fTabView->AddTab(fTeamsView = NewTeamsView(), tab);
		//tab = new BTab(); fTabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Services", B_FOLLOW_NONE), tab);
		//tab = new BTab(); fTabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Sockets", B_FOLLOW_NONE), tab);
		tab = new BTab(); fTabView->AddTab(fStatsView = NewStatsView(), tab);

		ListTeams(fTeamsView, fLayout);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(menuBar)
			.AddGroup(B_VERTICAL, 0)
				.Add(fTabView)
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
				if (team < B_OK) return;
				BPoint center((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2);
				OpenTeamWindow(team, center);
				return;
			}
			case updateMsg: {
				BTab *tab = fTabView->TabAt(fTabView->Selection());
				if (tab == NULL) return;
				BView *view = tab->View();
				if (view == fTeamsView)
					ListTeams(fTeamsView, fLayout);
				else if (view == fStatsView)
					ListStats(fStatsView);
				return;
			}
			case setLayoutMsg: {
				int32 layout;
				CheckRetVoid(msg->FindInt32("val", &layout));
				fLayout = (ViewLayout)layout;
				RelayoutTeams(fTeamsView, fLayout);
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
				}
				if (msg->FindPointer("invoker", &(void*&)invoker) >= B_OK) {
					delete invoker; invoker = NULL;
				}
				if (which != 1) return;
				CheckRetVoid(msg->FindInt32("team", &team));
				if (team < B_OK) return;
				Check(kill_team(team), "Can't terminate team.");
				return;
			}
			case suspendMsg: {
				team_id team = SelectedTeam();
				if (team < B_OK) return;
				CheckErrno(kill(team, SIGSTOP), "Can't suspend team.");
				return;
			}
			case resumeMsg: {
				team_id team = SelectedTeam();
				if (team < B_OK) return;
				CheckErrno(kill(team, SIGCONT), "Can't resume team.");
				return;
			}
			case sendSignalMsg: {
				int32 signal;
				CheckRetVoid(msg->FindInt32("val", &signal));
				team_id team = SelectedTeam();
				if (team < B_OK) return;
				CheckErrno(kill(team, signal), "Can't send signal.");
				return;
			}
			case showLocationMsg: {
				BRow *row = fTeamsView->CurrentSelection(NULL);
				if (row == NULL) return;
				ShowLocation(((BStringField*)row->GetField(pathCol))->String());
			}
			}
		} catch (StatusError &err) {
			ShowError(err);
		}
		BWindow::MessageReceived(msg);
	}
};


static property_info gProperties[] = {
	{"Team", {B_EXECUTE_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}},
	{"Image", {B_EXECUTE_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}},
	{"Thread", {B_EXECUTE_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}},
	{"Area", {B_EXECUTE_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}},
	{"Port", {B_EXECUTE_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}},
	{"Sem", {B_EXECUTE_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0}},
	{0}
};

class TestApplication: public BApplication
{
private:
	BWindow *fWnd;

public:
	TestApplication(): BApplication("application/x-vnd.Test-SystemManager")
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 640, 480));
		fWnd->SetFlags(fWnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		fWnd->CenterOnScreen();
		fWnd->Show();
	}
	
	BHandler *ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier, int32 what, const char* property)
	{
		printf("TestApplication::ResolveSpecifier()\n");
		BPropertyInfo propInfo(gProperties);
		printf("specifier: "); specifier->PrintToStream();
		if (propInfo.FindMatch(message, 0, specifier, what, property) >= 0)
			return this;
		return BApplication::ResolveSpecifier(message, index, specifier, what, property);
	}
	
	status_t GetSupportedSuites(BMessage *data)
	{
		printf("TestApplication::GetSupportedSuites()\n");
		if (data == NULL) return B_BAD_VALUE;
		CheckRet(data->AddString("suites", "suite/vnd.Test-SystemMangager"));
		BPropertyInfo propertyInfo(gProperties);
		CheckRet(data->AddFlat("messages", &propertyInfo));
		return BApplication::GetSupportedSuites(data);
	}
	
	status_t HandleScript(BMessage &message, BMessage &reply, int32 index, BMessage &specifier, int32 what, const char* property)
	{
		BPropertyInfo propInfo(gProperties);
		int32 propIdx = propInfo.FindMatch(&message, index, &specifier, what, property);
		switch (propIdx) {
		case 0: // Team: Execute
		case 1: // Image: Execute
		case 2: // Thread: Execute
		case 3: // Area: Execute
		case 4: // Port: Execute
		case 5: // Sem: Execute
			switch (what) {
			case B_INDEX_SPECIFIER: {
				CheckRet(specifier.FindInt32("index", &index));
				BRect frame = fWnd->Frame();
				BPoint center((frame.left + frame.right)/2, (frame.top + frame.bottom)/2);
				switch (propIdx) {
				case 0: {
					OpenTeamWindow(index, center);
					return B_OK;
				}
				case 2: {
					thread_info info;
					CheckRet(get_thread_info(index, &info));
					BWindow *wnd = OpenTeamWindow(info.team, center);
					BMessage wndMsg(teamWindowShowThreadMsg);
					wndMsg.AddInt32("val", index);
					BMessenger(wnd).SendMessage(&wndMsg);
					return B_OK;
				}
				case 3: {
					area_info info;
					CheckRet(get_area_info(index, &info));
					BWindow *wnd = OpenTeamWindow(info.team, center);
					BMessage wndMsg(teamWindowShowAreaMsg);
					wndMsg.AddInt32("val", index);
					BMessenger(wnd).SendMessage(&wndMsg);
					return B_OK;
				}
				case 4: {
					port_info info;
					CheckRet(get_port_info(index, &info));
					BWindow *wnd = OpenTeamWindow(info.team, center);
					BMessage wndMsg(teamWindowShowPortMsg);
					wndMsg.AddInt32("val", index);
					BMessenger(wnd).SendMessage(&wndMsg);
					return B_OK;
				}
				case 5: {
					sem_info info;
					CheckRet(get_sem_info(index, &info));
					BWindow *wnd = OpenTeamWindow(info.team, center);
					BMessage wndMsg(teamWindowShowSemMsg);
					wndMsg.AddInt32("val", index);
					BMessenger(wnd).SendMessage(&wndMsg);
					return B_OK;
				}
				}
			}
			}
			break;
		}
		return B_BAD_SCRIPT_SYNTAX;
	}
	
	void MessageReceived(BMessage *msg)
	{
		int32 index;
		BMessage specifier;
		int32 what;
		const char* property;

		if (msg->HasSpecifiers() && msg->GetCurrentSpecifier(&index, &specifier, &what, &property) >= B_OK) {
			BMessage reply(B_REPLY);
			status_t res = HandleScript(*msg, reply, index, specifier, what, property);
			if (res != B_OK) {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddString("message", strerror(res));
			}
			reply.AddInt32("error", res);
			msg->SendReply(&reply);
			return;
		}

		switch (msg->what) {
		case B_SILENT_RELAUNCH:
			fWnd->Activate();
			return;
		}

		BApplication::MessageReceived(msg);
	}
	
};


int main()
{
	(new TestApplication())->Run();
	return 0;
}
