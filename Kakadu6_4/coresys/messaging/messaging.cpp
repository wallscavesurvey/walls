/*****************************************************************************/
// File: messaging.cpp [scope = CORESYS/MESSAGING]
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
   Implements the interfaces defined by "kdu_messaging.h".
******************************************************************************/

#include <string.h>
#include <assert.h>
#include "kdu_elementary.h"
#include "kdu_messaging.h"

/* ========================================================================= */
/*                          Local Class Definitions                          */
/* ========================================================================= */

/*****************************************************************************/
/*                           kd_process_terminator                           */
/*****************************************************************************/

class kd_process_terminator {
public:
	virtual ~kd_process_terminator() { return; }
	virtual void terminate() { exit(-1); }
};

/*****************************************************************************/
/*                               kd_custom_elt                               */
/*****************************************************************************/

struct kd_custom_elt {
	const void *lead_in;  // These hold pointers to const char * or to
	const void *text;     // const kdu_uint16 *, depending on `is_unicode'
	kdu_uint32 id;
	bool is_unicode;
	kd_custom_elt *next;
};

/*****************************************************************************/
/*                             kd_custom_context                             */
/*****************************************************************************/

struct kd_custom_context {
	const char *context;
	kd_custom_elt *elts;
	kd_custom_context *next;
};

/*****************************************************************************/
/*                               kd_custom_bin                               */
/*****************************************************************************/

union kd_custom_bin {
	kd_custom_elt elt;
	kd_custom_context ctxt;
};

/*****************************************************************************/
/*                              kd_custom_store                              */
/*****************************************************************************/

struct kd_custom_store {
	kd_custom_bin bins[64]; // Use this to allocate actual storage 
	kd_custom_store *next;
};

/*****************************************************************************/
/*                               kd_custom_text                              */
/*****************************************************************************/

class kd_text_register {
public: // Member functions
	kd_text_register() { storage = NULL; next_free_elt = 64; contexts = NULL; }
	~kd_text_register()
	{
		kd_custom_store *tmp;
		while ((tmp = storage) != NULL)
		{
			storage = tmp->next; delete tmp;
		}
	}
	void add(const char *context, kdu_uint32 id,
		const char *lead_in, const char *text);
	void add(const char *context, kdu_uint32 id,
		const kdu_uint16 *lead_in, const kdu_uint16 *text);
	/* The above functions are used to implement the overloaded external
	   function, `kdu_customize_text'. */
	const kd_custom_elt *find(const char *context, kdu_uint32 id);
	/* Returns NULL if no element is found which matches the context/id
	   pair. */
private: // Data
	kd_custom_store *storage;
	int next_free_elt; // Index of first free element in head of `storage' list
	kd_custom_context *contexts;
};


/* ========================================================================= */
/*                              Static Resources                             */
/* ========================================================================= */

static kdu_message *warn_handler = NULL;
static kdu_message *err_handler = NULL;
static kd_text_register text_register;
static kd_process_terminator process_terminator;


/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN                    kdu_customize_warnings                          */
/*****************************************************************************/

void
kdu_customize_warnings(kdu_message *handler)
{
	warn_handler = handler;
}

/*****************************************************************************/
/* EXTERN                     kdu_customize_errors                           */
/*****************************************************************************/

void
kdu_customize_errors(kdu_message *handler)
{
	err_handler = handler;
}

/*****************************************************************************/
/* EXTERN                      kdu_customize_text                            */
/*****************************************************************************/

void
kdu_customize_text(const char *context, kdu_uint32 id,
	const char *lead_in, const char *text)
{
	text_register.add(context, id, lead_in, text);
}

/*****************************************************************************/
/* EXTERN                      kdu_customize_text                            */
/*****************************************************************************/

void
kdu_customize_text(const char *context, kdu_uint32 id,
	const kdu_uint16 *lead_in, const kdu_uint16 *text)
{
	text_register.add(context, id, lead_in, text);
}


