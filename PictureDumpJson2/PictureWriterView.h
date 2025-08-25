#pragma once

#include "PictureVisitor.h"

#include <vector>
#include <string_view>

#include <DataIO.h>


class PictureWriterView final: public PictureVisitor {
private:
	BView &fView;

	::pattern fPattern;

	void RaiseUnimplemented();
	void RaiseError();
	void Check(bool cond);
	void CheckStatus(status_t status);

	void BeginChunk(int16 op);
	void EndChunk();

public:
	PictureWriterView(BView &fView);

	// Meta
	void			EnterPicture(int32 version, int32 unknown) final;
	void			ExitPicture() final;
	void			EnterPictures(int32 count) final;
	void			ExitPictures() final;
	void			EnterOps() final;
	void			ExitOps() final;

	void			EnterStateChange() final;
	void			ExitStateChange() final;
	void			EnterFontState() final;
	void			ExitFontState() final;
	void			PushState() final;
	void			PopState() final;

	// State Absolute
	void			SetDrawingMode(drawing_mode mode) final;
	void			SetLineMode(cap_mode cap,
								join_mode join,
								float miterLimit) final;
	void			SetPenSize(float penSize) final;
	void			SetHighColor(const rgb_color& color) final;
	void			SetLowColor(const rgb_color& color) final;
	void			SetPattern(const ::pattern& pattern) final;
	void			SetBlendingMode(source_alpha srcAlpha,
								alpha_function alphaFunc) final;
	void			SetFillRule(int32 fillRule) final;

	// State Relative
	void			SetOrigin(const BPoint& point) final;
	void			SetScale(float scale) final;
	void			SetPenLocation(const BPoint& point) final;
	void			SetTransform(const BAffineTransform& transform) final;

	// Clipping
	void			SetClipping(const BRegion& region) final;
	void			ClearClipping() final;
	void			ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse) final;
	void			ClipToRect(const BRect& rect, bool inverse) final;
	void			ClipToShape(const BShape& shape, bool inverse) final;

	// Font
	void			SetFontFamily(const font_family family) final;
	void			SetFontStyle(const font_style style) final;
	void			SetFontSpacing(int32 spacing) final;
	void			SetFontSize(float size) final;
	void			SetFontRotation(float rotation) final;
	void			SetFontEncoding(int32 encoding) final;
	void			SetFontFlags(int32 flags) final;
	void			SetFontShear(float shear) final;
	void			SetFontBpp(int32 bpp) final;
	void			SetFontFace(int32 face) final;

	// State (delta)
	void			MovePenBy(float dx, float dy) final;
	void			TranslateBy(double x, double y) final;
	void			ScaleBy(double x, double y) final;
	void			RotateBy(double angleRadians) final;

	// Geometry
	void			StrokeLine(const BPoint& start, const BPoint& end) final;

	void			StrokeRect(const BRect& rect) final;
	void			FillRect(const BRect& rect) final;
	void			StrokeRect(const BRect& rect, const BGradient& gradient) final;
	void			FillRect(const BRect& rect, const BGradient& gradient) final;

	void			StrokeRoundRect(const BRect& rect,
								const BPoint& radius) final;
	void			FillRoundRect(const BRect& rect,
								const BPoint& radius) final;
	void			StrokeRoundRect(const BRect& rect,
								const BPoint& radius, const BGradient& gradient) final;
	void			FillRoundRect(const BRect& rect,
								const BPoint& radius, const BGradient& gradient) final;

	void			StrokeBezier(const BPoint points[4]) final;
	void			FillBezier(const BPoint points[4]) final;
	void			StrokeBezier(const BPoint points[4], const BGradient& gradient) final;
	void			FillBezier(const BPoint points[4], const BGradient& gradient) final;

	void			StrokePolygon(int32 numPoints,
								const BPoint* points, bool isClosed) final;
	void			FillPolygon(int32 numPoints,
								const BPoint* points) final;
	void			StrokePolygon(int32 numPoints,
								const BPoint* points, bool isClosed, const BGradient& gradient) final;
	void			FillPolygon(int32 numPoints,
								const BPoint* points, const BGradient& gradient) final;

	void			StrokeShape(const BShape& shape) final;
	void			FillShape(const BShape& shape) final;
	void			StrokeShape(const BShape& shape, const BGradient& gradient) final;
	void			FillShape(const BShape& shape, const BGradient& gradient) final;

	void			StrokeArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta) final;
	void			FillArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta) final;
	void			StrokeArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta,
								const BGradient& gradient) final;
	void			FillArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta,
								const BGradient& gradient) final;

	void			StrokeEllipse(const BRect& rect) final;
	void			FillEllipse(const BRect& rect) final;
	void			StrokeEllipse(const BRect& rect, const BGradient& gradient) final;
	void			FillEllipse(const BRect& rect, const BGradient& gradient) final;

	// Draw
	void			DrawString(const char* string, int32 length,
								const escapement_delta& delta) final;
	void			DrawString(const char* string,
								int32 length, const BPoint* locations,
								int32 locationCount) final;

	void			DrawBitmap(const BRect& srcRect,
								const BRect& dstRect, int32 width,
								int32 height,
								int32 bytesPerRow,
								int32 colorSpace,
								int32 flags,
								const void* data, int32 length) final;

	void			DrawPicture(const BPoint& where,
								int32 token) final;

	void			BlendLayer(Layer* layer) final;
};
