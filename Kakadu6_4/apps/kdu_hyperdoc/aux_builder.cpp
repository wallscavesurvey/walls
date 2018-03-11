/*****************************************************************************/
// File: aux_builder.cpp [scope = APPS/HYPERDOC]
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
   Implements automatic construction of "kdu_aux.cpp" and "kdu_aux.h",
for the "kdu_hyperdoc" utility.
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "hyperdoc_local.h"
#include "kdu_messaging.h"


/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                   generate_delegator_class                         */
/*****************************************************************************/

static void
  generate_delegator_class(kdd_class *cls, ostream &out)
{
  if (cls->is_struct)
    { kdu_error e; e << "Struct, \"" << cls->name << "\", defined on line "
      << cls->line_number << " in file, \"" << cls->file->name
      << "\", has functions with the \"[BIND: callback]\" binding.  Only "
         "classes may have callback functions in Kakadu.";
    }
  out << "class " AUX_DELEGATOR_PREFIX << cls->name << " {\n";
  out << "  public:\n";
  out << "    virtual ~" AUX_DELEGATOR_PREFIX
      << cls->name << "() { return; }\n";
  for (kdd_func *func=cls->functions; func != NULL; func=func->next)
    {
      if (func->binding != KDD_BIND_CALLBACK)
        continue;
      if (!func->is_virtual)
        { kdu_error e; e << "Error at line " << func->line_number
          << " in file, \"" << func->file->name << "\"; functions declared "
             "with the \"BIND: CALLBACK\" binding must be virtual.";
        }
      out << "    virtual ";
      func->return_type.print(out,NULL);
      out << func->name << "(";
      for (kdd_variable *arg=func->args; arg != NULL; arg=arg->next)
        {
          arg->type.print(out,arg->name);
          if (arg->next != NULL)
            out << ", ";
        }
      out << ") ";
      kdd_type &return_type = func->return_type;
      if (return_type.is_void())
        out << "{ return; }\n";
      else if ((return_type.as_array > 0) || (return_type.derefs > 0))
        out << "{ return NULL; }\n";
      else if (return_type.known_class != NULL)
        out << "{ return " << return_type.known_class->name << "(); }\n";
      else if (return_type.known_primitive != NULL)
        {
          if (return_type.known_primitive->string_type)
            out << "{ return NULL; }\n";
          else
            out << "{ return ("<<return_type.known_primitive->name<<") 0; }\n";
        }
      else if (return_type.unknown_name != NULL)
        out << "{ return NULL; }\n";
    }
  out << "  };\n";
}

/*****************************************************************************/
/* STATIC              generate_derived_class_declaration                    */
/*****************************************************************************/

static void
  generate_derived_class_declaration(kdd_class *cls, ostream &out)
{
  out << "class " AUX_EXTENSION_PREFIX << cls->name
      <<    " : public " << cls->name << " {\n";
  out << "  public:\n";
  out << "    " AUX_DELEGATOR_PREFIX << cls->name << " * _delegator;\n";
  out << "  public:\n";

  kdd_func *func;
  kdd_variable *arg;

  // Generate constructors
  bool has_constructor=false;
  for (func=cls->functions; func != NULL; func=func->next)
    if (func->is_constructor)
      { // Augment the constructor, to pass in the `_owner_ref' pointer
        has_constructor = true;
        out << "    KDU_AUX_EXPORT " AUX_EXTENSION_PREFIX << cls->name << "(";
        for (arg=func->args; arg != NULL; arg=arg->next)
          {
            arg->type.print(out,arg->name);
            if (arg->initializer != NULL)
              out << "=" << arg->initializer;
            if (arg->next != NULL)
              out << ", ";
          }
        out << ");\n";
      }
  if (!has_constructor)
    { kdu_error e; e << "Class, \"" << cls->name << "\", defined on line "
      << cls->line_number << " in file, \"" << cls->file->name
      << "\", has functions with the \"[BIND: callback]\" binding, but "
         "is missing a public constructor.  "
         "You must supply at least one public constructor, even "
         "if it does nothing.";
    }

  // Extend callback functions
  for (func=cls->functions; func != NULL; func=func->next)
    {
      if (func->binding != KDD_BIND_CALLBACK)
        continue;
      assert(func->is_virtual); // Checked inside `generate_delegator_class'
      out << "    virtual ";
      func->return_type.print(out,NULL);
      out << " " << func->name << "(";
      for (arg=func->args; arg != NULL; arg=arg->next)
        {
          arg->type.print(out,arg->name);
          if (arg->next != NULL)
            out << ", ";
        }
      out << ")\n";
      out << "      { ";
      if (!func->return_type.is_void())
        out << "return ";
      out << "_delegator->" << func->name << "(";
      for (arg=func->args; arg != NULL; arg=arg->next)
        {
          out << arg->name;
          if (arg->next != NULL)
            out << ",";
        }
      out << "); }\n";
    }

  // End class declaration
  out << "  };\n";
}

