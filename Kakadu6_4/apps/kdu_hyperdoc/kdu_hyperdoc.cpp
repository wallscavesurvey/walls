/*****************************************************************************/
// File: kdu_hyperdoc.cpp [scope = APPS/HYPERDOC]
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
#include "hyperdoc_local.h"
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
  print_usage(char *prog)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage:\n  \"" << prog << " ...\n";
  out.set_master_indent(3);
  out << "<header1.h*>,...\n";
  out << "\t   Any argument with the suffix \".h\" is considered "
         "the name of a source header file from which to build "
         "documentation.  Java interfaces generated via the \"-java\" "
         "switch and C# and VB interfaces generated via the \"-managed\" "
         "switch are all based upon the same set of header files; however "
         "interface bindings are generated only for those classes (or "
         "structures) declared in the header files, which contain an "
         "appropriate \"BIND:\" clause in their descriptions -- see the "
         "companion documentation if you wish to use this tool to build "
         "Java or managed native interface bindings to your own "
         "C++ objects.\n";
  out << "<extra1.*>,...\n";
  out << "\t   Any argument with an unrecognized suffix is interpreted as the "
         "name of a file to which a link is to be included on the main "
         "navigation bar of the documentation system.\n";
  out << "-o <output directory>\n";
  out << "\t   Location in which to put all HTML documentation files, except "
         "for the master navigation files which are always written to the "
         "current working directory.\n";
  out << "-long -- generate long HTML names\n";
  out << "\t   By default, the hyperdoc utility restricts the length "
         "of the HTML file names which it generates to at most 32 characters "
         "to cater for some problems which have been observed with long "
         "file names on the Mac.  If you want the original long file names, "
         "specify this flag.\n";
  out << "-java <CLASS_dir> <JNI_dir> <AUX_dir> <INCLUDE_dir>\n";
  out << "\t   Use this argument if you want to build Java native interfaces "
         "for use with the Kakadu system.  The first argument provides the "
         "directory into which the Java class definitions will be written, "
         "with extension, \".java\".  You will have to compile these "
         "into Java byte code yourself using a command-line like "
         "\"javac *.java\", for example.  The second and third arguments "
         "provide directories into which automatically generated C++ code "
         "will be written (see next paragraph), while the "
         "fourth argument provides a directory into which all relevant "
         "public Kakadu header files will be copied, for ease of access "
         "during subsequent compilation phases.  When working in the "
         "standard Kakadu build environment, these last three directories "
         "should be \"<base>/managed/kdu_jni\", \"<base>/managed/kdu_aux\" "
         "and \"<base>/managed/all_includes\", respectively.  Here, "
         "<base> stands for the top-level directory of your Kakadu "
         "distribution -- the one which contains sub-directories \"apps\", "
         "\"coresys\", and so forth.  The content written to <AUX_dir> and "
         "<INCLUDE_dir> is identical to that written when producing managed "
         "native interfaces via the \"-managed\" argument (see below).\n"
         "\t   The <AUX_dir> directory receives files \"kdu_aux.cpp\" "
         "and \"kdu_aux.h\".  These should be compiled together with the "
         "source files which implement all of the non-coresys objects "
         "declared in the supplied headers, creating a "
         "a DLL (Windows \".dll\" file) or shared library (Unix \".so\" "
         "file).  When building a windows DLL, be sure to define the "
         "special symbol, \"KDU_AUX_EXPORTS\" -- this is not necessary "
         "when building Unix shared libraries.  For your convenience, sample "
         "Unix make files and .NET build environments for achieving this "
         "objective are provided in the \"<base>/managed/kdu_aux\", but "
         "you may need to add or remove source files from these build "
         "environments, depending on how much of the Kakadu functionality "
         "you are making available to Java.\n"
         "\t   The <JNI_dir> directory receives files \"kdu_jni.cpp\" "
         "and \"kdu_jni.h\", which you need to compile against standard "
         "Java files, \"jni.h\" and \"jni_md.h\", together with the "
         "Kakadu core system DLL or shared library and the \"kdu_aux\" DLL "
         "or shared library, produced as described in the previous "
         "paragraph.  The compiled result should be a DLL or shared library "
         "named \"kdu_jni.dll\" (or \"kdu_jni.so\").  For your convenience, "
         "sample Unix make files and .NET build environments for achieving "
         "this objective are provided in the \"<base>/managed/kdu_jni\" "
         "directory of the standard Kakadu distribution.  You need to put all "
         "three DLL's or shared libraries in the executable path seen by "
         "the JVM when it runs.  You also need to put the compiled Java "
         "class files, from the `CLASS_dir', in the class loading path seen "
         "by the JVM when it runs.\n"
         "\t   Use of the \"-java\" option also causes the "
         "Java interface syntax to be appended to all class "
         "and function definitions in the generated HTML documentation.\n";
  out << "-managed <MNI_dir> <AUX_dir> <INCLUDE_dir>\n";
  out << "\t   Use this argument to build interfaces for accessing "
         "Kakadu via C#, Visual Basic, or possibly other managed langages.  "
         "The interfaces are implemented using Microsoft's "
         "\"Managed Extensions for C++\".  Both Java and managed bindings "
         "can (and generally should) share the same \"kdu_aux\" DLL or "
         "shared library, code for which is generated and written to the "
         "<AUX_dir> directory, using exactly the same procedure described "
         "above in connection with the \"-java\" argument.  Both Java and "
         "managed bindings can (and generally should) also share the same "
         "set of common include files, all of which are written to the "
         "<INCLUDE_dir> directory, following the same procedure described "
         "above for \"-java\".  As described there, these directories should "
         "be set to \"<base>/managed/kdu_aux\" and "
         "\"<base>/managed/all_includes\" when working with standard Kakadu "
         "build environments.\n"
         "\t   The <MNI_dir> directory receives two files, \"kdu_mni.h\" "
         "and \"kdu_mni.cpp\", which should be compiled against the header "
         "files in the <INCLUDES_dir> directory as a .NET class library, "
         "using Microsoft's managed extensions for C++.  Versions of .NET "
         "prior to the 2005 release used an older, now deprecated syntax, "
         "which can be accessed via the \"-old_managed_syntax\" argument.  "
         "In any case, successful compilation with an appropriate syntax for "
         "the compiler you have will result in a DLL which can be linked "
         "against the Kakadu core system DLL (e.g., "
         "\"kdu_v52R.dll\") and the \"kdu_aux.dll\" file, built in the "
         "manner described above under \"-java\".  The resulting DLL and "
         "its assembly provide a namespace, \"kdu_mni\", which offers the "
         "correctly bound functions for use in C#, visual basic, and "
         "other managed languages.  A sample .NET workspace is provided in "
         "the \"<base>/managed\" directory, to accomplish these objectives; "
         "it also provides some simple examples in C# and/or Visual Basic.\n"
         "\t   One important point to note is that the \"-managed\" command "
         "line option also generates files \"kdu_constants.cs\" and "
         "\"kdu_constants.vb\", which are written to the <MNI_dir> "
         "directory.  You should import these into your C# and VB "
         "applications, respectively, to gain access to the constants "
         "defined in the \"kdu_globals\" class.  You need to explicitly "
         "import the relevant constants file into each of the examples in "
         "the \"<base>/managed/managed.sln\" .NET workspace before "
         "compiling them -- use \"Project->Add Existing Item\" to do this.\n"
         "\t   Use of the \"-managed\" option also causes the "
         "C# and Visual Basic interface syntax to be appended to all class "
         "and function definitions in the generated HTML documentation.\n";
  out << "-old_managed_syntax -- for .NET 2003 managed native interfaces.\n";
  out << "\t   As mentioned above, the \"-managed\" argument generates "
         "source and header files \"kdu_mni.cpp\" and \"kdu_mni.h\" which "
         "conform to Microsoft's \"managed extensions for C++\", allowing "
         "the construction of portable class libraries which can run under "
         "the Commmon Language Runtime (CLR).  The syntax for this "
         "extended language changed dramatically between Microsoft's "
         "Visual Studio 2003 and Visual Studio 2005.  The new syntax is "
         "supposed to be the product of a standardization effort, so it "
         "is now used by default.  To access the old syntax, you should "
         "specify the \"-old_managed_syntax\".  This is essential if you "
         "intend to compile managed interfaces using Visual Studio 2003.  "
         "It is possible to build the old syntax under Visual Studio 2005 "
         "also, but only by specifying the \"/clr:oldSyntax\" compiler "
         "switch.\n";
  out << "-bind <class or function name to be bound>\n"
         "\t   By default, the \"-java\" and \"-managed\" options create "
         "language bindings for all possible classes and global functions.  "
         "The present argument, however, allows you to limit the set of "
         "classes or functions for which bindings will be created.  "
         "If this argument appears one or more times, bindings will not "
         "be created for any classes or functions other than those which "
         "are specified by \"-bind\" arguments.\n"
         "\t   Each \"-bind\" argument should provide the name of "
         "a single class, global function or class member function, where "
         "global functions must be prefixed by \"::\" and class member "
         "functions must be separated from their class name by the same "
         "C++ separator, \"::\".  Note that you must use the C++ names of "
         "the native Kakadu API, rather than the slightly different Java, "
         "C# or Visual Basic names.  As a convenience, however, warnings "
         "will be generated if any of the names you specify cannot be "
         "matched.\n"
         "\t   If you specify a class name, bindings will be created for "
         "all member functions of that class, as well as any base classes, "
         "if possible.  Note carefully, however, that bindings for functions "
         "can be created only if all non-default arguments for the function "
         "have bindings themelves.  As a result, you may find that you "
         "need to experiment with adding additional classes to your binding "
         "file until you get the functionality you are interested in.\n"
         "\t   The principle purpose of the \"-bind\" argument is to provide "
         "you with a mechanism for creating minimal binding interfaces for "
         "your foreign language applications.  You would normally only "
         "do this once you were ready to ship a product, at which point, the "
         "time required for a little fiddling around should be acceptable.\n";
  out << "-s <switch file>\n";
  out << "\tSwitch to reading arguments from a file.  In the file, argument "
         "strings are separated by whitespace characters, including spaces, "
         "tabs and new-line characters.  Comments may be included by "
         "introducing a `#' or a `%' character, either of which causes "
         "the remainder of the line to be discarded.  Any number of "
         "\"-s\" argument switch commands may be included on the command "
         "line.\n";
  out << "-u -- print this usage statement.\n";
  out.flush();
  exit(0);
}

/*****************************************************************************/
/* INLINE                          skip_white                                */
/*****************************************************************************/

static inline char *
  skip_white(char *string)
  /* Returns a pointer to the first non-white character in the string, or
     the terminating null character if there is none. */
{
  while ((*string == ' ') || (*string == '\t') || (*string == '\r'))
    string++;
  return string;
}

/*****************************************************************************/
/* INLINE                         copy_string                                */
/*****************************************************************************/

static inline void
  copy_string(char * &dst, const char *src)
{
  if (dst != NULL)
    delete[] dst;
  dst = NULL;
  if ((src == NULL) || (*src == '\0'))
    return;
  dst = new char[strlen(src)+1];
  strcpy(dst,src);
}

/*****************************************************************************/
/* STATIC                          start_var                                 */
/*****************************************************************************/

static kdd_variable *
  start_var(char *string, kdd_variable * &list, kdd_class *owner,
            kdd_file *file, int line_number)
  /* This function is called when a variable declaration is suspected.  If
     the suspicion turns out to be unfounded, NULL will be returned.
     Otherwise, a new `kdd_variable' object is created and appended to the
     list of variables managed by `owner'. */
{
  string = skip_white(string);

  // Skip over function prototypes and forward declarations.
  if ((strncmp(string,"typedef ",8) == 0) ||
      (strncmp(string,"class ",6) == 0) ||
      (strncmp(string,"struct ",7) == 0) ||
      (strncmp(string,"union ",6) == 0))
    return NULL;

  char *initializer = strrchr(string,'=');
  char *scan;
  if (initializer != NULL)
    {
      scan = initializer;
      initializer = skip_white(initializer+1);
    }
  else
    scan = string + strlen(string);
  while ((scan > string) && (scan[-1] == ' '))
    scan--;
  int as_array = 0;
  if ((scan > string) && (scan[-1] == ']'))
    {
      for (scan--; (scan > string) && (scan[-1] != '['); scan--);
      if (scan <= string)
        return NULL;
      scan--;
      as_array = 1;
    }
  *scan = '\0';
  while ((scan > string) && (scan[-1] != ' ') &&
         (scan[-1] != '*') && (scan[-1] != '&'))
    scan--;
  if (scan <= string)
    return NULL;

  // Collect any indirection modifiers
  char *name = scan;
  bool by_ref = false;
  int derefs = 0;
  for (; scan > string; scan--)
    {
      if (scan[-1] == '*')
        derefs++;
      else if (scan[-1] == '&')
        {
          if (derefs || by_ref)
            { kdu_error e;
              e << "By-reference arguments or data members must have a single "
                "\"&\" modifier following any \"*\" indirection modifiers.\n"
                << "\tProblem encountered on line " << line_number
                << " of file \"" << file->pathname << "\".";
            }
          by_ref = true;
        }
      else if (scan[-1] != ' ')
        break;
    }
  if (scan <= string)
    return NULL;

  // At this point we have what we consider to be a valid variable definition.
  kdd_variable *tail = NULL;
  if (list != NULL)
    do {
        tail = (tail==NULL)?list:(tail->next);
        if (strcmp(tail->name,name) == 0)
          { kdu_error e;
            e << "Multiple definitions for variable \"" << name << "\" in "
                 "current scope.\n"
              << "\tFirst definition found at line " << tail->line_number
              << " in file \"" << tail->file->pathname << "\".\n"
              << "\tSecond definition found at line " << line_number
              << " in file \"" << file->pathname << "\".";
          }
      } while (tail->next != NULL);
  if (tail == NULL)
    list = tail = new kdd_variable;
  else
    tail = tail->next = new kdd_variable;
  copy_string(tail->name,name);
  tail->file = file;
  tail->line_number = line_number;
  tail->owner = owner;
  copy_string(tail->initializer,initializer);
  tail->type.by_ref = by_ref;
  tail->type.derefs = derefs;
  tail->type.as_array = as_array;

  // Now separate the type name from any pre-modifier.
  *scan = '\0'; // Work back from here to get the type name.
  while ((scan > string) && (scan[-1] != ' '))
    scan--;
  assert(*scan != '\0');
  copy_string(tail->type.unknown_name,scan);
  while ((scan > string) && (scan[-1] == ' '))
    scan--;
  if (owner != NULL)
    return tail;
       // Discard modifiers except for function arguments.  They may include
       // such strings as "KDU_EXPORT" and "extern", which we do not want.
       // Even worse, there is no guarantee that all kinds of additional junk
       // will not be prepended.

  if (scan > string)
    { // We have a pre-modifier.
      *scan = '\0';
      copy_string(tail->type.modifier,string);
    }
  return tail;
}

/*****************************************************************************/
/* STATIC                         start_func                                 */
/*****************************************************************************/

