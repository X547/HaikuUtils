#include <stdio.h>
#include <Message.h>
#include <String.h>
#include <Application.h>
#include <Window.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <View.h>
#include <Alert.h>
#include <Rect.h>
#include <OS.h>
#include <Entry.h>
#include <File.h>
#include "ColumnListView.h"
#include "ColumnTypes.h"

enum {
	msgBase = 'm'<<24,
	expandAllMsg   = msgBase + 0,
	collapseAllMsg = msgBase + 1,
	invokeMsg      = msgBase + 2,
	selectMsg      = msgBase + 3,
};

enum {
	nameCol = 0,
	typeCol = 1,
	valCol = 2
};

void WriteType(BColumnListView *view, BRow *upRow, type_code type)
{
	BString buf;
	switch (type) {
	case 'AMTX': buf.SetToFormat("AFFINE_TRANSFORM"); break;
	case 'ALGN': buf.SetToFormat("ALIGNMENT"); break;
	case 'ANYT': buf.SetToFormat("ANY"); break;
	case 'ATOM': buf.SetToFormat("ATOM"); break;
	case 'ATMR': buf.SetToFormat("ATOMREF"); break;
	case 'BOOL': buf.SetToFormat("BOOL"); break;
	case 'CHAR': buf.SetToFormat("CHAR"); break;
	case 'CLRB': buf.SetToFormat("COLOR_8_BIT"); break;
	case 'DBLE': buf.SetToFormat("DOUBLE"); break;
	case 'FLOT': buf.SetToFormat("FLOAT"); break;
	case 'GRYB': buf.SetToFormat("GRAYSCALE_8_BIT"); break;
	case 'SHRT': buf.SetToFormat("INT16"); break;
	case 'LONG': buf.SetToFormat("INT32"); break;
	case 'LLNG': buf.SetToFormat("INT64"); break;
	case 'BYTE': buf.SetToFormat("INT8"); break;
	case 'ICON': buf.SetToFormat("LARGE_ICON"); break;
	case 'BMCG': buf.SetToFormat("MEDIA_PARAMETER_GROUP"); break;
	case 'BMCT': buf.SetToFormat("MEDIA_PARAMETER"); break;
	case 'BMCW': buf.SetToFormat("MEDIA_PARAMETER_WEB"); break;
	case 'MSGG': buf.SetToFormat("MESSAGE"); break;
	case 'MSNG': buf.SetToFormat("MESSENGER"); break;
	case 'MIME': buf.SetToFormat("MIME"); break;
	case 'MICN': buf.SetToFormat("MINI_ICON"); break;
	case 'MNOB': buf.SetToFormat("MONOCHROME_1_BIT"); break;
	case 'OPTR': buf.SetToFormat("OBJECT"); break;
	case 'OFFT': buf.SetToFormat("OFF_T"); break;
	case 'PATN': buf.SetToFormat("PATTERN"); break;
	case 'PNTR': buf.SetToFormat("POINTER"); break;
	case 'BPNT': buf.SetToFormat("POINT"); break;
	case 'SCTD': buf.SetToFormat("PROPERTY_INFO"); break;
	case 'RAWT': buf.SetToFormat("RAW"); break;
	case 'RECT': buf.SetToFormat("RECT"); break;
	case 'RREF': buf.SetToFormat("REF"); break;
	case 'RGBB': buf.SetToFormat("RGB_32_BIT"); break;
	case 'RGBC': buf.SetToFormat("RGB_COLOR"); break;
	case 'SIZE': buf.SetToFormat("SIZE"); break;
	case 'SIZT': buf.SetToFormat("SIZE_T"); break;
	case 'SSZT': buf.SetToFormat("SSIZE_T"); break;
	case 'CSTR': buf.SetToFormat("STRING"); break;
	case 'STRL': buf.SetToFormat("STRING_LIST"); break;
	case 'TIME': buf.SetToFormat("TIME"); break;
	case 'USHT': buf.SetToFormat("UINT16"); break;
	case 'ULNG': buf.SetToFormat("UINT32"); break;
	case 'ULLG': buf.SetToFormat("UINT64"); break;
	case 'UBYT': buf.SetToFormat("UINT8"); break;
	case 'VICN': buf.SetToFormat("VECTOR_ICON"); break;
	case 'XATR': buf.SetToFormat("XATTR"); break;
	case 'NWAD': buf.SetToFormat("NETWORK_ADDRESS"); break;
	case 'MIMS': buf.SetToFormat("MIME_STRING"); break;
	case 'TEXT': buf.SetToFormat("ASCII"); break;
	default: buf.SetToFormat("?(%d)", type);
	}
	upRow->SetField(new BStringField(buf), typeCol);
}

