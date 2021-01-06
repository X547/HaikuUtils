#include "Utils.h"

#include <pwd.h>
#include <grp.h>

#include <String.h>

#include <OS.h>
#include <Entry.h>
#include <Messenger.h>
#include <private/kernel/util/KMessage.h>
#include <private/system/extended_system_info_defs.h>
#include <private/libroot/extended_system_info.h>


SignalRec signals[] = {
	{"SIGHUP", 1},
	{"SIGINT", 2},
	{"SIGQUIT", 3},
	{"SIGILL", 4},
	{"SIGCHLD", 5},
	{"SIGABRT", 6},
	{"SIGPIPE", 7},
	{"SIGFPE", 8},
	{"SIGKILL", 9},
	{"SIGSTOP", 10},
	{"SIGSEGV", 11},
	{"SIGCONT", 12},
	{"SIGTSTP", 13},
	{"SIGALRM", 14},
	{"SIGTERM", 15},
	{"SIGTTIN", 16},
	{"SIGTTOU", 17},
	{"SIGUSR1", 18},
	{"SIGUSR2", 19},
	{"SIGWINCH", 20},
	{"SIGKILLTHR", 21},
	{"SIGTRAP", 22},
	{"SIGPOLL", 23},
	{"SIGPROF", 24},
	{"SIGSYS", 25},
	{"SIGURG", 26},
	{"SIGVTALRM", 27},
	{"SIGXCPU", 28},
	{"SIGXFSZ", 29},
	{"SIGBUS", 30},
	{"SIGRESERVED1", 31},
	{"SIGRESERVED2", 32},
};


void GetSizeString(BString &str, uint64 size)
{
	const char* prefixes[] = {"", "K", "M", "G", "T"};
	unsigned pow = 0;
	int fracSize = 0;
	while ((size >= 1024) && (pow < sizeof(prefixes)/sizeof(prefixes[0]) - 1)) {
		fracSize = size*100/1024%100;
		size /= 1024;
		pow++;
	}
	str.SetToFormat("%" B_PRIu64 ".%02d %sB", size, fracSize, prefixes[pow]);
}

void GetUsedMax(BString &str, uint64 used, uint64 max)
{
	int32 ratio = 100;
	if (max > 0) {
		ratio = int32(double(used)/double(max)*100.0);
	}
	str.SetToFormat("%" B_PRIu64 "/%" B_PRIu64 " (%" B_PRId32 "%%)", used, max, ratio);
}

void GetUsedMaxSize(BString &str, uint64 used, uint64 max)
{
	BString str2;
	int32 ratio = 100;
	if (max > 0) {
		ratio = int32(double(used)/double(max)*100.0);
	}
	GetSizeString(str, used);
	str += "/";
	GetSizeString(str2, max);
	str += str2;
	str2.SetToFormat(" (%" B_PRId32 "%%)", ratio);
	str += str2;
}


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
	sem_info info;
	if (
		sem >= B_OK &&
		get_sem_info(sem, &info) >= B_OK
	)
		str.SetToFormat("%" B_PRId32 " (%s)", sem, info.name);
	else
		str.SetToFormat("%" B_PRId32, sem);
}


void ShowLocation(const char *path)
{
	BEntry entry(path);
	if (!entry.Exists()) return;

	node_ref node;
	entry.GetNodeRef(&node);
	
	BEntry parent;
	entry.GetParent(&parent);
	entry_ref parentRef;
	parent.GetRef(&parentRef);
	
	BMessage message(B_REFS_RECEIVED);
	message.AddRef("refs", &parentRef);
	message.AddData("nodeRefToSelect", B_RAW_TYPE, &node, sizeof(node_ref));
	
	BMessenger("application/x-vnd.Be-TRAK").SendMessage(&message);
}
