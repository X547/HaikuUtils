#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Shape.h>

#include <stdio.h>
#include <stdlib.h>

#include <optional>
#include <string_view>

#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

#include <hb.h>
#include <hb-ft.h>


static void RaiseError()
{
	fprintf(stderr, "[!] error\n");
	abort();
}

static void Assert(bool cond)
{
	if (!cond) {
		RaiseError();
	}
}


class FreeType {
private:
	friend class FreeTypeFace;

	FT_Library fLibrary {};

public:
	FreeType();
	~FreeType();

	static void Check(FT_Error err);
};


FreeType::FreeType()
{
	Check(FT_Init_FreeType(&fLibrary));
}

FreeType::~FreeType()
{
	if (fLibrary != NULL) {
		FT_Done_FreeType(fLibrary);
	}
}

void FreeType::Check(FT_Error err)
{
	if (err != 0) {
		fprintf(stderr, "[!] FreeType error %d\n", err);
		abort();
	}
}


class FreeTypeFace {
private:
	friend class HarfBuzzFont;

	FT_Face fFace {};

public:
	FreeTypeFace(FreeType &ft, const char *fileName);
	~FreeTypeFace();

	uint32_t FindGlyphID(uint32_t charCode);

	void SetSize(float size);
	void LoadGlyph(uint32_t glyphId);
	void DrawGlyph(BView &view, uint32_t glyphId);
};


FreeTypeFace::FreeTypeFace(FreeType &ft, const char *fileName)
{
	FreeType::Check(FT_New_Face(ft.fLibrary, fileName, 0, &fFace));
}

FreeTypeFace::~FreeTypeFace()
{
	if (fFace != NULL) {
		FT_Done_Face(fFace);
	}
}

uint32_t FreeTypeFace::FindGlyphID(uint32_t charCode)
{
	return FT_Get_Char_Index(fFace, charCode);
}

void FreeTypeFace::SetSize(float size)
{
	FT_Size_RequestRec tReqSize;
	tReqSize.type			= FT_SIZE_REQUEST_TYPE_NOMINAL;
	tReqSize.width			= 0;
	tReqSize.height			= (uint32_t)(size * (1 << 6));
	tReqSize.horiResolution	= 0;
	tReqSize.vertResolution	= 0;
	FreeType::Check(FT_Request_Size(fFace, &tReqSize));
}

void FreeTypeFace::LoadGlyph(uint32_t glyphId)
{
	FreeType::Check(FT_Load_Glyph(fFace, glyphId, FT_LOAD_DEFAULT));
}

void FreeTypeFace::DrawGlyph(BView &view, uint32_t glyphId)
{
	FreeType::Check(FT_Load_Glyph(fFace, glyphId, FT_LOAD_DEFAULT));

	FT_GlyphSlot slot = fFace->glyph;

	BShape shape;

	static FT_Outline_Funcs funcs = {
		.move_to = [](const FT_Vector *to, void *user) {
			static_cast<BShape*>(user)->MoveTo(BPoint((float)to->x / (1 << 6), -(float)to->y / (1 << 6)));
			return 0;
		},
		.line_to = [](const FT_Vector *to, void *user) {
			static_cast<BShape*>(user)->LineTo(BPoint((float)to->x / (1 << 6), -(float)to->y / (1 << 6)));
			return 0;
		},
		.conic_to = [](const FT_Vector *control, const FT_Vector *to, void *user) {
			auto shape = static_cast<BShape*>(user);
			BPoint q[] = {
				shape->CurrentPosition(),
				BPoint((float)control->x / (1 << 6), -(float)control->y / (1 << 6)),
				BPoint((float)to->x / (1 << 6), -(float)to->y / (1 << 6))
			};
			static_cast<BShape*>(user)->BezierTo(
				BPoint(q[0].x + 2.0f/3.0f*(q[1].x - q[0].x), q[0].y + 2.0f/3.0f*(q[1].y - q[0].y)),
				BPoint(q[2].x + 2.0f/3.0f*(q[1].x - q[2].x), q[2].y + 2.0f/3.0f*(q[1].y - q[2].y)),
				q[2]
			);
			return 0;
		},
		.cubic_to = [](const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user) {
			static_cast<BShape*>(user)->BezierTo(
				BPoint((float)control1->x / (1 << 6), -(float)control1->y / (1 << 6)),
				BPoint((float)control2->x / (1 << 6), -(float)control2->y / (1 << 6)),
				BPoint((float)to->x / (1 << 6), -(float)to->y / (1 << 6))
			);
			return 0;
		},
	};

	FT_Outline_Decompose(&slot->outline, &funcs, &shape);

	view.FillShape(&shape);
}


