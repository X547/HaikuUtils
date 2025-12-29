#include "PictureWriterYaml.h"

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientConic.h>
#include <GradientDiamond.h>


class PictureWriterYaml::ShapeIterator final: public BShapeIterator {
private:
	PictureWriterYaml &fBase;

public:
	virtual ~ShapeIterator() = default;

	ShapeIterator(PictureWriterYaml &base): fBase(base) {}


	status_t IterateMoveTo(BPoint* point) final
	{
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::Key << "MoveTo" << YAML::Value;
		fBase.WritePoint(*point);
		fBase.fWr << YAML::EndMap;
		return B_OK;
	}

	status_t IterateLineTo(int32 lineCount, BPoint* linePoints) final
	{
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::Key << "LineTo" << YAML::Value;
		fBase.fWr << YAML::BeginSeq;
		for (int32 i = 0; i < lineCount; i++) {
			fBase.WritePoint(linePoints[i]);
		}
		fBase.fWr << YAML::EndSeq;
		fBase.fWr << YAML::EndMap;
		return B_OK;
	}

	status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPoints) final
	{
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::Key << "BezierTo" << YAML::Value;
		fBase.fWr << YAML::BeginSeq;
		for (int32 i = 0; i < 3*bezierCount; i++) {
			fBase.WritePoint(bezierPoints[i]);
		}
		fBase.fWr << YAML::EndSeq;
		fBase.fWr << YAML::EndMap;
		return B_OK;
	}

	status_t IterateClose()
	{
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::Key << "Close" << YAML::Value;
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::EndMap;
		fBase.fWr << YAML::EndMap;
		return B_OK;
	}

	status_t IterateArcTo(
		float& rx, float& ry,
		float& angle, bool largeArc,
		bool counterClockWise, BPoint& point
	)
	{
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::Key << "ArcTo" << YAML::Value;
		fBase.fWr << YAML::BeginMap;
		fBase.fWr << YAML::Key << "rx" << YAML::Value; fBase.fWr << rx;
		fBase.fWr << YAML::Key << "ry" << YAML::Value; fBase.fWr << ry;
		fBase.fWr << YAML::Key << "angle" << YAML::Value; fBase.fWr << angle;
		fBase.fWr << YAML::Key << "largeArc" << YAML::Value; fBase.fWr << largeArc;
		fBase.fWr << YAML::Key << "ccw" << YAML::Value; fBase.fWr << counterClockWise;
		fBase.fWr << YAML::Key << "point" << YAML::Value; fBase.WritePoint(point);
		fBase.fWr << YAML::EndMap;
		fBase.fWr << YAML::EndMap;
		return B_OK;
	}
};


PictureWriterYaml::PictureWriterYaml(YAML::Emitter &wr):
	fWr(wr)
{
}


void PictureWriterYaml::WriteString(const char *str, size_t len)
{
	std::string_view sv(str, len);
	std::string stdStr;
	stdStr = sv;
	fWr << stdStr;
}

void PictureWriterYaml::WriteColor(const rgb_color &c)
{
	char buf[64];
	sprintf(buf, "#%02x%02x%02x%02x",
		c.alpha,
		c.red,
		c.green,
		c.blue
	);
	fWr << buf;
}

