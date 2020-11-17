#include "TeamWindow.h"

#include <Application.h>
#include <View.h>
#include <TabView.h>
#include <Rect.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/shared/AutoLocker.h>
#include <String.h>
#include <image.h>
#include <drivers/KernelExport.h>

#include <map>

#include "StackWindow.h"


enum {
	imageIdCol = 0,
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
	updateMsg = 1,
	threadsInvokeMsg,
};


class TestView: public BView
{
public:
	TestView(BRect frame, const char *name, uint32 resizingMode):
		BView(
			frame, name, resizingMode,
			B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_SUBPIXEL_PRECISE| B_FRAME_EVENTS
		)
	{
		this->SetViewColor(B_TRANSPARENT_COLOR);
	}

	BSize MinSize()
	{
		return BSize(3, 3);
	}

	void Draw(BRect dirty)
	{
		BRect rect(BPoint(0, 0), this->Frame().Size());
		rect.left += 1; rect.top += 1;
		this->PushState();
		this->SetHighColor(0x44, 0x44, 0x44);
		this->SetLowColor(0xff - 0x44, 0xff - 0x44, 0xff - 0x44);
		this->FillRect(rect, B_SOLID_LOW);
		this->SetPenSize(2);
		this->StrokeRect(rect, B_SOLID_HIGH);
		this->SetPenSize(1);
		this->StrokeLine(rect.LeftTop(), rect.RightBottom());
		this->StrokeLine(rect.RightTop(), rect.LeftBottom());
		this->PopState();
	}
};

static const char *GetFileName(const char *path)
{
	const char *name = path;
	for (const char *it = name; *it != '\0'; it++) {
		if (*it == '/') name = it + 1;
	}
	return name;
}

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

		row->SetField(new BIntegerField(info.id), imageIdCol);
		str.SetToFormat("0x%lx", (uintptr_t)info.text);
		row->SetField(new BStringField(str), imageTextCol);
		str.SetToFormat("0x%lx", (uintptr_t)info.data);
		row->SetField(new BStringField(str), imageDataCol);
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
	view->AddColumn(new BStringColumn("Text", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), imageTextCol);
	view->AddColumn(new BStringColumn("Data", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), imageDataCol);
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
			str.SetToFormat("? (%ld)", info.state);
		}
		row->SetField(new BStringField(str), threadStateCol);

		row->SetField(new BIntegerField(info.priority), threadPriorityCol);
		row->SetField(new BIntegerField(info.sem), threadSemCol);
		row->SetField(new BIntegerField(info.user_time), threadUserTimeCol);
		row->SetField(new BIntegerField(info.kernel_time), threadKernelTimeCol);
		str.SetToFormat("0x%lx", (addr_t)info.stack_base);
		row->SetField(new BStringField(str), threadStackBaseCol);
		str.SetToFormat("0x%lx", (addr_t)info.stack_end);
		row->SetField(new BStringField(str), threadStackEndCol);
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
	view->AddColumn(new BIntegerColumn("Sem", 96, 32, 128, B_ALIGN_RIGHT), threadSemCol);
	view->AddColumn(new BIntegerColumn("User time", 96, 32, 128, B_ALIGN_RIGHT), threadUserTimeCol);
	view->AddColumn(new BIntegerColumn("Kernel time", 96, 32, 128, B_ALIGN_RIGHT), threadKernelTimeCol);
	view->AddColumn(new BStringColumn("Stack base", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), threadStackBaseCol);
	view->AddColumn(new BStringColumn("Stack end", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), threadStackEndCol);
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

		str.SetToFormat("0x%lx", (uintptr_t)info.address);
		row->SetField(new BStringField(str), areaAdrCol);

		str.SetToFormat("0x%lx", info.size);
		row->SetField(new BStringField(str), areaSizeCol);

		str.SetToFormat("0x%lx", info.ram_size);
		row->SetField(new BStringField(str), areaAllocCol);

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
		default:
			str.SetToFormat("? (%ld)", info.lock);
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
	view->AddColumn(new BStringColumn("Address", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), areaAdrCol);
	view->AddColumn(new BStringColumn("Size", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), areaSizeCol);
	view->AddColumn(new BStringColumn("Alloc", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), areaAllocCol);
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
		row->SetField(new BIntegerField(info.latest_holder), semLatestHolderCol);
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
	view->AddColumn(new BIntegerColumn("Latest holder", 96, 32, 128, B_ALIGN_RIGHT), semLatestHolderCol);
	ListSems(wnd, view);
	return view;
}


