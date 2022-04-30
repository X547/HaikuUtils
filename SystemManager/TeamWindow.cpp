#include "TeamWindow.h"

#include <Application.h>
#include <View.h>
#include <TabView.h>
#include <Rect.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Box.h>
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/shared/AutoLocker.h>
#include <String.h>
#include <image.h>
#include <private/kernel/util/KMessage.h>
#include <private/system/extended_system_info_defs.h>
#include <private/libroot/extended_system_info.h>
#include <drivers/KernelExport.h>
#include <fs_info.h>
#include <private/system/syscalls.h>
#include <private/system/vfs_defs.h>

#include <Entry.h>
#include <Path.h>

#include <stdio.h>
#include <map>

#include "StackWindow.h"
#include "Errors.h"
#include "Utils.h"
#include "UIUtils.h"


enum {
	imageIdCol = 0,
	imageTypeCol,
	imageTextCol,
	imageDataCol,
	imageNameCol,
	imagePathCol,
};

enum {
	threadIdCol = 0,
	threadNameCol,
	threadStateCol,
	threadPriorityCol,
	threadSemCol,
	threadUserTimeCol,
	threadKernelTimeCol,
	threadStackBaseCol,
	threadStackEndCol,
};

enum {
	areaIdCol = 0,
	areaNameCol,
	areaAdrCol,
	areaSizeCol,
	areaAllocCol,
	areaProtCol,
	areaLockCol,
};

enum {
	portIdCol = 0,
	portNameCol,
	portCapacityCol,
	portQueuedCol,
	portTotalCol,
};

enum {
	semIdCol = 0,
	semNameCol,
	semCountCol,
	semLatestHolderCol,
};

enum {
	fileNameCol,
	fileIdCol,
	fileModeCol,
	fileDevCol,
	fileNodeCol,
	fileDevNameCol,
	fileVolNameCol,
	fileFsNameCol
};

enum {
	infoNameCol = 0,
	infoValueCol,
};

enum {
	updateMsg = teamWindowPrivateMsgBase,
	infoShowWorkDirMsg,
	imagesShowLocationMsg,
	threadsInvokeMsg,
	threadsTerminateMsg,
	threadsSendSignalMsg,
	threadsShowSemMsg,
	threadsShowStackAreaMsg,
	semsShowThreadMsg,
};


static BRow *FindIntRow(BColumnListView *view, int32 col, BRow *parent, int32 val)
{
	BRow *row;
	BString name;
	for (int32 i = 0; i < view->CountRows(parent); i++) {
		row = view->RowAt(i, parent);
		if (((BIntegerField*)row->GetField(col))->Value() == val)
			return row;
	}
	return NULL;
}


//#pragma mark Lists

static void ListInfo(TeamWindow *wnd, BColumnListView *view)
{
	int32 rowId = 0;
	int32 int32Val = -1;
	BString str;
	const char *strPtr;
	int32 uid = -1, gid = -1;

	KMessage extInfo;

	if (get_extended_team_info(wnd->fId, B_TEAM_INFO_BASIC, extInfo) >= B_OK) {
		if (extInfo.FindInt32("id", &int32Val) >= B_OK) {
			str.SetToFormat("%" B_PRId32, int32Val);
			view->RowAt(rowId++)->SetField(new BStringField(str), infoValueCol);
		}
		if (extInfo.FindString("name", &strPtr) >= B_OK) {
			view->RowAt(rowId++)->SetField(new BStringField(strPtr), infoValueCol);
		}

		if (extInfo.FindInt32("process group", &int32Val) >= B_OK) {
			str.SetToFormat("%" B_PRId32, int32Val);
			view->RowAt(rowId++)->SetField(new BStringField(str), infoValueCol);
		}

		if (extInfo.FindInt32("session", &int32Val) >= B_OK) {
			str.SetToFormat("%" B_PRId32, int32Val);
			view->RowAt(rowId++)->SetField(new BStringField(str), infoValueCol);
		}

		if (extInfo.FindInt32("uid", &uid) < B_OK) uid = -1;
		if (extInfo.FindInt32("gid", &gid) < B_OK) gid = -1;
		GetUserGroupString(str, uid, gid, true);
		view->RowAt(rowId++)->SetField(new BStringField(str), infoValueCol);

		if (extInfo.FindInt32("euid", &uid) < B_OK) uid = -1;
		if (extInfo.FindInt32("egid", &gid) < B_OK) gid = -1;
		GetUserGroupString(str, uid, gid, true);
		view->RowAt(rowId++)->SetField(new BStringField(str), infoValueCol);

		entry_ref ref;
		if (
			extInfo.FindInt32("cwd device", &ref.device) >= B_OK &&
			extInfo.FindInt64("cwd directory", &ref.directory) >= B_OK
		) {
			ref.set_name(".");
			BPath path(&ref);
			view->RowAt(rowId++)->SetField(new BStringField(path.Path()), infoValueCol);
		}
	}
}