void PictureWriterYaml::WritePoint(const BPoint &pt)
{
	fWr << YAML::Flow << YAML::BeginMap;
	fWr << YAML::Key << "x" << YAML::Value; fWr << pt.x;
	fWr << YAML::Key << "y" << YAML::Value; fWr << pt.y;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::WriteRect(const BRect &rc)
{
	fWr << YAML::Flow << YAML::BeginMap;
	fWr << YAML::Key << "left" << YAML::Value;   fWr << rc.left;
	fWr << YAML::Key << "top" << YAML::Value;    fWr << rc.top;
	fWr << YAML::Key << "right" << YAML::Value;  fWr << rc.right;
	fWr << YAML::Key << "bottom" << YAML::Value; fWr << rc.bottom;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::WriteShape(const BShape &shape)
{
	ShapeIterator iter(*this);
	fWr << YAML::BeginSeq;
	iter.Iterate(const_cast<BShape*>(&shape));
	fWr << YAML::EndSeq;
}

void PictureWriterYaml::WriteGradient(const BGradient &gradient)
{
	fWr << YAML::BeginMap;
	switch (gradient.GetType()) {
		case BGradient::TYPE_LINEAR:
			fWr << YAML::Key << "BGradientLinear" << YAML::Value;
			break;
		case BGradient::TYPE_RADIAL:
			fWr << YAML::Key << "BGradientRadial" << YAML::Value;
			break;
		case BGradient::TYPE_RADIAL_FOCUS:
			fWr << YAML::Key << "BGradientRadialFocus" << YAML::Value;
			break;
		case BGradient::TYPE_DIAMOND:
			fWr << YAML::Key << "BGradientDiamond" << YAML::Value;
			break;
		case BGradient::TYPE_CONIC:
			fWr << YAML::Key << "BGradientConic" << YAML::Value;
			break;
		default:
			fWr << YAML::Key << "BGradient" << YAML::Value;
			break;
	}
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "stops" << YAML::Value;
	fWr << YAML::BeginSeq;
	for (int32 i = 0; i < gradient.CountColorStops(); i++) {
		BGradient::ColorStop *cs = gradient.ColorStopAt(i);
		fWr << YAML::BeginMap;
		fWr << YAML::Key << "color" << YAML::Value; WriteColor(cs->color);
		fWr << YAML::Key << "offset" << YAML::Value; fWr << cs->offset;
		fWr << YAML::EndMap;
	}
	fWr << YAML::EndSeq;
	switch (gradient.GetType()) {
	case BGradient::TYPE_LINEAR: {
		const BGradientLinear &grad = static_cast<const BGradientLinear &>(gradient);
		fWr << YAML::Key << "start" << YAML::Value; WritePoint(grad.Start());
		fWr << YAML::Key << "end" << YAML::Value; WritePoint(grad.End());
		break;
	}
	case BGradient::TYPE_RADIAL: {
		const BGradientRadial &grad = static_cast<const BGradientRadial &>(gradient);
		fWr << YAML::Key << "center" << YAML::Value; WritePoint(grad.Center());
		fWr << YAML::Key << "radius" << YAML::Value; fWr << grad.Radius();
		break;
	}
	case BGradient::TYPE_RADIAL_FOCUS: {
		const BGradientRadialFocus &grad = static_cast<const BGradientRadialFocus &>(gradient);
		fWr << YAML::Key << "center" << YAML::Value; WritePoint(grad.Center());
		fWr << YAML::Key << "focus" << YAML::Value; WritePoint(grad.Focal());
		fWr << YAML::Key << "radius" << YAML::Value; fWr << grad.Radius();
		break;
	}
	case BGradient::TYPE_DIAMOND: {
		const BGradientDiamond &grad = static_cast<const BGradientDiamond &>(gradient);
		fWr << YAML::Key << "center" << YAML::Value; WritePoint(grad.Center());
		break;
	}
	case BGradient::TYPE_CONIC: {
		const BGradientConic &grad = static_cast<const BGradientConic &>(gradient);
		fWr << YAML::Key << "center" << YAML::Value; WritePoint(grad.Center());
		fWr << YAML::Key << "angle" << YAML::Value; fWr << grad.Angle();
		break;
	}
	case BGradient::TYPE_NONE:
		break;
	}
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::WriteTransform(const BAffineTransform& tr)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "tx" << YAML::Value;  fWr << tr.tx;
	fWr << YAML::Key << "ty" << YAML::Value;  fWr << tr.ty;
	fWr << YAML::Key << "sx" << YAML::Value;  fWr << tr.sx;
	fWr << YAML::Key << "sy" << YAML::Value;  fWr << tr.sy;
	fWr << YAML::Key << "shy" << YAML::Value; fWr << tr.shy;
	fWr << YAML::Key << "shx" << YAML::Value; fWr << tr.shx;
	fWr << YAML::EndMap;
}



// #pragma mark - Meta

void PictureWriterYaml::EnterPicture(int32 version, int32 endian)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "version" << YAML::Value; fWr << version;
	fWr << YAML::Key << "endian" << YAML::Value; fWr << endian;
}

void PictureWriterYaml::ExitPicture()
{
	fWr << YAML::EndMap;
}

void PictureWriterYaml::EnterPictures(int32 count)
{
	fWr << YAML::Key << "pictures" << YAML::Value;
	fWr << YAML::BeginSeq;
}

void PictureWriterYaml::ExitPictures()
{
	fWr << YAML::EndSeq;
}

void PictureWriterYaml::EnterOps()
{
	fWr << YAML::Key << "ops" << YAML::Value;
	fWr << YAML::BeginSeq;
}

void PictureWriterYaml::ExitOps()
{
	fWr << YAML::EndSeq;
}


void PictureWriterYaml::EnterStateChange()
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "ENTER_STATE_CHANGE" << YAML::Value;
	fWr << YAML::BeginSeq;
}

