/* png frontend for cairo_snippets
 * (c) Øyvind Kolås 2004, placed in the public domain
 */

#include "snippets.h"

#define IMAGE_WIDTH  256
#define IMAGE_HEIGHT 256

#define LINE_WIDTH 0.04

/* process a snippet, writing it out to file */
static void
snippet_do_png (int no);

int
main (int argc, char **argv)
{
        int i;

	if (argc > 1) {
	    for (i = 1; i < argc; i++) {
		int snippet = snippet_name2no (argv[i]);
		if (snippet >= 0)
		    snippet_do_png (snippet);
	    }
	} else {
	    for (i=0;i<snippet_count;i++)
		snippet_do_png (i);
	}

        return 0;
}

static void
snippet_do_png (int no)
{
        cairo_t *cr;
	cairo_surface_t *surface;
        char filename[1024];

        fprintf (stdout, "processing %s", snippet_name[no]);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      IMAGE_WIDTH, IMAGE_HEIGHT);
        cr = cairo_create (surface);

        cairo_save (cr);
          snippet_do (cr, no, IMAGE_WIDTH, IMAGE_HEIGHT);
        cairo_restore (cr);

        sprintf (filename, "%s.png", snippet_name [no]);
	cairo_surface_write_to_png (surface, filename);

        fprintf (stdout, "\n");

        cairo_destroy (cr);
	cairo_surface_destroy (surface);
}
