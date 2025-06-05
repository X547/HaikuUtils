#include "PictureVisitorJson.h"

#include <vector>

#include <View.h>
#include <Shape.h>
#include <Gradient.h>
#include <Picture.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <File.h>
#include <private/interface/ShapePrivate.h>
#include <stdio.h>
#include <math.h>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>


#include <private/shared/AutoDeleter.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using JsonWriter = rapidjson::Writer<rapidjson::OStreamWrapper>;

void ReadBool(BDataIO &rd, bool &val) {int8 x; rd.Read(&x, sizeof(x)); val = x != 0;}
void Read8(BDataIO &rd, int8 &val) {rd.Read(&val, sizeof(val));}
void Read16(BDataIO &rd, int16 &val) {rd.Read(&val, sizeof(val));}
void Read32(BDataIO &rd, int32 &val) {rd.Read(&val, sizeof(val));}
void Read64(BDataIO &rd, int64 &val) {rd.Read(&val, sizeof(val));}
void ReadFloat(BDataIO &rd, float &val) {rd.Read(&val, sizeof(val));}
void ReadDouble(BDataIO &rd, double &val) {rd.Read(&val, sizeof(val));}
void ReadPoint(BDataIO &rd, BPoint &val) {rd.Read(&val, sizeof(val));}
void ReadRect(BDataIO &rd, BRect &val) {rd.Read(&val, sizeof(val));}
void ReadTransform(BDataIO &rd, BAffineTransform &val) {rd.Read(&val, sizeof(val));}
void ReadPattern(BDataIO &rd, pattern &val) {rd.Read(&val, sizeof(val));}

void ReadColor(BDataIO &rd, rgb_color& color)
{
	int8 val;
	Read8(rd, val); color.red = (uint8)val;
	Read8(rd, val); color.green = (uint8)val;
	Read8(rd, val); color.blue = (uint8)val;
	Read8(rd, val); color.alpha = (uint8)val;
}

void ReadString(BDataIO &rd, BString &val)
{
	int32 len;
	Read32(rd, len);
	val = "";
	for (; len > 0; len--) {
		char ch;
		rd.Read(&ch, sizeof(ch));
		val += ch;
	}
}

void ReadStringLen(BDataIO &rd, BString &val, size_t len)
{
	val = "";
	for (; len > 0; len--) {
		char ch;
		rd.Read(&ch, sizeof(ch));
		val += ch;
	}
}

void ReadShape(BDataIO &rd, BShape &shape)
{
	int32 opCount;
	int32 pointCount;
	ArrayDeleter<uint32> opList;
	ArrayDeleter<BPoint> pointList;

	Read32(rd, opCount);
	Read32(rd, pointCount);
	opList.SetTo(new uint32[opCount]);
	pointList.SetTo(new BPoint[pointCount]);
	rd.Read(opList.Get(), opCount*sizeof(uint32));
	rd.Read(pointList.Get(), pointCount*sizeof(BPoint));

	BShape::Private(shape).SetData(opCount, pointCount, &opList[0], &pointList[0]);
}

void ReadGradientStops(BDataIO &rd, BGradient &gradient)
{
	int32 stopCount;
	Read32(rd, stopCount);
	for (int32 i = 0; i < stopCount; i++) {
		BGradient::ColorStop cs;
		ReadColor(rd, cs.color);
		ReadFloat(rd, cs.offset);
		gradient.AddColorStop(cs, i);
	}
}