static kdd_func *
  start_func(char *string, kdd_class *owner, kdd_file *file, int line_number)
  /* This function is called when a function declaration is suspected.  If
     the suspicion turns out to be unfounded, NULL will be returned.
     Otherwise, a new `kdd_func' object is created and appended to the list
     of functions managed by `owner'. */
{
  string = skip_white(string);

  // Skip over function prototypes
  if (strstr(string,"typedef") != NULL)
    return NULL;

  char *arg_start = strchr(string,'(');
  if (arg_start == NULL)
    return NULL;
  char *arg_end = strrchr(arg_start+1,')');
  if (arg_end == NULL)
    return NULL;
  *(arg_start++) = *arg_end = '\0';

  char *scan = arg_start-1;
  while ((scan > string) && (scan[-1] == ' '))
    scan--;
  *scan = '\0';
  char *tscan = strstr(string,"operator&"); // Special case.
  if ((tscan != NULL) &&
      ((tscan == string) || (tscan[-1] == ' ') ||
       (tscan[-1] == '*') || (tscan[-1] == '&')))
    scan = tscan;
  else
    {
      while ((scan > string) && (scan[-1] != ' ') &&
             (scan[-1] != '*') && (scan[-1] != '&'))
        scan--;
    }
  if (*scan == '\0')
    return NULL;
  char *name = scan;
  if (strstr(name,"::") != NULL)
    return NULL;

  // Collect any indirection modifiers for the return type.
  bool by_ref = false;
  int derefs = 0;
  for (; scan > string; scan--)
    {
      if (scan[-1] == '*')
        derefs++;
      else if (scan[-1] == '&')
        {
          if (derefs || by_ref)
            { kdu_error e;
              e << "Functions which return values by reference must have a "
                "single \"&\" modifier following any \"*\" indirection "
                "modifiers.\n"
                << "\tProblem encountered on line " << line_number
                << " of file \"" << file->pathname << "\".";
            }
          by_ref = true;
        }
      else if (scan[-1] != ' ')
        break;
    }

  // Create the function
  kdd_func *tail = owner->functions;
  if (tail != NULL)
    {
      while (tail->next != NULL)
        tail = tail->next;
      tail = tail->next = new kdd_func;
    }
  else
    tail = owner->functions = new kdd_func;
  copy_string(tail->name,name);
  tail->file = file;
  tail->line_number = line_number;
  tail->owner = owner;
  *scan = '\0';

  // Look for keywords, "virtual" and "static"
  char *tmp, *new_string = string;
  if (((tmp = strstr(string,"virtual")) != NULL) &&
      ((tmp == string) || (tmp[-1] == ' ')) &&
      ((tmp[7] == '\0') || (tmp[7] == ' ')))
    {
      tail->is_virtual=true; new_string = skip_white(tmp+7);
    }
  if (((tmp = strstr(string,"static")) != NULL) &&
      ((tmp == string) || (tmp[-1] == ' ')) &&
      ((tmp[6] == '\0') || (tmp[6] == ' ')))
    {
      tail->is_static=true; tmp = skip_white(tmp+6);
      new_string = (tmp>new_string)?tmp:new_string;
    }
  string = new_string;
  
  // Skip over KDU_EXPORT or KDU_AUX_EXPORT prefix if any
  if ((tmp = strstr(string,"KDU_EXPORT")) != NULL)
    string = skip_white(tmp+strlen("KDU_EXPORT"));
  else if ((tmp = strstr(string,"KDU_AUX_EXPORT")) != NULL)
    string = skip_white(tmp+strlen("KDU_AUX_EXPORT"));
    
  // Look for the return type if any.
  while ((scan > string) && (scan[-1] != ' '))
    scan--;
  copy_string(tail->return_type.unknown_name,scan);
  tail->return_type.by_ref = by_ref;
  tail->return_type.derefs = derefs;

  // Look for specific modifiers for the return type (only "const" currently)
  *scan = '\0';
  if (strstr(string,"const ") != NULL)
    copy_string(tail->return_type.modifier,"const");

  // Fill out the argument list.
  arg_start = skip_white(arg_start);
  while (arg_start < arg_end)
    {
      char *delim = strchr(arg_start,',');
      if (delim == NULL)
        delim = arg_end;
      else
        { *delim = '\0'; delim = skip_white(delim+1); }
      start_var(arg_start,tail->args,NULL,file,line_number);
      arg_start = delim;
    }

  // Check for pure virtual function
  arg_end = skip_white(arg_end+1);
  if (*arg_end == '=')
    {
      arg_end = skip_white(arg_end+1);
      if (*arg_end == 0)
        {
          tail->is_pure = true;
          owner->is_abstract = true;
        }
    }

  return tail;
}

/*****************************************************************************/
/* STATIC                         start_class                                */
/*****************************************************************************/

static kdd_class *
  start_class(char *string, kdd_class *list, kdd_file *file, int line_number)
  /* This function is called once the keyword `class' or `struct' has been
     encountered.  The supplied `string' starts immediately after this
     keyword and continues until the opening '{' symbol of the class/struct
     definition.  If the function fails to identify a valid class or structure
     declaration, it generates a terminal error; it will never return NULL.
     The new class is appended to the list headed by `list'. */
{
  string = skip_white(string);
  char *name = string;
  while ((*string != '\0') && (*string != ':') &&
         (*string != ' ') && (*string != '\t'))
    string++;
  if ((*string != '\0') && (*string != ':'))
    {
      *string = '\0';
      string = skip_white(string+1);
    }
  char *base_name = NULL;
  if (*string == ':')
    {
      *string = '\0';
      string = skip_white(string+1);
      if (strncmp(string,"public",6) == 0)
        string += 6;
      else if (strncmp(string,"private",7) == 0)
        string += 7;
      else if (strncmp(string,"protected",9) == 0)
        string += 9;
      string = skip_white(string);
      if (*string != '\0')
        {
          base_name = string;
          while ((*string != '\0') && (*string != ' ') &&
                 (*string != '\t') && (*string != ','))
            string++;
          *string = '\0';
        }
    }

  kdd_class *tail=list;
  assert((tail != NULL) && (tail->name == NULL)); // List starts with globals
  while (tail->next != NULL)
    {
      tail = tail->next;
      if (strcmp(tail->name,name) == 0)
        { kdu_error e;
          e << "Multiple definitions for class/struct \"" << name << "\".\n"
            << "\tFirst definition found at line " << tail->line_number
            << " in file \"" << tail->file->pathname << "\".\n"
            << "\tSecond definition found at line " << line_number
            << " in file \"" << file->pathname << "\".";
        }
    }
  tail = tail->next = new kdd_class;
  copy_string(tail->name,name);
  tail->file = file;
  if (base_name != NULL)
    copy_string(tail->base.unknown_name,base_name);
  tail->line_number = line_number;
  return tail;
}

/*****************************************************************************/
/* STATIC                         start_enum                                 */
/*****************************************************************************/

static kdd_primitive *
  start_enum(char *string, kdd_primitive *primitives,
             kdd_file *file, int line_number)
  /* The supplied string contains the name of a new `enum' declaration,
     followed by the opening `{' character of the declaration and possibly
     further text.  The function creates a new primitive data type with this
     name, assigning it an integer-valued Java/C#/VB binding, and returning a
     pointer to the newly created primitive. */
{
  string = skip_white(string);
  char *to = string + strlen(string);
  while ((to > string) && (to[-1] == ' '))
    to--;
  *to = '\0';
  kdd_primitive *result = primitives->new_alias(string,"int");
  if (result == NULL)
    { kdu_error e;
      e << "Likely syntax/semantic error encountered in enum declaration at "
        << line_number << " in file, \"" << file->name << "\"\n."
        << "\tThe name of the enumerated type appears to be identical to "
           "that of an existing enumerated type or primitive data type.";
    }
  return result;
}

/*****************************************************************************/
/* STATIC                      process_enum_body                             */
/*****************************************************************************/

static void
  process_enum_body(kdd_primitive *type, char *string,
                    kdd_constant* &constants, kdd_file *file, int line_number)
{
  line_number = line_number;
  do {
      string = skip_white(string);
      char *next = strchr(string,',');
      if (next != NULL)
        { *next = '\0'; next++; }
      char *name = string;
      string = strchr(string,'=');
      if (string != NULL)
        {
          *string = '\0';
          string = skip_white(string+1);
          char *tmp=next;
          while ((tmp > string) && (tmp[-1] == ' '))
            { tmp[-1] = '\0'; tmp--; }
          if ((*string != '\0') && (*name != '\0'))
            {
              kdd_constant *last=NULL, *scan;
              for (scan=constants; scan != NULL; last=scan, scan=scan->next)
                if (strcmp(scan->name,name) == 0)
                  { kdu_error e;
                    e << "Likely syntax error encountered at line "
                      << line_number << " in file, \"" << file->name
                      << "\".\n\tEnum declaration has an element with the "
                         "same name as an existing constant.";
                  }
              kdd_constant *cls = new kdd_constant(name,string,type);
              if (last == NULL)
                constants = cls;
              else
                last->next = cls;
            }
        }
      string = next;
    } while (string != NULL);
}

/*****************************************************************************/
/* STATIC                        check_comment                               */
/*****************************************************************************/

static int
  check_comment(char * &from, char *to, char *filename, int line_number)
  /* If the supplied string contains a comment tag such as "[SYNOPSIS]", the
     function returns the corresponding macro (e.g., KDD_TAG_SYNOPSIS') and
     modifies `from' to point immediately after the tag.  Otherwise, the
     function returns 0 and does not modify `from'.  The function also takes
     the opportunity to check for likely syntax errors generating appropriate
     error messages as necessary. */
{
  char *scan = from;
  while (((scan = strchr(scan,'[')) != NULL) && ((scan+1) < to))
    {
      const char *synopsis_string = "SYNOPSIS]";
      const char *returns_string =  "RETURNS]";
      const char *arg_string =      "ARG:";
      const char *par_string =      "//]";
      const char *bullet_string =   ">>]";
      const char *bind_string =     "BIND:";
      int synopsis_errors = (int) strlen(synopsis_string);
      int returns_errors  = (int) strlen(returns_string);
      int arg_errors      = (int) strlen(arg_string);
      int par_errors      = (int) strlen(par_string);
      int bullet_errors   = (int) strlen(bullet_string);
      int bind_errors     = (int) strlen(bind_string);

      scan++;
      for (int i=0; (i < (to-scan)) && (i < 9); i++)
        {
          if ((i < (int) strlen(synopsis_string)) &&
              (scan[i] == synopsis_string[i]))
            synopsis_errors--;
          if ((i < (int) strlen(returns_string)) &&
              (scan[i] == returns_string[i]))
            returns_errors--;
          if ((i < (int) strlen(arg_string)) &&
              (scan[i] == arg_string[i]))
            arg_errors--;
          if ((i < (int) strlen(par_string)) &&
              (scan[i] == par_string[i]))
            par_errors--;
          if ((i < (int) strlen(bullet_string)) &&
              (scan[i] == bullet_string[i]))
            bullet_errors--;
          if ((i < (int) strlen(bind_string)) &&
              (scan[i] == bind_string[i]))
            bind_errors--;
        }
      if (synopsis_errors == 0)
        { from = scan+strlen(synopsis_string); return KDD_TAG_SYNOPSIS; }
      if (returns_errors == 0)
        { from = scan+strlen(returns_string); return KDD_TAG_RETURNS; }
      if (arg_errors == 0)
        { from = scan+strlen(arg_string); return KDD_TAG_ARG; }
      if (par_errors == 0)
        { from = scan+strlen(par_string); return KDD_TAG_PAR; }
      if (bullet_errors == 0)
        { from = scan+strlen(par_string); return KDD_TAG_BULLET; }
      if (bind_errors == 0)
        {
          scan += strlen(bind_string);
          while ((*scan == ' ') && (scan < to))
            scan++;
          char *delim = strchr(scan,']');
          if (delim == NULL)
            scan += strlen(scan); // Forces error code below
          else
            *delim = '\0';
          if (strcmp(scan,"interface") == 0)
            { from = delim+1; return KDD_BIND_IFC; }
          else if (strcmp(scan,"reference") == 0)
            { from = delim+1; return KDD_BIND_REF; }
          else if (strcmp(scan,"copy") == 0)
            { from = delim+1; return KDD_BIND_COPY; }
          else if (strcmp(scan,"donate") == 0)
            { from = delim+1; return KDD_BIND_DONATE; }
          else if (strcmp(scan,"callback") == 0)
            { from = delim+1; return KDD_BIND_CALLBACK; }
          else if (strcmp(scan,"no-java") == 0)
            { from = delim+1; return KDD_BIND_NO_BIND; }
          else if (strcmp(scan,"no-bind") == 0)
            { from = delim+1; return KDD_BIND_NO_BIND; }
          else
            { kdu_error e;
              e << "Likely syntax error encountered in comment string at line "
                << line_number << " in file, \"" << filename << "\"\n."
                << "\tThe \"[BIND: ...]\" tag must take one of the forms, "
                   "\"[BIND: interface]\", \"[BIND: reference]\", "
                   "\"[BIND: copy]\", \"[BIND: donate]\", "
                   "\"[BIND: callback] or [BIND: no-java]\".";
            }
        }

      if ((synopsis_errors <= (int)(strlen(synopsis_string)/2)) ||
          (returns_errors <= (int)(strlen(returns_string)/2)) ||
          (arg_errors <= (int)(strlen(arg_string)/2)) ||
          (par_errors <= (int)(strlen(par_string)/2)) ||
          (bullet_errors <= (int)(strlen(bullet_string)/2)) ||
          (bind_errors <= (int)(strlen(bind_string)/2)))
        { kdu_error e;
          e << "Likely syntax error encountered in comment string at line "
            << line_number << " in file, \"" << filename << "\"\n."
            << "\tLooks like you have misspelled one of the tags "
               "\"[SYNOPSIS]\", \"[RETURNS]\", \"[ARG: ...]\", "
               "\"[BIND: ...]\", \"[>>]\" or \"[//]\".";
        }
    }

  scan = from;
  while (((scan = strchr(scan,'`')) != NULL) && (scan < to))
    {
      for (scan++; scan < to; scan++)
        if ((*scan == '`') || (*scan == '\''))
          break;
      if (*scan != '\'')
        { kdu_error e;
          e << "Likely syntax error encountered in comment string at line "
            << line_number << " in file, \"" << filename << "\"\n."
            << "\tLooks like you have forgotten to terminate a quote "
               "which was opened using ` character.  Quotes should be closed "
               "on the same line in which they commence using the ' "
               "character.";
        }
    }
  return 0;
}

/*****************************************************************************/
/* STATIC                     process_comment_line                           */
/*****************************************************************************/

static void
  process_comment_line(char *from, char *to,
                       bool &in_synopsis, bool &in_arg_description,
                       bool &in_return_description, kdd_class *last_class,
                       kdd_func *last_func, kdd_variable * &last_variable,
                       char *filename, int line_number)
  /* This function looks for the special in-comment prompts, "[SYNOPSIS]",
     "[RETURNS]" and "[ARG: ...]" to adjust the current working notion of
     the particular description into which comments should be accumulated,
     if any.  There may be at most  one of these in-comment prompts on any
     given line and the prompts may not straddle multiple lines. */

{
  bool new_paragraph = false;
  bool new_bullet = false;
  int check_val = check_comment(from,to,filename,line_number);
  if ((check_val == KDD_BIND_IFC) || (check_val == KDD_BIND_REF) ||
      (check_val == KDD_BIND_COPY))
    {
      if ((last_class == NULL) || (last_class->binding != KDD_BIND_NONE))
        { kdu_error e;
          e << "Likely syntax error encountered in comment at line "
            << line_number << " of file, \"" << filename << "\".\n"
            << "\t\"[BIND: interface]\" or \"[BIND: reference]\" tag "
               "does not appear to be associated with any class/struct "
               "declaration, or the relevant class or structure has "
               "already been assigned a binding.";
        }
      last_class->binding = check_val;
    }
  else if (check_val == KDD_BIND_NO_BIND)
    {
      if ((last_func != NULL) && !in_arg_description)
        last_func->binding = KDD_BIND_NONE;
      else if ((last_func == NULL) && (last_class != NULL) &&
               (last_class->name != NULL))
        last_class->binding = KDD_BIND_NONE;
      else
        { kdu_error e;
          e << "Likely syntax error encountered in comment at line "
            << line_number << " of file, \"" << filename << "\".\n"
            << "\t\"[BIND: no-java]\" or \"[BIND: no-bind]\" tag found in "
               "comment which does not appear to be attached to a class or "
               "function.";
        }
    }
  else if ((check_val == KDD_BIND_DONATE) ||
           (check_val == KDD_BIND_CALLBACK))
    {
      if ((!in_arg_description) || (last_variable == NULL))
        { // Must be function donating its object
          if ((last_func == NULL) || (last_class == NULL) ||
              (last_class->name == NULL))
            { kdu_error e;
              e << "Likely syntax error encountered in comment at line "
                << line_number << " of file, \"" << filename << "\".\n"
                << "\t\"[BIND: donate]\" or \"[BIND: callback]\" "
                   "tag found in comment which does not appear to be "
                   "attached to any member function, or function argument.";
            }
          if (last_func->binding != KDD_BIND_TRANSIENT)
            { kdu_error e;
              e << "Likely syntax error encountered in comment at line "
                << line_number << " of file, \"" << filename << "\".\n"
                << "\t\"[BIND: donate]\", \"[BIND: callback]\" "
                   "tag found in comment which appears to be attached "
                   "to a function which has already been assigned a "
                   "special binding.";
            }
          last_func->binding = check_val;
        }
      else if (check_val == KDD_BIND_DONATE)
        {
          if (last_variable->type.binding != KDD_BIND_TRANSIENT)
            { kdu_error e;
              e << "Likely syntax error encountered in comment at line "
                << line_number << " of file, \"" << filename << "\".\n"
                << "\t\"[BIND: donate]\" tag appears inside a function "
               "argument description, whose argument has already been "
               "assigned a special binding.";
            }
          last_variable->type.binding = check_val;
        }
      else
        { kdu_error e;
          e << "Likely syntax error encountered in comment at line "
            << line_number << " of file, \"" << filename << "\".\n"
            << "\t[BIND: callback]\" tag appears within a function argument "
               "description; it may only be applied to functions as a whole.";
        }
    }
  else if (check_val == KDD_TAG_SYNOPSIS)
    {
      in_synopsis = true;
      in_return_description = false;
      if (in_arg_description)
        { in_arg_description = false; last_variable = NULL; }
    }
  else if (check_val == KDD_TAG_RETURNS)
    {
      if (last_func == NULL)
        { kdu_error e;
          e << "Likely syntax error encountered in comment at line "
            << line_number << " of file, \"" << filename << "\".\n"
            << "\t\"[RETURNS]\" tag found in comment which does not appear "
               "to be attached to any function.";
        }
      in_return_description = true;
      if (in_arg_description)
        { in_arg_description = false; last_variable = NULL; }
    }
  else if (check_val == KDD_TAG_ARG)
    {
      if (last_func == NULL)
        { kdu_error e;
          e << "Likely syntax error encountered in comment at line "
            << line_number << " of file, \"" << filename << "\".\n"
            << "\t\"[ARG: ...]\" tag found in comment which does not appear "
               "to be attached to any function.";
        }
      from = skip_white(from);
      char *tmp = strchr(from,']');
      if (tmp == NULL)
        { kdu_error e;
          e << "Malformed argument description tag found in comments "
               "attached to function declaration.  Such prompts should "
               "have the form \"[ARG: <name of argument>]\".\n"
            << "\tProblem occurred at line " << line_number << " of file \""
            << filename << "\".";
        }
      *tmp = '\0';
      in_return_description = false;
      for (last_variable=last_func->args;
           last_variable != NULL; last_variable=last_variable->next)
        if (strcmp(last_variable->name,from) == 0)
          break;
      if (last_variable == NULL)
        { kdu_error e;
          e << "Argument description prompt, \"" << from << "\", found in "
               "comments attached to function declaration appears to refer "
               "to a non-existent function argument.  Such prompts should "
               "have the form \"[ARG: <name of argument>]\".\n"
            << "\tProblem occurred at line "<< line_number << " of file \""
            << filename << "\".";
        }
      in_arg_description = true;
      from = skip_white(tmp+1);
    }
  else if (check_val == KDD_TAG_PAR)
    new_paragraph = true;
  else if (check_val == KDD_TAG_BULLET)
    new_bullet = true;
  if (from > to)
    from = to;

  // Now we are ready to record the comments in the most appopriate container.
  if (!in_synopsis)
    return;
  if (in_return_description && (last_func != NULL))
    {
      last_func->has_descriptions = true;
      last_func->owner->has_descriptions = true;
      last_func->return_description.append(from,to,new_paragraph,new_bullet);
    }
  else if (in_arg_description && (last_func!=NULL) && (last_variable!=NULL))
    {
      last_func->has_descriptions = true;
      last_func->owner->has_descriptions = true;
      last_variable->description.append(from,to,new_paragraph,new_bullet);
    }
  else if (last_func != NULL)
    {
      last_func->has_descriptions = true;
      last_func->owner->has_descriptions = true;
      last_func->description.append(from,to,new_paragraph,new_bullet);
    }
  else if (last_variable != NULL)
    {
      if (last_variable->owner != NULL)
        last_variable->owner->has_descriptions = true;
      last_variable->description.append(from,to,new_paragraph,new_bullet);
    }
  else if (last_class != NULL)
    {
      last_class->has_descriptions = true;
      last_class->description.append(from,to,new_paragraph,new_bullet);
    }
}

