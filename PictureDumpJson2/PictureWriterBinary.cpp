#include "PictureWriterBinary.h"

#include <stdio.h>
#include <stdlib.h>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>

#include <private/interface/ShapePrivate.h>

#include "PictureOpcodes.h"


PictureWriterBinary::PictureWriterBinary(BPositionIO &wr):
	fWr(wr)
{
}


void PictureWriterBinary::RaiseUnimplemented()
{
	fprintf(stderr, "[!] unimplemented\n");
	abort();
}

void PictureWriterBinary::RaiseError()
{
	fprintf(stderr, "[!] error\n");
	abort();
}

void PictureWriterBinary::Check(bool cond)
{
	if (!cond) {
		RaiseError();
	}
}

void PictureWriterBinary::CheckStatus(status_t status)
{
	if (status < B_OK) {
		RaiseError();
	}
}

void PictureWriterBinary::BeginChunk(int16 op)
{
	fChunkStack.push_back(fWr.Position());
	Write16(op);
	Write32(0);
}

void PictureWriterBinary::EndChunk()
{
	off_t pos = fWr.Position();
	off_t startPos = fChunkStack.back();
	fChunkStack.pop_back();
	fWr.Seek(startPos + 2, SEEK_SET);
	Write32(pos - startPos - (2 + 4));
	fWr.Seek(pos, SEEK_SET);
}

void PictureWriterBinary::WriteColor(const rgb_color &c)
{
	Write8(c.red);
	Write8(c.green);
	Write8(c.blue);
	Write8(c.alpha);
}

void PictureWriterBinary::WriteString(std::string_view str)
{
	Write32(str.size());
	CheckStatus(fWr.WriteExactly(str.data(), str.size()));
}

void PictureWriterBinary::WriteShape(const BShape &shape)
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

void PictureWriterBinary::WriteGradient(const BGradient &gradient)
{
	Write32(gradient.GetType());
	Write32(gradient.CountColorStops());
	for (int32 i = 0; i < gradient.CountColorStops(); i++) {
		BGradient::ColorStop *cs = gradient.ColorStopAt(i);
		WriteColor(cs->color);
		WriteFloat(cs->offset);
	}
	switch (gradient.GetType()) {
		case BGradient::TYPE_LINEAR: {
			const BGradientLinear &grad = static_cast<const BGradientLinear&>(gradient);
			WritePoint(grad.Start());
			WritePoint(grad.End());
			break;
		}
		case BGradient::TYPE_RADIAL: {
			const BGradientRadial &grad = static_cast<const BGradientRadial&>(gradient);
			WritePoint(grad.Center());
			WriteFloat(grad.Radius());
			break;
		}
		case BGradient::TYPE_RADIAL_FOCUS: {
			const BGradientRadialFocus &grad = static_cast<const BGradientRadialFocus&>(gradient);
			WritePoint(grad.Center());
			WritePoint(grad.Focal());
			WriteFloat(grad.Radius());
			break;
		}
		case BGradient::TYPE_DIAMOND: {
			const BGradientDiamond &grad = static_cast<const BGradientDiamond&>(gradient);
			WritePoint(grad.Center());
			break;
		}
		case BGradient::TYPE_CONIC: {
			const BGradientConic &grad = static_cast<const BGradientConic&>(gradient);
			WritePoint(grad.Center());
			WriteFloat(grad.Angle());
			break;
		}
		default: {
			RaiseError();
			break;
		}
	}
}


// #pragma mark - Meta

void PictureWriterBinary::EnterPicture(int32 version, int32 unknown)
{
	if (!fPictureStack.empty()) {
		PictureInfo &prevInfo = fPictureStack.back();
		prevInfo.pictCnt++;
	}

	fPictureStack.push_back({});
	PictureInfo &info = fPictureStack.back();

	info.pos = fWr.Position();

	Write32(version);
	Write32(unknown);
}

void PictureWriterBinary::ExitPicture()
{
	PictureInfo &info = fPictureStack.back();
	if (!info.isSet.pictures) {
		Write32(info.pictCnt);
	}
	if (!info.isSet.ops) {
		Write32(0);
	}
	fPictureStack.pop_back();
}

void PictureWriterBinary::EnterPictures(int32 count)
{
	PictureInfo &info = fPictureStack.back();
	Write32(0);
	info.isSet.pictures = true;
}

void PictureWriterBinary::ExitPictures()
{
	PictureInfo &info = fPictureStack.back();
	off_t oldPos = fWr.Position();
	fWr.Seek(info.pos + 4 + 4, SEEK_SET);
	Write32(info.pictCnt);
	fWr.Seek(oldPos, SEEK_SET);
}

