#include "GroupForm.h"

#include <stdio.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include <Window.h>
#include <Alert.h>
#include <Messenger.h>
#include <TextControl.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Box.h>
#include <StringView.h>
#include <SeparatorView.h>
#include <LayoutBuilder.h>

#include <private/kernel/util/KMessage.h>
#include "UserDB.h"
#include "UIUtils.h"
#include "Resources.h"


#define CheckRetVoid(err) {status_t _err = (err); if (_err < B_OK) return;}


enum {
	okMsg = 1,
	groupMsg,
	addMemberMsg,
	removeMemberMsg
};


static BLayoutItem *CreateTextControlLayoutItem(BTextControl *view)
{
	BGroupLayout *layout;
		BLayoutBuilder::Group<>(B_VERTICAL, 0)
			.GetLayout(&layout)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(view->CreateLabelLayoutItem())
				.AddGlue()
				.End()
			.Add(view->CreateTextViewLayoutItem())
			.End();
	return layout;
}


class GroupForm: public BWindow
{
private:
	int32 fGid;

	BTextControl *fGroupNameView;
	BMenuBar *fMembersToolbar;
	BListView *fMembers;
	BTextControl *fPasswordView;
	BTextControl *fPasswordRepeatView;

	BMenu *fAddMemberMenu;

	BButton *fOkView;
	BButton *fCancelView;

	void AddMember(const char *userName)
	{
		fMembers->AddItem(new BStringItem(userName));
		for (int32 i = 0; i < fAddMemberMenu->CountItems(); i++) {
			const char *name;
			if (
				(fAddMemberMenu->ItemAt(i)->Message()->FindString("val", &name) >= B_OK) &&
				(strcmp(name, userName) == 0)
			) {
				delete fAddMemberMenu->RemoveItem(i);
				return;
			}
		}
	}

public:
	GroupForm(int32 gid):
		BWindow(
			BRect(0, 0, 255, 0), (gid < 0)? "New group": "Edit group", B_TITLED_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_CLOSE_ON_ESCAPE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
		),
		fGid(gid)
	{
		fAddMemberMenu = new BMenu("Add");
		BLayoutBuilder::Menu<>(fMembersToolbar = new BMenuBar("membersToolbar", B_ITEMS_IN_ROW, true))
			.AddItem(new IconMenuItem(LoadIcon(resAddIcon, 16, 16), fAddMemberMenu, new BMessage(removeMemberMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resRemoveIcon, 16, 16), new BMessage(removeMemberMsg)))
			.End()
		;

		BBox *boxView = new BBox("box");
		BLayoutBuilder::Group<>(boxView, B_VERTICAL, 0)
			.SetInsets(2)
			.Add(fMembersToolbar)
			.AddGroup(B_VERTICAL, 0)
				.Add(new BScrollView(
					"membersScrollView",
					fMembers = new BListView("members"),
					B_SUPPORTS_LAYOUT, false, true, B_PLAIN_BORDER
				))
				.SetInsets(-1)
				.End()
			.End()
		;
		fMembers->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

		BGroupLayout *passwordLayout;

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_SMALL_SPACING)
				.Add(CreateTextControlLayoutItem(fGroupNameView = new BTextControl("groupName", "Group name:", "", NULL)))
				.AddGroup(B_VERTICAL, 0)
					.AddGroup(B_HORIZONTAL, 0)
						.Add(new BStringView("membersLabel", "Members:"))
						.AddGlue()
						.End()
					.Add(boxView)
					.End()
				.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
					.GetLayout(&passwordLayout)
					.Add(CreateTextControlLayoutItem(fPasswordView = new BTextControl("password", "Password:", "", NULL)))
					.Add(CreateTextControlLayoutItem(fPasswordRepeatView = new BTextControl("passwordRepeat", "Repeat password:", "", NULL)))
					.End()
				.End()
			.Add(new BSeparatorView(B_HORIZONTAL))
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_SMALL_SPACING)
				.AddGlue()
				.Add(fOkView = new BButton("ok", "OK", new BMessage(okMsg)))
				.Add(fCancelView = new BButton("cancel", "Cancel", new BMessage(B_QUIT_REQUESTED)))
				.End()
			.End()
		;

		{
			KMessage usersReply;
			int32 userCnt;
			passwd **users;
			if (GetUsers(usersReply, userCnt, users) < B_OK) userCnt = 0;
			for (int32 i = 0; i < userCnt; i++) {
				BMessage *msg = new BMessage(addMemberMsg);
				BMenuItem *item;
				msg->AddString("val", users[i]->pw_name);
				fAddMemberMenu->AddItem(item = new BMenuItem(users[i]->pw_name, msg));
			}
		}

		if (false && fGid < 0) {
			fPasswordView->TextView()->HideTyping(true);
			fPasswordRepeatView->TextView()->HideTyping(true);
		} else {
			passwordLayout->SetVisible(false);
		}

		if (fGid >= 0) {
			KMessage groupInfo;
			if (GetGroup(groupInfo, fGid, NULL) >= B_OK) {
				const char *name;
				if (groupInfo.FindString("name", &name) >= B_OK) {
					fGroupNameView->SetText(name);
				}
				for (int32 i = 0; groupInfo.FindString("members", i, &name) >= B_OK; i++)
					AddMember(name);
			}
		}

		SetDefaultButton(fOkView);
		fGroupNameView->MakeFocus();
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case okMsg: {
			if (fGid < 0) {
				gid_t gid = 100;
				while (getgrgid(gid) != NULL) gid++;

				KMessage message;
				CheckRetVoid(message.AddInt32("gid", gid));
				CheckRetVoid(message.AddString("name", fGroupNameView->Text()));
				CheckRetVoid(message.AddString("password", "x"));
				CheckRetVoid(message.AddBool("add group", true));
				for (int32 i = 0; i < fMembers->CountItems(); i++) {
					CheckRetVoid(message.AddString("members", dynamic_cast<BStringItem*>(fMembers->ItemAt(i))->Text()));
				}
				CheckRetVoid(UpdateGroup(message));
			} else {
				KMessage message;
				CheckRetVoid(message.AddInt32("gid", fGid));
				CheckRetVoid(message.AddString("name", fGroupNameView->Text()));
				if (fMembers->CountItems() == 0) {
					CheckRetVoid(message.AddString("members", ""));
				} else {
					for (int32 i = 0; i < fMembers->CountItems(); i++) {
						CheckRetVoid(message.AddString("members", dynamic_cast<BStringItem*>(fMembers->ItemAt(i))->Text()));
					}
				}
				CheckRetVoid(UpdateGroup(message));
			}
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		case addMemberMsg: {
			const char *userName;
			if (msg->FindString("val", &userName) < B_OK) return;
			AddMember(userName);
			return;
		}
		case removeMemberMsg: {
			BListItem *item = fMembers->ItemAt(fMembers->CurrentSelection());
			if (item == NULL) return;
			const char *userName = dynamic_cast<BStringItem*>(item)->Text();
			BMessage *itemMsg = new BMessage(addMemberMsg);
			itemMsg->AddString("val", userName);
			fAddMemberMenu->AddItem(new BMenuItem(userName, itemMsg));
			fMembers->RemoveItem(item);
			return;
		}
		}

		BWindow::MessageReceived(msg);
	}
};


void ShowGroupForm(int32 gid, BWindow *base)
{
	GroupForm *wnd = new GroupForm(gid);
	wnd->AddToSubset(base);
	wnd->CenterIn(base->Frame());
	wnd->Show();
}
