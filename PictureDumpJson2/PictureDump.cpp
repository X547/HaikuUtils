#include "PictureBinary.h"
#include "PictureVisitorJson.h"

#include <File.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using JsonWriter = rapidjson::Writer<rapidjson::OStreamWrapper>;


int main(int argCnt, char **args)
{
	if (argCnt < 2)
		return 1;

	rapidjson::OStreamWrapper os(std::cout);
	JsonWriter wr(os);
	PictureVisitorJson vis(wr);

	BFile file(args[1], B_READ_ONLY);
	PictureBinary pict(file);
	pict.Accept(vis);
	return 0;
}
