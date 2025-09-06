#include "PictureWriterView.h"

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>

#include <View.h>
#include <Bitmap.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>

#include <private/interface/ShapePrivate.h>

#include "PictureOpcodes.h"


PictureWriterView::PictureWriterView(BView &view):
	fView(view)
{
}


void PictureWriterView::RaiseUnimplemented()
{
	fprintf(stderr, "[!] unimplemented\n");
	abort();
}

void PictureWriterView::RaiseError()
{
	fprintf(stderr, "[!] error\n");
	abort();
}

void PictureWriterView::Check(bool cond)
{
	if (!cond) {
		RaiseError();
	}
}

void PictureWriterView::CheckStatus(status_t status)
{
	if (status < B_OK) {
		RaiseError();
	}
}


// #pragma mark - Meta

void PictureWriterView::EnterPicture(int32 version, int32 unknown)
{
}

void PictureWriterView::ExitPicture()
{
}

void PictureWriterView::EnterPictures(int32 count)
{
	RaiseUnimplemented();
}

void PictureWriterView::ExitPictures()
{
	RaiseUnimplemented();
}

void PictureWriterView::EnterOps()
{
}

void PictureWriterView::ExitOps()
{
}


void PictureWriterView::EnterStateChange()
{
}

void PictureWriterView::ExitStateChange()
{
}

void PictureWriterView::EnterFontState()
{
}

void PictureWriterView::ExitFontState()
{
}

void PictureWriterView::PushState()
{
	fView.PushState();
}

void PictureWriterView::PopState()
{
	fView.PopState();
}


// #pragma mark - State Absolute

void PictureWriterView::SetDrawingMode(drawing_mode mode)
{
	fView.SetDrawingMode(mode);
}

void PictureWriterView::SetLineMode(
	cap_mode cap,
	join_mode join,
	float miterLimit
)
{
	fView.SetLineMode(cap, join, miterLimit);
}

void PictureWriterView::SetPenSize(float penSize)
{
	fView.SetPenSize(penSize);
}

void PictureWriterView::SetHighColor(const rgb_color& color)
{
	fView.SetHighColor(color);
}

void PictureWriterView::SetLowColor(const rgb_color& color)
{
	fView.SetLowColor(color);
}

void PictureWriterView::SetPattern(const ::pattern& pat)
{
	fPattern = pat;
}

void PictureWriterView::SetBlendingMode(
	source_alpha srcAlpha, alpha_function alphaFunc
)
{
	fView.SetBlendingMode(srcAlpha, alphaFunc);
}

void PictureWriterView::SetFillRule(int32 fillRule)
{
	fView.SetFillRule(fillRule);
}


// #pragma mark - State Relative

void PictureWriterView::SetOrigin(const BPoint& point)
{
	fView.SetOrigin(point);
}

void PictureWriterView::SetScale(float scale)
{
	fView.SetScale(scale);
}

void PictureWriterView::SetPenLocation(const BPoint& point)
{
	fView.MovePenTo(point);
}

void PictureWriterView::SetTransform(const BAffineTransform& transform)
{
	fView.SetTransform(transform);
}


// #pragma mark - Clipping

void PictureWriterView::SetClipping(const BRegion& region)
{
	fView.ConstrainClippingRegion(const_cast<BRegion*>(&region));
}

void PictureWriterView::ClearClipping()
{
	fView.ConstrainClippingRegion(NULL);
}

void PictureWriterView::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
{
	RaiseUnimplemented();
}

void PictureWriterView::ClipToRect(const BRect& rect, bool inverse)
{
	if (inverse) {
		fView.ClipToRect(rect);
	} else {
		fView.ClipToInverseRect(rect);
	}
}

void PictureWriterView::ClipToShape(const BShape& shape, bool inverse)
{
	if (inverse) {
		fView.ClipToShape(const_cast<BShape*>(&shape));
	} else {
		fView.ClipToInverseShape(const_cast<BShape*>(&shape));
	}
}


// #pragma mark - Font

void PictureWriterView::SetFontFamily(const font_family family)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontStyle(const font_style style)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontSpacing(int32 spacing)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontSize(float size)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontRotation(float rotation)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontEncoding(int32 encoding)
{
	RaiseUnimplemented();
}

void PictureWriterView::PictureWriterView::SetFontFlags(int32 flags)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontShear(float shear)
{
	RaiseUnimplemented();
}

void PictureWriterView::SetFontBpp(int32 bpp)
{
	// not supported
}

void PictureWriterView::SetFontFace(int32 face)
{
	RaiseUnimplemented();
}


// #pragma mark - State (delta)

void PictureWriterView::MovePenBy(float dx, float dy)
{
	fView.MovePenBy(dx, dy);
}

void PictureWriterView::TranslateBy(double x, double y)
{
	fView.TranslateBy(x, y);
}

void PictureWriterView::ScaleBy(double x, double y)
{
	fView.ScaleBy(x, y);
}

