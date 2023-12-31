#ifndef _MSGDUMP_H_
#define _MSGDUMP_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <String.h>
#include <ObjectList.h>

class TestWindow;
class BTextControl;
class BButton;
class BColumnListView;
class BRow;
class BEntry;

struct Operation
{
	enum Kind {
		insertOp,
		removeOp,
		updateOp
	};
	BString path;
	Kind kind;
	int8 *data;
	ssize_t size;
	BMessage msg;

	Operation() {}
	Operation(BString &path, Kind kind, const int8 *data, ssize_t size): path(path), kind(kind), size(size) {
		if (size <= 0) {
			this->data = NULL;
		} else {
			this->data = new int8[size];
			memcpy(this->data, data, size);
		}
	}
	~Operation() {if (data != NULL) delete[] data;}
};

struct MessageLoc
{
	BMessage &msg;
	BString name;
	int32 idx;
	int32 ofs;
	bool useOfs;
};

class EditWindow: public BWindow
{
private:
	TestWindow *fBase;
	BTextControl *fNameView, *fTypeView, *fValueView;
	BButton *fOkView, *fCancelView;

public:
	EditWindow(TestWindow *base);
	void SetTo(BRow *row);
	void Quit();
};

class TestWindow: public BWindow
{
private:
	BMenuBar *fMenuBar;
	BTextControl *fPathView;
	BColumnListView *fView;
	EditWindow *fEditWnd;
	BMessage fMessage;

	BObjectList<Operation> fUndoStack, fRedoStack;

	friend class EditWindow;

public:
	TestWindow(BRect frame);
	bool Load(BEntry &entry);
	void Quit();
	void MessageReceived(BMessage* msg);
};

class TestApplication: public BApplication
{
public:
	TestApplication();
	void ArgvReceived(int32 argc, char** argv);
	void RefsReceived(BMessage *refsMsg);
	void ReadyToRun();
};

#endif	// _MSGDUMP_H_
