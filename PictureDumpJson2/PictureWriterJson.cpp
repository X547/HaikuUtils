#include "PictureWriterJson.h"

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>


class PictureWriterJson::ShapeIterator final: public BShapeIterator {
private:
	PictureWriterJson &fBase;

public:
	virtual ~ShapeIterator() = default;

	ShapeIterator(PictureWriterJson &base): fBase(base) {}


	status_t IterateMoveTo(BPoint* point) final
	{
		fBase.fWr.StartObject();
		fBase.fWr.Key("MoveTo");
		fBase.WritePoint(*point);
		fBase.fWr.EndObject();
		return B_OK;
	}

	status_t IterateLineTo(int32 lineCount, BPoint* linePoints) final
	{
		fBase.fWr.StartObject();
		fBase.fWr.Key("LineTo");
		fBase.fWr.StartArray();
		for (int32 i = 0; i < lineCount; i++) {
			fBase.WritePoint(linePoints[i]);
		}
		fBase.fWr.EndArray();
		fBase.fWr.EndObject();
		return B_OK;
	}

	status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPoints) final
	{
		fBase.fWr.StartObject();
		fBase.fWr.Key("BezierTo");
		fBase.fWr.StartArray();
		for (int32 i = 0; i < 3*bezierCount; i++) {
			fBase.WritePoint(bezierPoints[i]);
		}
		fBase.fWr.EndArray();
		fBase.fWr.EndObject();
		return B_OK;
	}

	status_t IterateClose()
	{
		fBase.fWr.StartObject();
		fBase.fWr.Key("Close");
		fBase.fWr.StartObject();
		fBase.fWr.EndObject();
		fBase.fWr.EndObject();
		return B_OK;
	}

	status_t IterateArcTo(
		float& rx, float& ry,
		float& angle, bool largeArc,
		bool counterClockWise, BPoint& point
	)
	{
		fBase.fWr.StartObject();
		fBase.fWr.Key("ArcTo");
		fBase.fWr.StartObject();
		fBase.fWr.Key("rx"); fBase.fWr.Double(rx);
		fBase.fWr.Key("ry"); fBase.fWr.Double(ry);
		fBase.fWr.Key("angle"); fBase.fWr.Double(angle);
		fBase.fWr.Key("largeArc"); fBase.fWr.Bool(largeArc);
		fBase.fWr.Key("ccw"); fBase.fWr.Bool(counterClockWise);
		fBase.fWr.Key("point"); fBase.WritePoint(point);
		fBase.fWr.EndObject();
		fBase.fWr.EndObject();
		return B_OK;
	}
};


PictureWriterJson::PictureWriterJson(JsonWriter &wr):
	fWr(wr)
{
}


void PictureWriterJson::WriteColor(const rgb_color &c)
{
	char buf[64];
	sprintf(buf, "#%02x%02x%02x%02x",
		c.alpha,
		c.red,
		c.green,
		c.blue
	);
	fWr.String(buf);
}

void PictureWriterJson::WritePoint(const BPoint &pt)
{
	fWr.StartObject();
	fWr.Key("x"); fWr.Double(pt.x);
	fWr.Key("y"); fWr.Double(pt.y);
	fWr.EndObject();
}

void PictureWriterJson::WriteRect(const BRect &rc)
{
	fWr.StartObject();
	fWr.Key("left");   fWr.Double(rc.left);
	fWr.Key("top");    fWr.Double(rc.top);
	fWr.Key("right");  fWr.Double(rc.right);
	fWr.Key("bottom"); fWr.Double(rc.bottom);
	fWr.EndObject();
}

void PictureWriterJson::WriteShape(const BShape &shape)
{
	ShapeIterator iter(*this);
	fWr.StartArray();
	iter.Iterate(const_cast<BShape*>(&shape));
	fWr.EndArray();
}

