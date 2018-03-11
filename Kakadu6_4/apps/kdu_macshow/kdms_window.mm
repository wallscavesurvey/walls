/*****************************************************************************/
// File: kdms_window.mm [scope = APPS/MACSHOW]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: Mr David McKenzie
// License number: 00590
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
  Implements the main `kdms_window' class and its `kdms_document_view', both
of which are Objective-C classes.   The embedded `kdms_renderer' object is
implemented separately in `kdms_renderer.mm' -- it is a C++ class but
also has to interact with Cocoa.
******************************************************************************/
#import "kdms_window.h"
#import "kdms_catalog.h"
#import "kdms_url_dialog.h"
#import "kdms_metadata_editor.h"


/*===========================================================================*/
/*                            EXTERNAL FUNCTIONS                             */
/*===========================================================================*/

/*****************************************************************************/
/*                             interrogate_user                              */
/*****************************************************************************/

int interrogate_user(const char *message, const char *option0,
                     const char *option1, const char *option2,
                     const char *option3)
{
  NSAlert *alert = [[NSAlert alloc] init];
  [alert setMessageText:[NSString stringWithUTF8String:message]];
  [alert addButtonWithTitle:[NSString stringWithUTF8String:option0]];
  if (option1[0] != '\0')
    {
      [alert addButtonWithTitle:[NSString stringWithUTF8String:option1]];
      if (option2[0] != '\0')
        {
          [alert addButtonWithTitle:[NSString stringWithUTF8String:option2]];
          if (option3[0] != '\0')
            [alert addButtonWithTitle:[NSString stringWithUTF8String:option3]];
        }
    }
  [alert setAlertStyle:NSInformationalAlertStyle];
  NSInteger result = [alert runModal];
  [alert release];
  if (result == NSAlertFirstButtonReturn)
    return 0;
  else if (result == NSAlertSecondButtonReturn)
    return 1;
  else if (result == NSAlertThirdButtonReturn)
    return 2;
  else
    return 3;
}

  
/*===========================================================================*/
/*                            kdms_document_view                             */
/*===========================================================================*/

@implementation kdms_document_view

/*****************************************************************************/
/*                     kdms_document_view:initWithFrame                      */
/*****************************************************************************/

- (id)initWithFrame:(NSRect)frame
{
  renderer = NULL;
  wnd = NULL;
  is_focussing = is_dragging = shape_dragging = shape_scribbling = false;
  is_first_responder = true;
  return [super initWithFrame:frame];
}

/*****************************************************************************/
/*                     kdms_document_view:set_renderer                       */
/*****************************************************************************/

- (void)set_renderer:(kdms_renderer *)the_renderer
{
  renderer = the_renderer;
}

/*****************************************************************************/
/*                       kdms_document_view:set_owner                        */
/*****************************************************************************/

- (void)set_owner:(kdms_window *)owner
{
  wnd = owner;
}

/*****************************************************************************/
/*                         kdms_document_view::isOpaque                      */
/*****************************************************************************/

- (BOOL)isOpaque
{
  return YES;
}

/*****************************************************************************/
/*                         kdms_document_view:drawRect                       */
/*****************************************************************************/

- (void)drawRect:(NSRect)clipping_rect
{
  if (renderer == NULL)
    return; // Should never happen when drawing is really required.
  
  const NSRect *dirty_rects;
  NSInteger i, num_dirty_rects=0;
  [self getRectsBeingDrawn:&dirty_rects count:&num_dirty_rects];
  renderer->lock_view();
  for (i=0; i < num_dirty_rects; i++)
    {
      kdu_dims render_region =
        renderer->convert_region_from_doc_view(dirty_rects[i]);
      NSGraphicsContext* gc = [NSGraphicsContext currentContext];
      CGContextRef gc_ref = (CGContextRef)[gc graphicsPort];
      if (!renderer->paint_region(render_region,gc_ref))
        { // Need to erase the region
          CGRect cg_rect;
          cg_rect.origin.x = dirty_rects[i].origin.x;
          cg_rect.origin.y = dirty_rects[i].origin.y;
          cg_rect.size.width = dirty_rects[i].size.width;          
          cg_rect.size.height = dirty_rects[i].size.height;
          CGColorSpaceRef colour_space =
            CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
          CGFloat colour_values[4] = {1.0F,1.0F,1.0F,1.0F};
          CGColorRef fill_colour =
            CGColorCreate(colour_space,colour_values);
          CGContextSetFillColorWithColor(gc_ref,fill_colour);
          CGContextFillRect(gc_ref,cg_rect);
          CGColorRelease(fill_colour);
          CGColorSpaceRelease(colour_space);
        }
    }
  
  kdu_dims focus_box;
  if (renderer->need_focus_outline(&focus_box))
    { // Draw the focus box
      NSRect rect = renderer->convert_region_to_doc_view(focus_box);
      NSPoint point1 = rect.origin;
      NSPoint point2 = rect.origin;
      point2.x += rect.size.width-0.5F;  point2.y += rect.size.height-0.5F;
      point1.x -= 0.5F; point1.y -= 0.5F;
      for (int p=0; p < 2; p++)
        {
          NSBezierPath *path = [NSBezierPath bezierPath];
          [path setLineWidth:1.0];
          [path setLineJoinStyle:NSRoundLineJoinStyle];
          [path moveToPoint:NSMakePoint(point1.x,point1.y)];
          [path lineToPoint:NSMakePoint(point1.x,point2.y)];
          [path lineToPoint:NSMakePoint(point2.x,point2.y)];
          [path lineToPoint:NSMakePoint(point2.x,point1.y)];
          [path lineToPoint:NSMakePoint(point1.x,point1.y)];
          point1.x += 1.0F;  point1.y += 1.0F;
          point2.x -= 1.0F;  point2.y -= 1.0F;
          if (p == 0)
            [[NSColor brownColor] setStroke];
          else
            [[NSColor yellowColor] setStroke];
          [path stroke];
        }
    }
  
  if (is_focussing)
    { // Draw a temporary focus box now
      CGFloat pattern[2] = {5.0F,5.0F};
      for (int p=0; p < 2; p++)
        {
          NSBezierPath *path = [NSBezierPath bezierPath];
          [path setLineWidth:3.0];
          [path setLineJoinStyle:NSRoundLineJoinStyle];
          [path setLineDash:pattern count:2 phase:(p==0)?0.0F:5.0F];
          [path moveToPoint:NSMakePoint(mouse_start.x,mouse_start.y)];
          [path lineToPoint:NSMakePoint(mouse_start.x,mouse_last.y)];
          [path lineToPoint:NSMakePoint(mouse_last.x,mouse_last.y)];
          [path lineToPoint:NSMakePoint(mouse_last.x,mouse_start.y)];
          [path lineToPoint:NSMakePoint(mouse_start.x,mouse_start.y)];
          if (p == 0)
            [[NSColor brownColor] setStroke];
          else
            [[NSColor yellowColor] setStroke];
          [path stroke];
        }
    }
  
  if (renderer->get_shape_editor() != NULL)
    { 
      int n, flags, sel, drag;
      CGFloat pattern[2] = {2.0F,2.0F};
      for (drag=0; drag < 2; drag++)
        { 
          // Draw edges, highlighting selected edges on the correct side
          for (sel=0; sel < ((drag)?2:3); sel++)
            { 
              int mask=-1;
              if (sel == 1)
                mask = JPX_EDITOR_FLAG_SHARED;
              else if (sel == 2)
                mask = JPX_EDITOR_FLAG_SELECTED;
              NSBezierPath *path = [NSBezierPath bezierPath];
              if (drag)
                [path setLineDash:pattern count:2 phase:0.0F];
              [path setLineWidth:(sel==2)?3.0:1.0];
              NSPoint v1, v2;
              for (n=0; (flags=renderer->get_shape_edge(v1,v2,n,(sel==2),
                                                        (drag>0))); n++)
                { 
                  if (((v1.x==v1.y) && (v2.x==v2.y)) || !(flags & mask))
                    continue;
                  v1.x += 0.5F; v1.y += 0.5F;
                  v2.x += 0.5F; v2.y += 0.5F;
                  if (sel == 2)
                    { 
                      double dx=(v2.y-v1.y), dy=-(v2.x-v1.x);
                      double norm = 3.0 / sqrt(dx*dx + dy*dy);
                      v1.x += dx*norm; v2.x += dx*norm;
                      v1.y += dy*norm; v2.y += dy*norm;
                    }
                  [path moveToPoint:v1];
                  [path lineToPoint:v2];
                }
              if (sel == 0)
                [[NSColor whiteColor] setStroke];
              else if (sel == 1)
                [[NSColor grayColor] setStroke];
              else
                [[NSColor yellowColor] setStroke];
              if (n > 0)
                [path stroke];
            }
          
          // Draw ellipses
          NSPoint centre; NSSize extent; double alpha;
          for (n=0;
               (flags=renderer->get_shape_curve(centre,extent,alpha,n,
                                                false,(drag>0)));
               n++)
            { 
              NSBezierPath *path = [NSBezierPath bezierPath];
              if (drag)
                [path setLineDash:pattern count:2 phase:0.0F];
              bool selected = ((flags & JPX_EDITOR_FLAG_SELECTED) != 0);
              [path setLineWidth:(selected)?3.0:1.0];
              NSRect rect;
              centre.x += 0.5; centre.y += 0.5;
              rect.origin.x = centre.x - extent.width;
              rect.origin.y = centre.y - extent.height;
              rect.size.width = 2*extent.width;
              rect.size.height = 2*extent.height;
              [path appendBezierPathWithOvalInRect:rect];
              if (alpha != 0.0)
                {
                  NSAffineTransformStruct xform_struct;
                  xform_struct.m11 = xform_struct.m22 = 1.0;
                  xform_struct.m21 = alpha; xform_struct.m12 = 0.0;
                  xform_struct.tX = -alpha*centre.y; xform_struct.tY = 0.0;
                  NSAffineTransform *xform = [NSAffineTransform transform];
                  [xform setTransformStruct:xform_struct];
                  [path transformUsingAffineTransform:xform];
                }
              if (selected)
                [[NSColor yellowColor] setStroke];
              else
                [[NSColor whiteColor] setStroke];
              [path stroke];
            }
          
          if (!drag)
            { // Draw path outline
              pattern[0] = pattern[1] = 4.0;
              NSBezierPath *path = [NSBezierPath bezierPath];
              [path setLineDash:pattern count:2 phase:0.0F];
              [path setLineWidth:3.0];
              NSPoint v1, v2;
              for (n=0; renderer->get_shape_path(v1,v2,n); n++)
                if ((v1.x!=v1.y) || (v2.x!=v2.y))
                  { 
                    v1.x += 0.5F; v1.y += 0.5F;
                    v2.x += 0.5F; v2.y += 0.5F;
                    [path moveToPoint:v1];
                    [path lineToPoint:v2];
                  }
              if (n > 0)
                {
                  [[NSColor redColor] setStroke];
                  [path stroke];
                }
            }
          
          // Draw anchor points, highlighting the selected anchor point
          for (sel=0; sel < 2; sel++)
            { 
              int mask=(sel>0)?JPX_EDITOR_FLAG_SELECTED:-1;
              NSBezierPath *path = [NSBezierPath bezierPath];
              if (drag)
                [path setLineDash:pattern count:2 phase:0.0F];
              [path setLineWidth:(sel>0)?3.0:1.0];
              NSPoint v;
              for (n=0; (flags=renderer->get_shape_anchor(v,n,(sel>0),
                                                          (drag>0))); n++)
                { 
                  if (!(flags & mask))
                    continue;
                  NSRect rect; rect.origin = v;
                  rect.size.width=rect.size.height=9;
                  rect.origin.x-=4.5F; rect.origin.y-=4.5F;
                  [path appendBezierPathWithOvalInRect:rect];
                }
              if (n > 0)
                { 
                  [[NSColor yellowColor] setStroke];
                  [path stroke];
                }
            }
        }
      
      if (renderer->get_shape_scribble_mode())
        { // Draw scribble path
          NSPoint v;
          if (renderer->get_scribble_point(v,0))
            { 
              NSBezierPath *path = [NSBezierPath bezierPath];
              [path setLineWidth:1.0];
              [path moveToPoint:v];
              for (n=1; renderer->get_scribble_point(v,n); n++)
                [path lineToPoint:v];
              [[NSColor orangeColor] setStroke];
              [path stroke];
            }
        }
    }
  
  renderer->unlock_view();
}

