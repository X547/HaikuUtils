#include <View.h>
#include <Shape.h>
#include <Gradient.h>
#include <Picture.h>
#include <Rect.h>
#include <String.h>
#include <File.h>
#include <private/interface/ShapePrivate.h>
#include <stdio.h>
#include <math.h>

#include <private/shared/AutoDeleter.h>

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using JsonWriter = rapidjson::Writer<rapidjson::OStreamWrapper>;

int8 val8;
int16 val16;
int32 val32;
int64 val64;
float floatVal;
double doubleVal;

void Read8(BDataIO &rd, int8 &val) {rd.Read(&val, sizeof(val));}
void Read16(BDataIO &rd, int16 &val) {rd.Read(&val, sizeof(val));}
void Read32(BDataIO &rd, int32 &val) {rd.Read(&val, sizeof(val));}
void Read64(BDataIO &rd, int64 &val) {rd.Read(&val, sizeof(val));}
void ReadFloat(BDataIO &rd, float &val) {rd.Read(&val, sizeof(val));}
void ReadDouble(BDataIO &rd, double &val) {rd.Read(&val, sizeof(val));}
void ReadPoint(BDataIO &rd, BPoint &val) {rd.Read(&val, sizeof(val));}
void ReadRect(BDataIO &rd, BRect &val) {rd.Read(&val, sizeof(val));}
void ReadTransform(BDataIO &rd, BAffineTransform &val) {rd.Read(&val, sizeof(val));}
void ReadPattern(BDataIO &rd, pattern &val) {rd.Read(&val, sizeof(val));}

void ReadString(BDataIO &rd, BString &val) {
	int32 len;
	Read32(rd, len);
	val = "";
	for (; len > 0; len--) {
		char ch;
		rd.Read(&ch, sizeof(ch));
		val += ch;
	}
}

void ReadStringLen(BDataIO &rd, BString &val, size_t len) {
	val = "";
	for (; len > 0; len--) {
		char ch;
		rd.Read(&ch, sizeof(ch));
		val += ch;
	}
}

void DumpBool(JsonWriter &wr, bool val)
{
	wr.Bool(val);
}

void DumpBool8(JsonWriter &wr, int8 val)
{
	wr.Bool(val != 0);
}

void DumpColor(JsonWriter &wr, uint32 val)
{
	char buf[64];
	sprintf(buf, "#%02x%02x%02x%02x",
		(val >> (8*3)) % 0x100,
		(val >> (8*0)) % 0x100,
		(val >> (8*1)) % 0x100,
		(val >> (8*2)) % 0x100
	);
	wr.String(buf);
}

void DumpPoint(JsonWriter &wr, const BPoint &pt)
{
	wr.StartObject();
	wr.Key("x"); wr.Double(pt.x);
	wr.Key("y"); wr.Double(pt.y);
	wr.EndObject();
}

void DumpRect(JsonWriter &wr, const BRect &rc)
{
	wr.StartObject();
	wr.Key("left");   wr.Double(rc.left);
	wr.Key("top");    wr.Double(rc.top);
	wr.Key("right");  wr.Double(rc.right);
	wr.Key("bottom"); wr.Double(rc.bottom);
	wr.EndObject();
}

void DumpPattern(JsonWriter &wr, const pattern &pat) {
	if (memcmp(&pat, &B_SOLID_HIGH, sizeof(pattern)) == 0)
		wr.String("B_SOLID_HIGH");
	else if (memcmp(&pat, &B_SOLID_LOW, sizeof(pattern)) == 0)
		wr.String("B_SOLID_LOW");
	else if (memcmp(&pat, &B_MIXED_COLORS, sizeof(pattern)) == 0)
		wr.String("B_MIXED_COLORS");
	else {
		wr.StartArray();
		for (int32 i = 0; i < 8; i++) {
			wr.Int(pat.data[i]);
		}
		wr.EndArray();
	}
}

void DumpTransform(JsonWriter &wr, const BAffineTransform &tr)
{
	wr.StartObject();
	wr.Key("tx");  wr.Double(tr.tx);
	wr.Key("ty");  wr.Double(tr.ty);
	wr.Key("sx");  wr.Double(tr.sx);
	wr.Key("sy");  wr.Double(tr.sy);
	wr.Key("shy"); wr.Double(tr.shy);
	wr.Key("shx"); wr.Double(tr.shx);
	wr.EndObject();
}