std::map<team_id, TeamWindow*> teamWindows;
BLocker teamWindowsLocker;

void OpenTeamWindow(team_id id, BPoint center)
{
	AutoLocker<BLocker> locker(teamWindowsLocker);
	auto it = teamWindows.find(id);
	if (it != teamWindows.end()) {
		it->second->Activate();
	} else {
		TeamWindow *wnd = new TeamWindow(id);
		teamWindows[id] = wnd;
		BRect frame = wnd->Frame();
		wnd->MoveTo(center.x - frame.Width()/2, center.y - frame.Height()/2);
		wnd->Show();
	}
}

TeamWindow::TeamWindow(team_id id): BWindow(BRect(0, 0, 800, 480), "Team", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fId(id),
	fListUpdater(BMessenger(this), BMessage(updateMsg), 500000)
{
	BMenuBar *menuBar;
	BMenu *menu2;
	BMenuItem *it;
	BTab *tab;

	int32 cookie = 0;
	image_info imageInfo;
	if (get_next_image_info(fId, &cookie, &imageInfo) >= B_OK) {
		BString title;
		title.SetToFormat("%s (%ld)", GetFileName(imageInfo.name), fId);
		SetTitle(title.String());
	}

	menuBar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
	menu2 = new BMenu("File");
		menu2->AddItem(new BMenuItem("New", new BMessage('item'), 'N'));
		menu2->AddItem(new BMenuItem("Open...", new BMessage('item'), 'O'));
		menu2->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
		menu2->AddItem(new BSeparatorItem());
		menu2->AddItem(new BMenuItem("Save", new BMessage('item'), 'S'));
		menu2->AddItem(new BMenuItem("Save as...", new BMessage('item'), 'S', B_SHIFT_KEY));
		menu2->AddItem(new BSeparatorItem());
		menu2->AddItem(it = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q')); it->SetTarget(be_app);
		menuBar->AddItem(menu2);

	fTabView = new BTabView("tabView", B_WIDTH_FROM_LABEL);
	fTabView->SetBorder(B_NO_BORDER);

	tab = new BTab(); fTabView->AddTab(fImagesView = NewImagesView(this), tab);
	tab = new BTab(); fTabView->AddTab(fThreadsView = NewThreadsView(this), tab);
	tab = new BTab(); fTabView->AddTab(fAreasView = NewAreasView(this), tab);
	tab = new BTab(); fTabView->AddTab(fPortsView = NewPortsView(this), tab);
	tab = new BTab(); fTabView->AddTab(fSemsView = NewSemsView(this), tab);
	tab = new BTab(); fTabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Handles", B_FOLLOW_NONE), tab);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, 0)
			.Add(fTabView)
			.SetInsets(-1, 0, -1, -1)
			.End()
		.End()
	;
}

TeamWindow::~TeamWindow()
{
	printf("-TeamWindow\n");
	AutoLocker<BLocker> locker(teamWindowsLocker);
	teamWindows.erase(fId);
}


void TeamWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case updateMsg: {
		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab != NULL) {
			BView *view = tab->View();
			if (view == fImagesView)
				ListImages(this, fImagesView);
			else if (view == fThreadsView)
				ListThreads(this, fThreadsView);
			else if (view == fAreasView)
				ListAreas(this, fAreasView);
			else if (view == fPortsView)
				ListPorts(this, fPortsView);
			else if (view == fSemsView)
				ListSems(this, fSemsView);
		}
		break;
	}
	case threadsInvokeMsg: {
		BRow *row = fThreadsView->CurrentSelection(NULL);
		if (row != NULL) {
			OpenStackWindow(((BIntegerField*)row->GetField(threadIdCol))->Value(),
				BPoint((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2)
			);
		}
		break;
	}
	default:
		BWindow::MessageReceived(msg);
	}
}
