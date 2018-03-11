/*****************************************************************************/
// File: mni_builder.cpp [scope = APPS/HYPERDOC]
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
   Implements automatic Managed Native Interface construction features offered
by the "kdu_hyperdoc" utility.
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
/* STATIC                    find_bindable_functions                         */
/*****************************************************************************/

static void
  find_bindable_functions(kdd_class *cls)
  /* This function determines whether or not each function in the supplied
     class can be associated with a managed interface, based on the binding
     information currently available.  Basically, a function is bindable if
     its class is bindable and its return value and all arguments are
     bindable, unless the function has the special KDD_BIND_NONE
     binding. */
{
  if (cls->binding == KDD_BIND_NONE)
    return;

  kdd_func *func;
  for (func=cls->functions; func != NULL; func=func->next)
    {
      if (func->binding == KDD_BIND_NONE)
        continue; // For the moment, functions which cannot be bound to Java
                  // will not be bound to Microsoft's Managed Extensions to C++
      if (cls->binding == KDD_BIND_IFC)
        {
          if (func->is_constructor)
            continue; // Do not bind any interface constructors
          if (func->is_destructor)
            { kdu_error e;
              e << "The class \"" << cls->name << "\", defined on "
                "line " << cls->line_number << " of file, \""
                << cls->file->name << " indicates that it is an "
                "interface; however, interfaces may not have destructors!";
            }
        }
      if (!func->return_type.is_bindable(true))
        continue;
      if (strncmp(func->name,"operator",8) == 0)
        continue; // Managed classes do not support operator overloading
      if (func->is_constructor && cls->is_abstract)
        continue; // Cannot instantiate abstract class
      kdd_variable *var;
      func->num_mni_args = 0;
      for (var=func->args; var != NULL; var=var->next, func->num_mni_args++)
        if (!var->type.is_bindable(false))
          break;
        else
          var->mni_bindable = true;
      if (func->binding != KDD_BIND_CALLBACK)
        for (; var != NULL; var=var->next)
          {
            var->mni_bindable = false;
            if (var->initializer == NULL)
              break;
          }
      if (var == NULL)
        {
          const char *name = func->name;
          if (func->is_destructor)
            name = "Dispose";
          else if (func->is_constructor)
            name = cls->mni_name;
          func->mni_name = new char[strlen(name)+1];
          strcpy(func->mni_name,name);
          func->file->has_bindings = true;
          if ((func->binding == KDD_BIND_CALLBACK) && func->is_virtual)
            {
              func->owner->has_callbacks = true;
              if (func->owner->binding != KDD_BIND_REF)
                { kdu_error e; e << "Error encountered at line "
                  << func->line_number << " of file, \"" << func->file->name
                  << "\"; functions with the \"[BIND: callback]\" binding "
                     "may be used only with classes having the "
                     "\"[BIND: reference]\" binding.";
                }
            }
          func->csharp_name = func->mni_name;
          func->vbasic_name = func->mni_name;
          if (func->is_constructor)
            func->vbasic_name = "New";
        }
    }
}

/*****************************************************************************/
/* STATIC                   generate_csharp_constants                        */
/*****************************************************************************/

static void
  generate_csharp_constants(kdd_class *cls, const char *mni_dir)
  /* Writes the "kdu_constants.cs" file, which defines `class Ckdu_global',
     extending imported class `Ckdu_global_funcs' by the addition of Kakadu
     constants.  The `cls' argument points to the artificial class which
     is used to hold global definitions. */
{
  assert(cls->name == NULL); // Verify that it is the globals class
  char *path = new char[strlen(mni_dir)+80]; // More than enough
  strcpy(path,mni_dir);
  strcat(path,"/kdu_constants.cs");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the C# class file, \"" << path << "\"."; }
  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n\n";
  out << "namespace kdu_mni {\n";
  out << "  public class Ckdu_global : Ckdu_global_funcs {\n";
  for (kdd_constant *cst=cls->constants; cst != NULL; cst=cst->next)
    {
      out << "    public const " << cst->type->csharp_name;
      out << " " << cst->name << " = unchecked(("
          << cst->type->csharp_name << ") " << cst->value << ");\n";
    }
  out << "  }\n";
  out << "}\n";
  out.close();
  delete[] path;
}

/*****************************************************************************/
/* STATIC                   generate_vbasic_constants                        */
/*****************************************************************************/

static void
  generate_vbasic_constants(kdd_class *cls, const char *mni_dir)
  /* Writes the "kdu_constants.vb" file, which defines `class Ckdu_global',
     extending imported class `Ckdu_global_funcs' by the addition of Kakadu
     constants.  The `cls' argument points to the artificial class which
     is used to hold global definitions. */
{
  assert(cls->name == NULL); // Verify that it is the globals class
  char *path = new char[strlen(mni_dir)+80]; // More than enough
  strcpy(path,mni_dir);
  strcat(path,"/kdu_constants.vb");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the VB class file, \"" << path << "\"."; }
  out << "' This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "' Do not edit manually.\n";
  out << "' Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n\n";
  out << "Namespace kdu_mni\n";

  out << "  Public Class Ckdu_lobal\n";
  out << "    Inherits Ckdu_global_funcs\n";

  for (kdd_constant *cst=cls->constants; cst != NULL; cst=cst->next)
    {
      // Before writing the constant value, we will do a little work to
      // convert integers to a form which Visual Basic will accept
      const char *cp, *value = cst->value;
      kdu_long ival = 0;
      bool is_hex = false;
      for (cp=value; *cp != '\0'; cp++)
        {
          if ((*cp >= '0') && (*cp <= '9'))
            ival = (ival * ((is_hex)?16:10)) + (((int) *cp) - ((int) '0'));
          else if (is_hex && (*cp >= 'a') && (*cp <= 'f'))
            ival = (ival * 16) + 10 + (((int) *cp) - ((int) 'a'));
          else if (is_hex && (*cp >= 'A') && (*cp <= 'F'))
            ival = (ival * 16) + 10 + (((int) *cp) - ((int) 'A'));
          else if ((*cp == 'x') || (*cp == 'X'))
            {
              if (is_hex || (cp != (value+1)) || (cp[-1] != '0'))
                break;
              is_hex = true;
            }
          else
            break;
        }
      char ibuf[80];
      if (*cp == '\0')
        { // We have found a valid integer.
          kdu_long max_ival = 0;
          if (strcmp(cst->type->mni_name,"Byte") == 0)
            max_ival = 127;
          else if (strcmp(cst->type->mni_name,"Int16") == 0)
            max_ival = 0x7FFF;
          else if (strcmp(cst->type->mni_name,"Int32") == 0)
            max_ival = 0x7FFFFFFF;
          if ((max_ival > 0) && (ival > max_ival))
            ival -= (max_ival + max_ival + 2); // Wrap around to negative value

          char *op=ibuf;
          kdu_long abs_val = ival;
          if (ival < 0)
            {
              *(op++) = '-';
              abs_val = -ival;
            }
          int digits=1;
          for (kdu_long tmp=abs_val; tmp >= 10; digits++)
            tmp /= 10;
          op[digits] = '\0';
          while (digits > 0)
            {
              digits--;
              op[digits] = (char)((abs_val % 10) + '0');
              abs_val /= 10;
            }
          value = ibuf;
        }

      out << "    Public Const " << cst->name
          << " As " << cst->type->vbasic_name
          << " = " <<  value << "\n";
    }

  out << "  End Class\n";
  out << "End Namespace\n";
  out.close();
  delete[] path;
}