void DumpString(JsonWriter &wr, const BString &str)
{
	wr.String(str.String(), str.Length());
}

void DumpOps(JsonWriter &wr, BPositionIO &rd, int32 size);

void DumpShape(JsonWriter &wr, BPositionIO &rd)
{
	int32 opCount;
	int32 pointCount;
	ArrayDeleter<uint32> opList;
	ArrayDeleter<BPoint> pointList;
	int32 curOp = 0;
	int32 curPt = 0;

	Read32(rd, opCount);
	Read32(rd, pointCount);
	opList.SetTo(new uint32[opCount]);
	pointList.SetTo(new BPoint[pointCount]);
	rd.Read(opList.Get(), opCount*sizeof(uint32));
	rd.Read(pointList.Get(), pointCount*sizeof(BPoint));

	wr.StartArray();
	for (; curOp < opCount; curOp++) {
		int32 op = opList.Get()[curOp] & 0xFF000000;
		int32 count = opList.Get()[curOp] & 0x00FFFFFF;

		if ((op & OP_MOVETO) != 0) {
			wr.StartObject();
			wr.Key("MoveTo");
			DumpPoint(wr, pointList.Get()[curPt++]);
			wr.EndObject();
		}

		if ((op & OP_LINETO) != 0) {
			for (int32 i = 0; i < count; i++) {
				wr.StartObject();
				wr.Key("LineTo");
				DumpPoint(wr, pointList.Get()[curPt++]);
				wr.EndObject();
			}
		}

		if ((op & OP_BEZIERTO) != 0) {
			for (int32 i = 0; i < count; i += 3) {
				wr.StartObject();
				wr.Key("BezierTo");
				wr.StartArray();
				DumpPoint(wr, pointList.Get()[curPt++]);
				DumpPoint(wr, pointList.Get()[curPt++]);
				DumpPoint(wr, pointList.Get()[curPt++]);
				wr.EndArray();
				wr.EndObject();
			}
		}

		if ((op & OP_LARGE_ARC_TO_CW) != 0 || (op & OP_LARGE_ARC_TO_CCW) != 0
			|| (op & OP_SMALL_ARC_TO_CW) != 0
			|| (op & OP_SMALL_ARC_TO_CCW) != 0) {
			for (int32 i = 0; i < count; i += 3) {
				wr.StartObject();
				wr.Key("ArcTo");
				wr.StartArray();
				DumpPoint(wr, pointList.Get()[curPt++]);
				wr.Double(pointList.Get()[curPt++].x);
				DumpBool(wr, op & (OP_LARGE_ARC_TO_CW | OP_LARGE_ARC_TO_CCW));
				DumpBool(wr, op & (OP_SMALL_ARC_TO_CCW | OP_LARGE_ARC_TO_CCW));
				DumpPoint(wr, pointList.Get()[curPt++]);
				wr.EndArray();
				wr.EndObject();
			}
		}

		if ((op & OP_CLOSE) != 0) {
			wr.StartObject();
			wr.Key("Close");
			wr.StartObject();
			wr.EndObject();
			wr.EndObject();
		}
	}
	wr.EndArray();
}

