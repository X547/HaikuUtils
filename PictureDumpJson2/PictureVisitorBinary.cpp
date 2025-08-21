#include "PictureVisitorBinary.h"

#include <stdio.h>
#include <stdlib.h>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>

#include <private/interface/ShapePrivate.h>

#include "PictureOpcodes.h"


PictureVisitorBinary::PictureVisitorBinary(BPositionIO &wr):
	fWr(wr)
{
}


void PictureVisitorBinary::RaiseUnimplemented()
{
	fprintf(stderr, "[!] unimplemented\n");
	abort();
}

void PictureVisitorBinary::Check(bool cond)
{
	if (!cond) {
		fprintf(stderr, "[!] error\n");
		abort();
	}
}

void PictureVisitorBinary::CheckStatus(status_t status)
{
	if (status < B_OK) {
		fprintf(stderr, "[!] error\n");
		abort();
	}
}

void PictureVisitorBinary::BeginChunk(int16 op)
{
	fChunkStack.push_back(fWr.Position());
	Write16(op);
	Write32(0);
}

void PictureVisitorBinary::EndChunk()
{
	off_t pos = fWr.Position();
	off_t startPos = fChunkStack.back();
	fChunkStack.pop_back();
	fWr.Seek(startPos + 2, SEEK_SET);
	Write32(pos - startPos - (2 + 4));
}

void PictureVisitorBinary::WriteColor(const rgb_color &c)
{
	Write8(c.red);
	Write8(c.green);
	Write8(c.blue);
	Write8(c.alpha);
}

void PictureVisitorBinary::WriteString(std::string_view str)
{
	Write32(str.size());
	CheckStatus(fWr.WriteExactly(str.data(), str.size()));
}

void PictureVisitorBinary::WriteShape(const BShape &shape)
{
	int32 opCount;
	int32 ptCount;
	uint32* opList;
	BPoint* ptList;

	BShape::Private(const_cast<BShape&>(shape)).GetData(&opCount, &ptCount, &opList, &ptList);

	Write32(opCount);
	Write32(ptCount);
	CheckStatus(fWr.WriteExactly(opList, opCount*sizeof(uint32)));
	CheckStatus(fWr.WriteExactly(ptList, ptCount*sizeof(BPoint)));
}

void PictureVisitorBinary::WriteGradient(const BGradient &gradient)
{
	RaiseUnimplemented();
}


// #pragma mark - Meta

void PictureVisitorBinary::EnterPicture(int32 version, int32 unknown)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ExitPicture()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::EnterPictures(int32 count)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ExitPictures()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::EnterOps()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ExitOps()
{
	RaiseUnimplemented();
}


void PictureVisitorBinary::EnterStateChange()
{
	BeginChunk(B_PIC_ENTER_STATE_CHANGE);
}

void PictureVisitorBinary::ExitStateChange()
{
	EndChunk();
}

void PictureVisitorBinary::EnterFontState()
{
	BeginChunk(B_PIC_ENTER_FONT_STATE);
}

void PictureVisitorBinary::ExitFontState()
{
	EndChunk();
}

void PictureVisitorBinary::PushState()
{
	BeginChunk(B_PIC_PUSH_STATE);
	EndChunk();
}

void PictureVisitorBinary::PopState()
{
	BeginChunk(B_PIC_POP_STATE);
	EndChunk();
}


// #pragma mark - State Absolute

void PictureVisitorBinary::SetDrawingMode(drawing_mode mode)
{
	BeginChunk(B_PIC_SET_DRAWING_MODE);
	Write16(mode);
	EndChunk();
}

void PictureVisitorBinary::SetLineMode(
	cap_mode cap,
	join_mode join,
	float miterLimit
)
{
	BeginChunk(B_PIC_SET_LINE_MODE);
	Write16(cap);
	Write16(join);
	WriteFloat(miterLimit);
	EndChunk();
}

void PictureVisitorBinary::SetPenSize(float penSize)
{
	BeginChunk(B_PIC_SET_PEN_SIZE);
	WriteFloat(penSize);
	EndChunk();
}

void PictureVisitorBinary::SetHighColor(const rgb_color& color)
{
	BeginChunk(B_PIC_SET_FORE_COLOR);
	WriteColor(color);
	EndChunk();
}

