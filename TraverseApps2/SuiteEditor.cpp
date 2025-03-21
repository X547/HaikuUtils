#include "SuiteEditor.h"

#include <PropertyInfo.h>

#include <String.h>
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/app/MessengerPrivate.h>

#include "ScriptingUtils.h"

#include <map>
#include <set>

enum {
	nameCol,
	cmdCol,
	specCol,
	typeCol,
	valueCol
};

class Property
{
public:
	std::set<uint32> commands;
	std::set<uint32> specifiers;
	std::set<uint32> types;

	Property(const property_info &info)
	{
		for (int32 i = 0; i < 10 && info.commands[i] != 0; i++)
			commands.insert(info.commands[i]);
		for (int32 i = 0; i < 10 && info.specifiers[i] != 0; i++)
			specifiers.insert(info.specifiers[i]);
		for (int32 i = 0; i < 10 && info.types[i] != 0; i++)
			types.insert(info.types[i]);
	}

	void Include(const Property &src)
	{
		commands.insert(src.commands.begin(), src.commands.end());
		specifiers.insert(src.specifiers.begin(), src.specifiers.end());
		types.insert(src.types.begin(), src.types.end());
	}

};

class PropertyList
{
public:
	std::map<BString, Property> props;

	PropertyList(const property_info *infoList)
	{
		for (int32 i = 0; infoList[i].name != NULL; i++) {
			BString name(infoList[i].name);
			Property src(infoList[i]);
			auto it = props.find(name);
			if (it == props.end()) {
				props.insert(std::pair<BString, Property>(name, src));
			} else {
				Property &prop = (*it).second;
				prop.Include(src);
			}
		}
	}

};


static void WriteType(BString &buf, type_code type)
{
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
}

static void WriteData(BColumnListView *view, BRow *upRow, const uint8 *data, ssize_t len)
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
		row->SetField(new BStringField(buf), valueCol);
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
		row->SetField(new BStringField(buf), valueCol);
	}
}

static void WriteValue(BColumnListView *view, BRow *upRow, type_code type, const void *data, ssize_t size)
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
	case B_MESSENGER_TYPE: {
		BString buf2;
		buf2.SetToFormat("port: %ld", *(int32*)data); buf.Append(buf2); ((int32*&)data)++;
		buf2.SetToFormat(", token: %ld", *(int32*)data); buf.Append(buf2); ((int32*&)data)++;
		buf2.SetToFormat(", team: %ld", *(int32*)data); buf.Append(buf2); ((int32*&)data)++;
		break;
	}
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
	case B_REF_TYPE: {
		BString buf2;
		buf2.SetToFormat("dev: %ld", *(int32*)data); buf.Append(buf2); ((int32*&)data)++;
		buf2.SetToFormat(", inode: %lld", *(int64*)data); buf.Append(buf2); ((int64*&)data)++;
		buf2.SetToFormat(", name: \"%s\"", (char*)data); buf.Append(buf2);
		break;
	}
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
		upRow->SetField(new BStringField(buf), valueCol);
		WriteData(view, upRow, (const uint8*)data, size);
		return;
	}
	upRow->SetField(new BStringField(buf), valueCol);
}

static void WritePropertyCmds(BString &str, const std::set<uint32> &cmds)
{
	bool first = true;
	BString buf;
	str = "";
	for (auto it = cmds.begin(); it != cmds.end(); it++) {
		if (first) first = false; else str += ", ";
		switch(*it) {
		case B_SET_PROPERTY: str += "SET"; break;
		case B_GET_PROPERTY: str += "GET"; break;
		case B_CREATE_PROPERTY: str += "CREATE"; break;
		case B_DELETE_PROPERTY: str += "DELETE"; break;
		case B_COUNT_PROPERTIES: str += "COUNT"; break;
		case B_EXECUTE_PROPERTY: str += "EXECUTE"; break;
		case B_GET_SUPPORTED_SUITES: str += "GET_SUITES"; break;
		default: buf.SetToFormat("? (%d)", *it); str += buf;
		}
	}
}