/*****************************************************************************/
/* STATIC                  implement_mni_to_native_ifc                       */
/*****************************************************************************/

static void
  implement_mni_to_native_ifc(kdd_class *cls, const char *mni_obj,
                              const char *native_obj, ostream &out,
                              bool old_syntax, bool check_for_null=false,
                              const char *mni_prefix="")
  /* This function generates a fragment of C++ code which copies the
     Kakadu interface embedded within the managed object referenced by
     variable `mni_obj', to the variable named `native_obj'. */
{
  assert(cls->binding == KDD_BIND_IFC);
  out << "    void *" << native_obj << "_S = ";
  if (check_for_null)
    {
      if (old_syntax)
        out << "((" << mni_prefix << mni_obj << "==NULL)?NULL:";
      else
        out << "((" << mni_prefix << mni_obj << "==nullptr)?NULL:";
    }
  out << mni_prefix << mni_obj << "->_kval.ToPointer()";
  out << ((check_for_null)?");\n":";\n");
  out << "    " << native_obj << " = *((" << cls->name  << " *)"
         "(&" << native_obj << "_S));\n";
}

/*****************************************************************************/
/* STATIC                implement_native_ifc_to_mni                         */
/*****************************************************************************/

static void
  implement_native_ifc_to_mni(const char *native_obj,
                              const char *mni_obj, ostream &out,
                              const char *mni_prefix="")
  /* This function generates a fragment of C++ code which copies the
     native Kakadu interface object, with name `native_obj' to the
     managed object named `mni_obj'.  Essentially this copies the
     value of the interface (which is just a pointer) to the managed
     object's `_kval' member. */
{
  out << "    " << mni_prefix << mni_obj
      << "->_kval = IntPtr(*((void **)(&" << native_obj << ")));\n";
}

/*****************************************************************************/
/* STATIC           implement_call_mni_virtual_with_defaults                 */
/*****************************************************************************/

static void
  implement_call_mni_virtual_with_defaults(kdd_func *func, int num_args,
                                           ostream &out, bool old_syntax)
  /* Used to implement default arguments for virtual functions.  These
     cannot be implemented by simply creating a version of the external
     interface function which has a reduced number of arguments, since then
     overrides to the core virtual function will not properly override
     all the variants produced by default arguments.  To solve the problem,
     we create member functions for each defaulted variant and
     then invoke the single virtual function (with a full set of arguments)
     from the corresponding function, passing in the default values.
     While calling the managed function with a reduced set of arguments is
     conceptually simple, it means that we have to translate C++ default
     parameters into their managed equivalent, which is not necessarily
     trivial.  `num_args' is the number of arguments which are not defaulted
     here. */
{
  kdd_variable *arg;
  for (arg=func->args; num_args > 0; num_args--, arg=arg->next);
  assert(arg != NULL); // Should point to the first default argument
  for (; (arg != NULL) && arg->mni_bindable; arg=arg->next)
    { // Create a managed version of each missing argument
      if (arg->initializer == NULL)
        { kdu_error e; e << "Error at line "
          << func->line_number << " in file, \""
          << func->file->name << "\"; expected argument \""
          << arg->name << "\" to have a default value.";
        }
      out << "    ";
      arg->type.write_mni_declaration(out,arg->name,old_syntax,"_");
      out << " = ";
      bool initializer_reconciled=true;
      if (strcmp(arg->initializer,"NULL") == 0)
        { // Have to be a bit careful about NULL initializers, since GC
          // objects need to be assigned a value of "nullptr" in Managed
          // Extensions to C++ V2.
          if (old_syntax)
            out << "NULL"; // Good generic initializer
          else if (arg->type.known_class != NULL)
            out << "nullptr";
          else if ((arg->type.known_primitive != NULL) &&
                   (arg->type.known_primitive->string_type ||
                    (((arg->type.as_array+arg->type.derefs)==1) &&
                     !arg->type.by_ref)))
            out << "nullptr";
          else
            out << "NULL"; // Not a GC object; safe to initialize as NULL
        }
      else if (arg->type.known_class != NULL)
        {
          const char *cp1=arg->initializer;
          const char *cp2=arg->type.known_class->name;
          for (; (*cp1 != '\0') && (*cp1 == *cp2); cp1++, cp2++);
          if (*cp2 == '\0')
            {
              if (old_syntax)
                out << "__gc new " << arg->type.known_class->mni_name << cp1;
              else
                out << "gcnew " << arg->type.known_class->mni_name << cp1;
            }
          else
            initializer_reconciled = false;
        }
      else if ((arg->type.known_primitive != NULL) &&
               arg->type.known_primitive->string_type)
        {
          if (old_syntax)
            out << "__gc new String((char *)(" << arg->initializer << "))";
          else
            out << "gcnew String((char *)(" << arg->initializer << "))";
        }
      else if ((arg->type.known_primitive != NULL) &&
               ((arg->type.as_array+arg->type.derefs)==0) &&
               !arg->type.by_ref)
        out << "(" << arg->type.known_primitive->mni_name
            << ")(" << arg->initializer << ")";
      else
        initializer_reconciled = false;

      if (!initializer_reconciled)      
        { kdu_error e; e << "Error at line "
          << func->line_number << " in file, \""
          << func->file->name << "\"; unable to convert default value for "
          << "argument \"" << arg->name
          << "\" to a corresponding managed type.";
        }

      out << ";\n";
    }
  out << "    ";
  if (!func->return_type.is_void())
    out << "return ";
  out << func->mni_name << "(";
  int nargs = func->num_mni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    {
      out << "_" << arg->name;
      if (nargs > 1)
        out << ",";
    }
  out << ");\n";
}

/*****************************************************************************/
/* STATIC              implement_managed_return_default                      */
/*****************************************************************************/

static void
  implement_managed_return_default(kdd_type &return_type, ostream &out,
                                   bool old_syntax)
{
  if (return_type.is_void())
    return;
  if (return_type.known_class != NULL)
    { // Return a null object
      if (old_syntax)
        out << "    return NULL;\n";
      else
        out << "    return nullptr;\n";
    }
  else if (return_type.known_primitive != NULL)
    {
      if (return_type.bind_as_opaque_pointer)
        out << "    IntPtr(NULL);\n";
      else if (return_type.known_primitive->string_type)
        {
          if (old_syntax)
            out << "    return NULL;\n";
          else
            out << "    return nullptr;\n";
        }
      else
        out << "    return (" << return_type.known_primitive->mni_name
            << ") 0;\n";
    }
  else if (return_type.unknown_name != NULL)
    out << "    IntPtr(NULL);\n";
}

/*****************************************************************************/
/* STATIC               implement_native_return_default                      */
/*****************************************************************************/

static void
  implement_native_return_default(kdd_type &return_type, ostream &out)
{
  if (return_type.is_void())
    return;
  if ((return_type.as_array > 0) || (return_type.derefs > 0))
    out << "    return NULL;\n";
  else if (return_type.known_class != NULL)
    out << "    return " << return_type.known_class->name << "();\n";
  else if (return_type.known_primitive != NULL)
    {
      if (return_type.known_primitive->string_type)
        out << "    return NULL;\n";
      else
        out << "    return ("<<return_type.known_primitive->name<<") 0;\n";
    }
  else if (return_type.unknown_name != NULL)
    out << "    return NULL;\n";
}

/*****************************************************************************/
/* STATIC                     implement_args_pass                            */
/*****************************************************************************/

