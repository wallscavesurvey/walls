/*****************************************************************************/
// File: vex_display.cpp [scope = APPS/VEX_FAST]
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
   DirectX-based display engine for use with the "kdu_vex_fast" demo app
on Windows platforms.
******************************************************************************/

#ifdef KDU_DX9
#include <time.h>
#include "vex_display.h"

/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                      display_startproc                             */
/*****************************************************************************/

static kdu_thread_startproc_result
  KDU_THREAD_STARTPROC_CALL_CONVENTION display_startproc(void *param)
{
  kdu_thread thread;  thread.set_to_self();
  int min_priority, max_priority;
  thread.get_priority(min_priority,max_priority);
  thread.set_priority(max_priority);
  vex_display *self = (vex_display *) param;
  self->run_display_loop();
  return KDU_THREAD_STARTPROC_ZERO_RESULT;
}

/*****************************************************************************/
/* STATIC                  choose_presentation_size                          */
/*****************************************************************************/

static void
  choose_presentation_size(D3DPRESENT_PARAMETERS *d3dpp, kdu_coords frame_size,
                           IDirect3D9 *direct3d)
  /* This function is used to determine the best presentation size when
     entering full-screen mode.  It scans through the screen sizes supported
     by the graphics card to determine a best match for the `frame_size' and
     records the resulting dimensions in `d3dpp->BackBufferWidth',
     `d3dpp->BackBufferHeight' and `d3dpp->FullScreen_RefreshRateInHz'.
     The `d3dpp.BackBufferFormat' member identifies the display format
     we need, which will generally be X8R8G8B8.  The function tries to select
     display modes which preserve the current pixel aspect ratio, as
     determined by an initial call to `direct3d->GetAdapterDisplayMode'. */
{
  HRESULT success;
  D3DDISPLAYMODE current_mode, best_mode, mode;
  UINT m, num_modes =
    direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT,d3dpp->BackBufferFormat);
  success = direct3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&current_mode);
  best_mode = current_mode; // Just in case
  bool fit_aspect_ratio = (success == D3D_OK);
  bool best_width_fits = false;
  bool best_height_fits = false;
  for (m=0; m < num_modes; m++)
    {
      success =
        direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT,d3dpp->BackBufferFormat,
                                   m,&mode);
      if (success != D3D_OK)
        break;
      bool height_fits = (mode.Height >= (UINT) frame_size.y);
      bool width_fits = (mode.Width >= (UINT) frame_size.x);
      if ((m == 0) ||
          ((!best_width_fits) && (mode.Width > best_mode.Width)) ||
          ((!best_height_fits) && (mode.Height > best_mode.Height) &&
           (width_fits || (mode.Width >= best_mode.Width))) ||
          (width_fits && height_fits &&
           ((mode.Height*mode.Width) <
            (best_mode.Height*best_mode.Width))) ||
          ((mode.Width == best_mode.Width) &&
           (mode.Height == best_mode.Height) &&
           (mode.RefreshRate > best_mode.RefreshRate)))
        {
          if (fit_aspect_ratio)
            {
              int diff =
                mode.Width*current_mode.Height-mode.Height*current_mode.Width;
              int ref =
                mode.Width*mode.Height+current_mode.Width*current_mode.Height;
              if ((diff > (ref>>5)) || (diff < -(ref>>5)))
                continue;
            }
          best_mode = mode;
          best_width_fits = width_fits;
          best_height_fits = height_fits;
        }
    }
  d3dpp->BackBufferWidth = best_mode.Width;
  d3dpp->BackBufferHeight = best_mode.Height;
  d3dpp->FullScreen_RefreshRateInHz = best_mode.RefreshRate;
}

/* ========================================================================= */
/*                                vex_display                                */
/* ========================================================================= */

/*****************************************************************************/
/*                             vex_display::init                             */
/*****************************************************************************/

