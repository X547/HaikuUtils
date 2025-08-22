#pragma once

#include <SupportDefs.h>


class BPositionIO;
class PictureVisitor;


class PictureReaderBinary {
private:
	BPositionIO &fRd;

public:
	PictureReaderBinary(BPositionIO &rd): fRd(rd) {}

	status_t Accept(PictureVisitor &vis) const;
};
