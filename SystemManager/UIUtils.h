#ifndef _UIUTILS_H_
#define _UIUTILS_H_


#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/shared/AutoDeleter.h>


class IconStringField: public BStringField
{
private:
	ObjectDeleter<BBitmap> fIcon;

public:
	IconStringField(BBitmap *icon, const char *string);
	inline BBitmap *Icon() {return fIcon.Get();}
};

class IconStringColumn: public BStringColumn
{
public:
	IconStringColumn(
		const char* title, float width,
		float minWidth, float maxWidth, uint32 truncate,
		alignment align = B_ALIGN_LEFT
	);

	void DrawField(BField* _field, BRect rect, BView* parent);
	float GetPreferredWidth(BField* field, BView* parent) const;
	bool AcceptsField(const BField* field) const;
};

class HexIntegerColumn: public BTitledColumn
{
public:
	HexIntegerColumn(
		const char* title,
		float width, float minWidth, float maxWidth,
		alignment align
	);

	void DrawField(BField *field, BRect rect, BView* parent);
	int CompareFields(BField *field1, BField *field2);
};

class Int64Field: public BField
{
public:
	Int64Field(int64 value);
	void SetValue(int64 value);
	int64 Value();

private:
	int64 fValue;
};


#endif	// _UIUTILS_H_
