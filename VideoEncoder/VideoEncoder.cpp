#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <String.h>
#include <File.h>
#include <Application.h>
#include <MediaFile.h>
#include <MediaTrack.h>

enum {
	width = 800,
	height = 600,
};

int main()
{
	BApplication app("application/x-vnd.test-app");
	BFile file("output.mp4", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	media_format mFormat;
	media_file_format mff;
	media_codec_info codec, firstCodec;
	media_format outFormat;

	ArrayDeleter<uint32> data(new uint32[width*height*4]);
	for (int i = 0; i < width*height; i++)
		data.Get()[i] = 0xff0099ff;

	memset(&mFormat, 0, sizeof(media_format));
	mFormat.type = B_MEDIA_RAW_VIDEO;
	mFormat.u.raw_video.display.line_width = width;
	mFormat.u.raw_video.display.line_count = height;
	mFormat.u.raw_video.last_active = mFormat.u.raw_video.display.line_count - 1;
	mFormat.u.raw_video.display.format = B_YUV422;
	mFormat.u.raw_video.display.bytes_per_row = width*4;
	mFormat.u.raw_video.interlace = 1;
	mFormat.u.raw_video.field_rate = 30;
	mFormat.u.raw_video.pixel_width_aspect = 1;
	mFormat.u.raw_video.pixel_height_aspect = 1;

	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		if (mff.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO) {
			if (BString(mff.file_extension) == "mp4") {
				printf("container: %s, %s\n", mff.pretty_name, mff.file_extension);
				break;
			}
		}
	}

	cookie = 0;
	while (get_next_encoder(&cookie, &mff, &mFormat, &outFormat, &codec) == B_OK) {
		//if (BString(codec.short_name) == "mpeg4") {
			printf("encoder: %s, %s\n", codec.pretty_name, codec.short_name);
		//	break;
		//}
	}

	ObjectDeleter<BMediaFile> mediaFile(new BMediaFile(&file, &mff));
	BMediaTrack* track = mediaFile->CreateTrack(&mFormat, &codec);
	mediaFile->CommitHeader();

	for (int32 i = 0; i < 256; i++) {
		track->WriteFrames(data.Get(), 1, B_MEDIA_KEY_FRAME);
	}

	track->Flush();
	mediaFile->CloseFile();
	return 0;
}
