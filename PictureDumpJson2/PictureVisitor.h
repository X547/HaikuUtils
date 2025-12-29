#pragma once

#include <View.h>
#include <Region.h>
#include <Shape.h>
#include <Gradient.h>
#include <AffineTransform.h>


class Layer;

struct DrawGeometryInfo {
	bool isStroke;
	const BGradient *gradient;
};

class PictureVisitor {
public:
	// Meta
	virtual void			EnterPicture(int32 version, int32 endian) {}
	virtual void			ExitPicture() {}
	virtual void			EnterPictures(int32 count) {}
	virtual void			ExitPictures() {}
	virtual void			EnterOps() {}
	virtual void			ExitOps() {}

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
	virtual void			DrawLine(const BPoint& start, const BPoint& end, const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawRect(const BRect& rect, const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawRoundRect(const BRect& rect, const BPoint& radius, const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawBezier(const BPoint points[4], const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawPolygon(int32 numPoints, const BPoint* points, bool isClosed, const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawShape(const BShape& shape, const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta,
								const DrawGeometryInfo &drawInfo) {}
	virtual void			DrawEllipse(const BRect& rect, const DrawGeometryInfo &drawInfo) {}

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
