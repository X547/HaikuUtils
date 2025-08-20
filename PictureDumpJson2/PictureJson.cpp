#include "PictureJson.h"

#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>

#include <math.h>
#include <string>
#include <string_view>
#include <iostream>


class JsonTokenHandler {
private:
	JsonToken &fToken;

	JsonTokenHandler(const JsonTokenHandler& noCopyConstruction);
	JsonTokenHandler& operator=(const JsonTokenHandler& noAssignment);

public:
	JsonTokenHandler(JsonToken &token): fToken(token) {}

	bool Null() {
		fToken.kind = JsonTokenKind::Null;
		return true;
	}

	bool Bool(bool b) {
		fToken.kind = JsonTokenKind::Bool;
		fToken.boolVal = b;
		return true;
	}

	bool Int(int i) {
		fToken.kind = JsonTokenKind::Int;
		fToken.int64Val = i;
		return true;
	}

	bool Uint(unsigned u) {
		fToken.kind = JsonTokenKind::UInt;
		fToken.uint64Val = u;
		return true;
	}

	bool Int64(int64_t i) {
		fToken.kind = JsonTokenKind::Int64;
		fToken.int64Val = i;
		return true;
	}

	bool Uint64(uint64_t u) {
		fToken.kind = JsonTokenKind::UInt64;
		fToken.uint64Val = u;
		return true;
	}

	bool Double(double d) {
		fToken.kind = JsonTokenKind::Double;
		fToken.doubleVal = d;
		return true;
	}

	bool RawNumber(const char* str, rapidjson::SizeType length, bool) {
		fToken.kind = JsonTokenKind::RawNumber;
		fToken.strVal = std::string_view(str, length);
		return true;
	}

	bool String(const char* str, rapidjson::SizeType length, bool) {
		fToken.kind = JsonTokenKind::String;
		fToken.strVal = std::string_view(str, length);
		return true;
	}

	bool StartObject() {
		fToken.kind = JsonTokenKind::StartObject;
		return true;
	}

	bool Key(const char* str, rapidjson::SizeType length, bool) {
		fToken.kind = JsonTokenKind::Key;
		fToken.strVal = std::string_view(str, length);
		return true;
	}

	bool EndObject(rapidjson::SizeType memberCount) {
		fToken.kind = JsonTokenKind::EndObject;
		fToken.uint64Val = memberCount;
		return true;
	}

	bool StartArray() {
		fToken.kind = JsonTokenKind::StartArray;
		return true;
	}

	bool EndArray(rapidjson::SizeType elementCount) {
		fToken.kind = JsonTokenKind::EndArray;
		fToken.uint64Val = elementCount;
		return true;
	}
};


PictureJson::PictureJson():
	fStream(std::cin)
{
}

void PictureJson::ReadToken()
{
	JsonTokenHandler handler(fToken);
	if (fRd.IterativeParseComplete()) {
		fToken.kind = JsonTokenKind::Eos;
		return;
	}
	if (!fRd.IterativeParseNext<rapidjson::kParseDefaultFlags>(fStream, handler)) {
		RaiseError();
	}
}

void PictureJson::RaiseError()
{
	// TODO: implement
	fprintf(stderr, "[!] error\n");
	abort();
}

void PictureJson::RaiseUnimplemented()
{
	fprintf(stderr, "[!] unimplemented\n");
	abort();
}

void PictureJson::Assume(bool cond)
{
	if (!cond) {
		RaiseError();
	}
}

void PictureJson::AssumeToken(JsonTokenKind tokenKind)
{
	Assume(fToken.kind == tokenKind);
}

void PictureJson::Accept(PictureVisitor &vis)
{
	fRd.IterativeParseInit();
	ReadToken();
	ReadPicture(vis);
}