static void
  implement_args_pass(kdd_variable *args, int num_args, ostream &out)
{
  kdd_variable *arg;
  for (arg=args; num_args > 0; num_args--, arg=arg->next)
    {
      out << arg->name;
      if (num_args > 1)
        out << ",";
    }
}

/*****************************************************************************/
/* STATIC                     implement_args_in                              */
/*****************************************************************************/

static bool
  implement_args_in(kdd_variable *args, int num_args, ostream &out,
                    bool old_syntax)
  /* The function returns true if any temporary arrays were created.  In
     this case, the native function call needs to be enclosed in a try/catch
     statement so that the created resources can be cleaned up. */
{
  bool need_cleanup=false;
  kdd_variable *arg;
  for (arg=args; num_args > 0; num_args--, arg=arg->next)
    {
      kdd_type &type = arg->type;
      if (type.known_class != NULL)
        {
          kdd_class *cls = type.known_class;
          assert(type.as_array == 0);
                // Functions which pass arrays of objects are not bindable
          if (cls->binding == KDD_BIND_IFC)
            { // Passing interface either by copy or by reference.  Both look
              // the same in implementation, since interfaces are embedded
              // inside managed objects, allowing altered values to be
              // passed back to the caller if required.
              out << "    " << cls->name << " " << arg->name << ";\n";
              implement_mni_to_native_ifc(cls,arg->name,arg->name,
                                          out,old_syntax,true,"_");
            }
          else if ((type.derefs == 0) && !type.by_ref)
            { // Passing object by copy
              assert(cls->binding == KDD_BIND_COPY);
              out << "    " << cls->name << " " << arg->name
                  << " = *(_" << arg->name << "->_get_kref());\n";
            }
          else if ((type.derefs == 0) && type.by_ref)
            { // Passing object by reference
              out << "    " << cls->name << " &" << arg->name
                  << " = *(_" << arg->name << "->_get_kref());\n";
            }
          else if ((type.derefs == 1) && !type.by_ref)
            { // Passing object pointer
              if (old_syntax)
                out << "    " << cls->name << " *" << arg->name
                    << " = ((_" << arg->name << "==NULL)?NULL:(_"
                    << arg->name << "->_get_kref()));\n";
              else
                out << "    " << cls->name << " *" << arg->name
                    << " = ((_" << arg->name << "==nullptr)?NULL:(_"
                    << arg->name << "->_get_kref()));\n";
            }
          else
            assert(0);
        }
      else if (type.known_primitive != NULL)
        {
          if (type.bind_as_opaque_pointer)
            { // Donated array treated as abstract pointer
              out << "    " << type.known_primitive->name << " *" << arg->name
                  << " = (" << type.known_primitive->name << " *)(_"
                  << arg->name << ".ToPointer());\n";
            }
          else if ((type.derefs+type.as_array) != 0)
            { // Argument passed as GC array
              assert(((type.derefs+type.as_array) == 1) && !type.by_ref);
              if (type.known_primitive->mni_size_compatible)
                { // Just pin the array and pass the pinned address directly
                  if (old_syntax)
                    {
                      out << "    " << type.known_primitive->mni_name
                          << " __pin* " << arg->name << " = ";
                      out << "((_" << arg->name << "==NULL)?NULL:"
                             "(&_" << arg->name << "[0]));\n";
                    }
                  else
                    {
                      out << "    cli::pin_ptr<"
                        << type.known_primitive->mni_name
                        << "> " << arg->name << " = ";
                      out << "((_" << arg->name << "==nullptr)?nullptr:"
                             "(&_" << arg->name << "[0]));\n";
                    }
                }
              else
                { // Create a temporary array to hold results
                  need_cleanup = true;
                  out << "    " << type.known_primitive->name
                      << " *" << arg->name << " = NULL;\n";
                  if (old_syntax)
                    out << "    if (_" << arg->name << " != NULL)\n";
                  else
                    out << "    if (_" << arg->name << " != nullptr)\n";
                  out << "      {\n";
                  out << "        int _len_ = _" << arg->name << "->Length;\n";
                  out << "        " << arg->name << " = new "
                      << type.known_primitive->name << "[_len_];\n";
                  out << "        for (int _n_=0; _n_ < _len_; _n_++)\n";
                  out << "          " << arg->name << "[_n_] = ("
                      << type.known_primitive->name << ") _"
                      << arg->name << "[_n_];\n";
                  out << "      }\n";
                }
            }
          else if (type.by_ref)
            { // Primitive type passed by reference
              out << "    " << type.known_primitive->name << " " << arg->name
                  << " = (" << type.known_primitive->name
                  << ") *_" << arg->name << ";\n";
            }
          else if (type.known_primitive->string_type)
            { // Pass string across as copied ASCII text
              need_cleanup = true;
              out << "    char * " << arg->name << " = NULL;\n";
              if (old_syntax)
                out << "    if (_" << arg->name << " != NULL)\n";
              else
                out << "    if (_" << arg->name << " != nullptr)\n";
              out << "      {\n";
              out << "        " << arg->name << " = new char[(_"
                  << arg->name << "->Length+1)*2];\n";
              if (old_syntax)
                out << "        const wchar_t __pin* " << arg->name
                    << "_W = PtrToStringChars(_" << arg->name << ");\n";
              else
                out << "        cli::pin_ptr<const wchar_t> " << arg->name
                    << "_W = PtrToStringChars(_" << arg->name << ");\n";
              out << "        wcstombs(" << arg->name << "," << arg->name
                  << "_W,(_" << arg->name << "->Length+1)*2);\n";
              out << "      }\n";
            }
          else
            { // Pass primitive type directly
              out << "    " << type.known_primitive->name << " " << arg->name
                  << " = (" << type.known_primitive->name
                  << ") _" << arg->name << ";\n";
            }
        }
      else if (type.unknown_name != NULL)
        {
          assert((type.as_array == 0) && (type.derefs == 1));
          out << "    " << type.unknown_name << " *" << arg->name
              << " = (" << type.unknown_name << " *) _" << arg->name;
          if (type.by_ref)
            out << "->ToPointer();\n";
          else
            out << ".ToPointer();\n";
        }
      else
        assert(0);
    }
  return need_cleanup;
}

/*****************************************************************************/
/* STATIC                     implement_args_out                             */
/*****************************************************************************/

static void
  implement_args_out(kdd_variable *args, int num_args, ostream &out,
                     bool old_syntax)
{
  kdd_variable *arg;
  for (arg=args; num_args > 0; num_args--, arg=arg->next)
    {
      kdd_type &type = arg->type;
      if (type.known_class != NULL)
        {
          kdd_class *cls = type.known_class;
          assert(type.as_array == 0);
                // Functions which pass arrays of objects are not bindable
          if (cls->binding == KDD_BIND_IFC)
            { // Interface passed either by copy or by reference.
              if (type.by_ref)
                implement_native_ifc_to_mni(arg->name,arg->name,out,"_");
            }
          else if ((type.derefs == 0) && !type.by_ref)
            { // Object passed by copy
              assert(cls->binding == KDD_BIND_COPY);
              continue; // Nothing to do
            }
          else if ((type.derefs == 0) && type.by_ref)
            { // Object passed by reference
              continue; // Nothing to do
            }
          else if ((type.derefs == 1) && !type.by_ref)
            { // Object passed by pointer
              if (arg->type.binding == KDD_BIND_DONATE)
                out << "    _" << arg->name << "->_natively_owned=true;\n";
            }
          else
            assert(0);
        }
      else if (type.known_primitive != NULL)
        {
          if (type.bind_as_opaque_pointer)
            { // Donated array treated as abstract pointer
              continue; // Nothing to do
            }
          else if ((type.derefs+type.as_array) != 0)
            { // Argument passed as GC array
              if (!type.known_primitive->mni_size_compatible)
                { // Copy results back from temporary array
                  if (old_syntax)
                    out << "    if (_" << arg->name << " != NULL)\n";
                  else
                    out << "    if (_" << arg->name << " != nullptr)\n";
                  out << "      {\n";
                  out << "        int _len_ = _" << arg->name << "->Length;\n";
                  out << "        for (int _n_=0; _n_ < _len_; _n_++)\n";
                  out << "          _" << arg->name << "[_n_] = ("
                      << type.known_primitive->mni_name << ") "
                      << arg->name << "[_n_];\n";
                  out << "      }\n";
                }
            }
          else if (type.by_ref)
            { // Primitive type passed by reference
              out << "    *_" << arg->name << " = " << arg->name << ";\n";
            }
        }
      else if (type.unknown_name != NULL)
        {
          if (type.by_ref)
            out << "    _" << arg->name << " = IntPtr((void *)"
                << arg->name << ");\n";
        }
      else
        assert(0);
    }
}