void PictureVisitorBinary::SetLowColor(const rgb_color& color)
{
	BeginChunk(B_PIC_SET_BACK_COLOR);
	WriteColor(color);
	EndChunk();
}

void PictureVisitorBinary::SetPattern(const ::pattern& pat)
{
	BeginChunk(B_PIC_SET_STIPLE_PATTERN);
	WritePattern(pat);
	EndChunk();
}

void PictureVisitorBinary::SetBlendingMode(
	source_alpha srcAlpha, alpha_function alphaFunc
)
{
	BeginChunk(B_PIC_SET_BLENDING_MODE);
	Write16(srcAlpha);
	Write16(alphaFunc);
	EndChunk();
}

void PictureVisitorBinary::SetFillRule(int32 fillRule)
{
	BeginChunk(B_PIC_SET_FILL_RULE);
	Write32(fillRule);
	EndChunk();
}


// #pragma mark - State Relative

void PictureVisitorBinary::SetOrigin(const BPoint& point)
{
	BeginChunk(B_PIC_SET_ORIGIN);
	WritePoint(point);
	EndChunk();
}

void PictureVisitorBinary::SetScale(float scale)
{
	BeginChunk(B_PIC_SET_SCALE);
	WriteFloat(scale);
	EndChunk();
}

void PictureVisitorBinary::SetPenLocation(const BPoint& point)
{
	BeginChunk(B_PIC_SET_PEN_LOCATION);
	WritePoint(point);
	EndChunk();
}

void PictureVisitorBinary::SetTransform(const BAffineTransform& transform)
{
	BeginChunk(B_PIC_SET_TRANSFORM);
	WriteTransform(transform);
	EndChunk();
}


// #pragma mark - Clipping

void PictureVisitorBinary::SetClipping(const BRegion& region)
{
	BeginChunk(B_PIC_SET_CLIPPING_RECTS);
	Write32(region.CountRects());
	for (int32 i = 0; i < region.CountRects(); i++) {
		WriteRect(region.RectAt(i));
	}
	EndChunk();
}

void PictureVisitorBinary::ClearClipping()
{
	BeginChunk(B_PIC_CLEAR_CLIPPING_RECTS);
	EndChunk();
}

void PictureVisitorBinary::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
{
	BeginChunk(B_PIC_CLIP_TO_PICTURE);
	Write32(pictureToken);
	WritePoint(origin);
	WriteBool(inverse);
	EndChunk();
}

void PictureVisitorBinary::ClipToRect(const BRect& rect, bool inverse)
{
	BeginChunk(B_PIC_CLIP_TO_RECT);
	WriteBool(inverse);
	WriteRect(rect);
	EndChunk();
}

void PictureVisitorBinary::ClipToShape(const BShape& shape, bool inverse)
{
	BeginChunk(B_PIC_CLIP_TO_SHAPE);
	WriteBool(inverse);
	WriteShape(shape);
	EndChunk();
}


// #pragma mark - Font

void PictureVisitorBinary::SetFontFamily(const font_family family)
{
	BeginChunk(B_PIC_SET_FONT_FAMILY);
	WriteString(std::string_view(family, strlen(family) + 1));
	EndChunk();
}

void PictureVisitorBinary::SetFontStyle(const font_style style)
{
	BeginChunk(B_PIC_SET_FONT_STYLE);
	WriteString(std::string_view(style, strlen(style) + 1));
	EndChunk();
}

void PictureVisitorBinary::SetFontSpacing(int32 spacing)
{
	BeginChunk(B_PIC_SET_FONT_SPACING);
	Write32(spacing);
	EndChunk();
}

void PictureVisitorBinary::SetFontSize(float size)
{
	BeginChunk(B_PIC_SET_FONT_SIZE);
	WriteFloat(size);
	EndChunk();
}

void PictureVisitorBinary::SetFontRotation(float rotation)
{
	BeginChunk(B_PIC_SET_FONT_ROTATE);
	WriteFloat(rotation);
	EndChunk();
}

void PictureVisitorBinary::SetFontEncoding(int32 encoding)
{
	BeginChunk(B_PIC_SET_FONT_ENCODING);
	Write32(encoding);
	EndChunk();
}

void PictureVisitorBinary::PictureVisitorBinary::SetFontFlags(int32 flags)
{
	BeginChunk(B_PIC_SET_FONT_FLAGS);
	Write32(flags);
	EndChunk();
}