void PictureWriterYaml::ExitStateChange()
{
	fWr << YAML::EndSeq;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::EnterFontState()
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "ENTER_FONT_STATE" << YAML::Value;
	fWr << YAML::BeginSeq;
}

void PictureWriterYaml::ExitFontState()
{
	fWr << YAML::EndSeq;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::PushState()
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "GROUP" << YAML::Value;
	fWr << YAML::BeginSeq;
}

void PictureWriterYaml::PopState()
{
	fWr << YAML::EndSeq;
	fWr << YAML::EndMap;
}


// #pragma mark - State Absolute

void PictureWriterYaml::SetDrawingMode(drawing_mode mode)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_DRAWING_MODE" << YAML::Value;
	switch (mode) {
		case B_OP_COPY: fWr << "B_OP_COPY"; break;
		case B_OP_OVER: fWr << "B_OP_OVER"; break;
		case B_OP_ERASE: fWr << "B_OP_ERASE"; break;
		case B_OP_INVERT: fWr << "B_OP_INVERT"; break;
		case B_OP_ADD: fWr << "B_OP_ADD"; break;
		case B_OP_SUBTRACT: fWr << "B_OP_SUBTRACT"; break;
		case B_OP_BLEND: fWr << "B_OP_BLEND"; break;
		case B_OP_MIN: fWr << "B_OP_MIN"; break;
		case B_OP_MAX: fWr << "B_OP_MAX"; break;
		case B_OP_SELECT: fWr << "B_OP_SELECT"; break;
		case B_OP_ALPHA: fWr << "B_OP_ALPHA"; break;
		default: fWr << mode;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetLineMode(cap_mode cap,
							join_mode join,
							float miterLimit)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_LINE_MODE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "capMode" << YAML::Value;
	switch (cap) {
		case B_ROUND_CAP: fWr << "B_ROUND_CAP"; break;
		case B_BUTT_CAP: fWr << "B_BUTT_CAP"; break;
		case B_SQUARE_CAP: fWr << "B_SQUARE_CAP"; break;
		default: fWr << cap;
	}
	fWr << YAML::Key << "joinMode" << YAML::Value;
	switch (join) {
		case B_ROUND_JOIN: fWr << "B_ROUND_JOIN"; break;
		case B_MITER_JOIN: fWr << "B_MITER_JOIN"; break;
		case B_BEVEL_JOIN: fWr << "B_BEVEL_JOIN"; break;
		case B_BUTT_JOIN: fWr << "B_BUTT_JOIN"; break;
		case B_SQUARE_JOIN: fWr << "B_SQUARE_JOIN"; break;
		default: fWr << join;
	}
	fWr << YAML::Key << "miterLimit" << YAML::Value; fWr << miterLimit;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetPenSize(float penSize)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_PEN_SIZE" << YAML::Value;
	fWr << penSize;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetHighColor(const rgb_color& color)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FORE_COLOR" << YAML::Value;
	WriteColor(color);
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetLowColor(const rgb_color& color)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_BACK_COLOR" << YAML::Value;
	WriteColor(color);
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetPattern(const ::pattern& pat)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_STIPLE_PATTERN" << YAML::Value;
	if (memcmp(&pat, &B_SOLID_HIGH, sizeof(pattern)) == 0)
		fWr << "B_SOLID_HIGH";
	else if (memcmp(&pat, &B_SOLID_LOW, sizeof(pattern)) == 0)
		fWr << "B_SOLID_LOW";
	else if (memcmp(&pat, &B_MIXED_COLORS, sizeof(pattern)) == 0)
		fWr << "B_MIXED_COLORS";
	else {
		fWr << YAML::Binary((const uint8*)pat.data, 8);
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetBlendingMode(source_alpha srcAlpha,
							alpha_function alphaFunc)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_BLENDING_MODE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "srcAlpha" << YAML::Value;
	switch (srcAlpha) {
		case B_PIXEL_ALPHA: fWr << "B_PIXEL_ALPHA"; break;
		case B_CONSTANT_ALPHA: fWr << "B_CONSTANT_ALPHA"; break;
		default: fWr << srcAlpha;
	}
	fWr << YAML::Key << "alphaFunc" << YAML::Value;
	switch (alphaFunc) {
		case B_ALPHA_OVERLAY: fWr << "B_ALPHA_OVERLAY"; break;
		case B_ALPHA_COMPOSITE: fWr << "B_ALPHA_COMPOSITE"; break;
		case B_ALPHA_COMPOSITE_SOURCE_IN: fWr << "B_ALPHA_COMPOSITE_SOURCE_IN"; break;
		case B_ALPHA_COMPOSITE_SOURCE_OUT: fWr << "B_ALPHA_COMPOSITE_SOURCE_OUT"; break;
		case B_ALPHA_COMPOSITE_SOURCE_ATOP: fWr << "B_ALPHA_COMPOSITE_SOURCE_ATOP"; break;
		case B_ALPHA_COMPOSITE_DESTINATION_OVER: fWr << "B_ALPHA_COMPOSITE_DESTINATION_OVER"; break;
		case B_ALPHA_COMPOSITE_DESTINATION_IN: fWr << "B_ALPHA_COMPOSITE_DESTINATION_IN"; break;
		case B_ALPHA_COMPOSITE_DESTINATION_OUT: fWr << "B_ALPHA_COMPOSITE_DESTINATION_OUT"; break;
		case B_ALPHA_COMPOSITE_DESTINATION_ATOP: fWr << "B_ALPHA_COMPOSITE_DESTINATION_ATOP"; break;
		case B_ALPHA_COMPOSITE_XOR: fWr << "B_ALPHA_COMPOSITE_XOR"; break;
		case B_ALPHA_COMPOSITE_CLEAR: fWr << "B_ALPHA_COMPOSITE_CLEAR"; break;
		case B_ALPHA_COMPOSITE_DIFFERENCE: fWr << "B_ALPHA_COMPOSITE_DIFFERENCE"; break;
		case B_ALPHA_COMPOSITE_LIGHTEN: fWr << "B_ALPHA_COMPOSITE_LIGHTEN"; break;
		case B_ALPHA_COMPOSITE_DARKEN: fWr << "B_ALPHA_COMPOSITE_DARKEN"; break;
		default: fWr << alphaFunc;
	}
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFillRule(int32 fillRule)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FILL_RULE" << YAML::Value;
	switch (fillRule) {
		case B_EVEN_ODD: fWr << "B_EVEN_ODD"; break;
		case B_NONZERO: fWr << "B_NONZERO"; break;
		default: fWr << fillRule;
	}
	fWr << YAML::EndMap;
}


// #pragma mark - State Relative

void PictureWriterYaml::SetOrigin(const BPoint& point)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_ORIGIN" << YAML::Value;
	WritePoint(point);
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetScale(float scale)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_SCALE" << YAML::Value;
	fWr << scale;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetPenLocation(const BPoint& point)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_PEN_LOCATION" << YAML::Value;
	WritePoint(point);
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetTransform(const BAffineTransform& transform)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_TRANSFORM" << YAML::Value;
	WriteTransform(transform);
	fWr << YAML::EndMap;
}