/*****************************************************************************/
/* STATIC                   implement_args_cleanup                           */
/*****************************************************************************/

static void
  implement_args_cleanup(kdd_variable *args, int num_args, ostream &out)
{
  kdd_variable *arg;
  for (arg=args; num_args > 0; num_args--, arg=arg->next)
    {
      kdd_type &type = arg->type;
      if (type.known_primitive == NULL)
        continue;
      if (type.bind_as_opaque_pointer)
        {
          continue; // Nothing to do
        }
      else if ((type.derefs+type.as_array) != 0)
        { // Argument passed as __gc array
          if (!type.known_primitive->mni_size_compatible)
            { // Delete temporary array
              out << "    if (" << arg->name << " != NULL)\n";
              out << "      delete[] " << arg->name << ";\n";
            }
        }
      else if (type.known_primitive->string_type)
        { // String passed across as ASCII text; need to delete the
          // ASCII string we allocated to hold the converted result.
          out << "    if (" << arg->name << " != NULL)\n";
          out << "      delete[] " << arg->name << ";\n";
        }
    }
}

/*****************************************************************************/
/* STATIC                    implement_result_delare                         */
/*****************************************************************************/

static void
  implement_result_declare(kdd_type &return_type, ostream &out)
{
  if (!return_type.is_void())
    {
      out << "    ";
      return_type.print(out,"Result");
      out << ";\n";
    }
}

/*****************************************************************************/
/* STATIC                  implement_result_assignment                       */
/*****************************************************************************/

static void
  implement_result_assignment(kdd_type &return_type, ostream &out)
{
  out << "    ";
  if (!return_type.is_void())
    out << "Result = ";
}

/*****************************************************************************/
/* STATIC                   implement_return_result                          */
/*****************************************************************************/

static void
  implement_return_result(kdd_func *func, ostream &out, bool old_syntax)
{
  kdd_type &return_type = func->return_type;
  if (return_type.is_void())
    return;
  if (return_type.known_class != NULL)
    { // Create and return a managed object of the relevant class
      if (return_type.known_class->has_callbacks)
        { kdu_error e; e << "Error at line "
          << func->line_number << " in file, \""
          << func->file->name << "\"; functions may not "
          "return objects which contain functions having the "
          "\"[BIND: callback]\" binding.";
        }
      assert((return_type.as_array == 0) && !return_type.by_ref);
      if (return_type.known_class->binding == KDD_BIND_IFC)
        {
          assert(return_type.derefs == 0);
          if (old_syntax)
            out << "    " << return_type.known_class->mni_name <<
                   " *_Result = __gc new "
                << return_type.known_class->mni_name << ";\n";
          else
            out << "    " << return_type.known_class->mni_name <<
                   " ^_Result = gcnew "
                << return_type.known_class->mni_name << ";\n";
          implement_native_ifc_to_mni("Result","_Result",out);
          out << "    return _Result;\n";
        }
      else if ((return_type.known_class->binding == KDD_BIND_COPY) &&
               (return_type.derefs == 0))
        {
          if (old_syntax)
            out << "    " << return_type.known_class->mni_name <<
                   " *_Result = new "
                << return_type.known_class->mni_name << ";\n";
          else
            out << "    " << return_type.known_class->mni_name <<
                   " ^_Result = gcnew "
                << return_type.known_class->mni_name << ";\n";
          out << "    *(_Result->_get_kref()) = Result;\n";
          out << "    return _Result;\n";
        }
      else
        {
          if (old_syntax)
            out << "    if (Result == NULL)\n"
                   "      return NULL;\n"
                   "    else\n"
                   "      return new " << return_type.known_class->mni_name
                << "(Result);\n";
          else
            out << "    if (Result == NULL)\n"
                   "      return nullptr;\n"
                   "    else\n"
                   "      return gcnew " << return_type.known_class->mni_name
                << "(Result);\n";
        }
    }
  else if (return_type.known_primitive != NULL)
    {
      if (return_type.bind_as_opaque_pointer)
        { // Returned array treated as abstract pointer
          out << "    return IntPtr((void *) Result);\n";
        }
      else if (return_type.known_primitive->string_type)
        {
          if (old_syntax)
            out << "    return __gc new String((char *) Result);\n";
          else
            out << "    return gcnew String((char *) Result);\n";
        }
      else
        {
          assert(return_type.derefs == 0);
          out << "    return Result;\n";
        }
    }
  else if (return_type.unknown_name != NULL)
    {
      assert((return_type.derefs == 1) &&
             !(return_type.as_array || return_type.by_ref));
      out << "    return IntPtr((void *) Result);\n";
    }
}

/*****************************************************************************/
/* STATIC                generate_mni_class_declaration                      */
/*****************************************************************************/

