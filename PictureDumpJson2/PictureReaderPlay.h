#pragma once

#include <SupportDefs.h>


class BPositionIO;
class PictureVisitor;


class PictureReaderPlay {
private:
	BPositionIO &fRd;

	void RaiseError() const;
	void CheckStatus(status_t status) const;

public:
	PictureReaderPlay(BPositionIO &rd): fRd(rd) {}

	status_t Accept(PictureVisitor &vis) const;
};