/*****************************************************************************/
/* STATIC             generate_derived_class_implementation                  */
/*****************************************************************************/

static void
  generate_derived_class_implementation(kdd_class *cls, ostream &out)
{
  kdd_func *func;
  kdd_variable *arg;

  // Generate constructors
  for (func=cls->functions; func != NULL; func=func->next)
    if (func->is_constructor)
      { // Augment the constructor, to pass in the `_owner_ref' pointer
        out << AUX_EXTENSION_PREFIX << cls->name << "::";
        out << AUX_EXTENSION_PREFIX << cls->name << "(";
        for (arg=func->args; arg != NULL; arg=arg->next)
          {
            arg->type.print(out,arg->name);
            if (arg->next != NULL)
              out << ", ";
          }
        out << ")\n";
        if (func->args != NULL)
          {
            out << "    : " << cls->name << "(";
            for (arg=func->args; arg != NULL; arg=arg->next)
              {
                out << arg->name;
                if (arg->next != NULL)
                  out << ",";
              }
            out << ")\n";
          }
        out << "{\n";
        out << "  this->_delegator = NULL;\n";
        out << "}\n";
      }
}

/*****************************************************************************/
/* STATIC                      generate_aux_header                           */
/*****************************************************************************/

static void
  generate_aux_header(kdd_file *header_files, kdd_class *classes,
                     const char *aux_dir)
{
  char *path = new char[strlen(aux_dir)+80]; // More than enough
  strcpy(path,aux_dir);
  strcat(path,"/kdu_aux.h");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the auxiliary header file, \""
        << path << "\".";
    }

  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n";
  out << "#ifndef KDU_AUX_H\n";
  out << "\n";
  out << "// Public header files used by \"kdu_aux.dll\" (or \"kdu_aux.h\")\n";
  for (; header_files != NULL; header_files=header_files->next)
    if (header_files->has_bindings)
      out << "#include \"" << header_files->name << "\"\n";
  out << "\n";
  out << "/* ============================================ */\n";
  out << "// Special classes for use in binding callbacks\n";
  out << "/* ============================================ */\n";
  out << "\n";
  for (kdd_class *cls=classes; cls != NULL; cls=cls->next)
    {
      if ((cls->binding != KDD_BIND_REF) || (!cls->has_callbacks))
        continue;
      out << "//---------------------------------------------------\n";
      out << "// Delegator for class: " << cls->name << "\n";
      generate_delegator_class(cls,out);
      out << "\n";
      out << "//---------------------------------------------------\n";
      out << "// Derived version of class: " << cls->name << "\n";
      generate_derived_class_declaration(cls,out);
      out << "\n";
    }

  out << "#endif // KDU_AUX_H\n";
  out.close();
}

/*****************************************************************************/
/* STATIC                  generate_aux_implementation                       */
/*****************************************************************************/

static void
  generate_aux_implementation(kdd_class *classes, const char *aux_dir)
{
  char *path = new char[strlen(aux_dir)+80]; // More than enough
  strcpy(path,aux_dir);
  strcat(path,"/kdu_aux.cpp");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the auxiliary source file, \""
        << path << "\".";
    }

  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n";
  out << "#include \"kdu_aux.h\"\n";
  out << "\n";
  out << "/* ============================================ */\n";
  out << "// Special classes for use in binding callbacks\n";
  out << "/* ============================================ */\n";
  out << "\n";
  for (kdd_class *cls=classes; cls != NULL; cls=cls->next)
    {
      if ((cls->binding != KDD_BIND_REF) || (!cls->has_callbacks))
        continue;
      out << "//---------------------------------------------------\n";
      out << "// Derived version of class: " << cls->name << "\n";
      generate_derived_class_implementation(cls,out);
      out << "\n";
    }
  out.close();
}

/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/*                              make_aux_bindings                            */
/*****************************************************************************/

void make_aux_bindings(kdd_file *header_files, kdd_class *classes,
                       const char *aux_dir)
{
  generate_aux_header(header_files,classes,aux_dir);
  generate_aux_implementation(classes,aux_dir);
}