void PictureWriterJson::WriteGradient(const BGradient &gradient)
{
	fWr.StartObject();
	switch (gradient.GetType()) {
		case BGradient::TYPE_LINEAR:
			fWr.Key("BGradientLinear");
			break;
		case BGradient::TYPE_RADIAL:
			fWr.Key("BGradientRadial");
			break;
		case BGradient::TYPE_RADIAL_FOCUS:
			fWr.Key("BGradientRadialFocus");
			break;
		case BGradient::TYPE_DIAMOND:
			fWr.Key("BGradientDiamond");
			break;
		case BGradient::TYPE_CONIC:
			fWr.Key("BGradientConic");
			break;
		default:
			fWr.Key("BGradient");
			break;
	}
	fWr.StartObject();
	fWr.Key("stops");
	fWr.StartArray();
	for (int32 i = 0; i < gradient.CountColorStops(); i++) {
		BGradient::ColorStop *cs = gradient.ColorStopAt(i);
		fWr.StartObject();
		fWr.Key("color"); WriteColor(cs->color);
		fWr.Key("offset"); fWr.Double(cs->offset);
		fWr.EndObject();
	}
	fWr.EndArray();
	switch (gradient.GetType()) {
	case BGradient::TYPE_LINEAR: {
		const BGradientLinear &grad = static_cast<const BGradientLinear &>(gradient);
		fWr.Key("start"); WritePoint(grad.Start());
		fWr.Key("end"); WritePoint(grad.End());
		break;
	}
	case BGradient::TYPE_RADIAL: {
		const BGradientRadial &grad = static_cast<const BGradientRadial &>(gradient);
		fWr.Key("center"); WritePoint(grad.Center());
		fWr.Key("radius"); fWr.Double(grad.Radius());
		break;
	}
	case BGradient::TYPE_RADIAL_FOCUS: {
		const BGradientRadialFocus &grad = static_cast<const BGradientRadialFocus &>(gradient);
		fWr.Key("center"); WritePoint(grad.Center());
		fWr.Key("focus"); WritePoint(grad.Focal());
		fWr.Key("radius"); fWr.Double(grad.Radius());
		break;
	}
	case BGradient::TYPE_DIAMOND: {
		const BGradientDiamond &grad = static_cast<const BGradientDiamond &>(gradient);
		fWr.Key("center"); WritePoint(grad.Center());
		break;
	}
	case BGradient::TYPE_CONIC: {
		const BGradientConic &grad = static_cast<const BGradientConic &>(gradient);
		fWr.Key("center"); WritePoint(grad.Center());
		fWr.Key("angle"); fWr.Double(grad.Angle());
		break;
	}
	case BGradient::TYPE_NONE:
		break;
	}
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::WriteTransform(const BAffineTransform& tr)
{
	fWr.StartObject();
	fWr.Key("tx");  fWr.Double(tr.tx);
	fWr.Key("ty");  fWr.Double(tr.ty);
	fWr.Key("sx");  fWr.Double(tr.sx);
	fWr.Key("sy");  fWr.Double(tr.sy);
	fWr.Key("shy"); fWr.Double(tr.shy);
	fWr.Key("shx"); fWr.Double(tr.shx);
	fWr.EndObject();
}



// #pragma mark - Meta

void PictureWriterJson::EnterPicture(int32 version, int32 endian)
{
	fWr.StartObject();
	fWr.Key("version"); fWr.Int(version);
	fWr.Key("endian"); fWr.Int(endian);
}

void PictureWriterJson::ExitPicture()
{
	fWr.EndObject();
}

void PictureWriterJson::EnterPictures(int32 count)
{
	fWr.Key("pictures");
	fWr.StartArray();
}

void PictureWriterJson::ExitPictures()
{
	fWr.EndArray();
}

void PictureWriterJson::EnterOps()
{
	fWr.Key("ops");
	fWr.StartArray();
}

void PictureWriterJson::ExitOps()
{
	fWr.EndArray();
}


void PictureWriterJson::EnterStateChange()
{
	fWr.StartObject();
	fWr.Key("ENTER_STATE_CHANGE");
	fWr.StartArray();
}

void PictureWriterJson::ExitStateChange()
{
	fWr.EndArray();
	fWr.EndObject();
}

void PictureWriterJson::EnterFontState()
{
	fWr.StartObject();
	fWr.Key("ENTER_FONT_STATE");
	fWr.StartArray();
}

void PictureWriterJson::ExitFontState()
{
	fWr.EndArray();
	fWr.EndObject();
}

void PictureWriterJson::PushState()
{
	fWr.StartObject();
	fWr.Key("GROUP");
	fWr.StartArray();
}

void PictureWriterJson::PopState()
{
	fWr.EndArray();
	fWr.EndObject();
}


// #pragma mark - State Absolute

void PictureWriterJson::SetDrawingMode(drawing_mode mode)
{
	fWr.StartObject();
	fWr.Key("SET_DRAWING_MODE");
	switch (mode) {
		case B_OP_COPY: fWr.String("B_OP_COPY"); break;
		case B_OP_OVER: fWr.String("B_OP_OVER"); break;
		case B_OP_ERASE: fWr.String("B_OP_ERASE"); break;
		case B_OP_INVERT: fWr.String("B_OP_INVERT"); break;
		case B_OP_ADD: fWr.String("B_OP_ADD"); break;
		case B_OP_SUBTRACT: fWr.String("B_OP_SUBTRACT"); break;
		case B_OP_BLEND: fWr.String("B_OP_BLEND"); break;
		case B_OP_MIN: fWr.String("B_OP_MIN"); break;
		case B_OP_MAX: fWr.String("B_OP_MAX"); break;
		case B_OP_SELECT: fWr.String("B_OP_SELECT"); break;
		case B_OP_ALPHA: fWr.String("B_OP_ALPHA"); break;
		default: fWr.Int(mode);
	}
	fWr.EndObject();
}

void PictureWriterJson::SetLineMode(cap_mode cap,
							join_mode join,
							float miterLimit)
{
	fWr.StartObject();
	fWr.Key("SET_LINE_MODE");
	fWr.StartObject();
	fWr.Key("capMode");
	switch (cap) {
		case B_ROUND_CAP: fWr.String("B_ROUND_CAP"); break;
		case B_BUTT_CAP: fWr.String("B_BUTT_CAP"); break;
		case B_SQUARE_CAP: fWr.String("B_SQUARE_CAP"); break;
		default: fWr.Int(cap);
	}
	fWr.Key("joinMode");
	switch (join) {
		case B_ROUND_JOIN: fWr.String("B_ROUND_JOIN"); break;
		case B_MITER_JOIN: fWr.String("B_MITER_JOIN"); break;
		case B_BEVEL_JOIN: fWr.String("B_BEVEL_JOIN"); break;
		case B_BUTT_JOIN: fWr.String("B_BUTT_JOIN"); break;
		case B_SQUARE_JOIN: fWr.String("B_SQUARE_JOIN"); break;
		default: fWr.Int(join);
	}
	fWr.Key("miterLimit"); fWr.Double(miterLimit);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::SetPenSize(float penSize)
{
	fWr.StartObject();
	fWr.Key("SET_PEN_SIZE");
	fWr.Double(penSize);
	fWr.EndObject();
}

void PictureWriterJson::SetHighColor(const rgb_color& color)
{
	fWr.StartObject();
	fWr.Key("SET_FORE_COLOR");
	WriteColor(color);
	fWr.EndObject();
}

void PictureWriterJson::SetLowColor(const rgb_color& color)
{
	fWr.StartObject();
	fWr.Key("SET_BACK_COLOR");
	WriteColor(color);
	fWr.EndObject();
}

void PictureWriterJson::SetPattern(const ::pattern& pat)
{
	fWr.StartObject();
	fWr.Key("SET_STIPLE_PATTERN");
	if (memcmp(&pat, &B_SOLID_HIGH, sizeof(pattern)) == 0)
		fWr.String("B_SOLID_HIGH");
	else if (memcmp(&pat, &B_SOLID_LOW, sizeof(pattern)) == 0)
		fWr.String("B_SOLID_LOW");
	else if (memcmp(&pat, &B_MIXED_COLORS, sizeof(pattern)) == 0)
		fWr.String("B_MIXED_COLORS");
	else {
		fWr.StartArray();
		for (int32 i = 0; i < 8; i++) {
			fWr.Int((uint8)pat.data[i]);
		}
		fWr.EndArray();
	}
	fWr.EndObject();
}

void PictureWriterJson::SetBlendingMode(source_alpha srcAlpha,
							alpha_function alphaFunc)
{
	fWr.StartObject();
	fWr.Key("SET_BLENDING_MODE");
	fWr.StartObject();
	fWr.Key("srcAlpha");
	switch (srcAlpha) {
		case B_PIXEL_ALPHA: fWr.String("B_PIXEL_ALPHA"); break;
		case B_CONSTANT_ALPHA: fWr.String("B_CONSTANT_ALPHA"); break;
		default: fWr.Int(srcAlpha);
	}
	fWr.Key("alphaFunc");
	switch (alphaFunc) {
		case B_ALPHA_OVERLAY: fWr.String("B_ALPHA_OVERLAY"); break;
		case B_ALPHA_COMPOSITE: fWr.String("B_ALPHA_COMPOSITE"); break;
		case B_ALPHA_COMPOSITE_SOURCE_IN: fWr.String("B_ALPHA_COMPOSITE_SOURCE_IN"); break;
		case B_ALPHA_COMPOSITE_SOURCE_OUT: fWr.String("B_ALPHA_COMPOSITE_SOURCE_OUT"); break;
		case B_ALPHA_COMPOSITE_SOURCE_ATOP: fWr.String("B_ALPHA_COMPOSITE_SOURCE_ATOP"); break;
		case B_ALPHA_COMPOSITE_DESTINATION_OVER: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_OVER"); break;
		case B_ALPHA_COMPOSITE_DESTINATION_IN: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_IN"); break;
		case B_ALPHA_COMPOSITE_DESTINATION_OUT: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_OUT"); break;
		case B_ALPHA_COMPOSITE_DESTINATION_ATOP: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_ATOP"); break;
		case B_ALPHA_COMPOSITE_XOR: fWr.String("B_ALPHA_COMPOSITE_XOR"); break;
		case B_ALPHA_COMPOSITE_CLEAR: fWr.String("B_ALPHA_COMPOSITE_CLEAR"); break;
		case B_ALPHA_COMPOSITE_DIFFERENCE: fWr.String("B_ALPHA_COMPOSITE_DIFFERENCE"); break;
		case B_ALPHA_COMPOSITE_LIGHTEN: fWr.String("B_ALPHA_COMPOSITE_LIGHTEN"); break;
		case B_ALPHA_COMPOSITE_DARKEN: fWr.String("B_ALPHA_COMPOSITE_DARKEN"); break;
		default: fWr.Int(alphaFunc);
	}
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::SetFillRule(int32 fillRule)
{
	fWr.StartObject();
	fWr.Key("SET_FILL_RULE");
	switch (fillRule) {
		case B_EVEN_ODD: fWr.String("B_EVEN_ODD"); break;
		case B_NONZERO: fWr.String("B_NONZERO"); break;
		default: fWr.Int(fillRule);
	}
	fWr.EndObject();
}


// #pragma mark - State Relative

void PictureWriterJson::SetOrigin(const BPoint& point)
{
	fWr.StartObject();
	fWr.Key("SET_ORIGIN");
	WritePoint(point);
	fWr.EndObject();
}

void PictureWriterJson::SetScale(float scale)
{
	fWr.StartObject();
	fWr.Key("SET_SCALE");
	fWr.Double(scale);
	fWr.EndObject();
}

void PictureWriterJson::SetPenLocation(const BPoint& point)
{
	fWr.StartObject();
	fWr.Key("SET_PEN_LOCATION");
	WritePoint(point);
	fWr.EndObject();
}

void PictureWriterJson::SetTransform(const BAffineTransform& transform)
{
	fWr.StartObject();
	fWr.Key("SET_TRANSFORM");
	WriteTransform(transform);
	fWr.EndObject();
}


// #pragma mark - Clipping

void PictureWriterJson::SetClipping(const BRegion& region)
{
	fWr.StartObject();
	fWr.Key("SET_CLIPPING_RECTS");
	fWr.StartArray();
	for (int32 i = 0; i < region.CountRects(); i++) {
		WriteRect(region.RectAt(i));
	}
	fWr.EndArray();
	fWr.EndObject();
}

void PictureWriterJson::ClearClipping()
{
	fWr.StartObject();
	fWr.Key("CLEAR_CLIPPING_RECTS");
	fWr.StartObject();
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
{
	fWr.StartObject();
	fWr.Key("CLIP_TO_PICTURE");
	fWr.StartObject();
	fWr.Key("token"); fWr.Int(pictureToken);
	fWr.Key("where"); WritePoint(origin);
	fWr.Key("inverse"); fWr.Bool(inverse);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::ClipToRect(const BRect& rect, bool inverse)
{
	fWr.StartObject();
	fWr.Key("CLIP_TO_RECT");
	fWr.StartObject();
	fWr.Key("inverse"); fWr.Bool(inverse);
	fWr.Key("rect"); WriteRect(rect);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::ClipToShape(const BShape& shape, bool inverse)
{
	fWr.StartObject();
	fWr.Key("CLIP_TO_SHAPE");
	fWr.StartObject();
	fWr.Key("inverse"); fWr.Bool(inverse);
	fWr.Key("shape"); WriteShape(shape);
	fWr.EndObject();
	fWr.EndObject();
}


// #pragma mark - Font

void PictureWriterJson::SetFontFamily(const font_family family)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_FAMILY");
	fWr.String(family, strlen(family) + 1);
	fWr.EndObject();
}

void PictureWriterJson::SetFontStyle(const font_style style)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_STYLE");
	fWr.String(style, strlen(style) + 1);
	fWr.EndObject();
}

void PictureWriterJson::SetFontSpacing(int32 spacing)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_SPACING");
	switch (spacing) {
		case B_CHAR_SPACING: fWr.String("B_CHAR_SPACING"); break;
		case B_STRING_SPACING: fWr.String("B_STRING_SPACING"); break;
		case B_BITMAP_SPACING: fWr.String("B_BITMAP_SPACING"); break;
		case B_FIXED_SPACING: fWr.String("B_FIXED_SPACING"); break;
		default: fWr.Int(spacing);
	}
	fWr.EndObject();
}

void PictureWriterJson::SetFontSize(float size)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_SIZE");
	fWr.Double(size);
	fWr.EndObject();
}

void PictureWriterJson::SetFontRotation(float rotation)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_ROTATE");
	fWr.Double(rotation);
	fWr.EndObject();
}

void PictureWriterJson::SetFontEncoding(int32 encoding)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_ENCODING");
	switch (encoding) {
		case B_UNICODE_UTF8: fWr.String("B_UNICODE_UTF8"); break;
		case B_ISO_8859_1: fWr.String("B_ISO_8859_1"); break;
		case B_ISO_8859_2: fWr.String("B_ISO_8859_2"); break;
		case B_ISO_8859_3: fWr.String("B_ISO_8859_3"); break;
		case B_ISO_8859_4: fWr.String("B_ISO_8859_4"); break;
		case B_ISO_8859_5: fWr.String("B_ISO_8859_5"); break;
		case B_ISO_8859_6: fWr.String("B_ISO_8859_6"); break;
		case B_ISO_8859_7: fWr.String("B_ISO_8859_7"); break;
		case B_ISO_8859_8: fWr.String("B_ISO_8859_8"); break;
		case B_ISO_8859_9: fWr.String("B_ISO_8859_9"); break;
		case B_ISO_8859_10: fWr.String("B_ISO_8859_10"); break;
		case B_MACINTOSH_ROMAN: fWr.String("B_MACINTOSH_ROMAN"); break;
		default: fWr.Int(encoding);
	}
	fWr.EndObject();
}