void ReadGradient(BDataIO &rd, ObjectDeleter<BGradient> &outGradient)
{
	int32 type;
	Read32(rd, type);
	switch (type) {
	case BGradient::TYPE_LINEAR: {
		ObjectDeleter<BGradientLinear> gradient(new BGradientLinear());
		ReadGradientStops(rd, *gradient.Get());
		BPoint start;
		BPoint end;
		ReadPoint(rd, start);
		ReadPoint(rd, end);
		gradient->SetStart(start);
		gradient->SetEnd(end);
		outGradient.SetTo(gradient.Detach());
		break;
	}
	case BGradient::TYPE_RADIAL: {
		ObjectDeleter<BGradientRadial> gradient(new BGradientRadial());
		ReadGradientStops(rd, *gradient.Get());
		BPoint center;
		float radius;
		ReadPoint(rd, center);
		ReadFloat(rd, radius);
		gradient->SetCenter(center);
		gradient->SetRadius(radius);
		outGradient.SetTo(gradient.Detach());
		break;
	}
	case BGradient::TYPE_RADIAL_FOCUS: {
		ObjectDeleter<BGradientRadialFocus> gradient(new BGradientRadialFocus());
		ReadGradientStops(rd, *gradient.Get());
		BPoint center;
		BPoint focal;
		float radius;
		ReadPoint(rd, center);
		ReadPoint(rd, focal);
		ReadFloat(rd, radius);
		gradient->SetCenter(center);
		gradient->SetFocal(focal);
		gradient->SetRadius(radius);
		outGradient.SetTo(gradient.Detach());
		break;
	}
	case BGradient::TYPE_DIAMOND: {
		ObjectDeleter<BGradientDiamond> gradient(new BGradientDiamond());
		ReadGradientStops(rd, *gradient.Get());
		BPoint center;
		ReadPoint(rd, center);
		gradient->SetCenter(center);
		outGradient.SetTo(gradient.Detach());
		break;
	}
	case BGradient::TYPE_CONIC: {
		ObjectDeleter<BGradientConic> gradient(new BGradientConic());
		ReadGradientStops(rd, *gradient.Get());
		BPoint center;
		float angle;
		ReadPoint(rd, center);
		ReadFloat(rd, angle);
		gradient->SetCenter(center);
		gradient->SetAngle(angle);
		outGradient.SetTo(gradient.Detach());
		break;
	}
	case BGradient::TYPE_NONE:
	default: {
		throw std::system_error(B_BAD_VALUE, std::generic_category());
	}
	}
}

void DumpOps(PictureVisitor &vis, BPositionIO &rd, int32 size);

