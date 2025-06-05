#include "PictureVisitorJson.h"

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>


class PictureVisitorJson::ShapeIterator final: public BShapeIterator {
private:
	PictureVisitorJson &fBase;

public:
	virtual ~ShapeIterator() = default;

	ShapeIterator(PictureVisitorJson &base): fBase(base) {}


	status_t IterateMoveTo(BPoint* point) final
	{
		fBase.fWr.Key("MoveTo");
		fBase.WritePoint(*point);
		return B_OK;
	}

	status_t IterateLineTo(int32 lineCount, BPoint* linePoints) final
	{
		fBase.fWr.Key("LineTo");
		fBase.fWr.StartArray();
		for (int32 i = 0; i < lineCount; i++) {
			fBase.WritePoint(linePoints[i]);
		}
		fBase.fWr.EndArray();
		return B_OK;
	}

	status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPoints) final
	{
		fBase.fWr.Key("BezierTo");
		fBase.fWr.StartArray();
		for (int32 i = 0; i < 3*bezierCount; i++) {
			fBase.WritePoint(bezierPoints[i]);
		}
		fBase.fWr.EndArray();
		return B_OK;
	}

	status_t IterateClose()
	{
		fBase.fWr.Key("Close");
		fBase.fWr.StartObject();
		fBase.fWr.EndObject();
		return B_OK;
	}

	status_t IterateArcTo(
		float& rx, float& ry,
		float& angle, bool largeArc,
		bool counterClockWise, BPoint& point
	)
	{
		fBase.fWr.Key("ArcTo");
		fBase.fWr.StartObject();
		fBase.fWr.Key("rx"); fBase.fWr.Double(rx);
		fBase.fWr.Key("ry"); fBase.fWr.Double(ry);
		fBase.fWr.Key("angle"); fBase.fWr.Double(angle);
		fBase.fWr.Key("largeArc"); fBase.fWr.Bool(largeArc);
		fBase.fWr.Key("ccw"); fBase.fWr.Bool(counterClockWise);
		fBase.fWr.Key("point"); fBase.WritePoint(point);
		fBase.fWr.EndObject();
		return B_OK;
	}
};


PictureVisitorJson::PictureVisitorJson(JsonWriter &wr):
	fWr(wr)
{
}


void PictureVisitorJson::WriteColor(const rgb_color &c)
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

void PictureVisitorJson::WritePoint(const BPoint &pt)
{
	fWr.StartObject();
	fWr.Key("x"); fWr.Double(pt.x);
	fWr.Key("y"); fWr.Double(pt.y);
	fWr.EndObject();
}

void PictureVisitorJson::WriteRect(const BRect &rc)
{
	fWr.StartObject();
	fWr.Key("left");   fWr.Double(rc.left);
	fWr.Key("top");    fWr.Double(rc.top);
	fWr.Key("right");  fWr.Double(rc.right);
	fWr.Key("bottom"); fWr.Double(rc.bottom);
	fWr.EndObject();
}

void PictureVisitorJson::WriteShape(const BShape &shape)
{
	ShapeIterator iter(*this);
	iter.Iterate(const_cast<BShape*>(&shape));
}

void PictureVisitorJson::WriteGradient(const BGradient &gradient)
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
		const BGradientRadialFocus &grad = static_cast<const BGradientRadialFocus &>(gradient);
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

void PictureVisitorJson::WriteTransform(const BAffineTransform& tr)
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

void PictureVisitorJson::EnterStateChange()
{
	fWr.StartObject();
	fWr.Key("ENTER_STATE_CHANGE");
	fWr.StartArray();
}

void PictureVisitorJson::ExitStateChange()
{
	fWr.EndArray();
	fWr.EndObject();
}

void PictureVisitorJson::EnterFontState()
{
	fWr.Key("ENTER_FONT_STATE");
	fWr.StartArray();
}

