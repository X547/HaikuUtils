#ifndef _USERDB_H_
#define _USERDB_H_

#include <SupportDefs.h>

namespace BPrivate {
	class KMessage;
}
struct passwd;
struct group;

using BPrivate::KMessage;

status_t GetUsers(KMessage &reply, int32 &count, passwd**& entries);
status_t GetGroups(KMessage &reply, int32 &count, group**& entries);
status_t DeleteUser(uid_t id, const char *name);
status_t DeleteGroup(gid_t id, const char *name);
status_t UpdateUser(KMessage &message);
status_t UpdateGroup(KMessage &message);
status_t GetUser(KMessage &reply, uid_t id, const char *name, bool shadow = false);
status_t GetGroup(KMessage &reply, gid_t id, const char *name);

#endif	// _USERDB_H_
