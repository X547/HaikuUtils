#include "PictureReaderJson.h"
#include "PictureWriterBinary.h"

#include <File.h>
//#include <BufferIO.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>


int main(int argCnt, char **args)
{
	if (argCnt < 2)
		return 1;

	PictureReaderJson pict(std::cin);

	BFile file(args[1], B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	//BBufferIO buf(&file, 65536, false);
	PictureWriterBinary vis(file);
	pict.Accept(vis);

	return 0;
}
