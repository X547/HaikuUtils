#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <SupportDefs.h>

#include <errno.h>


class StatusError
{
public:
	status_t res;
	const char *msg;

	StatusError(status_t res, const char *msg);
};

void ShowError(const StatusError &err);

static inline status_t Check(status_t res, const char *msg = NULL, bool fatal = true)
{
	if ((res < B_OK) && fatal)
		throw StatusError(res, msg);
	return res;
}

static inline status_t CheckErrno(status_t res, const char *msg = NULL, bool fatal = true)
{
	if ((res < 0) && fatal)
		throw StatusError(errno, msg);
	return res;
}

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}

#define CheckRetVoid(err) {status_t _err = (err); if (_err < B_OK) return;}


#endif	// _ERRORS_H_