/* ========================================================================= */
/*                              kd_text_register                             */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_text_register::add (ASCII)                      */
/*****************************************************************************/

void
kd_text_register::add(const char *context, kdu_uint32 id,
	const char *lead_in, const char *text)
{
	kd_custom_context *ctxt;
	for (ctxt = contexts; ctxt != NULL; ctxt = ctxt->next)
		if (strcmp(context, ctxt->context) == 0)
			break;
	if (ctxt == NULL)
	{
		if (next_free_elt == 64)
		{
			kd_custom_store *store = new kd_custom_store;
			store->next = storage;  storage = store;
			next_free_elt = 0;
		}
		assert(storage != NULL);
		ctxt = &(storage->bins[next_free_elt++].ctxt);
		ctxt->context = context;
		ctxt->elts = NULL;
		ctxt->next = contexts;
		contexts = ctxt;
	}

	kd_custom_elt *elt;
	for (elt = ctxt->elts; elt != NULL; elt = elt->next)
		if (elt->id == id)
			break;
	if (elt == NULL)
	{
		if (next_free_elt == 64)
		{
			kd_custom_store *store = new kd_custom_store;
			store->next = storage;  storage = store;
			next_free_elt = 0;
		}
		elt = &(storage->bins[next_free_elt++].elt);
		elt->id = id;
		elt->next = ctxt->elts;
		ctxt->elts = elt;
	}
	elt->is_unicode = false;
	elt->lead_in = lead_in;
	elt->text = text;
}

/*****************************************************************************/
/*                        kd_text_register::add (Unicode)                    */
/*****************************************************************************/

void
kd_text_register::add(const char *context, kdu_uint32 id,
	const kdu_uint16 *lead_in, const kdu_uint16 *text)
{
	kd_custom_context *ctxt;
	for (ctxt = contexts; ctxt != NULL; ctxt = ctxt->next)
		if (strcmp(context, ctxt->context) == 0)
			break;
	if (ctxt == NULL)
	{
		if (next_free_elt == 64)
		{
			kd_custom_store *store = new kd_custom_store;
			store->next = storage;  storage = store;
			next_free_elt = 0;
		}
		assert(storage != NULL);
		ctxt = &(storage->bins[next_free_elt++].ctxt);
		ctxt->context = context;
		ctxt->next = contexts;
		contexts = ctxt;
	}

	kd_custom_elt *elt;
	for (elt = ctxt->elts; elt != NULL; elt = elt->next)
		if (elt->id == id)
			break;
	if (elt == NULL)
	{
		if (next_free_elt == 64)
		{
			kd_custom_store *store = new kd_custom_store;
			store->next = storage;  storage = store;
			next_free_elt = 0;
		}
		elt = &(storage->bins[next_free_elt++].elt);
		elt->id = id;
		elt->next = ctxt->elts;
		ctxt->elts = elt;
	}
	elt->is_unicode = false;
	elt->lead_in = lead_in;
	elt->text = text;
}

/*****************************************************************************/
/*                            kd_text_register::find                         */
/*****************************************************************************/

const kd_custom_elt *
kd_text_register::find(const char *context, kdu_uint32 id)
{
	kd_custom_context *ctxt;
	for (ctxt = contexts; ctxt != NULL; ctxt = ctxt->next)
		if (strcmp(ctxt->context, context) == 0)
			break;
	if (ctxt == NULL)
		return NULL;
	kd_custom_elt *elt;
	for (elt = ctxt->elts; elt != NULL; elt = elt->next)
		if (elt->id == id)
			return elt;
	return NULL;
}


/* ========================================================================= */
/*                              kdu_message_queue                            */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdu_message_queue::put_text (UTF-8)                  */
/*****************************************************************************/

