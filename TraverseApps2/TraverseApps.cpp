#include <stdio.h>
#include <String.h>
#include <Application.h>
#include <Messenger.h>
#include <Roster.h>
#include <Window.h>
#include <View.h>
#include <Screen.h>
#include <MessageRunner.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Rect.h>
#include <OS.h>
#include <Resources.h>
#include <IconUtils.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/shared/AutoDeleter.h>

#include <vector>
#include <set>
#include <algorithm>

#include "Resources.h"
#include "HighlightRect.h"

enum {
	invokeMsg = 1,
	selectMsg,
	updateMsg,
	expandAllMsg,
	collapseAllMsg,
};

enum {
	typeCol,
	nameCol,
	idCol,
	suitesCol
};


port_id GetPort(BMessenger &obj)
{
	void *data = (void*)&obj;
	return *(port_id*)data; //fPort
}

team_id GetTeam(BMessenger &obj)
{
	port_id port = GetPort(obj);
	port_info info;
	status_t res = get_port_info(port, &info);
	if (res < B_OK) return res;
	return info.team;
}

int32 GetToken(BMessenger &obj)
{
	void *data = (void*)&obj;
	((port_id*&)data)++; //fPort
	return *(int32*)data; //fHandlerToken
}


class FrameTree
{
public:
	std::vector<FrameTree*> frames;
	BMessenger obj;
	BRect rect;
	bool visible;

	FrameTree(BMessenger obj, BRect rect, bool visible): obj(obj), rect(rect), visible(visible)
	{
	}

	void Clear()
	{
		for (std::vector<FrameTree*>::iterator it = frames.begin(); it != frames.end(); ++it) {
			delete *it;
		}
		frames.clear();
	}

	void Insert(FrameTree *frame)
	{
		frames.push_back(frame);
	}

	FrameTree *Remove(FrameTree *frame)
	{
		std::vector<FrameTree*>::iterator it = std::find(frames.begin(), frames.end(), frame);
		if (it != frames.end()) {
			FrameTree *frame = *it;
			frames.erase(it);
			return frame;
		}
		return NULL;
	}

	FrameTree *This(BPoint pos)
	{
		if (!visible)
			return NULL;
		for (std::vector<FrameTree*>::reverse_iterator it = frames.rbegin(); it != frames.rend(); ++it) {
			FrameTree *frame = *it;
			FrameTree *subFrame = frame->This(pos);
			if (subFrame != NULL)
				return subFrame;
			if (frame->rect.Contains(pos))
				return frame;
		}
		return NULL;
	}

	FrameTree *ThisTeamToken(team_id team, int32 token)
	{
		if (GetTeam(obj) == team && GetToken(obj) == token)
			return this;

		for (std::vector<FrameTree*>::iterator it = frames.begin(); it != frames.end(); ++it) {
			FrameTree *frame = *it;
			FrameTree *subFrame = frame->ThisTeamToken(team, token);
			if (subFrame != NULL)
				return subFrame;
		}
		return NULL;
	}

	~FrameTree()
	{
		for (std::vector<FrameTree*>::iterator it = frames.begin(); it != frames.end(); ++it) {
			delete *it;
		}
	}

};


BRow *FindIntRow(BColumnListView *view, BRow *parent, int32 val)
{
	BRow *row;
	BString name;
	for (int32 i = 0; i < view->CountRows(parent); i++) {
		row = view->RowAt(i, parent);
		if (((BIntegerField*)row->GetField(idCol))->Value() == val)
			return row;
	}
	return NULL;
}

status_t GetBool(bool &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindBool("result", &val)) != B_OK) return res;
	return res;
}

status_t GetInt32(int32 &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindInt32("result", &val)) != B_OK) return res;
	return res;
}

status_t GetString(BString &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindString("result", &val)) != B_OK) return res;
	return res;
}

status_t GetMessenger(BMessenger &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindMessenger("result", &val)) != B_OK) return res;
	return res;
}

status_t GetRect(BRect &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindRect("result", &val)) != B_OK) return res;
	return res;
}