void DumpGradient(JsonWriter &wr, BPositionIO &rd)
{
	int32 type;
	int32 stopCount;
	BPoint pt;
	wr.StartObject();
	Read32(rd, type); wr.Key("type");
	switch (type) {
	case BGradient::TYPE_LINEAR: wr.String("LINEAR"); break;
	case BGradient::TYPE_RADIAL: wr.String("RADIAL"); break;
	case BGradient::TYPE_RADIAL_FOCUS: wr.String("RADIAL_FOCUS"); break;
	case BGradient::TYPE_DIAMOND: wr.String("DIAMOND"); break;
	case BGradient::TYPE_CONIC: wr.String("CONIC"); break;
	case BGradient::TYPE_NONE: wr.String("NONE"); break;
	default: wr.Int(type);
	}
	Read32(rd, stopCount);
	wr.Key("stops");
	wr.StartArray();
	for (int32 i = 0; i < stopCount; i++) {
		wr.StartArray();
		Read32(rd, val32); DumpColor(wr, val32);
		ReadFloat(rd, floatVal); wr.Double(floatVal);
		wr.EndArray();
	}
	wr.EndArray();
	switch (type) {
	case BGradient::TYPE_LINEAR:
		ReadPoint(rd, pt); wr.Key("start"); DumpPoint(wr, pt);
		ReadPoint(rd, pt); wr.Key("end"); DumpPoint(wr, pt);
		break;
	case BGradient::TYPE_RADIAL:
		ReadPoint(rd, pt); wr.Key("center"); DumpPoint(wr, pt);
		ReadFloat(rd, floatVal); wr.Key("radius"); wr.Double(floatVal);
		break;
	case BGradient::TYPE_RADIAL_FOCUS:
		ReadPoint(rd, pt); wr.Key("center"); DumpPoint(wr, pt);
		ReadPoint(rd, pt); wr.Key("focus"); DumpPoint(wr, pt);
		ReadFloat(rd, floatVal); wr.Key("radius"); wr.Double(floatVal);
		break;
	case BGradient::TYPE_DIAMOND:
		ReadPoint(rd, pt); wr.Key("center"); DumpPoint(wr, pt);
		break;
	case BGradient::TYPE_CONIC:
		ReadPoint(rd, pt); wr.Key("center"); DumpPoint(wr, pt);
		ReadFloat(rd, floatVal); wr.Key("angle"); wr.Double(floatVal);
		break;
	case BGradient::TYPE_NONE:
		break;
	}
	wr.EndObject();
}

