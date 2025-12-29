#include "PictureReaderBinary.h"

#include <vector>
#include <system_error>

#include "PictureOpcodes.h"
#include "PictureVisitor.h"

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>

#include <private/interface/ShapePrivate.h>
#include <private/shared/AutoDeleter.h>


static void ReadBool(BDataIO &rd, bool &val) {int8 x; rd.Read(&x, sizeof(x)); val = x != 0;}
static void Read8(BDataIO &rd, int8 &val) {rd.Read(&val, sizeof(val));}
static void Read16(BDataIO &rd, int16 &val) {rd.Read(&val, sizeof(val));}
static void Read32(BDataIO &rd, int32 &val) {rd.Read(&val, sizeof(val));}
static void ReadFloat(BDataIO &rd, float &val) {rd.Read(&val, sizeof(val));}
static void ReadDouble(BDataIO &rd, double &val) {rd.Read(&val, sizeof(val));}
static void ReadPoint(BDataIO &rd, BPoint &val) {rd.Read(&val, sizeof(val));}
static void ReadRect(BDataIO &rd, BRect &val) {rd.Read(&val, sizeof(val));}
static void ReadTransform(BDataIO &rd, BAffineTransform &val) {rd.Read(&val, sizeof(val));}
static void ReadPattern(BDataIO &rd, pattern &val) {rd.Read(&val, sizeof(val));}

static void ReadColor(BDataIO &rd, rgb_color& color)
{
	int8 val;
	Read8(rd, val); color.red = (uint8)val;
	Read8(rd, val); color.green = (uint8)val;
	Read8(rd, val); color.blue = (uint8)val;
	Read8(rd, val); color.alpha = (uint8)val;
}

static void ReadString(BDataIO &rd, BString &val)
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

static void ReadShape(BDataIO &rd, BShape &shape)
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

static void ReadGradientStops(BDataIO &rd, BGradient &gradient)
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

static void ReadGradient(BDataIO &rd, ObjectDeleter<BGradient> &outGradient)
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

static void DumpOps(PictureVisitor &vis, BPositionIO &rd, int32 size);

