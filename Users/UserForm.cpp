#include "UserForm.h"

#include <stdio.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include <Window.h>
#include <Alert.h>
#include <Messenger.h>
#include <TextControl.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Button.h>
#include <Box.h>
#include <SeparatorView.h>
#include <LayoutBuilder.h>

#include <private/kernel/util/KMessage.h>
#include "UserDB.h"


#define CheckRetVoid(err) {status_t _err = (err); if (_err < B_OK) return;}


enum {
	okMsg = 1,
	groupMsg,
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

static BLayoutItem *CreateMenuFieldLayoutItem(BMenuField *view)
{
	BGroupLayout *layout;
	BLayoutBuilder::Group<>(B_HORIZONTAL, 0)
		.GetLayout(&layout)
		.Add(view->CreateLabelLayoutItem())
		.Add(view->CreateMenuBarLayoutItem())
	.End();
	return layout;
}


class UserForm: public BWindow
{
private:
	int32 fUid;

	BTextControl *fUserNameView;
	BTextControl *fRealNameView;
	BMenuField *fGroupView;
	BTextControl *fHomeDirView;
	BTextControl *fShellView;
	BTextControl *fPasswordView;
	BTextControl *fPasswordRepeatView;

	BButton *fOkView;
	BButton *fCancelView;

public:
	UserForm(int32 uid):
		BWindow(
			BRect(0, 0, 255, 0), (uid < 0)? "New user": "Edit user", B_TITLED_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_CLOSE_ON_ESCAPE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
		),
		fUid(uid)
	{
		BMenu *menu = new BPopUpMenu("groupMenu");

		int32 count;
		group** entries;

		KMessage reply;
		if (GetGroups(reply, count, entries) < B_OK) count = 0;
		for (int32 i = 0; i < count; i++) {
			BMessage *msg = new BMessage(groupMsg);
			BMenuItem *item;
			msg->AddInt32("val", entries[i]->gr_gid);
			menu->AddItem(item = new BMenuItem(entries[i]->gr_name, msg));
			if (entries[i]->gr_gid == 100)
				item->SetMarked(true);
		}
		
		BGroupLayout *passwordLayout;

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_SMALL_SPACING)
				.Add(CreateTextControlLayoutItem(fUserNameView = new BTextControl("userName", "User name:", "", NULL)))
				.Add(CreateTextControlLayoutItem(fRealNameView = new BTextControl("realName", "Real name:", "", NULL)))
				.Add(CreateMenuFieldLayoutItem(fGroupView = new BMenuField("group", "Group:", menu)))
				.Add(CreateTextControlLayoutItem(fHomeDirView = new BTextControl("homeDir", "Home directory:", "/boot/home", NULL)))
				.Add(CreateTextControlLayoutItem(fShellView = new BTextControl("shell", "Shell:", "/bin/sh", NULL)))
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
				.Add(fCancelView = new BButton("cancel", "Cancel", new BMessage(B_CANCEL)))
			.End()
		.End();

		if (fUid < 0) {
			fPasswordView->TextView()->HideTyping(true);
			fPasswordRepeatView->TextView()->HideTyping(true);
		} else {
			passwordLayout->SetVisible(false);
		}

		SetDefaultButton(fOkView);
		fUserNameView->MakeFocus();

		if (fUid >= 0) {
			const char *strPtr;
			int32 int32Val;
			KMessage userData;
			if (GetUser(userData, fUid, NULL) < B_OK) {
				PostMessage(B_QUIT_REQUESTED);
				return;
			}

			if (userData.FindString("name", &strPtr) >= B_OK) {fUserNameView->SetText(strPtr);}
			if (userData.FindString("real name", &strPtr) >= B_OK) {fRealNameView->SetText(strPtr);}
			if (userData.FindInt32("gid", &int32Val) >= B_OK) {
				for (int32 i = 0; i < fGroupView->Menu()->CountItems(); i++) {
					BMenuItem *item = fGroupView->Menu()->ItemAt(i);
					int32 group;
					if ((item->Message()->FindInt32("val", &group) >= B_OK) && (group == int32Val)) {
						item->SetMarked(true);
						break;
					}
				}
			}
			if (userData.FindString("home", &strPtr) >= B_OK) {fHomeDirView->SetText(strPtr);}
			if (userData.FindString("shell", &strPtr) >= B_OK) {fShellView->SetText(strPtr);}
		}
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case okMsg: {
			if (fUid < 0) {
				int expiration = 99999;
				int inactive = -1;
				int min = -1;
				int max = -1;
				int warn = 7;

				gid_t gid = 100;

				if (strcmp(fPasswordView->Text(), fPasswordRepeatView->Text()) != 0) {
					BAlert *alert = new BAlert("Users", "Passwords don't match.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					alert->Go(NULL);
					return;
				}
				const char *encryptedPassword;
				if (strcmp(fPasswordRepeatView->Text(), "") == 0)
					encryptedPassword = fPasswordRepeatView->Text();
				else
					encryptedPassword = crypt(fPasswordView->Text(), NULL);

				uid_t uid = 1000;
				while (getpwuid(uid) != NULL) uid++;

				BMenuItem *item = fGroupView->Menu()->FindMarked();
				if (item != NULL) {
					item->Message()->FindInt32("val", (int32*)&gid);
				}

				KMessage message;
				CheckRetVoid(message.AddInt32("uid", uid));
				CheckRetVoid(message.AddInt32("gid", gid));
				CheckRetVoid(message.AddString("name", fUserNameView->Text()));
				CheckRetVoid(message.AddString("password", "x"));
				CheckRetVoid(message.AddString("home", fHomeDirView->Text()));
				CheckRetVoid(message.AddString("shell", fShellView->Text()));
				CheckRetVoid(message.AddString("real name", fRealNameView->Text()));
				CheckRetVoid(message.AddString("shadow password", encryptedPassword));
				CheckRetVoid(message.AddInt32("last changed", time(NULL)));
				CheckRetVoid(message.AddInt32("min", min));
				CheckRetVoid(message.AddInt32("max", max));
				CheckRetVoid(message.AddInt32("warn", warn));
				CheckRetVoid(message.AddInt32("inactive", inactive));
				CheckRetVoid(message.AddInt32("expiration", expiration));
				CheckRetVoid(message.AddInt32("flags", 0));
				CheckRetVoid(message.AddBool("add user", true));
				CheckRetVoid(UpdateUser(message));
			} else {
				int32 gid = -1;

				BMenuItem *item = fGroupView->Menu()->FindMarked();
				if (item != NULL) {
					item->Message()->FindInt32("val", &gid);
				}
				KMessage message;
				CheckRetVoid(message.AddInt32("uid", fUid));
				if (gid >= 0) CheckRetVoid(message.AddInt32("gid", gid));
				CheckRetVoid(message.AddString("name", fUserNameView->Text()));
				CheckRetVoid(message.AddString("home", fHomeDirView->Text()));
				CheckRetVoid(message.AddString("shell", fShellView->Text()));
				CheckRetVoid(message.AddString("real name", fRealNameView->Text()));
				CheckRetVoid(message.AddInt32("last changed", time(NULL)));
				CheckRetVoid(UpdateUser(message));
			}
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			return;
		}

		BWindow::MessageReceived(msg);
	}
};


void ShowUserForm(int32 uid, BWindow *base)
{
	UserForm *wnd = new UserForm(uid);
	wnd->AddToSubset(base);
	wnd->CenterIn(base->Frame());
	wnd->Show();
}