void DumpOp(JsonWriter &wr, BPositionIO &rd, int16 op, int32 opSize)
{
	BPoint pt;
	BRect rc;
	BAffineTransform tr;
	pattern pat;
	BString str;

	wr.StartObject();
	switch (op) {
	case 0x0010: wr.Key("MOVE_PEN_BY"); {
		ReadPoint(rd, pt); DumpPoint(wr, pt);
		break;
	}
	case 0x0100: wr.Key("STROKE_LINE"); {
		wr.StartObject();
		ReadPoint(rd, pt); wr.Key("start"); DumpPoint(wr, pt);
		ReadPoint(rd, pt); wr.Key("end"); DumpPoint(wr, pt);
		wr.EndObject();
		break;
	}
	case 0x0101: wr.Key("STROKE_RECT"); {
		ReadRect(rd, rc); DumpRect(wr, rc);
		break;
	}
	case 0x0102: wr.Key("FILL_RECT"); {
		ReadRect(rd, rc); DumpRect(wr, rc);
		break;
	}
	case 0x0103: wr.Key("STROKE_ROUND_RECT"); {
		wr.StartObject();
		ReadRect(rd, rc); wr.Key("rect"); DumpRect(wr, rc);
		ReadPoint(rd, pt); wr.Key("radii"); DumpPoint(wr, pt);
		wr.EndObject();
		break;
	}
	case 0x0104: wr.Key("FILL_ROUND_RECT"); {
		wr.StartObject();
		ReadRect(rd, rc); wr.Key("rect"); DumpRect(wr, rc);
		ReadPoint(rd, pt); wr.Key("radii"); DumpPoint(wr, pt);
		wr.EndObject();
		break;
	}
	case 0x0105: wr.Key("STROKE_BEZIER"); wr.StartObject(); wr.EndObject(); break;
	case 0x0106: wr.Key("FILL_BEZIER"); wr.StartObject(); wr.EndObject(); break;
	case 0x010B: wr.Key("STROKE_POLYGON"); wr.StartObject(); wr.EndObject(); break;
	case 0x010C: wr.Key("FILL_POLYGON"); wr.StartObject(); wr.EndObject(); break;
	case 0x010D: wr.Key("STROKE_SHAPE"); {
		DumpShape(wr, rd);
		break;
	}
	case 0x010E: wr.Key("FILL_SHAPE"); {
		DumpShape(wr, rd);
		break;
	}
	case 0x010F: wr.Key("DRAW_STRING"); {
		wr.StartObject();
		off_t pos = rd.Position();
		ReadString(rd, str); wr.Key("string"); DumpString(wr, str);
		ReadFloat(rd, floatVal); wr.Key("escapementSpace"); wr.Double(floatVal);
		ReadFloat(rd, floatVal); wr.Key("escapementNonSpace"); wr.Double(floatVal);
		wr.EndObject();
		break;
	}
	case 0x0110: wr.Key("DRAW_PIXELS"); {
		wr.StartObject();
		off_t pos = rd.Position();
		ReadRect(rd, rc); wr.Key("sourceRect"); DumpRect(wr, rc);
		ReadRect(rd, rc); wr.Key("destinationRect"); DumpRect(wr, rc);
		Read32(rd, val32); wr.Key("width"); wr.Int(val32);
		Read32(rd, val32); wr.Key("height"); wr.Int(val32);
		Read32(rd, val32); wr.Key("bytesPerRow"); wr.Int(val32);
		Read32(rd, val32); wr.Key("colorSpace"); wr.Int(val32);
		Read32(rd, val32); wr.Key("flags"); wr.Int(val32);
		wr.Key("data");
		size_t size = opSize - (rd.Position() - pos);
		wr.StartArray();
		for (size_t i = 0; i < size; i++) {
			Read8(rd, val8); wr.Int(val8);
		}
		wr.EndArray();
		wr.EndObject();
		break;
	}
	case 0x0112: wr.Key("DRAW_PICTURE"); {
		wr.StartObject();
		ReadPoint(rd, pt); wr.Key("where"); DumpPoint(wr, pt);
		Read32(rd, val32); wr.Key("token"); wr.Int(val32);
		wr.EndObject();
		break;
	}
	case 0x0113: wr.Key("STROKE_ARC"); wr.StartObject(); wr.EndObject(); break;
	case 0x0114: wr.Key("FILL_ARC"); wr.StartObject(); wr.EndObject(); break;
	case 0x0115: wr.Key("STROKE_ELLIPSE"); {
		ReadRect(rd, rc); DumpRect(wr, rc);
		break;
	}
	case 0x0116: wr.Key("FILL_ELLIPSE"); {
		ReadRect(rd, rc); DumpRect(wr, rc);
		break;
	}
	case 0x0117: wr.Key("DRAW_STRING_LOCATIONS"); {
		wr.StartObject();
		off_t pos = rd.Position();
		int32 pointCount;
		Read32(rd, pointCount);
		wr.Key("pointList");
		wr.StartArray();
		for (int32 i = 0; i < pointCount; i++) {
			ReadPoint(rd, pt);
			DumpPoint(wr, pt);
		}
		wr.EndArray();
		ReadString(rd, str); wr.Key("string"); DumpString(wr, str);
		wr.EndObject();
		break;
	}

	case 0x0118: wr.Key("STROKE_RECT_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x0119: wr.Key("FILL_RECT_GRADIENT"); {
		wr.StartObject();
		ReadRect(rd, rc); wr.Key("rect"); DumpRect(wr, rc);
		wr.Key("gradient"); DumpGradient(wr, rd);
		wr.EndObject();
		break;
	}
	case 0x011A: wr.Key("STROKE_ROUND_RECT_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x011B: wr.Key("FILL_ROUND_RECT_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x011C: wr.Key("STROKE_BEZIER_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x011D: wr.Key("FILL_BEZIER_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x011E: wr.Key("STROKE_POLYGON_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x011F: wr.Key("FILL_POLYGON_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x0120: wr.Key("STROKE_SHAPE_GRADIENT"); {
		wr.StartObject();
		wr.Key("shape"); DumpShape(wr, rd);
		wr.Key("gradient"); DumpGradient(wr, rd);
		wr.EndObject();
		break;
	}
	case 0x0121: wr.Key("FILL_SHAPE_GRADIENT"); {
		wr.StartObject();
		wr.Key("shape"); DumpShape(wr, rd);
		wr.Key("gradient"); DumpGradient(wr, rd);
		wr.EndObject();
		break;
	}
	case 0x0122: wr.Key("STROKE_ARC_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x0123: wr.Key("FILL_ARC_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x0124: wr.Key("STROKE_ELLIPSE_GRADIENT"); wr.StartObject(); wr.EndObject(); break;
	case 0x0125: wr.Key("FILL_ELLIPSE_GRADIENT"); wr.StartObject(); wr.EndObject(); break;

	case 0x0200: wr.Key("ENTER_STATE_CHANGE");
		DumpOps(wr, rd, opSize);
		break;
	case 0x0201: wr.Key("SET_CLIPPING_RECTS"); {
		int32 numRects;
		Read32(rd, numRects);
		wr.StartArray();
		for (int32 i = 0; i < numRects; i++) {
			ReadRect(rd, rc);
			DumpRect(wr, rc);
		}
		wr.EndArray();
		break;
	}
	case 0x0202: wr.Key("CLIP_TO_PICTURE"); {
		wr.StartObject();
		Read32(rd, val32); wr.Key("token"); wr.Int(val32);
		ReadPoint(rd, pt); wr.Key("where"); DumpPoint(wr, pt);
		Read8(rd, val8); wr.Key("inverse"); DumpBool8(wr, val8);
		wr.EndObject();
		break;
	}
	case 0x0203: wr.Key("PUSH_STATE"); wr.StartObject(); wr.EndObject(); break;
	case 0x0204: wr.Key("POP_STATE"); wr.StartObject(); wr.EndObject(); break;
	case 0x0205: wr.Key("CLEAR_CLIPPING_RECTS"); wr.StartObject(); wr.EndObject(); break;
	case 0x0206: wr.Key("CLIP_TO_RECT"); {
		wr.StartObject();
		Read8(rd, val8); wr.Key("inverse"); DumpBool8(wr, val8);
		ReadRect(rd, rc); wr.Key("rect"); DumpRect(wr, rc);
		wr.EndObject();
		break;
	}
	case 0x0207: wr.Key("CLIP_TO_SHAPE"); {
		wr.StartObject();
		Read8(rd, val8); wr.Key("inverse"); DumpBool8(wr, val8);
		wr.Key("shape"); DumpShape(wr, rd);
		wr.EndObject();
		break;
	}
	case 0x0300: wr.Key("SET_ORIGIN"); {
		ReadPoint(rd, pt); DumpPoint(wr, pt);
		break;
	}
	case 0x0301: wr.Key("SET_PEN_LOCATION"); {
		ReadPoint(rd, pt); DumpPoint(wr, pt);
		break;
	}
	case 0x0302: wr.Key("SET_DRAWING_MODE"); {
		Read16(rd, val16);
		switch (val16) {
		case B_OP_COPY: wr.String("B_OP_COPY"); break;
		case B_OP_OVER: wr.String("B_OP_OVER"); break;
		case B_OP_ERASE: wr.String("B_OP_ERASE"); break;
		case B_OP_INVERT: wr.String("B_OP_INVERT"); break;
		case B_OP_ADD: wr.String("B_OP_ADD"); break;
		case B_OP_SUBTRACT: wr.String("B_OP_SUBTRACT"); break;
		case B_OP_BLEND: wr.String("B_OP_BLEND"); break;
		case B_OP_MIN: wr.String("B_OP_MIN"); break;
		case B_OP_MAX: wr.String("B_OP_MAX"); break;
		case B_OP_SELECT: wr.String("B_OP_SELECT"); break;
		case B_OP_ALPHA: wr.String("B_OP_ALPHA"); break;
		default: wr.Int(val16);
		}
		break;
	}
	case 0x0303: wr.Key("SET_LINE_MODE"); {
		wr.StartObject();
		Read16(rd, val16); wr.Key("capMode");
		switch (val16) {
		case B_ROUND_CAP: wr.String("B_ROUND_CAP"); break;
		case B_BUTT_CAP: wr.String("B_BUTT_CAP"); break;
		case B_SQUARE_CAP: wr.String("B_SQUARE_CAP"); break;
		default: wr.Int(val16);
		}
		Read16(rd, val16); wr.Key("joinMode");
		switch (val16) {
		case B_ROUND_JOIN: wr.String("B_ROUND_JOIN"); break;
		case B_MITER_JOIN: wr.String("B_MITER_JOIN"); break;
		case B_BEVEL_JOIN: wr.String("B_BEVEL_JOIN"); break;
		case B_BUTT_JOIN: wr.String("B_BUTT_JOIN"); break;
		case B_SQUARE_JOIN: wr.String("B_SQUARE_JOIN"); break;
		default: wr.Int(val16);
		}
		ReadFloat(rd, floatVal); wr.Key("miterLimit"); wr.Double(floatVal);
		wr.EndObject();
		break;
	}
	case 0x0304: wr.Key("SET_PEN_SIZE"); {
		ReadFloat(rd, floatVal); wr.Double(floatVal);
		break;
	}
	case 0x0305: wr.Key("SET_SCALE"); {
		ReadFloat(rd, floatVal); wr.Double(floatVal);
		break;
	}
	case 0x0306: wr.Key("SET_FORE_COLOR"); {
		Read32(rd, val32); DumpColor(wr, val32);
		break;
	}
	case 0x0307: wr.Key("SET_BACK_COLOR"); {
		Read32(rd, val32); DumpColor(wr, val32);
		break;
	}
	case 0x0308: wr.Key("SET_STIPLE_PATTERN"); {
		ReadPattern(rd, pat); DumpPattern(wr, pat);
		break;
	}
	case 0x0309: wr.Key("ENTER_FONT_STATE");
		DumpOps(wr, rd, opSize);
		break;
	case 0x030A: wr.Key("SET_BLENDING_MODE"); wr.StartObject(); wr.EndObject(); break;
	case 0x030B: wr.Key("SET_FILL_RULE"); {
		Read32(rd, val32);
		switch (val32) {
		case 0: wr.String("B_EVEN_ODD"); break;
		case 1: wr.String("B_NONZERO"); break;
		default: wr.Int(val32);
		}
		break;
	}
	case 0x0380: wr.Key("SET_FONT_FAMILY"); {
		ReadString(rd, str); DumpString(wr, str);
		break;
	}
	case 0x0381: wr.Key("SET_FONT_STYLE"); {
		ReadString(rd, str); DumpString(wr, str);
		break;
	}
	case 0x0382: wr.Key("SET_FONT_SPACING"); {
		Read32(rd, val32);
		switch (val32) {
		case 0: wr.String("B_CHAR_SPACING"); break;
		case 1: wr.String("B_STRING_SPACING"); break;
		case 2: wr.String("B_BITMAP_SPACING"); break;
		case 3: wr.String("B_FIXED_SPACING"); break;
		default: wr.Int(val32);
		}
		break;
	}
	case 0x0383: wr.Key("SET_FONT_ENCODING"); {
		Read32(rd, val32);
		switch (val32) {
		case 0: wr.String("B_UNICODE_UTF8"); break;
		case 1: wr.String("B_ISO_8859_1"); break;
		case 2: wr.String("B_ISO_8859_2"); break;
		case 3: wr.String("B_ISO_8859_3"); break;
		case 4: wr.String("B_ISO_8859_4"); break;
		case 5: wr.String("B_ISO_8859_5"); break;
		case 6: wr.String("B_ISO_8859_6"); break;
		case 7: wr.String("B_ISO_8859_7"); break;
		case 8: wr.String("B_ISO_8859_8"); break;
		case 9: wr.String("B_ISO_8859_9"); break;
		case 10: wr.String("B_ISO_8859_10"); break;
		case 11: wr.String("B_MACINTOSH_ROMAN"); break;
		default: wr.Int(val32);
		}
		break;
	}
	case 0x0384: wr.Key("SET_FONT_FLAGS"); {
		Read32(rd, val32); wr.StartArray();
		for (uint32 i = 0; i < 32; i++) {
			if ((1 << i) & (uint32)val32) {
				switch (i) {
				case 0: wr.String("B_DISABLE_ANTIALIASING"); break;
				case 1: wr.String("B_FORCE_ANTIALIASING"); break;
				default: wr.Int(i);
				}
			}
		}
		wr.EndArray();
		break;
	}
	case 0x0385: wr.Key("SET_FONT_SIZE"); {
		ReadFloat(rd, floatVal); wr.Double(floatVal);
		break;
	}
	case 0x0386: wr.Key("SET_FONT_ROTATE"); {
		ReadFloat(rd, floatVal); wr.Double(floatVal);
		break;
	}
	case 0x0387: wr.Key("SET_FONT_SHEAR"); {
		ReadFloat(rd, floatVal); wr.Double(floatVal);
		break;
	}
	case 0x0388: wr.Key("SET_FONT_BPP"); wr.StartObject(); wr.EndObject(); break;
	case 0x0389: wr.Key("SET_FONT_FACE"); {
		Read32(rd, val32); wr.StartArray();
		for (uint32 i = 0; i < 32; i++) {
			if ((1 << i) & (uint32)val32) {
				switch (i) {
				case 0: wr.String("B_ITALIC_FACE"); break;
				case 1: wr.String("B_UNDERSCORE_FACE"); break;
				case 2: wr.String("B_NEGATIVE_FACE"); break;
				case 3: wr.String("B_OUTLINED_FACE"); break;
				case 4: wr.String("B_STRIKEOUT_FACE"); break;
				case 5: wr.String("B_BOLD_FACE"); break;
				case 6: wr.String("B_REGULAR_FACE"); break;
				case 7: wr.String("B_CONDENSED_FACE"); break;
				case 8: wr.String("B_LIGHT_FACE"); break;
				case 9: wr.String("B_HEAVY_FACE"); break;
				default: wr.Int(i);
				}
			}
		}
		wr.EndArray();
		break;
	}
	case 0x0390: wr.Key("SET_TRANSFORM"); {
		ReadTransform(rd, tr); DumpTransform(wr, tr);
		break;
	}
	case 0x0391: wr.Key("AFFINE_TRANSLATE"); {
		wr.StartObject();
		ReadDouble(rd, doubleVal); wr.String("x"); wr.Double(doubleVal);
		ReadDouble(rd, doubleVal); wr.String("y"); wr.Double(doubleVal);
		wr.EndObject();
		break;
	}
	case 0x0392: wr.Key("AFFINE_SCALE"); {
		wr.StartObject();
		ReadDouble(rd, doubleVal); wr.String("x"); wr.Double(doubleVal);
		ReadDouble(rd, doubleVal); wr.String("y"); wr.Double(doubleVal);
		wr.EndObject();
		break;
	}
	case 0x0393: wr.Key("AFFINE_ROTATE"); {
		ReadDouble(rd, doubleVal); wr.Double(doubleVal);
		break;
	}
	case 0x0394: wr.Key("BLEND_LAYER"); wr.StartObject(); wr.EndObject(); break;
	default: wr.Key("UNKNOWN"); wr.Int(op); break;
	}
	wr.EndObject();
}

void DumpOps(JsonWriter &wr, BPositionIO &rd, int32 size)
{
	wr.StartArray();
	off_t beg = rd.Position();
	while (rd.Position() - beg < size) {
		int16 op;
		int32 opSize;
		Read16(rd, op);
		Read32(rd, opSize);
		off_t pos = rd.Position();
		switch (op) {
		case 0x0203 /* PUSH_STATE */:
			wr.StartObject();
			wr.Key("GROUP");
			wr.StartArray();
			break;
		case 0x0204 /* POP_STATE */:
			wr.EndArray();
			wr.EndObject();
			break;
		default:
			DumpOp(wr, rd, op, opSize);
			break;
		}
		rd.Seek(pos + opSize, SEEK_SET);
	}
	wr.EndArray();
}

void DumpPictureFile(JsonWriter &wr, BPositionIO &rd)
{
	wr.StartObject();
	int32 count;
	int32 size;
	Read32(rd, val32); wr.Key("version"); wr.Int(val32);
	Read32(rd, val32); wr.Key("unknown"); wr.Int(val32);
	Read32(rd, count);
	if (count > 0) {
	wr.Key("pictures");
		wr.StartArray();
		for (int32 i = 0; i < count; i++) {
			DumpPictureFile(wr, rd);
		}
		wr.EndArray();
	}
	Read32(rd, size);
	wr.Key("ops");
	DumpOps(wr, rd, size);
	wr.EndObject();
}


int main(int argCnt, char **args)
{
	if (argCnt < 2)
		return 1;

	rapidjson::OStreamWrapper os(std::cout);
	JsonWriter wr(os);

	BFile file(args[1], B_READ_ONLY);
	DumpPictureFile(wr, file);
	return 0;
}