void kdu_message_queue::put_text(const char *string)
{
	if (active_msg == NULL)
	{
		assert(0); // Failed to call `start_message'!
		return;
	}
	int new_len = (int)strlen(string);
	if (new_len & 0xF0000000)
		return; // Ridiculously long string
	int total_len = active_msg->len + new_len;
	if (total_len & 0xF0000000)
		return; // Ridiculously long total message
	if (total_len > active_msg->max_len)
	{
		int new_max_len = active_msg->max_len + total_len;
		char *new_buf = new char[new_max_len + 1];
		active_msg->max_len = new_max_len;
		memcpy(new_buf, active_msg->buf, (size_t)(active_msg->len));
		delete[] active_msg->buf;
		active_msg->buf = new_buf;
	}
	strcpy(active_msg->buf + active_msg->len, string);
	active_msg->len = total_len;
}

/*****************************************************************************/
/*                     kdu_message_queue::put_text (unicode)                 */
/*****************************************************************************/

void kdu_message_queue::put_text(const kdu_uint16 *string)
{
	if (active_msg == NULL)
	{
		assert(0); // Failed to call `start_message'!
		return;
	}
	int new_len = 0;
	const kdu_uint16 *sp;
	for (sp = string; *sp != 0; sp++)
	{
		kdu_uint16 val = *sp;
		if (val < 0x80)
			new_len++;
		else if (val < 0x800)
			new_len += 2;
		else
			new_len += 3;
		if (new_len & 0xF0000000)
			return; // String is ridiculously long
	}
	int total_len = active_msg->len + new_len;
	if (total_len & 0xF0000000)
		return; // Ridiculously long total message
	if (total_len > active_msg->max_len)
	{
		int new_max_len = active_msg->max_len + total_len;
		char *new_buf = new char[new_max_len + 1];
		memcpy(new_buf, active_msg->buf, (size_t)(active_msg->len));
		delete[] active_msg->buf;
		active_msg->buf = new_buf;
		active_msg->max_len = new_max_len;
	}
	char *dp = active_msg->buf + active_msg->len;
	for (sp = string; *sp != 0; sp++)
	{
		kdu_uint16 val = *sp;
		if (val < 0x80)
			*(dp++) = (char)val;
		else if (val < 0x800)
		{
			*(dp++) = (char)(0xC0 | (val >> 6));
			*(dp++) = (char)(0x80 | (val & 0x3F));
		}
		else
		{
			*(dp++) = (char)(0xE0 | (val >> 12));
			*(dp++) = (char)(0x80 | ((val >> 6) & 0x3F));
			*(dp++) = (char)(0x80 | (val & 0x3F));
		}
	}
	assert(dp == (active_msg->buf + total_len));
	*dp = '\0'; // Append the null-terminator
	active_msg->len = total_len;
}


/* ========================================================================= */
/*                            kdu_message_formatter                          */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdu_message_formatter::put_text                     */
/*****************************************************************************/

void
kdu_message_formatter::put_text(const char *string)
{
	if (dest == NULL)
		return;
	for (; *string != '\0'; string++)
	{
		char nCh = *string;

		if (nCh == '\t')
		{
			if (no_output_since_newline)
			{
				int indent_change = 4;

				if ((indent + indent_change + master_indent) > max_indent)
					indent_change = max_indent - indent - master_indent;
				indent += indent_change;
				while (indent_change--)
					line_buf[num_chars++] = ' ';
				assert(num_chars < line_chars);
				continue;
			}
			else
				nCh = ' ';
		}
		else if (nCh == '\f')
		{


		}

		if (nCh == '\n')
		{
			indent = 0;
			no_output_since_newline = true;
			line_buf[num_chars] = '\0';
			(*dest) << line_buf << "\n";
			for (num_chars = 0; num_chars < master_indent; num_chars++)
				line_buf[num_chars] = ' ';
			continue;
		}

		line_buf[num_chars++] = (char)nCh;
		no_output_since_newline = false;
		if (num_chars > line_chars)
		{
			int blank_chars, output_chars, i;

			for (blank_chars = 0; blank_chars < num_chars; blank_chars++)
				if (line_buf[blank_chars] != ' ')
					break;
			for (output_chars = num_chars - 1;
				output_chars > blank_chars; output_chars--)
				if (line_buf[output_chars] == ' ')
					break;
			if ((num_chars > 0) && (line_buf[num_chars - 1] == ' '))
			{ // Eat up any extra spaces, since we are wrapping the line
				assert(output_chars == (num_chars - 1));
				while (string[1] == ' ')
					string++;
			}
			if (output_chars == blank_chars)
				output_chars = line_chars; // Have to break word across lines.
			for (i = 0; i < output_chars; i++)
				(*dest) << line_buf[i];
			while ((line_buf[output_chars] == ' ') && (output_chars < num_chars))
				output_chars++;
			(*dest) << '\n';
			num_chars = num_chars - output_chars + indent + master_indent;
			assert(num_chars <= line_chars);
			for (i = 0; i < (indent + master_indent); i++)
				line_buf[i] = ' ';
			for (; i < num_chars; i++)
				line_buf[i] = line_buf[output_chars++];
		}
	}
}