static void
  generate_mni_class_declaration(kdd_class *cls, ostream &out,
                                 bool old_syntax)
{
  bool has_managed_base =
    (cls->base.known_class != NULL) &&
    (cls->base.known_class->mni_name != NULL);

  out << "  //---------------------------------------------------\n";
  out << "  // Managed Class: " << cls->mni_name << "\n";
  if (old_syntax)
    out << "  public __gc class " << cls->mni_name;
  else
    out << "  public ref class " << cls->mni_name;

  bool need_destructor =
    ((cls->binding == KDD_BIND_COPY) || (cls->binding == KDD_BIND_REF)) &&
    (!has_managed_base) && (!cls->destroy_private);

  bool need_do_dispose = // If a `Dispose(bool)' function is required
    ((cls->binding == KDD_BIND_COPY) || (cls->binding == KDD_BIND_REF)) &&
    ((!has_managed_base) || !cls->destroy_virtual) &&
    (!cls->destroy_private);
  if (cls->has_callbacks ||
      (has_managed_base && cls->base.known_class->has_callbacks))
    need_do_dispose = true; // Need our own `Dispose(bool)' override to ensure
                   // that an attempt to dispose a delegator is made if and
                   // only if one exists.

  if (has_managed_base)
    out << " : public " << cls->base.known_class->mni_name;
  else if (need_destructor && old_syntax)
    out << " : public IDisposable";
  out << " {\n";
  if (cls->binding == KDD_BIND_IFC)
    {
      assert(cls->name != NULL);
      assert(!has_managed_base);
      out << "  protected public:\n"
          << "    IntPtr _kval;\n";
    }
  else if (cls->name != NULL)
    {
      if (!has_managed_base)
        {
          out << "  protected:\n"
              << "    " << cls->name << " * _kref;\n";
          out << "  protected public:\n";
          out << "    bool _natively_owned;\n";
          if (need_destructor)
            {
              if (old_syntax)
                out << "  public:\n"
                       "    void Dispose()\n"
                       "      {\n"
                       "        Dispose(true);\n"
                       "        // GC::SuppressFinalize called by above func\n"
                       "      }\n";
              else
                out << "  public:\n"
                       "    ~" << cls->mni_name << "()\n"
                       "      {\n"
                       "        Do_dispose(true);\n"
                       "        // GC::SuppressFinalize implicit call here\n"
                       "      }\n";
            }

        }
      if (need_do_dispose)
        {
          if (old_syntax)
            out << "  protected:\n"
                   "    virtual void Dispose(bool in_dispose);\n";
          else if (has_managed_base)
            out << "  protected:\n"
                   "    virtual void Do_dispose(bool in_dispose) override;\n";
          else
            out << "  protected:\n"
                   "    virtual void Do_dispose(bool in_dispose);\n";
        }
      out << "  protected public:\n";
      out << "    " << cls->name << " * _get_kref() { return ("
          << cls->name << " *) _kref; }\n";
      out << "    " << cls->mni_name << "(" << cls->name
          << " * _existing_kref)\n";
      if (!has_managed_base)
        {
          out << "      {\n"
                 "        this->_kref=_existing_kref;\n"
                 "        _natively_owned=true;\n";
          out << "      }\n";
        }
      else
        {
          out << "        : " << cls->base.known_class->mni_name
              << "(_existing_kref)\n";
          out << "      { return; }\n";
        }
    }

  out << "  public:\n";

  if (need_destructor)
    {
      if (old_syntax)
        out << "    ~" << cls->mni_name << "() { Dispose(false); }\n";
      else
        out << "    !" << cls->mni_name << "() { Do_dispose(false); }\n";
    }

  int num_args, min_args, max_args, n;
  kdd_variable *arg;
  kdd_func *func;
  for (func=cls->functions; func != NULL; func=func->next)
    {
      if (func->mni_name == NULL)
        continue;
      if (func->is_destructor)
        continue; // Destructor is implemented separately, if required
      for (min_args=0, arg=func->args;
           (arg != NULL) && (arg->initializer==NULL);
           arg=arg->next, min_args++);
      max_args = func->num_mni_args;   assert(max_args >= min_args);
      for (num_args=max_args; num_args >= min_args; num_args--)
        {
          out << "    ";
          if ((func->property != NULL) && old_syntax)
            out << "__property ";
          if (func->owner->name == NULL)
            out << "static ";
          else if (func->is_virtual && (num_args == max_args))
            out << "virtual ";
          func->return_type.write_mni_declaration(out,NULL,old_syntax);
          if ((func->property != NULL) && !old_syntax)
            out << "__property_"; // True func name is reserved in new syntax
          out << func->mni_name << "(";
          for (arg=func->args, n=num_args; n > 0; n--, arg=arg->next)
            {
              arg->type.write_mni_declaration(out,arg->name,old_syntax);
              if (n > 1)
                out << ", ";
            }
          out << ")";
          if ((!old_syntax) && func->is_virtual && (num_args == max_args) &&
              (func->overrides != NULL) && (func->overrides->mni_name != NULL))
            out << " override";
          out << ";\n";
        }
    }
  if (!old_syntax)
    { // Generate property accessors
      kdd_variable *var;
      for (var=cls->variables; var != NULL; var=var->next)
        if ((var->property_get != NULL) || (var->property_set != NULL))
          {
            out << "    property ";
            var->type.write_mni_declaration(out,var->name,old_syntax);
            out << " {\n";
            if ((func = var->property_get) != NULL)
              {
                out << "        ";
                func->return_type.write_mni_declaration(out,NULL,old_syntax);
                out << "get() { return __property_"
                    << func->mni_name << "(); }\n";
              }
            if ((func = var->property_set) != NULL)
              {
                out << "        ";
                out << "void set(";
                var->type.write_mni_declaration(out,var->name,old_syntax);
                out << ") { __property_"
                    << func->mni_name << "(" << var->name << "); }\n";
              }
            out << "      }\n";
          }
    }
  out << "  };\n";
}

/*****************************************************************************/
/* STATIC                      generate_mni_header                           */
/*****************************************************************************/

static void
  generate_mni_header(kdd_class *classes, const char *mni_dir,
                      bool old_syntax)
{
  char *path = new char[strlen(mni_dir)+80]; // More than enough
  strcpy(path,mni_dir);
  strcat(path,"/kdu_mni.h");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the managed C++ extensions header file, \""
        << path << "\".";
    }

  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n\n";
  out << "#ifndef KDU_MNI_H\n";
  out << "using namespace System;\n";
  out << "namespace kdu_mni {\n";

  // Start by writing a list of managed classes, whose declarations
  // will appear later
  kdd_class *cls;
  out << "  // Managed classes defined here.\n";
  for (cls=classes; cls != NULL; cls=cls->next)
    if (cls->mni_name != NULL)
      {
        assert(cls->binding != KDD_BIND_NONE);
        if (old_syntax)
          out << "  public __gc class " << cls->mni_name << ";\n";
        else
          out << "  ref class " << cls->mni_name << ";\n";
      }
  out << "\n";

  // Now write a list of unmanaged extension classes which will be used
  // to implement members with CALLBACK bindings.
  out << "  // Derived versions of unmanaged classes, "
         "declared in \"kdu_mni.cpp\"\n";
  for (cls=classes; cls != NULL; cls=cls->next)
    if ((cls->mni_name != NULL) && cls->has_callbacks)
      out << "  class " MNI_DELEGATOR_PREFIX << cls->name << ";\n";
  out << "\n";

  // Finally write the class declarations for all managed classes
  int derivation_depth=0;
  bool done = false;
  for (; !done; derivation_depth++)
    { // Make sure we write less derived class declarations before more
      // derived declarations
      done = true;
      for (cls=classes; cls != NULL; cls=cls->next)
        {
          if (cls->mni_name == NULL)
            continue;
          int depth = 0;
          for (kdd_class *scan=cls->base.known_class; scan != NULL; depth++)
            scan = scan->base.known_class;
          if (depth >= derivation_depth)
            done = false;
          if (depth == derivation_depth)
            generate_mni_class_declaration(cls,out,old_syntax);
        }
    }
  out << "} // End of namespace \"kdu_mni\"\n";
  out << "#endif // KDU_MNI_H\n";
  out.close();
  delete[] path;
}

/*****************************************************************************/
/* STATIC                 implement_callback_args_in                         */
/*****************************************************************************/

