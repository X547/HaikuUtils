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

	BApplication app("application/x-vnd.Test.PictureCompileViewJson");

	PictureReaderJson pictRd;

	BView *view = new BView(BRect(0, 0, 256, 256), "view", B_FOLLOW_ALL, 0);
	wnd->AddChild(view);

	BPicture pict;
	view->BeginPicture(&pict);

	PictureWriterView vis(*view);
	pictRd.Accept(vis);

	view->EndPicture();
	view->Sync();

	BFile file(args[1], B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	//BBufferIO buf(&file, 65536, false);
	pict.Flatten(&file);

	view->RemoveSelf();
	delete view;

	return 0;
}