/*****************************************************************************/
/*                       kdu_message_formatter::flush                        */
/*****************************************************************************/

void
kdu_message_formatter::flush(bool end_of_message)
{
	if (dest == NULL)
		return;
	if (!no_output_since_newline)
	{
		line_buf[num_chars] = '\0';
		(*dest) << line_buf << "\n";
		for (num_chars = 0; num_chars < (indent + master_indent); num_chars++)
			line_buf[num_chars] = ' ';
		no_output_since_newline = true;
	}
	dest->flush(end_of_message);
}

/*****************************************************************************/
/*                 kdu_message_formatter::set_master_indent                  */
/*****************************************************************************/

void
kdu_message_formatter::set_master_indent(int val)
{
	if (!no_output_since_newline)
		flush();
	if (val < 0)
		val = 0;
	if (val > max_indent)
		val = max_indent;
	while (master_indent > val)
	{
		master_indent--; num_chars--;
	}
	while (master_indent < val)
	{
		master_indent++; line_buf[num_chars++] = ' ';
	}
}


/* ========================================================================= */
/*                                kdu_warning                                */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdu_warning::kdu_warning                         */
/*****************************************************************************/

kdu_warning::kdu_warning()
{
	handler = warn_handler;
	if (handler != NULL)
		handler->start_message();
	ascii_text = NULL;
	unicode_text = NULL;
	put_text("Kakadu Warning:\n");
}

/*****************************************************************************/
/*                  kdu_warning::kdu_warning (custom lead_in)                */
/*****************************************************************************/

kdu_warning::kdu_warning(const char *lead_in)
{
	handler = warn_handler;
	if (handler != NULL)
		handler->start_message();
	ascii_text = NULL;
	unicode_text = NULL;
	if (*lead_in != '\0')
		put_text(lead_in);
}

/*****************************************************************************/
/*                 kdu_warning::kdu_warning (registered text)                */
/*****************************************************************************/

kdu_warning::kdu_warning(const char *context, kdu_uint32 id)
{
	handler = warn_handler;
	if (handler != NULL)
		handler->start_message();
	const kd_custom_elt *elt = text_register.find(context, id);
	if (elt == NULL)
		handler = NULL;
	else if (!elt->is_unicode)
	{
		ascii_text = (const char *)elt->text;
		unicode_text = NULL;
		const char *lead_in = (const char *)elt->lead_in;
		if (*lead_in != '\0')
			put_text(lead_in);
	}
	else
	{
		ascii_text = NULL;
		unicode_text = (const kdu_uint16 *)elt->text;
		const kdu_uint16 *lead_in = (const kdu_uint16 *)elt->lead_in;
		if ((*lead_in != 0x0000) && (handler != NULL))
			handler->put_text(lead_in);
	}
}

/*****************************************************************************/
/*                          kdu_warning::~kdu_warning                        */
/*****************************************************************************/

kdu_warning::~kdu_warning()
{
	if (handler != NULL)
		handler->flush(true);
}

/*****************************************************************************/
/*                            kdu_warning::put_text                          */
/*****************************************************************************/