/*****************************************************************************/
/* STATIC                         parse_macro                                */
/*****************************************************************************/

static void
  parse_macro(char *text, kdd_constant * &constants,
              kdd_primitive *primitives)
  /* Parses the text following a #define directive to see if it defines a
     constant, with a well-defined primitive data type.  If so, the new
     constant is appended to the list headed by `constants'. */
{
  char *name = skip_white(text);
  for (; *text != ' '; text++)
    if (*text == '\0')
      return;
  *text = '\0';
  text = skip_white(text+1);

  // See if the macro takes arguments or not
  if (strchr(name,'(') != NULL)
    return;

  // Check to see if name already exists
  kdd_constant *last_cst=NULL, *cst;
  for (cst=constants; cst != NULL; last_cst=cst, cst=cst->next)
    if (strcmp(cst->name,name) == 0)
      return; // Constant already defined
  if (text[0] == '\"')
    {
      for (; primitives != NULL; primitives=primitives->next)
        if (strcmp(primitives->name,"const char *") == 0)
          break;
      if (primitives == NULL)
        return;
      char *end = text+1;
      for (; *end != '\"'; end++)
        if (*end == '\0')
          return; // Unterminated string.
        else if ((*end == '\\') && (end[1] != '\0'))
          end++; // Skip over escape character
      end[1] = '\0';
      cst = new kdd_constant(name,text,primitives);
    }
  else if (text[0] == '(')
    {
      text = skip_white(text+1);
      if (text[0] != '(')
        return;
      text = skip_white(text+1);
      char *end = text;
      for (; *end != ')'; end++)
        if ((*end == '\0') || (*end == '('))
          return;
      char *tmp = end;
      while ((tmp > text) && (tmp[-1] == ' '))
        tmp--;
      *tmp = '\0';
      for (; primitives != NULL; primitives=primitives->next)
        if (strcmp(primitives->name,text) == 0)
          break;
      if (primitives == NULL)
        return;
      text = skip_white(end+1);
      int nesting = 0;
      for (end=text; *end != '\0'; end++)
        if (*end == '(')
          nesting++;
        else if (*end == ')')
          {
            if (nesting == 0)
              break;
            nesting--;
          }
      if (*end == ')')
        {
          *end = '\0';
          cst = new kdd_constant(name,text,primitives);
        }
    }
  else
    return;
  if (last_cst == NULL)
    constants = cst;
  else
    last_cst->next = cst;
}

/*****************************************************************************/
/* STATIC                         process_file                               */
/*****************************************************************************/

static kdd_file *
  process_file(char *filename, kdd_primitive *primitives, kdd_class *classes)
  /* Processes the contents of the file, returning a pointer to the newly
     created `kdd_file' object. */
{
  ifstream stream;
  char line[1024]; // Can't handle longer lines in the file.
  bool in_comment = false; // Terminates with a '*/'
  bool in_directive = false; // True if we are inside a compiler directive
  bool in_synopsis = false; // In comment giving class/function synopsis
  bool in_arg_description = false; // In comment describing function arg
  bool in_return_description = false; // In comment describing function return
  int implementation_nesting = 0; // Non-zero means inside an implementation
  int class_nesting = 0; // Non-zero means inside a class/struct definition.
  int conditional_nesting = 0; // Nesting level for #if/#endif bracketed code
  bool in_public = false; // If in public area of a top level class/struct
  kdd_string statement; // Currently active statement.
  kdd_string directive; // Currently active compiler directive text.
  kdd_class *last_class = NULL;
  kdd_func *last_func = NULL;
  kdd_variable *last_variable = NULL;
  kdd_primitive *active_enum = NULL; // Points to the primitive type created by
                                     // any active enum declaration.

  stream.open(filename);
  if (!stream)
    { kdu_error e; e << "Cannot open file, \"" << filename << "\"!"; }
  kdd_file *file = new kdd_file(filename);
  int line_number = 0;
  while (stream.getline(line,1024).gcount() > 0)
    {
      line_number++;
      char *to, *from, *end = line + strlen(line);
      bool start_of_line = true;

      for (from=skip_white(line);
           from < end;
           from=skip_white(to), start_of_line=false)
        {
          if (in_comment)
            {
              to = strstr(from,"*/");
              if (to == NULL)
                to = end;
              if ((class_nesting == 0) || (last_func == NULL) || in_public)
                process_comment_line(from,to,in_synopsis,in_arg_description,
                                     in_return_description,last_class,
                                     last_func,last_variable,filename,
                                     line_number);
              if (to < end)
                {
                  assert((to[0] == '*') && (to[1] == '/'));
                  to += 2;
                  in_comment = false;
                }
            }
          else if (in_directive || (start_of_line && (*from == '#')))
            { // In compiler directive line
              if (!in_directive)
                directive.reset();
              to = strrchr(from,'\\');
              if ((to != NULL) && (skip_white(to+1) == end))
                {
                  directive.append(from,to);
                  in_directive = true;
                  to = end;
                  continue;
                }
              to = end;
              directive.append(from,to);
              in_directive = false;
              char *directive_text = skip_white(directive.get()+1);
              if (strncmp(directive_text,"if",2) == 0)
                conditional_nesting++;
              else if (strncmp(directive_text,"endif",5) == 0)
                {
                  if (conditional_nesting == 0)
                    { kdu_error e;  e << "Unexpected \"#endif\" directive "
                      "encountered on line " << line_number << " of file \""
                      << filename << "\".  I do not believe we are currently "
                      "inside a conditional compilation section.";
                    }
                  conditional_nesting--;
                }
              else if ((conditional_nesting == 1) &&
                       (strncmp(directive_text,"define ",7) == 0))
                parse_macro(directive_text+7,classes->constants,primitives);
            }
          else if (implementation_nesting > 0)
            { // Look for a '}', a '{', or the start of a comment.
              for (to=from; to < end; to++)
                {
                  if (*to == '}')
                    { // End of the implementation section.
                      to++;
                      implementation_nesting--;
                      break;
                    }
                  else if (*to == '{')
                    {
                      to++;
                      implementation_nesting++;
                      break;
                    }
                  else if ((to[0] == '/') && (to[1] == '*'))
                    { // Entering a comment
                      to += 2;
                      in_comment = true;
                      in_synopsis = false;
                      in_arg_description = false;
                      in_return_description = false;
                      break;
                    }
                  else if ((to[0] == '/') && (to[1] == '/'))
                    { // Entering a single-line comment
                      to = end;
                      break;
                    }
                }
            }
          else if (*from == '}')
            {
              if (active_enum != NULL)
                { // Enum declaration terminator
                  process_enum_body(active_enum,statement.get(),
                                    classes->constants,file,line_number);
                  active_enum = NULL;
                }
              else
                { // Class/Struct/Union declaration terminator
                  if (class_nesting == 0)
                    { kdu_error e; e << "Unexpected '}' encountered on line "
                      << line_number << " of file \"" << filename << "\".  "
                      "I do not believe we are currently inside a class or "
                      "structure definition, or a function implementation."; }
                  class_nesting--;
                  if (class_nesting == 0)
                    {
                      last_func = NULL;
                      last_variable = NULL;
                      in_public = false;
                    }
                }
              statement.reset();
              to = from+1;
            }
          else if ((*from == '{') || (*from == ';'))
            { // Statement terminator
              char *string = skip_white(statement.get());
              bool visibility_info;
              do { // Strip `public:', `private:', `protected:' modifiers
                  visibility_info = false;
                  if (strncmp(string,"public:",7) == 0)
                    {
                      string = skip_white(string+7);
                      visibility_info = true;
                      if (class_nesting == 1)
                        in_public = true;
                    }
                  else if (strncmp(string,"private:",8) == 0)
                    {
                      string = skip_white(string+8);
                      visibility_info = true;
                      if (class_nesting == 1)
                        in_public = false;
                    }
                  else if (strncmp(string,"protected:",10) == 0)
                    {
                      string = skip_white(string+10);
                      visibility_info = true;
                      if (class_nesting == 1)
                        in_public = false;
                    }
                } while (visibility_info);

              if ((strncmp(string,"class ",6) == 0) && (*from == '{'))
                { // Starting a new class
                  class_nesting++;
                  last_func = NULL;
                  last_variable = NULL;
                  if (class_nesting == 1)
                    {
                      in_public = false;
                      last_class =
                        start_class(string+6,classes,file,line_number);
                    }
                }
              else if ((strncmp(string,"struct ",7) == 0) && (*from == '{'))
                {
                  class_nesting++;
                  last_func = NULL;
                  last_variable = NULL;
                  if (class_nesting == 1)
                    {
                      in_public = true;
                      last_class =
                        start_class(string+7,classes,file,line_number);
                      if (last_class != NULL)
                        last_class->is_struct = true;
                    }
                }
              else if ((strncmp(string,"union ",6) == 0) && (*from == '{'))
                {
                  class_nesting++;
                  last_func = NULL;
                  last_variable = NULL;
                  if (class_nesting == 1)
                    {
                      in_public = true;
                      last_class =
                        start_class(string+6,classes,file,line_number);
                      if (last_class != NULL)
                        last_class->is_union = true;
                    }
                }
              else if ((strncmp(string,"enum ",5) == 0) && (*from == '{') &&
                       (class_nesting == 0))
                {
                  last_func = NULL;
                  last_variable = NULL;
                  active_enum =
                    start_enum(string+5,primitives,file,line_number);
                  statement.reset();
                }
              else
                {
                  if (*from == '{')
                    implementation_nesting++;
                  if (((class_nesting == 0) ||
                       ((class_nesting == 1) && in_public)))
                    { // Public function or data member declaration
                      last_variable = NULL;
                      last_func = NULL;
                      if (strchr(string,'(') != NULL)
                        { // Could be a new function
                          if (class_nesting == 1)
                            last_func =
                              start_func(string,last_class,file,line_number);
                          else
                            {
                              assert(classes->name == NULL); // Globals "class"
                              last_func =
                                start_func(string,classes,file,line_number);
                            }
                        }
                      else if (*from == ';')
                        { // Could be a public data member
                          if (class_nesting == 1)
                            last_variable =
                              start_var(string,last_class->variables,
                                      last_class,file,line_number);
                          else
                            {
                              assert(classes->name == NULL);
                              last_variable =
                                start_var(string,classes->variables,classes,
                                          file,line_number);
                            }
                        }
                    }
                  else if ((class_nesting==1) && (strchr(string,'(') != NULL))
                    { // May be private or protected function
                      char *arg_end = strrchr(string,')');
                      if (arg_end != NULL)
                        {
                          if (strncmp(string,"virtual ",8) == 0)
                            {
                              arg_end = skip_white(arg_end+1);
                              if (*arg_end == '=')
                                {
                                  arg_end = skip_white(arg_end+1);
                                  if (*arg_end == '0')
                                    last_class->is_abstract = true;
                                }
                            }
                          if ((strchr(string,'~') != NULL) &&
                              (strchr(string,'~') < strchr(string,'(')))
                            last_class->destroy_private = true;
                        }
                    }
                }
              statement.reset();
              to = from+1;
            }
          else if ((from[0] == '/') && (from[1] == '/'))
            to = end; // Rest of line is a comment.
          else if ((from[0] == '/') && (from[1] == '*'))
            { // Entering a comment.
              to = from+2;
              in_comment = true;
              in_synopsis = false;
              in_arg_description = false;
              in_return_description = false;
            }
          else
            { // Append as much text as possible to the current statement.
              for (to=from; to < end; to++)
                if ((*to == '{') || (*to == ';') || (*to == '}') ||
                    ((*to == '/') && ((to[1] == '/') || (to[1] == '*'))))
                  break;
              statement.append(from,to);
            }
        }
    }
  stream.close();
  return file;
}

/*****************************************************************************/
/* STATIC                    resolve_inheritance                             */
/*****************************************************************************/

static void
  resolve_inheritance(kdd_class *classes)
  /* Resolves base class links and builds derived class lists. */
{
  for (kdd_class *csp=classes; csp != NULL; csp=csp->next)
    if (csp->base.exists())
      {
        csp->base.finalize(classes,NULL);
      }
}

/*****************************************************************************/
/* STATIC                       resolve_links                                */
/*****************************************************************************/

static void
  resolve_links(kdd_class *classes, kdd_primitive *primitives)
  /* Fills in the function override and overload links and resolves
     as many type names as possible into known classes and structures. */
{
  for (kdd_class *csp=classes; csp != NULL; csp=csp->next)
    {
      kdd_variable *var;
      for (kdd_func *func=csp->functions; func != NULL; func=func->next)
        {
          kdd_func *aux;

          if ((csp->name != NULL) && (strcmp(func->name,csp->name) == 0))
            func->is_constructor = true;
          else if ((func->name[0] == '~') && (csp->name != NULL) &&
                   (strcmp(func->name+1,csp->name) == 0))
            {
              func->is_destructor = true;
              csp->destroy_private = false;
            }
          func->return_type.finalize(classes,primitives);
          for (var=func->args; var != NULL; var=var->next)
            var->type.finalize(classes,primitives);

          // Grow overload rings as necessary.
          for (aux=func->next; aux != NULL; aux=aux->next)
            if (strcmp(aux->name,func->name) == 0)
              {
                if ((aux->overload_next = func->overload_next) == NULL)
                  aux->overload_next = func;
                func->overload_next = aux;
                if (func->overload_index == 0)
                  func->overload_index = 1;
                aux->overload_index = func->overload_index+1;
                break;
              }

          // Look for overrides
          for (kdd_class *base=csp->base.known_class;
               base != NULL; base=base->base.known_class)
            {
              for (aux=base->functions; aux != NULL; aux=aux->next)
                if (*func == *aux)
                  break;
              if (aux != NULL)
                { func->overrides = aux; break; }
            }
        }
      for (var=csp->variables; var != NULL; var=var->next)
        var->type.finalize(classes,primitives);
    }
}

/*****************************************************************************/
/* STATIC                  resolve_virtual_inheritance                       */
/*****************************************************************************/

static void
  resolve_virtual_inheritance(kdd_class *classes)
  /* Scans through all function members of classes, looking for overridden
     virtual functions which implicitly make the overriding function virtual.
     This is complicated by the fact that there may be long inheritance
     chains and we do not necessarily visit base classes first. */
{
  kdd_class *csp;
  kdd_func *func;
  for (csp=classes; csp != NULL; csp=csp->next)
    for (func=csp->functions; func != NULL; func=func->next)
      {
        kdd_func *scan=func;
        while ((scan->overrides != NULL) && !scan->is_virtual)
          scan=scan->overrides;
        if (scan->is_virtual)
          func->is_virtual = true;
        if (func->is_destructor && func->is_virtual)
          csp->destroy_virtual = true;
      }
}

