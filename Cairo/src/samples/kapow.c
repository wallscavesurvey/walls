#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <cairo.h>
#ifndef CAIRO_HAS_PNG_FUNCTIONS
#error This program requires cairo with PNG support
#endif
#include <math.h>

const char filename[] = "kapow.png";
const char fontname[] = "Sans";
const char default_text[] = "KAPOW";

#define IMAGE_WIDTH 384
#define IMAGE_HEIGHT 256

#define SPIKES 10
#define SHADOW_OFFSET 10

#define X_FUZZ 16
#define Y_FUZZ 16

#define X_OUTER_RADIUS (IMAGE_WIDTH / 2 - X_FUZZ - SHADOW_OFFSET)
#define Y_OUTER_RADIUS (IMAGE_HEIGHT / 2 - Y_FUZZ - SHADOW_OFFSET)

#define X_INNER_RADIUS (X_OUTER_RADIUS * 0.7)
#define Y_INNER_RADIUS (Y_OUTER_RADIUS * 0.7)

static void
make_star_path (cairo_t *cr)
{
    double x;
    double y;
    int i;

    srand (45);

    for (i = 0; i < SPIKES * 2; i++) {

	x = IMAGE_WIDTH / 2 + cos (M_PI * i / SPIKES) * X_INNER_RADIUS +
	    (double) rand() * X_FUZZ / RAND_MAX;
	y = IMAGE_HEIGHT / 2 + sin (M_PI * i / SPIKES) * Y_INNER_RADIUS +
	    (double) rand() * Y_FUZZ / RAND_MAX;

	if (i == 0)
	    cairo_move_to (cr, x, y);
	else
	    cairo_line_to (cr, x, y);

	i++;

	x = IMAGE_WIDTH / 2 + cos (M_PI * i / SPIKES) * X_OUTER_RADIUS +
	    (double) rand() * X_FUZZ / RAND_MAX;
	y = IMAGE_HEIGHT / 2 + sin (M_PI * i / SPIKES) * Y_OUTER_RADIUS +
	    (double) rand() * Y_FUZZ / RAND_MAX;

	cairo_line_to (cr, x, y);
    }

    cairo_close_path (cr);
}

struct ctx {
    int first;
    cairo_t *cr;
};

static void
bend_it (double *x, double *y)
{
    const double cx = IMAGE_WIDTH / 2, cy = 500;
    double angle, radius, t;

    /* We're going to wrap the points around a big circle with center
     * at (cx, cy), with cy being somewhere well below the visible area.
     * On top of that, we'll scale up the letters going left to right.
     */

    angle = M_PI / 2 - (double) (*x - cx) / IMAGE_WIDTH;
    t = 3 * M_PI / 4 - angle + 0.05;
    angle = 3 * M_PI / 4 - pow (t, 1.8);
    radius = cy - (IMAGE_HEIGHT / 2 + (*y - IMAGE_HEIGHT / 2) * t * 2);

    *x = cx + cos (angle) * radius;
    *y = cy - sin (angle) * radius;
}

static void
make_text_path (cairo_t *cr, double x, double y, const char *text)
{
    cairo_path_t *path;
    cairo_path_data_t *data;
    int i;

    cairo_move_to (cr, x, y);
    cairo_text_path (cr, text);

    path = cairo_copy_path_flat (cr);

    cairo_new_path (cr);

    for (i=0; i < path->num_data; i += path->data[i].header.length) {
	data = &path->data[i];
	switch (data->header.type) {
	case CAIRO_PATH_MOVE_TO:
	    x = data[1].point.x;
	    y = data[1].point.y;
	    bend_it (&x, &y);
	    cairo_move_to (cr, x, y);
	    break;
	case CAIRO_PATH_LINE_TO:
	    x = data[1].point.x;
	    y = data[1].point.y;
	    bend_it (&x, &y);
	    cairo_line_to (cr, x, y);
	    break;
	case CAIRO_PATH_CLOSE_PATH:
	    cairo_close_path (cr);
	    break;
	default:
	    assert(0);
	}
    }

    free (path);
}

static cairo_status_t
stdio_write (void *closure, const unsigned char *data, unsigned int length)
{
    FILE *file = closure;
    if (fwrite (data, 1, length, file) == length)
	return CAIRO_STATUS_SUCCESS;
    else
	return CAIRO_STATUS_WRITE_ERROR;
}

int
main (int argc, char *argv[])
{
    FILE *fp;
    double x;
    double y;
    char text[20];
    cairo_text_extents_t extents;
    cairo_pattern_t *pattern;
    cairo_surface_t *surface;
    cairo_t *cr;
    char *query_string;

    query_string = getenv ("QUERY_STRING");
    if (query_string != NULL) {
	fp = stdout;
	sscanf (query_string, "text=%19s", text);
    }
    else {
	fp = fopen (filename, "w");

	if (argc == 2) {
	    strncpy (text, argv[1], sizeof text - 1);
	    text [sizeof text - 1] = '\0';
	}
	else {
	    fprintf (stderr, "Usage: %s <exclamation>\n", basename (argv[0]));
	    strcpy (text, default_text);
	    fprintf (stderr, "No exclamation provided, using \"%s\"\n", text);
	}
    }


    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					  IMAGE_WIDTH, IMAGE_HEIGHT);

    cr = cairo_create (surface);

    cairo_set_line_width (cr, 2);

    cairo_save (cr);
    cairo_translate (cr, SHADOW_OFFSET, SHADOW_OFFSET);
    make_star_path (cr);
    cairo_set_source_rgba (cr, 0., 0., 0., 0.5);
    cairo_fill (cr);
    cairo_restore (cr);

    make_star_path (cr);
    pattern =
	cairo_pattern_create_radial (IMAGE_WIDTH / 2, IMAGE_HEIGHT / 2, 10,
				     IMAGE_WIDTH / 2, IMAGE_HEIGHT / 2, 230);
    cairo_pattern_add_color_stop_rgba (pattern, 0, 1, 1, 0.2, 1);
    cairo_pattern_add_color_stop_rgba (pattern, 1, 1, 0, 0, 1);
    cairo_set_source (cr, pattern);
    cairo_fill (cr);

    make_star_path (cr);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_stroke (cr);

    cairo_select_font_face (cr, fontname,
		       CAIRO_FONT_SLANT_NORMAL,
		       CAIRO_FONT_WEIGHT_BOLD);

    cairo_set_font_size (cr, 50);
    cairo_text_extents (cr, text, &extents);
    x = IMAGE_WIDTH / 2 - (extents.width / 2 + extents.x_bearing);
    y = IMAGE_HEIGHT / 2 - (extents.height / 2 + extents.y_bearing);

    make_text_path (cr, x, y, text);

    pattern =
	cairo_pattern_create_linear (IMAGE_WIDTH / 2 - 10, IMAGE_HEIGHT / 4,
				     IMAGE_WIDTH / 2 + 10, 3 * IMAGE_HEIGHT / 4);
    cairo_pattern_add_color_stop_rgba (pattern, 0, 1, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba (pattern, 1, 0, 0, 0.4, 1);
    cairo_set_source (cr, pattern);

    cairo_fill (cr);

    make_text_path (cr, x, y, text);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_stroke (cr);

    if (query_string != NULL)
	printf ("Content-Type: image/png\n\n");

    cairo_surface_write_to_png_stream (surface, stdio_write, fp);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    fclose (fp);

    return 0;
}