void PictureWriterBinary::EnterOps()
{
	PictureInfo &info = fPictureStack.back();
	if (!info.isSet.pictures) {
		info.isSet.pictures = true;
		Write32(info.pictCnt);
	}
	Write32(0);
	info.opsPos = fWr.Position();
	info.isSet.ops = true;
}

void PictureWriterBinary::ExitOps()
{
	PictureInfo &info = fPictureStack.back();
	off_t oldPos = fWr.Position();
	fWr.Seek(info.opsPos - 4, SEEK_SET);
	Write32(oldPos - info.opsPos);
	fWr.Seek(oldPos, SEEK_SET);
}


void PictureWriterBinary::EnterStateChange()
{
	BeginChunk(B_PIC_ENTER_STATE_CHANGE);
}

void PictureWriterBinary::ExitStateChange()
{
	EndChunk();
}

void PictureWriterBinary::EnterFontState()
{
	BeginChunk(B_PIC_ENTER_FONT_STATE);
}

void PictureWriterBinary::ExitFontState()
{
	EndChunk();
}

void PictureWriterBinary::PushState()
{
	BeginChunk(B_PIC_PUSH_STATE);
	EndChunk();
}

void PictureWriterBinary::PopState()
{
	BeginChunk(B_PIC_POP_STATE);
	EndChunk();
}


// #pragma mark - State Absolute

void PictureWriterBinary::SetDrawingMode(drawing_mode mode)
{
	BeginChunk(B_PIC_SET_DRAWING_MODE);
	Write16(mode);
	EndChunk();
}