static void DumpOp(PictureVisitor &vis, BPositionIO &rd, int16 op, int32 opSize)
{
	switch (op) {
	case B_PIC_MOVE_PEN_BY: {
		float dx, dy;
		ReadFloat(rd, dx);
		ReadFloat(rd, dy);
		vis.MovePenBy(dx, dy);
		break;
	}
	case B_PIC_STROKE_LINE:
	case B_PIC_STROKE_LINE_GRADIENT: {
		bool isGradient = op == B_PIC_STROKE_LINE_GRADIENT;
		BPoint start, end;
		ObjectDeleter<BGradient> gradient;
		ReadPoint(rd, start);
		ReadPoint(rd, end);
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawLine(start, end, {.isStroke = true, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_RECT:
	case B_PIC_FILL_RECT:
	case B_PIC_STROKE_RECT_GRADIENT:
	case B_PIC_FILL_RECT_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_RECT || op == B_PIC_STROKE_RECT_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_RECT_GRADIENT || op == B_PIC_FILL_RECT_GRADIENT;
		BRect rect;
		ObjectDeleter<BGradient> gradient;
		ReadRect(rd, rect);
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawRect(rect, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_ROUND_RECT:
	case B_PIC_FILL_ROUND_RECT:
	case B_PIC_STROKE_ROUND_RECT_GRADIENT:
	case B_PIC_FILL_ROUND_RECT_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_ROUND_RECT || op == B_PIC_STROKE_ROUND_RECT_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_ROUND_RECT_GRADIENT || op == B_PIC_FILL_ROUND_RECT_GRADIENT;
		BRect rect;
		BPoint radius;
		ObjectDeleter<BGradient> gradient;
		ReadRect(rd, rect);
		ReadPoint(rd, radius);
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawRoundRect(rect, radius, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_BEZIER:
	case B_PIC_FILL_BEZIER:
	case B_PIC_STROKE_BEZIER_GRADIENT:
	case B_PIC_FILL_BEZIER_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_BEZIER || op == B_PIC_STROKE_BEZIER_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_BEZIER_GRADIENT || op == B_PIC_FILL_BEZIER_GRADIENT;
		BPoint points[4];
		ObjectDeleter<BGradient> gradient;
		for (int32 i = 0; i < 4; i++) {
			ReadPoint(rd, points[i]);
		}
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawBezier(points, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_POLYGON:
	case B_PIC_FILL_POLYGON:
	case B_PIC_STROKE_POLYGON_GRADIENT:
	case B_PIC_FILL_POLYGON_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_POLYGON || op == B_PIC_STROKE_POLYGON_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_POLYGON_GRADIENT || op == B_PIC_FILL_POLYGON_GRADIENT;
		std::vector<BPoint> points;
		int32 numPoints;
		bool isClosed;
		ObjectDeleter<BGradient> gradient;
		Read32(rd, numPoints);
		for (int32 i = 0; i < numPoints; i++) {
			BPoint point;
			ReadPoint(rd, point);
			points.push_back(point);
		}
		if (isStroke) {
			ReadBool(rd, isClosed);
		} else {
			isClosed = true;
		}
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawPolygon(numPoints, points.data(), isClosed, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_SHAPE:
	case B_PIC_FILL_SHAPE:
	case B_PIC_STROKE_SHAPE_GRADIENT:
	case B_PIC_FILL_SHAPE_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_SHAPE || op == B_PIC_STROKE_SHAPE_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_SHAPE_GRADIENT || op == B_PIC_FILL_SHAPE_GRADIENT;
		BShape shape;
		ObjectDeleter<BGradient> gradient;
		ReadShape(rd, shape);
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawShape(shape, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_ARC:
	case B_PIC_FILL_ARC:
	case B_PIC_STROKE_ARC_GRADIENT:
	case B_PIC_FILL_ARC_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_ARC || op == B_PIC_STROKE_ARC_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_ARC_GRADIENT || op == B_PIC_FILL_ARC_GRADIENT;
		BPoint center;
		BPoint radius;
		float startTheta;
		float arcTheta;
		ObjectDeleter<BGradient> gradient;
		ReadPoint(rd, center);
		ReadPoint(rd, radius);
		ReadFloat(rd, startTheta);
		ReadFloat(rd, arcTheta);
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawArc(center, radius, startTheta, arcTheta, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_STROKE_ELLIPSE:
	case B_PIC_FILL_ELLIPSE:
	case B_PIC_STROKE_ELLIPSE_GRADIENT:
	case B_PIC_FILL_ELLIPSE_GRADIENT: {
		bool isStroke = op == B_PIC_STROKE_ELLIPSE || op == B_PIC_STROKE_ELLIPSE_GRADIENT;
		bool isGradient = op == B_PIC_STROKE_ELLIPSE_GRADIENT || op == B_PIC_FILL_ELLIPSE_GRADIENT;
		BRect rect;
		ObjectDeleter<BGradient> gradient;
		ReadRect(rd, rect);
		if (isGradient) {
			ReadGradient(rd, gradient);
		}
		vis.DrawEllipse(rect, {.isStroke = isStroke, .gradient = gradient.Get()});
		break;
	}
	case B_PIC_DRAW_STRING: {
		BString string;
		escapement_delta delta;
		ReadString(rd, string);
		ReadFloat(rd, delta.space);
		ReadFloat(rd, delta.nonspace);
		vis.DrawString(string.String(), string.Length(), delta);
		break;
	}
	case B_PIC_DRAW_PIXELS: {
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
	case B_PIC_DRAW_PICTURE: {
		BPoint where;
		int32 token;
		ReadPoint(rd, where);
		Read32(rd, token);
		vis.DrawPicture(where, token);
		break;
	}
	case B_PIC_DRAW_STRING_LOCATIONS: {
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

	case B_PIC_ENTER_STATE_CHANGE: {
		vis.EnterStateChange();
		DumpOps(vis, rd, opSize);
		vis.ExitStateChange();
		break;
	}
	case B_PIC_SET_CLIPPING_RECTS: {
		BRegion region;
		clipping_rect bounds;
		Read32(rd, bounds.left);
		Read32(rd, bounds.top);
		Read32(rd, bounds.right);
		Read32(rd, bounds.bottom);
		int32 numRects = opSize / sizeof(clipping_rect);
		if (numRects >= 2) {
			for (int32 i = 1; i < numRects; i++) {
				clipping_rect rc;
				Read32(rd, rc.left);
				Read32(rd, rc.top);
				Read32(rd, rc.right);
				Read32(rd, rc.bottom);
				region.Include(rc);
			}
		} else {
			region.Include(bounds);
		}
		vis.SetClipping(region);
		break;
	}
	case B_PIC_CLIP_TO_PICTURE: {
		int32 token;
		BPoint where;
		bool inverse;
		Read32(rd, token);
		ReadPoint(rd, where);
		ReadBool(rd, inverse);
		vis.ClipToPicture(token, where, inverse);
		break;
	}
	case B_PIC_PUSH_STATE:
		vis.PushState();
		break;
	case B_PIC_POP_STATE:
		vis.PopState();
		break;
	case B_PIC_CLEAR_CLIPPING_RECTS:
		vis.ClearClipping();
		break;
	case B_PIC_CLIP_TO_RECT: {
		bool inverse;
		BRect rect;
		ReadBool(rd, inverse);
		ReadRect(rd, rect);
		vis.ClipToRect(rect, inverse);
		break;
	}
	case B_PIC_CLIP_TO_SHAPE: {
		bool inverse;
		BShape shape;
		ReadBool(rd, inverse);
		ReadShape(rd, shape);
		vis.ClipToShape(shape, inverse);
		break;
	}

	case B_PIC_SET_ORIGIN: {
		BPoint point;
		ReadPoint(rd, point);
		vis.SetOrigin(point);
		break;
	}
	case B_PIC_SET_PEN_LOCATION: {
		BPoint point;
		ReadPoint(rd, point);
		vis.SetPenLocation(point);
		break;
	}
	case B_PIC_SET_DRAWING_MODE: {
		int16 mode;
		Read16(rd, mode);
		vis.SetDrawingMode((drawing_mode)mode);
		break;
	}
	case B_PIC_SET_LINE_MODE: {
		int16 capMode;
		int16 joinMode;
		float miterLimit;

		Read16(rd, capMode);
		Read16(rd, joinMode);
		ReadFloat(rd, miterLimit);

		vis.SetLineMode((cap_mode)capMode, (join_mode)joinMode, miterLimit);
		break;
	}
	case B_PIC_SET_PEN_SIZE: {
		float penSize;
		ReadFloat(rd, penSize);
		vis.SetPenSize(penSize);
		break;
	}
	case B_PIC_SET_SCALE: {
		float scale;
		ReadFloat(rd, scale);
		vis.SetScale(scale);
		break;
	}
	case B_PIC_SET_FORE_COLOR: {
		rgb_color color;
		ReadColor(rd, color);
		vis.SetHighColor(color);
		break;
	}
	case B_PIC_SET_BACK_COLOR: {
		rgb_color color;
		ReadColor(rd, color);
		vis.SetLowColor(color);
		break;
	}
	case B_PIC_SET_STIPLE_PATTERN: {
		pattern pat;
		ReadPattern(rd, pat);
		vis.SetPattern(pat);
		break;
	}
	case B_PIC_ENTER_FONT_STATE: {
		vis.EnterFontState();
		DumpOps(vis, rd, opSize);
		vis.ExitFontState();
		break;
	}
	case B_PIC_SET_BLENDING_MODE: {
		int16 srcAlpha;
		int16 alphaFunc;

		Read16(rd, srcAlpha);
		Read16(rd, alphaFunc);

		vis.SetBlendingMode((source_alpha)srcAlpha, (alpha_function)alphaFunc);
		break;
	}
	case B_PIC_SET_FILL_RULE: {
		int32 fillRule;
		Read32(rd, fillRule);
		vis.SetFillRule(fillRule);
		break;
	}

	case B_PIC_SET_FONT_FAMILY: {
		BString str;
		ReadString(rd, str);
		font_family family;
		size_t len = std::min<size_t>(str.Length(), sizeof(family) - 1);
		memcpy(family, str.String(), len);
		family[len] = '\0';
		vis.SetFontFamily(family);
		break;
	}
	case B_PIC_SET_FONT_STYLE: {
		BString str;
		ReadString(rd, str);
		font_style style;
		size_t len = std::min<size_t>(str.Length(), sizeof(style) - 1);
		memcpy(style, str.String(), len);
		style[len] = '\0';
		vis.SetFontStyle(style);
		break;
	}
	case B_PIC_SET_FONT_SPACING: {
		int32 spacing;
		Read32(rd, spacing);
		vis.SetFontSpacing(spacing);
		break;
	}
	case B_PIC_SET_FONT_ENCODING: {
		int32 encoding;
		Read32(rd, encoding);
		vis.SetFontEncoding(encoding);
		break;
	}
	case B_PIC_SET_FONT_FLAGS: {
		int32 flags;
		Read32(rd, flags);
		vis.SetFontFlags(flags);
		break;
	}
	case B_PIC_SET_FONT_SIZE: {
		float size;
		ReadFloat(rd, size);
		vis.SetFontSize(size);
		break;
	}
	case B_PIC_SET_FONT_ROTATE: {
		float rotation;
		ReadFloat(rd, rotation);
		vis.SetFontRotation(rotation);
		break;
	}
	case B_PIC_SET_FONT_SHEAR: {
		float shear;
		ReadFloat(rd, shear);
		vis.SetFontShear(shear);
		break;
	}
	case B_PIC_SET_FONT_BPP: {
		int32 bpp;
		Read32(rd, bpp);
		vis.SetFontBpp(bpp);
		break;
	}
	case B_PIC_SET_FONT_FACE: {
		int32 face;
		Read32(rd, face);
		vis.SetFontFace(face);
		break;
	}
	case B_PIC_SET_TRANSFORM: {
		BAffineTransform transform;
		ReadTransform(rd, transform);
		vis.SetTransform(transform);
		break;
	}
	case B_PIC_AFFINE_TRANSLATE: {
		double x, y;
		ReadDouble(rd, x);
		ReadDouble(rd, y);
		vis.TranslateBy(x, y);
		break;
	}
	case B_PIC_AFFINE_SCALE: {
		double x, y;
		ReadDouble(rd, x);
		ReadDouble(rd, y);
		vis.ScaleBy(x, y);
		break;
	}
	case B_PIC_AFFINE_ROTATE: {
		double angleRadians;
		ReadDouble(rd, angleRadians);
		vis.RotateBy(angleRadians);
		break;
	}
	case B_PIC_BLEND_LAYER: {
		// TODO
		vis.BlendLayer(NULL);
		break;
	}
	default:
		break;
	}
}

static void DumpOps(PictureVisitor &vis, BPositionIO &rd, int32 size)
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


status_t PictureReaderBinary::Accept(PictureVisitor &vis) const
{
	int32 version;
	int32 endian;
	int32 count;
	int32 size;
	Read32(fRd, version);
	Read32(fRd, endian);
	vis.EnterPicture(version, endian);
	Read32(fRd, count);
	if (count > 0) {
		vis.EnterPictures(count);
		for (int32 i = 0; i < count; i++) {
			Accept(vis);
		}
		vis.ExitPictures();
	}
	Read32(fRd, size);
	vis.EnterOps();
	DumpOps(vis, fRd, size);
	vis.ExitOps();
	vis.ExitPicture();
	return B_OK;
}