void PictureVisitorBinary::SetFontShear(float shear)
{
	BeginChunk(B_PIC_SET_FONT_SHEAR);
	WriteFloat(shear);
	EndChunk();
}

void PictureVisitorBinary::SetFontBpp(int32 bpp)
{
	BeginChunk(B_PIC_SET_FONT_BPP);
	Write32(bpp);
	EndChunk();
}

void PictureVisitorBinary::SetFontFace(int32 face)
{
	BeginChunk(B_PIC_SET_FONT_FACE);
	Write32(face);
	EndChunk();
}


// #pragma mark - State (delta)

void PictureVisitorBinary::MovePenBy(float dx, float dy)
{
	BeginChunk(B_PIC_MOVE_PEN_BY);
	WriteFloat(dx);
	WriteFloat(dy);
	EndChunk();
}

void PictureVisitorBinary::TranslateBy(double x, double y)
{
	BeginChunk(B_PIC_AFFINE_TRANSLATE);
	WriteDouble(x);
	WriteDouble(y);
	EndChunk();
}

void PictureVisitorBinary::ScaleBy(double x, double y)
{
	BeginChunk(B_PIC_AFFINE_SCALE);
	WriteDouble(x);
	WriteDouble(y);
	EndChunk();
}

void PictureVisitorBinary::RotateBy(double angleRadians)
{
	BeginChunk(B_PIC_AFFINE_ROTATE);
	WriteDouble(angleRadians);
	EndChunk();
}


// #pragma mark - Geometry

void PictureVisitorBinary::StrokeLine(const BPoint& start, const BPoint& end)
{
	BeginChunk(B_PIC_STROKE_LINE);
	WritePoint(start);
	WritePoint(end);
	EndChunk();
}


void PictureVisitorBinary::StrokeRect(const BRect& rect)
{
	BeginChunk(B_PIC_STROKE_RECT);
	WriteRect(rect);
	EndChunk();
}

void PictureVisitorBinary::FillRect(const BRect& rect)
{
	BeginChunk(B_PIC_FILL_RECT);
	WriteRect(rect);
	EndChunk();
}

void PictureVisitorBinary::StrokeRect(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_RECT_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillRect(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_RECT_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}


void PictureVisitorBinary::StrokeRoundRect(const BRect& rect, const BPoint& radius)
{
	BeginChunk(B_PIC_STROKE_ROUND_RECT);
	WriteRect(rect);
	WritePoint(radius);
	EndChunk();
}

void PictureVisitorBinary::FillRoundRect(const BRect& rect, const BPoint& radius)
{
	BeginChunk(B_PIC_FILL_ROUND_RECT);
	WriteRect(rect);
	WritePoint(radius);
	EndChunk();
}

void PictureVisitorBinary::StrokeRoundRect(
	const BRect& rect, const BPoint& radius, const BGradient& gradient
)
{
	BeginChunk(B_PIC_STROKE_ROUND_RECT_GRADIENT);
	WriteRect(rect);
	WritePoint(radius);
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillRoundRect(
	const BRect& rect, const BPoint& radius, const BGradient& gradient
)
{
	BeginChunk(B_PIC_FILL_ROUND_RECT_GRADIENT);
	WriteRect(rect);
	WritePoint(radius);
	WriteGradient(gradient);
	EndChunk();
}


void PictureVisitorBinary::StrokeBezier(const BPoint points[4])
{
	BeginChunk(B_PIC_STROKE_BEZIER);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	EndChunk();
}

void PictureVisitorBinary::FillBezier(const BPoint points[4])
{
	BeginChunk(B_PIC_FILL_BEZIER);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	EndChunk();
}

void PictureVisitorBinary::StrokeBezier(const BPoint points[4], const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_BEZIER_GRADIENT);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillBezier(const BPoint points[4], const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_BEZIER_GRADIENT);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	WriteGradient(gradient);
	EndChunk();
}


void PictureVisitorBinary::StrokePolygon(int32 numPoints, const BPoint* points, bool isClosed)
{
	BeginChunk(B_PIC_STROKE_POLYGON);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	WriteBool(isClosed);
	EndChunk();
}

void PictureVisitorBinary::FillPolygon(int32 numPoints, const BPoint* points)
{
	BeginChunk(B_PIC_FILL_POLYGON);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	EndChunk();
}

void PictureVisitorBinary::StrokePolygon(int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_POLYGON_GRADIENT);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	WriteBool(isClosed);
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillPolygon(int32 numPoints, const BPoint* points, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_POLYGON_GRADIENT);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	WriteGradient(gradient);
	EndChunk();
}