void PictureVisitorJson::ExitFontState()
{
	fWr.EndArray();
}

void PictureVisitorJson::PushState()
{
	fWr.StartObject();
	fWr.Key("GROUP");
	fWr.StartArray();
}

void PictureVisitorJson::PopState()
{
	fWr.EndArray();
	fWr.EndObject();
}


// #pragma mark - State Absolute

void PictureVisitorJson::SetDrawingMode(drawing_mode mode)
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

void PictureVisitorJson::SetLineMode(cap_mode cap,
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

void PictureVisitorJson::SetPenSize(float penSize)
{
	fWr.StartObject();
	fWr.Key("SET_PEN_SIZE");
	fWr.Double(penSize);
	fWr.EndObject();
}

void PictureVisitorJson::SetHighColor(const rgb_color& color)
{
	fWr.StartObject();
	fWr.Key("SET_FORE_COLOR");
	WriteColor(color);
	fWr.EndObject();
}

void PictureVisitorJson::SetLowColor(const rgb_color& color)
{
	fWr.StartObject();
	fWr.Key("SET_BACK_COLOR");
	WriteColor(color);
	fWr.EndObject();
}

void PictureVisitorJson::SetPattern(const ::pattern& pat)
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

void PictureVisitorJson::SetBlendingMode(source_alpha srcAlpha,
							alpha_function alphaFunc)
{
	fWr.StartObject();
	fWr.Key("SET_BLENDING_MODE");
	fWr.StartObject();
	fWr.Key("srcAlpha");
	switch (srcAlpha) {
		case 0: fWr.String("B_PIXEL_ALPHA"); break;
		case 1: fWr.String("B_CONSTANT_ALPHA"); break;
		default: fWr.Int(srcAlpha);
	}
	fWr.Key("alphaFunc");
	switch (alphaFunc) {
		case 0: fWr.String("B_ALPHA_OVERLAY"); break;
		case 1: fWr.String("B_ALPHA_COMPOSITE"); break;
		case 2: fWr.String("B_ALPHA_COMPOSITE_SOURCE_IN"); break;
		case 3: fWr.String("B_ALPHA_COMPOSITE_SOURCE_OUT"); break;
		case 4: fWr.String("B_ALPHA_COMPOSITE_SOURCE_ATOP"); break;
		case 5: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_OVER"); break;
		case 6: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_IN"); break;
		case 7: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_OUT"); break;
		case 8: fWr.String("B_ALPHA_COMPOSITE_DESTINATION_ATOP"); break;
		case 9: fWr.String("B_ALPHA_COMPOSITE_XOR"); break;
		case 10: fWr.String("B_ALPHA_COMPOSITE_CLEAR"); break;
		case 11: fWr.String("B_ALPHA_COMPOSITE_DIFFERENCE"); break;
		case 12: fWr.String("B_ALPHA_COMPOSITE_LIGHTEN"); break;
		case 13: fWr.String("B_ALPHA_COMPOSITE_DARKEN"); break;
		default: fWr.Int(alphaFunc);
	}
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::SetFillRule(int32 fillRule)
{
	fWr.StartObject();
	fWr.Key("SET_FILL_RULE");
	switch (fillRule) {
		case 0: fWr.String("B_EVEN_ODD"); break;
		case 1: fWr.String("B_NONZERO"); break;
		default: fWr.Int(fillRule);
	}
	fWr.EndObject();
}


// #pragma mark - State Relative

void PictureVisitorJson::SetOrigin(const BPoint& point)
{
	fWr.StartObject();
	fWr.Key("SET_ORIGIN");
	WritePoint(point);
	fWr.EndObject();
}

void PictureVisitorJson::SetScale(float scale)
{
	fWr.StartObject();
	fWr.Key("SET_SCALE");
	fWr.Double(scale);
	fWr.EndObject();
}

void PictureVisitorJson::SetPenLocation(const BPoint& point)
{
	fWr.StartObject();
	fWr.Key("SET_PEN_LOCATION");
	WritePoint(point);
	fWr.EndObject();
}

void PictureVisitorJson::SetTransform(const BAffineTransform& transform)
{
	fWr.StartObject();
	fWr.Key("SET_TRANSFORM");
	WriteTransform(transform);
	fWr.EndObject();
}


// #pragma mark - Clipping

void PictureVisitorJson::SetClipping(const BRegion& region)
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

void PictureVisitorJson::ClearClipping()
{
	fWr.StartObject();
	fWr.Key("CLEAR_CLIPPING_RECTS");
	fWr.StartObject();
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
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

void PictureVisitorJson::ClipToRect(const BRect& rect, bool inverse)
{
	fWr.StartObject();
	fWr.Key("CLIP_TO_RECT");
	fWr.StartObject();
	fWr.Key("inverse"); fWr.Bool(inverse);
	fWr.Key("rect"); WriteRect(rect);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::ClipToShape(const BShape& shape, bool inverse)
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

void PictureVisitorJson::SetFontFamily(const font_family family)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_FAMILY");
	fWr.String(family, strlen(family) + 1);
	fWr.EndObject();
}

void PictureVisitorJson::SetFontStyle(const font_style style)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_STYLE");
	fWr.String(style, strlen(style) + 1);
	fWr.EndObject();
}

void PictureVisitorJson::SetFontSpacing(int32 spacing)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_SPACING");
	switch (spacing) {
		case 0: fWr.String("B_CHAR_SPACING"); break;
		case 1: fWr.String("B_STRING_SPACING"); break;
		case 2: fWr.String("B_BITMAP_SPACING"); break;
		case 3: fWr.String("B_FIXED_SPACING"); break;
		default: fWr.Int(spacing);
	}
	fWr.EndObject();
}

void PictureVisitorJson::SetFontSize(float size)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_SIZE");
	fWr.Double(size);
	fWr.EndObject();
}

