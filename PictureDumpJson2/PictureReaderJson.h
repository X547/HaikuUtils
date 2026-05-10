#pragma once

#include <private/shared/AutoDeleter.h>

#include "PictureVisitor.h"
#include "JsonScanner.h"


class PictureReaderJson {
private:
	JsonScanner fScanner;

	void ReadToken();
	JsonToken &Token() {return fScanner.Token();}

	[[noreturn]] void RaiseError();
	[[noreturn]] void RaiseUnimplemented();
	void Assume(bool cond);
	void AssumeToken(JsonTokenKind tokenKind);

	bool ReadBool();
	uint8 ReadUint8();
	int32 ReadInt32();
	double ReadReal();
	int32 HexDigit(char digit);
	void ReadColor(rgb_color &color);
	void ReadPoint(BPoint &pt);
	void ReadRect(BRect &rect);
	void ReadEscapementDelta(escapement_delta &delta);
	void ReadShape(BShape &shape);
	void ReadGradientStops(BGradient &gradient);
	void ReadGradient(ObjectDeleter<BGradient> &outGradient);
	void ReadColorSpace(color_space &val);

	void ReadPicture(PictureVisitor &vis);
	void ReadOps(PictureVisitor &vis);

	void ReadMovePenBy(PictureVisitor &vis);
	void ReadStrokeLine(PictureVisitor &vis);
	void ReadStrokeRect(PictureVisitor &vis);
	void ReadFillRect(PictureVisitor &vis);
	void ReadStrokeRoundRect(PictureVisitor &vis);
	void ReadFillRoundRect(PictureVisitor &vis);
	void ReadStrokeBezier(PictureVisitor &vis);
	void ReadFillBezier(PictureVisitor &vis);
	void ReadStrokePolygon(PictureVisitor &vis);
	void ReadFillPolygon(PictureVisitor &vis);
	void ReadStrokeShape(PictureVisitor &vis);
	void ReadFillShape(PictureVisitor &vis);
	void ReadDrawString(PictureVisitor &vis);
	void ReadDrawBitmap(PictureVisitor &vis);
	void ReadDrawPicture(PictureVisitor &vis);
	void ReadStrokeArc(PictureVisitor &vis);
	void ReadFillArc(PictureVisitor &vis);
	void ReadStrokeEllipse(PictureVisitor &vis);
	void ReadFillEllipse(PictureVisitor &vis);
	void ReadDrawStringLocations(PictureVisitor &vis);
	void ReadStrokeRectGradient(PictureVisitor &vis);
	void ReadFillRectGradient(PictureVisitor &vis);
	void ReadStrokeRoundRectGradient(PictureVisitor &vis);
	void ReadFillRoundRectGradient(PictureVisitor &vis);
	void ReadStrokeBezierGradient(PictureVisitor &vis);
	void ReadFillBezierGradient(PictureVisitor &vis);
	void ReadStrokePolygonGradient(PictureVisitor &vis);
	void ReadFillPolygonGradient(PictureVisitor &vis);
	void ReadStrokeShapeGradient(PictureVisitor &vis);
	void ReadFillShapeGradient(PictureVisitor &vis);
	void ReadStrokeArcGradient(PictureVisitor &vis);
	void ReadFillArcGradient(PictureVisitor &vis);
	void ReadStrokeEllipseGradient(PictureVisitor &vis);
	void ReadFillEllipseGradient(PictureVisitor &vis);
	void ReadEnterStateChange(PictureVisitor &vis);
	void ReadSetClipping(PictureVisitor &vis);
	void ReadClipToPicture(PictureVisitor &vis);
	void ReadGroup(PictureVisitor &vis);
	void ReadClearClipping(PictureVisitor &vis);
	void ReadClipToRect(PictureVisitor &vis);
	void ReadClipToShape(PictureVisitor &vis);
	void ReadSetOrigin(PictureVisitor &vis);
	void ReadSetPenLocation(PictureVisitor &vis);
	void ReadSetDrawingMode(PictureVisitor &vis);
	void ReadSetLineMode(PictureVisitor &vis);
	void ReadSetPenSize(PictureVisitor &vis);
	void ReadSetScale(PictureVisitor &vis);
	void ReadSetHighColor(PictureVisitor &vis);
	void ReadSetLowColor(PictureVisitor &vis);
	void ReadSetPattern(PictureVisitor &vis);
	void ReadEnterFontState(PictureVisitor &vis);
	void ReadSetBlendingMode(PictureVisitor &vis);
	void ReadSetFillRule(PictureVisitor &vis);
	void ReadSetFontFamily(PictureVisitor &vis);
	void ReadSetFontStyle(PictureVisitor &vis);
	void ReadSetFontSpacing(PictureVisitor &vis);
	void ReadSetFontEncoding(PictureVisitor &vis);
	void ReadSetFontFlags(PictureVisitor &vis);
	void ReadSetFontSize(PictureVisitor &vis);
	void ReadSetFontRotation(PictureVisitor &vis);
	void ReadSetFontShear(PictureVisitor &vis);
	void ReadSetFontBpp(PictureVisitor &vis);
	void ReadSetFontFace(PictureVisitor &vis);
	void ReadSetFontFalseBoldWidth(PictureVisitor &vis);
	void ReadSetTransform(PictureVisitor &vis);
	void ReadTranslateBy(PictureVisitor &vis);
	void ReadScaleBy(PictureVisitor &vis);
	void ReadRotateBy(PictureVisitor &vis);
	void ReadBlendLayer(PictureVisitor &vis);

public:
	PictureReaderJson(std::istream &is);

	void Accept(PictureVisitor &vis);
};
