#pragma once

#include <View.h>
#include <Region.h>
#include <Shape.h>
#include <Gradient.h>
#include <AffineTransform.h>


class Layer;

class PictureVisitor {
public:
	// Meta
	virtual void			EnterStateChange() {}
	virtual void			ExitStateChange() {}
	virtual void			EnterFontState() {}
	virtual void			ExitFontState() {}
	virtual void			PushState() {}
	virtual void			PopState() {}

	// State Absolute
	virtual void			SetDrawingMode(drawing_mode mode) {}
	virtual void			SetLineMode(cap_mode cap,
								join_mode join,
								float miterLimit) {}
	virtual void			SetPenSize(float penSize) {}
	virtual void			SetHighColor(const rgb_color& color) {}
	virtual void			SetLowColor(const rgb_color& color) {}
	virtual void			SetPattern(const ::pattern& pattern) {}
	virtual void			SetBlendingMode(source_alpha srcAlpha,
								alpha_function alphaFunc) {}
	virtual void			SetFillRule(int32 fillRule) {}

	// State Relative
	virtual void			SetOrigin(const BPoint& point) {}
	virtual void			SetScale(float scale) {}
	virtual void			SetPenLocation(const BPoint& point) {}
	virtual void			SetTransform(const BAffineTransform& transform) {}

	// Clipping
	virtual void			SetClipping(const BRegion& region) {}
	virtual void			ClearClipping() {}
	virtual void			ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse) {}
	virtual void			ClipToRect(const BRect& rect, bool inverse) {}
	virtual void			ClipToShape(const BShape& shape, bool inverse) {}

	// Font
	virtual void			SetFontFamily(const font_family family) {}
	virtual void			SetFontStyle(const font_style style) {}
	virtual void			SetFontSpacing(int32 spacing) {}
	virtual void			SetFontSize(float size) {}
	virtual void			SetFontRotation(float rotation) {}
	virtual void			SetFontEncoding(int32 encoding) {}
	virtual void			SetFontFlags(int32 flags) {}
	virtual void			SetFontShear(float shear) {}
	virtual void			SetFontBpp(int32 bpp) {}
	virtual void			SetFontFace(int32 face) {}

	// State (delta)
	virtual void			MovePenBy(float dx, float dy) {}
	virtual void			TranslateBy(double x, double y) {}
	virtual void			ScaleBy(double x, double y) {}
	virtual void			RotateBy(double angleRadians) {}

	// Geometry
	virtual void			StrokeLine(const BPoint& start, const BPoint& end) {}

	virtual void			StrokeRect(const BRect& rect) {}
	virtual void			FillRect(const BRect& rect) {}
	virtual void			StrokeRect(const BRect& rect, const BGradient& gradient) {}
	virtual void			FillRect(const BRect& rect, const BGradient& gradient) {}

	virtual void			StrokeRoundRect(const BRect& rect, const BPoint& radius) {}
	virtual void			FillRoundRect(const BRect& rect,
								const BPoint& radius) {}
	virtual void			StrokeRoundRect(const BRect& rect,
								const BPoint& radius, const BGradient& gradient) {}
	virtual void			FillRoundRect(const BRect& rect,
								const BPoint& radius, const BGradient& gradient) {}

	virtual void			StrokeBezier(const BPoint points[4]) {}
	virtual void			FillBezier(const BPoint points[4]) {}
	virtual void			StrokeBezier(const BPoint points[4], const BGradient& gradient) {}
	virtual void			FillBezier(const BPoint points[4], const BGradient& gradient) {}

	virtual void			StrokePolygon(int32 numPoints,
								const BPoint* points, bool isClosed) {}
	virtual void			FillPolygon(int32 numPoints,
								const BPoint* points) {}
	virtual void			StrokePolygon(int32 numPoints,
								const BPoint* points, bool isClosed, const BGradient& gradient) {}
	virtual void			FillPolygon(int32 numPoints,
								const BPoint* points, const BGradient& gradient) {}

	virtual void			StrokeShape(const BShape& shape) {}
	virtual void			FillShape(const BShape& shape) {}
	virtual void			StrokeShape(const BShape& shape, const BGradient& gradient) {}
	virtual void			FillShape(const BShape& shape, const BGradient& gradient) {}

	virtual void			StrokeArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta) {}
	virtual void			FillArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta) {}
	virtual void			StrokeArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta,
								const BGradient& gradient) {}
	virtual void			FillArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta,
								const BGradient& gradient) {}

	virtual void			StrokeEllipse(const BRect& rect) {}
	virtual void			FillEllipse(const BRect& rect) {}
	virtual void			StrokeEllipse(const BRect& rect, const BGradient& gradient) {}
	virtual void			FillEllipse(const BRect& rect, const BGradient& gradient) {}

	// Draw
	virtual void			DrawString(const char* string, int32 length,
								const escapement_delta& delta) {}
	virtual void			DrawString(const char* string,
								int32 length, const BPoint* locations,
								int32 locationCount) {}

	virtual void			DrawBitmap(const BRect& srcRect,
								const BRect& dstRect, int32 width,
								int32 height,
								int32 bytesPerRow,
								int32 colorSpace,
								int32 flags,
								const void* data, int32 length) {}

	virtual void			DrawPicture(const BPoint& where,
								int32 token) {}

	virtual void			BlendLayer(Layer* layer) {}
};