void PictureJson::ReadPicture(PictureVisitor &vis)
{
	int32 version = 2;
	int32 unknown = 0;
	struct {
		bool version: 1;
		bool unknown: 1;
	} isSet {};

	AssumeToken(JsonTokenKind::StartObject); ReadToken();

	while (fToken.kind == JsonTokenKind::Key) {
		if (fToken.strVal == "version") {
			ReadToken();
			isSet.version = true;
			Assume(fToken.IsInt32());
			version = fToken.Int32Val();
			ReadToken();
		} else if (fToken.strVal == "unknown") {
			ReadToken();
			isSet.unknown = true;
			Assume(fToken.IsInt32());
			unknown = fToken.Int32Val();
			ReadToken();
		} else {
			break;
		}
	}
	Assume(isSet.version);
	// Assume(isSet.unknown);
	vis.EnterPicture(version, unknown);

	if (fToken.kind == JsonTokenKind::Key && fToken.strVal == "pictures") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartArray); ReadToken();
		vis.EnterPictures(-1);
		while (fToken.kind == JsonTokenKind::StartObject) {
			ReadPicture(vis);
		}
		AssumeToken(JsonTokenKind::EndArray); ReadToken();
		vis.ExitPictures();
	}

	if (fToken.kind == JsonTokenKind::Key && fToken.strVal == "ops") {
		ReadToken();
		ReadOps(vis);
	}

	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ExitPicture();
}