void WriteError(BString &dst, status_t res)
{
	switch (res) {
	case B_TIMED_OUT: dst += "<TIME OUT>"; break;
	case B_NAME_NOT_FOUND: dst += "<NOT FOUND>"; break;
	default: dst += "<ERROR>";
	};
}

void WriteStringProp(BString &dst, BMessenger &obj, const char *field)
{
	BMessage spec;
	BString str;
	status_t res;
	spec.what = B_GET_PROPERTY;
	spec.AddSpecifier(field);
	if ((res = GetString(str, obj, spec)) != B_OK) {
		WriteError(dst, res);
	} else {
		dst += str;
	}
}

void WriteBoolProp(BString &dst, BMessenger &obj, const char *field)
{
	BMessage spec;
	bool val;
	status_t res;
	spec.what = B_GET_PROPERTY;
	spec.AddSpecifier(field);
	if ((res = GetBool(val, obj, spec)) != B_OK) {
		WriteError(dst, res);
	} else {
		if (val)
			dst += "true";
		else
			dst += "false";
	}
}

void WriteRectProp(BString &dst, BMessenger &obj, const char *field)
{
	BMessage spec;
	BRect rect;
	status_t res;
	spec.what = B_GET_PROPERTY;
	spec.AddSpecifier(field);
	if ((res = GetRect(rect, obj, spec)) != B_OK) {
		WriteError(dst, res);
	} else {
		BString buf;
		buf.SetToFormat("%g, %g, %g, %g", rect.left, rect.top, rect.right, rect.bottom);
		dst += buf;
	}
}