static void NewInfoRow(BColumnListView *view, const char *name)
{
	BRow *row = new BRow();
	row->SetField(new BStringField(name), infoNameCol);
	view->AddRow(row);
}

static BColumnListView *NewInfoView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Info", B_NAVIGABLE);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), infoNameCol);
	view->AddColumn(new BStringColumn("Value", 512, 50, 1024, B_TRUNCATE_END), infoValueCol);

	NewInfoRow(view, "ID");
	NewInfoRow(view, "Name");
	NewInfoRow(view, "Process group");
	NewInfoRow(view, "Session");
	NewInfoRow(view, "User");
	NewInfoRow(view, "Effective user");
	NewInfoRow(view, "Working directory");

	ListInfo(wnd, view);
	return view;
}


static void ListImages(TeamWindow *wnd, BColumnListView *view)
{
	int32 cookie = 0;
	image_info info;
	BString str;
	BRow *row;
	BList prevRows;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(imageIdCol))->Value()));
	}

	while (get_next_image_info(wnd->fId, &cookie, &info) >= B_OK) {
		prevRows.RemoveItem((void*)(addr_t)info.id);
		row = FindIntRow(view, imageIdCol, NULL, info.id);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}
		
		BString imageType;
		switch (info.type) {
			case B_APP_IMAGE: imageType.SetTo("app"); break;
			case B_LIBRARY_IMAGE: imageType.SetTo("lib"); break;
			case B_ADD_ON_IMAGE: imageType.SetTo("addon"); break;
			case B_SYSTEM_IMAGE: imageType.SetTo("sys"); break;
			default: imageType.SetToFormat("?(%d)", info.type);
		}

		row->SetField(new BIntegerField(info.id), imageIdCol);
		row->SetField(new BStringField(imageType), imageTypeCol);
		row->SetField(new Int64Field((uintptr_t)info.text), imageTextCol);
		row->SetField(new Int64Field((uintptr_t)info.data), imageDataCol);
		row->SetField(new BStringField(GetFileName(info.name)), imageNameCol);
		row->SetField(new BStringField(info.name), imagePathCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, imageIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewImagesView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Images", B_NAVIGABLE);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), imageIdCol);
	view->AddColumn(new BStringColumn("Type", 56, 32, 96, B_ALIGN_LEFT), imageTypeCol);
	view->AddColumn(new HexIntegerColumn("Text", 128, 50, 500, B_ALIGN_RIGHT), imageTextCol);
	view->AddColumn(new HexIntegerColumn("Data", 128, 50, 500, B_ALIGN_RIGHT), imageDataCol);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), imageNameCol);
	view->AddColumn(new BStringColumn("Path", 500, 50, 1000, B_TRUNCATE_MIDDLE), imagePathCol);
	ListImages(wnd, view);
	return view;
}

