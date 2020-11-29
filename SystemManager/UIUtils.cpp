#include "UIUtils.h"

#include <stdio.h>


IconStringField::IconStringField(BBitmap *icon, const char *string):
	BStringField(string), fIcon(icon)
{}


IconStringColumn::IconStringColumn(
	const char* title, float width,
	float minWidth, float maxWidth, uint32 truncate,
	alignment align
): BStringColumn(title, width, minWidth, maxWidth, truncate, align)
{}

void IconStringColumn::DrawField(BField* _field, BRect rect, BView* parent)
{
	IconStringField *field = (IconStringField*)_field;

	parent->PushState();
	parent->SetDrawingMode(B_OP_ALPHA);
	parent->DrawBitmap(field->Icon(), rect.LeftTop() + BPoint(4, 0));
	parent->PopState();
	rect.left += field->Icon()->Bounds().Width() + 1;

	BStringColumn::DrawField(field, rect, parent);
}

float IconStringColumn::GetPreferredWidth(BField* field, BView* parent) const
{
	return BStringColumn::GetPreferredWidth(field, parent) + dynamic_cast<IconStringField*>(field)->Icon()->Bounds().Width() + 1;
}

bool IconStringColumn::AcceptsField(const BField* field) const
{
	return dynamic_cast<const IconStringField*>(field) != NULL;
}


HexIntegerColumn::HexIntegerColumn(
	const char* title,
	float width, float minWidth, float maxWidth,
	alignment align
): BTitledColumn(title, width, minWidth, maxWidth, align)
{}

void HexIntegerColumn::DrawField(BField *field, BRect rect, BView* parent)
{
	char formatted[256];
	float width = rect.Width() - (2 * 8);
	BString string;

	sprintf(formatted, "0x%x", (int)((BIntegerField*)field)->Value());

	string = formatted;

	BFont oldFont;
	parent->GetFont(&oldFont);
	parent->SetFont(be_fixed_font);
	parent->TruncateString(&string, B_TRUNCATE_MIDDLE, width + 2);
	DrawString(string.String(), parent, rect);
	parent->SetFont(&oldFont);
}

int HexIntegerColumn::CompareFields(BField *field1, BField *field2)
{
	return (((BIntegerField*)field1)->Value() - ((BIntegerField*)field2)->Value());
}
