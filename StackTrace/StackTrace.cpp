#include <stdio.h>
#include <stdlib.h>
#include <OS.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <private/debug/debug_support.h>

#include <cxxabi.h>

team_id team;
thread_id thread;
port_id debuggerPort, nubPort;
debug_context debugContext;

status_t Check(status_t res, const char *msg = NULL, bool fatal = true)
{
	if (res < B_OK) {
		if (fatal)
			printf("fatal ");
		if (msg != NULL)
			fprintf(stderr, "error: %s (%s)\n", msg, strerror(res));
		else
			fprintf(stderr, "error: %s\n", strerror(res));
		if (fatal) exit(1);
	}
	return res;
}

void FatalError(const char *msg)
{
	printf("Fatal error: %s\n", msg);
	exit(1);
}

void WriteAddress(addr_t adr)
{
#if B_HAIKU_32_BIT
	printf("0x%08X", adr);
#elif B_HAIKU_64_BIT
	printf("0x%016lX", adr);
#else
#error "unsupported address size"
#endif
}


char *CppDemangle(const char *abiName)
{
  int status;
  char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);
  return ret;
}

void _LookupSymbolAddress(
	debug_symbol_lookup_context *lookupContext, const void *address,
	char *buffer, int32 bufferSize)
{
	// lookup the symbol
	void *baseAddress;
	char symbolName[1024];
	char imagePath[B_PATH_NAME_LENGTH], imageNameBuf[B_PATH_NAME_LENGTH], *imageName;
	bool exactMatch;
	bool lookupSucceeded = false;
	if (lookupContext) {
		status_t error = debug_lookup_symbol_address(lookupContext, address,
			&baseAddress, symbolName, sizeof(symbolName), imagePath,
			sizeof(imagePath), &exactMatch);
		lookupSucceeded = (error == B_OK);
	}

	if (lookupSucceeded) {
		strcpy(imageNameBuf, imagePath);
		imageName = basename(imageNameBuf);

		// we were able to look something up
		if (strlen(symbolName) > 0) {
			char *demangledName = CppDemangle(symbolName);
			if (demangledName != NULL) {
				strcpy(symbolName, demangledName);
				free(demangledName);
			}

			// we even got a symbol
			snprintf(buffer, bufferSize, "<%s> %s + %ld%s", imageName, symbolName,
				(addr_t)address - (addr_t)baseAddress,
				(exactMatch ? "" : " (closest symbol)"));

		} else {
			// no symbol: image relative address
			snprintf(buffer, bufferSize, "<%s> + %ld", imageName,
				(addr_t)address - (addr_t)baseAddress);
		}

	} else {
		// lookup failed: find area containing the IP
		bool useAreaInfo = false;
		area_info info;
		ssize_t cookie = 0;
		while (get_next_area_info(team, &cookie, &info) == B_OK) {
			if ((addr_t)info.address <= (addr_t)address
				&& (addr_t)info.address + info.size > (addr_t)address) {
				useAreaInfo = true;
				break;
			}
		}

		if (useAreaInfo) {
			snprintf(buffer, bufferSize, "<area: %s> + %#lx", info.name,
				(addr_t)address - (addr_t)info.address);
		} else if (bufferSize > 0)
			buffer[0] = '\0';
	}
}

void WriteStackTrace()
{
	status_t error;
	void *ip = NULL, *fp = NULL;

	error = debug_get_instruction_pointer(&debugContext, thread, &ip, &fp);

	debug_symbol_lookup_context *lookupContext = NULL;
	Check(debug_create_symbol_lookup_context(team, -1, &lookupContext), "can't create symbol lookup context");

	char symbolBuffer[2048];
	_LookupSymbolAddress(lookupContext, ip, symbolBuffer, sizeof(symbolBuffer) - 1);

	printf("FP: "); WriteAddress((addr_t)fp);
	printf(", %s\n", symbolBuffer);

	for (int32 i = 0; i < 400; i++) {
		debug_stack_frame_info stackFrameInfo;

		error = debug_get_stack_frame(&debugContext, fp, &stackFrameInfo);
		if (error < B_OK)
			break;

		ip = stackFrameInfo.return_address;
		fp = stackFrameInfo.parent_frame;

		_LookupSymbolAddress(lookupContext, ip, symbolBuffer, sizeof(symbolBuffer) - 1);

		printf("FP: "); WriteAddress((addr_t)fp);
		printf(", %s\n", symbolBuffer);

		if (fp == NULL)
			break;
	}

	if (lookupContext)
		debug_delete_symbol_lookup_context(lookupContext);
}

int main(int argCnt, char **args)
{
	thread_info threadInfo;
	void *ip = NULL, *fp = NULL;

	if (argCnt != 2)
		FatalError("thread id should be provided\n");

	thread = strtol(args[1], NULL, 10);
	
	Check(get_thread_info(thread, &threadInfo), "thread not found");
	team = threadInfo.team;

	debuggerPort = Check(create_port(10, "debugger port"));
	nubPort = Check(install_team_debugger(team, debuggerPort), "can't install debugger");
	Check(init_debug_context(&debugContext, team, nubPort));

	Check(debug_thread(thread));
	snooze(2000); // avoid "can't get IP and FP (Thread is inappropriate state)"

	if (Check(debug_get_instruction_pointer(&debugContext, thread, &ip, &fp), "can't get IP and FP", false) >= B_OK)
		WriteStackTrace();

	destroy_debug_context(&debugContext);
	remove_team_debugger(team);
	delete_port(debuggerPort);
	
	return 0;
}