static void ListThreads(TeamWindow *wnd, BColumnListView *view)
{
	int32 cookie = 0;
	thread_info info;
	BString str;
	BRow *row;
	BList prevRows;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(threadIdCol))->Value()));
	}

	while (get_next_thread_info(wnd->fId, &cookie, &info) >= B_OK) {
		prevRows.RemoveItem((void*)(addr_t)info.thread);
		row = FindIntRow(view, threadIdCol, NULL, info.thread);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BIntegerField(info.thread), threadIdCol);
		row->SetField(new BStringField(info.name), threadNameCol);

		switch (info.state) {
		case B_THREAD_RUNNING: str = "running"; break;
		case B_THREAD_READY: str = "ready"; break;
		case B_THREAD_RECEIVING: str = "receiving"; break;
		case B_THREAD_ASLEEP: str = "asleep"; break;
		case B_THREAD_SUSPENDED: str = "suspended"; break;
		case B_THREAD_WAITING: str = "waiting"; break;
		default:
			str.SetToFormat("? (%d)", info.state);
		}
		row->SetField(new BStringField(str), threadStateCol);

		row->SetField(new BIntegerField(info.priority), threadPriorityCol);
		GetSemString(str, info.sem);
		row->SetField(new BStringField(str), threadSemCol);
		row->SetField(new BIntegerField(info.user_time), threadUserTimeCol);
		row->SetField(new BIntegerField(info.kernel_time), threadKernelTimeCol);
		row->SetField(new Int64Field((addr_t)info.stack_base), threadStackBaseCol);
		row->SetField(new Int64Field((addr_t)info.stack_end), threadStackEndCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, threadIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewThreadsView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Threads", B_NAVIGABLE);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), threadIdCol);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), threadNameCol);
	view->AddColumn(new BStringColumn("State", 150, 50, 500, B_TRUNCATE_END), threadStateCol);
	view->AddColumn(new BIntegerColumn("Priority", 96, 32, 128, B_ALIGN_RIGHT), threadPriorityCol);
	view->AddColumn(new BStringColumn("Sem", 96, 32, 512, B_TRUNCATE_END), threadSemCol);
	view->AddColumn(new BIntegerColumn("User time", 96, 32, 128, B_ALIGN_RIGHT), threadUserTimeCol);
	view->AddColumn(new BIntegerColumn("Kernel time", 96, 32, 128, B_ALIGN_RIGHT), threadKernelTimeCol);
	view->AddColumn(new HexIntegerColumn("Stack base", 128, 50, 500, B_ALIGN_RIGHT), threadStackBaseCol);
	view->AddColumn(new HexIntegerColumn("Stack end", 128, 50, 500, B_ALIGN_RIGHT), threadStackEndCol);
	view->SetInvocationMessage(new BMessage(threadsInvokeMsg));
	ListThreads(wnd, view);
	return view;
}

static void ListAreas(TeamWindow *wnd, BColumnListView *view)
{
	ssize_t cookie = 0;
	area_info info;
	BString str, str2;
	BRow *row;
	BList prevRows;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(areaIdCol))->Value()));
	}

	while (get_next_area_info(wnd->fId, &cookie, &info) >= B_OK) {
		prevRows.RemoveItem((void*)(addr_t)info.area);
		row = FindIntRow(view, areaIdCol, NULL, info.area);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BIntegerField(info.area), areaIdCol);
		row->SetField(new BStringField(info.name), areaNameCol);
		row->SetField(new Int64Field((addr_t)info.address), areaAdrCol);
		row->SetField(new Int64Field(info.size), areaSizeCol);
		row->SetField(new Int64Field(info.ram_size), areaAllocCol);

		str = "";
		if (B_READ_AREA & info.protection) str += "R";
		if (B_WRITE_AREA & info.protection) str += "W";
		if (B_EXECUTE_AREA & info.protection) str += "X";
		if (B_STACK_AREA & info.protection) str += "S";
		if (B_KERNEL_READ_AREA & info.protection) str += "r";
		if (B_KERNEL_WRITE_AREA & info.protection) str += "w";
		if (B_KERNEL_EXECUTE_AREA & info.protection) str += "x";
		if (B_KERNEL_STACK_AREA & info.protection) str += "s";
		if (B_CLONEABLE_AREA & info.protection) str += "C";
		row->SetField(new BStringField(str), areaProtCol);

		switch (info.lock) {
		case B_NO_LOCK: str = "no"; break;
		case B_LAZY_LOCK: str = "lazy"; break;
		case B_FULL_LOCK: str = "full"; break;
		case B_CONTIGUOUS: str = "contiguous"; break;
		case B_LOMEM: str = "lomem"; break;
		case B_32_BIT_FULL_LOCK: str = "32 bit full"; break;
		case B_32_BIT_CONTIGUOUS: str = "32 bit contiguous"; break;
		case 7: str = "already wired"; break;
		default:
			str.SetToFormat("? (%" B_PRIu32 ")", info.lock);
		}
		row->SetField(new BStringField(str), areaLockCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, areaIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewAreasView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Areas", B_NAVIGABLE);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), areaIdCol);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), areaNameCol);
	view->AddColumn(new HexIntegerColumn("Address", 128, 50, 500, B_ALIGN_RIGHT), areaAdrCol);
	view->AddColumn(new HexIntegerColumn("Size", 128, 50, 500, B_ALIGN_RIGHT), areaSizeCol);
	view->AddColumn(new HexIntegerColumn("Alloc", 128, 50, 500, B_ALIGN_RIGHT), areaAllocCol);
	view->AddColumn(new BStringColumn("Prot", 64, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), areaProtCol);
	view->AddColumn(new BStringColumn("lock", 64, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), areaLockCol);
	ListAreas(wnd, view);
	return view;
}