/*****************************************************************************/
/*                   kdms_document_view:is_first_responder                   */
/*****************************************************************************/

- (bool)is_first_responder
{
  return self->is_first_responder;
}

/*****************************************************************************/
/*                    kdms_document_view:start_focusbox_at                   */
/*****************************************************************************/

- (void)start_focusbox_at:(NSPoint)point
{
  if (is_focussing || is_dragging)
    [self cancel_focus_drag];
  mouse_start = mouse_last = point;
  is_focussing = true;
}

/*****************************************************************************/
/*                    kdms_document_view:start_viewdrag_at                   */
/*****************************************************************************/

- (void)start_viewdrag_at:(NSPoint)point
{
  if (is_focussing || is_dragging)
    [self cancel_focus_drag];
  is_dragging = true;
  mouse_start = mouse_last = [self convertPoint:point toView:nil];
  if (wnd)
    [wnd set_drag_cursor:YES];
}

/*****************************************************************************/
/*                    kdms_document_view:cancel_focus_drag                   */
/*****************************************************************************/

- (void)cancel_focus_drag
{
  if (is_dragging)
    {
      is_dragging = false;
      if (wnd)
        [wnd set_drag_cursor:NO];
    }
  if (is_focussing)
    {
      is_focussing = false;
      [self invalidate_mouse_box];
    }
  if (shape_dragging && (renderer != NULL))
    {
      jpx_roi_editor *sed = renderer->get_shape_editor();
      if (sed == NULL)
        return;
      kdu_dims update_region = sed->cancel_drag();
      if (!update_region.is_empty())
        {
          [self repaint_shape_region:update_region];
          [renderer->editor update_shape_editor_controls];
        }
    }
  shape_dragging = false;
  if (shape_scribbling && (renderer != NULL))
    {
      jpx_roi_editor *sed = renderer->get_shape_editor();
      if (sed == NULL)
        return;
      kdu_dims update_region = sed->clear_scribble_points();
      if (!update_region.is_empty())
        {
          [self repaint_shape_region:update_region];
          [renderer->editor update_shape_editor_controls];
        }      
    }
  shape_scribbling = false;
}

/*****************************************************************************/
/*                     kdms_document_view:mouse_drag_to                      */
/*****************************************************************************/

- (void)mouse_drag_to:(NSPoint)point
{
  if (!(is_focussing || is_dragging))
    return;
  if (is_focussing)
    {
      [self invalidate_mouse_box];
      mouse_last = point;
      [self invalidate_mouse_box];
    }
  else if (is_dragging)
    {
      point = [self convertPoint:point toView:nil];
      if (renderer != NULL)
        renderer->scroll_absolute(2.0F*(point.x-mouse_last.x),
                                  2.0F*(point.y-mouse_last.y),
                                  true);
      mouse_last = point;
    }
}

/*****************************************************************************/
/*                    kdms_document_view:end_focus_drag_at                   */
/*****************************************************************************/

- (bool)end_focus_drag_at:(NSPoint)point
{
  if (is_focussing)
    {
      NSRect rect = [self rectangleFromMouseStartToPoint:point];
      [self cancel_focus_drag];
      if ((rect.size.width >= 2.0F) && (rect.size.height >= 2.0F) &&
          (renderer != NULL))
        {
          kdu_dims focus_box = renderer->convert_region_from_doc_view(rect);
          if ((focus_box.size.x > 1) && (focus_box.size.y > 1))
            renderer->set_focus_box(focus_box);
          return true;
        }
    }
  else if (is_dragging)
    {
      is_dragging = false;
      if (wnd)
        [wnd set_drag_cursor:NO];
      point = [self convertPoint:point toView:nil];
      if (renderer != NULL)
        renderer->scroll_absolute(2.0F*(point.x-mouse_last.x),
                                  2.0F*(point.y-mouse_last.y),
                                  true);
      return true;
    }
  return false;
}

/*****************************************************************************/
/*             kdms_document_view:rectangleFromMouseStartToPoint             */
/*****************************************************************************/

- (NSRect)rectangleFromMouseStartToPoint:(NSPoint)point
{
  NSRect rect;
  if (mouse_start.x <= point.x)
    {
      rect.origin.x = mouse_start.x;
      rect.size.width = point.x - mouse_start.x + 1.0F;
    }
  else
    {
      rect.origin.x = point.x;
      rect.size.width = mouse_start.x - point.x + 1.0F;
    }
  
  if (mouse_start.y <= point.y)
    {
      rect.origin.y = mouse_start.y;
      rect.size.height = point.y - mouse_start.y + 1.0F;
    }
  else
    {
      rect.origin.y = point.y;
      rect.size.height = mouse_start.y - point.y + 1.0F;
    }
  return rect;
}

/*****************************************************************************/
/*                  kdms_document_view:invalidate_mouse_box                  */
/*****************************************************************************/

- (void)invalidate_mouse_box
{
  NSRect dirty_rect = [self rectangleFromMouseStartToPoint:mouse_last];
  dirty_rect.origin.x -= KDMS_FOCUS_MARGIN;
  dirty_rect.size.width += 2.0F*KDMS_FOCUS_MARGIN;
  dirty_rect.origin.y -= KDMS_FOCUS_MARGIN;
  dirty_rect.size.height += 2.0F*KDMS_FOCUS_MARGIN;
  [self setNeedsDisplayInRect:dirty_rect];
}

/*****************************************************************************/
/*                  kdms_document_view:handle_single_click                   */
/*****************************************************************************/

- (void)handle_single_click:(NSPoint)point
{
  if (renderer == NULL)
    return;
  NSRect ns_rect;
  ns_rect.origin = point;
  ns_rect.size.width = ns_rect.size.height = 0.0F;
  kdu_coords coords = renderer->convert_region_from_doc_view(ns_rect).pos;
  renderer->reveal_metadata_at_point(coords);  
}

/*****************************************************************************/
/*                  kdms_document_view:handle_double_click                   */
/*****************************************************************************/

- (void)handle_double_click:(NSPoint)point
{
  if (renderer == NULL)
    return;
  NSRect ns_rect;
  ns_rect.origin = point;
  ns_rect.size.width = ns_rect.size.height = 0.0F;
  kdu_coords coords = renderer->convert_region_from_doc_view(ns_rect).pos;
  renderer->reveal_metadata_at_point(coords,true);
}

/*****************************************************************************/
/*                  kdms_document_view:handle_right_click                    */
/*****************************************************************************/

- (void)handle_right_click:(NSPoint)point
{
  if (renderer == NULL)
    return;
  NSRect ns_rect;
  ns_rect.origin = point;
  ns_rect.size.width = ns_rect.size.height = 0.0F;
  kdu_coords coords = renderer->convert_region_from_doc_view(ns_rect).pos;
  renderer->edit_metadata_at_point(&coords);
}

/*****************************************************************************/
/*              kdms_document_view:handle_shape_editor_click                   */
/*****************************************************************************/

- (void)handle_shape_editor_click:(NSPoint)point
{
  kdu_coords shape_point;
  jpx_roi_editor *sed =
    renderer->map_shape_point_from_doc_view(point,shape_point);
  if (sed == NULL)
    return;
  kdu_dims update_region;
  if (renderer->get_shape_scribble_mode())
    {
      update_region = sed->clear_scribble_points();
      if (!update_region.is_empty())
        [renderer->editor update_shape_editor_controls];
      shape_scribbling = true;
      update_region.augment(sed->add_scribble_point(shape_point));
    }
  else
    {
      if (!sed->find_nearest_anchor(shape_point,false))
        return;
      NSPoint nearest;
      renderer->map_shape_point_to_doc_view(shape_point,nearest);
      double dx=nearest.x-point.x, dy=nearest.y-point.y;
      if ((dx*dx+dy*dy) < 4.5*4.5)
        { 
          shape_dragging = true;
          mouse_start = mouse_last = point;
          update_region = sed->select_anchor(shape_point,true);
        }
      else
        update_region = sed->cancel_selection();
      if (!update_region.is_empty())
        [renderer->editor update_shape_editor_controls];
    }
  if (!update_region.is_empty())
    [self repaint_shape_region:update_region];
}

/*****************************************************************************/
/*              kdms_document_view:handle_shape_editor_drag                  */
/*****************************************************************************/

- (void)handle_shape_editor_drag:(NSPoint)point
{
  kdu_coords shape_point;
  if ([self snap_doc_point:point to_shape:&shape_point])
    {
      jpx_roi_editor *sed = renderer->get_shape_editor();
      kdu_dims update_region = sed->drag_selected_anchor(shape_point);
      if (!update_region.is_empty())
        [self repaint_shape_region:update_region];
      mouse_last = point;
    }
}

/*****************************************************************************/
/*              kdms_document_view:handle_shape_editor_move                  */
/*****************************************************************************/

