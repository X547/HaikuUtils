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

enum {
	invokeMsg = 1,
	selectMsg = 2,
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
		InsertRow(view, row, list);
		RowTree *next = list->next.Detach();
		list.SetTo(next);
	}
}

static void SetRowParent(BColumnListView *view, BRow *row, BRow *newParent)
{
	ObjectDeleter<RowTree> tree;
	RemoveRow(view, row, tree);
	InsertRow(view, newParent, tree);
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

static void ListTeams(BColumnListView *view) {
	status_t status;
	team_info info;
	int32 cookie;
	BRow *row;
	cookie = 0;
	view->AddColumn(new IconStringColumn("Name", 256 - 32, 50, 500, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), idCol);
	view->AddColumn(new BIntegerColumn("Parent", 64, 32, 128, B_ALIGN_RIGHT), parentIdCol);
	view->AddColumn(new BIntegerColumn("Session", 64, 32, 128, B_ALIGN_RIGHT), sidCol);
	view->AddColumn(new BIntegerColumn("Group", 64, 32, 128, B_ALIGN_RIGHT), gidCol);
	view->AddColumn(new BIntegerColumn("User", 64, 32, 128, B_ALIGN_RIGHT), uidCol);
	view->AddColumn(new BStringColumn("Path", 512, 50, 1024, B_TRUNCATE_MIDDLE), pathCol);
	view->SetColumnVisible(parentIdCol, false);
	while (get_next_team_info(&cookie, &info) == B_OK) {
		int32 uid = -1;
		KMessage extInfo;
		if (get_extended_team_info(info.team, B_TEAM_INFO_BASIC, extInfo) >= B_OK) {
			if (extInfo.FindInt32("uid", &uid) < B_OK) uid = -1;
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

		row = new BRow();
		row->SetField(new IconStringField(icon, path.Leaf()), nameCol);
		row->SetField(new BIntegerField(info.team), idCol);
		row->SetField(new BIntegerField((info.team == 1)? -1: _kern_process_info(info.team, PARENT_ID)), parentIdCol);
		row->SetField(new BIntegerField(_kern_process_info(info.team, SESSION_ID)), sidCol);
		row->SetField(new BIntegerField(_kern_process_info(info.team, GROUP_ID)), gidCol);
		row->SetField(new BIntegerField(uid), uidCol);
		row->SetField(new BStringField(imageInfo.name), pathCol);
		view->AddRow(row);
	}
	
	switch (1) {
	case 0: // linear
		break;
	case 1: { // process tree
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
	case 2: { // sessions and groups
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
}

static BColumnListView* NewTeamsView(const char *name)
{
	BColumnListView *view;
	view = new BColumnListView(name, B_NAVIGABLE);
	view->SetInvocationMessage(new BMessage(invokeMsg));
	ListTeams(view);
	return view;
}

class TestWindow: public BWindow
{
private:
	BColumnListView *fTeamsView;

public:
	TestWindow(BRect frame): BWindow(frame, "SystemManager", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		BRect r;
		BView *rootView;
		BMenuBar *menuBar;
		BMenu *menu2;
		BMenuItem *it;
		BTabView *tabView;
		BTab *tab;

		r = Bounds();

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

		tabView = new BTabView("tab_view", B_WIDTH_FROM_LABEL);
		tabView->SetBorder(B_NO_BORDER);

		//tab = new BTab(); tabView->AddTab(NewTeamsView("Apps"), tab);
		tab = new BTab(); tabView->AddTab(fTeamsView = NewTeamsView("Teams"), tab);
		//tab = new BTab(); tabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Services", B_FOLLOW_NONE), tab);
		//tab = new BTab(); tabView->AddTab(new TestView(BRect(0, 0, -1, -1), "Sockets", B_FOLLOW_NONE), tab);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			//.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(menuBar)
			.AddGroup(B_VERTICAL, 0)
				.Add(tabView)
				.SetInsets(-1, 0, -1, -1)
				.End()
			.End()
		;
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case invokeMsg: {
			BRow *row = fTeamsView->CurrentSelection(NULL);
			if (row != NULL) {
				OpenTeamWindow(((BIntegerField*)row->GetField(idCol))->Value(),
					BPoint((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2)
				);
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
		}
	}

};

class TestApplication: public BApplication
{
public:
	TestApplication(): BApplication("application/x-vnd.Test-SystemManager")
	{
	}

	void ReadyToRun() {
		BRect rect(0, 0, 640, 480);
		rect.OffsetTo(64, 64);
		BWindow *wnd = new TestWindow(rect);
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->Show();
	}
};


int main()
{
	(new TestApplication())->Run();
	return 0;
}
