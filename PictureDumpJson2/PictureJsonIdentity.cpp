#include "PictureReaderJson.h"
#include "PictureWriterJson.h"

#include <File.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using JsonWriter = rapidjson::Writer<rapidjson::OStreamWrapper>;


int main(int argCnt, char **args)
{
	rapidjson::OStreamWrapper os(std::cout);
	JsonWriter wr(os);
	PictureWriterJson vis(wr);

	PictureReaderJson pict;
	pict.Accept(vis);
	return 0;
}
