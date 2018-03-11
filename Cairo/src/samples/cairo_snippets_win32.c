/*
 * This example code is placed in the public domain
 *
 * Author: Owen Taylor <otaylor@redhat.com>, Red Hat Inc.
 */

#include "snippets.h"
#include <cairo-win32.h>

#define TITLE TEXT("Cairo Snippets")
#define BORDER_WIDTH 75.
#define SNIPPET_WIDTH  300.
#define SNIPPET_HEIGHT 300.
#define FONT_SIZE 24

static int current_snippet = 0;

static void 
on_paint (HDC hdc)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    double line_width;
    cairo_font_extents_t font_extents;

    surface = cairo_win32_surface_create (hdc);
    cr = cairo_create (surface);

    line_width = cairo_get_line_width (cr);

    /* Draw a box bordering the snippet */
    cairo_rectangle (cr,
		     BORDER_WIDTH - line_width / 2, BORDER_WIDTH - line_width / 2,
		     SNIPPET_WIDTH + line_width, SNIPPET_WIDTH + line_width);
    cairo_stroke (cr);

    /* And the name of the snippet */
    cairo_set_font_size (cr, FONT_SIZE);
    cairo_font_extents (cr, &font_extents);

    cairo_move_to (cr,
		   BORDER_WIDTH,
		   BORDER_WIDTH + SNIPPET_WIDTH + font_extents.ascent);
    cairo_show_text (cr, snippet_name[current_snippet]);

    /* Now draw the snippet, clipped to the box */
    cairo_save (cr);
    cairo_translate (cr, BORDER_WIDTH, BORDER_WIDTH);
    cairo_rectangle (cr,
		     0, 0,
		     SNIPPET_WIDTH, SNIPPET_WIDTH);
    cairo_clip (cr);
    snippet_do (cr, current_snippet, SNIPPET_WIDTH, SNIPPET_HEIGHT);
    cairo_restore (cr);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
}

static void 
next_snippet (HWND window)
{
    if (current_snippet == snippet_count - 1)
	current_snippet = 0;
    else
	current_snippet++;
    
    InvalidateRect (window, NULL, TRUE);
}

static void 
previous_snippet (HWND window)
{
    if (current_snippet == 0)
	current_snippet = snippet_count - 1;
    else
	current_snippet--;

    InvalidateRect (window, NULL, TRUE);
}

/* The WinMain and window procedure are loosely based on a example
 * from the Microsoft documentation.
 */
LRESULT CALLBACK 
WndProc (HWND   window,
	 UINT   message,
	 WPARAM wParam,
	 LPARAM lParam)
{
    PAINTSTRUCT paint_struct;
    HDC dc;
   
    switch(message) {
    case WM_CHAR:
	switch (wParam) {
	case 'q':
	case 'Q':
	    PostQuitMessage (0);
	    return 0;
	    break;
	case 'n':
	case 'N':
	    next_snippet (window);
	    break;
	case 'p':
	case 'P':
	    previous_snippet (window);
	}
	break;
    case WM_PAINT:
	dc = BeginPaint (window, &paint_struct);
	on_paint (dc);
	EndPaint (window, &paint_struct);
	return 0;
    case WM_LBUTTONDOWN:
	next_snippet (window);
	return 0;
    case WM_RBUTTONDOWN:
	previous_snippet (window);
	return 0;
    case WM_DESTROY:
	PostQuitMessage (0);
	return 0;
    default:
	;
    }
       
    return DefWindowProc (window, message, wParam, lParam);
}

#define WINDOW_STYLE WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME)

INT WINAPI 
WinMain (HINSTANCE hInstance,
	 HINSTANCE hPrevInstance,
	 PSTR      lpCmdLine,
	 INT       iCmdShow)
{
    HWND window;
    MSG message;
    WNDCLASS window_class;
    RECT rect;

    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WndProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hInstance;
    window_class.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor (NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = TITLE;
   
    RegisterClass (&window_class);
   
    /* Compute the window size to give us the desired client area */
    rect.left = 0;
    rect.top = 0;
    rect.right = SNIPPET_WIDTH + 2 * BORDER_WIDTH;
    rect.bottom = SNIPPET_WIDTH + 2 * BORDER_WIDTH;
   
    AdjustWindowRect (&rect, WINDOW_STYLE, FALSE /* no menu */);

    window = CreateWindow (TITLE, /* Class name */
			   TITLE, /* Window name */
			   WINDOW_STYLE,
			   CW_USEDEFAULT, CW_USEDEFAULT, /* initial position */
			   rect.right - rect.left, rect.bottom - rect.top, /* initial size */
			   NULL,	/* Parent */
			   NULL,	/* Menu */
			   hInstance,
			   NULL); /* WM_CREATE lpParam */
   
    ShowWindow (window, iCmdShow);
    UpdateWindow (window);
   
    while (GetMessage (&message, NULL, 0, 0)) {
	TranslateMessage (&message);
	DispatchMessage (&message);
    }
   
    return message.wParam;
}