static void WriteSpecifiers(BString &str, const std::set<uint32> &specs)
{
	bool first = true;
	BString buf;
	str = "";
	for (auto it = specs.begin(); it != specs.end(); it++) {
		if (first) first = false; else str += ", ";
		switch(*it) {
		case B_NO_SPECIFIER: str += "NO"; break;
		case B_DIRECT_SPECIFIER: str += "DIRECT"; break;
		case B_INDEX_SPECIFIER: str += "INDEX"; break;
		case B_REVERSE_INDEX_SPECIFIER: str += "REV_INDEX"; break;
		case B_RANGE_SPECIFIER: str += "RANGE"; break;
		case B_REVERSE_RANGE_SPECIFIER: str += "REV_RANGE"; break;
		case B_NAME_SPECIFIER: str += "NAME"; break;
		case B_ID_SPECIFIER: str += "ID"; break;
		default: buf.SetToFormat("? (%d)", *it); str += buf;
		}
	}
}

static void WritePropertyTypes(BString &str, const std::set<uint32> &types)
{
	bool first = true;
	BString buf;
	str = "";
	for (auto it = types.begin(); it != types.end(); it++) {
		if (first) first = false; else str += ", ";
		WriteType(buf, *it); str += buf;
	}
}

static bool HasPropertyCommand(const std::set<uint32> &cmds, uint32 cmd)
{
	auto it = cmds.find(cmd);
	return it != cmds.end();
}

static void WriteScriptingResult(BColumnListView *view, BRow *upRow, const BMessage &reply)
{
	char *name;
	type_code type;
	const void *data;
	ssize_t size;
	int32 count;
	BString buf;
	status_t error;
/*
	if (reply.FindInt32("error", &error) < B_OK) {
		upRow->SetField(new BStringField("<no \"error\" field>"), valueCol);
		return;
	}
*/
	if (reply.FindInt32("error", &error) >= B_OK && error < B_OK) {
		WriteError(buf, error);
		upRow->SetField(new BStringField(buf), valueCol);
		return;
	}
	for (int32 i = 0; reply.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++) {
		if (strcmp(name, "result") == 0) {
			reply.FindData(name, B_ANY_TYPE, 0, &data, &size);
			WriteType(buf, type); upRow->SetField(new BStringField(buf), typeCol);
			WriteValue(view, upRow, type, data, size);
			return;
		}
	}
	upRow->SetField(new BStringField("<no \"result\" field>"), valueCol);
}

void AppendSpecifiers(BMessage &msg, const BMessage &src)
{
	BMessage spec;
	for (int32 i = 0; src.FindMessage("specifiers", i, &spec) >= B_OK; i++) {
		msg.AddSpecifier(&spec);
	}
}

static void ListObjects(BColumnListView *listView, BRow *parent, const BMessenger &obj, const char *propName)
{
	printf("ListObjects(%s)\n", propName);
	status_t res = B_OK;
	BString buf;
	BMessage spec;
	BRow *row;
	int32 count;
	BMessenger obj2;

	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier(propName);
	if (GetInt32(count, obj, spec) != B_OK)
		count = 0;
	if (count <= 0) {
		row = new BRow();
		listView->AddRow(row, parent);
		row->SetField(new BStringField("<empty>"), nameCol);
		row->SetField(new BStringField(""), cmdCol);
		row->SetField(new BStringField(""), specCol);
		row->SetField(new BStringField(""), typeCol);
		row->SetField(new BStringField(""), valueCol);
	}
	for (int i = 0; i < count; i++) {
		row = new BRow();
		listView->AddRow(row, parent);

		buf.SetToFormat("%d", i); row->SetField(new BStringField(buf), nameCol);
		row->SetField(new BStringField(""), cmdCol);
		row->SetField(new BStringField(""), specCol);
		row->SetField(new BStringField("MESSENGER"), typeCol);

		spec = BMessage(B_GET_PROPERTY);
		spec.AddSpecifier(propName, i);
		BMessage reply;
		res = SendScriptingMessage(obj, spec, reply);
		if (res < B_OK) {
			buf.SetToFormat("<%s>", strerror(res)); row->SetField(new BStringField(buf), valueCol);
		} else
			WriteScriptingResult(listView, row, reply);
	}
}

