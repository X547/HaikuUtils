#include "StackWindow.h"

#include <stdio.h>
#include <errno.h>
#include <libgen.h>

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

#include <private/debug/debug_support.h>

#include <cxxabi.h>
#include <map>

#include "Errors.h"
#include "UIUtils.h"


enum {
	frameFpCol = 0,
	frameIpCol,
	frameImageCol,
	frameFunctionCol,
};


static char *CppDemangle(const char *abiName)
{
  int status;
  char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);
  return ret;
}

static void LookupSymbolAddress(
	StackWindow *wnd,
	debug_symbol_lookup_context *lookupContext, const void *address,
	BString &imageStr, BString &symbolStr)
{
	// lookup the symbol
	void *baseAddress;
	char symbolName[1024];
	char imagePath[B_PATH_NAME_LENGTH], imageNameBuf[B_PATH_NAME_LENGTH], *imageName;
	bool exactMatch;
	bool lookupSucceeded = false;
	if (lookupContext) {
		status_t error = debug_lookup_symbol_address(lookupContext, address,
			&baseAddress, symbolName, sizeof(symbolName), imagePath,
			sizeof(imagePath), &exactMatch);
		lookupSucceeded = (error == B_OK);
	}

	if (lookupSucceeded) {
		strcpy(imageNameBuf, imagePath);
		imageName = basename(imageNameBuf);

		// we were able to look something up
		if (strlen(symbolName) > 0) {
			char *demangledName = CppDemangle(symbolName);
			if (demangledName != NULL) {
				strcpy(symbolName, demangledName);
				free(demangledName);
			}

			// we even got a symbol
			imageStr.SetTo(imageName);
			symbolStr.SetToFormat("%s + %ld%s",
				symbolName,
				(addr_t)address - (addr_t)baseAddress,
				exactMatch ? "" : " (closest symbol)"
			);
		} else {
			// no symbol: image relative address
			imageStr.SetTo(imageName);
			symbolStr.SetToFormat("%ld", (addr_t)address - (addr_t)baseAddress);
		}

	} else {
		// lookup failed: find area containing the IP
		bool useAreaInfo = false;
		area_info info;
		ssize_t cookie = 0;
		while (get_next_area_info(wnd->fTeam, &cookie, &info) == B_OK) {
			if ((addr_t)info.address <= (addr_t)address &&
				(addr_t)info.address + info.size > (addr_t)address
			) {
				useAreaInfo = true;
				break;
			}
		}

		if (useAreaInfo) {
			imageStr.SetToFormat("<area: %s>", info.name);
			symbolStr.SetToFormat("%#lx", (addr_t)address - (addr_t)info.address);
		} else {
			imageStr.SetTo("");
			symbolStr.SetTo("");
		}
	}
}

static void WriteStackTrace(StackWindow *wnd)
{
	status_t error;
	void *ip = NULL, *fp = NULL;

	error = debug_get_instruction_pointer(&wnd->fDebugContext, wnd->fId, &ip, &fp);

	debug_symbol_lookup_context *lookupContext = NULL;
	Check(debug_create_symbol_lookup_context(wnd->fTeam, -1, &lookupContext), "can't create symbol lookup context", false);

	BString imageStr, symbolStr;
	LookupSymbolAddress(wnd, lookupContext, ip, imageStr, symbolStr);

	BRow *row;
	BString str;

	row = new BRow();
	row->SetField(new Int64Field((addr_t)fp), frameFpCol);
	row->SetField(new Int64Field((addr_t)ip), frameIpCol);
	row->SetField(new BStringField(imageStr), frameImageCol);
	row->SetField(new BStringField(symbolStr), frameFunctionCol);
	wnd->fView->AddRow(row);

	for (int32 i = 0; i < 400; i++) {
		debug_stack_frame_info stackFrameInfo;

		error = debug_get_stack_frame(&wnd->fDebugContext, fp, &stackFrameInfo);
		if (error < B_OK)
			break;

		ip = stackFrameInfo.return_address;
		fp = stackFrameInfo.parent_frame;

		LookupSymbolAddress(wnd, lookupContext, ip, imageStr, symbolStr);

		row = new BRow();
		row->SetField(new Int64Field((addr_t)fp), frameFpCol);
		row->SetField(new Int64Field((addr_t)ip), frameIpCol);
		row->SetField(new BStringField(imageStr), frameImageCol);
		row->SetField(new BStringField(symbolStr), frameFunctionCol);
		wnd->fView->AddRow(row);

		if (fp == NULL)
			break;
	}

	if (lookupContext)
		debug_delete_symbol_lookup_context(lookupContext);
}