void WriteString(BColumnListView *view, BRow *upRow, const char *str, ssize_t len)
{
	// TODO: escape
	BString dst;
	dst.Append("\"");
	dst.Append(str);
	dst.Append("\"");
	upRow->SetField(new BStringField(dst), valCol);
}

void WriteData(BColumnListView *view, BRow *upRow, const uint8 *data, ssize_t len)
{
	int ofs = 0;
	BString buf, buf2;
	BRow *row;
	while (len >= 16) {
		row = new BRow();
		view->AddRow(row, upRow);
		buf.SetToFormat("%08x", ofs); row->SetField(new BStringField(buf), nameCol);
		buf = "";
		for (int i = 0; i < 16; i++) {
			buf.Append(buf2.SetToFormat("%02x ", *data++));
			len--;
		}
		row->SetField(new BStringField(""), typeCol);
		row->SetField(new BStringField(buf), valCol);
		ofs += 16;
	}
	if (len > 0) {
		row = new BRow();
		view->AddRow(row, upRow);
		buf.SetToFormat("%08x", ofs); row->SetField(new BStringField(buf), nameCol);
		buf = "";
		while (len > 0) {
			buf.Append(buf2.SetToFormat("%02x ", *data++));
			len--;
		}
		row->SetField(new BStringField(""), typeCol);
		row->SetField(new BStringField(buf), valCol);
	}
}

void WriteValue(BColumnListView *view, BRow *upRow, type_code type, const void *data, ssize_t size)
{
	char buf[256];
	switch (type) {
	case B_BOOL_TYPE: if (*(bool*)data != 0) sprintf(buf, "true"); else sprintf(buf, "false"); break;
	case B_FLOAT_TYPE:  sprintf(buf, "%g",          *(float*)data); break;
	case B_DOUBLE_TYPE: sprintf(buf, "%g",         *(double*)data); break;
	case B_INT8_TYPE:   sprintf(buf, "%" B_PRId8,    *(int8*)data); break;
	case B_INT16_TYPE:  sprintf(buf, "%" B_PRId16,  *(int16*)data); break;
	case B_INT32_TYPE:  sprintf(buf, "%" B_PRId32,  *(int32*)data); break;
	case B_INT64_TYPE:  sprintf(buf, "%" B_PRId64,  *(int64*)data); break;
	case B_UINT8_TYPE:  sprintf(buf, "%" B_PRIu8,   *(uint8*)data); break;
	case B_UINT16_TYPE: sprintf(buf, "%" B_PRIu16, *(uint16*)data); break;
	case B_UINT32_TYPE: sprintf(buf, "%" B_PRIu32, *(uint32*)data); break;
	case B_UINT64_TYPE: sprintf(buf, "%" B_PRIu64, *(uint64*)data); break;
	case B_STRING_TYPE: WriteString(view, upRow, (const char*)data, size); return; break;
	//case B_STRING_LIST_TYPE: break;
	//case B_AFFINE_TRANSFORM_TYPE: break;
	//case B_ALIGNMENT_TYPE: break;
	//case B_ANY_TYPE: break;
	//case B_ATOM_TYPE: break;
	//case B_ATOMREF_TYPE: break;
	//case B_CHAR_TYPE: break;
	//case B_COLOR_8_BIT_TYPE: break;
	//case B_GRAYSCALE_8_BIT_TYPE: break;
	//case B_LARGE_ICON_TYPE: break;
	//case B_MEDIA_PARAMETER_GROUP_TYPE: break;
	//case B_MEDIA_PARAMETER_TYPE: break;
	//case B_MEDIA_PARAMETER_WEB_TYPE: break;
	//case B_MESSAGE_TYPE: break;
	//case B_MESSENGER_TYPE: break;
	//case B_MIME_TYPE: break;
	//case B_MINI_ICON_TYPE: break;
	//case B_MONOCHROME_1_BIT_TYPE: break;
	//case B_OBJECT_TYPE: break;
	//case B_OFF_T_TYPE: break;
	//case B_PATTERN_TYPE: break;
	//case B_POINTER_TYPE: break;
	case B_POINT_TYPE: sprintf(buf, "(%g, %g)", ((BPoint*)data)->x, ((BPoint*)data)->y); break;
	case B_RECT_TYPE: sprintf(buf, "(%g, %g, %g, %g)", ((BRect*)data)->left, ((BRect*)data)->top, ((BRect*)data)->right, ((BRect*)data)->bottom); break;
	case B_SIZE_TYPE: sprintf(buf, "(%g, %g)", ((BSize*)data)->width, ((BSize*)data)->height); break;
	case B_RGB_COLOR_TYPE: sprintf(buf, "0x%08x", *(uint32*)data); break;
	//case B_PROPERTY_INFO_TYPE: break;
	//case B_RAW_TYPE: break;
	//case B_REF_TYPE: break;
	//case B_RGB_32_BIT_TYPE: break;
	//case B_SIZE_T_TYPE: break;
	//case B_SSIZE_T_TYPE: break;
	//case B_TIME_TYPE: break;
	//case B_VECTOR_ICON_TYPE: break;
	//case B_XATTR_TYPE: break;
	//case B_NETWORK_ADDRESS_TYPE: break;
	//case B_MIME_STRING_TYPE: break;
	//case B_ASCII_TYPE: break;
	default:
		sprintf(buf, "<%d bytes>", size);
		upRow->SetField(new BStringField(buf), valCol);
		WriteData(view, upRow, (const uint8*)data, size);
		return;
	}
	upRow->SetField(new BStringField(buf), valCol);
}