static void ListProps(BColumnListView *listView, BRow *parent, const BMessenger &obj, const PropertyList &propList) {
	status_t res = B_OK;
	BString buf;
	BMessage spec;
	BRow *row;

	for (auto it = propList.props.begin(); it != propList.props.end(); it++) {
		const BString &name = (*it).first;
		const Property &prop = (*it).second;

		row = new BRow();
		listView->AddRow(row, parent);

		row->SetField(new BStringField(name), nameCol);
		WritePropertyCmds(buf, prop.commands); row->SetField(new BStringField(buf), cmdCol);
		WriteSpecifiers(buf, prop.specifiers); row->SetField(new BStringField(buf), specCol);
		WritePropertyTypes(buf, prop.types); row->SetField(new BStringField(buf), typeCol);
		row->SetField(new BStringField(""), valueCol);

		if (HasPropertyCommand(prop.commands, B_GET_PROPERTY)) {
			printf("has B_GET_PROPERTY\n");
			BMessage spec(B_GET_PROPERTY);
			if (HasPropertyCommand(prop.specifiers, B_RANGE_SPECIFIER)) {
				int32 count;
				{
					BMessage spec(B_COUNT_PROPERTIES);
					spec.AddSpecifier(name);
					if (GetInt32(count, obj, spec) < B_OK) count = 0;
				}
				spec.AddSpecifier(name, 0, count);
			} else
				spec.AddSpecifier(name);
			BMessage reply;
			res = SendScriptingMessage(obj, spec, reply);
			if (res >= B_OK) WriteScriptingResult(listView, row, reply);
		} else if (HasPropertyCommand(prop.commands, B_COUNT_PROPERTIES)) {
			printf("has B_COUNT_PROPERTIES\n");
			BMessage spec(B_COUNT_PROPERTIES);
			spec.AddSpecifier(name);
			BMessage reply;
			res = SendScriptingMessage(obj, spec, reply);
			if (res >= B_OK) WriteScriptingResult(listView, row, reply);
			if (HasPropertyCommand(prop.specifiers, B_INDEX_SPECIFIER)) {
				printf("has B_INDEX_SPECIFIER\n");
				ListObjects(listView, row, obj, name);
			}
		}
	}
}

static void ListSuites(BColumnListView *listView, BRow *parent, const BMessenger &obj, const BMessage &suites) {
	status_t res = B_OK;
	BMessage spec;
	BRow *row;

	const char *suiteName;
	for (int32 i = 0; suites.FindString("suites", i, &suiteName) >= B_OK; i++) {
		BPropertyInfo propInfo;
		res = suites.FindFlat("messages", i, &propInfo);
		if (res < B_OK) continue;
		PropertyList propList(propInfo.Properties());

		row = new BRow();
		listView->AddRow(row, parent);

		row->SetField(new BStringField(suiteName), nameCol);
		row->SetField(new BStringField(""), cmdCol);
		row->SetField(new BStringField(""), typeCol);
		row->SetField(new BStringField(""), valueCol);

		ListProps(listView, row, obj, propList);
	}
}


int32 indent = 0;

void Indent()
{
	for (int32 i = 0; i < indent; i++)
		printf("\t");
}

static void TraverseObject(const BMessenger &obj, const BMessage &path);