void PictureVisitorBinary::StrokeShape(const BShape& shape)
{
	BeginChunk(B_PIC_STROKE_SHAPE);
	WriteShape(shape);
	EndChunk();
}

void PictureVisitorBinary::FillShape(const BShape& shape)
{
	BeginChunk(B_PIC_FILL_SHAPE);
	WriteShape(shape);
	EndChunk();
}

void PictureVisitorBinary::StrokeShape(const BShape& shape, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_SHAPE_GRADIENT);
	WriteShape(shape);
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillShape(const BShape& shape, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_SHAPE_GRADIENT);
	WriteShape(shape);
	WriteGradient(gradient);
	EndChunk();
}


void PictureVisitorBinary::StrokeArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta
)
{
	BeginChunk(B_PIC_STROKE_ARC);
	WritePoint(center);
	WritePoint(radius);
	WriteFloat(startTheta);
	WriteFloat(arcTheta);
	EndChunk();
}

void PictureVisitorBinary::FillArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta
)
{
	BeginChunk(B_PIC_FILL_ARC);
	WritePoint(center);
	WritePoint(radius);
	WriteFloat(startTheta);
	WriteFloat(arcTheta);
	EndChunk();
}

void PictureVisitorBinary::StrokeArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const BGradient& gradient
)
{
	BeginChunk(B_PIC_STROKE_ARC_GRADIENT);
	WritePoint(center);
	WritePoint(radius);
	WriteFloat(startTheta);
	WriteFloat(arcTheta);
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const BGradient& gradient
)
{
	BeginChunk(B_PIC_FILL_ARC_GRADIENT);
	WritePoint(center);
	WritePoint(radius);
	WriteFloat(startTheta);
	WriteFloat(arcTheta);
	WriteGradient(gradient);
	EndChunk();
}


void PictureVisitorBinary::StrokeEllipse(const BRect& rect)
{
	BeginChunk(B_PIC_STROKE_ELLIPSE);
	WriteRect(rect);
	EndChunk();
}

void PictureVisitorBinary::FillEllipse(const BRect& rect)
{
	BeginChunk(B_PIC_FILL_ELLIPSE);
	WriteRect(rect);
	EndChunk();
}

void PictureVisitorBinary::StrokeEllipse(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_ELLIPSE_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}

void PictureVisitorBinary::FillEllipse(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_ELLIPSE_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}


// #pragma mark - Draw

void PictureVisitorBinary::DrawString(
	const char* string, int32 length,
	const escapement_delta& delta
)
{
	BeginChunk(B_PIC_DRAW_STRING);
	WriteString(std::string_view(string, length));
	WriteFloat(delta.space);
	WriteFloat(delta.nonspace);
	EndChunk();
}

void PictureVisitorBinary::DrawString(
	const char* string, int32 length,
	const BPoint* locations, int32 locationCount
)
{
	BeginChunk(B_PIC_DRAW_STRING_LOCATIONS);
	Write32(locationCount);
	for (int32 i = 0; i < locationCount; i++) {
		WritePoint(locations[i]);
	}
	WriteString(std::string_view(string, length));
	EndChunk();
}


void PictureVisitorBinary::DrawBitmap(
	const BRect& srcRect,
	const BRect& dstRect, int32 width,
	int32 height,
	int32 bytesPerRow,
	int32 colorSpace,
	int32 flags,
	const void* data, int32 length
)
{
	BeginChunk(B_PIC_DRAW_PIXELS);
	WriteRect(srcRect);
	WriteRect(dstRect);
	Write32(width);
	Write32(height);
	Write32(bytesPerRow);
	Write32(colorSpace);
	Write32(flags);
	CheckStatus(fWr.WriteExactly(data, length));
	EndChunk();
}

void PictureVisitorBinary::DrawPicture(const BPoint& where, int32 token)
{
	BeginChunk(B_PIC_DRAW_PICTURE);
	WritePoint(where);
	Write32(token);
	EndChunk();
}


void PictureVisitorBinary::BlendLayer(Layer* layer)
{
	RaiseUnimplemented();
}