void PictureWriterJson::PictureWriterJson::SetFontFlags(int32 flags)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_FLAGS");
	fWr.StartArray();
	for (uint32 i = 0; i < 32; i++) {
		if ((1U << i) & (uint32)flags) {
			switch (i) {
				case 0: fWr.String("B_DISABLE_ANTIALIASING"); break;
				case 1: fWr.String("B_FORCE_ANTIALIASING"); break;
				default: fWr.Int(i);
			}
		}
	}
	fWr.EndArray();
	fWr.EndObject();
}

void PictureWriterJson::SetFontShear(float shear)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_SHEAR");
	fWr.Double(shear);
	fWr.EndObject();
}

void PictureWriterJson::SetFontBpp(int32 bpp)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_BPP");
	fWr.Int(bpp);
	fWr.EndObject();
}

void PictureWriterJson::SetFontFace(int32 face)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_FACE");
	fWr.StartArray();
	for (uint32 i = 0; i < 32; i++) {
		if ((1U << i) & (uint32)face) {
			switch (i) {
				case 0: fWr.String("B_ITALIC_FACE"); break;
				case 1: fWr.String("B_UNDERSCORE_FACE"); break;
				case 2: fWr.String("B_NEGATIVE_FACE"); break;
				case 3: fWr.String("B_OUTLINED_FACE"); break;
				case 4: fWr.String("B_STRIKEOUT_FACE"); break;
				case 5: fWr.String("B_BOLD_FACE"); break;
				case 6: fWr.String("B_REGULAR_FACE"); break;
				case 7: fWr.String("B_CONDENSED_FACE"); break;
				case 8: fWr.String("B_LIGHT_FACE"); break;
				case 9: fWr.String("B_HEAVY_FACE"); break;
				default: fWr.Int(i);
			}
		}
	}
	fWr.EndArray();
	fWr.EndObject();
}