- (void)handle_shape_editor_move:(NSPoint)point
{
  [self handle_shape_editor_drag:point];
  point = mouse_last; // Ensure we move to the last valid point  
  kdu_coords shape_point;
  shape_dragging = false;
  jpx_roi_editor *sed = renderer->get_shape_editor();
  if (sed == NULL)
    return;
  kdu_dims update_region;
  if (((point.x == mouse_start.x) && (point.y == mouse_start.y)) ||
      ![self snap_doc_point:point to_shape:&shape_point])
    update_region = sed->cancel_drag();
  else
    { 
      update_region = sed->move_selected_anchor(shape_point);
      update_region.augment(renderer->shape_editor_adjust_path_thickness());
    }
  [renderer->editor update_shape_editor_controls];
  if (!update_region.is_empty())
    [self repaint_shape_region:update_region];
}

/*****************************************************************************/
/*              kdms_document_view:add_shape_scribble_point                  */
/*****************************************************************************/

- (void)add_shape_scribble_point:(NSPoint)point
{
  kdu_coords shape_point;
  jpx_roi_editor *sed =
    renderer->map_shape_point_from_doc_view(point,shape_point);
  if (sed != NULL)
    {
      kdu_dims update_region =
        sed->add_scribble_point(shape_point);
      if (!update_region.is_empty())
        [self repaint_shape_region:update_region];
    }
}

/*****************************************************************************/
/*              kdms_document_view:snap_doc_point:to_shape                   */
/*****************************************************************************/

- (bool)snap_doc_point:(NSPoint)point to_shape:(kdu_coords *)snapped_point
{
  kdu_coords shape_point;
  jpx_roi_editor *sed =
    renderer->map_shape_point_from_doc_view(point,shape_point);
  if (sed == NULL)
    return false;
  kdu_coords test_shape_point;
  NSPoint test_doc_point;
  kdu_coords nearest_shape_point = shape_point;
  double min_doc_dist_sq = 20.0; // Snap only if dist^2 is less than this
  double dx, dy, dist_sq;
  
  test_shape_point = shape_point;
  if (sed->find_nearest_anchor(test_shape_point,true))
    { // Consider snapping to a non-selected anchor point
      renderer->map_shape_point_to_doc_view(test_shape_point,test_doc_point);
      dx = test_doc_point.x-point.x;  dy = test_doc_point.y-point.y;
      dist_sq = dx*dx + dy*dy;
      if (dist_sq < min_doc_dist_sq)
        { 
          nearest_shape_point = test_shape_point;
          if (sed->can_move_selected_anchor(nearest_shape_point,false))
            { *snapped_point = nearest_shape_point; return true; }
        }
    }
  
  test_shape_point = shape_point;
  if (sed->find_nearest_boundary_point(test_shape_point,true))
    { // Consider snapping to the boundary of any non-selected region
      renderer->map_shape_point_to_doc_view(test_shape_point,test_doc_point);
      dx = test_doc_point.x-point.x;  dy = test_doc_point.y-point.y;
      dist_sq = dx*dx + dy*dy;
      if (dist_sq < min_doc_dist_sq)
        { min_doc_dist_sq = dist_sq; nearest_shape_point = test_shape_point; }
    }
  
  test_shape_point = shape_point;
  if (sed->find_nearest_guide_point(test_shape_point))
    { // Consider snapping to the nearest guide line -- see definitions
      renderer->map_shape_point_to_doc_view(test_shape_point,test_doc_point);
      dx = test_doc_point.x-point.x;  dy = test_doc_point.y-point.y;
      dist_sq = dx*dx + dy*dy;
      if (dist_sq < min_doc_dist_sq)
        { min_doc_dist_sq = dist_sq; nearest_shape_point = test_shape_point; }
    }
  
  if (!sed->can_move_selected_anchor(nearest_shape_point,false))
    return false;
  *snapped_point = nearest_shape_point;
  return true;
}

/*****************************************************************************/
/*                 kdms_document_view:repaint_shape_region                   */
/*****************************************************************************/

- (void)repaint_shape_region:(kdu_dims) region
{
  kdu_coords min=region.pos, max=min+region.size; min -= kdu_coords(1,1);
  NSPoint p1, p2;
  if ((renderer == NULL) ||
      !(renderer->map_shape_point_to_doc_view(min,p1) &&
        renderer->map_shape_point_to_doc_view(max,p2)))
    return;
  NSRect rect;
  if (p1.x < p2.x)
    { rect.origin.x = p1.x; rect.size.width = p2.x-p1.x; }
  else
    { rect.origin.x = p2.x; rect.size.width = p1.x-p2.x; }
  if (p1.y < p2.y)
    { rect.origin.y = p1.y; rect.size.height = p2.y-p1.y; }
  else
    { rect.origin.y = p2.y; rect.size.height = p1.y-p2.y; }
  rect.origin.x -= 7.0F; rect.origin.y -= 7.0F;
  rect.size.width += 14.0F; rect.size.height += 14.0F;
  [self setNeedsDisplayInRect:rect];
}

/*****************************************************************************/
/*                kdms_document_view:acceptsFirstResponder                   */
/*****************************************************************************/

- (BOOL)acceptsFirstResponder
{
  return YES;
}

/*****************************************************************************/
/*                 kdms_document_view:becomeFirstResponder                   */
/*****************************************************************************/

- (BOOL) becomeFirstResponder
{
  self->is_first_responder = true;
  return [super becomeFirstResponder];
}

/*****************************************************************************/
/*                 kdms_document_view:resignFirstResponder                   */
/*****************************************************************************/

- (BOOL) resignFirstResponder
{
  self->is_first_responder = false;
  return [super resignFirstResponder];
}

/*****************************************************************************/
/*                    kdms_document_view:rightMouseDown                      */
/*****************************************************************************/

- (void)rightMouseDown:(NSEvent *)mouse_event
{
  [wnd cancel_pending_show];
  [self cancel_focus_drag];
  NSPoint window_point = [mouse_event locationInWindow];
  NSPoint doc_view_point = [self convertPoint:window_point fromView:nil];
  [self handle_right_click:doc_view_point];
}

/*****************************************************************************/
/*                      kdms_document_view:mouseDown                         */
/*****************************************************************************/

- (void)mouseDown:(NSEvent *)mouse_event
{
  [wnd cancel_pending_show];
  NSUInteger modifier_flags = [mouse_event modifierFlags];
  bool shift_key_down = (modifier_flags & NSShiftKeyMask)?true:false;
  bool ctrl_key_down = (modifier_flags & NSControlKeyMask)?true:false;
  NSPoint window_point = [mouse_event locationInWindow];
  NSPoint doc_view_point = [self convertPoint:window_point fromView:nil];
  shape_dragging = false;
  if (ctrl_key_down)
    [self handle_right_click:doc_view_point];
  else if ((renderer != NULL) && (renderer->get_shape_editor() != NULL))
    [self handle_shape_editor_click:doc_view_point];
  else if (shift_key_down)
    [self start_viewdrag_at:doc_view_point];
  else if ([mouse_event clickCount] == 1)
    [self start_focusbox_at:doc_view_point];
  else
    [self handle_double_click:doc_view_point];
}

/*****************************************************************************/
/*                   kdms_document_view:mouseDragged                         */
/*****************************************************************************/

- (void)mouseDragged:(NSEvent *)mouse_event
{
  NSPoint window_point = [mouse_event locationInWindow];
  NSPoint doc_view_point = [self convertPoint:window_point fromView:nil];
  if ((renderer != NULL) && (renderer->get_shape_editor() != NULL))
    { 
      if (shape_dragging)
        [self handle_shape_editor_drag:doc_view_point];
      else if (shape_scribbling)
        [self add_shape_scribble_point:doc_view_point];
    }
  else
    [self mouse_drag_to:doc_view_point];
}

/*****************************************************************************/
/*                        kdms_document_view:mouseUp                         */
/*****************************************************************************/

- (void)mouseUp:(NSEvent *)mouse_event
{
  NSPoint window_point = [mouse_event locationInWindow];
  NSPoint doc_view_point = [self convertPoint:window_point fromView:nil];
  if (is_focussing && ![self end_focus_drag_at:doc_view_point])
    [self handle_single_click:doc_view_point];
  if ((renderer != NULL) && (renderer->get_shape_editor() != NULL))
    { 
      if (shape_dragging)
        [self handle_shape_editor_move:doc_view_point];
      else if (shape_scribbling)
        {
          [self add_shape_scribble_point:doc_view_point];
          [renderer->editor update_shape_editor_controls];
          shape_scribbling = false;
        }
    }
  shape_dragging = false;
}

/*****************************************************************************/
/*                         kdms_document_view:keyDown                        */
/*****************************************************************************/

- (void)keyDown:(NSEvent *)key_event
{
  [wnd cancel_pending_show];
  [self cancel_focus_drag];
  NSUInteger modifier_flags = [key_event modifierFlags];
  if (modifier_flags & NSNumericPadKeyMask)
    {
      bool shift_key_down = (modifier_flags & NSShiftKeyMask)?true:false;
      bool control_key_down = (modifier_flags & NSControlKeyMask)?true:false;
      bool command_key_down = (modifier_flags & NSCommandKeyMask)?true:false;
      NSString *the_arrow = [key_event charactersIgnoringModifiers];
      if (([the_arrow length] > 0) && !command_key_down)
        {
          unichar key_char = [the_arrow characterAtIndex:0];
          kdu_dims shape_regn;
          int na = (control_key_down)?1:10; // Nudge amount
          if (key_char == NSLeftArrowFunctionKey)
            {
              if ((renderer != NULL) && !shift_key_down)
                renderer->scroll_relative_to_view(-0.2F,0.0F);
              else if (renderer->nudge_selected_shape_points(-na,0,shape_regn))
                [self repaint_shape_region:shape_regn];
              else if ((renderer != NULL) && renderer->can_do_FocusLeft(nil))
                renderer->menu_FocusLeft();
              return;
            }
          else if (key_char == NSRightArrowFunctionKey)
            {
              if ((renderer != NULL) && !shift_key_down)
                renderer->scroll_relative_to_view(0.2F,0.0F);
              else if (renderer->nudge_selected_shape_points(na,0,shape_regn))
                [self repaint_shape_region:shape_regn];
              else if ((renderer != NULL) && renderer->can_do_FocusRight(nil))
                renderer->menu_FocusRight();
              return;
            }
          else if (key_char == NSUpArrowFunctionKey)
            {
              if ((renderer != NULL) && !shift_key_down)
                renderer->scroll_relative_to_view(0.0F,-0.2F);
              else if (renderer->nudge_selected_shape_points(0,-na,shape_regn))
                [self repaint_shape_region:shape_regn];
              else if ((renderer != NULL) && renderer->can_do_FocusUp(nil))
                renderer->menu_FocusUp();
              return;
            }
          else if (key_char == NSDownArrowFunctionKey)
            {
              if ((renderer != NULL) && !shift_key_down)
                renderer->scroll_relative_to_view(0.0F,0.2F);
              else if (renderer->nudge_selected_shape_points(0,na,shape_regn))
                [self repaint_shape_region:shape_regn];
              else if ((renderer != NULL) && renderer->can_do_FocusDown(nil))
                renderer->menu_FocusDown();
              return;
            }
        }
    }
  [super keyDown:key_event];
}


