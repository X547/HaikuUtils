#include "PictureReaderPlay.h"

#include <stdio.h>
#include <stdlib.h>

#include <Picture.h>

#include <private/interface/ShapePrivate.h>

#include "PictureVisitor.h"


static void Check(bool cond)
{
	if (!cond) {
		fprintf(stderr, "[!] error\n");
		abort();
	}
}


static void	_MovePenBy(void *p, BPoint delta)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.MovePenBy(delta.x, delta.y);
}

static void	_StrokeLine(void *p, BPoint start, BPoint end)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokeLine(start, end);
}

static void	_StrokeRect(void *p, BRect rect)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokeRect(rect);
}

static void	_FillRect(void *p, BRect rect)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.FillRect(rect);
}

static void	_StrokeRoundRect(void *p, BRect rect, BPoint radii)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokeRoundRect(rect, radii);
}

static void	_FillRoundRect(void *p, BRect rect, BPoint radii)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.FillRoundRect(rect, radii);
}

static void	_StrokeBezier(void *p, BPoint *control)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokeBezier(control);
}

static void	_FillBezier(void *p, BPoint *control)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.FillBezier(control);
}

static void	_StrokeArc(void *p, BPoint center, BPoint radii, float startTheta, float arcTheta)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokeArc(center, radii, startTheta, arcTheta);
}

static void	_FillArc(void *p, BPoint center, BPoint radii, float startTheta, float arcTheta)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.FillArc(center, radii, startTheta, arcTheta);
}

static void	_StrokeEllipse(void *p, BPoint center, BPoint radii)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	// TODO: check +-1 is needed
	BRect rect(
		center.x - radii.x,
		center.y - radii.y,
		center.x + radii.x,
		center.y + radii.y
	);
	vis.StrokeEllipse(rect);
}

static void	_FillEllipse(void *p, BPoint center, BPoint radii)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	// TODO: check +-1 is needed
	BRect rect(
		center.x - radii.x,
		center.y - radii.y,
		center.x + radii.x,
		center.y + radii.y
	);
	vis.FillEllipse(rect);
}

static void	_StrokePolygon(void *p, int32 numPoints, BPoint *points, bool isClosed)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokePolygon(numPoints, points, isClosed);
}

static void	_FillPolygon(void *p, int32 numPoints, BPoint *points, bool isClosed)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.FillPolygon(numPoints, points);
}

static void	_StrokeShape(void * p, BShape *shape)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.StrokeShape(*shape);
}

static void	_FillShape(void * p, BShape *shape)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.FillShape(*shape);
}

static void	_DrawString(void *p, char *string, float deltaSpace, float deltaNonSpace)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	escapement_delta delta {
		.nonspace = deltaNonSpace,
		.space = deltaSpace,
	};
	vis.DrawString(string, strlen(string), delta);
}

static void	_DrawPixels(
	void *p,
	BRect src,
	BRect dest,
	int32 width,
	int32 height,
	int32 bytesPerRow,
	int32 pixelFormat,
	int32 flags,
	void *data
)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawBitmap(
		src,
		dest,
		width,
		height,
		bytesPerRow,
		pixelFormat,
		flags,
		data,
		height*bytesPerRow
	);
}

static void	_SetClippingRects(void *p, BRect *rects, uint32 numRects)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	BRegion region;
	for (uint32 i = 0; i < numRects; i++) {
		region.Include(rects[i]);
	}
	vis.SetClipping(region);
}

static void	_ClipToPicture(void * p, BPicture *picture, BPoint point, bool clip_to_inverse_picture)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.ClipToPicture(-1 /* !!! */, point, clip_to_inverse_picture);
}

static void	_PushState(void *p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.PushState();
}

static void	_PopState(void *p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.PopState();
}

static void	_EnterStateChange(void *p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.EnterStateChange();
}

static void	_ExitStateChange(void *p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.ExitStateChange();
}

static void	_EnterFontState(void *p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.EnterFontState();
}

static void	_ExitFontState(void *p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.ExitFontState();
}

static void	_SetOrigin(void *p, BPoint pt)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetOrigin(pt);
}

static void	_SetPenLocation(void *p, BPoint pt)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetPenLocation(pt);
}

static void	_SetDrawingMode(void *p, drawing_mode mode)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetDrawingMode(mode);
}

static void	_SetLineMode(void *p, cap_mode capMode, join_mode joinMode, float miterLimit)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetLineMode(capMode, joinMode, miterLimit);
}

static void	_SetPenSize(void *p, float size)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetPenSize(size);
}

static void	_SetForeColor(void *p, rgb_color color)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetHighColor(color);
}

static void	_SetBackColor(void *p, rgb_color color)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetLowColor(color);
}

static void	_SetStipplePattern(void *p, pattern pat)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetPattern(pat);
}

