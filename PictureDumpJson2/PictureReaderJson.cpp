#include "PictureReaderJson.h"

#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>

#include <math.h>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>
#include <stdexcept>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>


PictureReaderJson::PictureReaderJson(std::istream &is):
	fScanner(is)
{
}

void PictureReaderJson::ReadToken()
{
	fScanner.ReadToken();
}

void PictureReaderJson::RaiseError()
{
	fScanner.RaiseError();
}

void PictureReaderJson::RaiseUnimplemented()
{
	throw std::runtime_error("PictureReaderJson: unimplemented");
}

void PictureReaderJson::Assume(bool cond)
{
	fScanner.Assume(cond);
}

void PictureReaderJson::AssumeToken(JsonTokenKind tokenKind)
{
	fScanner.AssumeToken(tokenKind);
}

void PictureReaderJson::Accept(PictureVisitor &vis)
{
	ReadToken();
	ReadPicture(vis);
}


// #pragma mark -

bool PictureReaderJson::ReadBool()
{
	return fScanner.ReadBool();
}

uint8 PictureReaderJson::ReadUint8()
{
	return fScanner.ReadUint8();
}

int32 PictureReaderJson::ReadInt32()
{
	return fScanner.ReadInt32();
}

double PictureReaderJson::ReadReal()
{
	return fScanner.ReadReal();
}

int32 PictureReaderJson::HexDigit(char digit)
{
	if (digit >= '0' && digit <= '9') {
		return digit - '0';
	}
	if (digit >= 'A' && digit <= 'F') {
		return (int32)(digit - 'A') + 10;
	}
	if (digit >= 'a' && digit <= 'f') {
		return (int32)(digit - 'a') + 10;
	}
	RaiseError();
	return 0;
}

void PictureReaderJson::ReadColor(rgb_color &color)
{
	AssumeToken(JsonTokenKind::String);
	Assume(Token().strVal.size() == 9);
	Assume(Token().strVal[0] == '#');
	color.alpha = (HexDigit(Token().strVal[1]) << 4) + HexDigit(Token().strVal[2]);
	color.red   = (HexDigit(Token().strVal[3]) << 4) + HexDigit(Token().strVal[4]);
	color.green = (HexDigit(Token().strVal[5]) << 4) + HexDigit(Token().strVal[6]);
	color.blue  = (HexDigit(Token().strVal[7]) << 4) + HexDigit(Token().strVal[8]);
	ReadToken();
}

void PictureReaderJson::ReadPoint(BPoint &pt)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	struct {
		bool x: 1;
		bool y: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "x") {
			ReadToken();
			isSet.x = true;
			pt.x = ReadReal();
		} else if (Token().strVal == "y") {
			ReadToken();
			isSet.y = true;
			pt.y = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.x);
	Assume(isSet.y);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
}

void PictureReaderJson::ReadRect(BRect &rect)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	struct {
		bool left: 1;
		bool top: 1;
		bool right: 1;
		bool bottom: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "left") {
			ReadToken();
			isSet.left = true;
			rect.left = ReadReal();
		} else if (Token().strVal == "top") {
			ReadToken();
			isSet.top = true;
			rect.top = ReadReal();
		} else if (Token().strVal == "right") {
			ReadToken();
			isSet.right = true;
			rect.right = ReadReal();
		} else if (Token().strVal == "bottom") {
			ReadToken();
			isSet.bottom = true;
			rect.bottom = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.left);
	Assume(isSet.top);
	Assume(isSet.right);
	Assume(isSet.bottom);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
}

void PictureReaderJson::ReadEscapementDelta(escapement_delta &delta)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	struct {
		bool nonspace: 1;
		bool space: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "nonspace") {
			ReadToken();
			isSet.nonspace = true;
			delta.nonspace = ReadReal();
		} else if (Token().strVal == "space") {
			ReadToken();
			isSet.space = true;
			delta.space = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.nonspace);
	Assume(isSet.space);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
}