// #pragma mark - Clipping

void PictureWriterYaml::SetClipping(const BRegion& region)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_CLIPPING_RECTS" << YAML::Value;
	fWr << YAML::BeginSeq;
	for (int32 i = 0; i < region.CountRects(); i++) {
		WriteRect(region.RectAt(i));
	}
	fWr << YAML::EndSeq;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::ClearClipping()
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "CLEAR_CLIPPING_RECTS" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::ClipToPicture(int32 pictureToken, const BPoint& origin, bool inverse)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "CLIP_TO_PICTURE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "token" << YAML::Value; fWr << pictureToken;
	fWr << YAML::Key << "where" << YAML::Value; WritePoint(origin);
	fWr << YAML::Key << "inverse" << YAML::Value; fWr << inverse;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::ClipToRect(const BRect& rect, bool inverse)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "CLIP_TO_RECT" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "inverse" << YAML::Value; fWr << inverse;
	fWr << YAML::Key << "rect" << YAML::Value; WriteRect(rect);
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::ClipToShape(const BShape& shape, bool inverse)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "CLIP_TO_SHAPE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "inverse" << YAML::Value; fWr << inverse;
	fWr << YAML::Key << "shape" << YAML::Value; WriteShape(shape);
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}


// #pragma mark - Font

void PictureWriterYaml::SetFontFamily(const font_family family)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_FAMILY" << YAML::Value;
	WriteString(family, strlen(family));
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontStyle(const font_style style)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_STYLE" << YAML::Value;
	WriteString(style, strlen(style));
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontSpacing(int32 spacing)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_SPACING" << YAML::Value;
	switch (spacing) {
		case B_CHAR_SPACING: fWr << "B_CHAR_SPACING"; break;
		case B_STRING_SPACING: fWr << "B_STRING_SPACING"; break;
		case B_BITMAP_SPACING: fWr << "B_BITMAP_SPACING"; break;
		case B_FIXED_SPACING: fWr << "B_FIXED_SPACING"; break;
		default: fWr << spacing;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontSize(float size)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_SIZE" << YAML::Value;
	fWr << size;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontRotation(float rotation)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_ROTATE" << YAML::Value;
	fWr << rotation;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontEncoding(int32 encoding)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_ENCODING" << YAML::Value;
	switch (encoding) {
		case B_UNICODE_UTF8: fWr << "B_UNICODE_UTF8"; break;
		case B_ISO_8859_1: fWr << "B_ISO_8859_1"; break;
		case B_ISO_8859_2: fWr << "B_ISO_8859_2"; break;
		case B_ISO_8859_3: fWr << "B_ISO_8859_3"; break;
		case B_ISO_8859_4: fWr << "B_ISO_8859_4"; break;
		case B_ISO_8859_5: fWr << "B_ISO_8859_5"; break;
		case B_ISO_8859_6: fWr << "B_ISO_8859_6"; break;
		case B_ISO_8859_7: fWr << "B_ISO_8859_7"; break;
		case B_ISO_8859_8: fWr << "B_ISO_8859_8"; break;
		case B_ISO_8859_9: fWr << "B_ISO_8859_9"; break;
		case B_ISO_8859_10: fWr << "B_ISO_8859_10"; break;
		case B_MACINTOSH_ROMAN: fWr << "B_MACINTOSH_ROMAN"; break;
		default: fWr << encoding;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontFlags(int32 flags)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_FLAGS" << YAML::Value;
	fWr << YAML::BeginSeq;
	for (uint32 i = 0; i < 32; i++) {
		if ((1U << i) & (uint32)flags) {
			switch (i) {
				case 0: fWr << "B_DISABLE_ANTIALIASING"; break;
				case 1: fWr << "B_FORCE_ANTIALIASING"; break;
				default: fWr << i;
			}
		}
	}
	fWr << YAML::EndSeq;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontShear(float shear)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_SHEAR" << YAML::Value;
	fWr << shear;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontBpp(int32 bpp)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_BPP" << YAML::Value;
	fWr << bpp;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::SetFontFace(int32 face)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "SET_FONT_FACE" << YAML::Value;
	fWr << YAML::BeginSeq;
	for (uint32 i = 0; i < 32; i++) {
		if ((1U << i) & (uint32)face) {
			switch (i) {
				case 0: fWr << "B_ITALIC_FACE"; break;
				case 1: fWr << "B_UNDERSCORE_FACE"; break;
				case 2: fWr << "B_NEGATIVE_FACE"; break;
				case 3: fWr << "B_OUTLINED_FACE"; break;
				case 4: fWr << "B_STRIKEOUT_FACE"; break;
				case 5: fWr << "B_BOLD_FACE"; break;
				case 6: fWr << "B_REGULAR_FACE"; break;
				case 7: fWr << "B_CONDENSED_FACE"; break;
				case 8: fWr << "B_LIGHT_FACE"; break;
				case 9: fWr << "B_HEAVY_FACE"; break;
				default: fWr << i;
			}
		}
	}
	fWr << YAML::EndSeq;
	fWr << YAML::EndMap;
}


// #pragma mark - State (delta)

void PictureWriterYaml::MovePenBy(float dx, float dy)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "MOVE_PEN_BY" << YAML::Value;
	WritePoint(BPoint(dx, dy));
	fWr << YAML::EndMap;
}