void PictureJson::ReadOps(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	vis.EnterOps();
	while (fToken.kind == JsonTokenKind::StartObject) {
		ReadToken();
		AssumeToken(JsonTokenKind::Key);
		fprintf(stderr, "%s\n", fToken.strVal.c_str());
		if (fToken.strVal == "MOVE_PEN_BY") {
			ReadToken();
			ReadMovePenBy(vis);
		} else if (fToken.strVal == "STROKE_LINE") {
			ReadToken();
			ReadStrokeLine(vis);
		} else if (fToken.strVal == "STROKE_RECT") {
			ReadToken();
			ReadStrokeRect(vis);
		} else if (fToken.strVal == "FILL_RECT") {
			ReadToken();
			ReadFillRect(vis);
		} else if (fToken.strVal == "STROKE_ROUND_RECT") {
			ReadToken();
			ReadStrokeRoundRect(vis);
		} else if (fToken.strVal == "FILL_ROUND_RECT") {
			ReadToken();
			ReadFillRoundRect(vis);
		} else if (fToken.strVal == "STROKE_BEZIER") {
			ReadToken();
			ReadStrokeBezier(vis);
		} else if (fToken.strVal == "FILL_BEZIER") {
			ReadToken();
			ReadFillBezier(vis);
		} else if (fToken.strVal == "STROKE_POLYGON") {
			ReadToken();
			ReadStrokePolygon(vis);
		} else if (fToken.strVal == "FILL_POLYGON") {
			ReadToken();
			ReadFillPolygon(vis);
		} else if (fToken.strVal == "STROKE_SHAPE") {
			ReadToken();
			ReadStrokeShape(vis);
		} else if (fToken.strVal == "FILL_SHAPE") {
			ReadToken();
			ReadFillShape(vis);
		} else if (fToken.strVal == "DRAW_STRING") {
			ReadToken();
			ReadDrawString(vis);
		} else if (fToken.strVal == "DRAW_PIXELS") {
			ReadToken();
			ReadDrawBitmap(vis);
		} else if (fToken.strVal == "DRAW_PICTURE") {
			ReadToken();
			ReadDrawPicture(vis);
		} else if (fToken.strVal == "STROKE_ARC") {
			ReadToken();
			ReadStrokeArc(vis);
		} else if (fToken.strVal == "FILL_ARC") {
			ReadToken();
			ReadFillArc(vis);
		} else if (fToken.strVal == "STROKE_ELLIPSE") {
			ReadToken();
			ReadStrokeEllipse(vis);
		} else if (fToken.strVal == "FILL_ELLIPSE") {
			ReadToken();
			ReadFillEllipse(vis);
		} else if (fToken.strVal == "DRAW_STRING_LOCATIONS") {
			ReadToken();
			ReadDrawStringLocations(vis);
		} else if (fToken.strVal == "STROKE_RECT_GRADIENT") {
			ReadToken();
			ReadStrokeRectGradient(vis);
		} else if (fToken.strVal == "FILL_RECT_GRADIENT") {
			ReadToken();
			ReadFillRectGradient(vis);
		} else if (fToken.strVal == "STROKE_ROUND_RECT_GRADIENT") {
			ReadToken();
			ReadStrokeRoundRectGradient(vis);
		} else if (fToken.strVal == "FILL_ROUND_RECT_GRADIENT") {
			ReadToken();
			ReadFillRoundRectGradient(vis);
		} else if (fToken.strVal == "STROKE_BEZIER_GRADIENT") {
			ReadToken();
			ReadStrokeBezierGradient(vis);
		} else if (fToken.strVal == "FILL_BEZIER_GRADIENT") {
			ReadToken();
			ReadFillBezierGradient(vis);
		} else if (fToken.strVal == "STROKE_POLYGON_GRADIENT") {
			ReadToken();
			ReadStrokePolygonGradient(vis);
		} else if (fToken.strVal == "FILL_POLYGON_GRADIENT") {
			ReadToken();
			ReadFillPolygonGradient(vis);
		} else if (fToken.strVal == "STROKE_SHAPE_GRADIENT") {
			ReadToken();
			ReadStrokeShapeGradient(vis);
		} else if (fToken.strVal == "FILL_SHAPE_GRADIENT") {
			ReadToken();
			ReadFillShapeGradient(vis);
		} else if (fToken.strVal == "STROKE_ARC_GRADIENT") {
			ReadToken();
			ReadStrokeArcGradient(vis);
		} else if (fToken.strVal == "FILL_ARC_GRADIENT") {
			ReadToken();
			ReadFillArcGradient(vis);
		} else if (fToken.strVal == "STROKE_ELLIPSE_GRADIENT") {
			ReadToken();
			ReadStrokeEllipseGradient(vis);
		} else if (fToken.strVal == "FILL_ELLIPSE_GRADIENT") {
			ReadToken();
			ReadFillEllipseGradient(vis);
		} else if (fToken.strVal == "ENTER_STATE_CHANGE") {
			ReadToken();
			ReadEnterStateChange(vis);
		} else if (fToken.strVal == "SET_CLIPPING_RECTS") {
			ReadToken();
			ReadSetClipping(vis);
		} else if (fToken.strVal == "CLIP_TO_PICTURE") {
			ReadToken();
			ReadClipToPicture(vis);
		} else if (fToken.strVal == "GROUP") {
			ReadToken();
			ReadGroup(vis);
		} else if (fToken.strVal == "CLEAR_CLIPPING_RECTS") {
			ReadToken();
			ReadClearClipping(vis);
		} else if (fToken.strVal == "CLIP_TO_RECT") {
			ReadToken();
			ReadClipToRect(vis);
		} else if (fToken.strVal == "CLIP_TO_SHAPE") {
			ReadToken();
			ReadClipToShape(vis);
		} else if (fToken.strVal == "SET_ORIGIN") {
			ReadToken();
			ReadSetOrigin(vis);
		} else if (fToken.strVal == "SET_PEN_LOCATION") {
			ReadToken();
			ReadSetPenLocation(vis);
		} else if (fToken.strVal == "SET_DRAWING_MODE") {
			ReadToken();
			ReadSetDrawingMode(vis);
		} else if (fToken.strVal == "SET_LINE_MODE") {
			ReadToken();
			ReadSetLineMode(vis);
		} else if (fToken.strVal == "SET_PEN_SIZE") {
			ReadToken();
			ReadSetPenSize(vis);
		} else if (fToken.strVal == "SET_SCALE") {
			ReadToken();
			ReadSetScale(vis);
		} else if (fToken.strVal == "SET_FORE_COLOR") {
			ReadToken();
			ReadSetHighColor(vis);
		} else if (fToken.strVal == "SET_BACK_COLOR") {
			ReadToken();
			ReadSetLowColor(vis);
		} else if (fToken.strVal == "SET_STIPLE_PATTERN") {
			ReadToken();
			ReadSetPattern(vis);
		} else if (fToken.strVal == "ENTER_FONT_STATE") {
			ReadToken();
			ReadEnterFontState(vis);
		} else if (fToken.strVal == "SET_BLENDING_MODE") {
			ReadToken();
			ReadSetBlendingMode(vis);
		} else if (fToken.strVal == "SET_FILL_RULE") {
			ReadToken();
			ReadSetFillRule(vis);
		} else if (fToken.strVal == "SET_FONT_FAMILY") {
			ReadToken();
			ReadSetFontFamily(vis);
		} else if (fToken.strVal == "SET_FONT_STYLE") {
			ReadToken();
			ReadSetFontStyle(vis);
		} else if (fToken.strVal == "SET_FONT_SPACING") {
			ReadToken();
			ReadSetFontSpacing(vis);
		} else if (fToken.strVal == "SET_FONT_ENCODING") {
			ReadToken();
			ReadSetFontEncoding(vis);
		} else if (fToken.strVal == "SET_FONT_FLAGS") {
			ReadToken();
			ReadSetFontFlags(vis);
		} else if (fToken.strVal == "SET_FONT_SIZE") {
			ReadToken();
			ReadSetFontSize(vis);
		} else if (fToken.strVal == "SET_FONT_ROTATE") {
			ReadToken();
			ReadSetFontRotation(vis);
		} else if (fToken.strVal == "SET_FONT_SHEAR") {
			ReadToken();
			ReadSetFontShear(vis);
		} else if (fToken.strVal == "SET_FONT_BPP") {
			ReadToken();
			ReadSetFontBpp(vis);
		} else if (fToken.strVal == "SET_FONT_FACE") {
			ReadToken();
			ReadSetFontFace(vis);
		} else if (fToken.strVal == "SET_TRANSFORM") {
			ReadToken();
			ReadSetTransform(vis);
		} else if (fToken.strVal == "AFFINE_TRANSLATE") {
			ReadToken();
			ReadTranslateBy(vis);
		} else if (fToken.strVal == "AFFINE_SCALE") {
			ReadToken();
			ReadScaleBy(vis);
		} else if (fToken.strVal == "AFFINE_ROTATE") {
			ReadToken();
			ReadRotateBy(vis);
		} else if (fToken.strVal == "BLEND_LAYER") {
			ReadToken();
			ReadBlendLayer(vis);
		} else {
			RaiseError();
		}
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.ExitOps();
}

void PictureJson::ReadMovePenBy(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeLine(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeRect(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillRect(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeRoundRect(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillRoundRect(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeBezier(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillBezier(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokePolygon(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillPolygon(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeShape(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillShape(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadDrawString(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadDrawBitmap(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadDrawPicture(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeArc(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillArc(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeEllipse(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillEllipse(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadDrawStringLocations(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeRectGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillRectGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeRoundRectGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillRoundRectGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeBezierGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillBezierGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokePolygonGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillPolygonGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeShapeGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillShapeGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeArcGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillArcGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadStrokeEllipseGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadFillEllipseGradient(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadEnterStateChange(PictureVisitor &vis)
{
	vis.EnterStateChange();
	ReadOps(vis);
	vis.ExitStateChange();
}

void PictureJson::ReadSetClipping(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadClipToPicture(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadGroup(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadClearClipping(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadClipToRect(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadClipToShape(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetOrigin(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetPenLocation(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint pt;
	struct {
		bool x: 1;
		bool y: 1;
	} isSet {};
	while (fToken.kind == JsonTokenKind::Key) {
		if (fToken.strVal == "x") {
			ReadToken();
			isSet.x = true;
			Assume(fToken.IsReal());
			pt.x = fToken.RealVal();
			ReadToken();
		} else if (fToken.strVal == "y") {
			ReadToken();
			isSet.y = true;
			Assume(fToken.IsReal());
			pt.y = fToken.RealVal();
			ReadToken();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.x);
	Assume(isSet.y);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.SetPenLocation(pt);
}

void PictureJson::ReadSetDrawingMode(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetLineMode(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetPenSize(PictureVisitor &vis)
{
	Assume(fToken.IsReal());
	float val = fToken.RealVal();
	ReadToken();
	vis.SetPenSize(val);
}

void PictureJson::ReadSetScale(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetHighColor(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetLowColor(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetPattern(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadEnterFontState(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetBlendingMode(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFillRule(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontFamily(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontStyle(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontSpacing(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontEncoding(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontFlags(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontSize(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontRotation(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontShear(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontBpp(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetFontFace(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadSetTransform(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadTranslateBy(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadScaleBy(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadRotateBy(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureJson::ReadBlendLayer(PictureVisitor &vis)
{
	RaiseUnimplemented();
}
