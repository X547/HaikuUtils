#include <View.h>
#include <Shape.h>
#include <Picture.h>
#include <Rect.h>
#include <String.h>
#include <File.h>
#include <private/interface/ShapePrivate.h>
#include <stdio.h>
#include <math.h>

#include <private/shared/AutoDeleter.h>

int32 indent = 0;

void Indent()
{
	for (int32 i = 0; i < indent; i++) {
		printf("\t");
	}
}

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
void ReadStringLen(BDataIO &rd, BString &val, size_t len) {
	val = "";
	for (; len > 0; len--) {
		char ch;
		rd.Read(&ch, sizeof(ch));
		val += ch;
	}
}

void DumpBool(bool val) {if (val) printf("true"); else printf("false");}
void DumpBool8(int8 val) {if (val == 1) printf("true"); else if (val == 0) printf("false"); else printf("?(%d)", val);}
void DumpPoint(const BPoint &pt) {printf("(%g, %g)", pt.x, pt.y);}
void DumpRect(const BRect &rc) {printf("(%g, %g, %g, %g)", rc.left, rc.top, rc.right, rc.bottom);}

void DumpPattern(const pattern &pat) {
	if (memcmp(&pat, &B_SOLID_HIGH, sizeof(pattern)) == 0)
		printf("B_SOLID_HIGH");
	else if (memcmp(&pat, &B_SOLID_LOW, sizeof(pattern)) == 0)
		printf("B_SOLID_LOW");
	else if (memcmp(&pat, &B_MIXED_COLORS, sizeof(pattern)) == 0)
		printf("B_MIXED_COLORS");
	else {
		printf("(");
		for (int32 i = 0; i < 8; i++) {
			if (i > 0) printf(", ");
			printf("0x%02x", pat.data[i]);
		}
		printf(")");
	}
}

void DumpTransform(const BAffineTransform &tr)
{
	printf("(tx: %g, ty: %g, sx: %g, sy: %g, shy: %g, shx: %g)", tr.tx, tr.ty, tr.sx, tr.sy, tr.shy, tr.shx);
}

void DumpString(const BString &str)
{
	printf("\"%s\"", str.String());
}

void DumpOps(BPositionIO &rd, int32 size);

void DumpShape(BPositionIO &rd)
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

	printf("{\n");
	indent++;
	for (; curOp < opCount; curOp++) {
		int32 op = opList.Get()[curOp] & 0xFF000000;
		int32 count = opList.Get()[curOp] & 0x00FFFFFF;

		if ((op & OP_MOVETO) != 0) {
			Indent(); printf("MoveTo("); DumpPoint(pointList.Get()[curPt++]); printf(")\n");
		}

		if ((op & OP_LINETO) != 0) {
			for (int32 i = 0; i < count; i++) {
				Indent(); printf("LineTo("); DumpPoint(pointList.Get()[curPt++]); printf(")\n");
			}
		}

		if ((op & OP_BEZIERTO) != 0) {
			for (int32 i = 0; i < count; i += 3) {
				Indent(); printf("BezierTo(");
				DumpPoint(pointList.Get()[curPt++]); printf(", ");
				DumpPoint(pointList.Get()[curPt++]); printf(", ");
				DumpPoint(pointList.Get()[curPt++]);
				printf(")\n");
			}
		}

		if ((op & OP_LARGE_ARC_TO_CW) != 0 || (op & OP_LARGE_ARC_TO_CCW) != 0
			|| (op & OP_SMALL_ARC_TO_CW) != 0
			|| (op & OP_SMALL_ARC_TO_CCW) != 0) {
			for (int32 i = 0; i < count; i += 3) {
				Indent(); printf("ArcTo(");
				DumpPoint(pointList.Get()[curPt++]); printf(", ");
				printf("%g, ", pointList.Get()[curPt++].x);
				DumpBool(op & (OP_LARGE_ARC_TO_CW | OP_LARGE_ARC_TO_CCW)); printf(", ");
				DumpBool(op & (OP_SMALL_ARC_TO_CCW | OP_LARGE_ARC_TO_CCW)); printf(", ");
				DumpPoint(pointList.Get()[curPt++]);
				printf(")\n");
			}
		}

		if ((op & OP_CLOSE) != 0) {
			Indent(); printf("Close()\n");
		}
	}
	indent--;
	Indent(); printf("}");
}

