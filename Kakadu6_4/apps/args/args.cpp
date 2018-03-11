/*****************************************************************************/
// File: args.cpp [scope = APPS/ARGS]
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
   Implements the interfaces defined by "kdu_args.h".
******************************************************************************/

#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_args.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(args.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(args.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Argument Processing Error:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Argument Processing Warning:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================= */
/*                                kdu_args                                   */
/* ========================================================================= */

struct kd_arg_list {
  public: // Functions
    ~kd_arg_list() { delete[] string; }
  public: // Data
    kd_arg_list *next;
    char *string;
  };

/*****************************************************************************/
/*                            kdu_args::new_arg                              */
/*****************************************************************************/

void
  kdu_args::new_arg(const char *string)
{
  if (current == NULL)
    current = first;
  while ((current != NULL) && (current->next != NULL))
    current = current->next;
  prev = current;
  current = new kd_arg_list;
  current->string = new char[strlen(string)+1];
  strcpy(current->string,string);
  current->next = NULL;
  if (prev == NULL)
    first = current;
  else
    prev->next = current;
  prev = NULL;
}

/*****************************************************************************/
/*                            kdu_args::kdu_args                             */
/*****************************************************************************/

kdu_args::kdu_args(int argc, char *argv[], const char *switch_pattern)
{
  first = current = prev = removed = NULL;
  assert(argc > 0);
  prog_name = argv[0];
  for (argc--, argv++; argc > 0; argc--, argv++)
    {
      if ((switch_pattern != NULL) && (strcmp(*argv,switch_pattern) == 0))
        {
          argc--; argv++;
          if (argc == 0)
            { KDU_ERROR(e,0); e <<
                KDU_TXT("The")
                << " \"" << switch_pattern << "\" " <<
                KDU_TXT("argument must be followed by a file name "
                "from which to read arguments.");
            }
          const char *fname = *argv;
          std::ifstream stream(fname,std::ios::in);
          if (!stream)
            { KDU_ERROR(e,1); e <<
                KDU_TXT("Unable to open the argument switch file")
                << ", \"" << fname << "\"!";
            }
          int max_line_length = 100000; // Should be plenty!!
          char *line = new char[max_line_length+1];
          while (!(stream.getline(line,max_line_length).fail()))
            {
              char *start=line;
              char *end;
              do {
                while ((*start == ' ') || (*start == '\t') || (*start == '\r'))
                  start++;
                if ((*start == '#') || (*start == '%'))
                  break;
                end = start;
                while ((*end != '\0') && (*end != ' ') &&
                       (*end != '\t') && (*end != '\r'))
                  end++;
                if (end == start)
                  break;
                char tmp = *end; *end = '\0'; new_arg(start); *end = tmp;
                start = end;
              } while (*start != '\0');
            }
          delete[] line;
        }
      else
        new_arg(*argv);
    }
  current = prev = NULL;
}

/*****************************************************************************/
/*                           kdu_args::~kdu_args                             */
/*****************************************************************************/

kdu_args::~kdu_args()
{
  while ((current=first) != NULL)
    {
      first = current->next;
      delete current;
    }
  while ((current=removed) != NULL)
    {
      removed = current->next;
      delete current;
    }
}

/*****************************************************************************/
/*                          kdu_args::get_first                              */
/*****************************************************************************/

char *
  kdu_args::get_first()
{
  current = first;
  prev = NULL;
  return (current==NULL)?NULL:current->string;
}

/*****************************************************************************/
/*                             kdu_args::find                                */
/*****************************************************************************/

char *
  kdu_args::find(const char *pattern)
{
  prev = NULL;
  for (current=first; current != NULL; prev=current, current=current->next)
    if (strcmp(current->string,pattern) == 0)
      break;
  return (current==NULL)?NULL:current->string;
}

/*****************************************************************************/
/*                            kdu_args::advance                              */
/*****************************************************************************/

char *
  kdu_args::advance(bool remove_last)
{
  if (current == NULL)
    { prev = NULL; return NULL; }
  if (remove_last)
    {
      if (prev == NULL)
        {
          assert(current == first);
          first = current->next;
          current->next = removed;
          removed = current;
          current = first;
        }
      else
        {
          prev->next = current->next;
          current->next = removed;
          removed = current;
          current = prev->next;
        }
    }
  else
    {
      prev = current;
      current = current->next;
    }
  return (current==NULL)?NULL:current->string;
}

/*****************************************************************************/
/*                       kdu_args::show_unrecognized                         */
/*****************************************************************************/

int
  kdu_args::show_unrecognized(kdu_message &out)
{
  int count = 0;
  for (kd_arg_list *scan=first; scan != NULL; scan=scan->next)
    {
      out << "Unused argument: \"" << scan->string << "\"\n";
      count++;
    }
  out.flush();
  return count;
}