static void	_SetScale(void *p, float scale)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetScale(scale);
}

static void	_SetFontFamily(void *p, char *inFamily)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);

	font_family family;
	size_t len = strlen(inFamily);
	Check(len <= sizeof(family) - 1);
	strcpy(family, inFamily);

	vis.SetFontFamily(family);
}

static void	_SetFontStyle(void *p, char *inStyle)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);

	font_style style;
	size_t len = strlen(inStyle);
	Check(len <= sizeof(style) - 1);
	strcpy(style, inStyle);

	vis.SetFontStyle(style);
}

static void	_SetFontSpacing(void *p, int32 spacing)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontSpacing(spacing);
}

static void	_SetFontSize(void *p, float size)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontSize(size);
}

static void	_SetFontRotate(void *p, float rotation)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontRotation(rotation);
}

static void	_SetFontEncoding(void *p, int32 encoding)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontEncoding(encoding);
}

static void	_SetFontFlags(void *p, int32 flags)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontFlags(flags);
}

static void	_SetFontShear(void *p, float shear)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontShear(shear);
}

static void	_SetFontFace(void * p, int32 flags)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFontFace(flags);
}

static void	_op0(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op0\n");
}

static void	_DrawPicture(void * p, BPoint where, int32 token)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawPicture(where, token);
}

static void	_op45(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op45\n");
}

static void	_SetBlendingMode(void * p, source_alpha srcAlpha, alpha_function alphaFunc)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetBlendingMode(srcAlpha, alphaFunc);
}

static void	_SetTransform(void * p, const BAffineTransform &transform)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetTransform(transform);
}

static void	_TranslateBy(void * p, double x, double y)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.TranslateBy(x, y);
}

static void	_ScaleBy(void * p, double x, double y)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.ScaleBy(x, y);
}

static void	_RotateBy(void * p, double angle)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.RotateBy(angle);
}

static void	_BlendLayer(void * p, Layer* layer)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.BlendLayer(layer);
}

static void	_ClipToRect(void * p, const BRect& rect, bool inverse)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.ClipToRect(rect, inverse);
}

static void	_ClipToShape(void * p, int32 opCount, const uint32* opList, int32 ptCount, const BPoint* ptList, bool inverse)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	BShape shape;
	BShape::Private(shape).SetData(opCount, ptCount, opList, ptList);
	vis.ClipToShape(shape, inverse);
}

static void	_op55(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op55\n");
}

static void	_op56(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op56\n");
}

static void	_op57(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op57\n");
}

static void	_op58(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op58\n");
}

static void	_op59(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op59\n");
}

static void	_op60(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op60\n");
}

static void	_op61(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op61\n");
}

static void	_op62(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op62\n");
}

static void	_op63(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op63\n");
}

static void	_op64(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op64\n");
}

static void	_op65(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op65\n");
}

static void	_op66(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op66\n");
}

static void	_op67(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op67\n");
}

static void	_op68(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op68\n");
}

static void	_op69(void * p)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	fprintf(stderr, "_op69\n");
}

static void	_SetFillRule(void * p, int32 fillRule)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.SetFillRule(fillRule);
}


struct picture_player_callbacks {
	void (*nop)(void* user);
	void (*move_pen_by)(void* user, BPoint delta);
	void (*stroke_line)(void* user, BPoint start, BPoint end);
	void (*stroke_rect)(void* user, BRect rect);
	void (*fill_rect)(void* user, BRect rect);
	void (*stroke_round_rect)(void* user, BRect rect, BPoint radii);
	void (*fill_round_rect)(void* user, BRect rect, BPoint radii);
	void (*stroke_bezier)(void* user, BPoint* control);
	void (*fill_bezier)(void* user, BPoint* control);
	void (*stroke_arc)(void* user, BPoint center, BPoint radii, float startTheta, float arcTheta);
	void (*fill_arc)(void* user, BPoint center, BPoint radii, float startTheta, float arcTheta);
	void (*stroke_ellipse)(void* user, BPoint center, BPoint radii);
	void (*fill_ellipse)(void* user, BPoint center, BPoint radii);
	void (*stroke_polygon)(void* user, int32 numPoints, const BPoint* points, bool isClosed);
	void (*fill_polygon)(void* user, int32 numPoints, const BPoint* points, bool isClosed);
	void (*op15)(void* user);
	void (*op16)(void* user);
	void (*draw_string)(void* user, const char* string, float deltax, float deltay);
	void (*draw_pixels)(void* user, BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, const void* data);
	void (*op19)(void* user); // draw_picture
	void (*set_clipping_rects)(void* user, const BRect* rects, uint32 numRects);
	void (*op21)(void* user); // clip_to_picture
	void (*push_state)(void* user);
	void (*pop_state)(void* user);
	void (*enter_state_change)(void* user);
	void (*exit_state_change)(void* user);
	void (*enter_font_state)(void* user);
	void (*exit_font_state)(void* user);
	void (*set_origin)(void* user, BPoint pt);
	void (*set_pen_location)(void* user, BPoint pt);
	void (*set_drawing_mode)(void* user, drawing_mode mode);
	void (*set_line_mode)(void* user, cap_mode capMode, join_mode joinMode, float miterLimit);
	void (*set_pen_size)(void* user, float size);
	void (*set_fore_color)(void* user, rgb_color color);
	void (*set_back_color)(void* user, rgb_color color);
	void (*set_stipple_pattern)(void* user, pattern p);
	void (*set_scale)(void* user, float scale);
	void (*set_font_family)(void* user, const char* family);
	void (*set_font_style)(void* user, const char* style);
	void (*set_font_spacing)(void* user, int32 spacing);
	void (*set_font_size)(void* user, float size);
	void (*set_font_rotate)(void* user, float rotation);
	void (*set_font_encoding)(void* user, int32 encoding);
	void (*set_font_flags)(void* user, int32 flags);
	void (*set_font_shear)(void* user, float shear);
	void (*op45)(void* user);
	void (*set_font_face)(void* user, int32 flags);