void DumpOp(BPositionIO &rd, int16 op, int32 opSize)
{
	BPoint pt;
	BRect rc;
	BAffineTransform tr;
	pattern pat;
	BString str;

	Indent();
	switch (op) {
	case 0x0010: printf("MOVE_PEN_BY\n"); {indent++;
		Indent(); ReadPoint(rd, pt); printf("where: "); DumpPoint(pt);
		indent--; break;
	}
	case 0x0100: printf("STROKE_LINE\n"); {indent++;
		Indent(); ReadPoint(rd, pt); printf("start: "); DumpPoint(pt); printf("\n");
		Indent(); ReadPoint(rd, pt); printf("end: "); DumpPoint(pt);
		indent--; break;
	}
	case 0x0101: printf("STROKE_RECT\n"); {indent++;
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc);
		indent--; break;
	}
	case 0x0102: printf("FILL_RECT\n"); {indent++;
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc);
		indent--; break;
	}
	case 0x0103: printf("STROKE_ROUND_RECT\n"); {indent++;
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc); printf("\n");
		Indent(); ReadPoint(rd, pt); printf("radii: "); DumpPoint(pt);
		indent--; break;
	}
	case 0x0104: printf("FILL_ROUND_RECT\n"); {indent++;
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc); printf("\n");
		Indent(); ReadPoint(rd, pt); printf("radii: "); DumpPoint(pt);
		indent--; break;
	}
	case 0x0105: printf("STROKE_BEZIER"); break;
	case 0x0106: printf("FILL_BEZIER"); break;
	case 0x010B: printf("STROKE_POLYGON"); break;
	case 0x010C: printf("FILL_POLYGON"); break;
	case 0x010D: printf("STROKE_SHAPE\n"); {indent++;
		Indent(); printf("shape: "); DumpShape(rd);
		indent--; break;
	}
	case 0x010E: printf("FILL_SHAPE\n"); {indent++;
		Indent(); printf("shape: "); DumpShape(rd);
		indent--; break;
	}
	case 0x010F: printf("DRAW_STRING\n"); {indent++;
		off_t pos = rd.Position();
		Indent(); ReadFloat(rd, floatVal); printf("escapementSpace: %g\n", floatVal);
		Indent(); ReadFloat(rd, floatVal); printf("escapementNonSpace: %g\n", floatVal);
		Indent(); ReadStringLen(rd, str, opSize - (rd.Position() - pos)); printf("string: "); DumpString(str);
		indent--; break;
	}
	case 0x0110: printf("DRAW_PIXELS\n"); {indent++;
		off_t pos = rd.Position();
		Indent(); ReadRect(rd, rc); printf("sourceRect: "); DumpRect(rc); printf("\n");
		Indent(); ReadRect(rd, rc); printf("destinationRect: "); DumpRect(rc); printf("\n");
		Indent(); Read32(rd, val32); printf("width: %d\n", val32);
		Indent(); Read32(rd, val32); printf("height: %d\n", val32);
		Indent(); Read32(rd, val32); printf("bytesPerRow: %d\n", val32);
		Indent(); Read32(rd, val32); printf("colorSpace: %d\n", val32);
		Indent(); Read32(rd, val32); printf("flags: %d\n", val32);
		Indent(); printf("data: <%d bytes>", opSize - (rd.Position() - pos));
		indent--; break;
	}
	case 0x0112: printf("DRAW_PICTURE\n"); {indent++;
		Indent(); ReadPoint(rd, pt); printf("where: "); DumpPoint(pt); printf("\n");
		Indent(); Read32(rd, val32); printf("token: %d", val32);
		indent--; break;
	}
	case 0x0113: printf("STROKE_ARC"); break;
	case 0x0114: printf("FILL_ARC"); break;
	case 0x0115: printf("STROKE_ELLIPSE\n"); {indent++;
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc);
		indent--; break;
	}
	case 0x0116: printf("FILL_ELLIPSE\n"); {indent++;
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc);
		indent--; break;
	}
	case 0x0117: printf("DRAW_STRING_LOCATIONS\n"); {indent++;
		off_t pos = rd.Position();
		int32 pointCount;
		Read32(rd, pointCount);
		Indent(); printf("pointList: {\n");
		indent++;
		for (int32 i = 0; i < pointCount; i++) {
			ReadPoint(rd, pt);
			Indent(); DumpPoint(pt); printf("\n");
		}
		indent--;
		Indent(); printf("}\n");
		Indent(); ReadStringLen(rd, str, opSize - (rd.Position() - pos)); printf("string: "); DumpString(str);
		indent--; break;
	}
	case 0x0200: printf("ENTER_STATE_CHANGE {\n");
		indent++;
		DumpOps(rd, opSize);
		indent--;
		Indent(); printf("}");
		break;
	case 0x0201: printf("SET_CLIPPING_RECTS\n"); {indent++;
		int32 numRects;
		Read32(rd, numRects);
		Indent(); printf("rects: {\n");
		indent++;
		for (int32 i = 0; i < numRects; i++) {
			ReadRect(rd, rc);
			Indent(); DumpRect(rc); printf("\n");
		}
		indent--;
		Indent(); printf("}");
		indent--; break;
	}
	case 0x0202: printf("CLIP_TO_PICTURE"); break;
	case 0x0203: printf("PUSH_STATE"); indent++; break;
	case 0x0204: printf("POP_STATE"); indent--; break;
	case 0x0205: printf("CLEAR_CLIPPING_RECTS"); break;
	case 0x0206: printf("CLIP_TO_RECT\n"); {indent++;
		Indent(); Read8(rd, val8); printf("inverse: "); DumpBool8(val8); printf("\n");
		Indent(); ReadRect(rd, rc); printf("rect: "); DumpRect(rc);
		indent--; break;
	}
	case 0x0207: printf("CLIP_TO_SHAPE\n"); {indent++;
		Indent(); Read8(rd, val8); printf("inverse: "); DumpBool8(val8); printf("\n");
		Indent(); printf("shape: "); DumpShape(rd);
		indent--; break;
	}
	case 0x0300: printf("SET_ORIGIN\n"); {indent++;
		Indent(); ReadPoint(rd, pt); printf("origin: "); DumpPoint(pt);
		indent--; break;
	}
	case 0x0301: printf("SET_PEN_LOCATION\n"); {indent++;
		Indent(); ReadPoint(rd, pt); printf("location: "); DumpPoint(pt);
		indent--; break;
	}
	case 0x0302: printf("SET_DRAWING_MODE\n"); {indent++;
		Indent(); Read16(rd, val16); printf("mode: ");
		switch (val16) {
		case B_OP_COPY: printf("B_OP_COPY"); break;
		case B_OP_OVER: printf("B_OP_OVER"); break;
		case B_OP_ERASE: printf("B_OP_ERASE"); break;
		case B_OP_INVERT: printf("B_OP_INVERT"); break;
		case B_OP_ADD: printf("B_OP_ADD"); break;
		case B_OP_SUBTRACT: printf("B_OP_SUBTRACT"); break;
		case B_OP_BLEND: printf("B_OP_BLEND"); break;
		case B_OP_MIN: printf("B_OP_MIN"); break;
		case B_OP_MAX: printf("B_OP_MAX"); break;
		case B_OP_SELECT: printf("B_OP_SELECT"); break;
		case B_OP_ALPHA: printf("B_OP_ALPHA"); break;
		default: printf("?(%d)", val16);
		}
		indent--; break;
	}
	case 0x0303: printf("SET_LINE_MODE\n"); {indent++;
		Indent(); Read16(rd, val16); printf("capMode: ");
		switch (val16) {
		case B_ROUND_CAP: printf("B_ROUND_CAP"); break;
		case B_BUTT_CAP: printf("B_BUTT_CAP"); break;
		case B_SQUARE_CAP: printf("B_SQUARE_CAP"); break;
		default: printf("?(%d)", val16);
		}
		printf("\n");
		Indent(); Read16(rd, val16); printf("joinMode: ");
		switch (val16) {
		case B_ROUND_JOIN: printf("B_ROUND_JOIN"); break;
		case B_MITER_JOIN: printf("B_MITER_JOIN"); break;
		case B_BEVEL_JOIN: printf("B_BEVEL_JOIN"); break;
		case B_BUTT_JOIN: printf("B_BUTT_JOIN"); break;
		case B_SQUARE_JOIN: printf("B_SQUARE_JOIN"); break;
		default: printf("?(%d)", val16);
		}
		printf("\n");
		Indent(); ReadFloat(rd, floatVal); printf("miterLimit: %g", floatVal);
		indent--; break;
	}
	case 0x0304: printf("SET_PEN_SIZE\n"); {indent++;
		Indent(); ReadFloat(rd, floatVal); printf("penSize: %g", floatVal);
		indent--; break;
	}
	case 0x0305: printf("SET_SCALE\n"); {indent++;
		Indent(); ReadFloat(rd, floatVal); printf("scale: %g", floatVal);
		indent--; break;
	}
	case 0x0306: printf("SET_FORE_COLOR\n"); {indent++;
		Indent(); Read32(rd, val32); printf("color: 0x%08x", val32);
		indent--; break;
	}
	case 0x0307: printf("SET_BACK_COLOR\n"); {indent++;
		Indent(); Read32(rd, val32); printf("color: 0x%08x", val32);
		indent--; break;
	}
	case 0x0308: printf("SET_STIPLE_PATTERN\n"); {indent++;
		Indent(); ReadPattern(rd, pat); printf("pattern: "); DumpPattern(pat);
		indent--; break;
	}
	case 0x0309: printf("ENTER_FONT_STATE {\n");
		indent++;
		DumpOps(rd, opSize);
		indent--;
		Indent(); printf("}");
		break;
	case 0x030A: printf("SET_BLENDING_MODE"); break;
	case 0x0380: printf("SET_FONT_FAMILY\n"); {indent++;
		Indent(); ReadStringLen(rd, str, opSize); printf("family: "); DumpString(str);
		indent--; break;
	}
	case 0x0381: printf("SET_FONT_STYLE\n"); {indent++;
		Indent(); ReadStringLen(rd, str, opSize); printf("style: "); DumpString(str);
		indent--; break;
	}
	case 0x0382: printf("SET_FONT_SPACING"); break;
	case 0x0383: printf("SET_FONT_ENCODING"); break;
	case 0x0384: printf("SET_FONT_FLAGS"); break;
	case 0x0385: printf("SET_FONT_SIZE\n"); {indent++;
		Indent(); ReadFloat(rd, floatVal); printf("size: %g", floatVal);
		indent--; break;
	}
	case 0x0386: printf("SET_FONT_ROTATE\n"); {indent++;
		Indent(); ReadFloat(rd, floatVal); printf("rotation: %g", floatVal);
		indent--; break;
	}
	case 0x0387: printf("SET_FONT_SHEAR\n"); {indent++;
		Indent(); ReadFloat(rd, floatVal); printf("shear: %g", floatVal);
		indent--; break;
	}
	case 0x0388: printf("SET_FONT_BPP"); break;
	case 0x0389: printf("SET_FONT_FACE"); break;
	case 0x0390: printf("SET_TRANSFORM\n"); {indent++;
		Indent(); ReadTransform(rd, tr); printf("transform: "); DumpTransform(tr);
		indent--; break;
	}
	case 0x0391: printf("AFFINE_TRANSLATE\n"); {indent++;
		Indent(); ReadDouble(rd, doubleVal); printf("x: %g\n", doubleVal);
		Indent(); ReadDouble(rd, doubleVal); printf("y: %g", doubleVal);
		indent--; break;
	}
	case 0x0392: printf("AFFINE_SCALE\n"); {indent++;
		Indent(); ReadDouble(rd, doubleVal); printf("x: %g\n", doubleVal);
		Indent(); ReadDouble(rd, doubleVal); printf("y: %g", doubleVal);
		indent--; break;
	}
	case 0x0393: printf("AFFINE_ROTATE\n"); {indent++;
		Indent(); ReadDouble(rd, doubleVal); printf("angleRadians: %g", doubleVal);
		indent--; break;
	}
	case 0x0394: printf("BLEND_LAYER"); break;
	default: printf("? (0x%x)", op);
	}
	printf("\n");
}

