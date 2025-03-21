#include <stdio.h>

#include <Application.h>
#include <MessageRunner.h>
#include <Invoker.h>
#include <Window.h>
#include <Alert.h>
#include <View.h>
#include <Menu.h>
#include <MenuBar.h>
#include <TabView.h>
#include <LayoutBuilder.h>
#include <Rect.h>
#include <IconUtils.h>
#include <Resources.h>

#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>

#include <pwd.h>
#include <grp.h>

#include <private/kernel/util/KMessage.h>

#include "Resources.h"
#include "UserForm.h"
#include "GroupForm.h"
#include "PasswordForm.h"
#include "UserDB.h"
#include "UIUtils.h"


enum {
	updateMsg = 1,
	addMsg,
	removeMsg,
	editMsg,
	setPasswordMsg,
	addMemberMsg,
	removeMemberMsg,
};

enum {
	userNameCol = 0,
	userIdCol,
	userGroupIdCol,
	userPasswdCol,
	userDirCol,
	userShellCol,
	userGecosCol,
};

enum {
	groupNameCol = 0,
	groupIdCol,
	groupPasswdCol,
	groupMembersCol,
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


static void ListUsers(BColumnListView *view)
{
	BRow *row;
	BList prevRows;

	int32 count;
	passwd** entries;

	KMessage reply;
	if (GetUsers(reply, count, entries) < B_OK) count = 0;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(userIdCol))->Value()));
	}

	for (int32 i = 0; i < count; i++) {
		passwd *entry = entries[i];

		prevRows.RemoveItem((void*)(addr_t)entry->pw_uid);
		row = FindIntRow(view, userIdCol, NULL, entry->pw_uid);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BStringField(entry->pw_name), userNameCol);
		row->SetField(new BIntegerField(entry->pw_uid), userIdCol);
		row->SetField(new BIntegerField(entry->pw_gid), userGroupIdCol);
		row->SetField(new BStringField(entry->pw_passwd), userPasswdCol);
		row->SetField(new BStringField(entry->pw_dir), userDirCol);
		row->SetField(new BStringField(entry->pw_shell), userShellCol);
		row->SetField(new BStringField(entry->pw_gecos), userGecosCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, userIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewUsersView()
{
	BColumnListView *view;
	view = new BColumnListView("Users", B_NAVIGABLE);
	view->SetInvocationMessage(new BMessage(editMsg));
	view->AddColumn(new BStringColumn("Name", 96, 50, 500, B_TRUNCATE_END), userNameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), userIdCol);
	view->AddColumn(new BIntegerColumn("Group ID", 64, 32, 128, B_ALIGN_RIGHT), userGroupIdCol);
	view->AddColumn(new BStringColumn("passwd", 48, 50, 500, B_TRUNCATE_END), userPasswdCol);
	view->AddColumn(new BStringColumn("Home directory", 150, 50, 500, B_TRUNCATE_END), userDirCol);
	view->AddColumn(new BStringColumn("Shell", 150, 50, 500, B_TRUNCATE_END), userShellCol);
	view->AddColumn(new BStringColumn("Real name", 150, 50, 500, B_TRUNCATE_END), userGecosCol);
	view->SetColumnVisible(userPasswdCol, false);
	ListUsers(view);
	return view;
}

static void ListGroups(BColumnListView *view)
{
	BRow *row;
	BList prevRows;

	int32 count;
	group** entries;

	KMessage reply;
	if (GetGroups(reply, count, entries) < B_OK) count = 0;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(groupIdCol))->Value()));
	}

	for (int32 i = 0; i < count; i++) {
		group* entry = entries[i];

		prevRows.RemoveItem((void*)(addr_t)entry->gr_gid);
		row = FindIntRow(view, userIdCol, NULL, entry->gr_gid);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BStringField(entry->gr_name), groupNameCol);
		row->SetField(new BIntegerField(entry->gr_gid), groupIdCol);
		row->SetField(new BStringField(entry->gr_passwd), groupPasswdCol);

		BString str;
		for (int j = 0; entry->gr_mem[j] != NULL; j++) {
			if (j > 0) str += ", ";
			str += entry->gr_mem[j];
		}
		row->SetField(new BStringField(str), groupMembersCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, groupIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewGroupsView()
{
	BColumnListView *view;
	view = new BColumnListView("Groups", B_NAVIGABLE);
	view->SetInvocationMessage(new BMessage(editMsg));
	view->AddColumn(new BStringColumn("Name", 96, 50, 500, B_TRUNCATE_END), groupNameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), groupIdCol);
	view->AddColumn(new BStringColumn("passwd", 48, 50, 500, B_TRUNCATE_END), groupPasswdCol);
	view->AddColumn(new BStringColumn("Members", 128, 50, 500, B_TRUNCATE_END), groupMembersCol);
	view->SetColumnVisible(groupPasswdCol, false);
	ListGroups(view);
	return view;
}