void PictureWriterYaml::TranslateBy(double x, double y)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "AFFINE_TRANSLATE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "x" << YAML::Value; fWr << x;
	fWr << YAML::Key << "y" << YAML::Value; fWr << y;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::ScaleBy(double x, double y)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "AFFINE_SCALE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "x" << YAML::Value; fWr << x;
	fWr << YAML::Key << "y" << YAML::Value; fWr << y;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::RotateBy(double angleRadians)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "AFFINE_ROTATE" << YAML::Value;
	fWr << angleRadians;
	fWr << YAML::EndMap;
}


// #pragma mark - Geometry

void PictureWriterYaml::DrawLine(const BPoint& start, const BPoint& end, const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << (drawInfo.gradient == NULL ? "STROKE_LINE" : "STROKE_LINE_GRADIENT") << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "start" << YAML::Value; WritePoint(start);
	fWr << YAML::Key << "end" << YAML::Value; WritePoint(end);
	if (drawInfo.gradient != NULL) {
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
	}
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}


void PictureWriterYaml::DrawRect(const BRect& rect, const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_RECT" : "FILL_RECT") << YAML::Value;
		WriteRect(rect);
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_RECT_GRADIENT" : "FILL_RECT_GRADIENT") << YAML::Value;
		fWr << YAML::BeginMap;
		fWr << YAML::Key << "rect" << YAML::Value; WriteRect(rect);
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
		fWr << YAML::EndMap;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawRoundRect(const BRect& rect, const BPoint& radius, const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_ROUND_RECT" : "FILL_ROUND_RECT") << YAML::Value;
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_ROUND_RECT_GRADIENT" : "FILL_ROUND_RECT_GRADIENT") << YAML::Value;
	}
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "rect" << YAML::Value; WriteRect(rect);
	fWr << YAML::Key << "radius" << YAML::Value; WritePoint(radius);
	if (drawInfo.gradient != NULL) {
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
	}
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawBezier(const BPoint points[4], const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_BEZIER" : "FILL_BEZIER") << YAML::Value;
		fWr << YAML::BeginSeq;
		for (int32 i = 0; i < 4; i++) {
			WritePoint(points[i]);
		}
		fWr << YAML::EndSeq;
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_BEZIER_GRADIENT" : "FILL_BEZIER_GRADIENT") << YAML::Value;
		fWr << YAML::BeginMap;
		fWr << YAML::Key << "points" << YAML::Value;
		fWr << YAML::BeginSeq;
		for (int32 i = 0; i < 4; i++) {
			WritePoint(points[i]);
		}
		fWr << YAML::EndSeq;
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
		fWr << YAML::EndMap;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawPolygon(int32 numPoints, const BPoint* points, bool isClosed, const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_POLYGON" : "FILL_POLYGON") << YAML::Value;
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_POLYGON_GRADIENT" : "FILL_POLYGON_GRADIENT") << YAML::Value;
	}
	if (drawInfo.isStroke || drawInfo.gradient != NULL) {
		fWr << YAML::BeginMap;
		fWr << YAML::Key << "points" << YAML::Value;
		fWr << YAML::BeginSeq;
		for (int32 i = 0; i < numPoints; i++) {
			WritePoint(points[i]);
		}
		fWr << YAML::EndSeq;
		if (drawInfo.isStroke) {
			fWr << YAML::Key << "isClosed" << YAML::Value; fWr << isClosed;
		}
		if (drawInfo.gradient != NULL) {
			fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
		}
		fWr << YAML::EndMap;
	} else {
		fWr << YAML::BeginSeq;
		for (int32 i = 0; i < numPoints; i++) {
			WritePoint(points[i]);
		}
		fWr << YAML::EndSeq;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawShape(const BShape& shape, const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_SHAPE" : "FILL_SHAPE") << YAML::Value;
		WriteShape(shape);
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_SHAPE_GRADIENT" : "FILL_SHAPE_GRADIENT") << YAML::Value;
		fWr << YAML::BeginMap;
		fWr << YAML::Key << "shape" << YAML::Value; WriteShape(shape);
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
		fWr << YAML::EndMap;
	}
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawArc(
	const BPoint& center,
	const BPoint& radius,
	float startTheta,
	float arcTheta,
	const DrawGeometryInfo &drawInfo
)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_ARC" : "FILL_ARC") << YAML::Value;
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_ARC_GRADIENT" : "FILL_ARC_GRADIENT") << YAML::Value;
	}
	fWr << YAML::Key << "STROKE_ARC" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "center" << YAML::Value; WritePoint(center);
	fWr << YAML::Key << "radius" << YAML::Value; WritePoint(radius);
	fWr << YAML::Key << "startTheta" << YAML::Value; fWr << startTheta;
	fWr << YAML::Key << "arcTheta" << YAML::Value; fWr << arcTheta;
	if (drawInfo.gradient != NULL) {
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
	}
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawEllipse(const BRect& rect, const DrawGeometryInfo &drawInfo)
{
	fWr << YAML::BeginMap;
	if (drawInfo.gradient == NULL) {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_ELLIPSE" : "FILL_ELLIPSE") << YAML::Value;
		WriteRect(rect);
	} else {
		fWr << YAML::Key << (drawInfo.isStroke ? "STROKE_ELLIPSE_GRADIENT" : "FILL_ELLIPSE_GRADIENT") << YAML::Value;
		fWr << YAML::BeginMap;
		fWr << YAML::Key << "rect" << YAML::Value; WriteRect(rect);
		fWr << YAML::Key << "gradient" << YAML::Value; WriteGradient(*drawInfo.gradient);
		fWr << YAML::EndMap;
	}
	fWr << YAML::EndMap;
}


// #pragma mark - Draw

void PictureWriterYaml::DrawString(
							const char* string, int32 length,
							const escapement_delta& delta)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "DRAW_STRING" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "string" << YAML::Value; WriteString(string, length);
	// TODO: check order
	fWr << YAML::Key << "escapementSpace" << YAML::Value; fWr << delta.space;
	fWr << YAML::Key << "escapementNonSpace" << YAML::Value; fWr << delta.nonspace;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawString(const char* string,
							int32 length, const BPoint* locations,
							int32 locationCount)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "DRAW_STRING_LOCATIONS" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "locations" << YAML::Value;
	fWr << YAML::BeginSeq;
	for (int32 i = 0; i < locationCount; i++) {
		WritePoint(locations[i]);
	}
	fWr << YAML::EndSeq;
	fWr << YAML::Key << "string" << YAML::Value; WriteString(string, length);
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}


void PictureWriterYaml::DrawBitmap(const BRect& srcRect,
							const BRect& dstRect, int32 width,
							int32 height,
							int32 bytesPerRow,
							int32 colorSpace,
							int32 flags,
							const void* data, int32 length)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "DRAW_PIXELS" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "sourceRect" << YAML::Value; WriteRect(srcRect);
	fWr << YAML::Key << "destinationRect" << YAML::Value; WriteRect(dstRect);
	fWr << YAML::Key << "width" << YAML::Value; fWr << width;
	fWr << YAML::Key << "height" << YAML::Value; fWr << height;
	fWr << YAML::Key << "bytesPerRow" << YAML::Value; fWr << bytesPerRow;
	fWr << YAML::Key << "colorSpace" << YAML::Value; fWr << colorSpace;
	fWr << YAML::Key << "flags" << YAML::Value; fWr << flags;
	fWr << YAML::Key << "data" << YAML::Value << YAML::Binary((const uint8*)data, length);
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}

void PictureWriterYaml::DrawPicture(const BPoint& where,
							int32 token)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "DRAW_PICTURE" << YAML::Value;
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "where" << YAML::Value; WritePoint(where);
	fWr << YAML::Key << "token" << YAML::Value; fWr << token;
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}


void PictureWriterYaml::BlendLayer(Layer* layer)
{
	fWr << YAML::BeginMap;
	fWr << YAML::Key << "BLEND_LAYER" << YAML::Value;
	fWr << YAML::BeginMap;
	// TODO: implement
	fWr << YAML::EndMap;
	fWr << YAML::EndMap;
}