void WriteMessage(BColumnListView *view, BRow *upRow, BMessage &msg)
{
	char buf[256];
	BRow *row, *row2;
	char *name;
	type_code type;
	const void *data;
	ssize_t size;
	int32 count;
	sprintf(buf, "%d\n", msg.what); upRow->SetField(new BStringField(buf), valCol);
	for (int32 i = 0; msg.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++) {
		row = new BRow();
		view->AddRow(row, upRow);
		row->SetField(new BStringField(name), nameCol);
		if (count > 1) {
			row->SetField(new BStringField("<array>"), typeCol);
			sprintf(buf, "<%d items>\n", count); row->SetField(new BStringField(buf), valCol);
			for (int32 j = 0; j < count; j++) {
				row2 = new BRow();
				view->AddRow(row2, row);
				sprintf(buf, "[%d]", j); row2->SetField(new BStringField(buf), nameCol);
				msg.FindData(name, B_ANY_TYPE, j, &data, &size);
				WriteType(view, row2, type);
				if (type == B_MESSAGE_TYPE) {
					BMessage msg2;
					msg.FindMessage(name, j, &msg2);
					WriteMessage(view, row2, msg2);
				} else {
					WriteValue(view, row2, type, data, size);
				}
			}
		} else {
			msg.FindData(name, B_ANY_TYPE, 0, &data, &size);
			WriteType(view, row, type);
			if (type == B_MESSAGE_TYPE) {
				BMessage msg2;
				msg.FindMessage(name, 0, &msg2);
				WriteMessage(view, row, msg2);
			} else {
				WriteValue(view, row, type, data, size);
			}
		}
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

class TestWindow: public BWindow
{
private:
	BMenuBar *menu;
	BColumnListView *view;
	BMessage message;
	
public:
	TestWindow(BRect frame): BWindow(frame, "Window", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		BMenu *menu2;
		BMenuItem *it;
		this->menu = new BMenuBar(BRect(0, 0, 32, 32), "menu", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_ITEMS_IN_ROW, true);
		menu2 = new BMenu("File");
			menu2->AddItem(new BMenuItem("New", new BMessage('item'), 'N'));
			menu2->AddItem(new BMenuItem("Open...", new BMessage('item'), 'O'));
			menu2->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
			menu2->AddItem(new BSeparatorItem());
			menu2->AddItem(new BMenuItem("Save", new BMessage('item'), 'S'));
			menu2->AddItem(new BMenuItem("Save as...", new BMessage('item'), 'S', B_SHIFT_KEY));
			menu2->AddItem(new BSeparatorItem());
			menu2->AddItem(it = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q')); it->SetTarget(be_app);
			this->menu->AddItem(menu2);
		menu2 = new BMenu("Edit");
			menu2->AddItem(new BMenuItem("Expand all", new BMessage(expandAllMsg)));
			menu2->AddItem(new BMenuItem("Collapse all", new BMessage(collapseAllMsg)));
			this->menu->AddItem(menu2);
		this->AddChild(this->menu);
		
		this->view = new BColumnListView(BRect(BPoint(0, this->menu->Frame().Height()), frame.Size()).InsetByCopy(-1, -1), "view", B_FOLLOW_ALL, 0);
		this->AddChild(this->view, NULL);
		this->view->SetInvocationMessage(new BMessage(invokeMsg));
		this->view->SetSelectionMessage(new BMessage(selectMsg));

		view->AddColumn(new BStringColumn("Name", 250, 50, 500, B_TRUNCATE_MIDDLE), nameCol);
		view->AddColumn(new BStringColumn("Type", 96, 32, 500, B_TRUNCATE_MIDDLE), typeCol);
		view->AddColumn(new BStringColumn("Value", 128, 32, 500, B_TRUNCATE_MIDDLE), valCol);	
	}
	
	bool Load(BEntry &entry)
	{
		BRow *row;
		BFile file(&entry, B_READ_ONLY);
		if (file.InitCheck() < B_OK) {(new BAlert("Error", "Can't open file.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(); return false;}
		if (this->message.Unflatten(&file) < B_OK) {(new BAlert("Error", "Bad file format.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(); return false;}
		this->SetTitle(entry.Name());
		
		this->view->Clear();
		row = new BRow();
		this->view->AddRow(row);
		view->ExpandOrCollapse(row, true);
		row->SetField(new BStringField("<root>"), nameCol);
		WriteType(this->view, row, B_MESSAGE_TYPE);
		WriteMessage(this->view, row, this->message);
		return true;
	}
	
	void Quit()
	{
		if (be_app->CountWindows() == 1) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
		BWindow::Quit();
	}
	
	void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
		case expandAllMsg: ExpandAll(this->view, this->view->RowAt(0)); break;
		case collapseAllMsg: CollapseAll(this->view, this->view->RowAt(0)); break;
		case invokeMsg: {
			BRow *row, *parent;
			bool isVisible;
			const char *str;
			BString path;
			row = this->view->CurrentSelection(NULL);
			if (row == NULL) {return;}
			path = ((BStringField*)row->GetField(nameCol))->String();
			while (this->view->FindParent(row, &parent, &isVisible)) {
				str = ((BStringField*)parent->GetField(nameCol))->String();
				path.Prepend("/");
				path.Prepend(str);
				row = parent;
			}
			(new BAlert("Path", path, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT))->Go();
			break;
		}
		case selectMsg:
			//msg->PrintToStream();
			break;
		default:
			BWindow::MessageReceived(msg);
		}
	}
};

class TestApplication: public BApplication
{
public:
	TestApplication(): BApplication("application/x-vnd.Test-MsgDump")
	{
	}
	
	void RefsReceived(BMessage *refsMsg)
	{
		entry_ref ref;
		if (refsMsg->FindRef("refs", &ref) < B_OK) {(new BAlert("Error", "No \"be:refs\" field.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(); return;}
		BEntry entry(&ref);
		
		BRect rect(0, 0, 640, 480);
		rect.OffsetBy(64, 64);
		TestWindow *wnd = new TestWindow(rect);
		if (!wnd->Load(entry)) {wnd->PostMessage(B_QUIT_REQUESTED); return;}
		wnd->Show();
	}
	
	void ReadyToRun()
	{
		if (be_app->CountWindows() == 0) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
	}
	
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}

/*
gcc -g -lbe MsgDump.cpp /boot/system/develop/lib/libcolumnlistview.a -o MsgDump
MsgDump "/boot/data/packages/haiku/data/artwork/GET HAIKU - download box 2"
*/
