#pragma once

#include <SupportDefs.h>


class BPositionIO;
class PictureVisitor;


class PictureBinary {
private:
	BPositionIO &fRd;

public:
	PictureBinary(BPositionIO &rd): fRd(rd) {}

	status_t Accept(PictureVisitor &vis) const;
};
