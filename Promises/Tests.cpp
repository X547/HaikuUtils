#include "Promises.h"
#include <stdio.h>


Promise <int, int> gPromise;


int main()
{
	gPromise.OnResolve([](Promise <int, int> &promise) {
		printf("(1) gPromise resolved: %d\n", promise.ResolvedValue());
	});
	gPromise.OnResolve([](Promise <int, int> &promise) {
		printf("(2) gPromise resolved: %d\n", promise.ResolvedValue());
	});
	gPromise.OnReject([](Promise <int, int> &promise) {
		printf("gPromise rejected: %d\n", promise.RejectedValue());
	});
	gPromise.Resolve(123);
	//gPromise.Reject(-123);
	return 0;
}