void PictureVisitorJson::SetFontRotation(float rotation)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_ROTATE");
	fWr.Double(rotation);
	fWr.EndObject();
}

void PictureVisitorJson::SetFontEncoding(int32 encoding)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_ENCODING");
	switch (encoding) {
		case 0: fWr.String("B_UNICODE_UTF8"); break;
		case 1: fWr.String("B_ISO_8859_1"); break;
		case 2: fWr.String("B_ISO_8859_2"); break;
		case 3: fWr.String("B_ISO_8859_3"); break;
		case 4: fWr.String("B_ISO_8859_4"); break;
		case 5: fWr.String("B_ISO_8859_5"); break;
		case 6: fWr.String("B_ISO_8859_6"); break;
		case 7: fWr.String("B_ISO_8859_7"); break;
		case 8: fWr.String("B_ISO_8859_8"); break;
		case 9: fWr.String("B_ISO_8859_9"); break;
		case 10: fWr.String("B_ISO_8859_10"); break;
		case 11: fWr.String("B_MACINTOSH_ROMAN"); break;
		default: fWr.Int(encoding);
	}
	fWr.EndObject();
}

void PictureVisitorJson::PictureVisitorJson::SetFontFlags(int32 flags)
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

void PictureVisitorJson::SetFontShear(float shear)
{
	fWr.StartObject();
	fWr.Key("SET_FONT_SHEAR");
	fWr.Double(shear);
	fWr.EndObject();
}

void PictureVisitorJson::SetFontFace(int32 face)
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

void PictureVisitorJson::MovePenBy(float dx, float dy)
{
	fWr.StartObject();
	fWr.Key("MOVE_PEN_BY");
	fWr.Key("start"); WritePoint(BPoint(dx, dy));
	fWr.EndObject();
}