void DumpOp(PictureVisitor &vis, BPositionIO &rd, int16 op, int32 opSize)
{
	switch (op) {
	case 0x0010: {
		float dx, dy;
		ReadFloat(rd, dx);
		ReadFloat(rd, dy);
		vis.MovePenBy(dx, dy);
		break;
	}
	case 0x0100: {
		BPoint start, end;
		ReadPoint(rd, start);
		ReadPoint(rd, end);
		vis.StrokeLine(start, end);
		break;
	}
	case 0x0101: {
		BRect rect;
		ReadRect(rd, rect);
		vis.StrokeRect(rect);
		break;
	}
	case 0x0102: {
		BRect rect;
		ReadRect(rd, rect);
		vis.FillRect(rect);
		break;
	}
	case 0x0103: {
		BRect rect;
		BPoint radius;
		ReadRect(rd, rect);
		ReadPoint(rd, radius);
		vis.StrokeRoundRect(rect, radius);
		break;
	}
	case 0x0104: {
		BRect rect;
		BPoint radius;
		ReadRect(rd, rect);
		ReadPoint(rd, radius);
		vis.FillRoundRect(rect, radius);
		break;
	}
	case 0x0105: {
		BPoint points[4];
		for (int32 i = 0; i < 4; i++) {
			ReadPoint(rd, points[i]);
		}
		vis.StrokeBezier(points);
		break;
	}
	case 0x0106: {
		BPoint points[4];
		for (int32 i = 0; i < 4; i++) {
			ReadPoint(rd, points[i]);
		}
		vis.FillBezier(points);
		break;
	}
	case 0x010B: {
		std::vector<BPoint> points;
		int32 numPoints;
		bool isClosed;
		Read32(rd, numPoints);
		for (int32 i = 0; i < numPoints; i++) {
			BPoint point;
			ReadPoint(rd, point);
			points.push_back(point);
		}
		ReadBool(rd, isClosed);
		vis.StrokePolygon(numPoints, points.data(), isClosed);
		break;
	}
	case 0x010C: {
		std::vector<BPoint> points;
		int32 numPoints;
		Read32(rd, numPoints);
		for (int32 i = 0; i < numPoints; i++) {
			BPoint point;
			ReadPoint(rd, point);
			points.push_back(point);
		}
		vis.FillPolygon(numPoints, points.data());
		break;
	}
	case 0x010D: {
		BShape shape;
		ReadShape(rd, shape);
		vis.StrokeShape(shape);
		break;
	}
	case 0x010E: {
		BShape shape;
		ReadShape(rd, shape);
		vis.FillShape(shape);
		break;
	}
	case 0x010F: {
		BString string;
		escapement_delta delta;
		ReadString(rd, string);
		ReadFloat(rd, delta.space);
		ReadFloat(rd, delta.nonspace);
		vis.DrawString(string.String(), string.Length(), delta);
		break;
	}
	case 0x0110: {
		BRect srcRect;
		BRect dstRect;
		int32 width;
		int32 height;
		int32 bytesPerRow;
		int32 colorSpace;
		int32 flags;
		ArrayDeleter<uint8> data;

		off_t pos = rd.Position();
		ReadRect(rd, srcRect);
		ReadRect(rd, dstRect);
		Read32(rd, width);
		Read32(rd, height);
		Read32(rd, bytesPerRow);
		Read32(rd, colorSpace);
		Read32(rd, flags);
		size_t size = opSize - (rd.Position() - pos);
		data.SetTo(new uint8[size]);
		rd.Read(data.Get(), size*sizeof(uint8));

		vis.DrawBitmap(srcRect, dstRect, width, height, bytesPerRow, colorSpace, flags, &data[0], size);
		break;
	}
	case 0x0112: {
		BPoint where;
		int32 token;
		ReadPoint(rd, where);
		Read32(rd, token);
		vis.DrawPicture(where, token);
		break;
	}
	case 0x0113: {
		BPoint center;
		BPoint radius;
		float startTheta;
		float arcTheta;
		ReadPoint(rd, center);
		ReadPoint(rd, radius);
		ReadFloat(rd, startTheta);
		ReadFloat(rd, arcTheta);
		vis.StrokeArc(center, radius, startTheta, arcTheta);
		break;
	}
	case 0x0114: {
		BPoint center;
		BPoint radius;
		float startTheta;
		float arcTheta;
		ReadPoint(rd, center);
		ReadPoint(rd, radius);
		ReadFloat(rd, startTheta);
		ReadFloat(rd, arcTheta);
		vis.FillArc(center, radius, startTheta, arcTheta);
		break;
	}
	case 0x0115: {
		BRect rect;
		ReadRect(rd, rect);
		vis.StrokeEllipse(rect);
		break;
	}
	case 0x0116: {
		BRect rect;
		ReadRect(rd, rect);
		vis.FillEllipse(rect);
		break;
	}
	case 0x0117: {
		BString string;
		int32 pointCount;
		ArrayDeleter<BPoint> locations;

		Read32(rd, pointCount);
		locations.SetTo(new BPoint[pointCount]);
		rd.Read(locations.Get(), pointCount*sizeof(BPoint));
		ReadString(rd, string);

		vis.DrawString(string.String(), string.Length(), &locations[0], pointCount);
		break;
	}

	case 0x0118: {
		BRect rect;
		ObjectDeleter<BGradient> gradient;

		ReadRect(rd, rect);
		ReadGradient(rd, gradient);

		vis.StrokeRect(rect, *gradient.Get());
		break;
	}
	case 0x0119: {
		BRect rect;
		ObjectDeleter<BGradient> gradient;

		ReadRect(rd, rect);
		ReadGradient(rd, gradient);

		vis.FillRect(rect, *gradient.Get());
		break;
	}
	case 0x011A: {
		BRect rect;
		BPoint radius;
		ObjectDeleter<BGradient> gradient;

		ReadRect(rd, rect);
		ReadPoint(rd, radius);
		ReadGradient(rd, gradient);

		vis.StrokeRoundRect(rect, radius, *gradient.Get());
		break;
	}
	case 0x011B: {
		BRect rect;
		BPoint radius;
		ObjectDeleter<BGradient> gradient;

		ReadRect(rd, rect);
		ReadPoint(rd, radius);
		ReadGradient(rd, gradient);

		vis.FillRoundRect(rect, radius, *gradient.Get());
		break;
	}
	case 0x011C: {
		BPoint points[4];
		ObjectDeleter<BGradient> gradient;

		rd.Read(points, 4*sizeof(BPoint));
		ReadGradient(rd, gradient);

		vis.StrokeBezier(points, *gradient.Get());
		break;
	}
	case 0x011D: {
		BPoint points[4];
		ObjectDeleter<BGradient> gradient;

		rd.Read(points, 4*sizeof(BPoint));
		ReadGradient(rd, gradient);

		vis.FillBezier(points, *gradient.Get());
		break;
	}
	case 0x011E: {
		int32 numPoints;
		ArrayDeleter<BPoint> points;
		bool isClosed;
		ObjectDeleter<BGradient> gradient;

		Read32(rd, numPoints);
		points.SetTo(new BPoint[numPoints]);
		rd.Read(points.Get(), numPoints*sizeof(BPoint));
		ReadBool(rd, isClosed);
		ReadGradient(rd, gradient);

		vis.StrokePolygon(numPoints, &points[0], isClosed, *gradient.Get());
		break;
	}
	case 0x011F: {
		int32 numPoints;
		ArrayDeleter<BPoint> points;
		ObjectDeleter<BGradient> gradient;

		Read32(rd, numPoints);
		points.SetTo(new BPoint[numPoints]);
		rd.Read(points.Get(), numPoints*sizeof(BPoint));
		ReadGradient(rd, gradient);

		vis.FillPolygon(numPoints, &points[0], *gradient.Get());
		break;
	}
	case 0x0120: {
		BShape shape;
		ObjectDeleter<BGradient> gradient;

		ReadShape(rd, shape);
		ReadGradient(rd, gradient);

		vis.StrokeShape(shape, *gradient.Get());
		break;
	}
	case 0x0121: {
		BShape shape;
		ObjectDeleter<BGradient> gradient;

		ReadShape(rd, shape);
		ReadGradient(rd, gradient);

		vis.FillShape(shape, *gradient.Get());
		break;
	}
	case 0x0122: {
		BPoint center;
		BPoint radius;
		float startTheta;
		float arcTheta;
		ObjectDeleter<BGradient> gradient;
		ReadPoint(rd, center);
		ReadPoint(rd, radius);
		ReadFloat(rd, startTheta);
		ReadFloat(rd, arcTheta);
		ReadGradient(rd, gradient);
		vis.StrokeArc(center, radius, startTheta, arcTheta, *gradient.Get());
		break;
	}
	case 0x0123: {
		BPoint center;
		BPoint radius;
		float startTheta;
		float arcTheta;
		ObjectDeleter<BGradient> gradient;
		ReadPoint(rd, center);
		ReadPoint(rd, radius);
		ReadFloat(rd, startTheta);
		ReadFloat(rd, arcTheta);
		ReadGradient(rd, gradient);
		vis.FillArc(center, radius, startTheta, arcTheta, *gradient.Get());
		break;
	}
	case 0x0124: {
		BRect rect;
		ObjectDeleter<BGradient> gradient;
		ReadRect(rd, rect);
		ReadGradient(rd, gradient);
		vis.StrokeEllipse(rect, *gradient.Get());
		break;
	}
	case 0x0125: {
		BRect rect;
		ObjectDeleter<BGradient> gradient;
		ReadRect(rd, rect);
		ReadGradient(rd, gradient);
		vis.FillEllipse(rect, *gradient.Get());
		break;
	}

	case 0x0200: {
		vis.EnterStateChange();
		DumpOps(vis, rd, opSize);
		vis.ExitStateChange();
		break;
	}
	case 0x0201: {
		BRegion region;
		int32 numRects;
		Read32(rd, numRects);
		for (int32 i = 0; i < numRects; i++) {
			BRect rc;
			ReadRect(rd, rc);
			region.Include(rc);
		}
		vis.SetClipping(region);
		break;
	}
	case 0x0202: {
		int32 token;
		BPoint where;
		bool inverse;
		Read32(rd, token);
		ReadPoint(rd, where);
		ReadBool(rd, inverse);
		vis.ClipToPicture(token, where, inverse);
		break;
	}
	case 0x0203:
		vis.PushState();
		break;
	case 0x0204:
		vis.PopState();
		break;
	case 0x0205:
		vis.ClearClipping();
		break;
	case 0x0206: {
		bool inverse;
		BRect rect;
		ReadBool(rd, inverse);
		ReadRect(rd, rect);
		vis.ClipToRect(rect, inverse);
		break;
	}
	case 0x0207: {
		bool inverse;
		BShape shape;
		ReadBool(rd, inverse);
		ReadShape(rd, shape);
		vis.ClipToShape(shape, inverse);
		break;
	}

	case 0x0300: {
		BPoint point;
		ReadPoint(rd, point);
		vis.SetOrigin(point);
		break;
	}
	case 0x0301: {
		BPoint point;
		ReadPoint(rd, point);
		vis.SetPenLocation(point);
		break;
	}
	case 0x0302: {
		int16 mode;
		Read16(rd, mode);
		vis.SetDrawingMode((drawing_mode)mode);
		break;
	}
	case 0x0303: {
		int16 capMode;
		int16 joinMode;
		float miterLimit;

		Read16(rd, capMode);
		Read16(rd, joinMode);
		ReadFloat(rd, miterLimit);

		vis.SetLineMode((cap_mode)capMode, (join_mode)joinMode, miterLimit);
		break;
	}
	case 0x0304: {
		float penSize;
		ReadFloat(rd, penSize);
		vis.SetPenSize(penSize);
		break;
	}
	case 0x0305: {
		float scale;
		ReadFloat(rd, scale);
		vis.SetScale(scale);
		break;
	}
	case 0x0306: {
		rgb_color color;
		ReadColor(rd, color);
		vis.SetHighColor(color);
		break;
	}
	case 0x0307: {
		rgb_color color;
		ReadColor(rd, color);
		vis.SetLowColor(color);
		break;
	}
	case 0x0308: {
		pattern pat;
		ReadPattern(rd, pat);
		vis.SetPattern(pat);
		break;
	}
	case 0x0309: {
		vis.EnterFontState();
		DumpOps(vis, rd, opSize);
		vis.ExitFontState();
		break;
	}
	case 0x030A: {
		int16 srcAlpha;
		int16 alphaFunc;

		Read16(rd, srcAlpha);
		Read16(rd, alphaFunc);

		vis.SetBlendingMode((source_alpha)srcAlpha, (alpha_function)alphaFunc);
		break;
	}
	case 0x030B: {
		int32 fillRule;
		Read32(rd, fillRule);
		vis.SetFillRule(fillRule);
		break;
	}

	case 0x0380: {
		BString str;
		ReadString(rd, str);
		font_family family;
		size_t len = std::min<size_t>(str.Length(), sizeof(family) - 1);
		memcpy(family, str.String(), len);
		family[len] = '\0';
		vis.SetFontFamily(family);
		break;
	}
	case 0x0381: {
		BString str;
		ReadString(rd, str);
		font_style style;
		size_t len = std::min<size_t>(str.Length(), sizeof(style) - 1);
		memcpy(style, str.String(), len);
		style[len] = '\0';
		vis.SetFontStyle(style);
		break;
	}
	case 0x0382: {
		int32 spacing;
		Read32(rd, spacing);
		vis.SetFontSpacing(spacing);
		break;
	}
	case 0x0383: {
		int32 encoding;
		Read32(rd, encoding);
		vis.SetFontEncoding(encoding);
		break;
	}
	case 0x0384: {
		int32 flags;
		Read32(rd, flags);
		vis.SetFontFlags(flags);
		break;
	}
	case 0x0385: {
		float size;
		ReadFloat(rd, size);
		vis.SetFontSize(size);
		break;
	}
	case 0x0386: {
		float rotation;
		ReadFloat(rd, rotation);
		vis.SetFontRotation(rotation);
		break;
	}
	case 0x0387: {
		float shear;
		ReadFloat(rd, shear);
		vis.SetFontShear(shear);
		break;
	}
	case 0x0388: {
		int32 bpp;
		Read32(rd, bpp);
		vis.SetFontBpp(bpp);
		break;
	}
	case 0x0389: {
		int32 face;
		Read32(rd, face);
		vis.SetFontFace(face);
		break;
	}
	case 0x0390: {
		BAffineTransform transform;
		ReadTransform(rd, transform);
		vis.SetTransform(transform);
		break;
	}
	case 0x0391: {
		double x, y;
		ReadDouble(rd, x);
		ReadDouble(rd, y);
		vis.TranslateBy(x, y);
		break;
	}
	case 0x0392: {
		double x, y;
		ReadDouble(rd, x);
		ReadDouble(rd, y);
		vis.ScaleBy(x, y);
		break;
	}
	case 0x0393: {
		double angleRadians;
		ReadDouble(rd, angleRadians);
		vis.RotateBy(angleRadians);
		break;
	}
	case 0x0394: {
		// TODO
		vis.BlendLayer(NULL);
		break;
	}
	default:
		break;
	}
}

