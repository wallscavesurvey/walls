/*****************************************************************************/
// File: kdu_text_extractor.cpp [scope = APPS/TEXT_EXTRACTOR]
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
   Automatic documentation builder for Kakadu.
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "text_extractor_local.h"
#include "kdu_messaging.h"
#include "kdu_args.h"

/* ========================================================================= */
/*                         Set up messaging services                         */
/* ========================================================================= */

class kdu_stream_message : public kdu_message {
  public: // Member classes
    kdu_stream_message(std::ostream *stream)
      { this->stream = stream; }
    void put_text(const char *string)
      { (*stream) << string; }
    void flush(bool end_of_message=false)
      { stream->flush(); }
  private: // Data
    std::ostream *stream;
  };

static kdu_stream_message cout_message(&std::cout);
static kdu_stream_message cerr_message(&std::cerr);
static kdu_message_formatter pretty_cout(&cout_message);
static kdu_message_formatter pretty_cerr(&cerr_message);

/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                          print_usage                               */
/*****************************************************************************/

static void
  print_usage(char *prog, bool comprehensive)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage:\n  \"" << prog << " ...\n";
  out.set_master_indent(3);
  out << "-i <source>\n";
  if (comprehensive)
    out << "\tSource files from which to extract text.  These will "
           "typically be \".c\" or \".cpp\" files, but "
           "need not be.  You may use this argument multiple times to "
           "provide multiple source files.\n";
  out << "-o <output .cpp file>\n";
  if (comprehensive)
    out << "\tOutput .cpp file into which will be written everything needed "
           "to register the text extracted from the source files with the "
           "Kakadu core system.  Including the generated .cpp file in any "
           "of your Kakadu projects will make the text strings available.\n"
           "\t   Normally, you will compile the original source files with "
           "the `KDU_CUSTOM_TEXT' macro defined so that the message text will "
           "not be compiled directly into your application.  You can then "
           "produce different versions of the file generated here, for "
           "each language (e.g., English, French, Chinese) of interest, "
           "and include the desired version when actually compiling your "
           "final application.\n"
           "\t   Alternatively, you can compile the .cpp "
           "file generated here, or translated versions of it, into "
           "language specific DLL's (or shared libraries) which are "
           "loaded dynamically by your application, depending on "
           "international settings.\n"
           "\t   The one thing to remember is that you must compile the "
           "output files produced here, or translated version of them, in "
           "such as way as to link against the same instance of the Kakadu "
           "core system (or at least the same compiled instance of "
           "\"kdu_messaging.cpp\" as the rest of your application.\n";
  out << "-quiet -- don't print summary statistics.\n";
  out << "-u -- print short version of this usage statement.\n";
  out << "-usage -- print long version of the usage statement.\n";
  out.flush();
  exit(0);
}

/*****************************************************************************/
/* INLINE                       check_prefix                                 */
/*****************************************************************************/

inline bool
  check_prefix(const char *string, const char *pattern)
{
  for (; *string == *pattern; string++, pattern++);
  return (*pattern == '\0');
}

/*****************************************************************************/
/* STATIC                         read_id                                    */
/*****************************************************************************/

static bool
  read_id(const char *cp, kdu_uint32 &id)
{
  cp += strcspn(cp,",)");
  if (*cp != ',')
    return false;
  for (cp++; (*cp == ' ') || (*cp == '\t'); cp++);
  if ((cp[0] == '0') && ((cp[1] == 'x') || (cp[1] == 'X')))
    {
      if (sscanf(cp+2,"%x",&id) == 0)
        return false;
      cp += 2;
      cp += strspn(cp,"0123456789abcdefABCDEF");
    }
  else
    {
      if (sscanf(cp,"%u",&id) == 0)
        return false;
      cp += strspn(cp,"0123456789");
    }      
  return (*cp == ')');
}

/*****************************************************************************/
/* STATIC                      insert_message                                */
/*****************************************************************************/

static void
  insert_message(kd_message * &head, kd_message *new_msg,
                 const char *fname)
  /* Inserts the new message in the ordered list headed by `head',
     generating an error if the new message has the same context/id pair
     as an existing one. */
{
  kd_message *scan, *prev;
  for (prev=NULL, scan=head; scan != NULL; prev=scan, scan=scan->next)
    {
      int context_compare =
        strcmp(scan->context.get(),new_msg->context.get());
      if ((context_compare == 0) && (scan->id == new_msg->id))
        { kdu_error e; e << "Encountered a message whose context/id pair "
          "is not unique.\n"
          "\t  Problem occurred in file, \"" << fname << "\".\n"
          "\t  Context=\"" << new_msg->context.get() << "\"; id = "
          << new_msg->id;
          e.set_hex_mode(true);
          e << " = 0x" << new_msg->id;
          e.set_hex_mode(false);
        }
      if ((context_compare > 0) ||
          ((context_compare == 0) && (scan->id > new_msg->id)))
        break;
    }
  new_msg->next = scan;
  if (prev == NULL)
    {
      assert(head == scan);
      head = new_msg;
    }
  else
    prev->next = new_msg;
}