	// New in Haiku
	void (*set_blending_mode)(void* user, source_alpha alphaSrcMode, alpha_function alphaFncMode);
	void (*set_transform)(void* user, const BAffineTransform& transform);
	void (*translate_by)(void* user, double x, double y);
	void (*rotate_by)(void* user, double angleRadians);
	void (*blend_layer)(void* user, class Layer* layer); // broken when saved to file
	void (*clip_to_rect)(void* user, const BRect& rect, bool inverse);
	void (*clip_to_shape)(void* user, int32 opCount, const uint32 opList[], int32 ptCount, const BPoint ptList[], bool inverse); // Why not BShape?
	void (*draw_string_locations)(void* user, const char* string, const BPoint* locations, size_t locationCount);

	void (*fill_rect_gradient)(void* user, BRect rect, const BGradient& gradient);
	void (*stroke_rect_gradient)(void* user, BRect rect, const BGradient& gradient);
	void (*fill_round_rect_gradient)(void* user, BRect rect, BPoint radii, const BGradient& gradient);
	void (*stroke_round_rect_gradient)(void* user, BRect rect, BPoint radii, const BGradient& gradient);
	void (*fill_bezier_gradient)(void* user, const BPoint* points, const BGradient& gradient);
	void (*stroke_bezier_gradient)(void* user, const BPoint* points, const BGradient& gradient);
	void (*fill_arc_gradient)(void* user, BPoint center, BPoint radii, float startTheta, float arcTheta, const BGradient& gradient);
	void (*stroke_arc_gradient)(void* user, BPoint center, BPoint radii, float startTheta, float arcTheta, const BGradient& gradient);
	void (*fill_ellipse_gradient)(void* user, BPoint center, BPoint radii, const BGradient& gradient);
	void (*stroke_ellipse_gradient)(void* user, BPoint center, BPoint radii, const BGradient& gradient);
	void (*fill_polygon_gradient)(void* user, int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient);
	void (*stroke_polygon_gradient)(void* user, int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient);
	void (*fill_shape_gradient)(void* user, BShape shape, const BGradient& gradient);
	void (*stroke_shape_gradient)(void* user, BShape shape, const BGradient& gradient);

	void (*set_fill_rule)(void* user, int32 fillRule);
};


