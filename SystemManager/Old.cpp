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

static BView *NewColorView(const char *name, rgb_color color)
{
	BView *view = new BView(name, B_SUPPORTS_LAYOUT);
	view->SetViewColor(color);
	if (color == B_TRANSPARENT_COLOR)
		view->SetFlags(view->Flags() | B_TRANSPARENT_BACKGROUND);
	return view;
}

static BView *NewInfoView(TeamWindow *wnd)
{
	BView *view = new BView("Info", B_TRANSPARENT_BACKGROUND);
	view->SetViewColor(B_TRANSPARENT_COLOR);

	BTextControl *fTeam, *fPort, *fToken;
	BTextControl *fLeftView, *fTopView, *fRightView, *fBottomView;
	BMenuField *fMenuField;

	fTeam = new BTextControl("team", "Team:", "0", NULL); fTeam->SetEnabled(false);
	fPort = new BTextControl("port", "Port:", "0", NULL); fPort->SetEnabled(false);
	fToken = new BTextControl("token", "Token:", "0", NULL); fToken->SetEnabled(false);

	fLeftView = new BTextControl("left", "User:", "0", NULL);
	fTopView = new BTextControl("top", "Group:", "0", NULL);
	fRightView = new BTextControl("right", "Right:", "255", NULL);
	fBottomView = new BTextControl("bottom", "Bottom:", "255", NULL);

	BMenu *menu = new BPopUpMenu("menu");
	menu->AddItem(new BMenuItem("Item 1", NULL));
	menu->AddItem(new BMenuItem("Item 2", NULL));
	menu->AddItem(new BMenuItem("Item 3", NULL));
	menu->AddItem(new BMenuItem("Item 4", NULL));
	fMenuField = new BMenuField("menuField", "Menu field:", menu);

	BLayoutBuilder::Group<>(view, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING)
/*
		.Add(
			BLayoutBuilder::Group<>(NewLabelBox("frame", "Frame", NULL), B_HORIZONTAL, B_USE_SMALL_SPACING)
				.SetInsets(padding, 2*padding, padding, padding)
				.Add(CreateTextControlLayoutItem(fLeftView))
				.Add(CreateTextControlLayoutItem(fTopView))
				.Add(CreateTextControlLayoutItem(fRightView))
				.Add(CreateTextControlLayoutItem(fBottomView))
				.View()
		)
*/
		.Add(
			NewLabelBox("box1", NULL,
				BLayoutBuilder::Group<>(NewColorView("content", B_TRANSPARENT_COLOR), B_HORIZONTAL, B_USE_SMALL_SPACING)
					.SetInsets(B_USE_SMALL_SPACING)
					.Add(CreateTextControlLayoutItem(fTeam))
					.Add(CreateTextControlLayoutItem(fPort))
					.Add(CreateTextControlLayoutItem(fToken))
					.View()
			)
		)

		.Add(
			NewLabelBox("frame", "Effective",
				BLayoutBuilder::Group<>(NewColorView("content", B_TRANSPARENT_COLOR), B_HORIZONTAL, B_USE_SMALL_SPACING)
					.SetInsets(B_USE_SMALL_SPACING, 0, B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
					.Add(CreateTextControlLayoutItem(new BTextControl("euid", "User:", "0", NULL)))
					.Add(CreateTextControlLayoutItem(new BTextControl("egid", "Group:", "0", NULL)))
					.View()
			)
		)
		.Add(
			NewLabelBox("frame", "Actual",
				BLayoutBuilder::Group<>(NewColorView("content", B_TRANSPARENT_COLOR), B_HORIZONTAL, B_USE_SMALL_SPACING)
					.SetInsets(B_USE_SMALL_SPACING, 0, B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
					.Add(CreateTextControlLayoutItem(new BTextControl("uid", "User:", "0", NULL)))
					.Add(CreateTextControlLayoutItem(new BTextControl("gid", "Group:", "0", NULL)))
					.View()
			)
		)
		.Add(CreateMenuFieldLayoutItem(fMenuField))
		.AddGlue()
		.End();

	return view;
}