/*****************************************************************************/
/* STATIC                    process_input_file                              */
/*****************************************************************************/

static void
  process_input_file(FILE *in, kd_message * &messages, const char *fname,
                     bool quiet)
{
  int num_errors = 0;
  int num_warnings = 0;
  int num_dev_errors = 0;
  int num_dev_warnings = 0;

  kd_string error_context;
  kd_string warning_context;
  kd_string error_lead_in;
  kd_string warning_lead_in;

  kd_message msg; // Temporary resource for building messages
  kd_string txt; // Temporary resource for assembling contents of KDU_TXT()
  kd_line line;
  int unmatched_braces = 0;
  bool in_txt = false; // If inside the `KDU_TXT' macro
  bool in_comment = false;
  while (line.read(in))
    {
      const char *cp;
      if (error_context.is_empty() &&
          ((cp=strstr(line.get(),"kdu_error _name(\"")) != NULL))
        error_context.add(strchr(cp,'\"'));
      else if (error_lead_in.is_empty() &&
               ((cp=strstr(line.get(),"kdu_error _name(\"")) != NULL))
        error_lead_in.add(strchr(cp,'\"'));
      else if (warning_context.is_empty() &&
               ((cp=strstr(line.get(),"kdu_warning _name(\"")) != NULL))
        warning_context.add(strchr(cp,'\"'));
      else if (warning_lead_in.is_empty() &&
               ((cp=strstr(line.get(),"kdu_warning _name(\"")) != NULL))
        warning_lead_in.add(strchr(cp,'\"'));

     cp = line.get();
     if (*cp == '#')
       continue;

     for (; *cp != '\0'; cp++)
       {
         if ((cp[0] == '/') && (cp[1] == '/'))
           break; // Rest of line is a comment
         if (in_comment)
           {
             if ((cp[0] == '*') && (cp[1] == '/'))
               { in_comment = false; cp++; }
             continue;
           }
         if ((cp[0] == '/') && (cp[1] == '*'))
           { in_comment = true; cp++; continue; }
         if (!msg.started())
           {
             kdu_uint32 id;
             if (check_prefix(cp,"KDU_ERROR("))
               {
                 cp += strlen("KDU_ERROR(");
                 if (read_id(cp,id) &&
                     !(error_context.is_empty() ||
                       error_lead_in.is_empty()))
                   {
                     msg.start(error_context,id,error_lead_in,false);
                     num_errors++;
                   }
               }
             else if (check_prefix(cp,"KDU_ERROR_DEV("))
               {
                 cp += strlen("KDU_ERROR_DEV(");
                 if (read_id(cp,id) &&
                     !(error_context.is_empty() ||
                       error_lead_in.is_empty()))
                   {
                     msg.start(error_context,id,error_lead_in,true);
                     num_errors++;  num_dev_errors++;
                   }
               }
             else if (check_prefix(cp,"KDU_WARNING("))
               {
                 cp += strlen("KDU_WARNING(");
                 if (read_id(cp,id) &&
                     !(warning_context.is_empty() ||
                       warning_lead_in.is_empty()))
                   {
                     msg.start(warning_context,id,warning_lead_in,false);
                     num_warnings++;
                   }
               }
             else if (check_prefix(cp,"KDU_WARNING_DEV("))
               {
                 cp += strlen("KDU_WARNING_DEV(");
                 if (read_id(cp,id) &&
                     !(warning_context.is_empty() ||
                       warning_lead_in.is_empty()))
                   {
                     msg.start(warning_context,id,warning_lead_in,true);
                     num_warnings++;  num_dev_warnings++;
                   }
               }
             assert((unmatched_braces == 0) && !in_txt);
             continue;
           }

         // If we get here, a message has been started
         if (!in_txt)
           {
             if (check_prefix(cp,"KDU_TXT("))
               {
                 cp += strlen("KDU_TXT");
                 in_txt = true;
               }
             else if (*cp == '{')
               unmatched_braces++;
             else if (*cp == '}')
               {
                 if (unmatched_braces > 0)
                   unmatched_braces--;
                 else
                   {
                     kd_message *new_msg = new kd_message;
                     msg.donate(new_msg);
                     assert(!msg.started());
                     insert_message(messages,new_msg,fname);
                   }
               }
             continue;
           }

         // If we get here, we are inside a `KDU_TXT' macro
         if (*cp == '\"')
           cp = txt.add(cp);
         else if (*cp == ')')
           {
             msg.add_text(txt);
             txt.clear();
             in_txt = false;
           }
       }
    }

  if (msg.started())
    { kdu_error e; e << "Encountered incomplete \"KDU_ERROR\", "
      "\"KDU_ERROR_DEV\", \"KDU_WARNING\" or \"KDU_WARNING_DEV\" environment "
      "while parsing source file \"" << fname << "\"."; }

  if (!quiet)
    {
      pretty_cout << ">> Parsed " << num_errors << " KDU_ERROR messages ("
                  << num_dev_errors << " for developers only)\n\tand "
                  << num_warnings << " KDU_WARNING messages ("
                  << num_dev_warnings << " for developers only)\n\tfrom \""
                  << fname << "\".\n";
      pretty_cout.flush();
    }
}