class TestWindow: public BWindow
{
private:
	BMessageRunner fListUpdater;
	BTabView *fTabView;
	BColumnListView *fUsersView;
	BColumnListView *fGroupsView;

	BMenu *fAddMemberMenu;
	BMenu *fRemoveMemberMenu;

public:
	TestWindow(BRect frame): BWindow(frame, "Users", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
		fListUpdater(BMessenger(this), BMessage(updateMsg), 500000)
	{
		BMenuBar *menuBar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
		BLayoutBuilder::Menu<>(menuBar)
/*
			.AddMenu(new BMenu("File"))
				.AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'))
			.End()
*/
			.AddMenu(new BMenu("Action"))
				.AddItem(new BMenuItem("Add", new BMessage(addMsg)))
				.AddItem(new BMenuItem("Remove", new BMessage(removeMsg)))
				.AddItem(new BMenuItem("Edit", new BMessage(editMsg)))
				.AddItem(new BMenuItem("Set password", new BMessage(setPasswordMsg)))
				.AddSeparator()
				.AddMenu(fAddMemberMenu = new BMenu("Add member"))
				.End()
				.AddMenu(fRemoveMemberMenu = new BMenu("Remove member"))
				.End()
			.End()
		.End();

		BMenuBar *toolBar = new BMenuBar("toolbar", B_ITEMS_IN_ROW, true);
		BLayoutBuilder::Menu<>(toolBar)
			.AddItem(new IconMenuItem(LoadIcon(resAddIcon, 16, 16), new BMessage(addMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resRemoveIcon, 16, 16), new BMessage(removeMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resEditIcon, 16, 16), new BMessage(editMsg)))
		.End();

		fTabView = new BTabView("tabView", B_WIDTH_FROM_LABEL);
		fTabView->SetBorder(B_NO_BORDER);

		BTab *tab = new BTab(); fTabView->AddTab(fUsersView = NewUsersView(), tab);
		tab = new BTab(); fTabView->AddTab(fGroupsView = NewGroupsView(), tab);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(menuBar)
			.Add(toolBar)
			.AddGroup(B_VERTICAL, 0)
				.Add(fTabView)
				.SetInsets(-1, 0, -1, -1)
			.End()
		.End();

		SetKeyMenuBar(menuBar);
		fUsersView->MakeFocus();
	}

	void MenusBeginning()
	{
		KMessage usersReply, groupsReply;
		int32 userCnt, groupCnt;
		passwd **users;
		group** groups;
		group *curGroup = NULL;
		int32 curGroupId;

		fAddMemberMenu->Superitem()->SetEnabled(false);
		fRemoveMemberMenu->Superitem()->SetEnabled(false);
		fAddMemberMenu->RemoveItems(0, fAddMemberMenu->CountItems(), true);
		fRemoveMemberMenu->RemoveItems(0, fRemoveMemberMenu->CountItems(), true);

		BTab *tab = fTabView->TabAt(fTabView->Selection());
		if (tab == NULL) return;
		BView *view = tab->View();
		if (view != fGroupsView) return;
		BRow *row = fGroupsView->CurrentSelection(NULL);
		if (row == NULL) return;

		fAddMemberMenu->Superitem()->SetEnabled(true);
		fRemoveMemberMenu->Superitem()->SetEnabled(true);

		if (GetUsers(usersReply, userCnt, users) < B_OK) return;
		if (GetGroups(groupsReply, groupCnt, groups) < B_OK) return;

		curGroupId = ((BIntegerField*)row->GetField(groupIdCol))->Value();

		{
			int32 i = 0;
			while ((i < groupCnt) && !((int32)groups[i]->gr_gid == curGroupId)) i++;
			if (i < groupCnt)
				curGroup = groups[i];
			else
				return;
		}

		for (int32 i = 0; i < userCnt; i++) {
			int j = 0;
			while ((curGroup->gr_mem[j] != NULL) && !(strcmp(users[i]->pw_name, curGroup->gr_mem[j]) == 0)) j++;
			if (curGroup->gr_mem[j] == NULL) {
				BMessage *msg = new BMessage(addMemberMsg);
				BMenuItem *item;
				msg->AddString("val", users[i]->pw_name);
				fAddMemberMenu->AddItem(item = new BMenuItem(users[i]->pw_name, msg));
			}
		}

		for (int32 i = 0; curGroup->gr_mem[i] != NULL; i++) {
			BMessage *msg = new BMessage(removeMemberMsg);
			BMenuItem *item;
			msg->AddString("val", curGroup->gr_mem[i]);
			fRemoveMemberMenu->AddItem(item = new BMenuItem(curGroup->gr_mem[i], msg));
		}
	}

	void MenusEnded()
	{
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case updateMsg: {
			ListUsers(fUsersView);
			ListGroups(fGroupsView);
			return;
		}
		case addMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				ShowUserForm(-1, this);
			} else if (view == fGroupsView) {
				ShowGroupForm(-1, this);
			}
			return;
		}
		case removeMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				BRow *row = fUsersView->CurrentSelection(NULL);
				if (row == NULL) return;
				const char *userName = ((BStringField*)row->GetField(userNameCol))->String();