/*****************************************************************************/
/* STATIC                         order_lists                                */
/*****************************************************************************/

static void
  order_lists(kdd_class *classes, kdd_file * &files)
  /* Organizes the classes in alphabetical order, with the globals class first;
     organizes the functions and variables in the globals class in
     alphabetical order. */
{
  kdd_class *cprev, *cscan;
  bool done;

  // Order classes.
  do {
      done = true;
      for (cprev=classes, cscan=classes->next;
           (cscan != NULL) && (cscan->next != NULL);
           cprev=cscan, cscan=cscan->next)
        {
          kdd_class *cnext = cscan->next;
          if (strcmp(cscan->name,cnext->name) > 0)
            { // Reverse order
              cprev->next = cnext;
              cscan->next = cnext->next;
              cnext->next = cscan;
              done = false;
            }
        }
    } while (!done);

  // Order functions and variables within global classes.
  for (cscan=classes; cscan != NULL; cscan=cscan->next)
    {
      // Order function members.
      kdd_func *fprev, *fscan;
      if (cscan->name == NULL)
        do {
          done = true;
          for (fprev=NULL, fscan=cscan->functions;
               (fscan != NULL) && (fscan->next != NULL);
               fprev=fscan, fscan=fscan->next)
            {
              kdd_func *fnext = fscan->next;
              if (strcmp(fscan->name,fnext->name) > 0)
                { // Reverse order
                  if (fprev == NULL)
                    cscan->functions = fnext;
                  else
                    fprev->next = fnext;
                  fscan->next = fnext->next;
                  fnext->next = fscan;
                  done = false;
                }
            }
        } while (!done);

      // Order data members.
      kdd_variable *vprev, *vscan;
      if (cscan->name == NULL)
        do {
          done = true;
          for (vprev=NULL, vscan=cscan->variables;
               (vscan != NULL) && (vscan->next != NULL);
               vprev=vscan, vscan=vscan->next)
            {
              kdd_variable *vnext = vscan->next;
              if (strcmp(vscan->name,vnext->name) > 0)
                { // Reverse order
                  if (vprev == NULL)
                    cscan->variables = vnext;
                  else
                    vprev->next = vnext;
                  vscan->next = vnext->next;
                  vnext->next = vscan;
                  done = false;
                }
            }
        } while (!done);

      // Assign the `prev' members of each function record.
      for (fprev=NULL, fscan=cscan->functions;
           fscan != NULL; fprev=fscan, fscan=fscan->next)
        fscan->prev = fprev;
    }

  // Order files
  kdd_file *fprev, *fscan;
  do {
      done = true;
      for (fprev=NULL, fscan=files;
           (fscan != NULL) && (fscan->next != NULL);
           fprev=fscan, fscan=fscan->next)
        {
          kdd_file *fnext = fscan->next;
          if (strcmp(fscan->pathname,fnext->pathname) > 0)
            { // Reverse order
              if (fprev == NULL)
                files = fnext;
              else
                fprev->next = fnext;
              fscan->next = fnext->next;
              fnext->next = fscan;
              done = false;
            }
        }
    } while (!done);
}

/*****************************************************************************/
/* STATIC                       search_for_link                              */
/*****************************************************************************/

static char *
  search_for_link(char *name, kdd_variable *args, kdd_func *funcs,
                  kdd_variable *vars, kdd_class *classes)
{
  char *aux = NULL, *delim = NULL, tchar='\0';

  // First separate any composite names into an initial name for matching
  // and an auxiliary string for refining the reference.
  if ((delim = strstr(name,"&rarr;")) != NULL)
    { tchar = *delim; *delim = '\0'; aux = delim + 6;
      funcs = NULL; classes = NULL; }
  else if ((delim = strstr(name,"::")) != NULL)
    { tchar = *delim; *delim = '\0'; aux = delim + 2;
      args = NULL; funcs = NULL; vars = NULL; }
  else if ((delim = strchr(name,'.')) != NULL)
    { tchar = *delim; *delim = '\0';; aux = delim + 1;
      funcs = NULL; classes = NULL; }

  // Search for `name'.
  kdd_variable *var = NULL;
  kdd_func *func = NULL;
  kdd_class *csp = NULL;
  kdd_constant *cnst = NULL;
  if ((var == NULL) && (func == NULL) && (csp == NULL) && (cnst == NULL))
    { // Look for a function argument
      for (var=args; var != NULL; var=var->next)
        if (strcmp(var->name,name) == 0)
          break;
    }
  if ((var == NULL) && (func == NULL) && (csp == NULL) && (cnst == NULL))
    { // Look for a member function
      for (func=funcs; func != NULL; func=func->next)
        if (strcmp(func->name,name) == 0)
          break;
      if ((func != NULL) && func->is_constructor)
        { csp = func->owner; func = NULL; }
    }
  if ((var == NULL) && (func == NULL) && (csp == NULL) && (cnst == NULL))
    { // Look for a member variable
      for (var=vars; var != NULL; var=var->next)
        if (strcmp(var->name,name) == 0)
          break;
    }
  if ((var == NULL) && (func == NULL) && (csp == NULL) && (cnst == NULL))
    { // Look for a class
      for (csp=classes; csp != NULL; csp=csp->next)
        if (csp->name == NULL)
          {
            for (func=csp->functions; func != NULL; func=func->next)
              if (strcmp(func->name,name) == 0)
                break;
            if (func != NULL)
              { csp = NULL; break; }
            for (var=csp->variables; var != NULL; var=var->next)
              if (strcmp(var->name,name) == 0)
                break;
            if (var != NULL)
              { csp = NULL; break; }
            for (cnst=csp->constants; cnst != NULL; cnst=cnst->next)
              if (strcmp(cnst->name,name) == 0)
                break;
            if (cnst != NULL)
              { csp = NULL; break; }
          }
        else if (strcmp(csp->name,name) == 0)
          break;
    }

  char *link = NULL;
  if (cnst != NULL)
    link = cnst->html_link;
  else if (var != NULL)
    {
      if (aux == NULL)
        link = var->html_link;
      else if (((csp=var->type.known_class)!=NULL) && (csp->html_name!=NULL))
        {
          for (func=csp->functions; func != NULL; func=func->next)
            if (strcmp(func->name,aux) == 0)
              break;
          if (func != NULL)
            link = func->html_name;
          else
            {
              for (var=csp->variables; var != NULL; var=var->next)
                if (strcmp(var->name,aux) == 0)
                  break;
              if (var != NULL)
                link = var->html_link;
            }
        }
    }
  else if (csp != NULL)
    {
      if (aux == NULL)
        link = csp->html_name;
      else
        {
          for (func=csp->functions; func != NULL; func=func->next)
            if (strcmp(func->name,aux) == 0)
              break;
          if (func != NULL)
            link = func->html_name;
          else
            {
              for (var=csp->variables; var != NULL; var=var->next)
                if (strcmp(var->name,aux) == 0)
                  break;
              if (var != NULL)
                link = var->html_link;
            }
        }
    }
  else if (func != NULL)
    link = func->html_name;
  if (delim != NULL)
    *delim = tchar;
  return link;
}

/*****************************************************************************/
/* STATIC                     build_function_list                            */
/*****************************************************************************/

static kdd_index *
  build_function_list(kdd_class *csp)
{
  kdd_index *head = NULL;
  kdd_index *entry, *scan, *prev;

  for (; csp != NULL; csp=csp->next)
    for (kdd_func *func=csp->functions; func != NULL; func=func->next)
      if ((func->html_name != NULL) && (func->overload_index <= 1))
        {
          entry = new kdd_index;
          copy_string(entry->link,func->html_name);
          int name_chars = (int) strlen(func->name);
          if (csp->name != NULL)
            name_chars += (int) strlen(csp->name);
          name_chars += 16; // Much more than sufficient extra characters
          entry->name = new char[name_chars];
          strcpy(entry->name,func->name);
          if (csp->name != NULL)
            {
              strcat(entry->name,"&nbsp;(");
              strcat(entry->name,csp->name);
              strcat(entry->name,"::)");
            }
          else
            strcat(entry->name,"&nbsp;(::)");

          // Insert into the index list in alphabetical order.
          char *test_name, *new_name = entry->name;
          if (*new_name == '~')
            new_name++;
          for (prev=NULL, scan=head; scan != NULL; prev=scan, scan=scan->next)
            {
              test_name = scan->name;
              if (*test_name == '~')
                test_name++;
              if (strcmp(test_name,new_name) > 0)
                break;
            }
          entry->next = scan;
          if (prev == NULL)
            head = entry;
          else
            prev->next = entry;
        }
  return head;
}

/*****************************************************************************/
/* STATIC                       generate_indices                             */
/*****************************************************************************/

static void
  generate_indices(kdd_class *classes, kdd_index *extras, kdd_file *files,
                   char *directory)
{
  ofstream out;
  char *path = new char[40+strlen(directory)]; // More than enough
  kdd_class *csp;

  // Generate the class list
  strcpy(path,directory); strcat(path,"/class_index.html");
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE>Kakadu Hyper-Doc (Classes)</TITLE>\n";
  out << "<STYLE TYPE=\"text/css\">\n<!--\n"
         "  A {text-decoration: none}\n"
         "-->\n"
         "</STYLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#FFF491\">\n";
  out << "<H3>Classes</H3>\n";
  out << "<P>\n";
  bool first_line = true;
  for (csp=classes; csp != NULL; csp=csp->next)
    if (csp->html_name != NULL)
      {
        if (!first_line)
          out << "<BR>\n";
        out << "<A HREF=\"" << csp->html_name << "\" TARGET=\"active\">";
        if (csp->name == NULL)
          out << "[Globals]";
        else
          out << csp->name;
        out << "</A>";
        first_line = false;
      }
  out << "</P>\n";
  out << "</BODY>\n</HTML>\n";
  out.close();

  // Generate the function list
  kdd_index *entry, *func_index = build_function_list(classes);
  strcpy(path,directory); strcat(path,"/function_index.html");
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE>Kakadu Hyper-Doc (Functions)</TITLE>\n";
  out << "<STYLE TYPE=\"text/css\">\n<!--\n"
         "  A {text-decoration: none}\n"
         "-->\n"
         "</STYLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#B4DCB4\">\n";
  out << "<H3>Functions</H3>\n";
  out << "<P>\n";
  first_line = true;
  while ((entry=func_index) != NULL)
    {
      func_index = entry->next;
      if (!first_line)
        out << "<BR>\n";
      out << "<A HREF=\"" << entry->link << "\" TARGET=\"active\">";
      out << entry->name;
      out << "</A>";
      first_line = false;
      delete entry;
    }
  out << "</P>\n";
  out << "</BODY>\n</HTML>\n";
  out.close();

  // Generate the files list
  strcpy(path,directory); strcat(path,"/file_index.html");
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE>Kakadu Hyper-Doc (Files)</TITLE>\n";
  out << "<STYLE TYPE=\"text/css\">\n<!--\n"
         "  A {text-decoration: none}\n"
         "-->\n"
         "</STYLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#CCECFF\">\n";
  out << "<H3>Public Headers</H3>\n";
  out << "<P>\n";
  first_line = true;
  for (; files != NULL; files=files->next)
    {
      if (!first_line)
        out << "<BR>\n";
      out << "<A HREF=\"" << files->html_name << "\" TARGET=\"active\">";
      out << files->pathname;
      out << "</A>";
      first_line = false;
    }
  out << "</P>\n";
  out << "</BODY>\n</HTML>\n";
  out.close();

  // Generate "blank.html"
  strcpy(path,directory); strcat(path,"/blank.html");
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE>Kakadu Hyper-Doc (Blank)</TITLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#FFFFFF\">\n";
  out << "<P>Make a selection from the list on the left.</P>\n";
  out << "</BODY>\n</HTML>\n";
  out.close();

  // Generate "nav.html"
  strcpy(path,"nav.html"); // Note: this file not written inside `directory'.
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE>Kakadu Hyper-Doc (Navigate)</TITLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000FF\" BGCOLOR=\"#C0C0C0\">\n";
  out << "<DIV ALIGN=\"CENTER\">\n";
  out << "<P><TABLE BORDER CELLSPACING=1>\n"
         "  <TR><TD BGCOLOR=\"#FFF491\"><STRONG>\n"
         "  <A HREF=\"" << directory << "/class_index.html\""
         "  TARGET=\"lists\">Classes</A>\n"
         "  </STRONG></TD></TR></TABLE></P>\n";
  out << "<P><TABLE BORDER CELLSPACING=1>\n"
         "  <TR><TD BGCOLOR=\"#B4DCB4\"><STRONG>\n"
         "  <A HREF=\"" << directory << "/function_index.html\""
         "  TARGET=\"lists\">Functions</A>\n"
         "  </STRONG></TD></TR></TABLE></P>\n";
  out << "<P><TABLE BORDER CELLSPACING=1>\n"
         "  <TR><TD BGCOLOR=\"#CCECFF\"><STRONG>\n"
         "  <A HREF=\"" << directory << "/file_index.html\""
         "  TARGET=\"lists\">Files</A>\n"
         "  </STRONG></TD></TR></TABLE></P>\n";
  out << "<P ALIGN=\"CENTER\"><HR></P>"; // Draws a horizontal rule
  for (; extras != NULL; extras=extras->next)
    out << "<P><A HREF=\"" << extras->link << "\" TARGET=\"active\">"
        << extras->name << "</A></P>\n";
  out << "</DIV>\n";
  out << "</BODY>\n</HTML>\n";
  out.close();

  // Generate "index.html"
  strcpy(path,"index.html"); // Note: this file not written inside `directory'.
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE>Kakadu Hyper-Doc</TITLE>\n";
  out << "</HEAD>\n";
  out << "<FRAMESET COLS=\"15%,25%,*\">\n";
  out << "<FRAME NAME=\"navigate\" SRC=\"nav.html\">\n";
  out << "<FRAME NAME=\"lists\" SRC=\""<<directory<<"/class_index.html\">\n";
  out << "<FRAME NAME=\"active\" SRC=\"" << directory << "/blank.html\">\n";
  out << "</FRAMESET>\n</HTML>\n";
  out.close();
  delete[] path;
}

/*****************************************************************************/
/* STATIC              substitute_illegal_html_name_chars                    */
/*****************************************************************************/

static void
  substitute_illegal_html_name_chars(char *string)
  /* This function parses the supplied `string', which is the name of a
     file in which HTML will be stored.  If the file name contains any
     illegal characters, they are substituted here. */
{
  char *cp;
  for (cp=string; *cp != '\0'; cp++)
    if (*cp == '>')
      *cp = 'G';
    else if (*cp == '<')
      *cp = 'L';
    else if (*cp == '|')
      *cp = 'B';
    else if (*cp == '&')
      *cp = 'A';
}

/*****************************************************************************/
/* STATIC                       create_directory                             */
/*****************************************************************************/

static bool
  create_directory(char *path)
{
  cout << "Trying to create directory \"" << path << "\"\n";
  char *command = new char[strlen(path)+80];
  sprintf(command,"mkdir \"%s\"",path);
  int result = system(command);
  delete[] command;
  if (result == 0)
    {
      cout << "Created directory \"" << path << "\"\n";
      return true;
    }
  char *cp = path+strlen(path)-1;
  while ((*cp != '\0') && (*cp != '/') && (*cp != '\\'))
    cp--;
  if (*cp == '\0')
    return false;
  char sep = *cp;
  *cp = '\0';
  bool success = create_directory(path);
  *cp = sep;
  if (success)
    { // Try again to build the directory
      cout << "Trying again to create directory \"" << path << "\"\n";
      char *command = new char[strlen(path)+80];
      sprintf(command,"mkdir \"%s\"",path);
      int result = system(command);
      delete[] command;
      if (result == 0)
        {
          cout << "Created directory \"" << path << "\"\n";
          return true;
        }
    }
  return false;
}


/* ========================================================================= */
/*                             kdd_bind_specific                             */
/* ========================================================================= */

/*****************************************************************************/
/*                    kdd_bind_specific::kdd_bind_specific                   */
/*****************************************************************************/

kdd_bind_specific::kdd_bind_specific(const char *string)
{
  next = NULL;
  class_name = func_name = NULL;
  full_name = new char[strlen(string)+1];
  strcpy(full_name,string);
  const char *sep = strstr(string,"::");
  if (sep == NULL)
    { // Must be a class name
      class_name = new char[strlen(string)+1];
      strcpy(class_name,string);
      return;
    }

  // If we get here, we have a reference either to a global function or a
  // class member function.
  if (sep > string)
    { // Must be member function
      class_name = new char[(sep-string)+1];
      strncpy(class_name,string,(size_t)(sep-string));
    }
  string = sep+2;
  func_name = new char[strlen(string)+1];
  strcpy(func_name,string);
}

/*****************************************************************************/
/*                         kdd_bind_specific::apply                          */
/*****************************************************************************/

