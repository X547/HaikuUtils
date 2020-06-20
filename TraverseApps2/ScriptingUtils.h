#ifndef _SCRIPTINGUTILS_H_
#define _SCRIPTINGUTILS_H_

#include <SupportDefs.h>

class BMessage;
class BMessenger;
class BRect;
class BString;


void DumpMessenger(const BMessenger &handle);
status_t SendScriptingMessage(const BMessenger &obj, BMessage &spec, BMessage &reply);
status_t GetBool(bool &val, const BMessenger &obj, BMessage &spec);
status_t GetInt32(int32 &val, const BMessenger &obj, BMessage &spec);
status_t GetString(BString &val, const BMessenger &obj, BMessage &spec);
status_t GetMessenger(BMessenger &val, const BMessenger &obj, BMessage &spec);
status_t GetRect(BRect &val, const BMessenger &obj, BMessage &spec);
void WriteError(BString &dst, status_t res);
void WriteStringProp(BString &dst, BMessenger &obj, const char *field);
void WriteSuites(BString &dst, BMessenger &obj);


#endif	// _SCRIPTINGUTILS_H_