const char *
  vex_display::init(kdu_coords frame_size, bool full_screen, float fps,
                    int total_frame_buffers)
{
  free_surfaces = NULL;
  ready_head = ready_tail = NULL;
  if (direct3d != NULL)
    shutdown();
  if (fps <= 0.0F)
    return "Invalid frame rate";
  target_frame_period = 1.0F / fps;
  direct3d = Direct3DCreate9(D3D_SDK_VERSION);
  if (direct3d == NULL)
    return "Could not initialize Direct3D.";
  if (hinst == NULL)
    hinst = GetModuleHandle(NULL);
  WNDCLASSEX wcx; 
  wcx.cbSize = sizeof(wcx);
  wcx.style = 0;
  wcx.lpfnWndProc = DefWindowProc;
  wcx.cbClsExtra = wcx.cbWndExtra = 0;
  wcx.hInstance = hinst;
  wcx.hIcon = NULL;  wcx.hCursor = NULL;
  wcx.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wcx.lpszMenuName =  NULL;
  wcx.lpszClassName = "kdu_vex_fast_wndclass";
  wcx.hIconSm = NULL;
  window_class = RegisterClassEx(&wcx);

  window = CreateWindow("kdu_vex_fast_wndclass","kdu_vex_fast",
                        WS_THICKFRAME|WS_VISIBLE,0,0,
                        640,640,NULL,NULL,NULL,this);
  if (window == NULL)
    { shutdown();
      return "Could not create a window."; }
  RECT frame_rect, client_rect;
  GetWindowRect(window,&frame_rect);
  GetClientRect(window,&client_rect);
  kdu_coords border_size =
    kdu_coords(frame_rect.right-client_rect.right+client_rect.left,
               frame_rect.bottom-client_rect.bottom+client_rect.top);
  frame_rect.right = frame_size.x + border_size.x;
  frame_rect.bottom = frame_size.y + border_size.y;
  WINDOWPLACEMENT placement;
  memset(&placement,0,sizeof(placement));
  placement.length = sizeof(placement);
  placement.flags = WPF_SETMINPOSITION;
  placement.rcNormalPosition = frame_rect;
  placement.showCmd = SW_SHOWNORMAL;
  SetWindowPlacement(window,&placement);
  
  HRESULT success;
  D3DPRESENT_PARAMETERS d3dpp;
  memset(&d3dpp,0,sizeof(d3dpp));
  d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
  if (full_screen)
    {
      choose_presentation_size(&d3dpp,frame_size,direct3d);
      d3dpp.Windowed = FALSE;
    }
  else
    d3dpp.Windowed = TRUE;
  d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
  d3dpp.BackBufferCount = 1;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.hDeviceWindow = window;
  d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
  d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
  success = direct3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,
 	       	            		         D3DCREATE_MIXED_VERTEXPROCESSING,
                                   &d3dpp,&device);
  if (success != D3D_OK)
    { device=NULL; shutdown();
      if (full_screen)
        return "Could not create DirectX display device.";
      else
        return "Could not create DirectX display device.  Try full-screen "
               "mode instead.";
    }

  frame_dims.pos = kdu_coords(0,0);
  frame_dims.size = frame_size;
  display_dims.pos = kdu_coords(0,0);
  display_dims.size.x = (int) d3dpp.BackBufferWidth;
  display_dims.size.y = (int) d3dpp.BackBufferHeight;
  set_backbuffer_rects();

  // Allocate `total_frame_buffers' surface buffers and put them on the
  // free list.
  for (; total_frame_buffers > 0; total_frame_buffers--)
    {
      vex_display_surface *surf = new vex_display_surface;
      surf->locked_rect.pBits = NULL; // Surfaces on the fee list not locked
      surf->surface = NULL;
      surf->next = free_surfaces;
      free_surfaces = surf;
      surf->alloc_size = frame_size;
      surf->alloc_size.x += (-surf->alloc_size.x) & 3; // Multiple of 4 pixels
      surf->alloc_size.x += 4; // Allow for buffer alignment adjustments
      success =
        device->CreateOffscreenPlainSurface((UINT)(surf->alloc_size.x),
                                            (UINT)(surf->alloc_size.y),
                                            d3dpp.BackBufferFormat,
                                            D3DPOOL_SYSTEMMEM,
                                            &(surf->surface),NULL);
      if (success != D3D_OK)
        { shutdown();
          if (free_surfaces->next == NULL)
            return "Could not create any DirectX surface buffers.";
          else
            return "Could not create sufficient DirectX surface buffers.";
        }
    }

  // Start up the display thread.
  pending_pan_up = pending_pan_down =
    pending_pan_left = pending_pan_right = 0;
  display_shutdown_requested = false;
  if (!(mutex.create() &&
        surface_ready_event.create(false) &&
        surface_free_event.create(false) &&
        display_thread.create(display_startproc,this)))
    { shutdown();
      return "Unable to start the display management thread."; }
  return NULL;
}