void DumpOps(PictureVisitor &vis, BPositionIO &rd, int32 size)
{
	off_t beg = rd.Position();
	while (rd.Position() - beg < size) {
		int16 op;
		int32 opSize;
		Read16(rd, op);
		Read32(rd, opSize);
		off_t pos = rd.Position();
		DumpOp(vis, rd, op, opSize);
		rd.Seek(pos + opSize, SEEK_SET);
	}
}

void DumpPictureFile(JsonWriter &wr, BPositionIO &rd)
{
	PictureVisitorJson vis(wr);

	wr.StartObject();
	int32 version;
	int32 unknown;
	int32 count;
	int32 size;
	Read32(rd, version); wr.Key("version"); wr.Int(version);
	Read32(rd, unknown); wr.Key("unknown"); wr.Int(unknown);
	Read32(rd, count);
	if (count > 0) {
	wr.Key("pictures");
		wr.StartArray();
		for (int32 i = 0; i < count; i++) {
			DumpPictureFile(wr, rd);
		}
		wr.EndArray();
	}
	Read32(rd, size);
	wr.Key("ops");
	wr.StartArray();
	DumpOps(vis, rd, size);
	wr.EndArray();
	wr.EndObject();
}


int main(int argCnt, char **args)
{
	if (argCnt < 2)
		return 1;

	rapidjson::OStreamWrapper os(std::cout);
	JsonWriter wr(os);
	PictureVisitorJson vis(wr);

	BFile file(args[1], B_READ_ONLY);
	DumpPictureFile(wr, file);
	return 0;
}