static status_t DebugThread(void *arg)
{
	StackWindow *wnd = (StackWindow*)arg;
	status_t res;
	bool run = true;
	try {
		while (run) {
			int32 code;
			debug_debugger_message_data message;
			res = read_port(wnd->fDebuggerPort, &code, &message, sizeof(message));
			if (res == B_INTERRUPTED) continue;
			Check(res, "read port failed");
			// printf("debug msg: %d\n", code);
			switch (code) {
			case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: {
				run = false;
				break;
			}
			}
		}
	} catch (StatusError &err) {
		ShowError(err);
		return err.res;
	}
	return B_OK;
}


static void ListFrames(StackWindow *wnd, BColumnListView *view)
{
	thread_info threadInfo;
	status_t res;

	Check(get_thread_info(wnd->fId, &threadInfo), "thread not found");
	wnd->fTeam = threadInfo.team;

	wnd->fDebuggerPort = Check(create_port(10, "debugger port"));
	HandleDeleter<port_id, status_t, delete_port> portDeleter(wnd->fDebuggerPort);

	wnd->fNubPort = Check(install_team_debugger(wnd->fTeam, wnd->fDebuggerPort), "can't install debugger");
	HandleDeleter<team_id, status_t, remove_team_debugger> teamDebuggerDeleter(wnd->fTeam);

	Check(init_debug_context(&wnd->fDebugContext, wnd->fTeam, wnd->fNubPort));
	CObjectDeleter<debug_context, void, destroy_debug_context> debugContextDeleter(&wnd->fDebugContext);

	Check(debug_thread(wnd->fId));

	thread_id debugThread = spawn_thread(DebugThread, "debug thread", B_NORMAL_PRIORITY, wnd);
	wait_for_thread(debugThread, &res); Check(res);

	WriteStackTrace(wnd);
}

static void NewFramesView(StackWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Frames", B_NAVIGABLE);
	view->AddColumn(new HexIntegerColumn("FP", 128, 50, 500, B_ALIGN_RIGHT), frameFpCol);
	view->AddColumn(new HexIntegerColumn("IP", 128, 50, 500, B_ALIGN_RIGHT), frameIpCol);
	view->AddColumn(new BStringColumn("Image", 150, 50, 500, B_TRUNCATE_END), frameImageCol);
	view->AddColumn(new BStringColumn("Function", 512, 50, 1024, B_TRUNCATE_END), frameFunctionCol);
	wnd->fView = view;
	ListFrames(wnd, view);
}


std::map<team_id, StackWindow*> stacks;
BLocker stacksLocker;

void OpenStackWindow(team_id id, BPoint center)
{
	AutoLocker<BLocker> locker(stacksLocker);
	auto it = stacks.find(id);
	if (it != stacks.end()) {
		it->second->Activate();
	} else {
		StackWindow *wnd = new StackWindow(id);
		stacks[id] = wnd;
		BRect frame = wnd->Frame();
		wnd->MoveTo(center.x - frame.Width()/2, center.y - frame.Height()/2);
		wnd->Show();
	}
}

StackWindow::StackWindow(thread_id id): BWindow(BRect(0, 0, 800, 480), "Thread", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fId(id)
{
	BMenuBar *menuBar;

	BString title;
	title.SetToFormat("Thread %" B_PRId32, fId);
	SetTitle(title.String());

	menuBar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
	BLayoutBuilder::Menu<>(menuBar)
		.AddMenu(new BMenu("File"))
			.AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'))
			.End()
		.End()
	;

	try {
		NewFramesView(this);
	} catch (StatusError &err) {
		ShowError(err);
		PostMessage(B_QUIT_REQUESTED);
	}

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, 0)
			.Add(fView)
			.SetInsets(-1)
			.End()
		.End()
	;
}

StackWindow::~StackWindow()
{
	printf("-StackWindow\n");
	AutoLocker<BLocker> locker(stacksLocker);
	stacks.erase(fId);
}