void PictureReaderJson::ReadShape(BShape &shape)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	while (Token().kind != JsonTokenKind::EndArray) {
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		AssumeToken(JsonTokenKind::Key);
		if (Token().strVal == "MoveTo") {
			ReadToken();
			BPoint pt;
			ReadPoint(pt);
			shape.MoveTo(pt);
		} else if (Token().strVal == "LineTo") {
			ReadToken();
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			while (Token().kind != JsonTokenKind::EndArray) {
				BPoint pt;
				ReadPoint(pt);
				shape.LineTo(pt);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "BezierTo") {
			ReadToken();
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			while (Token().kind != JsonTokenKind::EndArray) {
				BPoint pt1, pt2, pt3;
				ReadPoint(pt1);
				ReadPoint(pt2);
				ReadPoint(pt3);
				shape.BezierTo(pt1, pt2, pt3);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "Close") {
			ReadToken();
			AssumeToken(JsonTokenKind::StartObject); ReadToken();
			AssumeToken(JsonTokenKind::EndObject); ReadToken();
			shape.Close();
		} else if (Token().strVal == "ArcTo") {
			ReadToken();
			AssumeToken(JsonTokenKind::StartObject); ReadToken();
			float rx;
			float ry;
			float angle;
			bool largeArc;
			bool ccw;
			BPoint point;
			struct {
				bool rx: 1;
				bool ry: 1;
				bool angle: 1;
				bool largeArc: 1;
				bool ccw: 1;
				bool point: 1;
			} isSet {};
			while (Token().kind == JsonTokenKind::Key) {
				if (Token().strVal == "rx") {
					ReadToken();
					isSet.rx = true;
					rx = ReadReal();
				} else if (Token().strVal == "ry") {
					ReadToken();
					isSet.ry = true;
					ry = ReadReal();
				} else if (Token().strVal == "angle") {
					ReadToken();
					isSet.angle = true;
					angle = ReadReal();
				} else if (Token().strVal == "largeArc") {
					ReadToken();
					isSet.largeArc = true;
					largeArc = ReadBool();
				} else if (Token().strVal == "ccw") {
					ReadToken();
					isSet.ccw = true;
					ccw = ReadBool();
				} else if (Token().strVal == "point") {
					ReadToken();
					isSet.point = true;
					ReadPoint(point);
				} else {
					RaiseError();
				}
			}
			Assume(isSet.rx);
			Assume(isSet.ry);
			Assume(isSet.angle);
			Assume(isSet.largeArc);
			Assume(isSet.ccw);
			Assume(isSet.point);
			AssumeToken(JsonTokenKind::EndObject); ReadToken();
			shape.ArcTo(rx, ry, angle, largeArc, ccw, point);
		}
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
}

void PictureReaderJson::ReadGradientStops(BGradient &gradient)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	int32 i = 0;
	while (Token().kind != JsonTokenKind::EndArray) {
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		BGradient::ColorStop cs;
		struct {
			bool color: 1;
			bool offset: 1;
		} isSet {};
		while (Token().kind == JsonTokenKind::Key) {
			if (Token().strVal == "color") {
				ReadToken();
				isSet.color = true;
				ReadColor(cs.color);
			} else if (Token().strVal == "offset") {
				ReadToken();
				isSet.offset = true;
				cs.offset = ReadReal();
			} else {
				RaiseError();
			}
		}
		Assume(isSet.color);
		Assume(isSet.offset);
		gradient.AddColorStop(cs, i++);
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
}

void PictureReaderJson::ReadGradient(ObjectDeleter<BGradient> &outGradient)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	AssumeToken(JsonTokenKind::Key);
	if (Token().strVal == "BGradientLinear") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		ObjectDeleter<BGradientLinear> gradient(new BGradientLinear());
		struct {
			bool stops: 1;
			bool start: 1;
			bool end: 1;
		} isSet {};
		while (Token().kind == JsonTokenKind::Key) {
			if (Token().strVal == "stops") {
				ReadToken();
				isSet.stops = true;
				ReadGradientStops(*gradient.Get());
			} else if (Token().strVal == "start") {
				ReadToken();
				isSet.start = true;
				BPoint start;
				ReadPoint(start);
				gradient->SetStart(start);
			} else if (Token().strVal == "end") {
				ReadToken();
				isSet.end = true;
				BPoint end;
				ReadPoint(end);
				gradient->SetEnd(end);
			} else {
				RaiseError();
			}
		}
		Assume(isSet.stops);
		Assume(isSet.start);
		Assume(isSet.end);
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
		outGradient.SetTo(gradient.Detach());
	} else if (Token().strVal == "BGradientRadial") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		ObjectDeleter<BGradientRadial> gradient(new BGradientRadial());
		struct {
			bool stops: 1;
			bool center: 1;
			bool radius: 1;
		} isSet {};
		while (Token().kind == JsonTokenKind::Key) {
			if (Token().strVal == "stops") {
				ReadToken();
				isSet.stops = true;
				ReadGradientStops(*gradient.Get());
			} else if (Token().strVal == "center") {
				ReadToken();
				isSet.center = true;
				BPoint center;
				ReadPoint(center);
				gradient->SetCenter(center);
			} else if (Token().strVal == "radius") {
				ReadToken();
				isSet.radius = true;
				float radius = ReadReal();
				gradient->SetRadius(radius);
			} else {
				RaiseError();
			}
		}
		Assume(isSet.stops);
		Assume(isSet.center);
		Assume(isSet.radius);
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
		outGradient.SetTo(gradient.Detach());
	} else if (Token().strVal == "BGradientRadialFocus") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		ObjectDeleter<BGradientRadialFocus> gradient(new BGradientRadialFocus());
		struct {
			bool stops: 1;
			bool center: 1;
			bool focus: 1;
			bool radius: 1;
		} isSet {};
		while (Token().kind == JsonTokenKind::Key) {
			if (Token().strVal == "stops") {
				ReadToken();
				isSet.stops = true;
				ReadGradientStops(*gradient.Get());
			} else if (Token().strVal == "center") {
				ReadToken();
				isSet.center = true;
				BPoint center;
				ReadPoint(center);
				gradient->SetCenter(center);
			} else if (Token().strVal == "focus") {
				ReadToken();
				isSet.focus = true;
				BPoint focus;
				ReadPoint(focus);
				gradient->SetFocal(focus);
			} else if (Token().strVal == "radius") {
				ReadToken();
				isSet.radius = true;
				float radius = ReadReal();
				gradient->SetRadius(radius);
			} else {
				RaiseError();
			}
		}
		Assume(isSet.stops);
		Assume(isSet.center);
		Assume(isSet.focus);
		Assume(isSet.radius);
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
		outGradient.SetTo(gradient.Detach());
	} else if (Token().strVal == "BGradientDiamond") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		ObjectDeleter<BGradientDiamond> gradient(new BGradientDiamond());
		struct {
			bool stops: 1;
			bool center: 1;
		} isSet {};
		while (Token().kind == JsonTokenKind::Key) {
			if (Token().strVal == "stops") {
				ReadToken();
				isSet.stops = true;
				ReadGradientStops(*gradient.Get());
			} else if (Token().strVal == "center") {
				ReadToken();
				isSet.center = true;
				BPoint center;
				ReadPoint(center);
				gradient->SetCenter(center);
			} else {
				RaiseError();
			}
		}
		Assume(isSet.stops);
		Assume(isSet.center);
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
		outGradient.SetTo(gradient.Detach());
	} else if (Token().strVal == "BGradientConic") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartObject); ReadToken();
		ObjectDeleter<BGradientConic> gradient(new BGradientConic());
		struct {
			bool stops: 1;
			bool center: 1;
			bool angle: 1;
		} isSet {};
		while (Token().kind == JsonTokenKind::Key) {
			if (Token().strVal == "stops") {
				ReadToken();
				isSet.stops = true;
				ReadGradientStops(*gradient.Get());
			} else if (Token().strVal == "center") {
				ReadToken();
				isSet.center = true;
				BPoint center;
				ReadPoint(center);
				gradient->SetCenter(center);
			} else if (Token().strVal == "angle") {
				ReadToken();
				isSet.angle = true;
				float angle = ReadReal();
				gradient->SetAngle(angle);
			} else {
				RaiseError();
			}
		}
		Assume(isSet.stops);
		Assume(isSet.center);
		Assume(isSet.angle);
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
		outGradient.SetTo(gradient.Detach());
	} else {
		RaiseError();
	}
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
}

void PictureReaderJson::ReadColorSpace(color_space &val)
{
	AssumeToken(JsonTokenKind::String);
	if (Token().strVal == "B_NO_COLOR_SPACE") {
		val = B_NO_COLOR_SPACE;
	} else if (Token().strVal == "B_RGBA64") {
		val = B_RGBA64;
	} else if (Token().strVal == "B_RGB48") {
		val = B_RGB48;
	} else if (Token().strVal == "B_RGB32") {
		val = B_RGB32;
	} else if (Token().strVal == "B_RGBA32") {
		val = B_RGBA32;
	} else if (Token().strVal == "B_RGB24") {
		val = B_RGB24;
	} else if (Token().strVal == "B_RGB16") {
		val = B_RGB16;
	} else if (Token().strVal == "B_RGB15") {
		val = B_RGB15;
	} else if (Token().strVal == "B_RGBA15") {
		val = B_RGBA15;
	} else if (Token().strVal == "B_CMAP8") {
		val = B_CMAP8;
	} else if (Token().strVal == "B_GRAY8") {
		val = B_GRAY8;
	} else if (Token().strVal == "B_GRAY1") {
		val = B_GRAY1;
	} else if (Token().strVal == "B_RGBA64_BIG") {
		val = B_RGBA64_BIG;
	} else if (Token().strVal == "B_RGB48_BIG") {
		val = B_RGB48_BIG;
	} else if (Token().strVal == "B_RGB32_BIG") {
		val = B_RGB32_BIG;
	} else if (Token().strVal == "B_RGBA32_BIG") {
		val = B_RGBA32_BIG;
	} else if (Token().strVal == "B_RGB24_BIG") {
		val = B_RGB24_BIG;
	} else if (Token().strVal == "B_RGB16_BIG") {
		val = B_RGB16_BIG;
	} else if (Token().strVal == "B_RGB15_BIG") {
		val = B_RGB15_BIG;
	} else if (Token().strVal == "B_RGBA15_BIG") {
		val = B_RGBA15_BIG;
	} else if (Token().strVal == "B_YCbCr422") {
		val = B_YCbCr422;
	} else if (Token().strVal == "B_YCbCr411") {
		val = B_YCbCr411;
	} else if (Token().strVal == "B_YCbCr444") {
		val = B_YCbCr444;
	} else if (Token().strVal == "B_YCbCr420") {
		val = B_YCbCr420;
	} else if (Token().strVal == "B_YUV422") {
		val = B_YUV422;
	} else if (Token().strVal == "B_YUV411") {
		val = B_YUV411;
	} else if (Token().strVal == "B_YUV444") {
		val = B_YUV444;
	} else if (Token().strVal == "B_YUV420") {
		val = B_YUV420;
	} else if (Token().strVal == "B_YUV9") {
		val = B_YUV9;
	} else if (Token().strVal == "B_YUV12") {
		val = B_YUV12;
	} else if (Token().strVal == "B_UVL24") {
		val = B_UVL24;
	} else if (Token().strVal == "B_UVL32") {
		val = B_UVL32;
	} else if (Token().strVal == "B_UVLA32") {
		val = B_UVLA32;
	} else if (Token().strVal == "B_LAB24") {
		val = B_LAB24;
	} else if (Token().strVal == "B_LAB32") {
		val = B_LAB32;
	} else if (Token().strVal == "B_LABA32") {
		val = B_LABA32;
	} else if (Token().strVal == "B_HSI24") {
		val = B_HSI24;
	} else if (Token().strVal == "B_HSI32") {
		val = B_HSI32;
	} else if (Token().strVal == "B_HSIA32") {
		val = B_HSIA32;
	} else if (Token().strVal == "B_HSV24") {
		val = B_HSV24;
	} else if (Token().strVal == "B_HSV32") {
		val = B_HSV32;
	} else if (Token().strVal == "B_HSVA32") {
		val = B_HSVA32;
	} else if (Token().strVal == "B_HLS24") {
		val = B_HLS24;
	} else if (Token().strVal == "B_HLS32") {
		val = B_HLS32;
	} else if (Token().strVal == "B_HLSA32") {
		val = B_HLSA32;
	} else if (Token().strVal == "B_CMY24") {
		val = B_CMY24;
	} else if (Token().strVal == "B_CMY32") {
		val = B_CMY32;
	} else if (Token().strVal == "B_CMYA32") {
		val = B_CMYA32;
	} else if (Token().strVal == "B_CMYK32") {
		val = B_CMYK32;
	} else {
		RaiseError();
	}
	ReadToken();
}