@end // kdms_document_view

/*===========================================================================*/
/*                                kdms_window                                */
/*===========================================================================*/

@implementation kdms_window

/*****************************************************************************/
/*                 kdms_window:initWithManager:andCursors                    */
/*****************************************************************************/

- (void)initWithManager:(kdms_window_manager *)manager
            andCursors:(NSCursor **)cursors
{
  int p; // Used to index status panes
  
  // Initialize the private members to empty values for the moment
  window_manager = nil; // Until we are sure everything has worked
  renderer = NULL;
  doc_view = nil;
  scroll_view = nil;
  status_bar = nil;
  manager_cursors = cursors;
  doc_cursor_normal = cursors[0];
  doc_cursor_dragging = cursors[1];
  status_height = 0.0F;
  progress_indicator = nil;
  for (p=0; p < 3; p++)
    {
      status_panes[p] = nil;
      status_pane_lengths[p] = 0;
      status_pane_caches[p] = NULL;
    }
  catalog_panel = nil;
  catalog_tools = nil;
  catalog_paste_bar = nil;
  catalog_buttons[0] = catalog_buttons[1] = catalog_buttons[2] = nil;
  catalog_panel_width = 0.0F;
  
  // Find an initial frame location and size for the window
  NSRect screen_rect = [[NSScreen mainScreen] frame];
  NSRect frame_rect; 
  frame_rect.origin.x = frame_rect.origin.y = 0.0F;
  frame_rect.size.width = (float) ceil(screen_rect.size.width * 0.3);
  frame_rect.size.height = (float) ceil(screen_rect.size.height * 0.3);
  NSUInteger window_style = NSTitledWindowMask | NSClosableWindowMask |
                            NSMiniaturizableWindowMask |
                            NSResizableWindowMask;
  NSRect content_rect = [NSWindow contentRectForFrameRect:frame_rect
                         styleMask:window_style];
                         
  // Create window and link it in
  [super initWithContentRect:content_rect
                   styleMask:window_style
                     backing:NSBackingStoreBuffered defer:NO];
  manager->add_window(self);
  window_manager = manager; // Now we are on manager's list, we can record it
  
  // Get content view
  NSView *super_view = [self contentView];
  content_rect = [super_view frame]; // Get the relative view rectangle; size
              // should be the same, but origin is now relative to the window.

  // Determine the font, colours and dimensions for the status panes
  CGFloat font_size = [NSFont systemFontSizeForControlSize:NSSmallControlSize];
  status_font = [NSFont systemFontOfSize:font_size];
  [status_font retain];
  status_height = (float)
    ceil([status_font ascender] - [status_font descender] + 6.0F);
  NSColor *status_background_colour =
    [NSColor colorWithDeviceRed:0.95F green:0.95F blue:1.0F alpha:1.0F];
  NSColor *status_text_colour =
    [NSColor colorWithDeviceRed:0.3F green:0.1F blue: 0.0F alpha:1.0F];
  
  // Create status bar and panes
  NSRect status_bar_rect = content_rect;
  status_bar_rect.size.height = status_height;
  status_bar = [NSView alloc];
  [status_bar initWithFrame:status_bar_rect];
  [status_bar setAutoresizingMask:(NSViewWidthSizable | NSViewMinXMargin)];
  [super_view addSubview:status_bar];
  NSRect pane_rects[3];
  for (p=0; p < 3; p++)
    {
      pane_rects[p] = content_rect;
      pane_rects[p].size.height = status_height;
    }
  pane_rects[0].size.width = pane_rects[2].size.width =
    (float) floor(content_rect.size.width * 0.3F);
  pane_rects[1].size.width = content_rect.size.width -
    2 * pane_rects[0].size.width;
  pane_rects[1].origin.x += pane_rects[0].size.width;
  pane_rects[2].origin.x = pane_rects[1].origin.x + pane_rects[1].size.width;
  for (p=0; p < 3; p++)
    {
      NSTextField *pane = status_panes[p] = [NSTextField alloc];
      [pane initWithFrame:pane_rects[p]];
      [pane setBezeled:YES];
      [pane setBezelStyle:NSTextFieldSquareBezel];
      [pane setEditable:NO];
      [pane setDrawsBackground:YES];
      [pane setBackgroundColor:status_background_colour];
      [pane setTextColor:status_text_colour];
      [pane setAutoresizingMask:(NSViewWidthSizable | NSViewMaxYMargin |
                                 NSViewMinXMargin | NSViewMaxXMargin)];
      NSTextFieldCell *cell = [pane cell];
      [cell setFont:status_font];
      [status_bar addSubview:pane];
      [pane release];
    }
  
  // Create `NSScrollView' for the window
  NSRect scroll_view_rect = content_rect;
  scroll_view_rect.origin.y += pane_rects[0].size.height;
  scroll_view_rect.size.height -= pane_rects[0].size.height;
  int resizing_mask = NSViewWidthSizable | NSViewHeightSizable;
  scroll_view = [NSScrollView alloc];
  [scroll_view initWithFrame:scroll_view_rect];
  [scroll_view setBorderType:NSNoBorder];
  [scroll_view setAutoresizingMask:resizing_mask];
  [scroll_view setHasVerticalScroller:YES];
  [scroll_view setHasHorizontalScroller:YES];
  [[scroll_view contentView] setCopiesOnScroll:YES];
  [super_view addSubview:scroll_view];
  
  // Create `renderer' and `kdms_document_view' objects for the window
  doc_view = [kdms_document_view alloc];
  [doc_view initWithFrame:content_rect];
  renderer = new kdms_renderer(self,doc_view,scroll_view,manager);
  [doc_view set_renderer:renderer];
  [doc_view set_owner:self];
  [scroll_view setDocumentView:doc_view];
  [scroll_view setDocumentCursor:doc_cursor_normal];
  
  // Set things up to receive notifications of changes in the `scroll_view'.
  [[NSNotificationCenter
            defaultCenter] addObserver:self
                              selector:@selector(scroll_view_BoundsDidChange:)
                                  name:NSViewBoundsDidChangeNotification
                                object:[scroll_view contentView]];
  [[NSNotificationCenter
            defaultCenter] addObserver:self
                              selector:@selector(scroll_view_BoundsDidChange:)
                                  name:NSViewFrameDidChangeNotification
                                object:[scroll_view contentView]];
    
  // Release local references and display the window
  renderer->close_file(); // Causes the correct title to be written
  [scroll_view release]; // Retained by the window's content view
  [status_bar release]; // Retained by the window's content view
  [doc_view release]; // Retained by the scroll view
  [self center];
  [self makeKeyAndOrderFront:self]; // Display the window
}

/*****************************************************************************/
/*                             kdms_window:dealloc                           */
/*****************************************************************************/

- (void)dealloc
{
  if (window_manager != NULL)
    {
      kdms_window_manager *manager = window_manager;
      window_manager = nil;
      manager->remove_window(self);
    }
  if (renderer != NULL)
    { delete renderer; renderer = NULL; }
  [status_font release];
  [super dealloc];
}

/*****************************************************************************/
/*                           kdms_window:open_file                           */
/*****************************************************************************/

- (void)open_file:(NSString *)fname
{
  if (renderer == NULL)
    return;
  if (renderer->have_unsaved_edits &&
      (interrogate_user("You have edited the existing file but not saved "
                        "these edits ... perhaps you saved the file in a "
                        "format which cannot hold all the metadata ("
                        "use JPX).  Do you still want to close the existing "
                        "file to open a new one?",
                        "Cancel","Close without Saving") == 0))
      return;
  const char *filename = [fname UTF8String];
  if (window_manager->check_open_file_replaced(filename) &&
      !interrogate_user("A file you have elected to open is out of date, "
                        "meaning that another open window in the application "
                        "has saved over the file but not yet released its "
                        "access, so that the saved copy can replace the "
                        "original.  Do you still wish to open the old copy?",
                        "Cancel","Open Anyway"))
    return;
  renderer->open_file(filename);
}

/*****************************************************************************/
/*                           kdms_window:open_url                            */
/*****************************************************************************/

- (void)open_url:(NSString *)url
{
  if (renderer == NULL)
    return;
  if (renderer->have_unsaved_edits &&
      (interrogate_user("You have edited the existing file but not saved "
                        "these edits ... perhaps you saved the file in a "
                        "format which cannot hold all the metadata ("
                        "use JPX).  Do you still want to close the existing "
                        "file to open a new one?",
                        "Cancel","Close without Saving") == 0))
    return;
  const char *filename_or_url = [url UTF8String];
  renderer->open_file(filename_or_url);
}

/*****************************************************************************/
/*                   kdms_window:application_can_terminate                   */
/*****************************************************************************/

- (bool)application_can_terminate
{
  if (renderer == NULL)
    return true;
  if ((renderer->editor != nil) &&
      (interrogate_user("You have an open metadata editor for one or more "
                        "windows that will be closed when this application "
                        "terminates. Are you sure you want to quit the "
                        "application?",
                        "Cancel","Quit Application") != 1))
    return false;
  if (renderer->have_unsaved_edits &&
      (interrogate_user("You have unsaved edits in one or more windows that "
                        "will be closed when this application terminates.  "
                        "Are you sure you want to quit the application?",
                        "Cancel","Quit Application") != 1))
    return false;
  return true;
}

/*****************************************************************************/
/*                    kdms_window:application_terminating                    */
/*****************************************************************************/

- (void)application_terminating
{
  if (renderer != NULL)
    renderer->perform_essential_cleanup_steps();
}

/*****************************************************************************/
/*                             kdms_window:close                             */
/*****************************************************************************/