static void TraverseObjects(const BMessenger &obj, const BMessage &path, const char *prop)
{
	status_t res = B_OK;
	BMessage spec;
	int32 count;
	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier(prop);
	AppendSpecifiers(spec, path);
	res = GetInt32(count, obj, spec);
	if (res < B_OK) {
		Indent(); printf("error: %s\n", strerror(res));
		return;
	}
	for (int32 i = 0; i < count; i++) {
		Indent(); printf("%d\n", i);
		indent++;
		spec = BMessage();
		spec.AddSpecifier(prop, i);
		AppendSpecifiers(spec, path);
		TraverseObject(obj, spec);
		indent--;
	}
}

static void TraverseProps(const BMessenger &obj, const BMessage &path, const PropertyList &propList) {
	BString buf;
	BMessage spec;

	for (auto it = propList.props.begin(); it != propList.props.end(); it++) {
		const BString &name = (*it).first;
		const Property &prop = (*it).second;

		Indent(); printf("%s", name.String());

		if (HasPropertyCommand(prop.commands, B_GET_PROPERTY)) {
			spec = BMessage(B_GET_PROPERTY);
			spec.AddSpecifier(name);
			AppendSpecifiers(spec, path);
			int32 int32val;
			bool boolVal;
			BString strVal;
			if (GetInt32(int32val, obj, spec) >= B_OK) {
				printf(": %d", int32val);
			} else if (GetBool(boolVal, obj, spec) >= B_OK) {
				printf(": %s", boolVal? "true": "false");
			} else if (GetString(strVal, obj, spec) >= B_OK) {
				printf(": \"%s\"", strVal.String());
			}
		}
		printf("\n");

		if (HasPropertyCommand(prop.commands, B_COUNT_PROPERTIES) && HasPropertyCommand(prop.specifiers, B_INDEX_SPECIFIER)) {
			indent++;
			TraverseObjects(obj, path, name);
			indent--;
		}
	}
}

static void TraverseObject(const BMessenger &obj, const BMessage &path)
{
	status_t res;
	BMessage spec = path;
	BMessage suites;
	spec.what = B_GET_SUPPORTED_SUITES;
	res = SendScriptingMessage(obj, spec, suites);
	if (res < B_OK) return;
	const char *suiteName;

	for (int32 i = 0; suites.FindString("suites", i, &suiteName) >= B_OK; i++) {
		BPropertyInfo propInfo;
		res = suites.FindFlat("messages", i, &propInfo);
		if (res < B_OK) continue;
		PropertyList propList(propInfo.Properties());

		Indent(); printf("Suite \"%s\"\n", suiteName);
		indent++;
		TraverseProps(obj, path, propList);
		indent--;
	}
}

SuiteEditor::SuiteEditor(BMessenger handle):
	BWindow(BRect(0, 0, 640, 480), "Suite Editor", B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fHandle(handle),
	fStatus(B_OK)
{
/*
	TraverseObject(fHandle, BMessage());
	fStatus = B_ERROR;
	return;
*/
	BMessage spec(B_GET_SUPPORTED_SUITES);
	BMessage suites;
	fStatus = fHandle.SendMessage(&spec, &suites, 1000000, 1000000);
	if (fStatus < B_OK) return;
	suites.PrintToStream();

	fView = new BColumnListView("view", 0);
	fView->AddColumn(new BStringColumn("Name", 192, 50, 512, B_TRUNCATE_MIDDLE), nameCol);
	fView->AddColumn(new BStringColumn("Command", 250, 50, 512, B_TRUNCATE_MIDDLE), cmdCol);
	fView->AddColumn(new BStringColumn("Specfier", 250, 50, 512, B_TRUNCATE_MIDDLE), specCol);
	fView->AddColumn(new BStringColumn("Type", 192, 50, 512, B_TRUNCATE_MIDDLE), typeCol);
	fView->AddColumn(new BStringColumn("Value", 256, 50, 512, B_TRUNCATE_MIDDLE), valueCol);

	ListSuites(fView, NULL, fHandle, suites);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL)
			.Add(fView)
			.SetInsets(-1)
		.End()
	.End();

	CenterOnScreen();
}

status_t SuiteEditor::InitCheck()
{
	return fStatus;
}