void PictureWriterBinary::SetLineMode(
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

void PictureWriterBinary::SetPenSize(float penSize)
{
	BeginChunk(B_PIC_SET_PEN_SIZE);
	WriteFloat(penSize);
	EndChunk();
}

void PictureWriterBinary::SetHighColor(const rgb_color& color)
{
	BeginChunk(B_PIC_SET_FORE_COLOR);
	WriteColor(color);
	EndChunk();
}

void PictureWriterBinary::SetLowColor(const rgb_color& color)
{
	BeginChunk(B_PIC_SET_BACK_COLOR);
	WriteColor(color);
	EndChunk();
}

void PictureWriterBinary::SetPattern(const ::pattern& pat)
{
	BeginChunk(B_PIC_SET_STIPLE_PATTERN);
	WritePattern(pat);
	EndChunk();
}

void PictureWriterBinary::SetBlendingMode(
	source_alpha srcAlpha, alpha_function alphaFunc
)
{
	BeginChunk(B_PIC_SET_BLENDING_MODE);
	Write16(srcAlpha);
	Write16(alphaFunc);
	EndChunk();
}

void PictureWriterBinary::SetFillRule(int32 fillRule)
{
	BeginChunk(B_PIC_SET_FILL_RULE);
	Write32(fillRule);
	EndChunk();
}


// #pragma mark - State Relative

void PictureWriterBinary::SetOrigin(const BPoint& point)
{
	BeginChunk(B_PIC_SET_ORIGIN);
	WritePoint(point);
	EndChunk();
}

void PictureWriterBinary::SetScale(float scale)
{
	BeginChunk(B_PIC_SET_SCALE);
	WriteFloat(scale);
	EndChunk();
}

void PictureWriterBinary::SetPenLocation(const BPoint& point)
{
	BeginChunk(B_PIC_SET_PEN_LOCATION);
	WritePoint(point);
	EndChunk();
}

void PictureWriterBinary::SetTransform(const BAffineTransform& transform)
{
	BeginChunk(B_PIC_SET_TRANSFORM);
	WriteTransform(transform);
	EndChunk();
}


// #pragma mark - Clipping

void PictureWriterBinary::SetClipping(const BRegion& region)
{
	BeginChunk(B_PIC_SET_CLIPPING_RECTS);
	WriteRectInt(region.FrameInt());
	for (int32 i = 0; i < region.CountRects(); i++) {
		WriteRectInt(region.RectAtInt(i));
	}
	EndChunk();
}

void PictureWriterBinary::ClearClipping()
{
	BeginChunk(B_PIC_CLEAR_CLIPPING_RECTS);
	EndChunk();
}

void PictureWriterBinary::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
{
	BeginChunk(B_PIC_CLIP_TO_PICTURE);
	Write32(pictureToken);
	WritePoint(origin);
	WriteBool(inverse);
	EndChunk();
}

void PictureWriterBinary::ClipToRect(const BRect& rect, bool inverse)
{
	BeginChunk(B_PIC_CLIP_TO_RECT);
	WriteBool(inverse);
	WriteRect(rect);
	EndChunk();
}

void PictureWriterBinary::ClipToShape(const BShape& shape, bool inverse)
{
	BeginChunk(B_PIC_CLIP_TO_SHAPE);
	WriteBool(inverse);
	WriteShape(shape);
	EndChunk();
}


// #pragma mark - Font

void PictureWriterBinary::SetFontFamily(const font_family family)
{
	BeginChunk(B_PIC_SET_FONT_FAMILY);
	WriteString(std::string_view(family, strlen(family) + 1));
	EndChunk();
}

void PictureWriterBinary::SetFontStyle(const font_style style)
{
	BeginChunk(B_PIC_SET_FONT_STYLE);
	WriteString(std::string_view(style, strlen(style) + 1));
	EndChunk();
}

void PictureWriterBinary::SetFontSpacing(int32 spacing)
{
	BeginChunk(B_PIC_SET_FONT_SPACING);
	Write32(spacing);
	EndChunk();
}

void PictureWriterBinary::SetFontSize(float size)
{
	BeginChunk(B_PIC_SET_FONT_SIZE);
	WriteFloat(size);
	EndChunk();
}

void PictureWriterBinary::SetFontRotation(float rotation)
{
	BeginChunk(B_PIC_SET_FONT_ROTATE);
	WriteFloat(rotation);
	EndChunk();
}

void PictureWriterBinary::SetFontEncoding(int32 encoding)
{
	BeginChunk(B_PIC_SET_FONT_ENCODING);
	Write32(encoding);
	EndChunk();
}

void PictureWriterBinary::PictureWriterBinary::SetFontFlags(int32 flags)
{
	BeginChunk(B_PIC_SET_FONT_FLAGS);
	Write32(flags);
	EndChunk();
}

void PictureWriterBinary::SetFontShear(float shear)
{
	BeginChunk(B_PIC_SET_FONT_SHEAR);
	WriteFloat(shear);
	EndChunk();
}

void PictureWriterBinary::SetFontBpp(int32 bpp)
{
	BeginChunk(B_PIC_SET_FONT_BPP);
	Write32(bpp);
	EndChunk();
}

void PictureWriterBinary::SetFontFace(int32 face)
{
	BeginChunk(B_PIC_SET_FONT_FACE);
	Write32(face);
	EndChunk();
}


// #pragma mark - State (delta)

void PictureWriterBinary::MovePenBy(float dx, float dy)
{
	BeginChunk(B_PIC_MOVE_PEN_BY);
	WriteFloat(dx);
	WriteFloat(dy);
	EndChunk();
}

void PictureWriterBinary::TranslateBy(double x, double y)
{
	BeginChunk(B_PIC_AFFINE_TRANSLATE);
	WriteDouble(x);
	WriteDouble(y);
	EndChunk();
}

void PictureWriterBinary::ScaleBy(double x, double y)
{
	BeginChunk(B_PIC_AFFINE_SCALE);
	WriteDouble(x);
	WriteDouble(y);
	EndChunk();
}

void PictureWriterBinary::RotateBy(double angleRadians)
{
	BeginChunk(B_PIC_AFFINE_ROTATE);
	WriteDouble(angleRadians);
	EndChunk();
}


// #pragma mark - Geometry

void PictureWriterBinary::StrokeLine(const BPoint& start, const BPoint& end)
{
	BeginChunk(B_PIC_STROKE_LINE);
	WritePoint(start);
	WritePoint(end);
	EndChunk();
}


void PictureWriterBinary::StrokeRect(const BRect& rect)
{
	BeginChunk(B_PIC_STROKE_RECT);
	WriteRect(rect);
	EndChunk();
}

void PictureWriterBinary::FillRect(const BRect& rect)
{
	BeginChunk(B_PIC_FILL_RECT);
	WriteRect(rect);
	EndChunk();
}

void PictureWriterBinary::StrokeRect(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_RECT_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}

void PictureWriterBinary::FillRect(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_RECT_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}


void PictureWriterBinary::StrokeRoundRect(const BRect& rect, const BPoint& radius)
{
	BeginChunk(B_PIC_STROKE_ROUND_RECT);
	WriteRect(rect);
	WritePoint(radius);
	EndChunk();
}

void PictureWriterBinary::FillRoundRect(const BRect& rect, const BPoint& radius)
{
	BeginChunk(B_PIC_FILL_ROUND_RECT);
	WriteRect(rect);
	WritePoint(radius);
	EndChunk();
}

void PictureWriterBinary::StrokeRoundRect(
	const BRect& rect, const BPoint& radius, const BGradient& gradient
)
{
	BeginChunk(B_PIC_STROKE_ROUND_RECT_GRADIENT);
	WriteRect(rect);
	WritePoint(radius);
	WriteGradient(gradient);
	EndChunk();
}

void PictureWriterBinary::FillRoundRect(
	const BRect& rect, const BPoint& radius, const BGradient& gradient
)
{
	BeginChunk(B_PIC_FILL_ROUND_RECT_GRADIENT);
	WriteRect(rect);
	WritePoint(radius);
	WriteGradient(gradient);
	EndChunk();
}


void PictureWriterBinary::StrokeBezier(const BPoint points[4])
{
	BeginChunk(B_PIC_STROKE_BEZIER);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	EndChunk();
}

void PictureWriterBinary::FillBezier(const BPoint points[4])
{
	BeginChunk(B_PIC_FILL_BEZIER);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	EndChunk();
}

void PictureWriterBinary::StrokeBezier(const BPoint points[4], const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_BEZIER_GRADIENT);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	WriteGradient(gradient);
	EndChunk();
}