// #pragma mark -

void PictureReaderJson::ReadPicture(PictureVisitor &vis)
{
	int32 version = 2;
	int32 endian = 0;
	struct {
		bool version: 1;
		bool endian: 1;
	} isSet {};

	AssumeToken(JsonTokenKind::StartObject); ReadToken();

	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "version") {
			ReadToken();
			isSet.version = true;
			version = ReadInt32();
		} else if (Token().strVal == "endian") {
			ReadToken();
			isSet.endian = true;
			endian = ReadInt32();
		} else {
			break;
		}
	}
	Assume(isSet.version);
	// Assume(isSet.endian);
	vis.EnterPicture(version, endian);

	if (Token().kind == JsonTokenKind::Key && Token().strVal == "pictures") {
		ReadToken();
		AssumeToken(JsonTokenKind::StartArray); ReadToken();
		vis.EnterPictures(-1);
		while (Token().kind == JsonTokenKind::StartObject) {
			ReadPicture(vis);
		}
		AssumeToken(JsonTokenKind::EndArray); ReadToken();
		vis.ExitPictures();
	}

	if (Token().kind == JsonTokenKind::Key && Token().strVal == "ops") {
		ReadToken();
		vis.EnterOps();
		ReadOps(vis);
		vis.ExitOps();
	}

	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ExitPicture();
}