void WriteSuites(BString &dst, BMessenger &obj)
{
	BMessage spec, reply;
	type_code type;
	int32 count;
	const char *str;
	status_t res;
	spec = BMessage(B_GET_SUPPORTED_SUITES);
	if (
		((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK)
	) {
		WriteError(dst, res);
	} else {
		reply.GetInfo("suites", &type, &count);
		for (int32 i = 0; i < count; i++) {
			if (i > 0) {dst += ", ";}
			reply.FindString("suites", i, &str);
			dst += str;
		}
	}
}

void ListViews(BColumnListView *listView, FrameTree *frames, BRow *parent, BMessenger &wnd) {
	BMessage spec;
	int32 count;
	BMessenger view;
	BString buf;
	BRow *row;
	std::set<int32> prevViews;

	for (int32 i = 0; i < listView->CountRows(parent); i++) {
		row = listView->RowAt(i, parent);
		prevViews.insert(((BIntegerField*)row->GetField(idCol))->Value());
	}

	if (frames != NULL) frames->Clear();

	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier("View");
	if (GetInt32(count, wnd, spec) != B_OK)
		count = 0;

	for (int i = 0; i < count; i++) {
		spec = BMessage(B_GET_PROPERTY);
		spec.AddSpecifier("View", i);

		if (GetMessenger(view, wnd, spec) != B_OK)
			continue;

		team_id token = GetToken(view);

		prevViews.erase(token);

		row = FindIntRow(listView, parent, token);

		if (row == NULL) {
			row = new BRow();
			listView->AddRow(row, parent);
		}

		row->SetField(new BStringField("BView"), typeCol);
		buf = ""; WriteStringProp(buf, view, "InternalName");
		row->SetField(new BStringField(buf), nameCol);
		row->SetField(new BIntegerField(token), idCol);
		buf = ""; WriteSuites(buf, view);
		row->SetField(new BStringField(buf), suitesCol);

		FrameTree *frame = NULL;
		if (frames != NULL) {
			BRect rect;
			bool hidden;
			status_t res;
			{
				BMessage spec(B_GET_PROPERTY);
				spec.AddSpecifier("Frame");
				res = GetRect(rect, view, spec);
			}
			if (res >= B_OK) {
				{
					BMessage spec(B_GET_PROPERTY);
					spec.AddSpecifier("Hidden");
					res = GetBool(hidden, view, spec);
				}
			}
			if (res >= B_OK) {
				frame = new FrameTree(view, rect.OffsetByCopy(frames->rect.LeftTop()), !hidden);
				frames->Insert(frame);
			}
		}

		ListViews(listView, frame, row, view);
	}

	for (std::set<int32>::iterator it = prevViews.begin(); it != prevViews.end(); ++it) {
		row = FindIntRow(listView, parent, *it);
		listView->RemoveRow(row);
		delete row;
	}
}

void ListWindows(BColumnListView *listView, FrameTree *frames, BRow *parent, BMessenger &app) {
	BMessage spec;
	int32 count;
	BMessenger wnd;
	BString buf;
	BRow *row;
	std::set<int32> prevWnds;

	for (int32 i = 0; i < listView->CountRows(parent); i++) {
		row = listView->RowAt(i, parent);
		prevWnds.insert(((BIntegerField*)row->GetField(idCol))->Value());
	}

	if (frames != NULL) frames->Clear();

	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier("Window");
	if (GetInt32(count, app, spec) != B_OK)
		count = 0;

	for (int i = 0; i < count; i++) {
		spec = BMessage(B_GET_PROPERTY);
		spec.AddSpecifier("Window", i);

		if (GetMessenger(wnd, app, spec) != B_OK)
			continue;

		team_id token = GetToken(wnd);

		prevWnds.erase(token);

		row = FindIntRow(listView, parent, token);

		if (row == NULL) {
			row = new BRow();
			listView->AddRow(row, parent);
		}

		row->SetField(new BStringField("BWindow"), typeCol);
		buf = ""; WriteStringProp(buf, wnd, "InternalName");
		row->SetField(new BStringField(buf), nameCol);
		row->SetField(new BIntegerField(token), idCol);
		buf = ""; WriteSuites(buf, wnd);
		row->SetField(new BStringField(buf), suitesCol);

		FrameTree *frame = NULL;
		if (frames != NULL) {
			BRect rect;
			bool hidden;
			status_t res;
			{
				BMessage spec(B_GET_PROPERTY);
				spec.AddSpecifier("Frame");
				res = GetRect(rect, wnd, spec);
			}
			if (res >= B_OK) {
				{
					BMessage spec(B_GET_PROPERTY);
					spec.AddSpecifier("Hidden");
					res = GetBool(hidden, wnd, spec);
				}
			}
			if (res >= B_OK) {
				frame = new FrameTree(wnd, rect, !hidden);
				frames->Insert(frame);
			}
		}

		ListViews(listView, frame, row, wnd);
	}

	for (std::set<int32>::iterator it = prevWnds.begin(); it != prevWnds.end(); ++it) {
		row = FindIntRow(listView, parent, *it);
		listView->RemoveRow(row);
		delete row;
	}
}

void ListApps(BColumnListView *listView, FrameTree *frames) {
	BList appList;
	app_info info;
	BMessenger app;
	BString buf;
	BRow *row;
	std::set<int32> prevApps;

	for (int32 i = 0; i < listView->CountRows(); i++) {
		row = listView->RowAt(i);
		prevApps.insert(((BIntegerField*)row->GetField(idCol))->Value());
	}

	be_roster->GetAppList(&appList);
	for (int i = 0; i < appList.CountItems(); i++) {
		team_id team = (team_id)(intptr_t)appList.ItemAt(i);

		if (team == be_app->Team())
			continue;

		be_roster->GetRunningAppInfo(team, &info);
		app = BMessenger(NULL, team);

		prevApps.erase(team);

		row = FindIntRow(listView, NULL, team);

		if (row == NULL) {
			row = new BRow();
			listView->AddRow(row);
		}

		row->SetField(new BStringField("BApplication"), typeCol);
		row->SetField(new BStringField(info.signature), nameCol);
		row->SetField(new BIntegerField(team), idCol);
		buf = ""; WriteSuites(buf, app);
		row->SetField(new BStringField(buf), suitesCol);
		ListWindows(listView, frames, row, app);
	}

	for (std::set<int32>::iterator it = prevApps.begin(); it != prevApps.end(); ++it) {
		row = FindIntRow(listView, NULL, *it);
		listView->RemoveRow(row);
		delete row;
	}
}

void ExpandAll(BColumnListView *view, BRow *row)
{
	int32 count;
	view->ExpandOrCollapse(row, true);
	count = view->CountRows(row);
	for(int32 i = 0; i < count; i++) {
		ExpandAll(view, view->RowAt(i, row));
	}
}

void CollapseAll(BColumnListView *view, BRow *row)
{
	int32 count;
	count = view->CountRows(row);
	for(int32 i = 0; i < count; i++) {
		CollapseAll(view, view->RowAt(i, row));
	}
	view->ExpandOrCollapse(row, false);
}

void ExpandRow(BColumnListView *view, BRow *row)
{
	BRow *parent;
	bool isVisible;
	status_t res = view->FindParent(row, &parent, &isVisible);
	if (res >= B_OK && parent != NULL) {
		ExpandRow(view, parent);
		view->ExpandOrCollapse(parent, true);
	}
}

BRow *FindRowByToken(BColumnListView *view, BRow *parent, int32 token)
{
	int32 count;
	count = view->CountRows(parent);
	for(int32 i = 0; i < count; i++) {
		BRow *row = view->RowAt(i, parent);
		if (((BIntegerField*)row->GetField(idCol))->Value() == token)
			return row;
		row = FindRowByToken(view, row, token);
		if (row != NULL)
			return row;
	}
	return NULL;
}

BRow *FindRowByTeamToken(BColumnListView *view, team_id team, int32 token)
{
	BRow *row = FindIntRow(view, NULL, team);
	if (row == NULL) return NULL;
	return FindRowByToken(view, row, token);
}

bool GetTokenForRow(BColumnListView *view, int32 &token, BRow *parent, BRow *target)
{
	int32 count;
	count = view->CountRows(parent);
	for(int32 i = 0; i < count; i++) {
		BRow *row = view->RowAt(i, parent);
		if (row == target) {
			token = ((BIntegerField*)row->GetField(idCol))->Value();
			return true;
		}
		if (GetTokenForRow(view, token, row, target))
			return true;
	}
	return false;
}

bool GetTeamTokenForRow(BColumnListView *view, team_id &team, int32 &token, BRow *target)
{
	int32 count;
	count = view->CountRows(NULL);
	for(int32 i = 0; i < count; i++) {
		BRow *row = view->RowAt(i, NULL);
		team = ((BIntegerField*)row->GetField(idCol))->Value();
		if (GetTokenForRow(view, token, row, target))
			return true;
	}
	return false;
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


// void BMenuItem::Select(bool select)
extern "C" void _ZN9BMenuItem6SelectEb(BMenuItem *item, bool select);

class TestToolBar: public BMenuBar
{
private:
	bool fTrack;
	BMenuItem *fCurItem;
	HighlightRect fHRect;
	FrameTree *fFrames, *fCurFrame;
	BColumnListView *fView;

public:
	TestToolBar(const char* name, menu_layout layout, FrameTree *frames, BColumnListView *view):
		BMenuBar(name, layout),
		fTrack(false),
		fCurItem(NULL),
		fFrames(frames),
		fCurFrame(NULL),
		fView(view)
	{
	}

	void UpdateHighlight(BPoint pos)
	{
		fCurFrame = fFrames->This(ConvertToScreen(pos));
		if (fCurFrame == NULL)
			fHRect.Hide();
		else
			fHRect.Show(fCurFrame->rect);
	}

	void MouseDown(BPoint where)
	{
		BMenuItem *selectItem = ItemAt(0);
		BMenuItem *highlightItem = ItemAt(1);
		fCurItem = NULL;
		if (selectItem->Frame().Contains(where)) fCurItem = selectItem;
		if (highlightItem->Frame().Contains(where)) fCurItem = highlightItem;

		if (fCurItem != NULL) {
			SetMouseEventMask(B_POINTER_EVENTS);
			fTrack = true;
			_ZN9BMenuItem6SelectEb(fCurItem, true);
			if (fCurItem == selectItem)
				UpdateHighlight(where);
			else if (fCurItem == highlightItem) {
				BRow *row = fView->CurrentSelection();
				if (row != NULL) {
					team_id team;
					int32 token;
					if (GetTeamTokenForRow(fView, team, token, row)) {
						fCurFrame = fFrames->ThisTeamToken(team, token);
						if (fCurFrame != NULL)
							fHRect.Show(fCurFrame->rect);
					}
				}
			}
			return;
		}
		BMenuBar::MouseDown(where);
	}

	void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
	{
		if (fTrack) {
			if (fCurItem == ItemAt(0))
				UpdateHighlight(where);
			return;
		}
		BMenuBar::MouseMoved(where, code, dragMessage);
	}

	void MouseUp(BPoint where)
	{
		if (fTrack) {
			BMenuItem *selectItem = ItemAt(0);
			if (fCurItem != NULL)
				_ZN9BMenuItem6SelectEb(fCurItem, false);
			if (fCurItem == ItemAt(0) && fCurFrame != NULL) {
				team_id team = GetTeam(fCurFrame->obj);
				int32 token = GetToken(fCurFrame->obj);
				BRow *row = FindRowByTeamToken(fView, team, token);
				if (row != NULL) {
					ExpandRow(fView, row);
					fView->DeselectAll();
					fView->SetFocusRow(row, true);
					fView->ScrollTo(row);
				}
			}
			fCurItem = NULL;
			fCurFrame = NULL;
			fHRect.Hide();
			fTrack = false;
			return;
		}
		BMenuBar::MouseUp(where);
	}

};

class TestWindow: public BWindow
{
private:
	BColumnListView *fView;
	BMenuBar *fMenuBar;
	BMenuBar *fToolBar;
	FrameTree fFrames;
	//BMessageRunner listUpdater;
public:
	TestWindow(BRect frame):
		BWindow(frame, "TraverseApps", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
		fFrames(BMessenger(), BScreen().Frame(), true)
		//listUpdater(BMessenger(this), BMessage(updateMsg), 500000)
	{
		BRow *row1, *row2;
		fView = new BColumnListView("view", 0);
		fView->SetInvocationMessage(new BMessage(invokeMsg));
		fView->SetSelectionMessage(new BMessage(selectMsg));
		fView->AddColumn(new BStringColumn("Type", 96, 50, 512, B_TRUNCATE_MIDDLE), typeCol);
		fView->AddColumn(new BStringColumn("Name", 250, 50, 512, B_TRUNCATE_MIDDLE), nameCol);
		fView->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), idCol);
		fView->AddColumn(new BStringColumn("Suites", 128, 50, 512, B_TRUNCATE_MIDDLE), suitesCol);

		ListApps(fView, &fFrames);

		BMenu *menu2;
		BMenuItem *it;

		fMenuBar = new BMenuBar("menuBar", B_ITEMS_IN_ROW);
		menu2 = new BMenu("Edit");
			menu2->AddItem(new BMenuItem("Update", new BMessage(updateMsg), 'R'));
			menu2->AddItem(new BSeparatorItem());
			menu2->AddItem(new BMenuItem("Expand all", new BMessage(expandAllMsg)));
			menu2->AddItem(new BMenuItem("Collapse all", new BMessage(collapseAllMsg)));
			fMenuBar->AddItem(menu2);

		fToolBar = new TestToolBar("toolBar", B_ITEMS_IN_ROW, &fFrames, fView);
			fToolBar->AddItem(new IconMenuItem(LoadIcon(resTargetIcon, 16, 16), NULL));
			fToolBar->AddItem(new IconMenuItem(LoadIcon(resHighlightIcon, 16, 16), NULL));

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(fMenuBar)
			.Add(fToolBar)
			.AddGroup(B_HORIZONTAL)
				.Add(fView)
				.SetInsets(-1)
			.End();

		SetKeyMenuBar(fMenuBar);
	}

	void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
		case invokeMsg:
		case selectMsg:
			break;
		case updateMsg:
			ListApps(fView, &fFrames);
			break;
		case expandAllMsg: ExpandAll(fView, NULL); break;
		case collapseAllMsg: CollapseAll(fView, NULL); break;
		default:
			BWindow::MessageReceived(msg);
		}
	}
};

class TestApplication: public BApplication
{
public:
	TestApplication(): BApplication("application/x-vnd.Test-TraverseApps")
	{
	}

	void ReadyToRun()
	{
		BRect rect(0, 0, 832, 480);
		rect.OffsetBy(64, 64);
		BWindow *wnd = new TestWindow(rect);
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
