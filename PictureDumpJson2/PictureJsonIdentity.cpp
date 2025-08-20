#include "PictureJson.h"
#include "PictureVisitorJson.h"

#include <File.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using JsonWriter = rapidjson::Writer<rapidjson::OStreamWrapper>;


int main(int argCnt, char **args)
{
	rapidjson::OStreamWrapper os(std::cout);
	JsonWriter wr(os);
	PictureVisitorJson vis(wr);

	PictureJson pict;
	pict.Accept(vis);
	return 0;
}