static void ListPorts(TeamWindow *wnd, BColumnListView *view)
{
	int32 cookie = 0;
	port_info info;
	BString str, str2;
	BRow *row;
	BList prevRows;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(portIdCol))->Value()));
	}

	while (get_next_port_info(wnd->fId, &cookie, &info) >= B_OK) {
		prevRows.RemoveItem((void*)(addr_t)info.port);
		row = FindIntRow(view, portIdCol, NULL, info.port);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BIntegerField(info.port), portIdCol);
		row->SetField(new BStringField(info.name), portNameCol);
		row->SetField(new BIntegerField(info.capacity), portCapacityCol);
		row->SetField(new BIntegerField(info.queue_count), portQueuedCol);
		row->SetField(new BIntegerField(info.total_count), portTotalCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, portIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewPortsView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Ports", B_NAVIGABLE);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), portIdCol);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), portNameCol);
	view->AddColumn(new BIntegerColumn("Capacity", 64, 32, 128, B_ALIGN_RIGHT), portCapacityCol);
	view->AddColumn(new BIntegerColumn("Queued", 64, 32, 128, B_ALIGN_RIGHT), portQueuedCol);
	view->AddColumn(new BIntegerColumn("Total", 96, 32, 256, B_ALIGN_RIGHT), portTotalCol);
	ListPorts(wnd, view);
	return view;
}

static void ListSems(TeamWindow *wnd, BColumnListView *view)
{
	int32 cookie = 0;
	sem_info info;
	BRow *row;
	BList prevRows;
	BString str;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(semIdCol))->Value()));
	}

	while (get_next_sem_info(wnd->fId, &cookie, &info) >= B_OK) {
		prevRows.RemoveItem((void*)(addr_t)info.sem);
		row = FindIntRow(view, semIdCol, NULL, info.sem);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BIntegerField(info.sem), semIdCol);
		row->SetField(new BStringField(info.name), semNameCol);
		row->SetField(new BIntegerField(info.count), semCountCol);
		GetThreadString(str, info.latest_holder);
		row->SetField(new BStringField(str), semLatestHolderCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, semIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewSemsView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Semaphores", B_NAVIGABLE);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), semIdCol);
	view->AddColumn(new BStringColumn("Name", 150, 50, 500, B_TRUNCATE_END), semNameCol);
	view->AddColumn(new BIntegerColumn("Count", 64, 32, 128, B_ALIGN_RIGHT), semCountCol);
	view->AddColumn(new BStringColumn("Latest holder", 96, 32, 1024, B_TRUNCATE_END), semLatestHolderCol);
	ListSems(wnd, view);
	return view;
}