void PictureWriterView::RotateBy(double angleRadians)
{
	fView.RotateBy(angleRadians);
}


// #pragma mark - Geometry

void PictureWriterView::DrawLine(const BPoint& start, const BPoint& end, const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		fView.StrokeLine(start, end, fPattern);
	} else {
		fView.StrokeLine(start, end, *drawInfo.gradient);
	}
}

void PictureWriterView::DrawRect(const BRect& rect, const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokeRect(rect, fPattern);
		} else {
			fView.FillRect(rect, fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokeRect(rect, *drawInfo.gradient);
		} else {
			fView.FillRect(rect, *drawInfo.gradient);
		}
	}
}

void PictureWriterView::DrawRoundRect(const BRect& rect, const BPoint& radius, const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokeRoundRect(rect, radius.x, radius.y, fPattern);
		} else {
			fView.FillRoundRect(rect, radius.x, radius.y, fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokeRoundRect(rect, radius.x, radius.y, *drawInfo.gradient);
		} else {
			fView.FillRoundRect(rect, radius.x, radius.y, *drawInfo.gradient);
		}
	}
}

void PictureWriterView::DrawBezier(const BPoint points[4], const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokeBezier(const_cast<BPoint*>(points), fPattern);
		} else {
			fView.FillBezier(const_cast<BPoint*>(points), fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokeBezier(const_cast<BPoint*>(points), *drawInfo.gradient);
		} else {
			fView.FillBezier(const_cast<BPoint*>(points), *drawInfo.gradient);
		}
	}
}

void PictureWriterView::DrawPolygon(int32 numPoints, const BPoint* points, bool isClosed, const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokePolygon(points, numPoints, isClosed, fPattern);
		} else {
			fView.FillPolygon(points, numPoints, fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokePolygon(points, numPoints, isClosed, *drawInfo.gradient);
		} else {
			fView.FillPolygon(points, numPoints, *drawInfo.gradient);
		}
	}
}

void PictureWriterView::DrawShape(const BShape& shape, const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokeShape(const_cast<BShape*>(&shape), fPattern);
		} else {
			fView.FillShape(const_cast<BShape*>(&shape), fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokeShape(const_cast<BShape*>(&shape), *drawInfo.gradient);
		} else {
			fView.FillShape(const_cast<BShape*>(&shape), *drawInfo.gradient);
		}
	}
}

void PictureWriterView::DrawArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const DrawGeometryInfo &drawInfo
)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokeArc(center, radius.x, radius.y, startTheta, arcTheta, fPattern);
		} else {
			fView.FillArc(center, radius.x, radius.y, startTheta, arcTheta, fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokeArc(center, radius.x, radius.y, startTheta, arcTheta, *drawInfo.gradient);
		} else {
			fView.FillArc(center, radius.x, radius.y, startTheta, arcTheta, *drawInfo.gradient);
		}
	}
}

void PictureWriterView::DrawEllipse(const BRect& rect, const DrawGeometryInfo &drawInfo)
{
	if (drawInfo.gradient == NULL) {
		if (drawInfo.isStroke) {
			fView.StrokeEllipse(rect, fPattern);
		} else {
			fView.FillEllipse(rect, fPattern);
		}
	} else {
		if (drawInfo.isStroke) {
			fView.StrokeEllipse(rect, *drawInfo.gradient);
		} else {
			fView.FillEllipse(rect, *drawInfo.gradient);
		}
	}
}


// #pragma mark - Draw

void PictureWriterView::DrawString(
	const char* string, int32 length,
	const escapement_delta& delta
)
{
	fView.DrawString(string, length, const_cast<escapement_delta*>(&delta));
}

void PictureWriterView::DrawString(
	const char* string, int32 length,
	const BPoint* locations, int32 locationCount
)
{
	fView.DrawString(string, length, locations, locationCount);
}


void PictureWriterView::DrawBitmap(
	const BRect& srcRect,
	const BRect& dstRect, int32 width,
	int32 height,
	int32 bytesPerRow,
	int32 colorSpace,
	int32 flags,
	const void* data, int32 length
)
{
	BBitmap bitmap(BRect(0, 0, width - 1, height - 1), 0, (color_space)colorSpace);
	uint8 *dstBits = (uint8*)bitmap.Bits();
	const uint8 *srcBits = (const uint8*)data;
	size_t copyLen = std::min(bytesPerRow, bitmap.BytesPerRow());
	for (int32 y = 0; y < height; y++) {
		memcpy(dstBits, srcBits, copyLen);
		dstBits += bitmap.BytesPerRow();
		srcBits += bytesPerRow;
	}
	fView.DrawBitmap(&bitmap, dstRect, srcRect, flags);
}

void PictureWriterView::DrawPicture(const BPoint& where, int32 token)
{
	RaiseUnimplemented();
}


void PictureWriterView::BlendLayer(Layer* layer)
{
	RaiseUnimplemented();
}
