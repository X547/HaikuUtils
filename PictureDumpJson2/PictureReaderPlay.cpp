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
	vis.DrawLine(start, end, {.isStroke = true});
}

static void	_StrokeRect(void *p, BRect rect)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRect(rect, {.isStroke = true});
}

static void	_FillRect(void *p, BRect rect)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRect(rect, {.isStroke = false});
}

static void	_StrokeRoundRect(void *p, BRect rect, BPoint radii)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRoundRect(rect, radii, {.isStroke = true});
}

static void	_FillRoundRect(void *p, BRect rect, BPoint radii)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRoundRect(rect, radii, {.isStroke = false});
}

static void	_StrokeBezier(void *p, BPoint *control)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawBezier(control, {.isStroke = true});
}

static void	_FillBezier(void *p, BPoint *control)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawBezier(control, {.isStroke = false});
}

static void	_StrokeArc(void *p, BPoint center, BPoint radii, float startTheta, float arcTheta)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawArc(center, radii, startTheta, arcTheta, {.isStroke = true});
}

static void	_FillArc(void *p, BPoint center, BPoint radii, float startTheta, float arcTheta)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawArc(center, radii, startTheta, arcTheta, {.isStroke = false});
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
	vis.DrawEllipse(rect, {.isStroke = true});
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
	vis.DrawEllipse(rect, {.isStroke = false});
}

static void	_StrokePolygon(void *p, int32 numPoints, const BPoint *points, bool isClosed)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawPolygon(numPoints, points, isClosed, {.isStroke = true});
}

static void	_FillPolygon(void *p, int32 numPoints, const BPoint *points, bool isClosed)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawPolygon(numPoints, points, true, {.isStroke = false});
}

static void	_StrokeShape(void * p, const BShape *shape)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawShape(*shape, {.isStroke = true});
}

static void	_FillShape(void * p, const BShape *shape)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawShape(*shape, {.isStroke = false});
}

static void	_DrawString(void *p, const char *string, float deltaSpace, float deltaNonSpace)
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
	const void *data
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

static void	_SetClippingRects(void *p, const BRect *rects, uint32 numRects)
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

static void	_SetFontFamily(void *p, const char *inFamily)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);

	font_family family;
	size_t len = strlen(inFamily);
	Check(len <= sizeof(family) - 1);
	strcpy(family, inFamily);

	vis.SetFontFamily(family);
}

static void	_SetFontStyle(void *p, const char *inStyle)
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
	fprintf(stderr, "_op0\n");
}

static void	_DrawPicture(void * p, BPoint where, int32 token)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawPicture(where, token);
}

static void	_op45(void * p)
{
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

static void	_DrawStringLocations(void * p, const char* string, const BPoint* locations, size_t locationCount)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawString(string, strlen(string), locations, locationCount);
}

static void	_FillRectGradient(void * p, BRect rect, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRect(rect, {.isStroke = false, .gradient = &gradient});
}

static void	_StrokeRectGradient(void * p, BRect rect, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRect(rect, {.isStroke = true, .gradient = &gradient});
}

static void	_FillRoundRectGradient(void * p, BRect rect, BPoint radii, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRoundRect(rect, radii, {.isStroke = false, .gradient = &gradient});
}

static void	_StrokeRoundRectGradient(void * p, BRect rect, BPoint radii, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawRoundRect(rect, radii, {.isStroke = true, .gradient = &gradient});
}

static void	_FillBezierGradient(void * p, const BPoint* points, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawBezier(points, {.isStroke = false, .gradient = &gradient});
}

static void	_StrokeBezierGradient(void * p, const BPoint* points, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawBezier(points, {.isStroke = true, .gradient = &gradient});
}

static void	_FillArcGradient(void * p, BPoint center, BPoint radii, float startTheta, float arcTheta, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawArc(
		center,
		radii,
		startTheta,
		arcTheta,
		{.isStroke = false, .gradient = &gradient}
	);
}

static void	_StrokeArcGradient(void * p, BPoint center, BPoint radii, float startTheta, float arcTheta, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawArc(
		center,
		radii,
		startTheta,
		arcTheta,
		{.isStroke = true, .gradient = &gradient}
	);
}