bool
  kdd_bind_specific::apply(kdd_class *classes)
{
  kdd_class *csp;
  for (csp=classes; csp != NULL; csp=csp->next)
    if ((csp->name == NULL) && (this->class_name == NULL))
      break;
    else if ((csp->name != NULL) && (this->class_name != NULL) &&
             (strcmp(csp->name,this->class_name) == 0))
      break;
  if (csp == NULL)
    return false;

  kdd_func *func;
  if (this->func_name != NULL)
    {
      for (func=csp->functions; func != NULL; func=func->next)
        if (strcmp(func->name,this->func_name) == 0)
          break;
      if (func == NULL)
        return false;
      func->bind_specific = true;
    }
  for (; csp != NULL; csp=csp->base.known_class)
    { // Walk back through the class hierarchy, specifying that we want
      // to bind the classes and identifying those functions which need
      // binding.
      csp->bind_specific = true;
      for (func=csp->functions; func != NULL; func=func->next)
        if (this->func_name == NULL)
          func->bind_specific = true; // Bind all functions in class
        else if (func->is_destructor)
          func->bind_specific = true; // Must bind the destructor
    }
  return true;
}


/* ========================================================================= */
/*                                 kdd_string                                */
/* ========================================================================= */

/*****************************************************************************/
/*                             kdd_string::append                            */
/*****************************************************************************/

void
  kdd_string::append(char *from, char *to,
                     bool new_paragraph, bool new_bullet)
{
  if (from > to)
    from = to;
  int new_chars = (int)(to-from) + 1; // Make room for space or new line.
  for (char *scan=from; scan < to; scan++)
    if (*scan == '<')
      new_chars += 3; // Add enough space to convert to "&lt;"
    else if (*scan == '>')
      new_chars += 3; // Add enough space to convert to "&gt;"
    else if (*scan == '-')
      new_chars += 5; // Add enough space to convert "--" to "&mdash"

  if ((buffer_next+new_chars) > buffer_lim)
    { // Augment buffer
      int buffer_len = (int)(buffer_next-buffer_start) + new_chars + 32;
      char *new_buf = new char[buffer_len];
      memcpy(new_buf,buffer_start,(buffer_next-buffer_start));
      buffer_next += (new_buf-buffer_start);
      delete[] buffer_start;
      buffer_start = new_buf;
      buffer_lim = buffer_start+buffer_len-1;
    }
  bool have_space = true;
  for (; from < to; from++)
    if ((*from==' ') || (*from=='\t') || (*from=='\r') || (*from=='\n'))
      have_space = true;
    else
      {
        if (have_space)
          {
            if (new_bullet)
              *(buffer_next++) = '\t';
            else if (buffer_next > buffer_start)
              *(buffer_next++) = (new_paragraph)?'\n':' ';
            new_paragraph = false;
            new_bullet = false;
            have_space = false;
          }
        if ((from[0] == '-') && (from[1] == '>') && ((from+1) < to))
          { strcpy(buffer_next,"&rarr;"); buffer_next += 6; from++; }
        else if ((from[0] == '-') && (from[1] == '-') && ((from+1) < to))
          { // Print a long dash and skip over any extra dashes.
            strcpy(buffer_next,"&mdash;"); buffer_next += 7; from++;
            while ((from[1] == '-') && ((from+1) < to))
              from++;
          }
        else if (*from == '<')
          { strcpy(buffer_next,"&lt;"); buffer_next += 4; }
        else if (*from == '>')
          { strcpy(buffer_next,"&gt;"); buffer_next += 4; }
        else
          *(buffer_next++) = *from;
      }
  if (new_paragraph)
    *(buffer_next++) = '\n'; // Never got around to using the new-line above.
  *buffer_next = '\0';
}

/*****************************************************************************/
/*                          kdd_string::generate_html                        */
/*****************************************************************************/

void
  kdd_string::generate_html(ofstream &out, kdd_variable *reference_args,
                            kdd_func *reference_funcs,
                            kdd_variable *reference_vars,
                            kdd_class *reference_classes)
{
  if (buffer_next == buffer_start)
    return;
  bool in_list = false;
  out << "<P>\n";
  char *from = buffer_start;
  while (from < buffer_next)
    {
      char *to = from;
      while ((to < buffer_next) && (*to != '`') && (*to != '\t') &&
             (*to != '\n') && (((to-from) < 50) || (*to != ' ')))
        to++;
      if (to > from)
        {
          out.write(from,(int)(to-from));
          out << "\n";
        }
      from = to;
      if (*from == '\0')
        continue;
      if (*from == ' ')
        { from++; continue; }
      if (*from == '\n')
        { // New line, also terminates any existing list environment.
          from++;
          if (in_list)
            { out << "</LI></UL><P>\n"; in_list = false; }
          else
            out << "</P><P>\n";
          continue;
        }
      if (*from == '\t')
        { // New bullet item.
          from++;
          if (!in_list)
            { out << "</P><UL><LI>\n"; in_list = true; }
          else
            out << "</LI><LI>\n";
          continue;
        }

      // If we get here, we must be inside a quote.
      assert(*from == '`');
      from++;
      to = strchr(from,'\'');
      if (to == NULL)
        { // Unable to complete quote.  Print it explicitly.
          out << "`"; continue;
        }

      char tchar = *to; *to = '\0'; // Temporarily insert null-terminator.
      char *link = search_for_link(from,reference_args,reference_funcs,
                                   reference_vars,reference_classes);
      if (link == NULL)
        { // Print the quoted string in bold.
          out << "<B>" << from << "</B>";
        }
      else
        { // Include hyperlink.
          out << "<A HREF=\"" << link << "\">" << from << "</A>";
        }
      *to = tchar; // Replace the termination character.
      from = to+1;
    }
  if (in_list)
    out << "</LI></UL>\n";
  else
    out << "</P>\n";
}


/* ========================================================================= */
/*                                kdd_primitive                              */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdd_primitive::init_base                        */
/*****************************************************************************/

kdd_primitive *
  kdd_primitive::init_base(kdd_primitive *list, const char *name,
                       const char *java_name, const char *jni_name,
                       const char *java_signature, bool java_size_compatible,
                       const char *csharp_name, const char *vbasic_name,
                       const char *mni_name, bool mni_size_compatible,
                       bool allow_arrays)
{
  if (this->name != NULL)
    delete[] this->name;
  this->name = new char[strlen(name)+1];
  strcpy(this->name,name);
  this->java_name = java_name;
  this->jni_name = jni_name;
  this->java_signature = java_signature;
  this->java_size_compatible = java_size_compatible;
  this->csharp_name = csharp_name;
  this->vbasic_name = vbasic_name;
  this->mni_name = mni_name;
  this->mni_size_compatible = mni_size_compatible;
  this->allow_arrays = allow_arrays;
  this->alias = NULL;
  this->next = list;

  this->string_type = false;
  const char *str = strstr(name,"char");
  if ((str != NULL) &&
      ((name == (const char *) str) || (str[-1]==' ')) &&
      (str[4] == ' '))
    {
      for (str+=4; *str == ' '; str++);
      if (*str == '*')
        this->string_type = true;
    }
  return this;
}

/*****************************************************************************/
/*                           kdd_primitive::new_alias                        */
/*****************************************************************************/

kdd_primitive *
  kdd_primitive::new_alias(const char *name, const char *alias_name)
{
  kdd_primitive *scan;
  for (scan=this; scan != NULL; scan=scan->next)
    if (strcmp(scan->name,name) == 0)
      return scan;
  for (scan=this; scan != NULL; scan=scan->next)
    if (strcmp(scan->name,alias_name) == 0)
      break;
  if (scan == NULL)
    return NULL;
  kdd_primitive *elt = new kdd_primitive;
  elt->alias = scan;
  elt->name = new char[strlen(name)+1];
  strcpy(elt->name,name);
  elt->java_name = elt->alias->java_name;
  elt->jni_name = elt->alias->jni_name;
  elt->java_signature = elt->alias->java_signature;
  elt->java_size_compatible = elt->alias->java_size_compatible;
  elt->csharp_name = elt->alias->csharp_name;
  elt->vbasic_name = elt->alias->vbasic_name;
  elt->mni_name = elt->alias->mni_name;
  elt->string_type = elt->alias->string_type;
  elt->mni_size_compatible = elt->alias->mni_size_compatible;
  elt->allow_arrays = elt->alias->allow_arrays;
  for (; scan->next != NULL; scan=scan->next);
  scan->next = elt;
  elt->next = NULL;
  return elt;
}


/* ========================================================================= */
/*                                  kdd_type                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                            kdd_type::operator==                           */
/*****************************************************************************/

bool
  kdd_type::operator==(kdd_type &ref)
{
  if (unknown_name != NULL)
    {
      if ((ref.unknown_name == NULL) ||
          (strcmp(ref.unknown_name,unknown_name) != 0))
        return false;
    }
  else if (ref.known_class != known_class)
    return false;
  if ((by_ref != ref.by_ref) ||
      ((derefs+as_array) != (ref.derefs+ref.as_array)))
    return false;
  return true;
}

/*****************************************************************************/
/*                             kdd_type::finalize                            */
/*****************************************************************************/

void
  kdd_type::finalize(kdd_class *classes, kdd_primitive *primitives)
{
  if (unknown_name == NULL)
    return;
  
  // Look for the longest match amongst defined primitives
  kdd_primitive *prim, *best_prim=NULL;
  char *ch, *mp, *mod = modifier;
  if (mod != NULL)
    {
      mod += strlen(mod);
      while ((mod > modifier) && (mod[-1] != ' '))
        mod--;
    }
  for (prim=primitives; prim != NULL; prim=prim->next)
    {
      char *name = prim->name;
      if (mod != NULL)
        {
          for (ch=name, mp=mod; (*ch == *mp) && (*mp != '\0'); ch++, mp++);
          if (*mp == '\0')
            { // Modifier is indeed the first part of the name
              name = ch;
              while (*name == ' ')
                name++;
            }
        }

      bool match = false;
      ch = name + strlen(name)-1;
      if ((*ch == '*') && (derefs > 0))
        {
          while ((ch > name) && (ch[-1] == ' '))
            ch--;
          char save_ch = *ch; *ch = '\0';
          match = (strcmp(name,unknown_name) == 0);
          *ch = save_ch;
        }
      else
        match = (strcmp(name,unknown_name) == 0);
      
      if (match &&
          ((best_prim == NULL) ||
           (strlen(prim->name) > strlen(best_prim->name))))
        best_prim = prim;
    }

  if (best_prim != NULL)
    {
      known_primitive = best_prim;
      delete[] unknown_name;
      unknown_name = NULL;
      if (mod != NULL)
        {
          for (ch=best_prim->name, mp=mod;
               (*ch == *mp) && (*mp != '\0');
               ch++, mp++);
          if (*mp == '\0')
            { // Shorten modifier, part of which belongs to primitive name
              while ((mod > modifier) && (mod[-1] == ' '))
                mod--;
              *mod = '\0';
              if (mod == modifier)
                { delete[] modifier; modifier = NULL; }
            }
        }
      ch = best_prim->name + strlen(best_prim->name)-1;
      if (*ch == '*')
        { // One level of indirection is already part of the primitive name
          assert(derefs > 0);
          derefs--;
        }
      
      return;
    }

  // Look for a match amongst defined classes.
  for (; classes != NULL; classes=classes->next)
    if ((classes->name != NULL) && (strcmp(classes->name,unknown_name) == 0))
      break;
  if (classes != NULL)
    {
      known_class = classes;
      delete[] unknown_name;
      unknown_name = NULL;
    }
}

/*****************************************************************************/
/*                             kdd_type::print                               */
/*****************************************************************************/

void
  kdd_type::print(ostream &out, const char *name)
{
  if ((known_class == NULL) && (known_primitive == NULL) &&
      (unknown_name == NULL))
    return;
  if ((modifier != NULL) && (*modifier != '\0'))
    out << modifier << " ";
  if (known_primitive != NULL)
    out << known_primitive->name;
  else if (known_class == NULL)
    out << unknown_name;
  else
    out << known_class->name;
  out << " ";
  for (int i=0; i < derefs; i++)
    if (i == 0)
      out << "*";
    else
      out << "*";
  if (by_ref)
    out << "&";
  if (name != NULL)
    {
      out << name;
      if (as_array)
        out << "[]";
    }
}

/*****************************************************************************/
/*                          kdd_type::generate_html                          */
/*****************************************************************************/

void
  kdd_type::generate_html(ostream &out)
{
  if ((known_class == NULL) && (known_primitive == NULL) &&
      (unknown_name == NULL))
    return;
  if ((modifier != NULL) && (*modifier != '\0'))
    out << modifier << "&nbsp;";
  if (known_primitive != NULL)
    out << "<B>" << known_primitive->name << "</B>";
  else if (known_class == NULL)
    out << "<B>" << unknown_name << "</B>";
  else if (known_class->html_name == NULL)
    out << "<B>" << known_class->name << "</B>";
  else
    out << "<A HREF =\"" << known_class->html_name << "\">"
        << known_class->name << "</A>";
  for (int i=0; i < derefs; i++)
    if (i == 0)
      out << "&nbsp;*";
    else
      out << "*";
  if (by_ref)
    out << "&nbsp;&amp;";
}

/*****************************************************************************/
/*                             kdd_type::is_void                             */
/*****************************************************************************/

bool
  kdd_type::is_void()
{
  if ((known_primitive == NULL) && (known_class == NULL) &&
      (unknown_name == NULL))
    return true; // Must be return type of a constructor/destructor
  return (known_primitive != NULL) &&
         (!by_ref) && (derefs == 0) && (as_array == 0) &&
         (known_primitive->vbasic_name[0] == '\0');
}

/*****************************************************************************/
/*                           kdd_type::is_bindable                           */
/*****************************************************************************/

bool
  kdd_type::is_bindable(bool for_return)
{
  if (known_primitive != NULL)
    {
      if (((derefs+as_array) == 1) && (!by_ref) &&
          (for_return || (binding == KDD_BIND_DONATE)))
        {
          bind_as_opaque_pointer = true;
          return true;
        }
      else if (for_return || !known_primitive->allow_arrays)
        {
          if (((derefs+as_array) == 0) && !by_ref)
            return true;
        }
      else if (strcmp(known_primitive->java_name,"void") == 0)
        return false;
      else
        { // Pass args with 1 level of indirection as arrays or by-reference,
          // depending on the binding language
          if ((derefs + as_array + ((by_ref)?1:0)) <= 1)
            return true;
        }
    }
  else if (known_class != NULL)
    {
      if (known_class->binding == KDD_BIND_IFC)
        {
          if (for_return)
            { // Can only return the interface itself
              if (((derefs+as_array) == 0) && !by_ref)
                return true;
            }
          else
            { // Can pass interface by value, pointer or reference
              if ((derefs + as_array + ((by_ref)?1:0)) <= 1)
                return true;
            }
        }
      else if (known_class->binding == KDD_BIND_REF)
        {
          if (for_return)
            { // Can only return a pointer to the object
              if (((derefs+as_array) == 1) && !by_ref)
                return true;
            }
          else
            { // Can pass a pointer or reference to object
              if (((derefs+as_array) + ((by_ref)?1:0)) == 1)
                return true;
            }
        }
      else if (known_class->binding == KDD_BIND_COPY)
        {
          if (for_return)
            { // Can return a pointer to an object, or an object for copying
              if (((derefs+as_array) <= 1) && !by_ref)
                return true;
            }
          else
            { // Can pass a pointer to, reference to, or copy of the object
              if ((derefs + as_array + ((by_ref)?1:0)) <= 1)
                return true;
            }
        }
    }
  else if (unknown_name != NULL)
    { // Bind pointers to unknown objects as Java "long" integers
      if ((derefs == 1) && !(as_array || by_ref))
        return true;
    }
  else
    return true;

  return false;  
}

/*****************************************************************************/
/*                          kdd_type::write_as_java                          */
/*****************************************************************************/

void
  kdd_type::write_as_java(ostream &out, bool for_return)
{
  if (known_primitive != NULL)
    {
      if (((derefs+as_array) == 0) && !by_ref)
        { // Primitive type used directly as arg/return by value
          out << known_primitive->java_name;
        }
      else if (bind_as_opaque_pointer)
        { // Donating or returning an array -- treat as abstract pointer
          out << "long";
        }
      else
        { // Primitive argument(s) passed by array.
          assert((derefs + as_array + ((by_ref)?1:0)) == 1);
          out << known_primitive->java_name << "[]";
        }
    }
  else if (known_class != NULL)
    {
      out.put((char)(toupper(known_class->name[0])));
      out << known_class->name+1;
    }
  else if (unknown_name != NULL)
    {
      assert((derefs == 1) && !(as_array || by_ref));
      out << "long";
    }
}

/*****************************************************************************/
/*                         kdd_type::write_as_csharp                         */
/*****************************************************************************/