- (void) close
{
  if (renderer != NULL)
    {
      if ((renderer->editor != nil) &&
          (interrogate_user("You have an open metadata editor, which will "
                            "be closed if you continue.  Do you still want "
                            "to close the window?",
                            "Cancel","Continue with Close") == 0))
        return;
      if (renderer->have_unsaved_edits &&
          (interrogate_user("You have edited this file but not saved these "
                            "edits ... perhaps you saved the file in a "
                            "format which cannot hold all the metadata ("
                            "use JPX).  Do you still want to close?",
                            "Cancel","Close without Saving") == 0))
        return;
      if (renderer->metashow)
        [renderer->metashow close];
      if (renderer->editor)
        [renderer->editor close];
      if (renderer->catalog_source)
        renderer->set_metadata_catalog_source(nil); // So it gets released
    }
  [super close];  
}

/*****************************************************************************/
/*                            kdms_window:on_idle                            */
/*****************************************************************************/

- (bool)on_idle
{
  if (renderer == NULL)
    return false;
  else
    return renderer->on_idle();
}

/*****************************************************************************/
/*                       kdms_window:adjust_playclock                        */
/*****************************************************************************/

- (void)adjust_playclock:(double)delta
{
  if (renderer != NULL)
    renderer->adjust_playclock(delta);
}

/*****************************************************************************/
/*                 kdms_window:wakeupScheduledFor:occurredAt                 */
/*****************************************************************************/

- (void)wakeupScheduledFor:(CFAbsoluteTime)scheduled_time
                occurredAt:(CFAbsoluteTime)current_time
{
  if (renderer != NULL)
    renderer->wakeup(scheduled_time,current_time);
}

/*****************************************************************************/
/*                     kdms_window:client_notification                       */
/*****************************************************************************/

- (void)client_notification
{
  if (renderer != NULL)
    renderer->client_notification();
}

/*****************************************************************************/
/*                     kdms_window:cancel_pending_show                       */
/*****************************************************************************/

- (void)cancel_pending_show
{
  if (renderer != NULL)
    renderer->cancel_pending_show();
}

/*****************************************************************************/
/*                    kdms_window:create_metadata_catalog                    */
/*****************************************************************************/

- (void)create_metadata_catalog
{
  [self remove_metadata_catalog]; // Reset everything, just in case
  if (renderer == NULL)
    return;
  
  // Start by disabling auto-resizing and resizing the window itself
  NSView *super_view = [self contentView];
  [super_view setAutoresizesSubviews:NO];
  NSSize max_window_size = [self maxSize];
  catalog_panel_width = 280.0F; // A reasonable panel width
  max_window_size.width += catalog_panel_width;
  [self setMaxSize:max_window_size];
  NSRect frame_rect = [self frame];
  frame_rect.size.width += catalog_panel_width;
  [self setFrame:frame_rect display:NO];
  
  // Find the bounding rectangles for the top-level views
  NSRect content_rect = [super_view frame];
  
  NSRect scroll_view_rect = content_rect;
  scroll_view_rect.origin.y = status_height;
  scroll_view_rect.size.height -= scroll_view_rect.origin.y;
  scroll_view_rect.size.width -= catalog_panel_width;
  [scroll_view setFrame:scroll_view_rect];
  
  NSRect status_bar_rect = scroll_view_rect;
  status_bar_rect.origin.y = 0.0F;
  status_bar_rect.size.height = status_height;
  [status_bar setFrame:status_bar_rect];
  
  NSRect catalog_rect = content_rect;
  catalog_rect.origin.x = scroll_view_rect.size.width+3;
  catalog_rect.size.width -= catalog_rect.origin.x+3;
  catalog_rect.origin.y += 3;
  catalog_rect.size.height -= 3;

  // Start by creating the catalog tools box
  NSRect tools_rect = catalog_rect;
  tools_rect.size.height = 3*status_height; // Rough guess, corrected later
  tools_rect.origin.y += catalog_rect.size.height - tools_rect.size.height;
  catalog_tools = [NSBox alloc];
  [catalog_tools initWithFrame:tools_rect];
  [catalog_tools setTitlePosition:NSAtTop];
  [catalog_tools setTitle:@"Catalog Pastebar/Tools"];
  [catalog_tools setBorderType:NSLineBorder];
  [catalog_tools setAutoresizingMask:NSViewMinXMargin|NSViewMinYMargin];
  [super_view addSubview:catalog_tools];
  [catalog_tools release];
  NSView *tools_view = [catalog_tools contentView];
  NSRect tools_view_rect = [tools_view frame];
  NSSize tools_margin = [catalog_tools contentViewMargins];
  NSRect tools_place_rect = tools_view_rect;
  tools_place_rect.size.width += tools_margin.width*2.0F - 4;
  tools_place_rect.origin.x = -(tools_margin.width - 2);
  tools_place_rect.size.height += tools_margin.height*2.0F - 4;
  tools_place_rect.origin.y = -tools_margin.height;
  
  float delta_height = 28+status_height - tools_place_rect.size.height;
  tools_rect.size.height += delta_height;
  tools_place_rect.size.height += delta_height;
  tools_rect.origin.y -= delta_height;
  [catalog_tools setFrame:tools_rect];
  
  // Now create the pastebar
  NSColor *catalog_background_colour =
    [NSColor colorWithDeviceRed:1.0F green:1.0F blue:0.95F alpha:1.0F];
  NSRect pastebar_rect;
  pastebar_rect = tools_place_rect;
  pastebar_rect.origin.y += 26;
  pastebar_rect.size.height = status_height;
  catalog_paste_bar = [NSTextField alloc];
  [catalog_paste_bar initWithFrame:pastebar_rect];
  [catalog_paste_bar setBezeled:YES];
  [catalog_paste_bar setBezelStyle:NSTextFieldSquareBezel];
  [catalog_paste_bar setEditable:NO];
  [catalog_paste_bar setDrawsBackground:YES];
  [catalog_paste_bar setBackgroundColor:catalog_background_colour];
  [catalog_paste_bar setAutoresizingMask:NSViewWidthSizable];
  [[catalog_paste_bar cell] setFont:status_font];
  [tools_view addSubview:catalog_paste_bar];
  [catalog_paste_bar release];
  
  // Now create the buttons
  int b;
  unichar arrows[3] = {0x21E6,0x21E8,0x27F3};
  NSFont *arrow_font =
    [NSFont boldSystemFontOfSize:
     2.0F*[NSFont systemFontSizeForControlSize:NSRegularControlSize]];
  for (b=0; b < 3; b++)
    {
      NSRect button_rect = tools_place_rect;
      button_rect.origin.y += 4;
      button_rect.size.height = 19;
      button_rect.size.width = tools_place_rect.size.width * 0.3F;
      if (b == 0)
        button_rect.origin.x += 4;
      if (b == 1)
        button_rect.origin.x +=
          (tools_place_rect.size.width - button_rect.size.width) * 0.5F;
      else if (b == 2)
        button_rect.origin.x +=
          tools_place_rect.size.width - button_rect.size.width-4;
      catalog_buttons[b] = [NSButton alloc];
      [catalog_buttons[b] initWithFrame:button_rect];
      if (b == 2)
        [catalog_buttons[b] setTitle:@"peers"];
      else
        {
          [[catalog_buttons[b] cell] setFont:arrow_font];
          [catalog_buttons[b] setTitle:[NSString stringWithCharacters:arrows+b
                                                               length:1]];
        }
      [catalog_buttons[b] setButtonType:NSMomentaryLightButton];
      [catalog_buttons[b] setBordered:YES];
      [catalog_buttons[b] setBezelStyle:NSRoundRectBezelStyle];
      [catalog_buttons[b] setEnabled:NO];
      [catalog_buttons[b] setAutoresizingMask:
       NSViewMaxXMargin|NSViewMinXMargin|NSViewWidthSizable];
      [tools_view addSubview:catalog_buttons[b]];
    }

  // Now create the catalog panel and view
  NSRect catalog_panel_rect = catalog_rect;
  catalog_panel_rect.size.height -= tools_rect.size.height;
  catalog_panel = [NSScrollView alloc];
  [catalog_panel initWithFrame:catalog_panel_rect];
  [catalog_panel setBorderType:NSBezelBorder];
  [catalog_panel setAutoresizingMask:NSViewHeightSizable|NSViewMinXMargin];
  [catalog_panel setHasVerticalScroller:YES];
  [catalog_panel setHasHorizontalScroller:YES];
  [super_view addSubview:catalog_panel];
  [catalog_panel release];
  
  kdms_catalog *catalog_view = [kdms_catalog alloc];
  [catalog_view initWithFrame:[[catalog_panel contentView] frame]
                     pasteBar:catalog_paste_bar
                   backButton:catalog_buttons[0]
                    fwdButton:catalog_buttons[1]
                   peerButton:catalog_buttons[2]
                  andRenderer:renderer];
  [catalog_view setBackgroundColor:catalog_background_colour];
  [catalog_panel setDocumentView:catalog_view];
  [catalog_view release]; // Retained by catalog_panel

  // Finalize the configuration
  [super_view setAutoresizesSubviews:YES];
  renderer->set_metadata_catalog_source(catalog_view);
}

/*****************************************************************************/
/*                    kdms_window:remove_metadata_catalog                    */
/*****************************************************************************/

- (void)remove_metadata_catalog
{
  NSView *super_view = [self contentView];
  [super_view setAutoresizesSubviews:NO];
  
  if (catalog_panel)
    {
      [catalog_tools removeFromSuperview];
      [catalog_panel removeFromSuperview];
      catalog_panel = nil; // Should have been auto-released by above function
      catalog_tools = nil; // Should have been auto-released by above function
      catalog_paste_bar = nil;
      catalog_buttons[0] = catalog_buttons[1] = catalog_buttons[2] = nil;
      catalog_panel_width = 0.0F;
    }
  NSRect frame_rect = [self frame];
  if (renderer != NULL)
    {
      NSSize max_size;
      if (renderer->get_max_scroll_view_size(max_size))
        {
          max_size.height += status_height;
          NSRect max_content_rect;
          max_content_rect.size = max_size;
          max_content_rect.origin.x = max_content_rect.origin.y = 0.0F;
          NSRect max_window_rect =
            [self frameRectForContentRect:max_content_rect];
          bool need_frame_resize = false;
          if (frame_rect.size.width > max_window_rect.size.width)
            {
              frame_rect.size.width = max_window_rect.size.width;
              need_frame_resize = true;
            }
          if (frame_rect.size.height > max_window_rect.size.height)
            {
              frame_rect.size.height = max_window_rect.size.height;
              need_frame_resize = true;
            }
          if (need_frame_resize)
            [self setFrame:frame_rect display:NO];
          [self setMaxSize:max_window_rect.size];
        }
    }
  frame_rect = [super_view frame];
  if (status_bar)
    { // Adjust status bar location and size
      NSRect bar_rect = frame_rect;
      bar_rect.size.height = status_height;
      bar_rect.origin.y = 0.0F;
      [status_bar setFrame:bar_rect];
    }
  if (scroll_view)
    { // Adjust scroll_view to occupy the entire window, apart from status bar
      NSRect scroll_rect = frame_rect;
      scroll_rect.origin.y = status_height;
      scroll_rect.size.height -= scroll_rect.origin.y;
      scroll_rect.origin.x = 0.0F;
      [scroll_view setFrame:scroll_rect];
    }

  [super_view setAutoresizesSubviews:YES];
  if (renderer != NULL)
    renderer->set_metadata_catalog_source(NULL);
}