// #pragma mark - State (delta)

void PictureWriterJson::MovePenBy(float dx, float dy)
{
	fWr.StartObject();
	fWr.Key("MOVE_PEN_BY");
	WritePoint(BPoint(dx, dy));
	fWr.EndObject();
}

void PictureWriterJson::TranslateBy(double x, double y)
{
	fWr.StartObject();
	fWr.Key("AFFINE_TRANSLATE");
	fWr.StartObject();
	fWr.Key("x"); fWr.Double(x);
	fWr.Key("y"); fWr.Double(y);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::ScaleBy(double x, double y)
{
	fWr.StartObject();
	fWr.Key("AFFINE_SCALE");
	fWr.StartObject();
	fWr.Key("x"); fWr.Double(x);
	fWr.Key("y"); fWr.Double(y);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::RotateBy(double angleRadians)
{
	fWr.StartObject();
	fWr.Key("AFFINE_ROTATE");
	fWr.Double(angleRadians);
	fWr.EndObject();
}


// #pragma mark - Geometry

void PictureWriterJson::DrawLine(const BPoint& start, const BPoint& end, const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	fWr.Key(drawInfo.gradient == NULL ? "STROKE_LINE" : "STROKE_LINE_GRADIENT");
	fWr.StartObject();
	fWr.Key("start"); WritePoint(start);
	fWr.Key("end"); WritePoint(end);
	if (drawInfo.gradient != NULL) {
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
	}
	fWr.EndObject();
	fWr.EndObject();
}


void PictureWriterJson::DrawRect(const BRect& rect, const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_RECT" : "FILL_RECT");
		WriteRect(rect);
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_RECT_GRADIENT" : "FILL_RECT_GRADIENT");
		fWr.StartObject();
		fWr.Key("rect"); WriteRect(rect);
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
		fWr.EndObject();
	}
	fWr.EndObject();
}

void PictureWriterJson::DrawRoundRect(const BRect& rect, const BPoint& radius, const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_ROUND_RECT" : "FILL_ROUND_RECT");
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_ROUND_RECT_GRADIENT" : "FILL_ROUND_RECT_GRADIENT");
	}
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("radius"); WritePoint(radius);
	if (drawInfo.gradient != NULL) {
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
	}
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::DrawBezier(const BPoint points[4], const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_BEZIER" : "FILL_BEZIER");
		fWr.StartArray();
		for (int32 i = 0; i < 4; i++) {
			WritePoint(points[i]);
		}
		fWr.EndArray();
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_BEZIER_GRADIENT" : "FILL_BEZIER_GRADIENT");
		fWr.StartObject();
		fWr.Key("points");
		fWr.StartArray();
		for (int32 i = 0; i < 4; i++) {
			WritePoint(points[i]);
		}
		fWr.EndArray();
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
		fWr.EndObject();
	}
	fWr.EndObject();
}