/*****************************************************************************/
/* STATIC                   generate_output_file                             */
/*****************************************************************************/

static void
  generate_output_file(FILE *out, kd_message *messages, const char *fname,
                       bool quiet)
{
  fprintf(out,
    "// -----------------------------------------------------------------\n"
    "// This file is automatically generated by \"kdu_text_extractor\"\n"
    "// Do not edit the file directly.  You may, however, use this file\n"
    "// as a template for alternate versions in which the text strings\n"
    "// are translated into different languages.\n"
    "// -----------------------------------------------------------------\n"
    "#include <kdu_messaging.h>\n"
    "\n");

  fprintf(out,
    "struct kdl_message_info{\n"
    "  const char *context;\n"
    "  kdu_uint32 id;\n"
    "  const char *lead_in;\n"
    "  const char *text;\n"
    "};\n"
    "\n");

  fprintf(out,
    "static bool\n"
    "  kdl_register_text(kdl_message_info *info, int num_records)\n"
    "{\n"
    "  for (; num_records > 0; num_records--, info++)\n"
    "    kdu_customize_text(info->context,info->id,\n"
    "                       info->lead_in,info->text);\n"
    "  return true;\n"
    "}\n"
    "\n");

  kd_message *scan;
  kd_string *txt;
  int n, num_records;
  
  for (num_records=0, scan=messages; scan != NULL; scan=scan->next)
    if (!scan->for_development_only)
      num_records++;
  if (num_records > 0)
    {
      fprintf(out,
        "static kdl_message_info data[%d] = {\n",num_records);
      for (n=num_records, scan=messages; scan != NULL; scan=scan->next)
        if (!scan->for_development_only)
          {
            fprintf(out,
              "  {\n"
              "    \"%s\", %u,\n",scan->context.get(),scan->id);
            fprintf(out,
              "    \"%s\",\n",scan->lead_in.get());
            for (txt=scan->text_head; txt != NULL; txt=txt->next)
              fprintf(out,
                "    \"%s\\0\"\n",txt->get());
            if (scan->text_head == NULL)
              fprintf(out,"    \"\\0\"\n");
            fprintf(out,"  }");
            n--;
            if (n != 0)
              fprintf(out,",\n");
            else
              fprintf(out,"\n");
          }
      fprintf(out,
        "};\n"
        "\n"
        "static bool dummy_val = kdl_register_text(data,%d);\n"
        "\n",num_records);
    }

  for (num_records=0, scan=messages; scan != NULL; scan=scan->next)
    if (scan->for_development_only)
      num_records++;
  if (num_records > 0)
    {
      fprintf(out,
        "#ifndef KDU_EXCLUDE_DEVELOPER_TEXT\n"
        "\n");
      fprintf(out,
        "static kdl_message_info dev_data[%d] = {\n",num_records);
      for (n=num_records, scan=messages; scan != NULL; scan=scan->next)
        if (scan->for_development_only)
          {
            fprintf(out,
              "  {\n"
              "    \"%s\", %u,\n",scan->context.get(),scan->id);
            fprintf(out,
              "    \"%s\",\n",scan->lead_in.get());
            for (txt=scan->text_head; txt != NULL; txt=txt->next)
              fprintf(out,
                "    \"%s\\0\"\n",txt->get());
            if (scan->text_head == NULL)
              fprintf(out,"    \"\\0\"\n");
            fprintf(out,"  }");
            n--;
            if (n != 0)
              fprintf(out,",\n");
            else
              fprintf(out,"\n");
          }
      fprintf(out,
        "};\n"
        "\n"
        "static bool dev_dummy_val = kdl_register_text(dev_data,%d);\n"
        "\n",num_records);
      fprintf(out,
        "#endif // !KDU_EXCLUDE_DEVELOPER_TEXT\n");
    }

  if (!quiet)
    {
      pretty_cout << "Generated language file \"" << fname << "\"\n";
      pretty_cout.flush();
    }
}