void
  kdd_type::write_as_csharp(ostream &out, const char *name, bool for_html)
{
  const char *space = ((for_html)?"&nbsp;":" ");
  if (known_primitive != NULL)
    {
      if (bind_as_opaque_pointer)
        out << "IntPtr"; // Donated or returned arrays must be treated as
                         // opaque pointers
      else
        {
          if (by_ref)
            out << "ref" << space;
          out << known_primitive->csharp_name;
          if ((derefs+as_array) != 0)
            {
              assert((!by_ref) && ((derefs+as_array)==1)); // Else not bindable
              out << "[]";
            }
        }
      if (name != NULL)
        out << space << name;
    }
  else if (known_class != NULL)
    {
      out.put((char)(toupper(known_class->name[0])));
      out << known_class->name+1;
      if (name != NULL)
        out << space << name;
    }
  else if (unknown_name != NULL)
    {
      assert(derefs == 1);
      if (by_ref)
        { // Support the returning of opaque pointers via the argument list
          assert(name != NULL);
          out << "ref" << space;
        }
      out << "IntPtr";
    }
}

/*****************************************************************************/
/*                         kdd_type::write_as_vbasic                         */
/*****************************************************************************/

void
  kdd_type::write_as_vbasic(ostream &out, const char *name, bool for_html)
{
  const char *space = ((for_html)?"&nbsp;":" ");
  if (known_primitive != NULL)
    {
      if (name != NULL)
        {
          if (by_ref)
            out << "ByRef" << space;
          else
            out << "ByVal" << space;
          out << name;
          if (((as_array+derefs) != 0) && !bind_as_opaque_pointer)
            {
              assert((!by_ref) && ((derefs+as_array)==1)); // Else not bindable
              out << "()";
            }
        }
      else
        assert(!by_ref);
      if (!is_void())
        {
          out << space << "As";
          if (bind_as_opaque_pointer)
            out << space << "IntPtr";
          else
            out << space << known_primitive->vbasic_name;
        }
    }
  else if (known_class != NULL)
    {
      if (name != NULL)
        out << "ByVal" << space << name;
      out << space << "As" << space;
      out.put((char)(toupper(known_class->name[0])));
      out << known_class->name+1;
    }
  else if (unknown_name != NULL)
    {
      assert(derefs == 1);
      if (by_ref)
        { // Support the returning of opaque pointers via the argument list
          assert(name != NULL);
          out << "ByRef" << space;
        }
      else
        out << "ByVal";
      out << space << "As" << space << "IntPtr";
    }
}

/*****************************************************************************/
/*                      kdd_type::write_mni_declaration                      */
/*****************************************************************************/

void
  kdd_type::write_mni_declaration(ostream &out, const char *name,
                                  bool old_syntax, const char *prefix)
{
  if (known_primitive != NULL)
    {
      if (bind_as_opaque_pointer)
        {
          out << "IntPtr ";
          if (name != NULL)
            out << prefix << name;
        }
      else if (old_syntax)
        {
          out << known_primitive->mni_name << " ";
          if (by_ref)
            out << "*";
          if (name != NULL)
            out << prefix << name;
          if ((derefs+as_array) != 0)
            {
              assert((!by_ref) && ((derefs+as_array)==1)); // Else not bindable
              out << " __gc[]";
            }
        }
      else
        {
          if ((derefs+as_array) != 0)
            {
              assert((!by_ref) && ((derefs+as_array)==1)); // Else not bindable
              out << "cli::array<" << known_primitive->mni_name << "> ^";
            }
          else if (by_ref)
            out << "cli::interior_ptr<" << known_primitive->mni_name << "> ";
          else
            out << known_primitive->mni_name << " ";
         if (name != NULL)
            out << prefix << name;
        }
    }
  else if (known_class != NULL)
    {
      if (old_syntax)
        out << known_class->mni_name << " *";
      else
        out << known_class->mni_name << " ^";
      assert(as_array == 0); // Arrays of classes are not bindable
      if (name != NULL)
        out << prefix << name;
    }
  else if (unknown_name != NULL)
    {
      assert((derefs == 1) && !(as_array || by_ref));
      out << "IntPtr ";
      if (name != NULL)
        out << prefix << name;
    }
}

/*****************************************************************************/
/*                          kdd_type::get_jni_name                           */
/*****************************************************************************/

char *
  kdd_type::get_jni_name()
{
  static char jni_buf[256];

  if (known_primitive != NULL)
    {
      if (((derefs+as_array) == 0) && !by_ref)
        { // Primitive type used directly as arg/return by value
          strcpy(jni_buf,known_primitive->jni_name);
        }
      else if (bind_as_opaque_pointer)
        { // Donating or returning an array -- treat as abstract pointer
          strcpy(jni_buf,"jlong");
        }
      else
        { // Primitive argument passed by array
          assert((derefs + as_array + ((by_ref)?1:0)) == 1);
          strcpy(jni_buf,known_primitive->jni_name);
          strcat(jni_buf,"Array");
        }
    }
  else if (unknown_name != NULL)
    {
      assert((derefs == 1) && !(as_array || by_ref));
      strcpy(jni_buf,"jlong");
    }
  else
    {
      assert(known_class != NULL);
      strcpy(jni_buf,"jobject");
    }
  return jni_buf;
}


/* ========================================================================= */
/*                                kdd_constant                               */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdd_constant::create_html_link                      */
/*****************************************************************************/

void
  kdd_constant::create_html_link(const char *html_file)
{
  assert((html_file != NULL) && (html_link == NULL));
  html_link = new char[strlen(html_file)+2+strlen(name)];
  strcpy(html_link,html_file);
  strcat(html_link,"#");
  html_offset = html_link + strlen(html_link);
  strcat(html_link,name);
}


/* ========================================================================= */
/*                                kdd_variable                               */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdd_variable::create_html_link                      */
/*****************************************************************************/

void
  kdd_variable::create_html_link(const char *html_file)
{
  assert((html_file != NULL) && (html_link == NULL));
  html_link = new char[strlen(html_file)+2+strlen(name)];
  strcpy(html_link,html_file);
  strcat(html_link,"#");
  html_offset = html_link + strlen(html_link);
  strcat(html_link,name);
}


/* ========================================================================= */
/*                                  kdd_func                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                            kdd_func::operator==                           */
/*****************************************************************************/

bool
  kdd_func::operator==(kdd_func &ref)
{
  if (((name[0] != '~') || (ref.name[0] != '~')) && // Destructors equate
      (strcmp(name,ref.name) != 0))
    return false;

  kdd_variable *arg1, *arg2;
  for (arg1=args, arg2=ref.args;
       (arg1 != NULL) && (arg2 != NULL);
       arg1=arg1->next, arg2=arg2->next)
    if (arg1->type != arg2->type)
      break;
  return (arg1 == NULL) && (arg2 == NULL);
}

/*****************************************************************************/
/*                         kdd_func::create_html_names                       */
/*****************************************************************************/

void
  kdd_func::create_html_names(kdd_func *compaction_list)
{
  assert(html_name == NULL);
  const char *class_name = owner->html_name;
  int name_chars = (int) strlen(name);
  int prefix_chars = (int)(strlen(class_name)-strlen(".html"));
  assert(prefix_chars > 0);
  html_name = new char[prefix_chars+2+name_chars+16]; // Penty of extra
  if ((compaction_list == NULL) || ((name_chars+prefix_chars) <= 21))
    {
      strcpy(html_name,class_name);
      strcpy(html_name+prefix_chars,"__");
      strcat(html_name,name);
      if (overload_next != NULL)
        { // Append an overload index.
          char *end = html_name + strlen(html_name);
          sprintf(end,"__%d",overload_index);
        }
      strcat(html_name,".html");
      substitute_illegal_html_name_chars(html_name);
    }
  else
    {
      int i, counter=0;
      kdd_func *scan;
      do {
          int head_chars = (name_chars > 3)?3:name_chars;
          int tail_chars = 21 - head_chars - prefix_chars;
          if (tail_chars < 0)
            { head_chars += tail_chars; tail_chars = 0; }
          if (head_chars < 0)
            head_chars = 0;
          if ((head_chars+tail_chars) > name_chars)
            tail_chars = name_chars-head_chars;
          strcpy(html_name,class_name);
          strcpy(html_name+prefix_chars,"__");
          char *cp = html_name + strlen(html_name);
          for (i=0; i < head_chars; i++)
            *(cp++) = name[i];
          *(cp++) = '$';
          if (counter > 0)
            {
              sprintf(cp,"%d",counter);
              while (*cp != '\0')
                cp++;
            }
          for (i=tail_chars; i > 0; i--)
            *(cp++) = name[name_chars-i];
          if (overload_next != NULL)
            { // Append an overload index.
              sprintf(cp,"__%d",overload_index);
            }
          else
            *cp = '\0';
          strcat(html_name,".html");
          substitute_illegal_html_name_chars(html_name);
          for (scan=compaction_list; scan != this; scan=scan->next)
            if ((scan->html_name != NULL) &&
                (strcmp(scan->html_name,this->html_name) == 0))
              break;
          counter++;
        } while (scan != this);
    }

  for (kdd_variable *var=args; var != NULL; var=var->next)
    var->create_html_link(html_name);
}

/*****************************************************************************/
/*                           kdd_func::generate_html                         */
/*****************************************************************************/

