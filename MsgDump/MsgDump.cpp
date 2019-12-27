#include "MsgDump.h"
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
#include <TextControl.h>

enum {
	msgBase = 'm'<<24,
	expandAllMsg   = msgBase,
	collapseAllMsg,
	newFieldMsg,
	invokeMsg,
	selectMsg
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
		row->SetField(new BStringField("<binary>"), typeCol);
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
		row->SetField(new BStringField("<binary>"), typeCol);
		row->SetField(new BStringField(buf), valCol);
	}
}

void WriteValue(BColumnListView *view, BRow *upRow, type_code type, const void *data, ssize_t size)
{
	BString buf;
	switch (type) {
	case B_BOOL_TYPE: if (*(bool*)data != 0) buf.SetTo("true"); else buf.SetTo("false"); break;
	case B_FLOAT_TYPE:  buf.SetToFormat("%g",          *(float*)data); break;
	case B_DOUBLE_TYPE: buf.SetToFormat("%g",         *(double*)data); break;
	case B_INT8_TYPE:   buf.SetToFormat("%" B_PRId8,    *(int8*)data); break;
	case B_INT16_TYPE:  buf.SetToFormat("%" B_PRId16,  *(int16*)data); break;
	case B_INT32_TYPE:  buf.SetToFormat("%" B_PRId32,  *(int32*)data); break;
	case B_INT64_TYPE:  buf.SetToFormat("%" B_PRId64,  *(int64*)data); break;
	case B_UINT8_TYPE:  buf.SetToFormat("%" B_PRIu8,   *(uint8*)data); break;
	case B_UINT16_TYPE: buf.SetToFormat("%" B_PRIu16, *(uint16*)data); break;
	case B_UINT32_TYPE: buf.SetToFormat("%" B_PRIu32, *(uint32*)data); break;
	case B_UINT64_TYPE: buf.SetToFormat("%" B_PRIu64, *(uint64*)data); break;
	case B_STRING_TYPE: buf.SetTo      (                 (char*)data); break;
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
	case B_POINT_TYPE:     buf.SetToFormat("%g, %g", ((BPoint*)data)->x, ((BPoint*)data)->y); break;
	case B_RECT_TYPE:      buf.SetToFormat("%g, %g, %g, %g", ((BRect*)data)->left, ((BRect*)data)->top, ((BRect*)data)->right, ((BRect*)data)->bottom); break;
	case B_SIZE_TYPE:      buf.SetToFormat("%g, %g", ((BSize*)data)->width, ((BSize*)data)->height); break;
	case B_RGB_COLOR_TYPE: buf.SetToFormat("0x%08x", *(uint32*)data); break;
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
		buf.SetToFormat("<%d bytes>", size);
		upRow->SetField(new BStringField(buf), valCol);
		WriteData(view, upRow, (const uint8*)data, size);
		return;
	}
	upRow->SetField(new BStringField(buf), valCol);
}

void WriteFourCC(BString &buf, int32 code)
{
	char chars[4];
	int i;
	memcpy(chars, &code, 4);
	i = 0; while (
		(i < 4) && (
			((chars[i] >= 'A') && (chars[i] <= 'Z')) ||
			((chars[i] >= 'a') && (chars[i] <= 'z')) ||
			((chars[i] >= '0') && (chars[i] <= '9')) ||
			((chars[i] == '_'))
		)
	) i++;
	if (!(i < 4)) {
		buf.Append(" = '");
		buf.Append(chars, 4);
		buf.Append("'");
	}
}