void PictureWriterJson::DrawPolygon(int32 numPoints, const BPoint* points, bool isClosed, const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_POLYGON" : "FILL_POLYGON");
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_POLYGON_GRADIENT" : "FILL_POLYGON_GRADIENT");
	}
	if (drawInfo.isStroke || drawInfo.gradient != NULL) {
		fWr.StartObject();
		fWr.Key("points");
		fWr.StartArray();
		for (int32 i = 0; i < numPoints; i++) {
			WritePoint(points[i]);
		}
		fWr.EndArray();
		if (drawInfo.isStroke) {
			fWr.Key("isClosed"); fWr.Bool(isClosed);
		}
		if (drawInfo.gradient != NULL) {
			fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
		}
		fWr.EndObject();
	} else {
		fWr.StartArray();
		for (int32 i = 0; i < numPoints; i++) {
			WritePoint(points[i]);
		}
		fWr.EndArray();
	}
	fWr.EndObject();
}

void PictureWriterJson::DrawShape(const BShape& shape, const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_SHAPE" : "FILL_SHAPE");
		WriteShape(shape);
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_SHAPE_GRADIENT" : "FILL_SHAPE_GRADIENT");
		fWr.StartObject();
		fWr.Key("shape"); WriteShape(shape);
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
		fWr.EndObject();
	}
	fWr.EndObject();
}

