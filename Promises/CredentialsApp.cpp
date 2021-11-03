#include <stdio.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <String.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <Button.h>

#include "Promises.h"
#include "CredentialsWindow.h"

enum {
	loginBtnPressed = 1,
	cancelBtnPressed = 2,
};


class TestWindow: public BWindow
{
private:
	BTextControl *fUserField;
	BTextControl *fPasswordField;
	BButton *fLoginBtn;
	BButton *fCancelBtn;
	CredentialsPromiseRef fPromise;

public:
	TestWindow(BRect frame):
		BWindow(frame, "Login Request", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		fUserField = new BTextControl("user", "User:", "", NULL);
		fPasswordField = new BTextControl("password", "Password:", "", NULL);
		fLoginBtn = new BButton("login", "Request", new BMessage(loginBtnPressed));
		fCancelBtn = new BButton("cancel", "Cancel", new BMessage(cancelBtnPressed));

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
				.Add(fCancelBtn)
				.AddGlue()
				.End()
			.AddGlue()
			.End();

		fUserField->SetEnabled(false);
		fPasswordField->SetEnabled(false);
		fLoginBtn->MakeFocus(true);
		fCancelBtn->SetEnabled(false);
		SetDefaultButton(fLoginBtn);
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case loginBtnPressed: {
			CredentialsPromiseRef promise = GetCredentials();
			fPromise = promise;
			fLoginBtn->SetEnabled(false);
			fCancelBtn->SetEnabled(true);

			promise->OnResolve([this](Promise <Credentials, status_t> &promise) {
				printf("GetCredentials done: \"%s\", \"%s\"\n", promise.ResolvedValue().login.String(), promise.ResolvedValue().password.String());
			});
			promise->OnResolve([this](Promise <Credentials, status_t> &promise) {
				BAutolock lock(this);
				fUserField->SetText(promise.ResolvedValue().login);
				fPasswordField->SetText(promise.ResolvedValue().password);
				fLoginBtn->SetEnabled(true);
				fCancelBtn->SetEnabled(false);
				fPromise.Unset();
			});

			promise->OnReject([this](Promise <Credentials, status_t> &promise) {
				printf("GetCredentials failed: %d\n", promise.RejectedValue());
			});
			promise->OnReject([this](Promise <Credentials, status_t> &promise) {
				(void)promise;
				BAutolock lock(this);
				fLoginBtn->SetEnabled(true);
				fCancelBtn->SetEnabled(false);
				fPromise.Unset();
			});
			break;
		}
		case cancelBtnPressed: {
			fPromise->Reject(B_ERROR);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
		}
	}

};


class TestApplication: public BApplication
{
private:
	TestWindow *fWnd;
public:
	TestApplication(): BApplication("application/x-vnd.test.app")
	{
		TestWindow *fWnd = new TestWindow(BRect(0, 0, 255, 0));
		fWnd->SetFlags(fWnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		fWnd->CenterOnScreen();
		fWnd->MoveBy(-192, -192);
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