void PictureReaderJson::ReadOps(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	while (Token().kind == JsonTokenKind::StartObject) {
		ReadToken();
		AssumeToken(JsonTokenKind::Key);
		if (Token().strVal == "MOVE_PEN_BY") {
			ReadToken();
			ReadMovePenBy(vis);
		} else if (Token().strVal == "STROKE_LINE") {
			ReadToken();
			ReadStrokeLine(vis);
		} else if (Token().strVal == "STROKE_RECT") {
			ReadToken();
			ReadStrokeRect(vis);
		} else if (Token().strVal == "FILL_RECT") {
			ReadToken();
			ReadFillRect(vis);
		} else if (Token().strVal == "STROKE_ROUND_RECT") {
			ReadToken();
			ReadStrokeRoundRect(vis);
		} else if (Token().strVal == "FILL_ROUND_RECT") {
			ReadToken();
			ReadFillRoundRect(vis);
		} else if (Token().strVal == "STROKE_BEZIER") {
			ReadToken();
			ReadStrokeBezier(vis);
		} else if (Token().strVal == "FILL_BEZIER") {
			ReadToken();
			ReadFillBezier(vis);
		} else if (Token().strVal == "STROKE_POLYGON") {
			ReadToken();
			ReadStrokePolygon(vis);
		} else if (Token().strVal == "FILL_POLYGON") {
			ReadToken();
			ReadFillPolygon(vis);
		} else if (Token().strVal == "STROKE_SHAPE") {
			ReadToken();
			ReadStrokeShape(vis);
		} else if (Token().strVal == "FILL_SHAPE") {
			ReadToken();
			ReadFillShape(vis);
		} else if (Token().strVal == "DRAW_STRING") {
			ReadToken();
			ReadDrawString(vis);
		} else if (Token().strVal == "DRAW_PIXELS") {
			ReadToken();
			ReadDrawBitmap(vis);
		} else if (Token().strVal == "DRAW_PICTURE") {
			ReadToken();
			ReadDrawPicture(vis);
		} else if (Token().strVal == "STROKE_ARC") {
			ReadToken();
			ReadStrokeArc(vis);
		} else if (Token().strVal == "FILL_ARC") {
			ReadToken();
			ReadFillArc(vis);
		} else if (Token().strVal == "STROKE_ELLIPSE") {
			ReadToken();
			ReadStrokeEllipse(vis);
		} else if (Token().strVal == "FILL_ELLIPSE") {
			ReadToken();
			ReadFillEllipse(vis);
		} else if (Token().strVal == "DRAW_STRING_LOCATIONS") {
			ReadToken();
			ReadDrawStringLocations(vis);
		} else if (Token().strVal == "STROKE_RECT_GRADIENT") {
			ReadToken();
			ReadStrokeRectGradient(vis);
		} else if (Token().strVal == "FILL_RECT_GRADIENT") {
			ReadToken();
			ReadFillRectGradient(vis);
		} else if (Token().strVal == "STROKE_ROUND_RECT_GRADIENT") {
			ReadToken();
			ReadStrokeRoundRectGradient(vis);
		} else if (Token().strVal == "FILL_ROUND_RECT_GRADIENT") {
			ReadToken();
			ReadFillRoundRectGradient(vis);
		} else if (Token().strVal == "STROKE_BEZIER_GRADIENT") {
			ReadToken();
			ReadStrokeBezierGradient(vis);
		} else if (Token().strVal == "FILL_BEZIER_GRADIENT") {
			ReadToken();
			ReadFillBezierGradient(vis);
		} else if (Token().strVal == "STROKE_POLYGON_GRADIENT") {
			ReadToken();
			ReadStrokePolygonGradient(vis);
		} else if (Token().strVal == "FILL_POLYGON_GRADIENT") {
			ReadToken();
			ReadFillPolygonGradient(vis);
		} else if (Token().strVal == "STROKE_SHAPE_GRADIENT") {
			ReadToken();
			ReadStrokeShapeGradient(vis);
		} else if (Token().strVal == "FILL_SHAPE_GRADIENT") {
			ReadToken();
			ReadFillShapeGradient(vis);
		} else if (Token().strVal == "STROKE_ARC_GRADIENT") {
			ReadToken();
			ReadStrokeArcGradient(vis);
		} else if (Token().strVal == "FILL_ARC_GRADIENT") {
			ReadToken();
			ReadFillArcGradient(vis);
		} else if (Token().strVal == "STROKE_ELLIPSE_GRADIENT") {
			ReadToken();
			ReadStrokeEllipseGradient(vis);
		} else if (Token().strVal == "FILL_ELLIPSE_GRADIENT") {
			ReadToken();
			ReadFillEllipseGradient(vis);
		} else if (Token().strVal == "ENTER_STATE_CHANGE") {
			ReadToken();
			ReadEnterStateChange(vis);
		} else if (Token().strVal == "SET_CLIPPING_RECTS") {
			ReadToken();
			ReadSetClipping(vis);
		} else if (Token().strVal == "CLIP_TO_PICTURE") {
			ReadToken();
			ReadClipToPicture(vis);
		} else if (Token().strVal == "GROUP") {
			ReadToken();
			ReadGroup(vis);
		} else if (Token().strVal == "CLEAR_CLIPPING_RECTS") {
			ReadToken();
			ReadClearClipping(vis);
		} else if (Token().strVal == "CLIP_TO_RECT") {
			ReadToken();
			ReadClipToRect(vis);
		} else if (Token().strVal == "CLIP_TO_SHAPE") {
			ReadToken();
			ReadClipToShape(vis);
		} else if (Token().strVal == "SET_ORIGIN") {
			ReadToken();
			ReadSetOrigin(vis);
		} else if (Token().strVal == "SET_PEN_LOCATION") {
			ReadToken();
			ReadSetPenLocation(vis);
		} else if (Token().strVal == "SET_DRAWING_MODE") {
			ReadToken();
			ReadSetDrawingMode(vis);
		} else if (Token().strVal == "SET_LINE_MODE") {
			ReadToken();
			ReadSetLineMode(vis);
		} else if (Token().strVal == "SET_PEN_SIZE") {
			ReadToken();
			ReadSetPenSize(vis);
		} else if (Token().strVal == "SET_SCALE") {
			ReadToken();
			ReadSetScale(vis);
		} else if (Token().strVal == "SET_FORE_COLOR") {
			ReadToken();
			ReadSetHighColor(vis);
		} else if (Token().strVal == "SET_BACK_COLOR") {
			ReadToken();
			ReadSetLowColor(vis);
		} else if (Token().strVal == "SET_STIPLE_PATTERN") {
			ReadToken();
			ReadSetPattern(vis);
		} else if (Token().strVal == "ENTER_FONT_STATE") {
			ReadToken();
			ReadEnterFontState(vis);
		} else if (Token().strVal == "SET_BLENDING_MODE") {
			ReadToken();
			ReadSetBlendingMode(vis);
		} else if (Token().strVal == "SET_FILL_RULE") {
			ReadToken();
			ReadSetFillRule(vis);
		} else if (Token().strVal == "SET_FONT_FAMILY") {
			ReadToken();
			ReadSetFontFamily(vis);
		} else if (Token().strVal == "SET_FONT_STYLE") {
			ReadToken();
			ReadSetFontStyle(vis);
		} else if (Token().strVal == "SET_FONT_SPACING") {
			ReadToken();
			ReadSetFontSpacing(vis);
		} else if (Token().strVal == "SET_FONT_ENCODING") {
			ReadToken();
			ReadSetFontEncoding(vis);
		} else if (Token().strVal == "SET_FONT_FLAGS") {
			ReadToken();
			ReadSetFontFlags(vis);
		} else if (Token().strVal == "SET_FONT_SIZE") {
			ReadToken();
			ReadSetFontSize(vis);
		} else if (Token().strVal == "SET_FONT_ROTATE") {
			ReadToken();
			ReadSetFontRotation(vis);
		} else if (Token().strVal == "SET_FONT_SHEAR") {
			ReadToken();
			ReadSetFontShear(vis);
		} else if (Token().strVal == "SET_FONT_BPP") {
			ReadToken();
			ReadSetFontBpp(vis);
		} else if (Token().strVal == "SET_FONT_FACE") {
			ReadToken();
			ReadSetFontFace(vis);
		} else if (Token().strVal == "SET_FONT_FALSE_BOLD_WIDTH") {
			ReadToken();
			ReadSetFontFalseBoldWidth(vis);
		} else if (Token().strVal == "SET_TRANSFORM") {
			ReadToken();
			ReadSetTransform(vis);
		} else if (Token().strVal == "AFFINE_TRANSLATE") {
			ReadToken();
			ReadTranslateBy(vis);
		} else if (Token().strVal == "AFFINE_SCALE") {
			ReadToken();
			ReadScaleBy(vis);
		} else if (Token().strVal == "AFFINE_ROTATE") {
			ReadToken();
			ReadRotateBy(vis);
		} else if (Token().strVal == "BLEND_LAYER") {
			ReadToken();
			ReadBlendLayer(vis);
		} else {
			RaiseError();
		}
		AssumeToken(JsonTokenKind::EndObject); ReadToken();
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
}


// #pragma mark -

void PictureReaderJson::ReadMovePenBy(PictureVisitor &vis)
{
	BPoint dp;
	ReadPoint(dp);
	vis.MovePenBy(dp.x, dp.y);
}

void PictureReaderJson::ReadStrokeLine(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint start, end;
	struct {
		bool start: 1;
		bool end: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "start") {
			ReadToken();
			isSet.start = true;
			ReadPoint(start);
		} else if (Token().strVal == "end") {
			ReadToken();
			isSet.end = true;
			ReadPoint(end);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.start);
	Assume(isSet.end);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawLine(start, end, {.isStroke = true});
}

void PictureReaderJson::ReadStrokeRect(PictureVisitor &vis)
{
	BRect rect;
	ReadRect(rect);
	vis.DrawRect(rect, {.isStroke = true});
}

void PictureReaderJson::ReadFillRect(PictureVisitor &vis)
{
	BRect rect;
	ReadRect(rect);
	vis.DrawRect(rect, {.isStroke = false});
}

void PictureReaderJson::ReadStrokeRoundRect(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	BPoint radius;
	struct {
		bool rect: 1;
		bool radius: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.radius);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawRoundRect(rect, radius, {.isStroke = true});
}

void PictureReaderJson::ReadFillRoundRect(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	BPoint radius;
	struct {
		bool rect: 1;
		bool radius: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.radius);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawRoundRect(rect, radius, {.isStroke = false});
}

void PictureReaderJson::ReadStrokeBezier(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	BPoint points[4];
	for (int32 i = 0; i < 4; i++) {
		ReadPoint(points[i]);
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.DrawBezier(points, {.isStroke = true});
}

void PictureReaderJson::ReadFillBezier(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	BPoint points[4];
	for (int32 i = 0; i < 4; i++) {
		ReadPoint(points[i]);
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.DrawBezier(points, {.isStroke = false});
}

void PictureReaderJson::ReadStrokePolygon(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	std::vector<BPoint> points;
	bool isClosed;
	struct {
		bool points: 1;
		bool isClosed: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "points") {
			ReadToken();
			isSet.points = true;
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			while (Token().kind != JsonTokenKind::EndArray) {
				BPoint pt;
				ReadPoint(pt);
				points.push_back(pt);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "isClosed") {
			ReadToken();
			isSet.isClosed = true;
			isClosed = ReadBool();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.points);
	Assume(isSet.isClosed);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawPolygon(points.size(), points.data(), isClosed, {.isStroke = true});
}

void PictureReaderJson::ReadFillPolygon(PictureVisitor &vis)
{
	std::vector<BPoint> points;
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	while (Token().kind != JsonTokenKind::EndArray) {
		BPoint pt;
		ReadPoint(pt);
		points.push_back(pt);
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.DrawPolygon(points.size(), points.data(), true, {.isStroke = false});
}

void PictureReaderJson::ReadStrokeShape(PictureVisitor &vis)
{
	BShape shape;
	ReadShape(shape);
	vis.DrawShape(shape, {.isStroke = true});
}

void PictureReaderJson::ReadFillShape(PictureVisitor &vis)
{
	BShape shape;
	ReadShape(shape);
	vis.DrawShape(shape, {.isStroke = false});
}

void PictureReaderJson::ReadDrawString(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	std::string string;
	escapement_delta delta {};
	struct {
		bool string: 1;
		bool delta: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "string") {
			ReadToken();
			isSet.string = true;
			AssumeToken(JsonTokenKind::String);
			string = Token().strVal;
			ReadToken();
		} else if (Token().strVal == "delta") {
			ReadToken();
			isSet.delta = true;
			ReadEscapementDelta(delta);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.string);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawString(string.c_str(), string.size(), delta);
}

void PictureReaderJson::ReadDrawBitmap(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();

	BRect sourceRect;
	BRect destinationRect;
	int32 width;
	int32 height;
	int32 bytesPerRow;
	color_space colorSpace;
	int32 flags;
	std::vector<uint8> data;

	struct {
		bool sourceRect: 1;
		bool destinationRect: 1;
		bool width: 1;
		bool height: 1;
		bool bytesPerRow: 1;
		bool colorSpace: 1;
		bool flags: 1;
		bool data: 1;
	} isSet {};

	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "sourceRect") {
			ReadToken();
			isSet.sourceRect = true;
			ReadRect(sourceRect);
		} else if (Token().strVal == "destinationRect") {
			ReadToken();
			isSet.destinationRect = true;
			ReadRect(destinationRect);
		} else if (Token().strVal == "width") {
			ReadToken();
			isSet.width = true;
			width = ReadInt32();
		} else if (Token().strVal == "height") {
			ReadToken();
			isSet.height = true;
			height = ReadInt32();
		} else if (Token().strVal == "bytesPerRow") {
			ReadToken();
			isSet.bytesPerRow = true;
			bytesPerRow = ReadInt32();
		} else if (Token().strVal == "colorSpace") {
			ReadToken();
			isSet.colorSpace = true;
			ReadColorSpace(colorSpace);
		} else if (Token().strVal == "flags") {
			ReadToken();
			isSet.flags = true;
			flags = ReadInt32();
		} else if (Token().strVal == "data") {
			ReadToken();
			isSet.data = true;
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			while (Token().kind != JsonTokenKind::EndArray) {
				uint8 val = ReadUint8();
				data.push_back(val);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else {
			RaiseError();
		}
	}

	Assume(isSet.sourceRect);
	Assume(isSet.destinationRect);
	Assume(isSet.width);
	Assume(isSet.height);
	Assume(isSet.bytesPerRow);
	Assume(isSet.colorSpace);
	Assume(isSet.flags);
	Assume(isSet.data);

	AssumeToken(JsonTokenKind::EndObject); ReadToken();

	vis.DrawBitmap(
		sourceRect,
		destinationRect,
		width,
		height,
		bytesPerRow,
		colorSpace,
		flags,
		data.data(),
		data.size()
	);
}

void PictureReaderJson::ReadDrawPicture(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint where;
	int32 token;
	struct {
		bool where: 1;
		bool token: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "where") {
			ReadToken();
			isSet.where = true;
			ReadPoint(where);
		} else if (Token().strVal == "token") {
			ReadToken();
			isSet.token = true;
			token = ReadInt32();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.where);
	Assume(isSet.token);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawPicture(where, token);
}

void PictureReaderJson::ReadStrokeArc(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint center;
	BPoint radius;
	float startTheta;
	float arcTheta;
	struct {
		bool center: 1;
		bool radius: 1;
		bool startTheta: 1;
		bool arcTheta: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "center") {
			ReadToken();
			isSet.center = true;
			ReadPoint(center);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else if (Token().strVal == "startTheta") {
			ReadToken();
			isSet.startTheta = true;
			startTheta = ReadReal();
		} else if (Token().strVal == "arcTheta") {
			ReadToken();
			isSet.arcTheta = true;
			arcTheta = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.center);
	Assume(isSet.radius);
	Assume(isSet.startTheta);
	Assume(isSet.arcTheta);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawArc(center, radius, startTheta, arcTheta, {.isStroke = true});
}

void PictureReaderJson::ReadFillArc(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint center;
	BPoint radius;
	float startTheta;
	float arcTheta;
	struct {
		bool center: 1;
		bool radius: 1;
		bool startTheta: 1;
		bool arcTheta: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "center") {
			ReadToken();
			isSet.center = true;
			ReadPoint(center);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else if (Token().strVal == "startTheta") {
			ReadToken();
			isSet.startTheta = true;
			startTheta = ReadReal();
		} else if (Token().strVal == "arcTheta") {
			ReadToken();
			isSet.arcTheta = true;
			arcTheta = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.center);
	Assume(isSet.radius);
	Assume(isSet.startTheta);
	Assume(isSet.arcTheta);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawArc(center, radius, startTheta, arcTheta, {.isStroke = false});
}

void PictureReaderJson::ReadStrokeEllipse(PictureVisitor &vis)
{
	BRect rect;
	ReadRect(rect);
	vis.DrawEllipse(rect, {.isStroke = true});
}

void PictureReaderJson::ReadFillEllipse(PictureVisitor &vis)
{
	BRect rect;
	ReadRect(rect);
	vis.DrawEllipse(rect, {.isStroke = false});
}

void PictureReaderJson::ReadDrawStringLocations(PictureVisitor &vis)
{
	RaiseUnimplemented();
}

void PictureReaderJson::ReadStrokeRectGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool rect: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawRect(rect, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillRectGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool rect: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawRect(rect, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadStrokeRoundRectGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	BPoint radius;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool rect: 1;
		bool radius: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.radius);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawRoundRect(rect, radius, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillRoundRectGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	BPoint radius;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool rect: 1;
		bool radius: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.radius);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawRoundRect(rect, radius, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadStrokeBezierGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint points[4];
	ObjectDeleter<BGradient> gradient;
	struct {
		bool points: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "points") {
			ReadToken();
			isSet.points = true;
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			for (int32 i = 0; i < 4; i++) {
				ReadPoint(points[i]);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.points);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawBezier(points, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillBezierGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint points[4];
	ObjectDeleter<BGradient> gradient;
	struct {
		bool points: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "points") {
			ReadToken();
			isSet.points = true;
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			for (int32 i = 0; i < 4; i++) {
				ReadPoint(points[i]);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.points);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawBezier(points, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadStrokePolygonGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	std::vector<BPoint> points;
	bool isClosed;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool points: 1;
		bool isClosed: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "points") {
			ReadToken();
			isSet.points = true;
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			while (Token().kind != JsonTokenKind::EndArray) {
				BPoint pt;
				ReadPoint(pt);
				points.push_back(pt);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "isClosed") {
			ReadToken();
			isSet.isClosed = true;
			isClosed = ReadBool();
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.points);
	Assume(isSet.isClosed);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawPolygon(points.size(), points.data(), isClosed, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillPolygonGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	std::vector<BPoint> points;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool points: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "points") {
			ReadToken();
			isSet.points = true;
			AssumeToken(JsonTokenKind::StartArray); ReadToken();
			while (Token().kind != JsonTokenKind::EndArray) {
				BPoint pt;
				ReadPoint(pt);
				points.push_back(pt);
			}
			AssumeToken(JsonTokenKind::EndArray); ReadToken();
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.points);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawPolygon(points.size(), points.data(), true, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadStrokeShapeGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BShape shape;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool shape: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "shape") {
			ReadToken();
			isSet.shape = true;
			ReadShape(shape);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.shape);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawShape(shape, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillShapeGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BShape shape;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool shape: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "shape") {
			ReadToken();
			isSet.shape = true;
			ReadShape(shape);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.shape);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawShape(shape, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadStrokeArcGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint center;
	BPoint radius;
	float startTheta;
	float arcTheta;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool center: 1;
		bool radius: 1;
		bool startTheta: 1;
		bool arcTheta: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "center") {
			ReadToken();
			isSet.center = true;
			ReadPoint(center);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else if (Token().strVal == "startTheta") {
			ReadToken();
			isSet.startTheta = true;
			startTheta = ReadReal();
		} else if (Token().strVal == "arcTheta") {
			ReadToken();
			isSet.arcTheta = true;
			arcTheta = ReadReal();
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.center);
	Assume(isSet.radius);
	Assume(isSet.startTheta);
	Assume(isSet.arcTheta);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawArc(center, radius, startTheta, arcTheta, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillArcGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BPoint center;
	BPoint radius;
	float startTheta;
	float arcTheta;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool center: 1;
		bool radius: 1;
		bool startTheta: 1;
		bool arcTheta: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "center") {
			ReadToken();
			isSet.center = true;
			ReadPoint(center);
		} else if (Token().strVal == "radius") {
			ReadToken();
			isSet.radius = true;
			ReadPoint(radius);
		} else if (Token().strVal == "startTheta") {
			ReadToken();
			isSet.startTheta = true;
			startTheta = ReadReal();
		} else if (Token().strVal == "arcTheta") {
			ReadToken();
			isSet.arcTheta = true;
			arcTheta = ReadReal();
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.center);
	Assume(isSet.radius);
	Assume(isSet.startTheta);
	Assume(isSet.arcTheta);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawArc(center, radius, startTheta, arcTheta, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadStrokeEllipseGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool rect: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawEllipse(rect, {.isStroke = true, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadFillEllipseGradient(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BRect rect;
	ObjectDeleter<BGradient> gradient;
	struct {
		bool rect: 1;
		bool gradient: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else if (Token().strVal == "gradient") {
			ReadToken();
			isSet.gradient = true;
			ReadGradient(gradient);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.rect);
	Assume(isSet.gradient);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.DrawEllipse(rect, {.isStroke = false, .gradient = gradient.Get()});
}

void PictureReaderJson::ReadEnterStateChange(PictureVisitor &vis)
{
	vis.EnterStateChange();
	ReadOps(vis);
	vis.ExitStateChange();
}

void PictureReaderJson::ReadSetClipping(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	BRegion region;
	while (Token().kind != JsonTokenKind::EndArray) {
		BRect rect;
		ReadRect(rect);
		region.Include(rect);
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.SetClipping(region);
}

void PictureReaderJson::ReadClipToPicture(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	int32 token;
	BPoint where;
	bool inverse;
	struct {
		bool token: 1;
		bool where: 1;
		bool inverse: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "token") {
			ReadToken();
			isSet.token = true;
			token = ReadInt32();
		} else if (Token().strVal == "where") {
			ReadToken();
			isSet.where = true;
			ReadPoint(where);
		} else if (Token().strVal == "inverse") {
			ReadToken();
			isSet.inverse = true;
			inverse = ReadBool();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.token);
	Assume(isSet.where);
	Assume(isSet.inverse);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ClipToPicture(token, where, inverse);
}

void PictureReaderJson::ReadGroup(PictureVisitor &vis)
{
	vis.PushState();
	ReadOps(vis);
	vis.PopState();
}

void PictureReaderJson::ReadClearClipping(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ClearClipping();
}

void PictureReaderJson::ReadClipToRect(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	bool inverse;
	BRect rect;
	struct {
		bool inverse: 1;
		bool rect: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "inverse") {
			ReadToken();
			isSet.inverse = true;
			inverse = ReadBool();
		} else if (Token().strVal == "rect") {
			ReadToken();
			isSet.rect = true;
			ReadRect(rect);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.inverse);
	Assume(isSet.rect);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ClipToRect(rect, inverse);
}

void PictureReaderJson::ReadClipToShape(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	bool inverse;
	BShape shape;
	struct {
		bool inverse: 1;
		bool shape: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "inverse") {
			ReadToken();
			isSet.inverse = true;
			inverse = ReadBool();
		} else if (Token().strVal == "shape") {
			ReadToken();
			isSet.shape = true;
			ReadShape(shape);
		} else {
			RaiseError();
		}
	}
	Assume(isSet.inverse);
	Assume(isSet.shape);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ClipToShape(shape, inverse);
}

void PictureReaderJson::ReadSetOrigin(PictureVisitor &vis)
{
	BPoint pt;
	ReadPoint(pt);
	vis.SetOrigin(pt);
}

void PictureReaderJson::ReadSetPenLocation(PictureVisitor &vis)
{
	BPoint pt;
	ReadPoint(pt);
	vis.SetPenLocation(pt);
}

void PictureReaderJson::ReadSetDrawingMode(PictureVisitor &vis)
{
	drawing_mode mode;
	AssumeToken(JsonTokenKind::String);
	if (Token().strVal == "B_OP_COPY") {
		mode = B_OP_COPY;
	} else if (Token().strVal == "B_OP_OVER") {
		mode = B_OP_OVER;
	} else if (Token().strVal == "B_OP_ERASE") {
		mode = B_OP_ERASE;
	} else if (Token().strVal == "B_OP_INVERT") {
		mode = B_OP_INVERT;
	} else if (Token().strVal == "B_OP_ADD") {
		mode = B_OP_ADD;
	} else if (Token().strVal == "B_OP_SUBTRACT") {
		mode = B_OP_SUBTRACT;
	} else if (Token().strVal == "B_OP_BLEND") {
		mode = B_OP_BLEND;
	} else if (Token().strVal == "B_OP_MIN") {
		mode = B_OP_MIN;
	} else if (Token().strVal == "B_OP_MAX") {
		mode = B_OP_MAX;
	} else if (Token().strVal == "B_OP_SELECT") {
		mode = B_OP_SELECT;
	} else if (Token().strVal == "B_OP_ALPHA") {
		mode = B_OP_ALPHA;
	} else {
		RaiseError();
	}
	ReadToken();
	vis.SetDrawingMode(mode);
}

void PictureReaderJson::ReadSetLineMode(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	cap_mode capMode;
	join_mode joinMode;
	float miterLimit;
	struct {
		bool capMode: 1;
		bool joinMode: 1;
		bool miterLimit: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "capMode") {
			ReadToken();
			isSet.capMode = true;
			AssumeToken(JsonTokenKind::String);
			if (Token().strVal == "B_ROUND_CAP") {
				capMode = B_ROUND_CAP;
			} else if (Token().strVal == "B_BUTT_CAP") {
				capMode = B_BUTT_CAP;
			} else if (Token().strVal == "B_SQUARE_CAP") {
				capMode = B_SQUARE_CAP;
			} else {
				RaiseError();
			}
			ReadToken();
		} else if (Token().strVal == "joinMode") {
			ReadToken();
			isSet.joinMode = true;
			AssumeToken(JsonTokenKind::String);
			if (Token().strVal == "B_ROUND_JOIN") {
				joinMode = B_ROUND_JOIN;
			} else if (Token().strVal == "B_MITER_JOIN") {
				joinMode = B_MITER_JOIN;
			} else if (Token().strVal == "B_BEVEL_JOIN") {
				joinMode = B_BEVEL_JOIN;
			} else if (Token().strVal == "B_BUTT_JOIN") {
				joinMode = B_BUTT_JOIN;
			} else if (Token().strVal == "B_SQUARE_JOIN") {
				joinMode = B_SQUARE_JOIN;
			} else {
				RaiseError();
			}
			ReadToken();
		} else if (Token().strVal == "miterLimit") {
			ReadToken();
			isSet.miterLimit = true;
			miterLimit = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.capMode);
	Assume(isSet.joinMode);
	Assume(isSet.miterLimit);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.SetLineMode(capMode, joinMode, miterLimit);
}

void PictureReaderJson::ReadSetPenSize(PictureVisitor &vis)
{
	float val = ReadReal();
	vis.SetPenSize(val);
}

void PictureReaderJson::ReadSetScale(PictureVisitor &vis)
{
	float val = ReadReal();
	vis.SetScale(val);
}

void PictureReaderJson::ReadSetHighColor(PictureVisitor &vis)
{
	rgb_color color;
	ReadColor(color);
	vis.SetHighColor(color);
}

void PictureReaderJson::ReadSetLowColor(PictureVisitor &vis)
{
	rgb_color color;
	ReadColor(color);
	vis.SetLowColor(color);
}

void PictureReaderJson::ReadSetPattern(PictureVisitor &vis)
{
	::pattern pat;
	if (Token().kind == JsonTokenKind::String) {
		if (Token().strVal == "B_SOLID_HIGH") {
			ReadToken();
			pat = B_SOLID_HIGH;
		} else if (Token().strVal == "B_SOLID_LOW") {
			ReadToken();
			pat = B_SOLID_LOW;
		} else if (Token().strVal == "B_MIXED_COLORS") {
			ReadToken();
			pat = B_MIXED_COLORS;
		} else {
			RaiseError();
		}
	} else if (Token().kind == JsonTokenKind::StartArray) {
		ReadToken();
		for (int32 i = 0; i < 8; i++) {
			pat.data[i] = ReadUint8();
		}
		AssumeToken(JsonTokenKind::EndArray); ReadToken();
	} else {
		RaiseError();
	}
	vis.SetPattern(pat);
}

void PictureReaderJson::ReadEnterFontState(PictureVisitor &vis)
{
	vis.EnterFontState();
	ReadOps(vis);
	vis.ExitFontState();
}

void PictureReaderJson::ReadSetBlendingMode(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	source_alpha srcAlpha;
	alpha_function alphaFunc;
	struct {
		bool srcAlpha: 1;
		bool alphaFunc: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "srcAlpha") {
			ReadToken();
			isSet.srcAlpha = true;
			AssumeToken(JsonTokenKind::String);
			if (Token().strVal == "B_PIXEL_ALPHA") {
				srcAlpha = B_PIXEL_ALPHA;
			} else if (Token().strVal == "B_CONSTANT_ALPHA") {
				srcAlpha = B_CONSTANT_ALPHA;
			} else {
				RaiseError();
			}
			ReadToken();
		} else if (Token().strVal == "alphaFunc") {
			ReadToken();
			isSet.alphaFunc = true;
			AssumeToken(JsonTokenKind::String);
			if (Token().strVal == "B_ALPHA_OVERLAY") {
				alphaFunc = B_ALPHA_OVERLAY;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE") {
				alphaFunc = B_ALPHA_COMPOSITE;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_SOURCE_IN") {
				alphaFunc = B_ALPHA_COMPOSITE_SOURCE_IN;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_SOURCE_OUT") {
				alphaFunc = B_ALPHA_COMPOSITE_SOURCE_OUT;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_SOURCE_ATOP") {
				alphaFunc = B_ALPHA_COMPOSITE_SOURCE_ATOP;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_DESTINATION_OVER") {
				alphaFunc = B_ALPHA_COMPOSITE_DESTINATION_OVER;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_DESTINATION_IN") {
				alphaFunc = B_ALPHA_COMPOSITE_DESTINATION_IN;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_DESTINATION_OUT") {
				alphaFunc = B_ALPHA_COMPOSITE_DESTINATION_OUT;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_DESTINATION_ATOP") {
				alphaFunc = B_ALPHA_COMPOSITE_DESTINATION_ATOP;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_XOR") {
				alphaFunc = B_ALPHA_COMPOSITE_XOR;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_CLEAR") {
				alphaFunc = B_ALPHA_COMPOSITE_CLEAR;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_DIFFERENCE") {
				alphaFunc = B_ALPHA_COMPOSITE_DIFFERENCE;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_LIGHTEN") {
				alphaFunc = B_ALPHA_COMPOSITE_LIGHTEN;
			} else if (Token().strVal == "B_ALPHA_COMPOSITE_DARKEN") {
				alphaFunc = B_ALPHA_COMPOSITE_DARKEN;
			} else {
				RaiseError();
			}
			ReadToken();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.srcAlpha);
	Assume(isSet.alphaFunc);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.SetBlendingMode(srcAlpha, alphaFunc);
}

void PictureReaderJson::ReadSetFillRule(PictureVisitor &vis)
{
	int32 fillRule;
	AssumeToken(JsonTokenKind::String);
	if (Token().strVal == "B_EVEN_ODD") {
		fillRule = B_EVEN_ODD;
	} else if (Token().strVal == "B_NONZERO") {
		fillRule = B_NONZERO;
	} else {
		RaiseError();
	}
	ReadToken();
	vis.SetFillRule(fillRule);
}

void PictureReaderJson::ReadSetFontFamily(PictureVisitor &vis)
{
	font_family family;
	AssumeToken(JsonTokenKind::String);
	Assume(Token().strVal.size() <= sizeof(family) - 1);
	strcpy(family, Token().strVal.c_str());
	ReadToken();
	vis.SetFontFamily(family);
}

void PictureReaderJson::ReadSetFontStyle(PictureVisitor &vis)
{
	font_style style;
	AssumeToken(JsonTokenKind::String);
	Assume(Token().strVal.size() <= sizeof(style) - 1);
	strcpy(style, Token().strVal.c_str());
	ReadToken();
	vis.SetFontStyle(style);
}

void PictureReaderJson::ReadSetFontSpacing(PictureVisitor &vis)
{
	int32 spacing;
	AssumeToken(JsonTokenKind::String);
	if (Token().strVal == "B_CHAR_SPACING") {
		spacing = B_CHAR_SPACING;
	} else if (Token().strVal == "B_STRING_SPACING") {
		spacing = B_STRING_SPACING;
	} else if (Token().strVal == "B_BITMAP_SPACING") {
		spacing = B_BITMAP_SPACING;
	} else if (Token().strVal == "B_FIXED_SPACING") {
		spacing = B_FIXED_SPACING;
	} else {
		RaiseError();
	}
	ReadToken();
	vis.SetFontSpacing(spacing);
}

void PictureReaderJson::ReadSetFontEncoding(PictureVisitor &vis)
{
	int32 encoding;
	AssumeToken(JsonTokenKind::String);
	if (Token().strVal == "B_UNICODE_UTF8") {
		encoding = B_UNICODE_UTF8;
	} else if (Token().strVal == "B_ISO_8859_1") {
		encoding = B_ISO_8859_1;
	} else if (Token().strVal == "B_ISO_8859_2") {
		encoding = B_ISO_8859_2;
	} else if (Token().strVal == "B_ISO_8859_3") {
		encoding = B_ISO_8859_3;
	} else if (Token().strVal == "B_ISO_8859_4") {
		encoding = B_ISO_8859_4;
	} else if (Token().strVal == "B_ISO_8859_5") {
		encoding = B_ISO_8859_5;
	} else if (Token().strVal == "B_ISO_8859_6") {
		encoding = B_ISO_8859_6;
	} else if (Token().strVal == "B_ISO_8859_7") {
		encoding = B_ISO_8859_7;
	} else if (Token().strVal == "B_ISO_8859_8") {
		encoding = B_ISO_8859_8;
	} else if (Token().strVal == "B_ISO_8859_9") {
		encoding = B_ISO_8859_9;
	} else if (Token().strVal == "B_ISO_8859_10") {
		encoding = B_ISO_8859_10;
	} else if (Token().strVal == "B_MACINTOSH_ROMAN") {
		encoding = B_MACINTOSH_ROMAN;
	} else {
		RaiseError();
	}
	ReadToken();
	vis.SetFontEncoding(encoding);
}

void PictureReaderJson::ReadSetFontFlags(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	int32 flags = 0;
	while (Token().kind != JsonTokenKind::EndArray) {
		if (Token().strVal == "B_DISABLE_ANTIALIASING") {
			flags |= B_DISABLE_ANTIALIASING;
		} else if (Token().strVal == "B_FORCE_ANTIALIASING") {
			flags |= B_FORCE_ANTIALIASING;
		} else {
			RaiseError();
		}
		ReadToken();
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.SetFontFlags(flags);
}

void PictureReaderJson::ReadSetFontSize(PictureVisitor &vis)
{
	float size = ReadReal();
	vis.SetFontSize(size);
}

void PictureReaderJson::ReadSetFontRotation(PictureVisitor &vis)
{
	float rotation = ReadReal();
	vis.SetFontRotation(rotation);
}

void PictureReaderJson::ReadSetFontShear(PictureVisitor &vis)
{
	float shear = ReadReal();
	vis.SetFontShear(shear);
}

void PictureReaderJson::ReadSetFontBpp(PictureVisitor &vis)
{
	int32 bpp = ReadInt32();
	vis.SetFontBpp(bpp);
}

void PictureReaderJson::ReadSetFontFace(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartArray); ReadToken();
	int32 face = 0;
	while (Token().kind != JsonTokenKind::EndArray) {
		if (Token().strVal == "B_ITALIC_FACE") {
			face |= B_ITALIC_FACE;
		} else if (Token().strVal == "B_UNDERSCORE_FACE") {
			face |= B_UNDERSCORE_FACE;
		} else if (Token().strVal == "B_NEGATIVE_FACE") {
			face |= B_NEGATIVE_FACE;
		} else if (Token().strVal == "B_OUTLINED_FACE") {
			face |= B_OUTLINED_FACE;
		} else if (Token().strVal == "B_STRIKEOUT_FACE") {
			face |= B_STRIKEOUT_FACE;
		} else if (Token().strVal == "B_BOLD_FACE") {
			face |= B_BOLD_FACE;
		} else if (Token().strVal == "B_REGULAR_FACE") {
			face |= B_REGULAR_FACE;
		} else if (Token().strVal == "B_CONDENSED_FACE") {
			face |= B_CONDENSED_FACE;
		} else if (Token().strVal == "B_LIGHT_FACE") {
			face |= B_LIGHT_FACE;
		} else if (Token().strVal == "B_HEAVY_FACE") {
			face |= B_HEAVY_FACE;
		} else {
			RaiseError();
		}
		ReadToken();
	}
	AssumeToken(JsonTokenKind::EndArray); ReadToken();
	vis.SetFontFace(face);
}

void PictureReaderJson::ReadSetFontFalseBoldWidth(PictureVisitor &vis)
{
	float width = ReadReal();
	vis.SetFontFalseBoldWidth(width);
}

void PictureReaderJson::ReadSetTransform(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	BAffineTransform tr;
	struct {
		bool tx: 1;
		bool ty: 1;
		bool sx: 1;
		bool sy: 1;
		bool shy: 1;
		bool shx: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "tx") {
			ReadToken();
			isSet.tx = true;
			tr.tx = ReadReal();
		} else if (Token().strVal == "ty") {
			ReadToken();
			isSet.ty = true;
			tr.ty = ReadReal();
		} else if (Token().strVal == "sx") {
			ReadToken();
			isSet.sx = true;
			tr.sx = ReadReal();
		} else if (Token().strVal == "sy") {
			ReadToken();
			isSet.sy = true;
			tr.sy = ReadReal();
		} else if (Token().strVal == "shy") {
			ReadToken();
			isSet.shy = true;
			tr.shy = ReadReal();
		} else if (Token().strVal == "shx") {
			ReadToken();
			isSet.shx = true;
			tr.shx = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.tx);
	Assume(isSet.ty);
	Assume(isSet.sx);
	Assume(isSet.sy);
	Assume(isSet.shy);
	Assume(isSet.shx);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.SetTransform(tr);
}

void PictureReaderJson::ReadTranslateBy(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	double x;
	double y;
	struct {
		bool x: 1;
		bool y: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "x") {
			ReadToken();
			isSet.x = true;
			x = ReadReal();
		} else if (Token().strVal == "y") {
			ReadToken();
			isSet.y = true;
			y = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.x);
	Assume(isSet.y);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.TranslateBy(x, y);
}

void PictureReaderJson::ReadScaleBy(PictureVisitor &vis)
{
	AssumeToken(JsonTokenKind::StartObject); ReadToken();
	double x;
	double y;
	struct {
		bool x: 1;
		bool y: 1;
	} isSet {};
	while (Token().kind == JsonTokenKind::Key) {
		if (Token().strVal == "x") {
			ReadToken();
			isSet.x = true;
			x = ReadReal();
		} else if (Token().strVal == "y") {
			ReadToken();
			isSet.y = true;
			y = ReadReal();
		} else {
			RaiseError();
		}
	}
	Assume(isSet.x);
	Assume(isSet.y);
	AssumeToken(JsonTokenKind::EndObject); ReadToken();
	vis.ScaleBy(x, y);
}

void PictureReaderJson::ReadRotateBy(PictureVisitor &vis)
{
	double rotation = ReadReal();
	vis.RotateBy(rotation);
}

void PictureReaderJson::ReadBlendLayer(PictureVisitor &vis)
{
	RaiseUnimplemented();
}