static void
  implement_callback_args_in(kdd_func *func, ostream &out, bool old_syntax)
{
  kdd_variable *arg;
  int nargs = func->num_mni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    {
      kdd_type &type = arg->type;
      if ((type.known_class != NULL) &&
          (type.known_class->binding == KDD_BIND_IFC) &&
          ((type.derefs+type.as_array) == 0) && !type.by_ref)
        { // Interface object passed by value
          if (old_syntax)
            out << "      " << type.known_class->mni_name << " *_" << arg->name
                << " = __gc new " << type.known_class->mni_name << ";\n";
          else
            out << "      " << type.known_class->mni_name << " ^_" << arg->name
                << " = gcnew " << type.known_class->mni_name << ";\n";
          implement_native_ifc_to_mni(arg->name,arg->name,out,"_");
        }
      else if ((type.known_class != NULL) &&
               (type.known_class->binding == KDD_BIND_COPY) &&
               (type.derefs == 0) && (type.as_array == 0) && !type.by_ref)
        { // Copy-able object passed by value; make a managed copy
          out << "      " << type.known_class->name << " *_BUF_"
              << arg->name << " = new " << type.known_class->name
              << "; *_BUF_" << arg->name << " = " << arg->name << ";\n";
          if (old_syntax)
            out << "      " << type.known_class->mni_name << " *_" << arg->name
                << " = __gc new " << type.known_class->mni_name << "(_BUF_"
                << arg->name << ");\n";
          else
            out << "      " << type.known_class->mni_name << " ^_" << arg->name
                << " = gcnew " << type.known_class->mni_name << "(_BUF_"
                << arg->name << ");\n";
          out << "      _" << arg->name << "->_natively_owned = false;\n";
        }
      else if ((type.known_class != NULL) &&
               (type.known_class->binding != KDD_BIND_IFC) &&
               (type.as_array == 0) &&
               ((type.derefs + ((type.by_ref)?1:0)) == 1))
        { // Non-interface object passed by reference or pointer.
          if (old_syntax)
            {
              out << "      " << type.known_class->mni_name
                  << " *_" << arg->name;
              if (type.derefs  == 1)
                {
                  out << " = NULL;\n";
                  out << "      if (" << arg->name << " != NULL)\n";
                  out << "        _" << arg->name;
                }
              out << " = __gc new " << type.known_class->mni_name << "(";
            }
          else
            {
              out << "      " << type.known_class->mni_name
                  << " ^_" << arg->name;
              if (type.derefs  == 1)
                {
                  out << " = nullptr;\n";
                  out << "      if (" << arg->name << " != NULL)\n";
                  out << "        _" << arg->name;
                }
              out << " = gcnew " << type.known_class->mni_name << "(";
            }
          if (type.derefs == 0)
            out << "&";
          out << arg->name << ");\n";
        }
      else if ((type.known_primitive != NULL) &&
               type.known_primitive->string_type)
        {
          if (old_syntax)
            {
              out << "      String *_" << arg->name << " = NULL;\n";
              out << "      if (" << arg->name << " != NULL)\n";
              out << "       _" << arg->name
                  << " = __gc new String((char *) " << arg->name << ");\n";
            }
          else
            {
              out << "      String ^_" << arg->name << " = nullptr;\n";
              out << "      if (" << arg->name << " != NULL)\n";
              out << "       _" << arg->name
                  << " = gcnew String((char *) " << arg->name << ");\n";
            }
        }
      else if ((type.known_primitive != NULL) &&
               ((type.derefs+type.as_array) == 0))
        { // Primitive type passed by value or by reference
          out << "      " << type.known_primitive->mni_name << " _"
              << arg->name << " = (" << type.known_primitive->mni_name
              << ") " << arg->name << ";\n";
        }
      else if ((type.known_primitive != NULL) && type.bind_as_opaque_pointer)
        { // Primitive array passed as opaque pointer
          out << "      IntPtr _" << arg->name << " = IntPtr((void *) "
              << arg->name << ");\n";
        }
      else if ((type.unknown_name != NULL) && (type.as_array == 0) &&
               (type.derefs == 1))
        { // Unknown pointer passed directly or by reference
          out << "      IntPtr _" << arg->name << " = IntPtr((void *) "
              << arg->name << ");\n";
        }
      else
        { kdu_error e; e << "Error at line " << func->line_number
          << " in file, \"" << func->file->name << "\"; arguments of "
          "functions having the \"[BIND: callback]\" binding must have "
          "one of the following forms: 1) primitive type passed by "
          "value or reference; 2) interface class passed by value; "
          "3) known non-interface class passed by value, reference "
          "or pointer (i.e., at most one level of indirection); 4) "
          "donated primitive type array (must be marked as [BIND: donate]); "
          "or 5) unknown object pointer passed by value or reference.";
        }
    }
}

/*****************************************************************************/
/* STATIC                 implement_callback_args_out                        */
/*****************************************************************************/

static void
  implement_callback_args_out(kdd_func *func, ostream &out)
{
  int nargs = func->num_mni_args;
  kdd_variable *arg;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    {
      kdd_type &type = arg->type;
      if (!type.by_ref)
        continue;
      if ((type.known_primitive != NULL) && !type.bind_as_opaque_pointer)
        { // Primitive type passed by reference
          assert((type.derefs + type.as_array) == 0);
          out << "      " << arg->name << " = (" << type.known_primitive->name
              << ") _" << arg->name << ";\n";
        }
      else if (type.unknown_name != NULL)
        { // Unknown pointer passed by reference
          assert((type.derefs == 1) && (type.as_array == 0));
          out << "      " << arg->name << " = (" << type.unknown_name << "*)(_"
              << arg->name << ".ToPointer());\n";
        }
    }
}

/*****************************************************************************/
/* STATIC              implement_callback_return_result                      */
/*****************************************************************************/

static void
  implement_callback_return_result(kdd_func *func, ostream &out,
                                   bool old_syntax)
{
  kdd_type &type = func->return_type;
  if (type.is_void())
    {
      out << "      return;\n";
      return;
    }
  assert(!type.by_ref); // No bindable function can return reference types.
  if ((type.known_class != NULL) &&
      (type.known_class->binding == KDD_BIND_IFC) &&
      ((type.derefs+type.as_array) == 0))
    { // Returning an interface object by value
      out << "      " << type.known_class->name << " Result;\n";
      implement_mni_to_native_ifc(type.known_class,
                                  "_Result","Result",out,old_syntax,true);
      out << "      return Result;\n";
    }
  else if ((type.known_class != NULL) &&
           (type.known_class->binding != KDD_BIND_COPY) &&
           ((type.derefs+type.as_array) == 0))
    { // Returning an object which can be copied by value
      out << "      return *(_Result->_get_kref());\n";
    }
  else if ((type.known_class != NULL) &&
           (type.known_class->binding != KDD_BIND_IFC) &&
           (type.derefs == 1) && (type.as_array == 0))
    { // Returning a non-interface object by pointer
      if (old_syntax)
        out << "      return "
               "((_Result==NULL)?NULL:(_Result->_get_kref()));\n";
      else
        out << "      return "
               "((_Result==nullptr)?NULL:(_Result->_get_kref()));\n";
    }
  else if ((type.known_primitive != NULL) &&
           ((type.derefs+type.as_array)==0) &&
           !type.known_primitive->string_type)
    { // Returning a simple primitive by value
      out << "      return (" << type.known_primitive->name << ") _Result;\n";
    }
  else if ((type.known_primitive != NULL) && type.bind_as_opaque_pointer)
    { // Returning primitive array as opaque pointer
      assert(type.derefs==1);
      out << "      return (" << type.known_primitive->name << " *)("
          << "_Result.ToPointer());\n";
    }
  else if ((type.unknown_name != NULL) &&
           (type.derefs==1) && (type.as_array==0))
    { // Returning an opaque pointer
      out << "      return (" << type.unknown_name << " *)("
          << "_Result.ToPointer());\n";
    }
  else
    { kdu_error e; e << "Error at line " << func->line_number
        << " in file, \"" << func->file->name << "\"; functions having the "
        "\"[BIND: callback]\" binding must have one of the following "
        "return types (if non-void): 1) primitive type value; "
        "2) interface class returned by value; 3) pointer to a known "
        "non-interface class; 4) known class of type [BIND: copy], returned "
        "by value; 5) opaque pointer of primitive type; or 6) unknown "
        "object pointer.";
    }
}

