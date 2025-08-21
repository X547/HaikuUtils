#include "PictureVisitorBinary.h"

#include <stdio.h>
#include <stdlib.h>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>

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

void PictureVisitorBinary::WriteShape(const BShape &shape)
{
	RaiseUnimplemented();
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
	RaiseUnimplemented();
}

void PictureVisitorBinary::ExitStateChange()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::EnterFontState()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ExitFontState()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::PushState()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::PopState()
{
	RaiseUnimplemented();
}


// #pragma mark - State Absolute

void PictureVisitorBinary::SetDrawingMode(drawing_mode mode)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetLineMode(
	cap_mode cap,
	join_mode join,
	float miterLimit
)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetPenSize(float penSize)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetHighColor(const rgb_color& color)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetLowColor(const rgb_color& color)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetPattern(const ::pattern& pat)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetBlendingMode(
	source_alpha srcAlpha, alpha_function alphaFunc
)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFillRule(int32 fillRule)
{
	RaiseUnimplemented();
}


// #pragma mark - State Relative

void PictureVisitorBinary::SetOrigin(const BPoint& point)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetScale(float scale)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetPenLocation(const BPoint& point)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetTransform(const BAffineTransform& transform)
{
	RaiseUnimplemented();
}


// #pragma mark - Clipping

void PictureVisitorBinary::SetClipping(const BRegion& region)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ClearClipping()
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ClipToRect(const BRect& rect, bool inverse)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ClipToShape(const BShape& shape, bool inverse)
{
	RaiseUnimplemented();
}


// #pragma mark - Font

void PictureVisitorBinary::SetFontFamily(const font_family family)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontStyle(const font_style style)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontSpacing(int32 spacing)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontSize(float size)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontRotation(float rotation)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontEncoding(int32 encoding)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::PictureVisitorBinary::SetFontFlags(int32 flags)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontShear(float shear)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontBpp(int32 bpp)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::SetFontFace(int32 face)
{
	RaiseUnimplemented();
}


// #pragma mark - State (delta)

void PictureVisitorBinary::MovePenBy(float dx, float dy)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::TranslateBy(double x, double y)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::ScaleBy(double x, double y)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::RotateBy(double angleRadians)
{
	RaiseUnimplemented();
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
	RaiseUnimplemented();
}

void PictureVisitorBinary::FillShape(const BShape& shape)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::StrokeShape(const BShape& shape, const BGradient& gradient)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::FillShape(const BShape& shape, const BGradient& gradient)
{
	RaiseUnimplemented();
}


void PictureVisitorBinary::StrokeArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta
)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::FillArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta
)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::StrokeArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const BGradient& gradient
)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::FillArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const BGradient& gradient
)
{
	RaiseUnimplemented();
}


void PictureVisitorBinary::StrokeEllipse(const BRect& rect)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::FillEllipse(const BRect& rect)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::StrokeEllipse(const BRect& rect, const BGradient& gradient)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::FillEllipse(const BRect& rect, const BGradient& gradient)
{
	RaiseUnimplemented();
}


// #pragma mark - Draw

void PictureVisitorBinary::DrawString(
	const char* string, int32 length,
	const escapement_delta& delta
)
{
	RaiseUnimplemented();
}

void PictureVisitorBinary::DrawString(
	const char* string,
	int32 length, const BPoint* locations,
	int32 locationCount
)
{
	RaiseUnimplemented();
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
	RaiseUnimplemented();
}

void PictureVisitorBinary::DrawPicture(const BPoint& where, int32 token)
{
	RaiseUnimplemented();
}


void PictureVisitorBinary::BlendLayer(Layer* layer)
{
	RaiseUnimplemented();
}
