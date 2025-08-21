#include "PictureJson.h"
#include "PictureVisitorBinary.h"

#include <File.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>


int main(int argCnt, char **args)
{
	if (argCnt < 2)
		return 1;

	PictureJson pict;

	BFile file(args[1], B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	PictureVisitorBinary vis(file);
	pict.Accept(vis);

	return 0;
}