/*****************************************************************************/
/*                       kdms_window:set_progress_bar                        */
/*****************************************************************************/

- (void)set_progress_bar:(double)value
{
  if (progress_indicator == nil)
    {
      NSRect pane_rect = [status_panes[0] frame];
      float pane_height = pane_rect.size.height;
      float progress_height = pane_height * 0.5F;
      status_height = pane_height + progress_height;
      
      // Start by disabling auto-resizing and resize the window itself
      NSView *super_view = [self contentView];
      [super_view setAutoresizesSubviews:NO];
      NSSize max_window_size = [self maxSize];
      max_window_size.height += progress_height;
      NSRect screen_size = [[NSScreen mainScreen] visibleFrame];
      if (max_window_size.height > screen_size.size.height)
        max_window_size.height = screen_size.size.height;
      [self setMaxSize:max_window_size];
      NSRect frame_rect = [self frame];
      frame_rect.size.height += progress_height;
      if (frame_rect.size.height > max_window_size.height)
        frame_rect.size.height = max_window_size.height;
      [self setFrame:frame_rect display:NO];
      
      // Find the bounding rectangles for the top-level views
      NSRect content_rect = [super_view frame];

      NSRect scroll_view_rect = content_rect;
      scroll_view_rect.origin.y = status_height;
      scroll_view_rect.size.height -= scroll_view_rect.origin.y;
      scroll_view_rect.size.width -= catalog_panel_width;
      [scroll_view setFrame:scroll_view_rect];

      NSRect status_bar_rect = scroll_view_rect;
      status_bar_rect.origin.y = 0.0F;
      status_bar_rect.size.height = status_height;
      [status_bar setFrame:status_bar_rect];
      
      if (catalog_panel != nil)
        {
          NSRect catalog_panel_rect = content_rect;
          catalog_panel_rect.origin.x = scroll_view_rect.size.width;
          catalog_panel_rect.size.width -= catalog_panel_rect.origin.x;
          [catalog_panel setFrame:catalog_panel_rect];
        }
      
      // Add in the progress indicator
      NSRect progress_rect = status_bar_rect;
      progress_rect.origin.y = pane_height;
      progress_rect.size.height = progress_height;
      progress_indicator = [NSProgressIndicator alloc];
      [progress_indicator initWithFrame:progress_rect];
      [progress_indicator setIndeterminate:NO];
      [progress_indicator
       setAutoresizingMask:(NSViewWidthSizable | NSViewMinYMargin |
                            NSViewMinXMargin | NSViewMaxXMargin)];
      [status_bar addSubview:progress_indicator];
      [progress_indicator release]; // Retained by the status bar
      
      // Re-activate auto-resizing
      [super_view setAutoresizesSubviews:YES];
    }
  [progress_indicator setDoubleValue:value];
}

/*****************************************************************************/
/*                      kdms_window:remove_progress_bar                      */
/*****************************************************************************/

- (void)remove_progress_bar
{
  if (!progress_indicator)
    return;
  if (catalog_panel)
    [self remove_metadata_catalog];
  assert(catalog_panel == nil);
  
  // Start by disabling auto-resize and removing the progress indicator
  NSView *super_view = [self contentView];
  [super_view setAutoresizesSubviews:NO];
  [progress_indicator removeFromSuperview];
  progress_indicator = nil;
  
  // Now resize the window and top level views
  NSRect pane_rect = [status_panes[0] frame];
  float pane_height = pane_rect.size.height;
  float progress_height = pane_height * 0.5F;
  assert(status_height == (pane_height + progress_height));
  status_height = pane_height;
  
  NSRect frame_rect = [self frame];
  frame_rect.size.height -= progress_height;
  [self setFrame:frame_rect display:NO];
  
  if (renderer != NULL)
    {
      NSSize max_size;
      if (renderer->get_max_scroll_view_size(max_size))
        {
          max_size.height += status_height;
          NSRect max_content_rect;
          max_content_rect.size = max_size;
          max_content_rect.origin.x = max_content_rect.origin.y = 0.0F;
          NSRect max_window_rect =
            [self frameRectForContentRect:max_content_rect];
          [self setMaxSize:max_window_rect.size];
        }
    }
  
  NSRect bar_rect = frame_rect;
  bar_rect.size.height = status_height;
  bar_rect.origin.y = 0.0F;
  [status_bar setFrame:bar_rect];
  
  if (scroll_view)
    { // Adjust the scroll view to occupy the remainder of the window
      NSRect scroll_rect = frame_rect;
      scroll_rect.origin.y = status_height;
      scroll_rect.size.height -= status_height;
      scroll_rect.origin.x = 0.0F;
      [scroll_view setFrame:scroll_rect];
    }
  
  // Re-enable sub-view resizing
  [super_view setAutoresizesSubviews:YES];
}

/*****************************************************************************/
/*                       kdms_window:get_bottom_margin_height                */
/*****************************************************************************/

- (float)get_bottom_margin_height
{
  return status_height;
}

/*****************************************************************************/
/*                    kdms_window:get_right_margin_width                     */
/*****************************************************************************/

- (float)get_right_margin_width
{
  return catalog_panel_width; 
}

/*****************************************************************************/
/*                       kdms_window:set_status_strings                      */
/*****************************************************************************/

- (void)set_status_strings:(const char **)three_strings
{
  int p; // Panel index
  
  // Start by finding the status panes which have changed, and allocating
  // space to cache the new status strings.
  int total_chars = 0; // Including NULL chars.
  bool pane_updated[3] = {false,false,false};
  for (p=0; p < 3; p++)
    {
      const char *string = three_strings[p];
      if (string == NULL)
        string = "";
      int len = strlen(string)+1;
      pane_updated[p] = ((len != status_pane_lengths[p]) ||
                         (strcmp(string,status_pane_caches[p]) != 0));
      if ((total_chars + len) <= 256)
        { // Can cache this string
          status_pane_lengths[p] = len;
          status_pane_caches[p] = status_pane_cache_buf + total_chars;
          total_chars += len;
        }
      else
        { // Cannot cache this string
          status_pane_lengths[p] = 0;
          status_pane_caches[p] = NULL;
        }          
    }
  
  // Now update any status panes whose text has changed and also cache the
  // new text if we can, for next time.  Caching allows us to avoid updating
  // too many status panes, which can speed things up in video applications,
  // for example.
  for (p=0; p < 3; p++)
    {
      if (!pane_updated[p])
        continue;
      const char *string = three_strings[p];
      if (string == NULL)
        string = "";
      if (status_pane_caches[p] != NULL)
        strcpy(status_pane_caches[p],string);
      NSString *ns_string = [NSString stringWithUTF8String:string];
      NSTextFieldCell *cell = [status_panes[p] cell];
      [cell setTitle:ns_string];
    }
}

/*****************************************************************************/
/*                        kdms_window:set_drag_cursor                        */
/*****************************************************************************/

- (void)set_drag_cursor:(BOOL)is_dragging
{
  if (!scroll_view)
    return;
  if (is_dragging)
    [scroll_view setDocumentCursor:doc_cursor_dragging];
  else
    [scroll_view setDocumentCursor:doc_cursor_normal];
}

/*****************************************************************************/
/*                          kdms_window:menuFile....                         */
/*****************************************************************************/

- (IBAction) menuFileOpen:(NSMenuItem *)sender
{
  NSArray *file_types =
    [NSArray arrayWithObjects:@"jp2",@"jpx",@"jpf",@"mj2",@"j2c",@"j2k",nil];
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel setPrompt:@"Open"];
  [panel setMessage:@"Click \"Cancel\" to leave everything as-is"];
  [panel setTitle:@"Choose file(s) to open"];
  [panel setAllowsMultipleSelection:YES];
  if ([panel runModalForTypes:file_types] == NSOKButton)
    { // Open one or more files
      NSArray *filenames = [panel filenames];
      for (int n=0; n < filenames.count; n++)
        {
          NSString *filename = [[panel filenames] objectAtIndex:n];
          if (n == 0)
            [self open_file:filename];
          else
            {
              kdms_window *wnd = [kdms_window alloc];
              [wnd initWithManager:window_manager andCursors:manager_cursors];
              [wnd open_file:filename];
            }
        }
    }
}

- (IBAction) menuFileOpenUrl:(NSMenuItem *)sender
{
  kdms_url_dialog *dialog = [[kdms_url_dialog alloc] init];
  [dialog setTitle:@"Open JPIP URL in Current Window"];
  NSString *url = [dialog run_modal];
  [dialog release];
  if (url != nil)
    [self open_url:url];
}

- (IBAction) menuFileDisconnectURL:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileDisconnectURL();
  [window_manager->get_next_action_window(self) menuFileDisconnectURL:sender];
}

- (IBAction) menuFileSave:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileSave();
  [window_manager->get_next_action_window(self) menuFileSave:sender];
}

- (IBAction) menuFileSaveAs:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileSaveAs(true,false);
}

- (IBAction) menuFileSaveAsLinked:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileSaveAs(true,true);
}

- (IBAction) menuFileSaveAsEmbedded:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileSaveAs(false,false);
}

- (IBAction) menuFileSaveReload:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileSaveReload();
  [window_manager->get_next_action_window(self) menuFileSaveReload:sender];
}

- (IBAction) menuFileProperties:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FileProperties();
}

/*****************************************************************************/
/*                          kdms_window:menuView....                         */
/*****************************************************************************/

- (IBAction) menuViewHflip:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewHflip();
  [window_manager->get_next_action_window(self) menuViewHflip:sender];
}

- (IBAction) menuViewVflip:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewVflip();
  [window_manager->get_next_action_window(self) menuViewVflip:sender];
}

- (IBAction) menuViewRotate:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewRotate();
  [window_manager->get_next_action_window(self) menuViewRotate:sender];
}

- (IBAction) menuViewCounterRotate:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewCounterRotate();
  [window_manager->get_next_action_window(self) menuViewCounterRotate:sender];
}

- (IBAction) menuViewZoomIn:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewZoomIn();
  [window_manager->get_next_action_window(self) menuViewZoomIn:sender];
}