/*****************************************************************************/
/*                           vex_display::shutdown                           */
/*****************************************************************************/

void
  vex_display::shutdown()
{
  if (display_thread.exists())
    {
      display_shutdown_requested = true;
      surface_ready_event.set(); // Force `run_display_loop' to wake up
      display_thread.destroy();
    }
  if (surface_ready_event.exists())
    surface_ready_event.destroy();
  if (surface_free_event.exists())
    surface_free_event.destroy();
  if (mutex.exists())
    mutex.destroy();
  if (free_surfaces != NULL)
    {
      if (ready_tail == NULL)
        ready_head = free_surfaces;
      else
        ready_tail->next = free_surfaces;
      ready_tail = free_surfaces = NULL;
    }
  while ((ready_tail = ready_head) != NULL)
    {
      ready_head = ready_tail->next;
      if (ready_tail->surface != NULL)
        {
          if (ready_tail->locked_rect.pBits != NULL)
            ready_tail->surface->UnlockRect();
          ready_tail->surface->Release();
        }
      delete ready_tail;
    }
  if (device != NULL)
    { device->Release(); device = NULL; }
  if (window != NULL)
    { DestroyWindow(window); window = NULL; }
  if (window_class != NULL)
    { UnregisterClass("kdu_vex_fast_wndclass",hinst); window_class = NULL; }
  if (direct3d == NULL)
    return;
  direct3d->Release();
  direct3d = NULL;
}

/*****************************************************************************/
/*                         vex_display::push_frame                           */
/*****************************************************************************/

bool
  vex_display::push_frame(vex_frame *frame)
{
  if (display_shutdown_requested || !flush_message_queue())
    return false;
  vex_display_surface *surf = (vex_display_surface *)(frame->buffer_handle);
  assert(surf->locked_rect.pBits != NULL);
  surf->surface->UnlockRect();
  surf->locked_rect.pBits = NULL;
  frame->buffer_handle = NULL; // Make sure we don't forget to re-assign the
  frame->buffer = NULL; // frame's buffer from the free surface list later on.
  mutex.lock();
  surf->next = NULL;
  if (ready_tail == NULL)
    {
      surface_ready_event.set();
      ready_head = ready_tail = surf;
    }
  else
    ready_tail = ready_tail->next = surf;
  mutex.unlock();
  frame->buffer_handle =
    allocate_mem(0,NULL,4,VEX_FRAME_FORMAT_XRGB,frame->buffer,
                 frame->frame_bytes,frame->stride,frame->fully_aligned);
      // This call may block on the availability of free surfaces.
  return (!display_shutdown_requested) && flush_message_queue();
}

/*****************************************************************************/
/*                     vex_display::run_display_loop                         */
/*****************************************************************************/

