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


enum {
	frameFpCol = 0,
	frameIpCol,
	frameImageCol,
	frameFunctionCol,
};


static status_t Check(status_t res, const char *msg = NULL, bool fatal = true)
{
	if (res < B_OK) {
		if (fatal)
			fprintf(stderr, "fatal ");
		if (msg != NULL)
			fprintf(stderr, "error: %s (%s)\n", msg, strerror(res));
		else
			fprintf(stderr, "error: %s\n", strerror(res));
		if (fatal) exit(1);
	}
	return res;
}

static void FatalError(const char *msg)
{
	printf("Fatal error: %s\n", msg);
	exit(1);
}

static void WriteAddress2(BString &str, addr_t adr)
{
	str.SetToFormat("0x%lX", adr);
}

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
	Check(debug_create_symbol_lookup_context(wnd->fTeam, -1, &lookupContext), "can't create symbol lookup context");

	BString imageStr, symbolStr;
	LookupSymbolAddress(wnd, lookupContext, ip, imageStr, symbolStr);
	
	BRow *row;
	BString str;
	
	row = new BRow();
	WriteAddress2(str, (addr_t)fp); row->SetField(new BStringField(str), frameFpCol);
	WriteAddress2(str, (addr_t)ip); row->SetField(new BStringField(str), frameIpCol);
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
		WriteAddress2(str, (addr_t)fp); row->SetField(new BStringField(str), frameFpCol);
		WriteAddress2(str, (addr_t)ip); row->SetField(new BStringField(str), frameIpCol);
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
	while (run) {
		int32 code;
		debug_debugger_message_data message;
		res = read_port(wnd->fDebuggerPort, &code, &message, sizeof(message));
		if (res == B_INTERRUPTED) continue;
		Check(res, "read port failed");
		// printf("debug msg: %d\n", code);
		switch (code) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: {
			void *ip = NULL, *fp = NULL;
			if (Check(debug_get_instruction_pointer(&wnd->fDebugContext, wnd->fId, &ip, &fp), "can't get IP and FP", false) >= B_OK) {
				wnd->Lock();
				WriteStackTrace(wnd);
				wnd->Unlock();
			}
			run = false;
			break;
		}
		}
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
	wnd->fNubPort = Check(install_team_debugger(wnd->fTeam, wnd->fDebuggerPort), "can't install debugger");
	Check(init_debug_context(&wnd->fDebugContext, wnd->fTeam, wnd->fNubPort));

	Check(debug_thread(wnd->fId));

	thread_id debugThread = spawn_thread(DebugThread, "debug thread", B_NORMAL_PRIORITY, wnd);
	wnd->Unlock();
	wait_for_thread(debugThread, &res); Check(res);
	wnd->Lock();

	destroy_debug_context(&wnd->fDebugContext);
	remove_team_debugger(wnd->fTeam);
	delete_port(wnd->fDebuggerPort);
}

static void NewFramesView(StackWindow *wnd)
{
	BColumnListView *view;
	view = new BColumnListView("Frames", B_NAVIGABLE);
	view->AddColumn(new BStringColumn("FP", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), frameFpCol);
	view->AddColumn(new BStringColumn("IP", 128, 50, 500, B_TRUNCATE_END, B_ALIGN_RIGHT), frameIpCol);
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
	BMenu *menu2;
	BMenuItem *it;
	BTab *tab;

	BString title;
	title.SetToFormat("Thread %ld", fId);
	SetTitle(title.String());

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
	
	NewFramesView(this);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, 0)
			.Add(fView)
			.SetInsets(-1, 0, -1, -1)
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
