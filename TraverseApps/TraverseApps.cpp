#include <OS.h>
#include <Application.h>
#include <Roster.h>
#include <Message.h>
#include <Entry.h>
#include <Messenger.h>
#include <List.h>
#include <String.h>
#include <stdio.h>

int indent = 0;

void Indent()
{
	for (int i = 0; i < indent; i++) {
		printf("\t");
	}
}

void WriteString(const char *str)
{
	printf("\"%s\"", str);
}

void WriteError(status_t res)
{
	switch (res) {
	case B_TIMED_OUT: printf("<TIME OUT>"); break;
	case B_NAME_NOT_FOUND: printf("<NOT FOUND>"); break;
	default: printf("<ERROR>");
	};
}

status_t GetInt32(int32 &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindInt32("result", &val)) != B_OK) return res;
	return res;
}

status_t GetString(BString &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindString("result", &val)) != B_OK) return res;
	return res;
}

status_t GetMessenger(BMessenger &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindMessenger("result", &val)) != B_OK) return res;
	return res;
}

status_t GetRect(BRect &val, BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK) return res;
	if ((res = reply.FindRect("result", &val)) != B_OK) return res;
	return res;
}

void WriteStringProp(BMessenger &obj, const char *field)
{
	BMessage spec;
	BString str;
	status_t res;
	spec.what = B_GET_PROPERTY;
	spec.AddSpecifier(field);
	if ((res = GetString(str, obj, spec)) != B_OK) {
		WriteError(res);
	} else {
		WriteString(str);
	}
}

void WriteRectProp(BMessenger &obj, const char *field)
{
	BMessage spec;
	BRect rect;
	status_t res;
	spec.what = B_GET_PROPERTY;
	spec.AddSpecifier(field);
	if ((res = GetRect(rect, obj, spec)) != B_OK) {
		WriteError(res);
	} else {
		printf("(%g, %g, %g, %g)", rect.left, rect.top, rect.right, rect.bottom);
	}
}

void WriteSuites(BMessenger &obj)
{
	BMessage spec, reply;
	type_code type;
	int32 count;
	const char *str;
	status_t res;
	spec = BMessage(B_GET_SUPPORTED_SUITES);
	if (
		((res = obj.SendMessage(&spec, &reply, 1000000, 1000000)) != B_OK)
	) {
		Indent(); WriteError(res);
	} else {
		printf("(");
		reply.GetInfo("suites", &type, &count);
		for (int32 i = 0; i < count; i++) {
			if (i > 0) {printf(", ");}
			reply.FindString("suites", i, &str);
			WriteString(str);
		}
		printf(")");
	}
}

void WriteView(BMessenger &view)
{
	BMessage spec, reply;
	int32 count;
	BMessenger view2;
	status_t res;
	printf("BView(");
	WriteSuites(view); printf(", ");
	WriteStringProp(view, "InternalName"); printf(", ");
	WriteRectProp(view, "Frame");
	printf(")\n");
	indent++;
	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier("View");
	
	if ((res = GetInt32(count, view, spec)) != B_OK) {
		Indent(); WriteError(res); printf("\n");
	} else {
		for (int32 i = 0; i < count; i++) {
			Indent(); printf("%d: ", i);
			spec = BMessage(B_GET_PROPERTY);
			spec.AddSpecifier("View", i);
			if ((res = GetMessenger(view2, view, spec)) != B_OK) {
				WriteError(res); printf("\n");
			} else {
				WriteView(view2);
			}
		}
	}
	indent--;
}

void WriteWindow(BMessenger &wnd)
{
	BMessage spec, reply;
	int32 count;
	BMessenger view;
	status_t res;
	printf("BWindow(");
	WriteSuites(wnd); printf(", ");
	WriteStringProp(wnd, "InternalName"); printf(", ");
	WriteStringProp(wnd, "Title"); printf(", ");
	WriteRectProp(wnd, "Frame");
	printf(")\n");
	indent++;
	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier("View");
	if ((res = GetInt32(count, wnd, spec)) != B_OK) {
		Indent(); WriteError(res); printf("\n");
	} else {
		for (int32 i = 0; i < count; i++) {
			Indent(); printf("%d: ", i);
			spec = BMessage(B_GET_PROPERTY);
			spec.AddSpecifier("View", i);
			if ((res = GetMessenger(view, wnd, spec)) != B_OK) {
				WriteError(res); printf("\n");
			} else {
				WriteView(view);
			}
		}
	}
	indent--;
}

void WriteApp(BMessenger &app)
{
	BMessage spec, reply;
	app_info info;
	int32 count;
	BMessenger wnd;
	status_t res;
	be_roster->GetRunningAppInfo(app.Team(), &info);
	printf("BApplication(");
	WriteSuites(app); printf(", ");
	WriteString(info.signature); printf(")\n");
	indent++;
	spec = BMessage(B_COUNT_PROPERTIES);
	spec.AddSpecifier("Window");
	if ((res = GetInt32(count, app, spec)) != B_OK) {
		Indent(); WriteError(res); printf("\n");
	} else {
		for (int32 i = 0; i < count; i++) {
			Indent(); printf("%d: ", i);
			spec = BMessage(B_GET_PROPERTY);
			spec.AddSpecifier("Window", i);
			if ((res = GetMessenger(wnd, app, spec)) != B_OK) {
				WriteError(res); printf("\n");
			} else {
				WriteWindow(wnd);
			}
		}
	}
	indent--;
}

int main()
{
	BApplication application("application/x-vnd.Test-App");
	BList appList;
	BMessenger app;
	be_roster->GetAppList(&appList);
	for (int i = 0; i < appList.CountItems(); i++) {
		team_id team = (team_id)(intptr_t)appList.ItemAt(i);
		if (team != be_app->Team()) {
			app = BMessenger(NULL, team);
			Indent(); WriteApp(app);
		}
	}
	return 0;
}