void
  vex_display::run_display_loop()
{
  clock_t start_ticks = clock();
  float next_frame_time = target_frame_period;
  clock_t next_frame_ticks = start_ticks +
    ((clock_t)(1000.0F * next_frame_time));
  do {
      mutex.lock();
      while ((ready_head == NULL) && (!display_shutdown_requested))
        surface_ready_event.wait(mutex);
      if ((pending_pan_up | pending_pan_down |
           pending_pan_left | pending_pan_right) != 0)
        {
          display_dims.pos.y -= (display_dims.size.y >> 4)*pending_pan_up;
          display_dims.pos.y += (display_dims.size.y >> 4)*pending_pan_down;
          display_dims.pos.x -= (display_dims.size.x >> 4)*pending_pan_left;
          display_dims.pos.x += (display_dims.size.x >> 4)*pending_pan_right;
          pending_pan_up = pending_pan_down =
            pending_pan_left = pending_pan_right = 0;
          set_backbuffer_rects();
        }
      mutex.unlock();
      if (!display_shutdown_requested)
        {
          vex_display_surface *surf = ready_head;
          HRESULT success;
          IDirect3DSurface9 *backbuffer;
          success = device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,
                                          &backbuffer);
          if (success != D3D_OK)
            { display_shutdown_requested = true;
              kdu_warning w; w << "Unable to execute DirectX `GetBackBuffer'.";
              break; }
          POINT dest_point;
          dest_point.x = paint_rect.left;  dest_point.y = paint_rect.top;
          RECT src_rect;
          src_rect.left = surf->frame_region.pos.x + display_dims.pos.x;
          src_rect.top = surf->frame_region.pos.y + display_dims.pos.y;
          src_rect.right = src_rect.left + paint_rect.right-paint_rect.left;
          src_rect.bottom = src_rect.top + paint_rect.bottom-paint_rect.top;
          success = device->UpdateSurface(surf->surface,&src_rect,
                                          backbuffer,&dest_point);
          for (int n=0; n < num_dirty_rects; n++)
            device->ColorFill(backbuffer,dirty_rects+n,
                              D3DCOLOR_ARGB(0,0,0,0));
          backbuffer->Release();
          if (success != D3D_OK)
            { display_shutdown_requested = true;
              kdu_warning w; w << "Unable to execute DirectX `UpdateSurface'.";
              break; }

          int wait_ticks = (int)(next_frame_ticks - clock());
          if (wait_ticks > 0)
            Sleep(wait_ticks);
          next_frame_time += target_frame_period;
          next_frame_ticks = start_ticks +
            ((clock_t)(1000.0F*next_frame_time));

          success = device->Present(NULL,NULL,NULL,NULL);
          if (success != D3D_OK)
            { display_shutdown_requested = true;
              kdu_warning w; w << "Unable to execute DirectX `Present'.";
              break; }
          mutex.lock();
          if ((ready_head = surf->next) == NULL)
            ready_tail = NULL;
          if ((surf->next = free_surfaces) == NULL)
            surface_free_event.set();
          free_surfaces = surf;
          mutex.unlock();
        }
    } while (!display_shutdown_requested);
  surface_free_event.set(); // In case we need to wake up the management thread
}

/*****************************************************************************/
/*                    vex_display::set_backbuffer_rects                      */
/*****************************************************************************/

void
  vex_display::set_backbuffer_rects()
{
  if ((display_dims.pos.x+display_dims.size.x) > frame_dims.size.x)
    display_dims.pos.x = frame_dims.size.x - display_dims.size.x;
  if ((display_dims.pos.y+display_dims.size.y) > frame_dims.size.y)
    display_dims.pos.y = frame_dims.size.y - display_dims.size.y;
  if (display_dims.pos.x < 0)
    display_dims.pos.x = 0;
  if (display_dims.pos.y < 0)
    display_dims.pos.y = 0;
  paint_rect.left = paint_rect.top = 0;
  paint_rect.right = frame_dims.size.x - display_dims.pos.x;
  paint_rect.bottom = frame_dims.size.y - display_dims.pos.y;
  num_dirty_rects = 0;
  if (paint_rect.right >= display_dims.size.x)
    paint_rect.right = display_dims.size.x;
  else
    {
      RECT *dirty = dirty_rects + (num_dirty_rects++);
      dirty->left = paint_rect.right;
      dirty->right = display_dims.size.x;
      dirty->top = 0;
      dirty->bottom = display_dims.size.y;
    }
  if (paint_rect.bottom > display_dims.size.y)
    paint_rect.bottom = display_dims.size.y;
  else
    {
      RECT *dirty = dirty_rects + (num_dirty_rects++);
      dirty->left = 0;
      dirty->right = display_dims.size.x;
      dirty->top = paint_rect.bottom;
      dirty->bottom = display_dims.size.y;
    }
}

