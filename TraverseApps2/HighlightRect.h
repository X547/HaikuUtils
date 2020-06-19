#ifndef _HIGHLIGHTRECT_H_
#define _HIGHLIGHTRECT_H_

#include <Rect.h>

class BWindow;

class HighlightRect
{
private:
	BRect fRect;
	BWindow *fWnds[4];

public:
	HighlightRect();
	~HighlightRect();

	void Show(BRect rect);
	void Hide();
};

#endif	// _HIGHLIGHTRECT_H_