static void	_FillEllipseGradient(void * p, BPoint center, BPoint radii, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	// TODO: check if +-1 is needed
	vis.DrawEllipse(
		BRect(center.x - radii.x, center.y - radii.y, center.x + radii.x, center.y + radii.y),
		{.isStroke = false, .gradient = &gradient}
	);
}

static void	_StrokeEllipseGradient(void * p, BPoint center, BPoint radii, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	// TODO: check if +-1 is needed
	vis.DrawEllipse(
		BRect(center.x - radii.x, center.y - radii.y, center.x + radii.x, center.y + radii.y),
		{.isStroke = true, .gradient = &gradient}
	);
}

static void	_FillPolygonGradient(void * p, int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawPolygon(numPoints, points, true, {.isStroke = false, .gradient = &gradient});
}

static void	_StrokePolygonGradient(void * p, int32 numPoints, const BPoint* points, bool isClosed, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawPolygon(numPoints, points, isClosed, {.isStroke = false, .gradient = &gradient});
}

static void	_FillShapeGradient(void * p, BShape shape, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawShape(shape, {.isStroke = false, .gradient = &gradient});
}

static void	_StrokeShapeGradient(void * p, BShape shape, const BGradient& gradient)
{
	PictureVisitor &vis = *static_cast<PictureVisitor*>(p);
	vis.DrawShape(shape, {.isStroke = true, .gradient = &gradient});
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
	void (*stroke_shape)(void* user, const BShape *shape);
	void (*fill_shape)(void* user, const BShape *shape);
	void (*draw_string)(void* user, const char* string, float deltax, float deltay);
	void (*draw_pixels)(void* user, BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, const void* data);
	void (*draw_picture)(void* user, BPoint where, int32 token);
	void (*set_clipping_rects)(void* user, const BRect* rects, uint32 numRects);
	void (*clip_to_picture)(void* user, BPicture *picture, BPoint point, bool clip_to_inverse_picture);
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
	void (*scale_by)(void* user, double x, double y);
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


static picture_player_callbacks
playbackHandlers = {
	.nop = _op0,								// 0	no operation
	.move_pen_by = _MovePenBy,					// 1	MovePenBy(void *user, BPoint delta)
	.stroke_line = _StrokeLine,					// 2	StrokeLine(void *user, BPoint start, BPoint end)
	.stroke_rect = _StrokeRect,					// 3	StrokeRect(void *user, BRect rect)
	.fill_rect = _FillRect,						// 4	FillRect(void *user, BRect rect)
	.stroke_round_rect = _StrokeRoundRect,		// 5	StrokeRoundRect(void *user, BRect rect, BPoint radii)
	.fill_round_rect = _FillRoundRect,			// 6	FillRoundRect(void *user, BRect rect, BPoint radii)
	.stroke_bezier = _StrokeBezier,				// 7	StrokeBezier(void *user, BPoint *control)
	.fill_bezier = _FillBezier,					// 8	FillBezier(void *user, BPoint *control)
	.stroke_arc = _StrokeArc,					// 9	StrokeArc(void *user, BPoint center, BPoint radii, float startTheta, float arcTheta)
	.fill_arc = _FillArc,						// 10	FillArc(void *user, BPoint center, BPoint radii, float startTheta, float arcTheta)
	.stroke_ellipse = _StrokeEllipse,			// 11	StrokeEllipse(void *user, BPoint center, BPoint radii)
	.fill_ellipse = _FillEllipse,				// 12	FillEllipse(void *user, BPoint center, BPoint radii)
	.stroke_polygon = _StrokePolygon,			// 13	StrokePolygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
	.fill_polygon = _FillPolygon,				// 14	FillPolygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
	.stroke_shape = _StrokeShape,				// 15	StrokeShape(void *user, BShape *shape)
	.fill_shape = _FillShape,					// 16	FillShape(void *user, BShape *shape)
	.draw_string = _DrawString,					// 17	DrawString(void *user, char *string, float deltax, float deltay)
	.draw_pixels = _DrawPixels,					// 18	DrawPixels(void *user, BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
	.draw_picture = _DrawPicture,				// 19	*reserved*
	.set_clipping_rects = _SetClippingRects,	// 20	SetClippingRects(void *user, BRect *rects, uint32 numRects)
	.clip_to_picture = _ClipToPicture,			// 21	ClipToPicture(void *user, BPicture *picture, BPoint pt, bool clip_to_inverse_picture)
	.push_state = _PushState,					// 22	PushState(void *user)
	.pop_state = _PopState,						// 23	PopState(void *user)
	.enter_state_change = _EnterStateChange,	// 24	EnterStateChange(void *user)
	.exit_state_change = _ExitStateChange,		// 25	ExitStateChange(void *user)
	.enter_font_state = _EnterFontState,		// 26	EnterFontState(void *user)
	.exit_font_state = _ExitFontState,			// 27	ExitFontState(void *user)
	.set_origin = _SetOrigin,					// 28	SetOrigin(void *user, BPoint pt)
	.set_pen_location = _SetPenLocation,		// 29	SetPenLocation(void *user, BPoint pt)
	.set_drawing_mode = _SetDrawingMode,		// 30	SetDrawingMode(void *user, drawing_mode mode)
	.set_line_mode = _SetLineMode,				// 31	SetLineMode(void *user, cap_mode capMode, join_mode joinMode, float miterLimit)
	.set_pen_size = _SetPenSize,				// 32	SetPenSize(void *user, float size)
	.set_fore_color = _SetForeColor,			// 33	SetForeColor(void *user, rgb_color color)
	.set_back_color = _SetBackColor,			// 34	SetBackColor(void *user, rgb_color color)
	.set_stipple_pattern = _SetStipplePattern,	// 35	SetStipplePattern(void *user, pattern p)
	.set_scale = _SetScale,						// 36	SetScale(void *user, float scale)
	.set_font_family = _SetFontFamily,			// 37	SetFontFamily(void *user, char *family)
	.set_font_style = _SetFontStyle,			// 38	SetFontStyle(void *user, char *style)
	.set_font_spacing = _SetFontSpacing,		// 39	SetFontSpacing(void *user, int32 spacing)
	.set_font_size = _SetFontSize,				// 40	SetFontSize(void *user, float size)
	.set_font_rotate = _SetFontRotate,			// 41	SetFontRotate(void *user, float rotation)
	.set_font_encoding = _SetFontEncoding,		// 42	SetFontEncoding(void *user, int32 encoding)
	.set_font_flags = _SetFontFlags,			// 43	SetFontFlags(void *user, int32 flags)
	.set_font_shear = _SetFontShear,			// 44	SetFontShear(void *user, float shear)
	.op45 = _op45,								// 45	*reserved*
	.set_font_face = _SetFontFace,				// 46	SetFontFace(void *user, int32 flags)

	.set_blending_mode = _SetBlendingMode,      	// 47
	.set_transform = _SetTransform,         		// 48
	.translate_by = _TranslateBy,					// 49
	.scale_by = _ScaleBy,							// 50
	.rotate_by = _RotateBy,							// 51
	.blend_layer = _BlendLayer,						// 52
	.clip_to_rect = _ClipToRect,					// 53
	.clip_to_shape = _ClipToShape,					// 54
	.draw_string_locations = _DrawStringLocations,	// 55

	.fill_rect_gradient = _FillRectGradient,				// 56
	.stroke_rect_gradient = _StrokeRectGradient,			// 57
	.fill_round_rect_gradient = _FillRoundRectGradient,		// 58
	.stroke_round_rect_gradient = _StrokeRoundRectGradient,	// 59
	.fill_bezier_gradient = _FillBezierGradient,			// 60
	.stroke_bezier_gradient = _StrokeBezierGradient,		// 61
	.fill_arc_gradient = _FillArcGradient,					// 62
	.stroke_arc_gradient = _StrokeArcGradient,				// 63
	.fill_ellipse_gradient = _FillEllipseGradient,			// 64
	.stroke_ellipse_gradient = _StrokeEllipseGradient,		// 65
	.fill_polygon_gradient = _FillPolygonGradient,			// 66
	.stroke_polygon_gradient = _StrokePolygonGradient,		// 67
	.fill_shape_gradient = _FillShapeGradient,				// 68
	.stroke_shape_gradient = _StrokeShapeGradient,			// 69
	.set_fill_rule = _SetFillRule,							// 70
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
	pict.Play((void**)&playbackHandlers, sizeof(playbackHandlers) / sizeof(void*), &vis);
	vis.ExitOps();
	vis.ExitPicture();

	return B_OK;
}

