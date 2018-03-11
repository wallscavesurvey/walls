/*****************************************************************************/
// File: text_extractor_local.h [scope = APPS/TEXT_EXTRACTOR]
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
   Local definitions used by "kdu_text_extractor.cpp".
******************************************************************************/

#ifndef TEXT_EXTRACTOR_LOCAL_H
#define TEXT_EXTRACTOR_LOCAL_H

#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;
#include "kdu_elementary.h"

// Defined here:
class kd_line;
class kd_string;
class kd_message;

/*****************************************************************************/
/*                                  kd_line                                  */
/*****************************************************************************/

class kd_line {
  public: // Member functions
    kd_line() { buf=NULL; buf_size=0; num_chars=0; }
    ~kd_line() { if (buf != NULL) delete[] buf; }
    bool is_empty() { return (num_chars==0); }
    bool read(FILE *fp);
      /* Attempts to read a new line of text from the open file.  Returns
         true if successful. */
    const char *get() { return ((num_chars==0)?"":buf); }
  private: // Data
    int buf_size;
    int num_chars;
    char *buf;
  };

/*****************************************************************************/
/*                                 kd_string                                 */
/*****************************************************************************/

class kd_string {
  public: // Member functions
    kd_string() { buf=NULL; buf_size=0; num_chars=0; next=NULL; }
    ~kd_string() { if (buf != NULL) delete[] buf; }
    bool is_empty() { return (num_chars==0); }
    const char *add(const char *start);
      /* Expects `start' to point to a `"' symbol.  Adds all text immediately
         following this quote symbol until the terminating quote, returning
         a pointer to the terminating quote itself. */
    void clear() { num_chars=0; }
      /* Clears the internal buffer so that subsequent calls to `add' will
         start building the string from scratch. */
    const char *get() { return ((num_chars==0)?"":buf); }
      /* Retrieves a pointer to the internal string which has been assembled
         so far, or an empty string if there is none.  The string is always
         null-terminated. */
    void copy_from(kd_string &src);
  private: // Data
    int buf_size; // Length of `buf' array.
    int num_chars; // Length of current string in array (excluding null)
    char *buf;
  public: // Links
    kd_string *next;
  };

/*****************************************************************************/
/*                                kd_message                                 */
/*****************************************************************************/

class kd_message {
  public: // Member functions
    kd_message()
      {
        is_started=for_development_only=false; id=0;
        text_head=text_tail=NULL; next=NULL;
      }
    ~kd_message()
      {
        while ((text_tail=text_head) != NULL)
          { text_head = text_tail->next;  delete text_tail; }
      }
    bool started() { return is_started; }
    void start(kd_string &context, kdu_uint32 id, kd_string &lead_in,
               bool for_development_only)
      {
        while ((text_tail=text_head) != NULL)
          { text_head = text_tail->next;  delete text_tail; }
        this->is_started = true;
        this->for_development_only = for_development_only;
        this->id = id;
        this->context.copy_from(context);
        this->lead_in.copy_from(lead_in);
      }
    void add_text(kd_string &src)
      { /* Adds a new element to the list of translation strings, copied from
           `src'. */
        kd_string *elt = new kd_string;
        elt->copy_from(src);  elt->next = NULL;
        if (text_head == NULL)
          text_head = text_tail = elt;
        else
          text_tail = text_tail->next = elt;
      }
    void donate(kd_message *dst)
      { /* Donates the contents of the message to `dst' and leaves the
           current object in the unstarted state. */
        dst->start(context,id,lead_in,for_development_only);
        dst->text_head = text_head;
        dst->text_tail = text_tail;
        text_head = text_tail = NULL;
        is_started = for_development_only = false;  id = 0;
        context.clear();  lead_in.clear();
      }
  private: // Private data
    bool is_started;
  public: // Public data
    bool for_development_only;
    kd_string context;
    kdu_uint32 id;
    kd_string lead_in;
    kd_string *text_head; // For building a linked list of translation strings
    kd_string *text_tail;
    kd_message *next;
  };



#endif // TEXT_EXTRACTOR_LOCAL_H