class HarfBuzzFont {
private:
	FreeTypeFace &fFTFace;
	hb_font_t *fFont {};

public:
	HarfBuzzFont(FreeTypeFace &ftFace);
	~HarfBuzzFont();

	hb_font_t *Handle() {return fFont;}

	void DrawString(BView &view, std::string_view str, hb_direction_t direction, hb_script_t script);
};


HarfBuzzFont::HarfBuzzFont(FreeTypeFace &ftFace):
	fFTFace(ftFace)
{
	fFont = hb_ft_font_create(ftFace.fFace, NULL);
	Assert(fFont != NULL);
}

HarfBuzzFont::~HarfBuzzFont()
{
	if (fFont != NULL) {
		hb_font_destroy(fFont);
	}
}

void HarfBuzzFont::DrawString(BView &view, std::string_view str, hb_direction_t direction, hb_script_t script)
{
	hb_buffer_t* buffer = hb_buffer_create();
	hb_buffer_reset(buffer);

	hb_buffer_set_direction(buffer, direction);
	hb_buffer_set_script(buffer, script);
	hb_buffer_set_language(buffer, hb_language_from_string(str.data(), str.size()));
	hb_buffer_add_utf8(buffer, str.data(), str.size(), 0, str.size());

	hb_shape(fFont, buffer, NULL, 0);

	unsigned int glyphCount;
	hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buffer, &glyphCount);
	hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(buffer, &glyphCount);

	for (unsigned int i = 0; i < glyphCount; ++i) {
		BPoint advance((float)glyphPos[i].x_advance / 64, -(float)glyphPos[i].y_advance / 64);
		BPoint offset((float)glyphPos[i].x_offset / 64, -(float)glyphPos[i].y_offset / 64);

		BPoint org = view.PenLocation();
		view.MovePenTo(org + offset);
		fFTFace.DrawGlyph(view, glyphInfo[i].codepoint);
		view.MovePenTo(org + advance);
	}

	hb_buffer_destroy(buffer);
}


class TestView: public BView {
private:
	FreeType fFt;
	std::optional<FreeTypeFace> fFace1;
	std::optional<HarfBuzzFont> fHBFont1;
	std::optional<FreeTypeFace> fFace2;
	std::optional<HarfBuzzFont> fHBFont2;

public:
	TestView(BRect frame, const char *name);

	void Draw(BRect dirty) final;
};


TestView::TestView(BRect frame, const char *name):
	BView(frame, name, B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_SUBPIXEL_PRECISE | B_FRAME_EVENTS)
{
	fFace1.emplace(fFt, "/boot/system/data/fonts/ttfonts/DejaVuSans.ttf");
	fFace1.value().SetSize(36);
	fHBFont1.emplace(fFace1.value());

	fFace2.emplace(fFt, "/boot/system/data/fonts/otfonts/NotoSansCJKjp-VF.otf");
	fFace2.value().SetSize(36);
	fHBFont2.emplace(fFace2.value());
}

void TestView::Draw(BRect dirty)
{
	MovePenTo(10, 50);
	fHBFont1.value().DrawString(*this, "This is a test 123.", HB_DIRECTION_LTR, HB_SCRIPT_LATIN);
	MovePenTo(100, 100);
	fHBFont1.value().DrawString(*this, "تسجّل يتكلّم", HB_DIRECTION_RTL, HB_SCRIPT_ARABIC);
	MovePenTo(400, 20);
	fHBFont2.value().DrawString(*this, "「テスト1」、「テスト2」。", HB_DIRECTION_TTB, HB_SCRIPT_HAN);
}


int main()
{
	BApplication app("application/x-vnd.test.app");

	BWindow *wnd = new BWindow(
		BRect(0, 0, 450, 550),
		"FreeType2 & HarfBuzz Demo",
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
	);
	BView *view = new TestView(wnd->Frame().OffsetToCopy(B_ORIGIN), "view");
	view->SetResizingMode(B_FOLLOW_ALL);
	wnd->AddChild(view);

	wnd->CenterOnScreen();
	wnd->Show();

	app.Run();

	return 0;
}