/*****************************************************************************/
/* STATIC              implement_callback_result_declare                     */
/*****************************************************************************/

static void
  implement_callback_result_declare(kdd_func *func, ostream &out,
                                    bool old_syntax)
{
  kdd_type &type = func->return_type;
  if (type.is_void())
    return;
  if (type.known_class != NULL)
    {
      if (old_syntax)
        out << "      " << type.known_class->mni_name << " *_Result;\n";
      else
        out << "      " << type.known_class->mni_name << " ^_Result;\n";
    }
  else if ((type.known_primitive != NULL) && !type.bind_as_opaque_pointer)
    out << "      " << type.known_primitive->mni_name << " _Result;\n";
  else if ((type.known_primitive != NULL) && type.bind_as_opaque_pointer)
    out << "      IntPtr _Result;\n";
  else if (type.unknown_name != NULL)
    out << "      IntPtr _Result;\n";
}

/*****************************************************************************/
/* STATIC                 implement_callback_binding                         */
/*****************************************************************************/

static void
  implement_callback_binding(kdd_func *func, ostream &out, bool old_syntax)
{
  kdd_variable *arg;
  out << "  ";
  func->return_type.print(out,NULL);
  out << MNI_DELEGATOR_PREFIX << func->owner->name
      << "::" << func->name << "(";
  int nargs = func->num_mni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    {
      arg->type.print(out,arg->name);
      if (nargs > 1)
        out << ", ";
    }
  out << ")\n";
  out << "  {\n";
  out << "    try {\n";
  implement_callback_args_in(func,out,old_syntax);
  implement_callback_result_declare(func,out,old_syntax);
  out << "      ";
  if (!func->return_type.is_void())
    out << "_Result = ";
  out << "_owner_->" << func->mni_name << "(";
  nargs = func->num_mni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    {
      if ((arg->type.known_class == NULL) && arg->type.by_ref)
        out << "&";
      out << "_" << arg->name;
      if (nargs > 1)
        out << ",";
    }
  out << ");\n";
  implement_callback_args_out(func,out);
  implement_callback_return_result(func,out,old_syntax);
  out << "    }\n";
  out << "    catch (std::bad_alloc &exc) { throw exc; }\n";
  out << "    catch (kdu_exception exc) { kdu_rethrow(exc); }\n";
  out << "    catch (...) { }\n";
  out << "    throw KDU_CONVERTED_EXCEPTION;\n";
  implement_native_return_default(func->return_type,out);
  out << "  }\n";
}

/*****************************************************************************/
/* STATIC                 generate_derived_delegator                         */
/*****************************************************************************/

static void
  generate_derived_delegator(kdd_class *cls, ostream &out, bool old_syntax)
{
  out << "  class " MNI_DELEGATOR_PREFIX << cls->name
      << " : public " AUX_DELEGATOR_PREFIX << cls->name << " {\n";
  out << "  private:\n";
  if (old_syntax)
    out << "    gcroot<" << cls->mni_name << " *> _owner_;\n";
  else
    out << "    gcroot<" << cls->mni_name << " ^> _owner_;\n";
  out << "  public:\n";
  if (old_syntax)
    out << "    " MNI_DELEGATOR_PREFIX << cls->name << "("
        << cls->mni_name << " * _owner_)\n";
  else
    out << "    " MNI_DELEGATOR_PREFIX << cls->name << "("
        << cls->mni_name << " ^ _owner_)\n";
  out << "      { this->_owner_ = _owner_; }\n";
  kdd_func *func;
  kdd_variable *arg;
  for (func=cls->functions; func != NULL; func=func->next)
    {
      if ((func->mni_name == NULL) || (func->binding != KDD_BIND_CALLBACK))
        continue;
      out << "    virtual ";
      func->return_type.print(out,NULL);
      out << func->name << "(";
      for (arg=func->args; arg != NULL; arg=arg->next)
        {
          arg->type.print(out,arg->name);
          if (arg->next != NULL)
            out << ", ";
        }
      out << ");\n";
    }
  // End class declaration
  out << "  };\n";
  for (func=cls->functions; func != NULL; func=func->next)
    {
      if ((func->mni_name == NULL) || (func->binding != KDD_BIND_CALLBACK))
        continue;
      out << "\n";
      implement_callback_binding(func,out,old_syntax);
    }
  out << "\n";
}

/*****************************************************************************/
/* STATIC                 generate_mni_implementation                        */
/*****************************************************************************/