/*****************************************************************************/
/*                          flush_message_queue                              */
/*****************************************************************************/

bool
  vex_display::flush_message_queue()
  /* Returns false if a user quit request is detected. */
{
  MSG msg;
  while (PeekMessage(&msg,window,0,0xFFFFFFFF,PM_REMOVE))
    {
      if (msg.message == WM_KEYDOWN)
        {
          if ((msg.wParam == VK_DOWN) || (msg.wParam == VK_UP) ||
              (msg.wParam == VK_LEFT) || (msg.wParam == VK_RIGHT))
            {
              mutex.lock();
              pending_pan_down += (msg.wParam == VK_DOWN)?1:0;
              pending_pan_up += (msg.wParam == VK_UP)?1:0;
              pending_pan_left += (msg.wParam == VK_LEFT)?1:0;
              pending_pan_right += (msg.wParam == VK_RIGHT)?1:0;
              mutex.unlock();
            }
          else
            return false; // Need to terminate
        }
      DefWindowProc(msg.hwnd,msg.message,msg.wParam,msg.lParam);
    }
  return true;
}

/*****************************************************************************/
/*                       vex_display::allocate_mem                           */
/*****************************************************************************/

void *
  vex_display::allocate_mem(int num_components, kdu_dims *component_dims,
                            size_t sample_bytes, int format,
                            kdu_byte * &buffer, size_t &frame_bytes,
                            size_t &stride, bool &fully_aligned)
{
  if ((component_dims != NULL) &&
      ((component_dims->size != frame_dims.size) || (sample_bytes != 4) ||
       (format != VEX_FRAME_FORMAT_XRGB)))
    { kdu_error e; e << "The `vex_display::allocate_mem' function expects "
      "has been invoked with inconsistent frame dimensions or a frame "
      "buffer format other than XRGB."; }
  if (device == NULL)
    { kdu_error e; e << "Attempting to invoke `vex_display::allocate_mem' "
      "after the `vex_display' object has been shutdown!"; }
  mutex.lock();
  vex_display_surface *surf;
  while (((surf = free_surfaces) == NULL) && !display_shutdown_requested)
    surface_free_event.wait(mutex);
  free_surfaces = surf->next;
  surf->next = NULL;
  mutex.unlock();
  HRESULT success =
    surf->surface->LockRect(&(surf->locked_rect),NULL,D3DLOCK_NOSYSLOCK);
  if (success != D3D_OK)
    {
      mutex.lock();
      surf->next = free_surfaces;  free_surfaces = surf;
      mutex.unlock();
      display_shutdown_requested = true;
      kdu_error e; e << "Unable to lock DirectX surface into memory.";
    }
  surf->frame_region.pos.x =
    ((-(int) _addr_to_kdu_long(surf->locked_rect.pBits)) & 15) >> 2;
  surf->frame_region.pos.y = 0;
  surf->frame_region.size = this->frame_dims.size;
  buffer =
    ((kdu_byte *)(surf->locked_rect.pBits)) + (surf->frame_region.pos.x<<2);
  stride = (size_t)(surf->locked_rect.Pitch);
  frame_bytes = (stride * (size_t) surf->alloc_size.y)
              - surf->frame_region.pos.x;
  fully_aligned = !(stride & 15);
  return surf;
}
                            
/*****************************************************************************/
/*                        vex_display::destroy_mem                           */
/*****************************************************************************/

void
  vex_display::destroy_mem(void *handle)
{
  if (handle == NULL)
    return;
  vex_display_surface *surf = (vex_display_surface *) handle;
  if (device != NULL)
    {
      if (surf->locked_rect.pBits != NULL)
        { surf->surface->UnlockRect();  surf->locked_rect.pBits = NULL; }
      mutex.lock();
      surf->next = free_surfaces;
      free_surfaces = surf;
      mutex.unlock();
    }
  else
    delete surf;
}

#endif // KDU_DX9
