#include "ScriptingUtils.h"

#include <String.h>
#include <Messenger.h>
#include <private/app/MessengerPrivate.h>

#include <stdio.h>

void DumpMessenger(const BMessenger &handle)
{
	printf("(team: %" B_PRId32 ", token: %" B_PRId32 ")", BMessenger::Private((BMessenger&)handle).Team(), BMessenger::Private((BMessenger&)handle).Token());
}

status_t SendScriptingMessage(const BMessenger &obj, BMessage &spec, BMessage &reply)
{
	status_t res = obj.SendMessage(&spec, &reply, 1000000, 1000000);
	if (res < B_OK) {
		printf("[!] can't send message to "); DumpMessenger(obj); printf(", error: %s\n", strerror(res));
		return res;
	}
	return res;
}

status_t GetBool(bool &val, const BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = SendScriptingMessage(obj, spec, reply)) != B_OK) return res;
	if ((res = reply.FindBool("result", &val)) != B_OK) return res;
	return res;
}

status_t GetInt32(int32 &val, const BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = SendScriptingMessage(obj, spec, reply)) != B_OK) return res;
	if ((res = reply.FindInt32("result", &val)) != B_OK) return res;
	return res;
}

status_t GetString(BString &val, const BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = SendScriptingMessage(obj, spec, reply)) != B_OK) return res;
	if ((res = reply.FindString("result", &val)) != B_OK) return res;
	return res;
}

status_t GetMessenger(BMessenger &val, const BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = SendScriptingMessage(obj, spec, reply)) != B_OK) return res;
	if ((res = reply.FindMessenger("result", &val)) != B_OK) return res;
	return res;
}

status_t GetRect(BRect &val, const BMessenger &obj, BMessage &spec)
{
	status_t res;
	BMessage reply;
	if ((res = SendScriptingMessage(obj, spec, reply)) != B_OK) return res;
	if ((res = reply.FindRect("result", &val)) != B_OK) return res;
	return res;
}

void WriteError(BString &dst, status_t res)
{
	dst += "<";
	dst += strerror(res);
	dst += ">";
}

void WriteStringProp(BString &dst, BMessenger &obj, const char *field)
{
	BMessage spec;
	BString str;
	status_t res;
	spec.what = B_GET_PROPERTY;
	spec.AddSpecifier(field);
	if ((res = GetString(str, obj, spec)) != B_OK) {
		WriteError(dst, res);
	} else {
		dst += str;
	}
}

void WriteSuites(BString &dst, BMessenger &obj)
{
	BMessage spec, reply;
	type_code type;
	int32 count;
	const char *str;
	status_t res;
	spec = BMessage(B_GET_SUPPORTED_SUITES);
	if (
		((res = SendScriptingMessage(obj, spec, reply)) != B_OK)
	) {
		WriteError(dst, res);
	} else {
		reply.GetInfo("suites", &type, &count);
		for (int32 i = 0; i < count; i++) {
			if (i > 0) {dst += ", ";}
			reply.FindString("suites", i, &str);
			dst += str;
		}
	}
}