void
  kdd_func::generate_html(kdd_class *classes, const char *directory)
{
  if (html_name == NULL)
    return;
  kdd_variable *arg;
  ofstream out;

  char *path = new char[strlen(html_name)+strlen(directory)+2];
  strcpy(path,directory);
  strcat(path,"/");
  strcat(path,html_name);
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  delete[] path;

  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE> Kakadu Hyper-Doc ("
      << ((owner->name==NULL)?"":(owner->name))
      << "::" << name
      << ") </TITLE>\n";
  out << "<STYLE TYPE=\"text/css\">\n<!--\n"
         ".indented-text { padding-left: 20pt; }\n"
         ".function-text { padding-left: 80pt;\n"
         "                 text-indent: -60pt; }\n"
         "-->\n</STYLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#B4DCB4\">\n";
  // Insert navigation links at top of page.
  out << "<P ALIGN=\"RIGHT\">";
  if (*(description.get()) != '\0')
    out << "|<A HREF=\"#SynopsiS\"> synopsis </A>|";
  if (*(return_description.get()) != '\0')
    out << "|<A HREF=\"#ReturnS\"> return value </A>|";
  if (args != NULL)
    out << "|<A HREF=\"#ArgS\"> arguments </A>|";
  if ((prev != NULL) && (prev->html_name != NULL))
    out << "|<A HREF=\"" << prev->html_name << "\"> prev </A>|";
  if ((next != NULL) && (next->html_name != NULL))
    out << "|<A HREF=\"" << next->html_name << "\"> next </A>|";
  out << "</P>\n";

  // Insert main heading line.
  out << "<H1><A NAME=\"ToP\">"
      << ((owner->name==NULL)?"":(owner->name))
      << "::" << name;
  out << "</A></H1>\n";
  if (java_name != NULL)
    {
      out << "<DIR><DIR><H3><EM><U>Java:</U> ";
      if (owner->name == NULL)
        out << "static ";
      out << owner->java_name << "."
          << java_name << "</EM></H3></DIR></DIR>\n";
    }
  if (csharp_name != NULL)
    {
      out << "<DIR><DIR><H3><EM><U>C#:</U> ";
      if (owner->name == NULL)
        out << "static ";
      out << owner->mni_name << ".";
      if (property == NULL)
        out << csharp_name
            << "</EM></H3></DIR></DIR>\n";
      else
        out << property->name
            << "</EM> -- (accessed by direct assignment)</H3></DIR></DIR>\n";
    }
  if (vbasic_name != NULL)
    {
      out << "<DIR><DIR><H3><EM><U>VB:</U> ";
      if (owner->name == NULL)
        out << "Shared ";
      out << owner->mni_name << ".";
      if (property == NULL)
        out << vbasic_name << "</EM></H3></DIR></DIR>\n";
      else
        out << property->name
            << "</EM> -- (accessed by direct assignment)</H3></DIR></DIR>\n";
    }
  if ((binding == KDD_BIND_CALLBACK) &&
      ((java_name != NULL) || (csharp_name != NULL) || (vbasic_name != NULL)))
      out << "<DIR><DIR><DIR><H3><U>CALLBACK:</U> Your implementation of "
             "this function in Java, C# or another foreign language will "
             "be called from the internal implementation of this object.  "
             "See \"java-and_managed-interfaces.pdf\" for more on CALLBACK "
             "functions.</H3>\n";
  if (overload_next != NULL)
    { // Insert links to the other overloaded functions with this name.
      kdd_func *fscan, *first=((overload_index==1)?this:NULL);
      int n, num_overloads = 1;
      for (fscan=overload_next; fscan != this; fscan=fscan->overload_next)
        {
          num_overloads++;
          assert(fscan->overload_index >= 1);
          if (fscan->overload_index == 1)
            first = fscan;
        }
      assert(first != NULL);
      out << "<P>Overload navigation: <B>";
      for (n=1, fscan=first; n<=num_overloads; n++, fscan=fscan->overload_next)
        {
          if (fscan != first)
            out << ",\n";
          if (fscan == this)
            out << n;
          else
            out << "<A HREF=\"" << fscan->html_name << "\">" << n << "</A>";
        }
      out << "</B></P>\n";
    }

  out << "<P CLASS=\"function-text\">";
  if (is_virtual)
    out << "virtual ";
  if (is_static)
    out << "static ";
  return_type.generate_html(out);
  out << " " << name << "(\n";
  for (arg=args; arg != NULL; arg=arg->next)
    {
      arg->type.generate_html(out);
      out << "&nbsp;" << arg->name;
      if (arg->type.as_array)
        out << "[]";
      if (arg->initializer != NULL)
        out << "=" << arg->initializer;
      if (arg->next != NULL)
        out << ",\n";
    }
  out << ")</P>\n";
  
  if (java_name != NULL)
    {
      out << "<DIR><DIR><P CLASS=\"function-text\"><EM><U>Java:</U> ";
      out << "public native ";
      if (owner->name == NULL)
        out << "static ";
      return_type.write_as_java(out,true);
      out << " " << java_name << "(\n";
      int nargs=num_jni_args;
      for (arg=args; nargs > 0; nargs--, arg=arg->next)
        {
          arg->type.write_as_java(out,false);
          out << "&nbsp;" << arg->name;
          if (nargs > 1)
            out << ",\n";
        }
      out << ")</EM></P></DIR></DIR>\n";
    }
  if (csharp_name != NULL)
    {
      out << "<DIR><DIR><P CLASS=\"function-text\"><EM><U>C#:</U> ";
      out << "public ";
      if (owner->name == NULL)
        out << "static ";
      if (property == NULL)
        {
          return_type.write_as_csharp(out,NULL,true);
          out << " " << csharp_name << "(\n";
          int nargs=num_mni_args;
          for (arg=args; nargs > 0; nargs--, arg=arg->next)
            {
              arg->type.write_as_csharp(out,arg->name,true);
              if (nargs > 1)
                out << ",\n";
            }
          out << ")";
        }
      else
        {
          out << "<A HREF=\"" << property->html_link << "\">";
          property->type.write_as_csharp(out,NULL,true);
          out << "&nbsp;" << property->name;
          out << "</A> {";
          if (property->property_get != NULL)
            out << " get;";
          if (property->property_set != NULL)
            out << " set;";
          out << " }";
        }
      out << "</EM></P></DIR></DIR>\n";
    }
  if (vbasic_name != NULL)
    {
      out << "<DIR><DIR><P CLASS=\"function-text\"><EM><U>VB:</U> ";
      out << "Public ";
      if (owner->name == NULL)
        out << "Shared ";
      if (property == NULL)
        {
          if (return_type.is_void())
            out << "Sub";
          else
            out << "Function";
          out << " " << vbasic_name << "(\n";
          int nargs = num_mni_args;
          for (arg=args; nargs > 0; nargs--, arg=arg->next)
            {
              arg->type.write_as_vbasic(out,arg->name,true);
              if (nargs > 1)
                out << ",\n";
            }
          out << ") ";
          return_type.write_as_vbasic(out,NULL,true);
        }
      else
        {
          if (property->property_set == NULL)
            out << "ReadOnly ";
          else if (property->property_get == NULL)
            out << "WriteOnly ";
          out << "Property <A HREF=\"" << property->html_link << "\">";
          out << property->name << "&nbsp;";
          property->type.write_as_vbasic(out,NULL,true);
          out << "</A>";
        }

      out << "</EM></P></DIR></DIR>\n";
    }

  if (overrides != NULL)
    {
      out << "<DIV CLASS=\"indented-text\"><H3>Overrides ";
      if (overrides->html_name != NULL)
        out << "<A HREF=\"" << overrides->html_name << "\">";
      out << overrides->owner->name << "::" << overrides->name;
      if (overrides->html_name != NULL)
        out << "</A>";
      out << "</H3></DIV>\n";
    }
  out << "<P>[Declared in <A HREF=\"" << file->html_name
      << "\">\"" << file->pathname << "\"</A>]</P>";
  if (owner->name != NULL)
    out << "<P><A HREF=\"" << owner->html_name << "\">"
        << "Go to class description.</A></P>\n";
  out << "\n<P ALIGN=\"CENTER\"><HR></P>"; // Draws a horizontal rule
  if (*(description.get()) != '\0')
    {
      out << "<H2><A NAME=\"SynopsiS\">Synopsis</A></H2>\n";
      out << "<DIV CLASS=\"indented-text\">\n";
      description.generate_html(out,args,owner->functions,
                                owner->variables,classes);
      out << "</DIV>\n";
      out << "\n<P ALIGN=\"CENTER\"><HR></P>"; // Draws a horizontal rule
    }

  if (*(return_description.get()) != '\0')
    {
      out << "<H2><A NAME=\"ReturnS\">Return Value</A></H2>"
             "<DIV CLASS=\"indented-text\">\n";
      return_description.generate_html(out,args,owner->functions,
                                       owner->variables,classes);
      out << "</DIV>\n";
    }
  if (args != NULL)
    {
      out << "<H2><A NAME=\"ArgS\">Arguments</A></H2>"
             "<DIV CLASS=\"indented-text\">\n";
      for (arg=args; arg != NULL; arg=arg->next)
        {
          assert(arg->html_offset != NULL);
          out << "<H4><A NAME=\"" << arg->html_offset << "\">" << arg->name;
          if (arg->type.as_array)
            out << "[]";
          out << "</A> [";
          arg->type.generate_html(out);
          out << "]</H4><DIV CLASS=\"indented-text\">\n";
          arg->description.generate_html(out,args,owner->functions,
                                         owner->variables,classes);
          out << "</DIV>\n";
        }
      out << "</DIV>\n";
    }

  // Insert navigation links at bottom of page.
  if ((*(return_description.get()) != '\0') || (args != NULL))
    out << "\n<P ALIGN=\"CENTER\"><HR></P>"; // Draws a horizontal rule
  out << "<P ALIGN=\"RIGHT\">";
  out << "|<A HREF=\"#ToP\"> top </A>|";
  if (*(description.get()) != '\0')
    out << "|<A HREF=\"#SynopsiS\"> synopsis </A>|";
  if (*(return_description.get()) != '\0')
    out << "|<A HREF=\"#ReturnS\"> return value </A>|";
  if (args != NULL)
    out << "|<A HREF=\"#ArgS\"> arguments </A>|";
  if ((prev != NULL) && (prev->html_name != NULL))
    out << "|<A HREF=\"" << prev->html_name << "\"> prev </A>|";
  if ((next != NULL) && (next->html_name != NULL))
    out << "|<A HREF=\"" << next->html_name << "\"> next </A>|";
  out << "</P>\n";

  out << "</BODY>\n</HTML>\n";
  out.close();
}


/* ========================================================================= */
/*                                 kdd_class                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                         kdd_class::find_properties                        */
/*****************************************************************************/

void
  kdd_class::find_properties()
{
  kdd_variable *var;
  kdd_func *func;
  for (var=variables; var != NULL; var=var->next)
    {
      if (var->type.by_ref || ((var->type.as_array+var->type.derefs) > 0))
        continue;
      for (func=functions; func != NULL; func=func->next)
        {
          const char *name = func->name;
          kdd_variable *arg = func->args;
          if ((name[0]=='s') && (name[1]=='e') && (name[2]=='t') &&
              (name[3]=='_') && (strcmp(name+4,var->name) == 0))
            { // We may have a `set_' accessor
              if (!func->return_type.is_void())
                break; // Must have void return
              if ((arg == NULL) || (arg->next != NULL))
                break; // Must have exactly one argument
              if (arg->type != var->type)
                break; // Argument type must agree with variable
              var->property_set = func;
              func->property = var;
            }
          if ((name[0]=='g') && (name[1]=='e') && (name[2]=='t') &&
              (name[3]=='_') && (strcmp(name+4,var->name) == 0))
            { // We may have a `get_' accessor
              if (arg != NULL)
                break; // May not have any arguments
              if (func->return_type != var->type)
                break; // Return must agree with variable
              var->property_get = func;
              func->property = var;
            }
        }
      if (func != NULL)
        { // Found at least one function name which is compatible with a
          // property, but is otherwise inadmissable.  This will become a
          // reserved name in the managed class interface if any property
          // accessor is created (either get or set), so we must disallow
          // any property accessors for this variable.  This is perhaps a
          // weakness of Microsoft's CLI compiler.
          if (var->property_get != NULL)
            {
              var->property_get->property = NULL;
              var->property_get = NULL;
            }
          if (var->property_set != NULL)
            {
              var->property_set->property = NULL;
              var->property_set = NULL;
            }
        }
    }
}

/*****************************************************************************/
/*                        kdd_class::create_html_names                       */
/*****************************************************************************/

void
  kdd_class::create_html_names(kdd_class *compaction_list)
{
  assert(html_name == NULL);
  if (!has_descriptions)
    return;
  if (name == NULL)
    {
      html_name = new char[strlen("globals.html")+1];
      strcpy(html_name,"globals.html");
    }
  else
    {
      int name_chars = (int) strlen(name);
      html_name = new char[name_chars+10]; // Plenty of head room
      if ((compaction_list == NULL) || (name_chars <= 10))
        {
          strcpy(html_name,name);
          strcat(html_name,".html");
        }
      else
        {
          int i, counter=0;
          kdd_class *scan;
          do {
              int head_chars = (name_chars > 3)?3:name_chars;
              int tail_chars = 5;
              if ((head_chars+tail_chars) > name_chars)
                tail_chars = name_chars-head_chars;
              char *cp = html_name;
              for (i=0; i < head_chars; i++)
                *(cp++) = name[i];
              *(cp++) = '$';
              if (counter > 0)
                {
                  sprintf(cp,"%d",counter);
                  while (*cp != '\0')
                    cp++;
                }
              for (i=tail_chars; i > 0; i--)
                *(cp++) = name[name_chars-i];
              strcpy(cp,".html");
              for (scan=compaction_list; scan != this; scan=scan->next)
                if ((scan->html_name != NULL) &&
                    (strcmp(scan->html_name,this->html_name) == 0))
                  break;
              counter++;
            } while (scan != this);
        }
    }

  for (kdd_func *func=functions; func != NULL; func=func->next)
    if ((name != NULL) || func->has_descriptions)
      func->create_html_names((compaction_list==NULL)?NULL:functions);
  for (kdd_variable *var=variables; var != NULL; var=var->next)
    if ((name != NULL) || (*(var->description.get()) != '\0'))
      var->create_html_link(html_name);
  for (kdd_constant *cnst=constants; cnst != NULL; cnst=cnst->next)
    cnst->create_html_link(html_name);
}

/*****************************************************************************/
/*                          kdd_class::generate_html                         */
/*****************************************************************************/

void
  kdd_class::generate_html(kdd_class *classes, const char *directory)
{
  if (html_name == NULL)
    return;
  ofstream out;
  kdd_class *scan;

  char *path = new char[strlen(html_name)+strlen(directory)+2];
  strcpy(path,directory);
  strcat(path,"/");
  strcat(path,html_name);
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  delete[] path;

  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE> Kakadu Hyper-Doc ("
      << ((name==NULL)?"Globals":name)
      << ") </TITLE>\n";
  out << "<STYLE TYPE=\"text/css\">\n<!--\n"
         ".indented-text { padding-left: 20pt; }\n"
         "-->\n</STYLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#FFF491\">\n";

  // Insert navigation links at top of page.
  out << "<P ALIGN=\"RIGHT\">";
  if (*(description.get()) != '\0')
    out << "|<A HREF=\"#SynopsiS\"> synopsis </A>|";
  if (functions != NULL)
    out << "|<A HREF=\"#FuncS\"> functions </A>|";
  if (variables != NULL)
    out << "|<A HREF=\"#VarS\"> variables </A>|";
  if (constants != NULL)
    out << "|<A HREF=\"#ConstS\"> constants </A>|";
  out << "</P>\n";

  // Main heading.
  if (name == NULL)
    out << "<H1><A NAME=\"ToP\">Globals</A></H1>\n";
  else
    {
      out << "<H1><A NAME=\"ToP\">" << name;
      if (is_struct)
        out << " [struct]";
      else if (is_union)
        out << " [union]";
      out << "</A></H1>\n";
    }
  if (java_name != NULL)
    out << "<DIR><DIR><H3><U>Java:</U> class "
        << java_name << "</H3></DIR></DIR>\n";
  if (mni_name != NULL)
    {
      out << "<DIR><DIR><H3><U>C#:</U> class " << mni_name;
      if (name == NULL)
        out << " (inherited by \"Ckdu_global\" in \"kdu_constants.cs\")";
      out << "</H3></DIR></DIR>\n";
      out << "<DIR><DIR><H3><U>VB:</U> class " << mni_name;
      if (name == NULL)
        out << " (inherited by \"Ckdu_global\" in \"kdu_constants.vb\")";
      out << "</H3></DIR></DIR>\n";
    }

  for (scan=this; scan != NULL; scan=scan->base.known_class)
    if (scan->base.exists())
      {
        out << "<DIR>"; // Augment left margin indent.
        out << "<P>Derives from ";
        scan->base.generate_html(out);
        out << "</P>\n";
      }
  for (scan=this; scan != NULL; scan=scan->base.known_class)
    if (scan->base.exists())
      out << "</DIR>"; // Decrease left margin indent.
  out << "\n";
  if (file != NULL)
    out << "<P>[Declared in <A HREF=\"" << file->html_name
        << "\">\"" << file->pathname << "\"</A>]</P>";

  bool in_derived_list = false;
  for (scan=classes; scan != NULL; scan = scan->next)
    if (scan->base.known_class == this)
      {
        if (!in_derived_list)
          {
            out << "<P ALIGN=\"CENTER\"><HR></P><H3>"
                   "Known objects derived from this class</H3><UL>\n";
            in_derived_list = true;
          }
        if (scan->html_name != NULL)
          out << "<LI><A HREF=\"" << scan->html_name << "\">"
              << scan->name << "</A></LI>\n";
        else
          out << "<LI><B>" << scan->name << "</B></LI>\n";
      }
  if (in_derived_list)
    out << "</UL>\n";
  out << "<P ALIGN=\"CENTER\"><HR></P>"; // Draws a horizontal rule
  if (*(description.get()) != '\0')
    {
      out << "<H2><A NAME=\"SynopsiS\">Synopsis</A></H2>\n";
      out << "<DIV CLASS=\"indented-text\">\n";
      description.generate_html(out,NULL,functions,variables,classes);
      out << "</DIV>\n";
    }
  out << "\n<P ALIGN=\"CENTER\"><HR></P>"; // Draws a horizontal rule
  if (functions != NULL)
    {
      out << "<H2><A NAME=\"FuncS\">Public Functions</A></H2>"
             "<DIV CLASS=\"indented-text\">\n";
      for (kdd_func *func=functions; func != NULL; func=func->next)
        if (func->html_name != NULL)
          {
            func->generate_html(classes,directory);
            if (func->overload_index > 1)
              continue;
            out << "<P><A HREF=\"" << func->html_name << "\">"
                << func->name << "</A>";
            if (func->is_virtual || func->is_static ||
                (func->binding == KDD_BIND_CALLBACK))
              {
                out << " [";
                if (func->binding == KDD_BIND_CALLBACK)
                  {
                    out << "CALLBACK";
                    if (func->is_virtual || func->is_static)
                      out << ", ";
                  }
                if (func->is_virtual)
                  {
                    out << "virtual";
                    if (func->is_static)
                      out << ", ";
                  }
                if (func->is_static)
                  out << "static";
                out << "]";
              }
            if (func->overload_next != NULL)
              {
                int num_overloads = 1;
                for (kdd_func *fp=func->overload_next;
                     fp != func; fp=fp->overload_next)
                  num_overloads++;
                out << " <EM>(" << num_overloads << " forms)</EM>";
              }
            if (func->java_name != NULL)
              out << " {<U>Java:</U> " << func->java_name << "}\n";
            if (func->csharp_name != NULL)
              {
                out << " {<U>C#:</U> ";
                if (func->property == NULL)
                   out << func->csharp_name;
                else
                  out << "see <A HREF=\"" << func->property->html_link
                      << "\">" << func->property->name << "</A>";
                out << "}\n";
              }
            if (func->vbasic_name != NULL)
              {
                out << " {<U>VB:</U> ";
                if (func->property == NULL)
                   out << func->vbasic_name;
                else
                  out << "see <A HREF=\"" << func->property->html_link
                      << "\">" << func->property->name << "</A>";
                out << "}\n";
              }
            out << "</P>\n";
         }
      out << "</DIV>\n";
    }
  if (variables != NULL)
    {
      out << "<H2><A NAME=\"VarS\">Public Variables</A></H2>"
             "<DIV CLASS=\"indented-text\">\n";
      for (kdd_variable *var=variables; var != NULL; var=var->next)
        if ((name != NULL) || (*(var->description.get()) != '\0'))
          {
            assert(var->html_offset != NULL);
            out << "<H4><A NAME=\"" << var->html_offset << "\">"
                << var->name;
            if (var->type.as_array)
              out << "[]";
            out << "</A> [";
            var->type.generate_html(out);
            out << "]";
            if ((var->property_get != NULL) || (var->property_set != NULL))
              {
                out << " -- <U>Exported Property</U>&nbsp;{";
                if (var->property_get != NULL)
                  out << " <A HREF=\"" << var->property_get->html_name << "\">"
                      << "get</A>;";
                if (var->property_set != NULL)
                  out << " <A HREF=\"" << var->property_set->html_name << "\">"
                      << "set</A>;";
                out << " }";
              }
            out << "</H4><DIV CLASS=\"indented-text\">\n";
            var->description.generate_html(out,NULL,functions,
                                           variables,classes);
            out << "</DIV>\n";
          }
      out << "</DIV>\n";
    }
  if (constants != NULL)
    {
      out << "<H2><A NAME=\"ConstS\">Constants</A></H2>"
             "<DIV CLASS=\"indented-text\">\n";
      for (kdd_constant *cst=constants; cst != NULL; cst=cst->next)
        {
          assert(cst->html_offset != NULL);
          out << "<H4><A NAME=\"" << cst->html_offset << "\">"
              << cst->name << " [" << cst->type->name << "]";
          out << " = " << cst->value << "</H4>\n";
          if (name == NULL)
            {
              out << "<DIV CLASS=\"indented-text\">\n";
              out << "<P><U>Java:</U> " << cst->type->java_name
                  << " Kdu_global." << cst->name
                  << "</P>\n";
              out << "<P><U>C#:</U> " << cst->type->csharp_name
                  << " Ckdu_global." << cst->name
                  << " (include \"kdu_constants.cs\")</P>\n";
              out << "<P><U>VB:</U> "
                  << " Ckdu_global." << cst->name
                  << " As " << cst->type->vbasic_name
                  << " (include \"kdu_constants.vb\")</P>\n";
              out << "</DIV>\n";
            }
        }
      out << "</DIV>\n";
    }

  // Insert navigation links at bottom of page.
  out << "<P ALIGN=\"CENTER\"><HR></P>\n"; // Draws a horizontal rule
  out << "<P ALIGN=\"RIGHT\">";
  out << "|<A HREF=\"#ToP\"> top </A>|";
  if (*(description.get()) != '\0')
    out << "|<A HREF=\"#SynopsiS\"> synopsis </A>|";
  if (functions != NULL)
    out << "|<A HREF=\"#FuncS\"> functions </A>|";
  if (variables != NULL)
    out << "|<A HREF=\"#VarS\"> variables </A>|";
  if (constants != NULL)
    out << "|<A HREF=\"#ConstS\"> constants </A>|";
  out << "</P>\n";

  out << "</BODY>\n</HTML>\n";
  out.close();
}


/* ========================================================================= */
/*                                  kdd_file                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                             kdd_file::kdd_file                            */
/*****************************************************************************/

kdd_file::kdd_file(const char *full_path_name)
{
  char *cp;

  pathname = name = html_name = NULL;
  has_bindings = false;
  copy_string(pathname,full_path_name);
  name = pathname;
  for (cp=pathname; *cp != '\0'; cp++)
    if ((*cp == '/') || (*cp == '\\') || (*cp == ':'))
      name = cp+1;
  html_name = new char[strlen(pathname)+7];
  html_name[0] = '+';
  strcpy(html_name+1,pathname);
  for (cp=html_name; *cp != '\0'; cp++)
    if ((*cp == '.') || (*cp == '/') || (*cp == '\\'))
      *cp = '+';
  strcpy(cp,".html");
}

/*****************************************************************************/
/*                        kdd_file::shorten_html_name                        */
/*****************************************************************************/

void
  kdd_file::shorten_html_name(kdd_file *list)
{
  int name_chars = (int) strlen(html_name);
  if (name_chars <= 32)
    return;
  char *cp, *new_name = new char[name_chars+1];
  int i, counter = 0;
  kdd_file *scan;
  do {
      int head_chars = 10;
      int tail_chars = 19;
      cp = new_name;
      for (i=0; i < head_chars; i++)
        *(cp++) = html_name[i];
      *(cp++) = '$';
      if (counter > 0)
        {
          sprintf(cp,"%d",counter);
          while (*cp != '\0')
            cp++;
        }
      for (i=tail_chars; i > 0; i--)
        *(cp++) = html_name[name_chars-i];
      *cp = '\0';
      for (scan=list; scan != this; scan=scan->next)
        if ((scan->html_name != NULL) && (strcmp(scan->html_name,new_name)==0))
          break;
      counter++;
    } while (scan != this);
  delete[] html_name;
  html_name = new_name;
}

/*****************************************************************************/
/*                           kdd_file::generate_html                         */
/*****************************************************************************/

void
  kdd_file::generate_html(kdd_class *classes, const char *directory)
{
  if (html_name == NULL)
    return;
  ofstream out;
  kdd_class *scan;

  char *path = new char[strlen(html_name)+strlen(directory)+2];
  strcpy(path,directory);
  strcat(path,"/");
  strcat(path,html_name);
  if (!create_file(path,out))
    { kdu_error e; e << "Unable to create the HTML file, \"" << path << "\".";}
  delete[] path;

  out << "<HTML>\n<HEAD>\n";
  out << "<TITLE> Kakadu Hyper-Doc (\"" << name << "\") </TITLE>\n";
  out << "<STYLE TYPE=\"text/css\">\n<!--\n"
         ".indented-text { padding-left: 20pt; }\n"
         "-->\n</STYLE>\n";
  out << "</HEAD>\n";
  out << "<BODY TEXT=\"#000000\" LINK=\"#0000ff\" BGCOLOR=\"#CCECFF\">\n";

  out << "<H1>\"" << pathname << "\"</H1>\n";
  out << "<H2>Classes</H2>\n";
  out << "<DIV CLASS=\"indented-text\">\n";
  for (scan=classes; scan != NULL; scan=scan->next)
    if ((scan->file == this) && (scan->html_name != NULL))
      out << "<P><A HREF=\"" << scan->html_name << "\">"
          << scan->name << "</A>\n";
  out << "</DIV>\n";

  out << "</BODY>\n</HTML>\n";
  out.close();
}

/*****************************************************************************/
/*                             kdd_file::copy_file                           */
/*****************************************************************************/

void
  kdd_file::copy_file(const char *directory)
{
  ifstream in;

  in.open(pathname);
  if (!in)
    { kdu_error e; e << "Cannot open file, \"" << pathname << "\"!"; }

  char *out_pathname = new char[strlen(directory)+256];
  strcpy(out_pathname,directory);
  strcat(out_pathname,"/");
  strcat(out_pathname,name);
  ofstream out;
  if (!create_file(out_pathname,out))
    { kdu_error e; e << "Cannot open file, \"" << out_pathname << "\"!"; }
  delete[] out_pathname;
  out_pathname = NULL;

  char line[1024]; // Can't handle longer lines in the file.
  while (in.getline(line,1024).gcount() > 0)
    out << line << "\n";

  in.close();
  out.close();
}


/* ========================================================================= */
/*                           External Functions                              */
/* ========================================================================= */

/*****************************************************************************/
/*                           write_relative_path                             */
/*****************************************************************************/

extern void
  write_relative_path(const char *target_dir, const char *ref_dir,
                      const char *target_file, ostream &out)
{
  if (ref_dir == NULL)
    ref_dir = "";
  if (target_dir == NULL)
    target_dir = "";

  // Eliminate common prefix between `target_dir' and `ref_dir'
  const char *tp, *rp;
  for (tp=target_dir, rp=ref_dir; *tp != '\0'; tp++, rp++)
    if ((*tp == '/') || (*tp == '\\'))
      {
        if ((*rp == '/') || (*rp == '\\'))
          { target_dir=tp+1; ref_dir=rp+1; }
        else
          break; // Paths disagree at this point
      }
    else if ((*rp == '/') || (*rp == '\\'))
      break; // Paths disagree at this point
    else if (*rp != *tp)
      break; // Paths disagree at this point
  if ((*ref_dir == '/') || (*target_dir == '/') ||
      (*ref_dir == '\\') || (*target_dir == '\\'))
    { out << target_file; return; }
  if ((strchr(ref_dir,':') != NULL) || (strchr(target_dir,':') != NULL) ||
      (strstr(ref_dir,"..") != NULL))
    { out << target_file; return; }
  if (*ref_dir != '\0')
    out << "../";
  for (; *ref_dir != '\0'; ref_dir++)
    if ((*ref_dir == '/') || (*ref_dir == '\\'))
      out << "../";
  out << target_dir << "/" << target_file;
}

/*****************************************************************************/
/*                              create_file                                  */
/*****************************************************************************/

extern bool
  create_file(char *path, ofstream &out)
{
  out.open(path);
  if (out.is_open())
    return true;

  // If we get here, we may need to create some directories
  char *cp = path+strlen(path)-1;
  while ((*cp != '\0') && (*cp != '/') && (*cp != '\\'))
    cp--;
  if (*cp == '\0')
    return false; // No directories to create
  char sep = *cp;
  *cp = '\0';
  bool success = create_directory(path);
  *cp = sep;
  if (!success)
    return false;
  out.clear();
  out.open(path);
  return out.is_open();
}

/*****************************************************************************/
/*                                  main                                     */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  extern void make_java_bindings(kdd_file *header_files,
                                 kdd_class *classes, const char *java_dir,
                                 const char *jni_dir, const char *aux_dir);
       /* This function is implemented in "jni_builder.cpp". */
  extern void make_managed_bindings(kdd_file *header_files, kdd_class *classes,
                                    const char *mni_dir, const char *aux_dir,
                                    bool old_syntax);
       /* This function is implemented in "mni_builder.cpp". */
  extern void make_aux_bindings(kdd_file *header_files,
                                kdd_class *classes, const char *aux_dir);
       /* This function is implemented in "aux_builder.cpp". */

  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);
  kdu_args args(argc,argv,"-s");
  char *string;

  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  char *out_dir = NULL;
  bool long_names = false;
  if (args.find("-o") != NULL)
    {
      if ((string = args.advance()) == NULL)
        { kdu_error e;
          e << "The \"-o\" argument requires a directory name parameter."; }
      if (strstr(string,".h") != NULL)
        { kdu_error e; e << "The directory supplied with the \"-o\" argument "
          "looks like a header file."; }
      out_dir = new char[strlen(string)+1];
      strcpy(out_dir,string);
      args.advance();
    }
  if (args.find("-long") != NULL)
    {
      long_names = true;
      args.advance();
    }

  char *java_dir = NULL;
  char *jni_dir = NULL;
  char *java_aux_dir = NULL;
  char *java_includes_dir = NULL;
  if (args.find("-java") != NULL)
    {
      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-java\" argument requires four "
          "directory name parameters."; }
      java_dir = new char[strlen(string)+1];
      strcpy(java_dir,string);

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-java\" argument requires four "
          "directory name parameters."; }
      jni_dir = new char[strlen(string)+1];
      strcpy(jni_dir,string);

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-java\" argument requires four "
          "directory name parameters."; }
      java_aux_dir = new char[strlen(string)+1];
      strcpy(java_aux_dir,string);

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-java\" argument requires four "
          "directory name parameters."; }
      java_includes_dir = new char[strlen(string)+1];
      strcpy(java_includes_dir,string);

      if ((strstr(java_dir,".h") != NULL) ||
          (strstr(jni_dir,".h") != NULL) ||
          (strstr(java_aux_dir,".h") != NULL) ||
          (strstr(java_includes_dir,".h") != NULL))
        { kdu_error e; e << "One of the FOUR directory names supplied "
        "with the \"-java\" argument looks like a header file.  These are:\n"
          "<CLASS_dir>=\""    << java_dir << "\";\n"
          "<JNI_dir>=\""      << jni_dir << "\";\n"
          "<AUX_dir>=\""      << java_aux_dir << "\"; and\n"
          "<INCLUDES_dir>=\"" << java_includes_dir << "\".";
        }
      args.advance();
    }

  char *mni_dir = NULL;
  char *managed_aux_dir = NULL;
  char *managed_includes_dir = NULL;
  bool old_managed_syntax = false;
  if (args.find("-old_managed_syntax") != NULL)
    {
      old_managed_syntax = true;
      args.advance();
    }
  if (args.find("-managed") != NULL)
    {
      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-managed\" argument requires three "
          "directory name parameters."; }
      mni_dir = new char[strlen(string)+1];
      strcpy(mni_dir,string);

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-managed\" argument requires three "
          "directory name parameters."; }
      managed_aux_dir = new char[strlen(string)+1];
      strcpy(managed_aux_dir,string);

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-managed\" argument requires three "
          "directory name parameters."; }
      managed_includes_dir = new char[strlen(string)+1];
      strcpy(managed_includes_dir,string);

      if ((strstr(mni_dir,".h") != NULL) ||
          (strstr(managed_aux_dir,".h") != NULL) ||
          (strstr(managed_includes_dir,".h") != NULL))
        { kdu_error e; e << "One of the THREE directory names supplied "
          "with the \"-managed\" argument looks like a header file.  "
          "These are:\n"
          "<MNI_dir>=\""      << jni_dir << "\";\n"
          "<AUX_dir>=\""      << managed_aux_dir << "\"; and\n"
          "<INCLUDES_dir>=\"" << managed_includes_dir << "\".";
        }
      if ((java_aux_dir != NULL) &&
          (strcmp(java_aux_dir,managed_aux_dir) != 0))
        { kdu_warning w; w << "You are building both Java interfaces and "
          "managed native interfaces, but you are using different <AUX_dir> "
          "directories, \"" << java_aux_dir << "\" and \""
          << managed_aux_dir << "\", in each case.  This is allowed, but "
          "not recommended, since the content of both directories will be "
          "identical."; }
      else
        {
          delete[] java_aux_dir;
          java_aux_dir = NULL;
        }

      if ((java_includes_dir != NULL) &&
          (strcmp(java_includes_dir,managed_includes_dir) != 0))
        { kdu_warning w; w << "You are building both Java interfaces and "
          "managed native interfaces, but you are using different "
          "<INCLUDES_dir> directories, \"" << java_includes_dir << "\" and \""
          << managed_includes_dir << "\", in each case.  This is allowed, but "
          "not recommended."; }
      else
        {
          delete[] java_includes_dir;
          java_includes_dir = NULL;
        }

      args.advance();
    }

  // Collect all "-bind" arguments
  kdd_bind_specific *bind_specific = NULL;
  while (args.find("-bind") != NULL)
    {
      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-bind\" argument requires a "
          "class or function name -- see the \"-usage\" statement."; }
      kdd_bind_specific *elt = new kdd_bind_specific(string);
      elt->next = bind_specific;
      bind_specific = elt;
      args.advance();
    }

  // Set up base primitives (those with no aliases)
  kdd_primitive *primitives = NULL;
  primitives = (new kdd_primitive)->init_base(primitives,"void",
                                "void","void","V",false,
                                "void","","void",false,
                                false);
  primitives = (new kdd_primitive)->init_base(primitives,"bool",
                                "boolean","jboolean","Z",false,
                                "bool","Boolean","Boolean",false,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"unsigned char",
                                "byte","jbyte","B",true,
                                "byte","Byte","Byte",true,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"short",
                                "short","jshort","S",true,
                                "short","Short","Int16",true,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"unsigned short",
                                "int","jint","I",false,
                                "int","Integer","Int32",false,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"int",
                                "int","jint","I",true,
                                "int","Integer","Int32",true,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"unsigned",
                                "long","jlong","J",false,
                                "long","Long","Int64",false,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"kdu_long",
                                "long","jlong","J",false,
                                "long","Long","Int64",false,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"float",
                                "float","jfloat","F",true,
                                "float","Single","Single",true,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"double",
                                "double","jdouble","D",true,
                                "double","Double","Double",true,
                                true);
  primitives = (new kdd_primitive)->init_base(primitives,"kdu_exception",
                                "int","jint","I",false,
                                "int","Integer","Int32",true,true);
     // `kdu_exception' is a bit different to everything else.  It is
     // currently typedef'ed as an integer in "kdu_elementary.h" so we make
     // this explicit here.
  if (old_managed_syntax)
    {
      primitives = (new kdd_primitive)->init_base(primitives,"const char *",
                                "String","jstring","Ljava/lang/String;",false,
                                "string","String","String *",false,      
                                false);
      primitives = (new kdd_primitive)->init_base(primitives,"char *",
                                "String","jstring","Ljava/lang/String;",false,
                                "string","String","String *",false,
                                false);
    }
  else
    {
      primitives = (new kdd_primitive)->init_base(primitives,"const char *",
                                "String","jstring","Ljava/lang/String;",false,
                                "string","String","String ^",false,      
                                false);
      primitives = (new kdd_primitive)->init_base(primitives,"char *",
                                "String","jstring","Ljava/lang/String;",false,
                                "string","String","String ^",false,
                                false);
    }

  // Set up some initial primitive type aliases which will always exist
  primitives->new_alias("short int","short");
  primitives->new_alias("unsigned short int","unsigned short");
  primitives->new_alias("unsigned int","unsigned");
  primitives->new_alias("kdu_byte","unsigned char");
  primitives->new_alias("kdu_int16","short int");
  primitives->new_alias("kdu_uint16","unsigned short int");
  primitives->new_alias("kdu_int32","int");
  primitives->new_alias("kdu_uint32","unsigned int");

  // Process source files
  kdd_class *classes = new kdd_class; // Create empty class for globals
  classes->binding = KDD_BIND_GLOBALS;
  kdd_file *files = NULL;
  for (string=args.get_first(); string != NULL; )
    {
      const char *delim;
      if (((delim = strrchr(string,'.')) != NULL) &&
          (toupper(delim[1]) == (int) 'H') && (delim[2] == '\0'))
        {
          kdd_file *new_file = process_file(string,primitives,classes);
          if (new_file != NULL)
            {
              new_file->next = files;
              files = new_file;
              if (!long_names)
                new_file->shorten_html_name(files);
            }
          string = args.advance(true);
        }
      else
        string = args.advance(false);
    }

  // Create list of extra documents.
  kdd_index *extras=NULL, *extras_tail=NULL;
  for (string=args.get_first(); string != NULL; )
    {
      if (strchr(string,'.') != NULL)
        {
          if (extras_tail == NULL)
            extras = extras_tail = new kdd_index;
          else
            extras_tail = extras_tail->next = new kdd_index;
          copy_string(extras_tail->name,string);
          copy_string(extras_tail->link,string);
          string = args.advance(true);
        }
      else
        string = args.advance(false);
    }
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

  resolve_inheritance(classes);
  resolve_links(classes,primitives);
  resolve_virtual_inheritance(classes);
  order_lists(classes,files);

  kdd_class *csp;
  for (csp=classes; csp != NULL; csp=csp->next)
    csp->find_properties();

  if (bind_specific != NULL)
    {
      kdd_bind_specific *elt;
      while ((elt=bind_specific) != NULL)
        {
          if (!elt->apply(classes))
            { kdu_warning w; w << "Unable to find class identified by "
              "\"-bind " << elt->full_name << "\" argument."; }
          bind_specific = elt->next;
          delete elt;
        }
      for (csp=classes; csp != NULL; csp=csp->next)
        if ((csp->name == NULL) || csp->bind_specific)
          {
            csp->bind_specific = true; // So that globals class is always bound
            for (kdd_func *func=csp->functions; func != NULL; func=func->next)
              if (!func->bind_specific)
                func->binding = KDD_BIND_NONE;
          }
        else
          csp->binding = KDD_BIND_NONE;
    }

  if (java_dir != NULL)
    {
      assert(jni_dir != NULL);
      const char *aux_dir =
        (java_aux_dir!=NULL)?java_aux_dir:managed_aux_dir;
      make_java_bindings(files,classes,java_dir,jni_dir,aux_dir);
      delete[] java_dir;
      delete[] jni_dir;
    }
  if (mni_dir != NULL)
    {
      make_managed_bindings(files,classes,mni_dir,managed_aux_dir,
                            old_managed_syntax);
      delete[] mni_dir;
    }
  if (java_aux_dir != NULL)
    {
      make_aux_bindings(files,classes,java_aux_dir);
      delete[] java_aux_dir;
    }
  if (managed_aux_dir != NULL)
    {
      make_aux_bindings(files,classes,managed_aux_dir);
      delete[] managed_aux_dir;
    }
  if (java_includes_dir != NULL)
    {
      for (kdd_file *fp=files; fp != NULL; fp=fp->next)
        fp->copy_file(java_includes_dir);
      delete[] java_includes_dir;
    }
  if (managed_includes_dir != NULL)
    {
      for (kdd_file *fp=files; fp != NULL; fp=fp->next)
        fp->copy_file(managed_includes_dir);
      delete[] managed_includes_dir;
    }
  if (out_dir != NULL)
    {
      for (csp=classes; csp != NULL; csp=csp->next)
        csp->create_html_names((long_names)?NULL:classes);
      for (csp=classes; csp != NULL; csp=csp->next)
        csp->generate_html(classes,out_dir);
      for (kdd_file *fp=files; fp != NULL; fp=fp->next)
        fp->generate_html(classes,out_dir);
      generate_indices(classes,extras,files,out_dir);
      delete[] out_dir;
    }

  return 0;
}
