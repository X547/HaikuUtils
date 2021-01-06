#ifndef _UTILS_H_
#define _UTILS_H_

#include <OS.h>

class BString;


struct SignalRec
{
	const char *name;
	int val;
};

extern SignalRec signals[32];


void GetSizeString(BString &str, uint64 size);
void GetUsedMax(BString &str, uint64 used, uint64 max);
void GetUsedMaxSize(BString &str, uint64 used, uint64 max);

void GetUserGroupString(BString &str, int32 uid, int32 gid, bool showId = false);
void GetTeamString(BString &str, team_id team);
void GetThreadString(BString &str, thread_id thread);
void GetSemString(BString &str, sem_id sem);

void ShowLocation(const char *path);

#endif	// _UTILS_H_
