#include "Utils.h"

#include <pwd.h>
#include <grp.h>

#include <String.h>

#include <OS.h>
#include <private/kernel/util/KMessage.h>
#include <private/system/extended_system_info_defs.h>
#include <private/libroot/extended_system_info.h>


void GetUserGroupString(BString &str, int32 uid, int32 gid, bool showId)
{
	BString str2;
	passwd *userRec = getpwuid(uid);
	if (userRec == NULL) str.SetToFormat("%" B_PRId32, uid);
	else str = userRec->pw_name;
	str += ":";
	group *grpRec = getgrgid(gid);
	if (grpRec == NULL) str2.SetToFormat("%" B_PRId32, gid);
	else str2 = grpRec->gr_name;
	str += str2;
	if (showId) {
		str2.SetToFormat(" (%" B_PRId32 ":%" B_PRId32 ")", uid, gid);
		str += str2;
	}
}

void GetTeamString(BString &str, team_id team)
{
	const char *name = NULL;
	KMessage extInfo;
	if (
		team >= B_OK &&
		get_extended_team_info(team, B_TEAM_INFO_BASIC, extInfo) >= B_OK &&
		extInfo.FindString("name", &name) >= B_OK
	)
		str.SetToFormat("%" B_PRId32 " (%s)", team, name);
	else
		str.SetToFormat("%" B_PRId32, team);
}

void GetThreadString(BString &str, thread_id thread)
{
	const char *name = NULL;
	thread_info info;
	if (
		thread != 0 &&
		get_thread_info((thread >= 0)? thread: -thread, &info) >= B_OK
	)
		str.SetToFormat("%" B_PRId32 " (%s)", thread, info.name);
	else
		str.SetToFormat("%" B_PRId32, thread);
}

void GetSemString(BString &str, sem_id sem)
{
	const char *name = NULL;
	sem_info info;
	if (
		sem >= B_OK &&
		get_sem_info(sem, &info) >= B_OK
	)
		str.SetToFormat("%" B_PRId32 " (%s)", sem, info.name);
	else
		str.SetToFormat("%" B_PRId32, sem);
}

