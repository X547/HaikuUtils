#include "SuiteEditor.h"

#include <PropertyInfo.h>

#include <LayoutBuilder.h>
#include <TextControl.h>
#include <StringView.h>
#include <CheckBox.h>
#include <ScrollView.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Box.h>


BLayoutItem *CreateTextControlLayoutItem(BTextControl *view)
{
	BGroupLayout *layout;
		BLayoutBuilder::Group<>(B_VERTICAL, 0)
			.GetLayout(&layout)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(view->CreateLabelLayoutItem())
				.AddGlue()
				.End()
			.Add(view->CreateTextViewLayoutItem())
			.End();
	return layout;
}

BLayoutItem *CreateMenuFieldLayoutItem(BMenuField *view)
{
	BGroupLayout *layout;
		BLayoutBuilder::Group<>(B_VERTICAL, 0)
			.GetLayout(&layout)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(view->CreateLabelLayoutItem())
				.AddGlue()
				.End()
			.AddStrut(16)
			.Add(view->CreateMenuBarLayoutItem())
			.End();
	return layout;
}

BBox *NewLabelBox(const char *name, const char *label, BView *content)
{
	BBox *box = new BBox(name);
	if (label != NULL)
		box->SetLabel(label);
	if (content != NULL)
		box->AddChild(content);
	return box;
}

BView *NewColorView(const char *name, rgb_color color)
{
	BView *view = new BView(name, B_SUPPORTS_LAYOUT);
	view->SetViewColor(color);
	if (color == B_TRANSPARENT_COLOR)
		view->SetFlags(view->Flags() | B_TRANSPARENT_BACKGROUND);
	return view;
}


SuiteEditor::SuiteEditor(BMessenger handle):
	BWindow(BRect(), "Suite Editor", B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fHandle(handle),
	fStatus(B_OK)
{
	BMessage spec(B_GET_SUPPORTED_SUITES);
	BMessage suites;
	fStatus = fHandle.SendMessage(&spec, &suites, 1000000, 1000000);
	if (fStatus < B_OK) return;
	suites.PrintToStream();

	BGroupLayout* suitesLayout;

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING)
		.GetLayout(&suitesLayout)
		.AddGlue()
		.End();

	const char *suiteName;
	for (int32 i = 0; suites.FindString("suites", i, &suiteName) >= B_OK; i++) {
		BPropertyInfo propInfo;
		fStatus = suites.FindFlat("messages", i, &propInfo);
		if (fStatus < B_OK) return;
		const property_info *propList = propInfo.Properties();
		propInfo.PrintToStream();

		BGroupLayout* propsLayout;

		suitesLayout->AddView(
			NewLabelBox("suite", suiteName,
				BLayoutBuilder::Group<>(NewColorView("content", B_TRANSPARENT_COLOR), B_VERTICAL, B_USE_SMALL_SPACING)
					.SetInsets(B_USE_SMALL_SPACING, 0, B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
					.GetLayout(&propsLayout)
					.View()
			)
		);

		for (int32 j = 0; propList[j].name != NULL; j++) {
			propsLayout->AddItem(CreateTextControlLayoutItem(new BTextControl("prop", propList[j].name, "0", NULL)));
		}
	}

}

status_t SuiteEditor::InitCheck()
{
	return fStatus;
}