static void ListFiles(TeamWindow *wnd, BColumnListView *view)
{
	uint32 cookie = 0;
	fd_info info;
	fs_info fsInfo;
	BRow *row;
	BList prevRows;
	BString buf;
	char path[B_OS_NAME_LENGTH];

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(fileIdCol))->Value()));
	}

	while (_kern_get_next_fd_info(wnd->fId, &cookie, &info, sizeof(fd_info)) >= B_OK) {
		prevRows.RemoveItem((void*)(addr_t)info.number);
		row = FindIntRow(view, fileIdCol, NULL, info.number);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		if (_kern_entry_ref_to_path(info.device, info.node, NULL, path, B_OS_NAME_LENGTH) == B_OK)
			row->SetField(new BStringField(path), fileNameCol);
		else
			row->SetField(new BStringField("?"), fileNameCol);

		row->SetField(new BIntegerField(info.number), fileIdCol);

		if ((info.open_mode & O_RWMASK) == O_RDONLY) {buf = "R";}
		if ((info.open_mode & O_RWMASK) == O_WRONLY) {buf = "W";}
		if ((info.open_mode & O_RWMASK) == O_RDWR  ) {buf = "RW";}
		row->SetField(new BStringField(buf), fileModeCol);
		row->SetField(new BIntegerField(info.device), fileDevCol);
		row->SetField(new BIntegerField(info.node), fileNodeCol);

		fs_stat_dev(info.device, &fsInfo);
		row->SetField(new BStringField(fsInfo.device_name), fileDevNameCol);
		row->SetField(new BStringField(fsInfo.volume_name), fileVolNameCol);
		row->SetField(new BStringField(fsInfo.fsh_name), fileFsNameCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, fileIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewFilesView(TeamWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Files", B_NAVIGABLE);
	view->AddColumn(new BStringColumn("Name", 250, 50, 512, B_TRUNCATE_MIDDLE), fileNameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), fileIdCol);
	view->AddColumn(new BStringColumn("Mode", 48, 32, 128, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), fileModeCol);
	view->AddColumn(new BIntegerColumn("Dev", 64, 32, 128, B_ALIGN_RIGHT), fileDevCol);
	view->AddColumn(new BIntegerColumn("Node", 64, 32, 128, B_ALIGN_RIGHT), fileNodeCol);
	view->AddColumn(new BStringColumn("Device", 96, 32, 512, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), fileDevNameCol);
	view->AddColumn(new BStringColumn("Volume", 96, 32, 512, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), fileVolNameCol);
	view->AddColumn(new BStringColumn("FS", 96, 32, 512, B_TRUNCATE_MIDDLE, B_ALIGN_LEFT), fileFsNameCol);
	ListFiles(wnd, view);
	return view;
}


class TabView: public BTabView
{
private:
	TeamWindow *fWnd;

public:
	TabView(TeamWindow *wnd): BTabView("tabView", B_WIDTH_FROM_LABEL), fWnd(wnd) {}
	
	void Select(int32 index)
	{
		BTabView::Select(index);
		fWnd->TabChanged();
	}
	
};


//#pragma mark TeamWindow

std::map<team_id, TeamWindow*> teamWindows;
BLocker teamWindowsLocker;

TeamWindow *OpenTeamWindow(team_id id, BPoint center)
{
	AutoLocker<BLocker> locker(teamWindowsLocker);
	auto it = teamWindows.find(id);
	if (it != teamWindows.end()) {
		it->second->Activate();
		return it->second;
	} else {
		TeamWindow *wnd = new TeamWindow(id);
		teamWindows[id] = wnd;
		BRect frame = wnd->Frame();
		wnd->MoveTo(center.x - frame.Width()/2, center.y - frame.Height()/2);
		wnd->Show();
		return wnd;
	}
}

static BMessage *NewSignalMsg(int32 signal)
{
	BMessage *msg = new BMessage(threadsSendSignalMsg);
	msg->AddInt32("val", signal);
	return msg;
}

TeamWindow::TeamWindow(team_id id): BWindow(BRect(0, 0, 800, 480), "Team", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fId(id),
	fListUpdater(BMessenger(this), BMessage(updateMsg), 500000),
	fCurMenu(NULL)
{
	BTab *tab;
	BMenu *menu;

	try {
		int32 cookie = 0;
		image_info imageInfo;
		Check(get_next_image_info(fId, &cookie, &imageInfo), "Invalid team id.");
		BString title;
		title.SetToFormat("%s (%" B_PRId32 ")", GetFileName(imageInfo.name), fId);
		SetTitle(title.String());
	} catch (StatusError &err) {
		ShowError(err);
		PostMessage(B_QUIT_REQUESTED);
	}

	fMenuBar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
	BLayoutBuilder::Menu<>(fMenuBar)
		.AddMenu(new BMenu("File"))
			.AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'))
			.End()
		.End()
	;

	menu = new BMenu("Info");
	BLayoutBuilder::Menu<>(menu)
		.AddItem(new BMenuItem("Show working directory", new BMessage(infoShowWorkDirMsg)))
		.End()
	;
	fInfoMenu = new BMenuItem(menu);

	menu = new BMenu("Image");
	BLayoutBuilder::Menu<>(menu)
		.AddItem(new BMenuItem("Show location", new BMessage(imagesShowLocationMsg)))
		.End()
	;
	fImagesMenu = new BMenuItem(menu);

	BMenu *signalMenu;
	menu = new BMenu("Thread");
	BLayoutBuilder::Menu<>(menu)
		.AddItem(new BMenuItem("Terminate", new BMessage(threadsTerminateMsg), 'T'))
		.AddItem(new BMenuItem("Suspend", NewSignalMsg(SIGSTOP)))
		.AddItem(new BMenuItem("Resume", NewSignalMsg(SIGCONT)))
		.AddMenu(signalMenu = new BMenu("Send signal"))
			.End()
		.AddSeparator()
		.AddItem(new BMenuItem("Show semaphore", new BMessage(threadsShowSemMsg)))
		.AddItem(new BMenuItem("Show stack area", new BMessage(threadsShowStackAreaMsg)))
		.End()
	;
	fThreadsMenu = new BMenuItem(menu);
	for (size_t i = 0; i < sizeof(signals)/sizeof(signals[0]); i++)
		signalMenu->AddItem(new BMenuItem(signals[i].name, NewSignalMsg(signals[i].val)));

	menu = new BMenu("Semaphore");
	BLayoutBuilder::Menu<>(menu)
		.AddItem(new BMenuItem("Show holder thread", new BMessage(semsShowThreadMsg)))
		.End()
	;
	fSemsMenu = new BMenuItem(menu);

	fTabView = new TabView(this);
	fTabView->SetBorder(B_NO_BORDER);

	tab = new BTab(); fTabView->AddTab(fInfoView = NewInfoView(this), tab);
	tab = new BTab(); fTabView->AddTab(fImagesView = NewImagesView(this), tab);
	tab = new BTab(); fTabView->AddTab(fThreadsView = NewThreadsView(this), tab);
	tab = new BTab(); fTabView->AddTab(fAreasView = NewAreasView(this), tab);
	tab = new BTab(); fTabView->AddTab(fPortsView = NewPortsView(this), tab);
	tab = new BTab(); fTabView->AddTab(fSemsView = NewSemsView(this), tab);
	tab = new BTab(); fTabView->AddTab(fFilesView = NewFilesView(this), tab);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.AddGroup(B_VERTICAL, 0)
			.Add(fTabView)
			.SetInsets(-1, 0, -1, -1)
			.End()
		.End()
	;

	TabChanged();
}

TeamWindow::~TeamWindow()
{
	printf("-TeamWindow\n");
	AutoLocker<BLocker> locker(teamWindowsLocker);
	teamWindows.erase(fId);
}

void TeamWindow::TabChanged()
{
	BMenuItem *newMenu = NULL;
	BTab *tab = fTabView->TabAt(fTabView->Selection());
	if (tab != NULL) {
		BView *view = tab->View();
		if (view == fInfoView)
			newMenu = fInfoMenu;
		else if (view == fImagesView)
			newMenu = fImagesMenu;
		else if (view == fThreadsView)
			newMenu = fThreadsMenu;
		else if (view == fSemsView)
			newMenu = fSemsMenu;
	}
	SetMenu(newMenu);
}

void TeamWindow::SetMenu(BMenuItem *menu)
{
	if (fCurMenu == menu) return;
	if (fCurMenu != NULL) fMenuBar->RemoveItem(fCurMenu);
	fCurMenu = menu;
	if (fCurMenu != NULL) fMenuBar->AddItem(fCurMenu);
}

void TeamWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case updateMsg: {
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab != NULL) {
			BView *view = tab->View();
			if (view == fInfoView)
				ListInfo(this, fInfoView);
			else if (view == fImagesView)
				ListImages(this, fImagesView);
			else if (view == fThreadsView)
				ListThreads(this, fThreadsView);
			else if (view == fAreasView)
				ListAreas(this, fAreasView);
			else if (view == fPortsView)
				ListPorts(this, fPortsView);
			else if (view == fSemsView)
				ListSems(this, fSemsView);
			else if (view == fFilesView)
				ListFiles(this, fFilesView);
		}
		return;
	}

	case infoShowWorkDirMsg: {
		BRow *row = fInfoView->RowAt(6); // !!!
		if (row == NULL) return;
		ShowLocation(((BStringField*)row->GetField(infoValueCol))->String());
		return;
	}
	case imagesShowLocationMsg: {
		BRow *row = fImagesView->CurrentSelection(NULL);
		if (row == NULL) return;
		ShowLocation(((BStringField*)row->GetField(imagePathCol))->String());
		return;
	}
	
	case threadsInvokeMsg: {
		BRow *row = fThreadsView->CurrentSelection(NULL);
		if (row == NULL) return;
		BPoint center((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2);
		OpenStackWindow(((BIntegerField*)row->GetField(threadIdCol))->Value(), center);
		return;
	}	
	case threadsTerminateMsg:
	case threadsSendSignalMsg: {
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab == NULL) return;
		BView *view = tab->View();
		if (view != fThreadsView) return;
		BRow *row = fThreadsView->CurrentSelection(NULL);
		if (row == NULL) return;
		thread_id thread = ((BIntegerField*)row->GetField(threadIdCol))->Value();
		if (thread < B_OK) return;
		switch (msg->what) {
		case threadsTerminateMsg:
			CheckErrno(kill_thread(thread), "Can't terminate thread.");
			break;
		case threadsSendSignalMsg:
			int32 signal;
			if (msg->FindInt32("val", &signal) < B_OK) return;
			CheckErrno(send_signal(thread, signal), "Can't send signal.");
			break;
		}
		return;
	}
	case threadsShowSemMsg: {
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab == NULL) return;
		BView *view = tab->View();
		if (view != fThreadsView) return;
		BRow *row = fThreadsView->CurrentSelection(NULL);
		if (row == NULL) return;
		sem_id sem = atoi(((BStringField*)row->GetField(threadSemCol))->String());
		if (sem < B_OK) return;
		BMessage showMsg(B_EXECUTE_PROPERTY);
		showMsg.AddSpecifier("Sem", sem);
		be_app_messenger.SendMessage(&showMsg);
		return;
	}
	case threadsShowStackAreaMsg: {
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab == NULL) return;
		BView *view = tab->View();
		if (view != fThreadsView) return;
		BRow *row = fThreadsView->CurrentSelection(NULL);
		if (row == NULL) return;
		int64 stackBase = ((Int64Field*)row->GetField(threadStackBaseCol))->Value();
		ListAreas(this, fAreasView);
		for (int32 i = 0; i < fAreasView->CountRows(); i++) {
			BRow *areaRow = fAreasView->RowAt(i);
			int64 areaAdr = ((Int64Field*)areaRow->GetField(areaAdrCol))->Value();
			int64 areaSize = ((Int64Field*)areaRow->GetField(areaSizeCol))->Value();
			if (stackBase >= areaAdr && stackBase < areaAdr + areaSize) {
				int32 area = ((BIntegerField*)areaRow->GetField(areaIdCol))->Value();
				BMessage showMsg(teamWindowShowAreaMsg);
				CheckRetVoid(showMsg.AddInt32("val", area));
				CheckRetVoid(showMsg.AddInt32("refresh", false));
				BMessenger(this).SendMessage(&showMsg);
			}
		}
	}
	case semsShowThreadMsg: {
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab == NULL) return;
		BView *view = tab->View();
		if (view != fSemsView) return;
		BRow *row = fSemsView->CurrentSelection(NULL);
		if (row == NULL) return;
		thread_id thread = atoi(((BStringField*)row->GetField(semLatestHolderCol))->String());
		if (thread < B_OK) return;
		BMessage showMsg(B_EXECUTE_PROPERTY);
		showMsg.AddSpecifier("Thread", thread);
		be_app_messenger.SendMessage(&showMsg);
		return;
	}

	case teamWindowShowImageMsg: {
		int32 id;
		if (msg->FindInt32("val", &id) < B_OK) return;
		fTabView->Select(1); // TODO: remove hard-coded constant
		ListImages(this, fImagesView);
		BRow *itemRow = FindIntRow(fImagesView, imageIdCol, NULL, id);
		if (itemRow == NULL) return;
		fImagesView->DeselectAll();
		fImagesView->SetFocusRow(itemRow, true);
		fImagesView->ScrollTo(itemRow);
		return;
	}
	case teamWindowShowThreadMsg: {
		int32 id;
		if (msg->FindInt32("val", &id) < B_OK) return;
		fTabView->Select(2); // TODO: remove hard-coded constant
		ListThreads(this, fThreadsView);
		BRow *itemRow = FindIntRow(fThreadsView, threadIdCol, NULL, id);
		if (itemRow == NULL) return;
		fThreadsView->DeselectAll();
		fThreadsView->SetFocusRow(itemRow, true);
		fThreadsView->ScrollTo(itemRow);
		return;
	}
	case teamWindowShowAreaMsg: {
		int32 id;
		bool refresh;
		if (msg->FindInt32("val", &id) < B_OK) return;
		if (msg->FindBool("refresh", &refresh) < B_OK) refresh = true;
		fTabView->Select(3); // TODO: remove hard-coded constant
		if (refresh) ListAreas(this, fAreasView);
		BRow *itemRow = FindIntRow(fAreasView, areaIdCol, NULL, id);
		if (itemRow == NULL) return;
		fAreasView->DeselectAll();
		fAreasView->SetFocusRow(itemRow, true);
		fAreasView->ScrollTo(itemRow);
		return;
	}
	case teamWindowShowPortMsg: {
		int32 id;
		if (msg->FindInt32("val", &id) < B_OK) return;
		fTabView->Select(4); // TODO: remove hard-coded constant
		ListPorts(this, fPortsView);
		BRow *itemRow = FindIntRow(fPortsView, portIdCol, NULL, id);
		if (itemRow == NULL) return;
		fPortsView->DeselectAll();
		fPortsView->SetFocusRow(itemRow, true);
		fPortsView->ScrollTo(itemRow);
		return;
	}
	case teamWindowShowSemMsg: {
		int32 id;
		if (msg->FindInt32("val", &id) < B_OK) return;
		fTabView->Select(5); // TODO: remove hard-coded constant
		ListSems(this, fSemsView);
		BRow *itemRow = FindIntRow(fSemsView, semIdCol, NULL, id);
		if (itemRow == NULL) return;
		fSemsView->DeselectAll();
		fSemsView->SetFocusRow(itemRow, true);
		fSemsView->ScrollTo(itemRow);
		return;
	}
	case teamWindowShowFileMsg: {
		int32 id;
		if (msg->FindInt32("val", &id) < B_OK) return;
		fTabView->Select(6); // TODO: remove hard-coded constant
		ListFiles(this, fFilesView);
		BRow *itemRow = FindIntRow(fFilesView, fileIdCol, NULL, id);
		if (itemRow == NULL) return;
		fFilesView->DeselectAll();
		fFilesView->SetFocusRow(itemRow, true);
		fFilesView->ScrollTo(itemRow);
		return;
	}
	}
	BWindow::MessageReceived(msg);
}