void DumpOps(BPositionIO &rd, int32 size)
{
	off_t beg = rd.Position();
	while (rd.Position() - beg < size) {
		int16 op;
		int32 opSize;
		Read16(rd, op);
		Read32(rd, opSize);
		off_t pos = rd.Position();
		DumpOp(rd, op, opSize);
		rd.Seek(pos + opSize, SEEK_SET);
	}
}

void DumpPictureFile(BPositionIO &rd)
{
	int32 count;
	int32 size;
	Read32(rd, val32); Indent(); printf("version: %d\n", val32);
	Read32(rd, val32); Indent(); printf("unknown: %d\n", val32);
	Read32(rd, count); Indent(); printf("count: %d\n", count);
	Indent(); printf("pictures:\n");
	indent++;
	for (int32 i = 0; i < count; i++) {
		DumpPictureFile(rd);
	}
	indent--;
	Read32(rd, size); printf("size: %d\n", size);
	printf("ops:\n");
	indent++;
	DumpOps(rd, size);
	indent--;
}

void DumpPicture(BPicture &pict)
{
	BMallocIO data;
	pict.Flatten(&data);
	data.Seek(0, SEEK_SET);
	DumpPictureFile(data);
}


int main(int argCnt, char **args)
{
	if (argCnt < 2)
		return 1;

	BFile file(args[1], B_READ_ONLY);
	DumpPictureFile(file);
	return 0;
}