void
kdu_warning::put_text(const char *string)
{
	if (handler == NULL)
		return;
	if ((string[0] == '<') && (string[1] == '#') &&
		(string[2] == '>') && (string[3] == '\0'))
	{ // See if we can replace the special pattern with registered text
		if (ascii_text != NULL)
		{
			if (*ascii_text != '\0')
			{
				handler->put_text(ascii_text);
				for (; *ascii_text != '\0'; ascii_text++);
				ascii_text++; // Walk over the null terminator
				return;
			}
		}
		else if (unicode_text != NULL)
		{
			if (*unicode_text != 0x0000)
			{
				handler->put_text(unicode_text);
				for (; *unicode_text != 0x0000; unicode_text++);
				unicode_text++; // Walk over the null terminator
				return;
			}
		}
	}
	handler->put_text(string);
}


/* ========================================================================= */
/*                                 kdu_error                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                            kdu_error::kdu_error                           */
/*****************************************************************************/

kdu_error::kdu_error()
{
	handler = err_handler;
	if (handler != NULL)
		handler->start_message();
	ascii_text = NULL;
	unicode_text = NULL;
	put_text("Kakadu Error:\n");
}

/*****************************************************************************/
/*                   kdu_error::kdu_error (custom lead_in)                   */
/*****************************************************************************/

kdu_error::kdu_error(const char *lead_in)
{
	handler = err_handler;
	if (handler != NULL)
		handler->start_message();
	ascii_text = NULL;
	unicode_text = NULL;
	if (*lead_in != '\0')
		put_text(lead_in);
}

/*****************************************************************************/
/*                   kdu_error::kdu_error (registered text)                  */
/*****************************************************************************/

kdu_error::kdu_error(const char *context, kdu_uint32 id)
{
	handler = err_handler;
	if (handler != NULL)
		handler->start_message();
	const kd_custom_elt *elt = text_register.find(context, id);
	if (elt == NULL)
	{
		ascii_text = NULL;
		unicode_text = NULL;
		(*this) << "Untranslated error --\n"
			<< "Consult vendor for more information\n"
			<< "Details:\n"
			<< "  context=\"" << context << "\"; id=" << id << "; ";
	}
	else if (!elt->is_unicode)
	{
		ascii_text = (const char *)elt->text;
		unicode_text = NULL;
		const char *lead_in = (const char *)elt->lead_in;
		if (*lead_in != '\0')
			put_text(lead_in);
	}
	else
	{
		ascii_text = NULL;
		unicode_text = (const kdu_uint16 *)elt->text;
		const kdu_uint16 *lead_in = (const kdu_uint16 *)elt->lead_in;
		if ((*lead_in != 0x0000) && (handler != NULL))
			handler->put_text(lead_in);
	}
}

/*****************************************************************************/
/*                           kdu_error::~kdu_error                           */
/*****************************************************************************/

kdu_error::~kdu_error()
{
	if (handler != NULL)
		handler->flush(true);
	kd_process_terminator *terminator = &process_terminator;
	terminator->terminate();
}

/*****************************************************************************/
/*                             kdu_error::put_text                           */
/*****************************************************************************/

void
kdu_error::put_text(const char *string)
{
	if (handler == NULL)
		return;
	if ((string[0] == '<') && (string[1] == '#') &&
		(string[2] == '>') && (string[3] == '\0'))
	{ // See if we can replace the special pattern with registered text
		if (ascii_text != NULL)
		{
			if (*ascii_text != '\0')
			{
				handler->put_text(ascii_text);
				for (; *ascii_text != '\0'; ascii_text++);
				ascii_text++; // Walk over the null terminator
				return;
			}
		}
		else if (unicode_text != NULL)
		{
			if (*unicode_text != 0x0000)
			{
				handler->put_text(unicode_text);
				for (; *unicode_text != 0x0000; unicode_text++);
				unicode_text++; // Walk over the null terminator
				return;
			}
		}
	}
	handler->put_text(string);
}