				int32 which;
				BInvoker *invoker = NULL;
				if (msg->FindInt32("which", &which) < B_OK) {
					BString str;
					str.SetToFormat("Are you sure you want to delete user \"%s\"?", userName);
					BAlert *alert = new BAlert("Users", str, "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					invoker = new BInvoker(new BMessage(*msg), this);
					invoker->Message()->SetPointer("invoker", invoker);
					alert->Go(invoker);
					return;
				} else {
					if (msg->FindPointer("invoker", &(void*&)invoker) >= B_OK) {
						delete invoker; invoker = NULL;
					}
					if (which != 0) return;
				}
				if (DeleteUser(-1, userName) < B_OK) return;
				ListUsers(fUsersView);
			} else if (view == fGroupsView) {
				BRow *row = fGroupsView->CurrentSelection(NULL);
				if (row == NULL) return;
				const char *groupName = ((BStringField*)row->GetField(groupNameCol))->String();

				int32 which;
				BInvoker *invoker = NULL;
				if (msg->FindInt32("which", &which) < B_OK) {
					BString str;
					str.SetToFormat("Are you sure you want to delete group \"%s\"?", groupName);
					BAlert *alert = new BAlert("Users", str, "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					invoker = new BInvoker(new BMessage(*msg), this);
					invoker->Message()->SetPointer("invoker", invoker);
					alert->Go(invoker);
					return;
				} else {
					if (msg->FindPointer("invoker", &(void*&)invoker) >= B_OK) {
						delete invoker; invoker = NULL;
					}
					if (which != 0) return;
				}
				if (DeleteGroup(-1, groupName) < B_OK) return;
				ListGroups(fGroupsView);
			}
			return;
		}
		case editMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				BRow *row = fUsersView->CurrentSelection(NULL);
				if (row == NULL) return;
				int32 uid = ((BIntegerField*)row->GetField(userIdCol))->Value();
				ShowUserForm(uid, this);
			} else if (view == fGroupsView) {
				BRow *row = fGroupsView->CurrentSelection(NULL);
				if (row == NULL) return;
				int32 gid = ((BIntegerField*)row->GetField(groupIdCol))->Value();
				ShowGroupForm(gid, this);
			}
			return;
		}
		case setPasswordMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				BRow *row = fUsersView->CurrentSelection(NULL);
				if (row == NULL) return;
				int32 uid = ((BIntegerField*)row->GetField(userIdCol))->Value();
				ShowPasswordForm(uid, this);
			} else if (view == fGroupsView) {
				BAlert *alert = new BAlert("Users", "Set group password feature is not yet implemented.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go(NULL);
			}
			return;
		}
		case addMemberMsg:
		case removeMemberMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view != fGroupsView) return;
			BRow *row = fGroupsView->CurrentSelection(NULL);
			if (row == NULL) return;
			int32 groupId = ((BIntegerField*)row->GetField(groupIdCol))->Value();
			const char *memberName;
			if (msg->FindString("val", &memberName) < B_OK) return;

			KMessage groupsReply, updateGroupMsg;
			int32 groupCnt;
			group **groups;
			group *curGroup;
			if (GetGroups(groupsReply, groupCnt, groups) < B_OK) return;
			{
				int32 i = 0;
				while ((i < groupCnt) && !((int32)groups[i]->gr_gid == groupId)) i++;
				if (i < groupCnt)
					curGroup = groups[i];
				else
					return;
			}
			updateGroupMsg.AddInt32("gid", groupId);
			switch (msg->what) {
			case addMemberMsg: {
				for (int32 i = 0; curGroup->gr_mem[i] != NULL; i++)
					updateGroupMsg.AddString("members", curGroup->gr_mem[i]);

				updateGroupMsg.AddString("members", memberName);
				break;
			}
			case removeMemberMsg: {
				bool isEmpty = true;
				for (int32 i = 0; curGroup->gr_mem[i] != NULL; i++) {
					if (strcmp(memberName, curGroup->gr_mem[i]) != 0) {
						isEmpty = false;
						updateGroupMsg.AddString("members", curGroup->gr_mem[i]);
					}
				}
				if (isEmpty)
					updateGroupMsg.AddString("members", "");

				break;
			}
			}
			UpdateGroup(updateGroupMsg);
			ListGroups(fGroupsView);
			return;
		}
		}
		BWindow::MessageReceived(msg);
	}

	void Quit()
	{
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		BWindow::Quit();
	}

};

class TestApplication: public BApplication
{
private:
	TestWindow *fWnd;

public:
	TestApplication(): BApplication("application/x-vnd.Test-Users")
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 512 + 256, 256));
		fWnd->CenterOnScreen();
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
