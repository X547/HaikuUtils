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


class PasswordForm: public BWindow
{
private:
	int32 fUid;

	BTextControl *fPasswordView;
	BTextControl *fPasswordRepeatView;

	BButton *fOkView;
	BButton *fCancelView;

public:
	PasswordForm(int32 uid):
		BWindow(
			BRect(0, 0, 255, 0), "Set password", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_CLOSE_ON_ESCAPE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
		),
		fUid(uid)
	{

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_SMALL_SPACING)
				.Add(CreateTextControlLayoutItem(fPasswordView = new BTextControl("password", "Password:", "", NULL)))
				.Add(CreateTextControlLayoutItem(fPasswordRepeatView = new BTextControl("passwordRepeat", "Repeat password:", "", NULL)))
				.End()
			.Add(new BSeparatorView(B_HORIZONTAL))
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_SMALL_SPACING)
				.AddGlue()
				.Add(fOkView = new BButton("ok", "OK", new BMessage(okMsg)))
				.Add(fCancelView = new BButton("cancel", "Cancel", new BMessage(B_CANCEL)))
				.End()
			.End()
		;

		fPasswordView->TextView()->HideTyping(true);
		fPasswordRepeatView->TextView()->HideTyping(true);

		SetDefaultButton(fOkView);
		fPasswordView->MakeFocus();
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case okMsg: {
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

			KMessage message;
			if (message.AddInt32("uid", fUid) != B_OK
				|| message.AddInt32("last changed", time(NULL)) != B_OK
				|| message.AddString("password", "x") != B_OK
				|| message.AddString("shadow password", encryptedPassword) != B_OK
			) return;

			UpdateUser(message);

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


void ShowPasswordForm(int32 uid, BWindow *base)
{
	PasswordForm *wnd = new PasswordForm(uid);
	wnd->AddToSubset(base);
	wnd->CenterIn(base->Frame());
	wnd->Show();
}
