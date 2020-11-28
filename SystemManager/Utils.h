#ifndef _UTILS_H_
#define _UTILS_H_

#include <OS.h>

class BString;

void GetUserGroupString(BString &str, int32 uid, int32 gid, bool showId = false);
void GetTeamString(BString &str, team_id team);
void GetThreadString(BString &str, thread_id thread);
void GetSemString(BString &str, sem_id sem);

#endif	// _UTILS_H_