void PictureWriterJson::DrawArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const DrawGeometryInfo &drawInfo
)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_ARC" : "FILL_ARC");
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_ARC_GRADIENT" : "FILL_ARC_GRADIENT");
	}
	fWr.Key("STROKE_ARC");
	fWr.StartObject();
	fWr.Key("center"); WritePoint(center);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("startTheta"); fWr.Double(startTheta);
	fWr.Key("arcTheta"); fWr.Double(arcTheta);
	if (drawInfo.gradient != NULL) {
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
	}
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::DrawEllipse(const BRect& rect, const DrawGeometryInfo &drawInfo)
{
	fWr.StartObject();
	if (drawInfo.gradient == NULL) {
		fWr.Key(drawInfo.isStroke ? "STROKE_ELLIPSE" : "FILL_ELLIPSE");
		WriteRect(rect);
	} else {
		fWr.Key(drawInfo.isStroke ? "STROKE_ELLIPSE_GRADIENT" : "FILL_ELLIPSE_GRADIENT");
		fWr.StartObject();
		fWr.Key("rect"); WriteRect(rect);
		fWr.Key("gradient"); WriteGradient(*drawInfo.gradient);
		fWr.EndObject();
	}
	fWr.EndObject();
}


// #pragma mark - Draw

void PictureWriterJson::DrawString(
							const char* string, int32 length,
							const escapement_delta& delta)
{
	fWr.StartObject();
	fWr.Key("DRAW_STRING");
	fWr.StartObject();
	fWr.Key("string"); fWr.String(string, length);
	// TODO: check order
	fWr.Key("escapementSpace"); fWr.Double(delta.space);
	fWr.Key("escapementNonSpace"); fWr.Double(delta.nonspace);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::DrawString(const char* string,
							int32 length, const BPoint* locations,
							int32 locationCount)
{
	fWr.StartObject();
	fWr.Key("DRAW_STRING_LOCATIONS");
	fWr.StartObject();
	fWr.Key("locations");
	fWr.StartArray();
	for (int32 i = 0; i < locationCount; i++) {
		WritePoint(locations[i]);
	}
	fWr.EndArray();
	fWr.Key("string"); fWr.String(string, length);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureWriterJson::DrawBitmap(const BRect& srcRect,
							const BRect& dstRect, int32 width,
							int32 height,
							int32 bytesPerRow,
							int32 colorSpace,
							int32 flags,
							const void* data, int32 length)
{
	fWr.StartObject();
	fWr.Key("DRAW_PIXELS");
	fWr.StartObject();
	fWr.Key("sourceRect"); WriteRect(srcRect);
	fWr.Key("destinationRect"); WriteRect(dstRect);
	fWr.Key("width"); fWr.Int(width);
	fWr.Key("height"); fWr.Int(height);
	fWr.Key("bytesPerRow"); fWr.Int(bytesPerRow);
	fWr.Key("colorSpace"); fWr.Int(colorSpace);
	fWr.Key("flags"); fWr.Int(flags);
	fWr.Key("data");
	fWr.StartArray();
	for (int32 i = 0; i < length; i++) {
		fWr.Int(((uint8*)data)[i]);
	}
	fWr.EndArray();
	fWr.EndObject();
	fWr.EndObject();
}

void PictureWriterJson::DrawPicture(const BPoint& where,
							int32 token)
{
	fWr.StartObject();
	fWr.Key("DRAW_PICTURE");
	fWr.StartObject();
	fWr.Key("where"); WritePoint(where);
	fWr.Key("token"); fWr.Int(token);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureWriterJson::BlendLayer(Layer* layer)
{
	fWr.StartObject();
	fWr.Key("BLEND_LAYER");
	fWr.StartObject();
	// TODO: implement
	fWr.EndObject();
	fWr.EndObject();
}
