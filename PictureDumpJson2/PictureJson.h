#pragma once

#include <math.h>

#include <iostream>

#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>

#include <private/shared/AutoDeleter.h>

#include "PictureVisitor.h"


enum class JsonTokenKind {
	Eos,
	Null,
	Bool,
	Int,
	UInt,
	Int64,
	UInt64,
	Double,
	RawNumber,
	String,
	StartObject,
	Key,
	EndObject,
	StartArray,
	EndArray,
};


struct JsonToken {
	JsonTokenKind kind = JsonTokenKind::Eos;
	std::string strVal;
	bool boolVal;
	int64_t int64Val;
	uint64_t uint64Val;
	double doubleVal;

	bool IsInt32()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
				return int64Val >= INT32_MIN && int64Val <= INT32_MAX;
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
				return uint64Val <= INT32_MAX;
			default:
				return false;
		}
	}

	bool IsReal()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
			case JsonTokenKind::Double:
				return true;
			default:
				return false;
		}
	}

	int32 Int32Val()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
				return int64Val;
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
				return uint64Val;
			case JsonTokenKind::Double:
				return doubleVal;
			default:
				return 0;
		}
	}

	double RealVal()
	{
		switch (kind) {
			case JsonTokenKind::Int:
			case JsonTokenKind::Int64:
				return int64Val;
			case JsonTokenKind::UInt:
			case JsonTokenKind::UInt64:
				return uint64Val;
			case JsonTokenKind::Double:
				return doubleVal;
			default:
				return NAN;
		}
	}
};


class PictureJson {
private:
	rapidjson::IStreamWrapper fStream;
	rapidjson::Reader fRd;
	JsonToken fToken;

	void ReadToken();

	void RaiseError();
	void RaiseUnimplemented();
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
	void ReadShape(BShape &shape);
	void ReadGradientStops(BGradient &gradient);
	void ReadGradient(ObjectDeleter<BGradient> &outGradient);

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
	void ReadSetTransform(PictureVisitor &vis);
	void ReadTranslateBy(PictureVisitor &vis);
	void ReadScaleBy(PictureVisitor &vis);
	void ReadRotateBy(PictureVisitor &vis);
	void ReadBlendLayer(PictureVisitor &vis);

public:
	PictureJson();

	void Accept(PictureVisitor &vis);
};
