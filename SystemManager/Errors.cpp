#include "Errors.h"

#include <errno.h>

#include <Alert.h>
#include <String.h>


StatusError::StatusError(status_t res, const char *msg):
	res(res), msg(msg)
{}

void ShowError(const StatusError &err)
{
	BString msg, buf;

	if (err.msg != NULL) {
		buf.SetToFormat("%s\n\n", err.msg);
		msg += buf;
	}
	buf.SetToFormat("Error code: 0x%08" B_PRIx32 " (%s).", err.res, strerror(err.res));
	msg += buf;

	BAlert *alert = new BAlert("Error", msg, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go(NULL);
}