- (IBAction) menuViewZoomOut:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewZoomOut();
  [window_manager->get_next_action_window(self) menuViewZoomOut:sender];
}

- (IBAction) menuViewZoomInSlightly:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewZoomInSlightly();
  [window_manager->get_next_action_window(self) menuViewZoomInSlightly:sender];
}

- (IBAction) menuViewZoomOutSlightly:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewZoomOutSlightly();
  [window_manager->get_next_action_window(self) menuViewZoomOutSlightly:sender];
}

- (IBAction) menuViewWiden:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewWiden();
  [window_manager->get_next_action_window(self) menuViewWiden:sender];
}

- (IBAction) menuViewShrink:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewShrink();
  [window_manager->get_next_action_window(self) menuViewShrink:sender];
}

- (IBAction) menuViewRestore:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewRestore();
  [window_manager->get_next_action_window(self) menuViewRestore:sender];
}

- (IBAction) menuViewRefresh:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewRefresh();
  [window_manager->get_next_action_window(self) menuViewRefresh:sender];
}

- (IBAction) menuViewLayersLess:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewLayersLess();
  [window_manager->get_next_action_window(self) menuViewLayersLess:sender];
}

- (IBAction) menuViewLayersMore:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewLayersMore();
  [window_manager->get_next_action_window(self) menuViewLayersMore:sender];
}

- (IBAction) menuViewOptimizeScale:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewOptimizeScale();
  [window_manager->get_next_action_window(self) menuViewOptimizeScale:sender];
}

- (IBAction) menuViewPixelScaleX1:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewPixelScaleX1();
  [window_manager->get_next_action_window(self) menuViewPixelScaleX1:sender];
}

- (IBAction) menuViewPixelScaleX2:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ViewPixelScaleX2();
  [window_manager->get_next_action_window(self) menuViewPixelScaleX2:sender];
}

/*****************************************************************************/
/*                         kdms_window:menuFocus....                         */
/*****************************************************************************/

- (IBAction) menuFocusOff:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusOff();
  [window_manager->get_next_action_window(self) menuFocusOff:sender];
}

- (IBAction) menuFocusHighlighting:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusHighlighting();
  [window_manager->get_next_action_window(self) menuFocusHighlighting:sender];
}

- (IBAction) menuFocusWiden:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusWiden();
  [window_manager->get_next_action_window(self) menuFocusWiden:sender];
}

- (IBAction) menuFocusShrink:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusShrink();
  [window_manager->get_next_action_window(self) menuFocusShrink:sender];
}

- (IBAction) menuFocusLeft:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusLeft();
  [window_manager->get_next_action_window(self) menuFocusLeft:sender];
}

- (IBAction) menuFocusRight:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusRight();
  [window_manager->get_next_action_window(self) menuFocusRight:sender];
}

- (IBAction) menuFocusUp:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusUp();
  [window_manager->get_next_action_window(self) menuFocusUp:sender];
}

- (IBAction) menuFocusDown:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_FocusDown();
  [window_manager->get_next_action_window(self) menuFocusDown:sender];
}

/*****************************************************************************/
/*                          kdms_window:menuMode....                         */
/*****************************************************************************/

- (IBAction) menuModeFast:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeFast();
  [window_manager->get_next_action_window(self) menuModeFast:sender];
}

- (IBAction) menuModeFussy:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeFussy();
  [window_manager->get_next_action_window(self) menuModeFussy:sender];
}

- (IBAction) menuModeResilient:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeResilient();
  [window_manager->get_next_action_window(self) menuModeResilient:sender];
}

- (IBAction) menuModeResilientSOP:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeResilientSOP();
  [window_manager->get_next_action_window(self) menuModeResilientSOP:sender];
}

- (IBAction) menuModeSingleThreaded:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeSingleThreaded();
  [window_manager->get_next_action_window(self) menuModeSingleThreaded:sender];
}

- (IBAction) menuModeMultiThreaded:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeMultiThreaded();
  [window_manager->get_next_action_window(self) menuModeMultiThreaded:sender];
}

- (IBAction) menuModeMoreThreads:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeMoreThreads();
  [window_manager->get_next_action_window(self) menuModeMoreThreads:sender];
}

- (IBAction) menuModeLessThreads:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeLessThreads();
  [window_manager->get_next_action_window(self) menuModeLessThreads:sender];
}

- (IBAction) menuModeRecommendedThreads:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_ModeRecommendedThreads();
  [window_manager->get_next_action_window(self)
   menuModeRecommendedThreads:sender];
}

/*****************************************************************************/
/*                          kdms_window:menuNav....                          */
/*****************************************************************************/

- (IBAction) menuNavComponent1:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavComponent1();
  [window_manager->get_next_action_window(self) menuNavComponent1:sender];
}

- (IBAction) menuNavMultiComponent:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavMultiComponent();
  [window_manager->get_next_action_window(self) menuNavMultiComponent:sender];
}

- (IBAction) menuNavComponentNext:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavComponentNext();
  [window_manager->get_next_action_window(self) menuNavComponentNext:sender];
}

- (IBAction) menuNavComponentPrev:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavComponentPrev();
  [window_manager->get_next_action_window(self) menuNavComponentPrev:sender];
}

- (IBAction) menuNavImageNext:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavImageNext();
  [window_manager->get_next_action_window(self) menuNavImageNext:sender];
}

- (IBAction) menuNavImagePrev:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavImagePrev();
  [window_manager->get_next_action_window(self) menuNavImagePrev:sender];
}

- (IBAction) menuNavCompositingLayer:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavCompositingLayer();
  [window_manager->get_next_action_window(self)
   menuNavCompositingLayer:sender];
}

- (IBAction) menuNavTrackNext:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_NavTrackNext();
  [window_manager->get_next_action_window(self) menuNavTrackNext:sender];
}

/*****************************************************************************/
/*                        kdms_window:menuMetadata...                        */
/*****************************************************************************/

- (IBAction) menuMetadataCatalog:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataCatalog();
  [window_manager->get_next_action_window(self) menuMetadataCatalog:sender];
}

- (IBAction) menuMetadataSwapFocus:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataSwapFocus();
  [window_manager->get_next_action_window(self) menuMetadataSwapFocus:sender];
}

- (IBAction) menuMetadataShow:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataShow();
  [window_manager->get_next_action_window(self) menuMetadataShow:sender];
}

- (IBAction) menuMetadataAdd:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataAdd();
}

- (IBAction) menuMetadataEdit:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataEdit();
}

- (IBAction) menuMetadataDelete:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataDelete();
}

- (IBAction) menuMetadataFindOrphanedROI:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataFindOrphanedROI();
}

- (IBAction) menuMetadataCopyLabel:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataCopyLabel();  
}

- (IBAction) menuMetadataCopyLink:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataCopyLink();
}

- (IBAction) menuMetadataCut:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataCut();
}

- (IBAction) menuMetadataPasteNew:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataPasteNew();
}

- (IBAction) menuMetadataDuplicate:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataDuplicate();
}

- (IBAction) menuMetadataUndo:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataUndo();
}

- (IBAction) menuMetadataRedo:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_MetadataRedo();
}

- (IBAction) menuOverlayEnable:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayEnable();
  [window_manager->get_next_action_window(self) menuOverlayEnable:sender];
}

- (IBAction) menuOverlayFlashing:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayFlashing();
  [window_manager->get_next_action_window(self) menuOverlayFlashing:sender];
}

- (IBAction) menuOverlayToggle:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayToggle();
  [window_manager->get_next_action_window(self) menuOverlayToggle:sender];
}

- (IBAction) menuOverlayRestrict:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayRestrict();
}

- (IBAction) menuOverlayLighter:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayLighter();
  [window_manager->get_next_action_window(self) menuOverlayLighter:sender];
}

- (IBAction) menuOverlayHeavier:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayHeavier();
  [window_manager->get_next_action_window(self) menuOverlayHeavier:sender];
}

- (IBAction) menuOverlayDoubleSize:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayDoubleSize();
  [window_manager->get_next_action_window(self) menuOverlayDoubleSize:sender];
}

- (IBAction) menuOverlayHalveSize:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_OverlayHalveSize();
  [window_manager->get_next_action_window(self) menuOverlayHalveSize:sender];
}

/*****************************************************************************/
/*                          kdms_window:menuPlay....                         */
/*****************************************************************************/

- (IBAction) menuPlayStartForward:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayStartForward();
  [window_manager->get_next_action_window(self) menuPlayStartForward:sender];
}

- (IBAction) menuPlayStartBackward:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayStartBackward();
  [window_manager->get_next_action_window(self) menuPlayStartBackward:sender];
}

- (IBAction) menuPlayStop:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayStop();
  [window_manager->get_next_action_window(self) menuPlayStop:sender];
}

- (IBAction) menuPlayRepeat:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayRepeat();
  [window_manager->get_next_action_window(self) menuPlayRepeat:sender];
}

- (IBAction) menuPlayNative:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayNative();
  [window_manager->get_next_action_window(self) menuPlayNative:sender];
}

- (IBAction) menuPlayCustom:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayCustom();
  [window_manager->get_next_action_window(self) menuPlayCustom:sender];
}

- (IBAction) menuPlayFrameRateUp:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayFrameRateUp();
  [window_manager->get_next_action_window(self) menuPlayFrameRateUp:sender];
}

- (IBAction) menuPlayFrameRateDown:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_PlayFrameRateDown();
  [window_manager->get_next_action_window(self) menuPlayFrameRateDown:sender];
}

/*****************************************************************************/
/*                         kdms_window:menuStatus....                        */
/*****************************************************************************/

- (IBAction) menuStatusToggle:(NSMenuItem *)sender
{
  if (renderer != NULL) renderer->menu_StatusToggle();
  [window_manager->get_next_action_window(self) menuStatusToggle:sender];
}

/*****************************************************************************/
/*                         kdms_window:menuWindow....                        */
/*****************************************************************************/

- (IBAction) menuWindowDuplicate:(NSMenuItem *)sender
{
  if ((renderer != NULL) && renderer->can_do_WindowDuplicate(nil))
    {
      kdms_window *wnd = [kdms_window alloc];
      [wnd initWithManager:window_manager andCursors:manager_cursors];
      wnd->renderer->open_as_duplicate(self->renderer);
    }
}

- (IBAction) menuWindowNext:(NSMenuItem *)sender
{
  int idx = window_manager->get_access_idx(self);
  if (idx < 0) return;
  kdms_window *target_wnd = window_manager->access_window(idx+1);
  if (target_wnd == nil)
    target_wnd = window_manager->access_window(0);
  if (target_wnd == nil)
    return;
  [target_wnd makeKeyAndOrderFront:self];
}