void PictureVisitorJson::TranslateBy(double x, double y)
{
	fWr.StartObject();
	fWr.Key("AFFINE_TRANSLATE");
	fWr.StartObject();
	fWr.Key("x"); fWr.Double(x);
	fWr.Key("y"); fWr.Double(y);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::ScaleBy(double x, double y)
{
	fWr.StartObject();
	fWr.Key("AFFINE_SCALE");
	fWr.StartObject();
	fWr.Key("x"); fWr.Double(x);
	fWr.Key("y"); fWr.Double(y);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::RotateBy(double angleRadians)
{
	fWr.StartObject();
	fWr.Key("AFFINE_ROTATE");
	fWr.Double(angleRadians);
	fWr.EndObject();
}


// #pragma mark - Geometry

void PictureVisitorJson::StrokeLine(const BPoint& start, const BPoint& end)
{
	fWr.StartObject();
	fWr.Key("STROKE_LINE");
	fWr.StartObject();
	fWr.Key("start"); WritePoint(start);
	fWr.Key("end"); WritePoint(end);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokeRect(const BRect& rect)
{
	fWr.StartObject();
	fWr.Key("STROKE_RECT");
	WriteRect(rect);
	fWr.EndObject();
}

void PictureVisitorJson::FillRect(const BRect& rect)
{
	fWr.StartObject();
	fWr.Key("FILL_RECT");
	WriteRect(rect);
	fWr.EndObject();
}

void PictureVisitorJson::StrokeRect(const BRect& rect, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_RECT_GRADIENT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillRect(const BRect& rect, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("FILL_RECT_GRADIENT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokeRoundRect(const BRect& rect,
							const BPoint& radius)
{
	fWr.StartObject();
	fWr.Key("STROKE_ROUND_RECT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("radius"); WritePoint(radius);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillRoundRect(const BRect& rect,
							const BPoint& radius)
{
	fWr.StartObject();
	fWr.Key("FILL_ROUND_RECT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("radius"); WritePoint(radius);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::StrokeRoundRect(const BRect& rect,
							const BPoint& radius, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_ROUND_RECT_GRADIENT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillRoundRect(const BRect& rect,
							const BPoint& radius, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("FILL_ROUND_RECT_GRADIENT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokeBezier(const BPoint points[4])
{
	fWr.StartObject();
	fWr.Key("STROKE_BEZIER");
	fWr.StartArray();
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.EndObject();
}

void PictureVisitorJson::FillBezier(const BPoint points[4])
{
	fWr.StartObject();
	fWr.Key("FILL_BEZIER");
	fWr.StartArray();
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.EndObject();
}

void PictureVisitorJson::StrokeBezier(const BPoint points[4], const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_BEZIER_GRADIENT");
	fWr.StartObject();
	fWr.Key("points");
	fWr.StartArray();
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillBezier(const BPoint points[4], const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("FILL_BEZIER_GRADIENT");
	fWr.StartObject();
	fWr.Key("points");
	fWr.StartArray();
	for (int32 i = 0; i < 4; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokePolygon(int32 numPoints, const BPoint* points, bool isClosed)
{
	fWr.StartObject();
	fWr.Key("STROKE_POLYGON");
	fWr.StartObject();
	fWr.Key("points");
	fWr.StartArray();
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.Key("isClosed"); fWr.Bool(isClosed);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillPolygon(int32 numPoints, const BPoint* points)
{
	fWr.StartObject();
	fWr.Key("FILL_POLYGON");
	fWr.StartArray();
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.EndObject();
}

void PictureVisitorJson::StrokePolygon(int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_POLYGON_GRADIENT");
	fWr.StartObject();
	fWr.Key("points");
	fWr.StartArray();
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.Key("isClosed"); fWr.Bool(isClosed);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillPolygon(int32 numPoints, const BPoint* points, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_POLYGON_GRADIENT");
	fWr.StartObject();
	fWr.Key("points");
	fWr.StartArray();
	for (int32 i = 0; i < numPoints; i++) {
		WritePoint(points[i]);
	}
	fWr.EndArray();
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokeShape(const BShape& shape)
{
	fWr.StartObject();
	fWr.Key("STROKE_SHAPE");
	WriteShape(shape);
	fWr.EndObject();
}

void PictureVisitorJson::FillShape(const BShape& shape)
{
	fWr.StartObject();
	fWr.Key("FILL_SHAPE");
	WriteShape(shape);
	fWr.EndObject();
}

void PictureVisitorJson::StrokeShape(const BShape& shape, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_SHAPE_GRADIENT");
	fWr.StartObject();
	fWr.Key("shape"); WriteShape(shape);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillShape(const BShape& shape, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("FILL_SHAPE_GRADIENT");
	fWr.StartObject();
	fWr.Key("shape"); WriteShape(shape);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokeArc(const BPoint& center,
							const BPoint& radius,
							float startTheta,
							float arcTheta)
{
	fWr.StartObject();
	fWr.Key("STROKE_ARC");
	fWr.StartObject();
	fWr.Key("center"); WritePoint(center);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("startTheta"); fWr.Double(startTheta);
	fWr.Key("arcTheta"); fWr.Double(arcTheta);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillArc(const BPoint& center,
							const BPoint& radius,
							float startTheta,
							float arcTheta)
{
	fWr.StartObject();
	fWr.Key("FILL_ARC");
	fWr.StartObject();
	fWr.Key("center"); WritePoint(center);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("startTheta"); fWr.Double(startTheta);
	fWr.Key("arcTheta"); fWr.Double(arcTheta);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::StrokeArc(const BPoint& center,
							const BPoint& radius,
							float startTheta,
							float arcTheta,
							const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_ARC_GRADIENT");
	fWr.StartObject();
	fWr.Key("center"); WritePoint(center);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("startTheta"); fWr.Double(startTheta);
	fWr.Key("arcTheta"); fWr.Double(arcTheta);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillArc(const BPoint& center,
							const BPoint& radius,
							float startTheta,
							float arcTheta,
							const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("FILL_ARC_GRADIENT");
	fWr.StartObject();
	fWr.Key("center"); WritePoint(center);
	fWr.Key("radius"); WritePoint(radius);
	fWr.Key("startTheta"); fWr.Double(startTheta);
	fWr.Key("arcTheta"); fWr.Double(arcTheta);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


void PictureVisitorJson::StrokeEllipse(const BRect& rect)
{
	fWr.StartObject();
	fWr.Key("STROKE_ELLIPSE");
	WriteRect(rect);
	fWr.EndObject();
}

void PictureVisitorJson::FillEllipse(const BRect& rect)
{
	fWr.StartObject();
	fWr.Key("FILL_ELLIPSE");
	WriteRect(rect);
	fWr.EndObject();
}

void PictureVisitorJson::StrokeEllipse(const BRect& rect, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("STROKE_ELLIPSE_GRADIENT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}

void PictureVisitorJson::FillEllipse(const BRect& rect, const BGradient& gradient)
{
	fWr.StartObject();
	fWr.Key("FILL_ELLIPSE_GRADIENT");
	fWr.StartObject();
	fWr.Key("rect"); WriteRect(rect);
	fWr.Key("gradient"); WriteGradient(gradient);
	fWr.EndObject();
	fWr.EndObject();
}


// #pragma mark - Draw

void PictureVisitorJson::DrawString(
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

void PictureVisitorJson::DrawString(const char* string,
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


void PictureVisitorJson::DrawBitmap(const BRect& srcRect,
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

void PictureVisitorJson::DrawPicture(const BPoint& where,
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


void PictureVisitorJson::BlendLayer(Layer* layer)
{
	fWr.StartObject();
	fWr.Key("BLEND_LAYER");
	fWr.StartObject();
	// TODO: implement
	fWr.EndObject();
	fWr.EndObject();
}