/* ========================================================================= */
/*                                kd_line                                    */
/* ========================================================================= */

/*****************************************************************************/
/*                             kd_line::read                                 */
/*****************************************************************************/

bool
  kd_line::read(FILE *fp)
{
  num_chars = 0;
  int ch;
  while ((ch=getc(fp)) != EOF)
    {
      if (buf_size < (num_chars+2))
        {
          buf_size += 80;
          char *new_buf = new char[buf_size];
          if (num_chars > 0)
            memcpy(new_buf,buf,(size_t) num_chars);
          if (buf != NULL)
            delete[] buf;
          buf = new_buf;
        }
      if (ch == '\r')
        continue;
      if (ch == '\n')
        break;
      buf[num_chars++] = (char) ch;
    }
  if (buf != NULL)
    buf[num_chars] = '\0';
  return ((num_chars > 0) || (ch != EOF));
}


/* ========================================================================= */
/*                               kd_string                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                             kd_string::add                                */
/*****************************************************************************/

const char *
  kd_string::add(const char *start)
{
  assert(*start == '\"');
  start++;
  const char *end = start;
  for (; *end != '\"'; end++)
    {
      if (*end == '\0')
        { kdu_error e; e << "New-line encountered within string while "
          "extracting text from the \"KDU_TXT\" macro."; }
      if ((end[0] == '\\') && (end[1] == '\"'))
        end++;
    }
  int new_chars = (int)(end-start);
  if ((num_chars+new_chars+1) > buf_size)
    { // Reallocate buffer
      buf_size = num_chars+new_chars+1;
      char *new_buf = new char[buf_size];
      if (num_chars > 0)
        memcpy(new_buf,buf,(size_t) num_chars);
      if (buf != NULL)
        delete[] buf;
      buf = new_buf;
    }
  memcpy(buf+num_chars,start,(size_t) new_chars);
  num_chars += new_chars;
  buf[num_chars] = '\0';
  return end;
}

/*****************************************************************************/
/*                          kd_string::copy_from                             */
/*****************************************************************************/

void
  kd_string::copy_from(kd_string &src)
{
  if (buf_size < (src.num_chars+1))
    {
      buf_size = src.num_chars+1;
      if (buf != NULL)
        delete[] buf;
      buf = new char[buf_size];
    }
  num_chars = src.num_chars;
  if (num_chars > 0)
    memcpy(buf,src.buf,num_chars);
  buf[num_chars] = '\0';
}


/* ========================================================================= */
/*                           External Functions                              */
/* ========================================================================= */

/*****************************************************************************/
/*                                  main                                     */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);
  kdu_args args(argc,argv,"-s");

  if ((args.get_first() == NULL) || (args.find("-usage") != NULL))
    print_usage(args.get_prog_name(),true);
  if (args.find("-u") != NULL)
    print_usage(args.get_prog_name(),false);
  bool quiet = false;
  if (args.find("-quiet") != NULL)
    {
      quiet = true;
      args.advance();
    }

  kd_message *messages=NULL;
  while (args.find("-i") != NULL)
    {
      const char *string = args.advance();
      if (string == NULL)
        { kdu_error e;
          e << "The \"-i\" argument requires an input file name."; }
      FILE *in = fopen(string,"r");
      if (in == NULL)
        { kdu_error e; e << "Cannot open input file, \"" << string << "\"."; }
      process_input_file(in,messages,string,quiet);
      fclose(in);
      args.advance();
    }

  if (args.find("-o") != NULL)
    {
      const char *string = args.advance();
      if (string == NULL)
        { kdu_error e;
          e << "The \"-o\" argument requires an output file name."; }
      FILE *out = fopen(string,"w");
      if (out == NULL)
        { kdu_error e; e << "Cannot open output file, \"" << string << "\"."; }
      generate_output_file(out,messages,string,quiet);
      fclose(out);
      args.advance();
    }

  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

  return 0;
}