static void *
playbackHandlers[] = {
	(void *)_op0,					// 0	no operation
	(void *)_MovePenBy,				// 1	MovePenBy(void *user, BPoint delta)
	(void *)_StrokeLine,			// 2	StrokeLine(void *user, BPoint start, BPoint end)
	(void *)_StrokeRect,			// 3	StrokeRect(void *user, BRect rect)
	(void *)_FillRect,				// 4	FillRect(void *user, BRect rect)
	(void *)_StrokeRoundRect,		// 5	StrokeRoundRect(void *user, BRect rect, BPoint radii)
	(void *)_FillRoundRect,			// 6	FillRoundRect(void *user, BRect rect, BPoint radii)
	(void *)_StrokeBezier,			// 7	StrokeBezier(void *user, BPoint *control)
	(void *)_FillBezier,			// 8	FillBezier(void *user, BPoint *control)
	(void *)_StrokeArc,				// 9	StrokeArc(void *user, BPoint center, BPoint radii, float startTheta, float arcTheta)
	(void *)_FillArc,				// 10	FillArc(void *user, BPoint center, BPoint radii, float startTheta, float arcTheta)
	(void *)_StrokeEllipse,			// 11	StrokeEllipse(void *user, BPoint center, BPoint radii)
	(void *)_FillEllipse,			// 12	FillEllipse(void *user, BPoint center, BPoint radii)
	(void *)_StrokePolygon,			// 13	StrokePolygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
	(void *)_FillPolygon,			// 14	FillPolygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
	(void *)_StrokeShape,			// 15	StrokeShape(void *user, BShape *shape)
	(void *)_FillShape,				// 16	FillShape(void *user, BShape *shape)
	(void *)_DrawString,			// 17	DrawString(void *user, char *string, float deltax, float deltay)
	(void *)_DrawPixels,			// 18	DrawPixels(void *user, BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
	(void *)_DrawPicture,			// 19	*reserved*
	(void *)_SetClippingRects,		// 20	SetClippingRects(void *user, BRect *rects, uint32 numRects)
	(void *)_ClipToPicture,			// 21	ClipToPicture(void *user, BPicture *picture, BPoint pt, bool clip_to_inverse_picture)
	(void *)_PushState,				// 22	PushState(void *user)
	(void *)_PopState,				// 23	PopState(void *user)
	(void *)_EnterStateChange,		// 24	EnterStateChange(void *user)
	(void *)_ExitStateChange,		// 25	ExitStateChange(void *user)
	(void *)_EnterFontState,		// 26	EnterFontState(void *user)
	(void *)_ExitFontState,			// 27	ExitFontState(void *user)
	(void *)_SetOrigin,				// 28	SetOrigin(void *user, BPoint pt)
	(void *)_SetPenLocation,		// 29	SetPenLocation(void *user, BPoint pt)
	(void *)_SetDrawingMode,		// 30	SetDrawingMode(void *user, drawing_mode mode)
	(void *)_SetLineMode,			// 31	SetLineMode(void *user, cap_mode capMode, join_mode joinMode, float miterLimit)
	(void *)_SetPenSize,			// 32	SetPenSize(void *user, float size)
	(void *)_SetForeColor,			// 33	SetForeColor(void *user, rgb_color color)
	(void *)_SetBackColor,			// 34	SetBackColor(void *user, rgb_color color)
	(void *)_SetStipplePattern,		// 35	SetStipplePattern(void *user, pattern p)
	(void *)_SetScale,				// 36	SetScale(void *user, float scale)
	(void *)_SetFontFamily,			// 37	SetFontFamily(void *user, char *family)
	(void *)_SetFontStyle,			// 38	SetFontStyle(void *user, char *style)
	(void *)_SetFontSpacing,		// 39	SetFontSpacing(void *user, int32 spacing)
	(void *)_SetFontSize,			// 40	SetFontSize(void *user, float size)
	(void *)_SetFontRotate,			// 41	SetFontRotate(void *user, float rotation)
	(void *)_SetFontEncoding,		// 42	SetFontEncoding(void *user, int32 encoding)
	(void *)_SetFontFlags,			// 43	SetFontFlags(void *user, int32 flags)
	(void *)_SetFontShear,			// 44	SetFontShear(void *user, float shear)
	(void *)_op45,					// 45	*reserved*
	(void *)_SetFontFace,			// 46	SetFontFace(void *user, int32 flags)
	(void *)_SetBlendingMode,       // 47
	(void *)_SetTransform,          // 48
	(void *)_TranslateBy,           // 49
	(void *)_ScaleBy,				// 50
	(void *)_RotateBy,				// 51
	(void *)_BlendLayer,			// 52
	(void *)_ClipToRect,			// 53
	(void *)_ClipToShape,			// 54
	(void *)_op55,					// 55
	(void *)_op56,					// 56
	(void *)_op57,					// 57
	(void *)_op58,					// 58
	(void *)_op59,					// 59
	(void *)_op60,					// 60
	(void *)_op61,					// 61
	(void *)_op62,					// 62
	(void *)_op63,					// 63
	(void *)_op64,					// 64
	(void *)_op65,					// 65
	(void *)_op66,					// 66
	(void *)_op67,					// 67
	(void *)_op68,					// 68
	(void *)_op69,					// 69
	(void *)_SetFillRule,			// 70

	NULL
};


void PictureReaderPlay::RaiseError() const
{
	fprintf(stderr, "[!] error\n");
	abort();
}

void PictureReaderPlay::CheckStatus(status_t status) const
{
	if (status < B_OK) {
		RaiseError();
	}
}


status_t PictureReaderPlay::Accept(PictureVisitor &vis) const
{
	BPicture pict;
	CheckStatus(pict.Unflatten(&fRd));

	vis.EnterPicture(2, 0);
	vis.EnterOps();
	pict.Play(playbackHandlers, 71, &vis);
	vis.ExitOps();
	vis.ExitPicture();

	return B_OK;
}

