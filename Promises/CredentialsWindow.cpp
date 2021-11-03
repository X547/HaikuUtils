#include "CredentialsWindow.h"

#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <Button.h>


enum {
	loginBtnPressed = 1,
};


class CredentialsWindow: public BWindow
{
private:
	CredentialsPromiseRef fPromise;
	bool fResolved;
	BTextControl *fUserField;
	BTextControl *fPasswordField;
	BButton *fLoginBtn;

public:
	CredentialsWindow(CredentialsPromiseRef promise, BRect frame):
		BWindow(frame, "Login", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
		fPromise(promise),
		fResolved(false)
	{
		fUserField = new BTextControl("user", "User:", "", NULL);
		fPasswordField = new BTextControl("password", "Password:", "", NULL);
		fPasswordField->TextView()->HideTyping(true);
		fLoginBtn = new BButton("login", "Login", new BMessage(loginBtnPressed));

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.Add(fUserField->CreateLabelLayoutItem())
				.AddGlue()
				.End()
			.Add(fUserField->CreateTextViewLayoutItem())
			.AddStrut(B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL)
				.Add(fPasswordField->CreateLabelLayoutItem())
				.AddGlue()
				.End()
			.Add(fPasswordField->CreateTextViewLayoutItem())
			.AddStrut(B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fLoginBtn)
				.AddGlue()
				.End()
			.AddGlue()
			.End();

		fUserField->MakeFocus(true);
		SetDefaultButton(fLoginBtn);

		// close window if promise is set
		fPromise->OnSet([this](Promise <Credentials, status_t> &promise) {
			(void)promise;
			PostMessage(B_QUIT_REQUESTED);
		});
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case loginBtnPressed: {
			if (!fResolved) {
				fResolved = true;
				fPromise->Resolve(fUserField->Text(), fPasswordField->Text());
				// PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
		}
	}

	void Quit()
	{
		if (!fResolved)
			fPromise->Reject(B_ERROR);
		BWindow::Quit();
	}

};


CredentialsPromiseRef GetCredentials()
{
	CredentialsPromiseRef promise(new Promise<Credentials, status_t>(), true);
	CredentialsWindow *fWnd = new CredentialsWindow(promise, BRect(0, 0, 255, 0));
	fWnd->CenterOnScreen();
	fWnd->Show();
	return promise;
}