static void
  generate_mni_implementation(kdd_file *header_files, kdd_class *classes,
                              const char *mni_dir, const char *aux_dir,
                              bool old_syntax)
{
  char *path = new char[strlen(mni_dir)+80]; // More than enough
  strcpy(path,mni_dir);
  strcat(path,"/kdu_mni.cpp");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the managed C++ extensions implementation file, "
        << "\"" << path << "\".";
    }

  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n";

  out << "#include \"";
  write_relative_path(aux_dir,mni_dir,"kdu_aux.h",out);
  out << "\"\n";
  out << "#include \"kdu_mni.h\"\n";
  out << "\n";
  if (old_syntax)
    out << "#using <mscorlib.dll>\n"; // Implicit in Managed Extensions V2
  out << "#include <vcclr.h>\n";
  out << "\n";

  out << "namespace kdu_mni {\n";

  // Start by declaring native class extensions for callbacks
  kdd_class *cls;
  out << "  /* ===================================== */\n";
  out << "  // Derived versions of unmanaged classes\n";
  out << "  /* ===================================== */\n\n";
  for (cls=classes; cls != NULL; cls=cls->next)
    if ((cls->mni_name != NULL) && cls->has_callbacks)
      {
        out << "  //---------------------------------------------------\n";
        out << "  // Delegator for class: " << cls->name << "\n";
        generate_derived_delegator(cls,out,old_syntax);
      }

  // Now walk through the classes, generating their implementations
  out << "  /* ===================================== */\n";
  out << "  // Managed Class Implementation\n";
  out << "  /* ===================================== */\n\n";
  for (cls=classes; cls != NULL; cls=cls->next)
    {
      if (cls->mni_name == NULL)
        continue;
      
      bool has_managed_base =
        (cls->base.known_class != NULL) &&
        (cls->base.known_class->mni_name != NULL);

      bool need_do_dispose = // If a `Dispose(bool)' function is required
        ((cls->binding == KDD_BIND_COPY) || (cls->binding == KDD_BIND_REF)) &&
        ((!has_managed_base) || !cls->destroy_virtual) &&
        (!cls->destroy_private);
      if (cls->has_callbacks ||
          (has_managed_base && cls->base.known_class->has_callbacks))
        need_do_dispose = true;

      out << "  //---------------------------------------------------\n";
      out << "  // Managed Class: " << cls->mni_name << "\n";
      bool have_constructor = false;
      for (kdd_func *func=cls->functions; func != NULL; func=func->next)
        {
          kdd_variable *arg;
          if ((func->mni_name == NULL) || func->is_destructor)
            continue;  // Implement destructor separately below

          int min_args, max_args, num_args, n;
          for (min_args=0, arg=func->args;
              (arg != NULL) && (arg->initializer==NULL);
              arg=arg->next, min_args++);
          max_args = func->num_mni_args;
          for (num_args=max_args; num_args >= min_args; num_args--)
            {
              out << "  ";
              func->return_type.write_mni_declaration(out,NULL,old_syntax);
              out << cls->mni_name << "::";
              if ((func->property != NULL) && (!old_syntax))
                out << "__property_"; // True name is reserved in new syntax
              out << func->mni_name << "(";
              for (arg=func->args, n=num_args; n > 0; n--, arg=arg->next)
                {
                  arg->type.write_mni_declaration(out,arg->name,old_syntax,
                                                  "_");
                  if (n > 1) out << ", ";
                }
              out << ")\n";
              if (has_managed_base && func->is_constructor)
                out << "      : " << cls->base.known_class->mni_name
                    << "((" << cls->base.known_class->name << " *) NULL)\n";

              if (func->is_virtual && (num_args < max_args))
                {
                  out << "  {\n";
                  implement_call_mni_virtual_with_defaults(func,num_args,out,
                                                           old_syntax);
                  out << "  }\n";
                  continue;
                }

              if (func->binding == KDD_BIND_CALLBACK)
                { // Function should not do anything; expecting override
                  out << "  {\n";
                  implement_managed_return_default(func->return_type,out,
                                                   old_syntax);
                  out << "  }\n";
                  continue;
                }

              // If we get here, we should implement the function directly
              out << "  {\n";
              bool need_cleanup =
                implement_args_in(func->args,num_args,out,old_syntax);
              implement_result_declare(func->return_type,out);
              if (need_cleanup)
                out << "    try { // So we can clean up temporary arrays\n";
              if (func->is_constructor)
                {
                  have_constructor = true;
                  if ((cls->binding == KDD_BIND_REF) ||
                      (cls->binding == KDD_BIND_COPY))
                    {
                      out << "    _natively_owned = false;\n";
                      if (cls->has_callbacks)
                        {
                          if (func->args != NULL)
                            { // Instantiate default arguments
                              kdd_variable *arg=func->args;
                              for (int n=num_args; n > 0; n--, arg=arg->next);
                              for (; (arg != NULL) && arg->mni_bindable;
                                   arg=arg->next)
                                { // Instantiate default argument
                                  out << "    ";
                                  arg->type.print(out,arg->name);
                                  out << " = " << arg->initializer << ";\n";
                                }
                            }
                          out << "    " AUX_EXTENSION_PREFIX << cls->name
                              << " *_true_kref = new "
                              AUX_EXTENSION_PREFIX << cls->name << "(";
                          implement_args_pass(func->args,max_args,out);
                          out << ");\n";
                          out << "    _kref = _true_kref;\n";
                          out << "    _true_kref->_delegator = new "
                              MNI_DELEGATOR_PREFIX << cls->name << "(this);\n";
                        }
                      else
                        {
                          out << "    _kref = new " << cls->name << "(";
                          implement_args_pass(func->args,num_args,out);
                          out << ");\n";
                        }
                    }
                  else if (cls->binding == KDD_BIND_IFC)
                    {
                      out << "    " << cls->name << "_kval_tmp(";
                      implement_args_pass(func->args,num_args,out);
                      out << ");\n";
                      implement_native_ifc_to_mni("_kval_tmp","this",out);
                    }
                  else
                    assert(0);
                }
              else if (cls->name == NULL)
                {
                  implement_result_assignment(func->return_type,out);
                  out << "::" << func->name << "(";
                  implement_args_pass(func->args,num_args,out);
                  out << ");\n";
                }
              else if (func->is_static)
                {
                  implement_result_assignment(func->return_type,out);
                  out << cls->name << "::" << func->name << "(";
                  implement_args_pass(func->args,num_args,out);
                  out << ");\n";
                }
              else if (cls->binding == KDD_BIND_IFC)
                {
                  out << "    " << cls->name << " _kval_cast;\n";
                  implement_mni_to_native_ifc(cls,"this","_kval_cast",
                                              out,old_syntax);
                  implement_result_assignment(func->return_type,out);
                  out << "_kval_cast." << func->name << "(";
                  implement_args_pass(func->args,num_args,out);
                  out << ");\n";
                  implement_native_ifc_to_mni("_kval_cast","this",out);
                }
              else
                {
                  implement_result_assignment(func->return_type,out);
                  out << "_get_kref()->" << func->name << "(";
                  implement_args_pass(func->args,num_args,out);
                  out << ");\n";
                  if (func->binding == KDD_BIND_DONATE)
                    out << "    _natively_owned = true;\n";
                }
              if (need_cleanup)
                {
                  out << "    } catch (...) {\n";
                  implement_args_cleanup(func->args,num_args,out);
                  out << "    throw;\n";
                  out << "    }\n";
                }

              implement_args_out(func->args,num_args,out,old_syntax);
              implement_args_cleanup(func->args,num_args,out);
              implement_return_result(func,out,old_syntax);
              out << "  }\n";
            }
        }

      // Finally, implement destructors and `Dispose(bool)',
      // as required
      if (need_do_dispose)
        {
          if (old_syntax)
            out << "  void " << cls->mni_name << "::Dispose(bool in_dispose)\n";
          else
            out << "  void " << cls->mni_name << "::Do_dispose(bool in_dispose)\n";
          out << "  {\n";
          if (cls->has_callbacks)
            out << "    " AUX_EXTENSION_PREFIX << cls->name
                << " *_kref_tmp = (" << AUX_EXTENSION_PREFIX << cls->name
                << " *) _kref;\n";
          else
            out << "    " << cls->name << " *_kref_tmp = _get_kref();\n";
          out << "    _kref = NULL;\n";
          out << "    if ((!_natively_owned) && (_kref_tmp != NULL))\n";
          out << "      {\n";
          if (cls->has_callbacks)
            {
              out << "        " MNI_DELEGATOR_PREFIX << cls->name
                  << " *_delegator = (" MNI_DELEGATOR_PREFIX << cls->name
                  << " *)(_kref_tmp->_delegator);\n";
              out << "        delete _kref_tmp;\n";
              out << "        if (_delegator != NULL)\n"
                  << "          delete _delegator;\n";
            }
          else
            out << "        delete _kref_tmp;\n";
          if (old_syntax)
            out << "        if (in_dispose)\n"
                   "          GC::SuppressFinalize(this);\n";
          out << "      }\n";
          out << "  }\n";
        }
    }

  out << "} // End of namespace \"kdu_mni\"\n";
  out.close();
  delete[] path;
}


/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/*                            make_managed_bindings                          */
/*****************************************************************************/

void make_managed_bindings(kdd_file *header_files, kdd_class *classes,
                           const char *mni_dir, const char *aux_dir,
                           bool old_syntax)
{
  kdd_class *cls;
  for (cls=classes; cls != NULL; cls=cls->next)
    {
      if (cls->binding == KDD_BIND_NONE)
        continue;
      const char *name = cls->name;
      if (name == NULL)
        name = "kdu_global_funcs";
      else
        cls->file->has_bindings = true;
      cls->mni_name = new char[strlen(name)+2];
      cls->mni_name[0] = 'C';
      strcpy(cls->mni_name+1,name);
      find_bindable_functions(cls);
    }

  for (cls=classes; cls != NULL; cls=cls->next)
    if (cls->name == NULL)
      {
        generate_csharp_constants(cls,mni_dir);
        generate_vbasic_constants(cls,mni_dir);
        break;
      }

  generate_mni_header(classes,mni_dir,old_syntax);
  generate_mni_implementation(header_files,classes,mni_dir,aux_dir,
                              old_syntax);
}
