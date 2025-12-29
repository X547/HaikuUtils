#pragma once

#include "PictureVisitor.h"

#include <vector>
#include <string_view>

#include <DataIO.h>


class PictureWriterBinary final: public PictureVisitor {
private:
	struct PictureInfo {
		off_t pos {};
		int32 pictCnt {};
		off_t opsPos {};
		struct {
			bool pictures: 1;
			bool ops: 1;
		} isSet {};
	};

	BPositionIO &fWr;
	std::vector<PictureInfo> fPictureStack;
	std::vector<off_t> fChunkStack;

	void RaiseUnimplemented();
	void RaiseError();
	void Check(bool cond);
	void CheckStatus(status_t status);

	void BeginChunk(int16 op);
	void EndChunk();

	void Write8(int8 val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void Write16(int16 val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void Write32(int32 val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WriteBool(bool val) {Write8(val ? 1 : 0);}
	void WriteFloat(float val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WriteDouble(double val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WritePoint(const BPoint &val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WriteRect(const BRect &val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WriteRectInt(const clipping_rect &val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WriteTransform(const BAffineTransform& val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}
	void WritePattern(const pattern& val) {CheckStatus(fWr.WriteExactly(&val, sizeof(val)));}

	void WriteColor(const rgb_color &c);
	void WriteString(std::string_view str);
	void WriteShape(const BShape &shape);
	void WriteGradient(const BGradient &gradient);

public:
	PictureWriterBinary(BPositionIO &fWr);

	// Meta
	void			EnterPicture(int32 version, int32 endian) final;
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
	void			DrawLine(const BPoint& start, const BPoint& end, const DrawGeometryInfo &drawInfo) final;
	void			DrawRect(const BRect& rect, const DrawGeometryInfo &drawInfo) final;
	void			DrawRoundRect(const BRect& rect, const BPoint& radius, const DrawGeometryInfo &drawInfo) final;
	void			DrawBezier(const BPoint points[4], const DrawGeometryInfo &drawInfo) final;
	void			DrawPolygon(int32 numPoints,
								const BPoint* points, bool isClosed, const DrawGeometryInfo &drawInfo) final;
	void			DrawShape(const BShape& shape, const DrawGeometryInfo &drawInfo) final;
	void			DrawArc(const BPoint& center,
								const BPoint& radius,
								float startTheta,
								float arcTheta,
								const DrawGeometryInfo &drawInfo) final;
	void			DrawEllipse(const BRect& rect, const DrawGeometryInfo &drawInfo) final;

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
