#include "UserDB.h"

#include <pwd.h>
#include <grp.h>

#include <private/app/RegistrarDefs.h>
#include <private/libroot/user_group.h>
#include <private/kernel/util/KMessage.h>


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) {debugger(strerror(_err)); return _err;}}


status_t GetUsers(KMessage &reply, int32 &count, passwd**& entries)
{
	int32 numBytes;

	KMessage message(BPrivate::B_REG_GET_PASSWD_DB);
	CheckRet(BPrivate::send_authentication_request_to_registrar(message, reply));
	CheckRet(reply.What());
	CheckRet(reply.FindInt32("count", &count));
	CheckRet(reply.FindData("entries", B_RAW_TYPE, (const void**)&entries, &numBytes));

	addr_t baseAddress = (addr_t)entries;
	for (int32 i = 0; i < count; i++) {
		passwd* entry = BPrivate::relocate_pointer(baseAddress, entries[i]);
		BPrivate::relocate_pointer(baseAddress, entry->pw_name);
		BPrivate::relocate_pointer(baseAddress, entry->pw_passwd);
		BPrivate::relocate_pointer(baseAddress, entry->pw_dir);
		BPrivate::relocate_pointer(baseAddress, entry->pw_shell);
		BPrivate::relocate_pointer(baseAddress, entry->pw_gecos);
	}

	return B_OK;
}

status_t GetGroups(KMessage &reply, int32 &count, group**& entries)
{
	int32 numBytes;

	KMessage message(BPrivate::B_REG_GET_GROUP_DB);
	CheckRet(BPrivate::send_authentication_request_to_registrar(message, reply));
	CheckRet(reply.What());
	CheckRet(reply.FindInt32("count", &count));
	CheckRet(reply.FindData("entries", B_RAW_TYPE, (const void**)&entries, &numBytes));

	addr_t baseAddress = (addr_t)entries;
	for (int32 i = 0; i < count; i++) {
		group* entry = BPrivate::relocate_pointer(baseAddress, entries[i]);
		BPrivate::relocate_pointer(baseAddress, entry->gr_name);
		BPrivate::relocate_pointer(baseAddress, entry->gr_passwd);
		BPrivate::relocate_pointer(baseAddress, entry->gr_mem);
		int32 k = 0;
		for (; entry->gr_mem[k] != (void*)-1; k++)
			BPrivate::relocate_pointer(baseAddress, entry->gr_mem[k]);
		entry->gr_mem[k] = NULL;
	}

	return B_OK;
}

status_t DeleteUser(uid_t id, const char *name)
{
	KMessage message(BPrivate::B_REG_DELETE_USER), reply;
	if ((int32)id >= 0)
		CheckRet(message.AddInt32("uid", id));
	if (name != NULL)
		CheckRet(message.AddString("name", name));
	CheckRet(send_authentication_request_to_registrar(message, reply));
	return reply.What();
}

status_t DeleteGroup(gid_t id, const char *name)
{
	KMessage message(BPrivate::B_REG_DELETE_GROUP), reply;
	if ((int32)id >= 0)
		CheckRet(message.AddInt32("gid", id));
	if (name != NULL)
		CheckRet(message.AddString("name", name));
	CheckRet(send_authentication_request_to_registrar(message, reply));
	return reply.What();
}

status_t UpdateUser(KMessage &message)
{
	KMessage reply;
	message.SetWhat(BPrivate::B_REG_UPDATE_USER);
	CheckRet(send_authentication_request_to_registrar(message, reply));
	return reply.What();
}

status_t UpdateGroup(KMessage &message)
{
	KMessage reply;
	message.SetWhat(BPrivate::B_REG_UPDATE_GROUP);
	CheckRet(send_authentication_request_to_registrar(message, reply));
	return reply.What();
}

status_t GetUser(KMessage &reply, uid_t id, const char *name, bool shadow)
{
	KMessage message(BPrivate::B_REG_GET_USER);
	if ((int32)id >= 0)
		CheckRet(message.AddInt32("uid", id));
	if (name != NULL)
		CheckRet(message.AddString("name", name));
	if (shadow)
		CheckRet(message.AddBool("shadow", shadow));
	CheckRet(send_authentication_request_to_registrar(message, reply));
	return reply.What();
}

status_t GetGroup(KMessage &reply, gid_t id, const char *name)
{
	KMessage message(BPrivate::B_REG_GET_GROUP);
	if ((int32)id >= 0)
		CheckRet(message.AddInt32("gid", id));
	if (name != NULL)
		CheckRet(message.AddString("name", name));
	CheckRet(send_authentication_request_to_registrar(message, reply));
	return reply.What();
}
