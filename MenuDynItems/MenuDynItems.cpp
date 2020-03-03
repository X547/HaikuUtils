#include <stdio.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuBar.h>
#include <String.h>

void BuildMenu(BMenu *dst, int level)
{
	char buf[64];
	int cnt = 4;
	BMenu *subMenu;
	if (level == 0) {
		dst->AddItem(new BMenuItem("Menu item", new BMessage('item')));
		subMenu = new BMenu("Long menu");
		dst->AddItem(subMenu);
		for (int i = 0; i < 200; ++i) {
				sprintf(buf, "Item %d", i);
				subMenu->AddItem(new BMenuItem(buf, new BMessage('item')));
		}
		cnt = 16;
	}
	for (int i = 0; i < cnt; ++i) {
		if (level < 3) {
			sprintf(buf, "%d", i);
			subMenu = new BMenu(buf);
			BuildMenu(subMenu, level + 1);
			dst->AddItem(subMenu);
		} else {
			dst->AddItem(new BMenuItem("Item", new BMessage('item')));
		}
	}
}

class TestMenuBar: public BMenuBar
{
public:
	TestMenuBar(): BMenuBar(BRect(0, 0, 32, 32), "menu", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_ITEMS_IN_ROW, true)
	{
		BuildMenu(this, 0);
		fDynItem = ItemAt(2)->Submenu()->ItemAt(1);
	}

	void UpdateDynItems(int32 modifiers)
	{
		if (((modifiers & B_SHIFT_KEY) != 0) != fModified) {
			BMenu *menu = fDynItem->Menu();
			if (menu->Window() == NULL || menu->LockLooper()) {
				fModified = !fModified;
				if (fModified) {
					fOrigText = fDynItem->Label();
					BString buf;
					buf += fDynItem->Label();
					buf += " (Shift)";
					fDynItem->SetLabel(buf);
				} else {
					fDynItem->SetLabel(fOrigText);
				}
				int32 index = menu->IndexOf(fDynItem);
				menu->RemoveItem(fDynItem);
				menu->AddItem(fDynItem, index);
				if (menu->Window() != NULL)
					menu->UnlockLooper();
			}
		}
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
			case B_MODIFIERS_CHANGED: {
				int32 modifiers = 0;
				msg->FindInt32("modifiers", &modifiers);
				UpdateDynItems(modifiers);
				break;
			}
		}
		BMenuBar::MessageReceived(msg);
	}

private:
	BMenuItem *fDynItem;
	bool fModified;
	BString fOrigText;
};

class TestWindow: public BWindow
{
private:
	TestMenuBar *fMenu;
public:
	TestWindow(BRect frame): BWindow(frame, "TestApp", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		fMenu = new TestMenuBar();
		this->AddChild(fMenu);
	}

	~TestWindow()
	{
	}

	void MenusBeginning()
	{
		fMenu->SetEventMask(fMenu->EventMask() | B_KEYBOARD_EVENTS);
		fMenu->UpdateDynItems(modifiers());
	}

	void MenusEnded()
	{
		fMenu->SetEventMask(fMenu->EventMask() & ~B_KEYBOARD_EVENTS);
	}

};

class TestApplication: public BApplication
{
public:
	TestApplication(): BApplication("application/x-vnd.test.app")
	{
	}

	void ReadyToRun() {
		BWindow *wnd = new TestWindow(BRect(32, 32, 512, 256));
		wnd->SetFlags(wnd->Flags() | B_QUIT_ON_WINDOW_CLOSE);
		wnd->Show();
	}
};


int main()
{
	(new TestApplication())->Run();
	return 0;
}

/*
gcc -lbe -lstdc++ MenuDyn.cpp -o MenuDyn
MenuDyn
*/