void PictureWriterBinary::FillBezier(const BPoint points[4], const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_BEZIER_GRADIENT);
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	WriteGradient(gradient);
	EndChunk();
}


void PictureWriterBinary::StrokePolygon(int32 numPoints, const BPoint* points, bool isClosed)
{
	BeginChunk(B_PIC_STROKE_POLYGON);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	WriteBool(isClosed);
	EndChunk();
}

void PictureWriterBinary::FillPolygon(int32 numPoints, const BPoint* points)
{
	BeginChunk(B_PIC_FILL_POLYGON);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	EndChunk();
}

void PictureWriterBinary::StrokePolygon(int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient)
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

void PictureWriterBinary::FillPolygon(int32 numPoints, const BPoint* points, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_POLYGON_GRADIENT);
	Write32(numPoints);
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	WriteGradient(gradient);
	EndChunk();
}


void PictureWriterBinary::StrokeShape(const BShape& shape)
{
	BeginChunk(B_PIC_STROKE_SHAPE);
	WriteShape(shape);
	EndChunk();
}

void PictureWriterBinary::FillShape(const BShape& shape)
{
	BeginChunk(B_PIC_FILL_SHAPE);
	WriteShape(shape);
	EndChunk();
}

void PictureWriterBinary::StrokeShape(const BShape& shape, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_SHAPE_GRADIENT);
	WriteShape(shape);
	WriteGradient(gradient);
	EndChunk();
}

void PictureWriterBinary::FillShape(const BShape& shape, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_SHAPE_GRADIENT);
	WriteShape(shape);
	WriteGradient(gradient);
	EndChunk();
}


void PictureWriterBinary::StrokeArc(
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

void PictureWriterBinary::FillArc(
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

void PictureWriterBinary::StrokeArc(
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

void PictureWriterBinary::FillArc(
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


void PictureWriterBinary::StrokeEllipse(const BRect& rect)
{
	BeginChunk(B_PIC_STROKE_ELLIPSE);
	WriteRect(rect);
	EndChunk();
}

void PictureWriterBinary::FillEllipse(const BRect& rect)
{
	BeginChunk(B_PIC_FILL_ELLIPSE);
	WriteRect(rect);
	EndChunk();
}

void PictureWriterBinary::StrokeEllipse(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_STROKE_ELLIPSE_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}

void PictureWriterBinary::FillEllipse(const BRect& rect, const BGradient& gradient)
{
	BeginChunk(B_PIC_FILL_ELLIPSE_GRADIENT);
	WriteRect(rect);
	WriteGradient(gradient);
	EndChunk();
}


// #pragma mark - Draw

void PictureWriterBinary::DrawString(
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

void PictureWriterBinary::DrawString(
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


void PictureWriterBinary::DrawBitmap(
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

void PictureWriterBinary::DrawPicture(const BPoint& where, int32 token)
{
	BeginChunk(B_PIC_DRAW_PICTURE);
	WritePoint(where);
	Write32(token);
	EndChunk();
}


void PictureWriterBinary::BlendLayer(Layer* layer)
{
	RaiseUnimplemented();
}
