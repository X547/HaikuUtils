#pragma once

#include "Promises.h"
#include <String.h>

struct Credentials {
	BString login;
	BString password;
	Credentials(const char *login, const char *password): login(login), password(password) {}
};

typedef BReference<Promise<Credentials, status_t> > CredentialsPromiseRef;

CredentialsPromiseRef GetCredentials();