void WriteMessage(BColumnListView *view, BRow *upRow, BMessage &msg)
{
	BString buf;
	BRow *row, *row2;
	char *name;
	type_code type;
	const void *data;
	ssize_t size;
	int32 count;
	buf.SetToFormat("%d\n", msg.what); WriteFourCC(buf, msg.what); upRow->SetField(new BStringField(buf), valCol);
	for (int32 i = 0; msg.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++) {
		row = new BRow();
		view->AddRow(row, upRow);
		row->SetField(new BStringField(name), nameCol);
		if (count > 1) {
			row->SetField(new BStringField("<array>"), typeCol);
			buf.SetToFormat("<%d items>\n", count); row->SetField(new BStringField(buf), valCol);
			for (int32 j = 0; j < count; j++) {
				row2 = new BRow();
				view->AddRow(row2, row);
				buf.SetToFormat("[%d]", j); row2->SetField(new BStringField(buf), nameCol);
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


// #pragma mark - EditWindow

EditWindow::EditWindow(TestWindow *base, BRect frame): BWindow(frame, "", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS), base(base)
{
	BRect rect, rect2;

	rect = frame.OffsetToCopy(B_ORIGIN);
	
	this->rootView = new BView(rect, "root", B_FOLLOW_ALL, 0);
	rootView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	this->AddChild(this->rootView);
	
	rect2 = rect; rect2.bottom = rect2.top;
	this->nameView = new BTextControl(rect2.InsetByCopy(4, 4), "name", "Name: ", "", NULL, B_FOLLOW_LEFT_RIGHT);
	this->nameView->SetDivider(48);
	this->rootView->AddChild(this->nameView);
	rect.top += this->nameView->Frame().Height() + 8;
	
	rect2 = rect; rect2.bottom = rect2.top;
	this->typeView = new BTextControl(rect2.InsetByCopy(4, 4), "type", "Type: ", "", NULL, B_FOLLOW_LEFT_RIGHT);
	this->typeView->SetDivider(48);
	this->rootView->AddChild(this->typeView);
	rect.top += this->typeView->Frame().Height() + 8;
	
	rect2 = rect; rect2.bottom = rect2.top;
	this->valueView = new BTextControl(rect2.InsetByCopy(4, 4), "value", "Value: ", "", NULL, B_FOLLOW_LEFT_RIGHT);
	this->valueView->SetDivider(48);
	this->rootView->AddChild(this->valueView);
	this->valueView->MakeFocus(true);
	rect.top += this->valueView->Frame().Height() + 8;
	
	rect.right -= 256;
	
	ResizeBy(-rect.Width(), -rect.Height());
	MoveBy(rect.Width()/2, rect.Height()/2);
}

void EditWindow::SetTo(BRow *row)
{
	BRow *parent;
	bool isVisible;
	const char *str;
	if (this->Lock()) {
		if (row == NULL) {
			this->SetTitle("New field");
			this->nameView->SetEnabled(true); this->nameView->TextView()->SetText("");
			this->typeView->SetEnabled(true); this->typeView->TextView()->SetText("");
			this->valueView->SetEnabled(true); this->valueView->TextView()->SetText("");
		} else {
			this->SetTitle("Edit field");
			if (!this->base->view->FindParent(row, &parent, &isVisible)) parent = NULL;
			this->nameView->SetEnabled(true);
			if ((parent != NULL) && (strcmp(((BStringField*)parent->GetField(typeCol))->String(), "<array>") == 0)) {
				this->nameView->SetEnabled(false);
			}
			if (strcmp(((BStringField*)row->GetField(typeCol))->String(), "<binary>") == 0) {
				this->nameView->SetEnabled(false);
			}
			this->valueView->SetEnabled(strcmp(((BStringField*)row->GetField(typeCol))->String(), "<array>") != 0);
			this->typeView->SetEnabled(false);
			this->nameView->TextView()->SetText(((BStringField*)row->GetField(nameCol))->String());
			this->typeView->TextView()->SetText(((BStringField*)row->GetField(typeCol))->String());
			this->valueView->TextView()->SetText(((BStringField*)row->GetField(valCol))->String());
			
		}
		this->Unlock();
	}
}

void EditWindow::Quit()
{
	if (base->Lock()) {
		base->editWnd = NULL;
		base->Unlock();
	}
	BWindow::Quit();
}


// #pragma mark - TestWindow

TestWindow::TestWindow(BRect frame): BWindow(frame, "", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	editWnd(NULL)
{
	BMenu *menu2;
	BMenuItem *it;
	BRect rect, rect2;

	rect = frame.OffsetToCopy(B_ORIGIN);
	
	this->rootView = new BView(rect, "root", B_FOLLOW_ALL, 0);
	rootView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	this->AddChild(this->rootView);
	
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
		menu2->AddItem(new BMenuItem("Undo", new BMessage('item'), 'Z'));
		menu2->AddItem(new BMenuItem("Redo", new BMessage('item'), 'Y'));
		menu2->AddItem(new BSeparatorItem());
		menu2->AddItem(new BMenuItem("Cut", new BMessage('item'), 'X'));
		menu2->AddItem(new BMenuItem("Copy", new BMessage('item'), 'C'));
		menu2->AddItem(new BMenuItem("Paste", new BMessage('item'), 'V'));
		menu2->AddItem(new BMenuItem("Delete", new BMessage('item')));
		menu2->AddItem(new BSeparatorItem());
		menu2->AddItem(new BMenuItem("New field", new BMessage(newFieldMsg)));
		menu2->AddItem(new BSeparatorItem());
		menu2->AddItem(new BMenuItem("Expand all", new BMessage(expandAllMsg)));
		menu2->AddItem(new BMenuItem("Collapse all", new BMessage(collapseAllMsg)));
		this->menu->AddItem(menu2);
	this->rootView->AddChild(this->menu);
	rect.top += this->menu->Frame().Height();
	
	rect2 = rect; rect2.bottom = rect2.top;
	this->pathView = new BTextControl(rect2.InsetByCopy(4, 4), "path", "Path: ", "", NULL, B_FOLLOW_LEFT_RIGHT);
	this->pathView->SetDivider(48);
	this->rootView->AddChild(this->pathView);
	rect.top += this->pathView->Frame().Height() + 8;
	
	this->view = new BColumnListView(rect.InsetByCopy(-1, -1), "view", B_FOLLOW_ALL, B_NAVIGABLE);
	this->rootView->AddChild(this->view, NULL);
	this->view->SetInvocationMessage(new BMessage(invokeMsg));
	this->view->SetSelectionMessage(new BMessage(selectMsg));

	view->AddColumn(new BStringColumn("Name", 250, 50, 500, B_TRUNCATE_MIDDLE), nameCol);
	view->AddColumn(new BStringColumn("Type", 96, 32, 500, B_TRUNCATE_MIDDLE), typeCol);
	view->AddColumn(new BStringColumn("Value", 256, 32, 500, B_TRUNCATE_MIDDLE), valCol);	
}

bool TestWindow::Load(BEntry &entry)
{
	BRow *row;
	BFile file(&entry, B_READ_ONLY);
	if (file.InitCheck() < B_OK) {(new BAlert("Error", "Can't open file.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(); return false;}
	if (this->message.Unflatten(&file) < B_OK) {
		// skip signature
		file.Seek(4, SEEK_SET);
		if (this->message.Unflatten(&file) < B_OK) {
			(new BAlert("Error", "Bad file format.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(); return false;
		}
	}
	this->SetTitle(entry.Name());
	
	this->view->Clear();
	row = new BRow();
	this->view->AddRow(row);
	view->ExpandOrCollapse(row, true);
	row->SetField(new BStringField("<root>"), nameCol);
	WriteType(this->view, row, B_MESSAGE_TYPE);
	WriteMessage(this->view, row, this->message);
	this->view->AddToSelection(row); this->PostMessage(new BMessage(selectMsg));
	return true;
}

void TestWindow::Quit()
{
	if (be_app->CountWindows() == 1) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	BWindow::Quit();
}

void TestWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case expandAllMsg: ExpandAll(this->view, this->view->RowAt(0)); break;
	case collapseAllMsg: CollapseAll(this->view, this->view->RowAt(0)); break;
	case newFieldMsg: {
		if (this->editWnd != NULL) {
			this->editWnd->Activate();
			this->editWnd->SetTo(NULL);
		} else {
			this->editWnd = new EditWindow(this, Frame());
			this->editWnd->SetTo(NULL);
			this->editWnd->Show();
		}
		break;
	}
	case invokeMsg: {
		if (this->editWnd != NULL) {
			this->editWnd->Activate();
			this->editWnd->SetTo(this->view->CurrentSelection(NULL));
		} else {
			this->editWnd = new EditWindow(this, Frame());
			this->editWnd->SetTo(this->view->CurrentSelection(NULL));
			this->editWnd->Show();
		}
		break;
	}
	case selectMsg: {
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
		this->pathView->TextView()->SetText(path);
		break;
	}
	default:
		BWindow::MessageReceived(msg);
	}
}


// #pragma mark - TestApplication

TestApplication::TestApplication(): BApplication("application/x-vnd.Test-MsgDump")
{
}

void TestApplication::RefsReceived(BMessage *refsMsg)
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

void TestApplication::ReadyToRun()
{
	if (be_app->CountWindows() == 0) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