- (bool) can_do_WindowNext
{
  int idx = window_manager->get_access_idx(self);
  return ((idx > 0) || ((idx == 0) && window_manager->access_window(idx+1)));
}

/*****************************************************************************/
/*                        kdms_window:validateMenuItem                       */
/*****************************************************************************/

- (BOOL) validateMenuItem:(NSMenuItem *)item
{
  bool item_recognized = true;
  bool validated = true;
  
  if (renderer == NULL)
    item_recognized = false;
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuFileDisconnectURL:))
    validated = renderer->can_do_FileDisconnectURL(item);
  else if ([item action] == @selector(menuFileSave:))
    validated = renderer->can_do_FileSave(item);
  else if ([item action] == @selector(menuFileSaveAs:))
    validated = renderer->can_do_FileSaveAs(item);
  else if ([item action] == @selector(menuFileSaveAsLinked:))
    validated = renderer->can_do_FileSaveAs(item);
  else if ([item action] == @selector(menuFileSaveAsEmbedded:))
    validated = renderer->can_do_FileSaveAs(item);
  else if ([item action] == @selector(menuFileSaveReload:))
    validated = renderer->can_do_FileSaveReload(item);
  else if ([item action] == @selector(menuFileProperties:))
    validated = renderer->can_do_FileProperties(item);
  //-----------------------------------------------------------    
  else if ([item action] == @selector(menuViewHflip:))
    validated = renderer->can_do_ViewHflip(item);
  else if ([item action] == @selector(menuViewVflip:))
    validated = renderer->can_do_ViewVflip(item);
  else if ([item action] == @selector(menuViewRotate:))
    validated = renderer->can_do_ViewRotate(item);
  else if ([item action] == @selector(menuViewCounterRotate:))
    validated = renderer->can_do_ViewCounterRotate(item);
  else if ([item action] == @selector(menuViewZoomIn:))
    validated = renderer->can_do_ViewZoomIn(item);
  else if ([item action] == @selector(menuViewZoomOut:))
    validated = renderer->can_do_ViewZoomOut(item);
  else if ([item action] == @selector(menuViewWiden:))
    validated = renderer->can_do_ViewWiden(item);
  else if ([item action] == @selector(menuViewShrink:))
    validated = renderer->can_do_ViewShrink(item);
  else if ([item action] == @selector(menuViewRestore:))
    validated = renderer->can_do_ViewRestore(item);
  else if ([item action] == @selector(menuViewOptimizeScale:))
    validated = renderer->can_do_ViewOptimizeScale(item);
  else if ([item action] == @selector(menuViewPixelScaleX1:))
    validated = renderer->can_do_ViewPixelScaleX1(item);
  else if ([item action] == @selector(menuViewPixelScaleX2:))
    validated = renderer->can_do_ViewPixelScaleX2(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuFocusOff:))
    validated = renderer->can_do_FocusOff(item);
  else if ([item action] == @selector(menuFocusHighlighting:))
    validated = renderer->can_do_FocusHighlighting(item);
  else if ([item action] == @selector(menuFocusWiden:))
    validated = renderer->can_do_FocusWiden(item);
  else if ([item action] == @selector(menuFocusShrink:))
    validated = renderer->can_do_FocusShrink(item);
  else if ([item action] == @selector(menuFocusLeft:))
    validated = renderer->can_do_FocusLeft(item);  
  else if ([item action] == @selector(menuFocusRight:))
    validated = renderer->can_do_FocusRight(item);
  else if ([item action] == @selector(menuFocusUp:))
    validated = renderer->can_do_FocusUp(item);  
  else if ([item action] == @selector(menuFocusDown:))
    validated = renderer->can_do_FocusDown(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuModeFast:))
    validated = renderer->can_do_ModeFast(item);
  else if ([item action] == @selector(menuModeFussy:))
    validated = renderer->can_do_ModeFussy(item);
  else if ([item action] == @selector(menuModeResilient:))
    validated = renderer->can_do_ModeResilient(item);
  else if ([item action] == @selector(menuModeResilientSOP:))
    validated = renderer->can_do_ModeResilientSOP(item);
  else if ([item action] == @selector(menuModeSingleThreaded:))
    validated = renderer->can_do_ModeSingleThreaded(item);
  else if ([item action] == @selector(menuModeMultiThreaded:))
    validated = renderer->can_do_ModeMultiThreaded(item);
  else if ([item action] == @selector(menuModeMoreThreads:))
    validated = renderer->can_do_ModeMoreThreads(item);
  else if ([item action] == @selector(menuModeLessThreads:))
    validated = renderer->can_do_ModeLessThreads(item);
  else if ([item action] == @selector(menuModeRecommendedThreads:))
    validated = renderer->can_do_ModeRecommendedThreads(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuNavComponent1:))
    validated = renderer->can_do_NavComponent1(item);
  else if ([item action] == @selector(menuNavMultiComponent:))
    validated = renderer->can_do_NavMultiComponent(item);
  else if ([item action] == @selector(menuNavComponentNext:))
    validated = renderer->can_do_NavComponentNext(item);
  else if ([item action] == @selector(menuNavComponentPrev:))
    validated = renderer->can_do_NavComponentPrev(item);
  else if ([item action] == @selector(menuNavImageNext:))
    validated = renderer->can_do_NavImageNext(item);
  else if ([item action] == @selector(menuNavImagePrev:))
    validated = renderer->can_do_NavImagePrev(item);
  else if ([item action] == @selector(menuNavCompositingLayer:))
    validated = renderer->can_do_NavCompositingLayer(item);
  else if ([item action] == @selector(menuNavTrackNext:))
    validated = renderer->can_do_NavTrackNext(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuMetadataCatalog:))
    validated = renderer->can_do_MetadataCatalog(item);
  else if ([item action] == @selector(menuMetadataSwapFocus:))
    validated = renderer->can_do_MetadataSwapFocus(item);
  else if ([item action] == @selector(menuMetadataShow:))
    validated = renderer->can_do_MetadataShow(item);
  else if ([item action] == @selector(menuMetadataAdd:))
    validated = renderer->can_do_MetadataAdd(item);
  else if ([item action] == @selector(menuMetadataEdit:))
    validated = renderer->can_do_MetadataEdit(item);
  else if ([item action] == @selector(menuMetadataDelete:))
    validated = renderer->can_do_MetadataDelete(item);
  else if ([item action] == @selector(menuMetadataFindOrphanedROI:))
    validated = renderer->can_do_MetadataFindOrphanedROI(item);
  else if ([item action] == @selector(menuMetadataCopyLabel:))
    validated = renderer->can_do_MetadataCopyLabel(item);
  else if ([item action] == @selector(menuMetadataCopyLink:))
    validated = renderer->can_do_MetadataCopyLink(item);
  else if ([item action] == @selector(menuMetadataCut:))
    validated = renderer->can_do_MetadataCut(item);
  else if ([item action] == @selector(menuMetadataPasteNew:))
    validated = renderer->can_do_MetadataPasteNew(item);
  else if ([item action] == @selector(menuMetadataDuplicate:))
    validated = renderer->can_do_MetadataDuplicate(item);
  else if ([item action] == @selector(menuMetadataUndo:))
    validated = renderer->can_do_MetadataUndo(item);
  else if ([item action] == @selector(menuMetadataRedo:))
    validated = renderer->can_do_MetadataRedo(item);  
  else if ([item action] == @selector(menuOverlayEnable:))
    validated = renderer->can_do_OverlayEnable(item);
  else if ([item action] == @selector(menuOverlayFlashing:))
    validated = renderer->can_do_OverlayFlashing(item);
  else if ([item action] == @selector(menuOverlayToggle:))
    validated = renderer->can_do_OverlayToggle(item);
  else if ([item action] == @selector(menuOverlayRestrict:))
    validated = renderer->can_do_OverlayRestrict(item);
  else if ([item action] == @selector(menuOverlayLighter:))
    validated = renderer->can_do_OverlayLighter(item);
  else if ([item action] == @selector(menuOverlayHeavier:))
    validated = renderer->can_do_OverlayHeavier(item);
  else if ([item action] == @selector(menuOverlayDoubleSize:))
    validated = renderer->can_do_OverlayDoubleSize(item);
  else if ([item action] == @selector(menuOverlayHalveSize:))
    validated = renderer->can_do_OverlayHalveSize(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuPlayStartForward:))
    validated = renderer->can_do_PlayStartForward(item);
  else if ([item action] == @selector(menuPlayStartBackward:))
    validated = renderer->can_do_PlayStartBackward(item);
  else if ([item action] == @selector(menuPlayStop:))
    validated = renderer->can_do_PlayStop(item);
  else if ([item action] == @selector(menuPlayRepeat:))
    validated = renderer->can_do_PlayRepeat(item);
  else if ([item action] == @selector(menuPlayNative:))
    validated = renderer->can_do_PlayNative(item);
  else if ([item action] == @selector(menuPlayCustom:))
    validated = renderer->can_do_PlayCustom(item);
  else if ([item action] == @selector(menuPlayFrameRateUp:))
    validated = renderer->can_do_PlayFrameRateUp(item);
  else if ([item action] == @selector(menuPlayFrameRateDown:))
    validated = renderer->can_do_PlayFrameRateDown(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuStatusToggle:))
    validated = renderer->can_do_StatusToggle(item);
  //-----------------------------------------------------------
  else if ([item action] == @selector(menuWindowDuplicate:))
    validated = renderer->can_do_WindowDuplicate(item);
  else if ([item action] == @selector(menuWindowNext:))
    validated = [self can_do_WindowNext]; 
  //-----------------------------------------------------------
  else
    item_recognized = false;
  
  if (item_recognized && validated)
    renderer->cancel_pending_show();
  return (validated)?YES:NO;
}

/*****************************************************************************/
/*                       kdms_window:becomeKeyWindow                         */
/*****************************************************************************/

- (void) becomeKeyWindow
{
  if (renderer != NULL)
    renderer->set_key_window_status(true);
  [super becomeKeyWindow];
}

/*****************************************************************************/
/*                       kdms_window:resignKeyWindow                         */
/*****************************************************************************/

- (void) resignKeyWindow
{
  if (renderer != NULL)
    renderer->set_key_window_status(false);
  [super resignKeyWindow];
}

/*****************************************************************************/
/*                 kdms_window:scroll_view_BoundsDidChange                   */
/*****************************************************************************/

- (void) scroll_view_BoundsDidChange:(NSNotification *)notification
{
  if (renderer != NULL)
    renderer->view_dims_changed();
}
  

@end // kdms_window
