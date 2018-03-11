/*****************************************************************************/
// File: jni_builder.cpp [scope = APPS/HYPERDOC]
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
   Implements automatic JNI interface construction features offered
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
/* STATIC                        print_banner                                */
/*****************************************************************************/

static void
  print_banner(const char *java_name, ostream &out)
{
  int i;

  out << "\n/*";
  for (i=0; i < 75; i++)
    out.put('*');
  out << "*/\n";

  int spaces = 75-(int) strlen(java_name);
  if (spaces < 2)
    spaces = 2;

  out << "/*";
  for (i=0; i < (spaces>>1); i++)
    out.put(' ');
  out << java_name;
  for (i=0; i < ((spaces+1)>>1); i++)
    out.put(' ');
  out << "*/\n";

  out << "/*";
  for (i=0; i < 75; i++)
    out.put('*');
  out << "*/\n";
}

/*****************************************************************************/
/* STATIC                    find_bindable_functions                         */
/*****************************************************************************/

static void
  find_bindable_functions(kdd_class *cls)
  /* This function determines whether or not each function in the supplied
     class can be bound to Java, based on the binding information currently
     available.  Basically, a function is bindable if its class is bindable
     and its return value and all arguments are bindable, unless the
     function has the KDD_BIND_NONE binding. */
{
  if (cls->binding == KDD_BIND_NONE)
    return;

  kdd_func *func;
  for (func=cls->functions; func != NULL; func=func->next)
    {
      if (func->binding == KDD_BIND_NONE)
        continue;
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
        continue; // Java does not support operator overloading
      if (func->is_constructor && cls->is_abstract)
        continue; // Cannot instantiate abstract class
      if (func->is_constructor && (func->args != NULL) &&
          ((func->args->next == NULL) ||
           (func->args->next->initializer != NULL)) &&
          ((func->args->type.unknown_name != NULL) ||
           ((func->args->type.known_primitive != NULL) &&
            (strcmp(func->args->type.known_primitive->java_name,"long")==0))))
        continue; // Cannot build constructor with only one argument, which
                  // is of type long or a reference to an unknown type, since
                  // unknown type pointers are passed as Java longs; this
                  // could get confused with the protected constructor we
                  // create to allow the Java class to be instantiated
                  // from within JNI code.
      kdd_variable *var;
      func->num_jni_args = 0;
      for (var=func->args; var != NULL; var=var->next, func->num_jni_args++)
        if (!var->type.is_bindable(false))
          break;
        else
          var->jni_bindable = true;
      if (func->binding != KDD_BIND_CALLBACK)
        for (; var != NULL; var=var->next)
          {
            var->jni_bindable = false;
            if (var->initializer == NULL)
              break;
          }
      if (var == NULL)
        {
          const char *jname = func->name;
          if (func->is_destructor)
            jname = "Native_destroy";
          func->java_name = new char[strlen(jname)+1];
          strcpy(func->java_name,jname);
          func->java_name[0] = (char) toupper(jname[0]);
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
        }
    }
}

/*****************************************************************************/
/* STATIC                  generate_jni_func_name                            */
/*****************************************************************************/

static void
  generate_jni_func_name(const char *return_type, const char *class_name,
                         const char *func_name, kdd_variable *arguments,
                         bool is_static, bool is_overloaded, ostream &out)
{
  kdd_variable *arg;

  out << "JNIEXPORT " << return_type << " JNICALL Java_kdu_1jni_";
  for (; *class_name != '\0'; class_name++)
    if (*class_name == '_')
      out << "_1";
    else
      out.put(*class_name);
  out << "_";
  for (; *func_name != '\0'; func_name++)
    if (*func_name == '_')
      out << "_1";
    else
      out.put(*func_name);
  if (is_overloaded)
    { // Need to include mangled argument signature in function name
      out << "__";
      for (arg=arguments; (arg != NULL) && arg->jni_bindable; arg=arg->next)
        {
          const char *ch;

          if (arg->type.known_primitive != NULL)
            {
              if (arg->type.bind_as_opaque_pointer)
                {
                  out.put('J'); // Java signature for type "long"
                }
              else
                {
                  if ((arg->type.derefs + arg->type.as_array +
                       ((arg->type.by_ref)?1:0)) == 1)
                    out << "_3"; // Mangling of "[" in arg signature
                  ch = arg->type.known_primitive->java_signature;
                  for (; *ch != '\0'; ch++)
                    if (*ch == '_')
                      out << "_1";
                    else if (*ch == '/')
                      out.put('_');
                    else if (*ch == ';')
                      out << "_2";
                    else
                      out.put(*ch);
                }
            }
          else if (arg->type.unknown_name != NULL)
            {
              assert((arg->type.derefs == 1) &&
                     !(arg->type.as_array || arg->type.by_ref));
              out.put('J');
            }
          else
            {
              assert(arg->type.known_class != NULL);
              out << "Lkdu_1jni_";
              ch = arg->type.known_class->java_name;
              for (; *ch != '\0'; ch++)
                if (*ch == '_')
                  out << "_1";
                else
                  out.put(*ch);
                out << "_2"; // Mangling of ";" in arg signature
            }
        }
    }
  if (is_static)
    out << "(JNIEnv *__env, jclass this_class";
  else
    out << "(JNIEnv *__env, jobject _self";
  for (arg=arguments; (arg != NULL) && arg->jni_bindable; arg=arg->next)
    {
      out << ", " << arg->type.get_jni_name() << " _";
      out << arg->name;
    }
  out << ")";
}

/*****************************************************************************/
/* STATIC                  implement_class_loading                           */
/*****************************************************************************/
            
static void
  implement_class_loading(kdd_class *self_class, bool is_static,
                          kdd_variable *args, kdd_class *return_class,
                          ostream &out)
  /* Inserts the code to load Java classes which are not already loaded.
     There is potentially one new class load for each argument, one for
     the function return value and one for the class to which the object
     belongs.  Note, however, that any or all of these may be NULL. */
{
  if (is_static)
    self_class = NULL;
  if ((self_class != NULL) && (self_class->name != NULL))
    out << "    if (" << self_class->java_name << "_CLS==NULL)\n      "
        << self_class->java_name << "_LOADER(__env);\n";
  if ((return_class != NULL) && (return_class != self_class))
    out << "    if (" << return_class->java_name << "_CLS==NULL)\n      "
        << return_class->java_name << "_LOADER(__env);\n";
  kdd_variable *scan, *prev;
  for (scan=args; (scan != NULL) && scan->jni_bindable; scan=scan->next)
    {
      kdd_class *cls = scan->type.known_class;
      if ((cls == NULL) || (cls == self_class) || (cls == return_class))
        continue;
      for (prev=args; prev != scan; prev=prev->next)
        if (prev->type.known_class == cls)
          break;
      if (prev == scan)
        out << "    if (" << cls->java_name << "_CLS==NULL)\n      "
            << cls->java_name << "_LOADER(__env);\n";
    }
}

/*****************************************************************************/
/* STATIC                   implement_get_native                             */
/*****************************************************************************/

static void
  implement_get_native(kdd_class *cls, const char *jni_obj,
                       const char *native_obj, ostream &out,
                       bool check_for_null=false, const char *jni_prefix="",
                       int indent=4)
  /* This function generates a fragment of JNI implementation code which
     accesses the native object pointer in the object whose name is obtained
     by concatenating `jni_prefix' and `jni_obj', assigning it to a variable
     named `native_obj'.  Both the variable and the field identifier
     associated with the native pointer field are assigned to local
     variables. */
{
  int i;
  char *prefix = new char[indent+1];
  for (i=0; i < indent; i++)
    prefix[i] = ' ';
  prefix[i] = '\0';
  if ((cls->binding == KDD_BIND_REF) || (cls->binding == KDD_BIND_COPY))
    {
      out << prefix << cls->name << " *" << native_obj
          << " = (" << cls->name << " *)\n" << prefix << "  _kdu_long_to_addr";
      if (check_for_null)
        out << "((" << jni_prefix << jni_obj << "==NULL)?((jlong) 0):";
      out << "(__env->GetLongField(" << jni_prefix << jni_obj
          << "," << cls->java_name << "_PTR) & ~((jlong) 1))";
      if (check_for_null)
        out << ")";
      out << ";\n";
    }
  else if (cls->binding == KDD_BIND_IFC)
    {
      out << prefix << "void *" << native_obj << "_S = _kdu_long_to_addr";
      if (check_for_null)
        out << "((" << jni_prefix << jni_obj << "==NULL)?((jlong) 0):";
      out << "(__env->GetLongField(" << jni_prefix << jni_obj
          << "," << cls->java_name << "_PTR))";
      if (check_for_null)
        out << ")";
      out << ";\n";
      out << prefix << cls->name << " " << native_obj
          << " = *((" << cls->name  << " *)(&" << native_obj << "_S));\n";
    }
  else
    assert(0);
  delete[] prefix;
}

/*****************************************************************************/
/* STATIC                   implement_set_native                             */
/*****************************************************************************/

static void
  implement_set_native(kdd_class *cls, const char *native_obj,
                       const char *jni_obj, ostream &out,
                       bool matched_by_get=true,
                       const char *jni_prefix="", int indent=4)
  /* This function does essentially the opposite of `implement_get_native'.
     Set `matched_by_get' to false only if there is no corresponding
     `implement_get_native' call -- this is relevant for interface objects,
     for which an intermediate variable with the "_S" suffix is created. */
{
  int i;
  char *prefix = new char[indent+1];
  for (i=0; i < indent; i++)
    prefix[i] = ' ';
  prefix[i] = '\0';
  if ((cls->binding == KDD_BIND_REF) || (cls->binding == KDD_BIND_COPY))
    {
      out << prefix << "__env->SetLongField(" << jni_prefix << jni_obj << ","
          << cls->java_name << "_PTR,(jlong) _addr_to_kdu_long("
          << native_obj << "));\n";
    }
  else if (cls->binding == KDD_BIND_IFC)
    {
      out << prefix;
      if (!matched_by_get)
        out << "void *";
      out << native_obj << "_S = *((void **)(&("
        << native_obj << ")));\n";
      out << prefix << "  __env->SetLongField(" << jni_prefix << jni_obj
          << "," << cls->java_name
          << "_PTR,(jlong) _addr_to_kdu_long(" << native_obj << "_S));\n";
    }
  delete[] prefix;
}

/*****************************************************************************/
/* STATIC                     implement_args_in                              */
/*****************************************************************************/

static bool
  implement_args_in(kdd_variable *args, ostream &out)
  /* The function returns true if any temporary arrays were created.  In
     this case, the native function call needs to be enclosed in a try/catch
     statement so that the created resources can be cleaned up. */
{
  bool need_cleanup=false;
  for (; (args != NULL) && args->jni_bindable; args=args->next)
    {
      if (args->type.known_class != NULL)
        {
          bool check_for_null =
            ((args->type.known_class->binding == KDD_BIND_IFC) &&
             ((args->type.derefs + args->type.as_array) == 0) &&
             !args->type.by_ref) ||
            ((args->type.known_class->binding != KDD_BIND_IFC) &&
             (args->type.derefs == 1));
          implement_get_native(args->type.known_class,args->name,args->name,
                               out,check_for_null,"_");
        }
      else if (args->type.known_primitive != NULL)
        {
          if (args->type.bind_as_opaque_pointer)
            {
              out << "    " << args->type.known_primitive->name << " *"
                  << args->name << " = (" << args->type.known_primitive->name
                  << " *) _kdu_long_to_addr(_" << args->name << ");\n";
            }
          else if ((args->type.derefs + args->type.as_array) != 0)
            { // Passing array argument type
              assert(((args->type.derefs + args->type.as_array) == 1) &&
                     !args->type.by_ref);
              need_cleanup = true;
              out << "    " << args->type.known_primitive->name << " *"
                  << args->name << " = NULL;\n";
              out << "    " << args->type.known_primitive->jni_name << " *"
                  << args->name << "_ELTS = NULL;\n";
              if (!args->type.known_primitive->java_size_compatible)
                out << "    jsize " << args->name << "_L = 0;\n";
              out << "    if (_" << args->name << " != NULL)\n";
              out << "      {\n";
              out << "        " << args->name << "_ELTS = __env->Get";
              out.put((char)toupper(args->type.known_primitive->java_name[0]));
              out << args->type.known_primitive->java_name+1;
              out << "ArrayElements(_" << args->name << ",NULL);\n";
              if (!args->type.known_primitive->java_size_compatible)
                { // C++ and JNI types may differ in their sizes
                  out << "        " << args->name << "_L = "
                      << "__env->GetArrayLength(_" << args->name << ");\n";
                  out << "        " << args->name << " = new "
                      << args->type.known_primitive->name << "["
                      << args->name << "_L];\n";
                  out << "        { for (int i=0; i<"<<args->name<< "_L; i++) "
                      << args->name << "[i] = (";
                  if (strcmp(args->type.known_primitive->name,"bool") == 0)
                    out << args->name << "_ELTS[i])?true:false;";
                  else
                    out << args->type.known_primitive->name
                        << ")(" << args->name << "_ELTS[i]);";
                  out << " }\n";
                }
              else
                {
                  out << "        " << args->name << " = ("
                      << args->type.known_primitive->name << " *) "
                      << args->name << "_ELTS;\n";
                }
              out << "      }\n";
            }
          else if (args->type.known_primitive->string_type)
            { // Passing a string argument type
              need_cleanup = true;
              out << "    const char *" << args->name << " = NULL;\n";
              out << "    if (_" << args->name << " != NULL)\n"
                  << "      {\n";
              out << "        " << args->name << " = "
                     "__env->GetStringUTFChars(_" << args->name << ",NULL);\n";
              out << "      }\n";
            }
          else if (args->type.by_ref)
            { // Passing a scalar by reference; Java equivalent is an array.
              out << "    " << args->type.known_primitive->jni_name << " "
                  << args->name << "_ELT; __env->Get";
              out.put((char)toupper(args->type.known_primitive->java_name[0]));
              out << args->type.known_primitive->java_name+1;
              out << "ArrayRegion(_" << args->name << ",0,1,&"
                  << args->name << "_ELT);\n";
              out << "    " << args->type.known_primitive->name << " "
                  << args->name << " = (";
              if (strcmp(args->type.known_primitive->name,"bool") == 0)
                out << args->name << "_ELT)?true:false;\n";
              else
                out << args->type.known_primitive->name
                    << ") " << args->name << "_ELT;\n";
            }
          else
            { // Passing a simple scalar argument by value.
              out << "    " << args->type.known_primitive->name << " "
                  << args->name << " = (";
              if (strcmp(args->type.known_primitive->name,"bool") == 0)
                out << "_" << args->name << ")?true:false;\n";
              else
                out << args->type.known_primitive->name
                    << ") _" << args->name << ";\n";
            }
        }
      else if (args->type.unknown_name != NULL)
        { // Passing pointer to unknown class as "jlong"
          assert((args->type.derefs == 1) &&
                 !(args->type.as_array || args->type.by_ref));
          out << "    " << args->type.unknown_name << " *"
              << args->name << " = (";
          out << args->type.unknown_name << " *) "
              << "_kdu_long_to_addr(_" << args->name << ");\n";
        }
      else
        assert(0);
    }
  return need_cleanup;
}

/*****************************************************************************/
/* STATIC                     implement_args_pass                            */
/*****************************************************************************/

static void
  implement_args_pass(kdd_variable *args, ostream &out)
{
  for (; (args != NULL) && args->jni_bindable; args=args->next)
    {
      if ((args->type.known_class != NULL) &&
          ((args->type.known_class->binding == KDD_BIND_COPY) ||
           (args->type.known_class->binding == KDD_BIND_REF)) &&
          !(args->type.derefs + args->type.as_array))
        out.put('*');
      out << args->name;
      if ((args->next != NULL) && args->next->jni_bindable)
        out.put(',');
    }
}

/*****************************************************************************/
/* STATIC                     implement_args_out                             */
/*****************************************************************************/

static void
  implement_args_out(kdd_variable *args, ostream &out)
{
  for (; (args != NULL) && args->jni_bindable; args=args->next)
    {
      if (args->type.known_class != NULL)
        {
          if (args->type.known_class->binding == KDD_BIND_IFC)
            { // Interfaces need to be updated if passed by reference
              if (((args->type.derefs + args->type.as_array) != 0) ||
                  args->type.by_ref)
                implement_set_native(args->type.known_class,args->name,
                                     args->name,out,true,"_");
            }
          else
            { // Referenced objects need auto-destruct disabled if donated
              if (args->type.binding == KDD_BIND_DONATE)
                implement_set_native(args->type.known_class,args->name,
                                     args->name,out,true,"_");
            }
        }
      else if (args->type.known_primitive != NULL)
        {
          if (args->type.bind_as_opaque_pointer)
            {
              continue; // Nothing to do
            }
          else if ((args->type.derefs + args->type.as_array) != 0)
            { // Passed an array argument type; copy entries back to Java array
              assert(((args->type.derefs + args->type.as_array) == 1) &&
                     !args->type.by_ref);
              if (!args->type.known_primitive->java_size_compatible)
                { // Only case where C++ and JNI types may differ in lengths
                  out << "    if (_" << args->name << " != NULL)\n"
                      << "      {\n";
                  out << "        for (int i=0; i<"<<args->name<< "_L; i++) "
                      << args->name << "_ELTS[i] = ("
                      << args->type.known_primitive->jni_name
                      << ")(" << args->name << "[i]);\n";
                  // out << "        delete[] " << args->name << ";\n";
                  out << "      }\n";
                }
            }
          else if (args->type.by_ref)
            { // Passed a scalar argument by reference
              out << "    " << args->name << "_ELT = ("
                  << args->type.known_primitive->jni_name << ") "
                  << args->name << ";\n";
              out << "  __env->Set";
              out.put((char)toupper(args->type.known_primitive->java_name[0]));
              out << args->type.known_primitive->java_name+1;
              out << "ArrayRegion(_" << args->name << ",0,1,&"
                  << args->name << "_ELT);\n";
            }
        }
    }
}

/*****************************************************************************/
/* STATIC                  implement_args_cleanup                            */
/*****************************************************************************/

static void
  implement_args_cleanup(kdd_variable *args, ostream &out)
  /* This function implements code to delete any temporary arrays which
     were created to pass arguments into the embedded native function. */
{
  for (; (args != NULL) && args->jni_bindable; args=args->next)
    {
      if (args->type.known_primitive == NULL)
        continue;
      if (args->type.bind_as_opaque_pointer)
        {
          continue; // Nothing to do
        }
      else if ((args->type.derefs + args->type.as_array) != 0)
        { // Argument was passed via a temporary array
          assert(((args->type.derefs + args->type.as_array) == 1) &&
                 !args->type.by_ref);
          out << "    if (_" << args->name << " != NULL)\n"
              << "      {\n";
          if (!args->type.known_primitive->java_size_compatible)
            out << "        delete[] " << args->name << ";\n";
          out << "        __env->Release";
          out.put((char)toupper(args->type.known_primitive->java_name[0]));
          out << args->type.known_primitive->java_name+1;
          out << "ArrayElements(_" << args->name << ","
            << args->name << "_ELTS,0);\n";
          out << "      }\n";
        }
      else if (args->type.known_primitive->string_type)
        { // Passed a string argument; release the UTF character array
          out << "    if (_" << args->name << " != NULL)\n"
              << "      {\n";
          out << "        __env->ReleaseStringUTFChars(_" << args->name
              << "," << args->name << ");\n";
          out << "      }\n";
        }
    }
}

/*****************************************************************************/
/* STATIC                  implement_result_declare                          */
/*****************************************************************************/

static void
  implement_result_declare(kdd_type &return_type, ostream &out)
{
  if (!return_type.is_void())
    {
      out << "    ";
      if ((return_type.known_class != NULL) &&
          (return_type.known_class->binding == KDD_BIND_COPY) &&
          (return_type.derefs == 0) && (return_type.as_array == 0))
        return_type.print(out,"Result_copy");
      else
        return_type.print(out,"Result");
      out << ";\n";
    }
}

/*****************************************************************************/
/* STATIC                 implement_result_assignment                        */
/*****************************************************************************/

static void
  implement_result_assignment(kdd_type &return_type, ostream &out)
{
  out << "    ";
  if (return_type.is_void())
    return;
  if ((return_type.derefs+return_type.as_array) != 0)
    out << "Result = ";
  else if ((return_type.known_class != NULL) &&
           (return_type.known_class->binding == KDD_BIND_COPY))
    out << "Result_copy = ";
  else
    out << "Result = ";
}

/*****************************************************************************/
/* STATIC                   implement_return_result                          */
/*****************************************************************************/

static void
  implement_return_result(kdd_func *func, ostream &out)
{
  kdd_type &return_type = func->return_type;
  if (return_type.is_void())
    return;
  if (return_type.known_class != NULL)
    { // Create and return a Java object of the relevant class
      if (return_type.known_class->has_callbacks)
        { kdu_error e; e << "Error at line "
          << func->line_number << " in file, \""
          << func->file->name << "\"; functions may not "
          "return objects which contain functions having the "
          "\"[BIND: callback]\" binding.";
        }

      out << "    jobject _Result = NULL;\n";
      bool return_copy =
        (return_type.known_class->binding == KDD_BIND_COPY) &&
        ((return_type.derefs+return_type.as_array) == 0);
      bool return_reference =
        ((return_type.known_class->binding == KDD_BIND_COPY) ||
         (return_type.known_class->binding == KDD_BIND_REF)) && !return_copy;
      if (return_reference)
        out << "    if (Result != NULL)\n";

      out << "      {\n";
      out << "        _Result = __env->AllocObject("
          << return_type.known_class->java_name << "_CLS);\n";
      if (return_copy)
        {
          out << "        " << return_type.known_class->name
              << " *Result = new " << return_type.known_class->name
              << "; *Result = Result_copy;\n";
          out << "        Result = (" << return_type.known_class->name << " *)\n"
              << "          _kdu_long_to_addr(((jlong) _addr_to_kdu_long(Result))+1); "
                 "// Enable auto-destroy.\n";
        }
      implement_set_native(return_type.known_class,"Result","_Result",out,
                           false,"",8);
      out << "      }\n";
      out << "    return _Result;\n";
    }
  else if (return_type.known_primitive != NULL)
    {
      if (return_type.bind_as_opaque_pointer)
        {
          out << "    return (jlong) _addr_to_kdu_long(Result);\n";
        }
      else if (return_type.known_primitive->string_type)
        { // Create and return a Java string object
          out << "    return __env->NewStringUTF(Result);\n";
        }
      else
        { // Return ordinary scalar
          out << "    return (" << return_type.known_primitive->jni_name
              << ") Result;\n";
        }
    }
  else if (return_type.unknown_name != NULL)
    {
      assert((return_type.derefs == 1) &&
             !(return_type.as_array || return_type.by_ref));
      out << "    return (jlong) _addr_to_kdu_long(Result);\n";
    }
}

/*****************************************************************************/
/* STATIC                  implement_return_default                          */
/*****************************************************************************/

static void
  implement_return_default(kdd_type &return_type, ostream &out,
                           bool in_native_function=false)
{
  if (return_type.is_void())
    return;
  if (in_native_function)
    {
      if ((return_type.as_array > 0) || (return_type.derefs > 0))
        out << "  return NULL;\n";
      else if (return_type.known_class != NULL)
        out << "  return " << return_type.known_class->name << "();\n";
      else if (return_type.known_primitive != NULL)
        {
          if (return_type.known_primitive->string_type)
            out << "  return NULL;\n";
          else
            out << "  return ("<<return_type.known_primitive->name<<") 0;\n";
        }
      else if (return_type.unknown_name != NULL)
        out << "  return NULL;\n";
    }
  else
    {
      if (return_type.known_class != NULL)
        out << "  return NULL;\n";
      else if (return_type.known_primitive != NULL)
        {
          if (return_type.bind_as_opaque_pointer)
            out << "  return (jlong) 0;\n";
          else if (return_type.known_primitive->string_type)
            out << "  return NULL;\n";
          else
            out << "  return (" << return_type.known_primitive->jni_name
                << ") 0;\n";
        }
      else if (return_type.unknown_name != NULL)
        out << "  return (jlong) 0;\n";
    }
}

/*****************************************************************************/
/* STATIC                  generate_derived_delegator                        */
/*****************************************************************************/
          
static void
  generate_derived_delegator(kdd_class *cls, ostream &out)
  /* Classes containing functions with the BIND_CALLBACK binding must
     have their C++ delegator objects, derived from the corresponding
     object defined in "kdu_aux.h".  The delegator object defines only
     the callback functions. */
{
  kdd_func *func;
  kdd_variable *arg;

  out << "\nclass " JNI_DELEGATOR_PREFIX << cls->name
      << " : public " AUX_DELEGATOR_PREFIX << cls->name << " {\n";
  out << "  private: // Data\n"
         "    JavaVM *jvm;\n"
         "    jobject _self;\n";

  // Generate a private method to return the JNIEnv pointer for this thread.
  // We get this from the JavaVM pointer which is stored when the object
  // is created. The JavaVM pointer is constant across threads, but the
  // JNIEnv pointer is not, so we must get the JNIEnv pointer from the
  // JavaVM pointer each time that we need it if it is not available directly
  // from the JNI method invocation. This problem arises when doing java
  // callbacks from C++ code (java calls C++ calls other C++ that calls java).
  out << "    JNIEnv * _jniEnv( void )\n"
      << "      {\n"
      << "        JNIEnv *__env;\n"
      << "        if (jvm->AttachCurrentThread("
                  "reinterpret_cast<void **> (&__env),NULL) < 0)\n"
      << "          throw KDU_NULL_EXCEPTION;\n"
      << "        return __env;\n"
      << "      }\n";
  
  out << "  public: // Member functions\n";
  out << "    " JNI_DELEGATOR_PREFIX << cls->name << "(JNIEnv *__env)\n";
  out << "      {\n"
      << "        if (__env->GetJavaVM(&( this->jvm)) < 0)\n"
      << "          throw KDU_NULL_EXCEPTION;\n"
      << "        this->_self = NULL;\n"
      << "      }\n";
  out << "    ~" JNI_DELEGATOR_PREFIX << cls->name << "()\n"
      << "      {\n"
      << "        if (this->_self != NULL)\n"
      << "          _jniEnv()->DeleteGlobalRef(_self);\n"
      << "      }\n";
  out << "    void _init(JNIEnv *__env, jobject _self)\n"
      << "      {\n"
      << "        if (this->_self == NULL)\n"
      << "          this->_self = __env->NewGlobalRef(_self);\n"
      << "      }\n";

  for (func=cls->functions; func != NULL; func=func->next)
    if ((func->binding == KDD_BIND_CALLBACK) &&
        (func->java_name != NULL) && func->is_virtual)
      {
        out << "    ";
        if (func->return_type.exists())
          { func->return_type.print(out,NULL); out << " "; }
        out << func->name << "(";
        int nargs = func->num_jni_args;
        for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
          {
            arg->type.print(out,arg->name);
            if (nargs > 1)
              out << ", ";
          }
        out << ");\n";
      }

  out << "  };\n";
}

/*****************************************************************************/
/* STATIC                 implement_callback_args_in                         */
/*****************************************************************************/

static void
  implement_callback_args_in(kdd_func *func, ostream &out)
{
  kdd_variable *arg;
  for (arg=func->args; (arg != NULL) && arg->jni_bindable; arg=arg->next)
    {
      kdd_type &type = arg->type;
      if ((type.known_class != NULL) &&
          (type.known_class->binding == KDD_BIND_IFC) &&
          ((type.derefs+type.as_array) == 0) && !type.by_ref)
        { // Interface object passed by value
          out << "    jobject _" << arg->name << " = __env->AllocObject("
              << type.known_class->java_name << "_CLS);\n";
          implement_set_native(type.known_class,arg->name,arg->name,out,
                               false,"_");
        }
      else if ((type.known_class != NULL) &&
               (type.known_class->binding == KDD_BIND_COPY) &&
               (type.derefs == 0) && (type.as_array == 0) && !type.by_ref)
        { // Copy-able object passed by value; make a Java copy
          out << "    jobject _" << arg->name << " = __env->AllocObject("
              << type.known_class->java_name << "_CLS);\n";
          char buf_name[256]; // More than enough
          *buf_name='_'; strcpy(buf_name+1,arg->name); strcat(buf_name,"_BUF");
          out << "    " << type.known_class->name << " *" << buf_name
              << " = new " << type.known_class->name
              << "; *" << buf_name << " = " << arg->name << ";\n";
          out << "    " << buf_name << " = (" << type.known_class->name << " *)\n"
              << "      _kdu_long_to_addr(((jlong) _addr_to_kdu_long("
              << buf_name << ")) + 1); // Enable auto-destroy.\n";
          implement_set_native(type.known_class,buf_name,arg->name,
                               out,false,"_");
        }
      else if ((type.known_class != NULL) &&
               (type.known_class->binding != KDD_BIND_IFC) &&
               (type.as_array == 0) &&
               ((type.derefs + ((type.by_ref)?1:0)) == 1))
        { // Non-interface object passed by reference or pointer.
          out << "    jobject _" << arg->name << " = NULL;\n";
          if (type.derefs == 1)
            out << "    if (" << arg->name << " != NULL)\n";
          out << "      {\n";
          out << "        _" << arg->name << " = __env->AllocObject("
              << type.known_class->java_name << "_CLS);\n";
          if (type.derefs == 0)
            {
              char amp_name[256];
              amp_name[0] = '&';  strcpy(amp_name+1,arg->name);
              implement_set_native(type.known_class,amp_name,arg->name,out,
                                   false,"_",8);
            }
          else
            implement_set_native(type.known_class,arg->name,arg->name,out,
                                 false,"_",8);
          out << "      }\n";
        }
      else if ((type.known_primitive != NULL) &&
               type.known_primitive->string_type)
        {
          out << "    jstring _" << arg->name << " = "
                 "__env->NewStringUTF(" << arg->name << ");\n";
        }
      else if ((type.known_primitive != NULL) &&
               ((type.derefs+type.as_array) == 0))
        { // Primitive type passed by value or by reference
          if (type.by_ref)
            { // Communicate reference value via an intermediate array
              out << "    " << type.known_primitive->jni_name << "_"
                  << arg->name << "_VAL = (";
              if (strcmp(type.known_primitive->name,"bool") == 0)
                out << arg->name << ")?true:false;\n";
              else
                out << arg->type.known_primitive->jni_name
                    << ") " << arg->name << ";\n";
              out << "    " << type.known_primitive->java_name << "Array _"
                  << arg->name << " = __env->New"
                  << (char)(toupper(type.known_primitive->java_name[0]))
                  << type.known_primitive->java_name+1 << "Array(1);\n";
              out << "    __env->Set"
                  << (char)(toupper(type.known_primitive->java_name[0]))
                  << type.known_primitive->java_name+1 << "ArrayRegion(_"
                  << arg->name << ",0,1,&_" << arg->name << "_VAL);\n";
            }
          else
            { // Much simpler, pass by value semantics
              out << "    " << type.known_primitive->jni_name << " _"
                  << arg->name << " = (";
              if (strcmp(type.known_primitive->name,"bool") == 0)
                out << arg->name << ")?true:false;\n";
              else
                out << arg->type.known_primitive->jni_name
                    << ") " << arg->name << ";\n";
            }
        }
      else if ((type.known_primitive != NULL) && type.bind_as_opaque_pointer)
        { // Primitive array passed as opaque pointer
          out << "    jlong _" << arg->name << " = (jlong) _addr_to_kdu_long("
              << arg->name << ");\n";
        }
      else if ((type.unknown_name != NULL) && (type.as_array == 0) &&
               (type.derefs == 1))
        { // Unknown pointer passed directly or by reference
          if (type.by_ref)
            { // Communicate reference value via an intermediate array
              out << "    jlong _" << arg->name << "_VAL = (jlong) _addr_to_kdu_long("
                  << arg->name << ")\n";
              out << "    jlongArray _" << arg->name
                  << " = __env->NewLongArray(1);\n";
              out << "    __env->SetLongArrayRegion(_"
                  << arg->name << ",0,1,&_" << arg->name << "_VAL);\n";
            }
          else
            { // Much simpler, pass by value semantics
              out << "    jlong _" << arg->name << " = (jlong) _addr_to_kdu_long("
                  << arg->name << ");\n";
            }
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
  kdd_variable *arg;
  for (arg=func->args; (arg != NULL) && arg->jni_bindable; arg=arg->next)
    {
      kdd_type &type = arg->type;
      if (!type.by_ref)
        continue;
      if ((type.known_primitive != NULL) && !type.bind_as_opaque_pointer)
        { // Primitive type passed by reference
          assert((type.derefs + type.as_array) == 0);
          out << "    __env->Get"
              << (char)(toupper(type.known_primitive->java_name[0]))
              << type.known_primitive->java_name+1 << "ArrayRegion(_"
              << arg->name << ",0,1,&_" << arg->name << "_VAL);\n";
          out << "    " << arg->name << " = (" << type.known_primitive->name
              << ") _" << arg->name << "_VAL;\n";
        }
      else if (type.unknown_name != NULL)
        { // Unknown pointer passed by reference
          assert((type.derefs == 1) && (type.as_array == 0));
          out << "    __env->GetLongArrayRegion(_"
              << arg->name << ",0,1,&_" << arg->name << "_VAL);\n";
          out << "    " << arg->name << " = (" << type.unknown_name << " *)(_"
              << arg->name << "_VAL);\n";
        }
    }
}

/*****************************************************************************/
/* STATIC              implement_callback_return_result                      */
/*****************************************************************************/

static void
  implement_callback_return_result(kdd_func *func, ostream &out)
{
  kdd_type &type = func->return_type;
  if (type.is_void())
    return;
  assert(!type.by_ref); // No bindable function can return reference types.
  if ((type.known_class != NULL) &&
      (type.known_class->binding == KDD_BIND_IFC) &&
      ((type.derefs+type.as_array) == 0))
    { // Returning an interface object by value
      implement_get_native(type.known_class,"_Result","Result",out);
      out << "    return Result;\n";
    }
  else if ((type.known_class != NULL) &&
           (type.known_class->binding != KDD_BIND_COPY) &&
           ((type.derefs+type.as_array) == 0))
    { // Returning an object which can be copied by value
      implement_get_native(type.known_class,"_Result","Result",out);
      out << "    return *Result;\n";
    }
  else if ((type.known_class != NULL) &&
           (type.known_class->binding != KDD_BIND_IFC) &&
           (type.derefs == 1) && (type.as_array == 0))
    { // Returning a non-interface object by pointer
      implement_get_native(type.known_class,"_Result","Result",out,true);
      out << "    return Result;\n";
    }
  else if ((type.known_primitive != NULL) &&
           ((type.derefs+type.as_array)==0) &&
           !type.known_primitive->string_type)
    { // Returning a simple primitive by value
      if (strcmp(type.known_primitive->name,"bool") == 0)
        out << "    return (_Result)?true:false;\n";
      else
        out << "    return (" << type.known_primitive->name << ") _Result;\n";
    }
  else if ((type.known_primitive != NULL) && type.bind_as_opaque_pointer)
    { // Returning primitive array as opaque pointer
      assert(type.derefs==1);
      out << "    return (" << type.known_primitive->name << " *) "
          << "_kdu_long_to_addr(_Result);\n";
    }
  else if ((type.unknown_name != NULL) &&
           (type.derefs==1) && (type.as_array==0))
    { // Returning an opaque pointer
      out << "    return (" << type.unknown_name << " *) "
          << "_kdu_long_to_addr(_Result);\n";
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
  implement_callback_result_declare(kdd_func *func, ostream &out)
{
  kdd_type &type = func->return_type;
  if (type.is_void())
    return;
  if (type.known_class != NULL)
    out << "    jobject _Result;\n";
  else if ((type.known_primitive != NULL) && !type.bind_as_opaque_pointer)
    out << "    " << type.known_primitive->jni_name << " _Result;\n";
  else if ((type.known_primitive != NULL) && type.bind_as_opaque_pointer)
    out << "    jlong _Result;\n";
  else if (type.unknown_name != NULL)
    out << "    jlong _Result;\n";
}

/*****************************************************************************/
/* STATIC                  implement_callback_binding                        */
/*****************************************************************************/

static void
  implement_callback_binding(kdd_func *func, ostream &out)
{
  kdd_variable *arg;

  if (func->return_type.exists())
    func->return_type.print(out,NULL);
  out << JNI_DELEGATOR_PREFIX << func->owner->name
      << "::" << func->name << "(";
  int nargs=func->num_jni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    {
      arg->type.print(out,arg->name);
      if (nargs > 1)
        out << ", ";
    }
  out << ")\n{\n";
  out << "  try {\n";
  out << "    JNIEnv *__env = _jniEnv();\n";
  implement_class_loading(func->owner,false,func->args,
                          func->return_type.known_class,out);
  implement_callback_args_in(func,out);

  out << "    jmethodID __" << func->java_name << "_ID = "
      << "__env->GetMethodID(__env->GetObjectClass(_self),\""
      << func->java_name << "\",\"(";
  nargs = func->num_jni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    if (arg->type.known_primitive != NULL)
      {
        if (arg->type.bind_as_opaque_pointer)
          out.put('J'); // Java signature for type "long"
        else
          {
            if (arg->type.by_ref)
              out << "["; // Java signature prefix for arrays
            out << arg->type.known_primitive->java_signature;
          }
      }
    else if (arg->type.unknown_name != NULL)
      out.put('J');
    else if (arg->type.known_class != NULL)
      out << "Lkdu_jni/" << arg->type.known_class->java_name << ";";
  out << ")";
  if (func->return_type.known_primitive != NULL)
    {
      if (func->return_type.bind_as_opaque_pointer)
        out.put('J'); // Java signature for type "long"
      else
        out << func->return_type.known_primitive->java_signature;
    }
  else if (func->return_type.unknown_name != NULL)
    out.put('J');
  else if (func->return_type.known_class != NULL)
    out << "Lkdu_jni/" << func->return_type.known_class->java_name << ";";
  out << "\");\n";

  implement_callback_result_declare(func,out);
  out << "    ";
  if (!func->return_type.is_void())
    out << "_Result = ";

  out << "__env->Call";
  if (func->return_type.known_class != NULL)
    out << "Object";
  else if (func->return_type.unknown_name != NULL)
    out << "Long";
  else if (func->return_type.bind_as_opaque_pointer)
    out << "Long";
  else
    {
      assert(func->return_type.known_primitive != NULL);
      out.put((char) toupper(func->return_type.known_primitive->java_name[0]));
      out << func->return_type.known_primitive->java_name+1;
    }
  out << "Method(_self," << "__" << func->java_name << "_ID";
  nargs = func->num_jni_args;
  for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
    out << ",_" << arg->name;
  out << ");\n";
  out << "    jthrowable __cbk_exc = __env->ExceptionOccurred();\n";
  out << "    if (__cbk_exc != NULL)\n";
  out << "      {\n";
  out << "        kdu_exception __kdu_exc = KDU_NULL_EXCEPTION;\n";
  out << "        if (__env->IsInstanceOf(__cbk_exc,\n"
         "                 __env->FindClass(\"java/lang/OutOfMemoryError\")))\n";
  out << "          __kdu_exc = KDU_MEMORY_EXCEPTION;\n";
  out << "        else\n";
  out << "          {\n";
  out << "            jclass __cbk_class = __env->FindClass(\"kdu_jni/KduException\");\n";
  out << "            if (__cbk_class != NULL)\n";
  out << "              {\n";
  out << "                __kdu_exc = KDU_CONVERTED_EXCEPTION;\n";
  out << "                if (__env->IsInstanceOf(__cbk_exc,__cbk_class))\n";
  out << "                  {\n";
  out << "                    jfieldID field_id = __env->GetFieldID(__cbk_class,\n"
         "                                          \"kdu_exception_code\",\"I\");\n";
  out << "                    if (field_id != NULL)\n";
  out << "                      __kdu_exc = (kdu_exception)\n"
         "                                __env->GetIntField(__cbk_exc,field_id);";
  out << "                  }\n";
  out << "              }\n";
  out << "          }\n";
  out << "        __env->ExceptionClear();\n";
  out << "        kdu_rethrow(__kdu_exc);\n";
  out << "      }\n";

  implement_callback_args_out(func,out);
  if (func->return_type.is_void())
    out << "    return;\n";
  else
    implement_callback_return_result(func,out);

  out << "  } catch(std::bad_alloc &exc) { throw exc; }\n"
         "  catch(...) { }\n";
  out << "  throw KDU_CONVERTED_EXCEPTION;\n";
  implement_return_default(func->return_type,out,true);
  out << "}\n";
}

/*****************************************************************************/
/* STATIC              generate_java_methods_from_defaults                   */
/*****************************************************************************/

static void
  generate_java_methods_from_defaults(kdd_func *func, ostream &out)
  /* This function scans through the list of arguments, generating extra
     Java methods to give application programmers simple access to default
     parameters.
  */
{
  int num_args = 0;
  int default_args = 0;
  kdd_variable *arg;
  for (arg=func->args; (arg != NULL) && arg->jni_bindable; arg=arg->next)
    {
      num_args++;
      if (arg->initializer != NULL)
        {
          if ((arg->type.known_class != NULL) &&
              (arg->type.known_class->binding == KDD_BIND_REF) &&
              ((arg->type.derefs+((arg->type.as_array)?1:0))==1) &&
              (strcmp(arg->initializer,"NULL") == 0))
            default_args++;
          else if ((arg->type.unknown_name != NULL) &&
                   (strcmp(arg->initializer,"NULL") == 0))
            default_args++;
          else if (arg->type.known_primitive == NULL)
            default_args = 0;
          else if ((strcmp(arg->initializer,"false") == 0) ||
                   (strcmp(arg->initializer,"true") == 0) ||
                   (strcmp(arg->initializer,"NULL") == 0))
            default_args++;
          else
            {
              char *ch = arg->initializer;
              bool have_digit = false;
              bool have_dot = false;
              bool have_e = false;
              for (; *ch != '\0'; ch++)
                if (isdigit(*ch))
                  have_digit = true;
                else if ((*ch == '.') && !have_dot)
                  have_dot = true;
                else if (((*ch == '-') || (*ch == '+')) &&
                         ((ch == arg->initializer) ||
                          (ch[-1]=='e') || (ch[-1]=='E')))
                  continue;
                else if (((*ch == 'e') || (*ch == 'E')) && !have_e)
                  have_e = true;
                else if (((*ch == 'F') || (*ch == 'f') ||
                          (*ch == 'L') || (*ch == 'l')) && (ch[1] == '\0'))
                  continue;
                else
                  break;
              if (*ch == '\0')
                default_args++;
              else
                default_args = 0;
            }
        }
      else if (default_args)
        { kdu_error e;
          e << "Error encountered at line " << func->line_number
            << " in file, \"" << func->file->name << "\"; all arguments "
               "with default initializers must appear at the end of the "
               "argument list.";
        }
    }
  if (!default_args)
    return;

  for (; default_args > 0; default_args--)
    {
      if (func->is_constructor)
        out << "  private static long Native_create(";
      else
        {
          out << "  public ";
          func->return_type.write_as_java(out,true);
          out << " " << func->java_name << "(";
        }
      int arg_count = num_args - default_args;
      for (arg=func->args; arg_count > 0; arg=arg->next, arg_count--)
        {
          arg->type.write_as_java(out,false);
          out << " _" << arg->name;
          if (arg_count > 1)
            out << ", ";
        }
      if (func->is_constructor)
        out << ")\n";
      else
        out << ") throws KduException\n";
      out << "  {\n";
      for (; (arg != NULL) && arg->jni_bindable; arg=arg->next)
        {
          if (arg->type.known_class == NULL)
            continue;
          out << "    " << arg->type.known_class->java_name
              << " " << arg->name << " = null;\n";
        }
      out << "    ";
      if (func->is_constructor)
        out << "return Native_create(";
      else
        {
          if (!func->return_type.is_void())
            out << "return ";
          out << func->java_name << "(";
        }
      arg_count = num_args - default_args;
      for (arg=func->args; (arg != NULL) && arg->jni_bindable;
           arg=arg->next, arg_count--)
        {
          if (arg_count > 0)
            out << "_" << arg->name;
          else if (arg->type.known_class != NULL)
            out << arg->name;
          else if (arg->type.unknown_name != NULL)
            out << "0";
          else if (strcmp(arg->initializer,"NULL") == 0)
            out << "null";
          else
            {
              out << "(";
              arg->type.write_as_java(out,false);
              out << ") " << arg->initializer;
            }
          if ((arg->next != NULL) && arg->next->jni_bindable)
            out << ",";
        }
      out << ");\n  }\n";

      if (!func->is_constructor)
        continue;

      // For constructors, we have just implemented an overloaded version of
      // the Native_create function.  We now need to implement the actual
      // constructor which calls it.
      out << "  public " << func->owner->java_name << "(";
      arg_count = num_args - default_args;

      for (arg=func->args; arg_count > 0; arg=arg->next, arg_count--)
        {
          arg->type.write_as_java(out,false);
          out << " _" << arg->name;
          if (arg_count > 1)
            out << ", ";
        }
      out << ") {\n";
      out << "    this(Native_create(";
      arg_count = num_args - default_args;
      for (arg=func->args; arg_count > 0; arg=arg->next, arg_count--)
        {
          out << "_";
          out << arg->name;
          if (arg_count > 1)
            out << ", ";
        }
      out << "));\n";
      // Call Native_init method to save reference to this
      // for callback classes
      if (func->owner->has_callbacks)
        {
          out << "    this.Native_init();\n";
        }
      out << "  }\n";
    }
}

/*****************************************************************************/
/* STATIC                    generate_java_classes                           */
/*****************************************************************************/

static void
  generate_java_classes(kdd_class *all_classes, const char *java_dir)
{
  char *path = new char[strlen(java_dir)+1024]; // Should be plenty!

  { // Generate exception class
    strcpy(path,java_dir);
    strcat(path,"/KduException.java");
    ofstream out;
    if (!create_file(path,out))
      { kdu_error e;
        e << "Unable to create the Java class file, \"" << path << "\"."; }
    out << "package kdu_jni;\n\n";
    out << "public class KduException extends Exception {\n";
    out << "    public KduException() {\n"
        << "      super();\n"
        << "      this.kdu_exception_code = Kdu_global.KDU_NULL_EXCEPTION;\n"
        << "    }\n";
    out << "    public KduException(String message) {\n"
        << "      super(message);\n"
        << "      this.kdu_exception_code = Kdu_global.KDU_NULL_EXCEPTION;\n"
        << "    }\n";
    out << "    public KduException(int exc_code) {\n"
        << "      super();\n"
        << "      this.kdu_exception_code = exc_code;\n"
        << "    }\n";
    out << "    public KduException(int exc_code, String message) {\n"
        << "      super(message);\n"
        << "      this.kdu_exception_code = exc_code;\n"
        << "    }\n";
    out << "    public int Get_kdu_exception_code() {\n"
        << "      return kdu_exception_code;\n"
        << "    }\n";
    out << "    public boolean Compare(int exc_code) {\n"
        << "      if (exc_code == kdu_exception_code)\n"
        << "        return true;\n"
        << "      else\n"
        << "        return false;\n"
        << "    }\n";
    out << "    private int kdu_exception_code;\n"; 
    out << "}\n";
    out.close();
  }

  // Generate bound classes
  for (kdd_class *cls=all_classes; cls != NULL; cls=cls->next)
    {
      if (cls->java_name == NULL)
        continue;
      assert(cls->binding != KDD_BIND_NONE);

      strcpy(path,java_dir);
      strcat(path,"/");
      strcat(path,cls->java_name);
      strcat(path,".java");

      ofstream out;
      if (!create_file(path,out))
        { kdu_error e;
          e << "Unable to create the Java class file, \"" << path << "\"."; }
      out << "package kdu_jni;\n\n";
      out << "public class " << cls->java_name;
      if (cls->base.exists())
        {
          if ((cls->base.known_class == NULL) ||
              (cls->base.known_class->binding != cls->binding))
            { kdu_error e;
              e << "Class/struct defined on line "
                << cls->line_number << " of file, \""
                << cls->file->name
                << "\" has a different binding to that of the class "
                   "from which it derives.  All classes for which a "
                   "language binding exists must have the same binding "
                   "type as their base classes.";
            }
          out << " extends ";
          cls->base.write_as_java(out,false);
        }
      out << " {\n";
      out << "  static {\n"
          << "    System.loadLibrary(\"kdu_jni\");\n"
          << "    Native_init_class();\n"
          << "  }\n";

      out << "  private static native void Native_init_class();\n";

      // Build state and basic constructor/destructor logic
      kdd_func *func;
      kdd_variable *arg;
      kdd_constant *cst;
      if ((cls->binding != KDD_BIND_GLOBALS) && !cls->base.exists())
        { // Base class has a pointer to the native object, or interface state
          out << "  public long _native_ptr = 0;\n";
        }
      if ((cls->binding == KDD_BIND_REF) || (cls->binding == KDD_BIND_COPY))
        { // Class has destructor and derivation-safe constructors
          out << "  protected " << cls->java_name << "(long ptr) {\n";
          if (!cls->base.exists())
            out << "    _native_ptr = ptr;\n";
          else
            out << "    super(ptr);\n"; 
          out << "  }\n";
          
          if (!cls->destroy_private)
            { // Can destroy the internal object from Java
              
              // Begin by determining whether or not to make an explicit
              // "Native_destroy" function public.  We do this only if the
              // C++ object has an explicit destructor somewhere in its
              // inheritance chain.
              kdd_class *base;
              for (base=cls; base != NULL; base=base->base.known_class)
                {
                  for (func=base->functions; func != NULL; func=func->next)
                    if (func->is_destructor)
                      break;
                  if (func != NULL)
                    break;
                }
              if (base != NULL)
                out << "  public native void Native_destroy();\n";
              else
                out << "  private native void Native_destroy();\n";

              // Now implement a `finalize' member to be called if/when the
              // garbage collector cleans up the Java interfacing object.
              out << "  public void finalize() {\n"
                     "    if ((_native_ptr & 1) != 0)\n"
                     "      { // Resource created and not donated\n"
                     "        Native_destroy();\n"
                     "      }\n"
                     "  }\n";
            }

          // Declare derivation-safe constructors for non-abstract classes.
          bool initial_constructor = true;
          for (func=cls->functions; func != NULL; func=func->next)
            if (func->is_constructor && (func->java_name != NULL))
              { // Each constructor has Java constructor and static JNI creator
                out << "  private static native long Native_create(";
                int nargs = func->num_jni_args;
                for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
                  {
                    arg->type.write_as_java(out,false);
                    out << " _" << arg->name;
                    if (nargs > 1)
                      out << ", ";
                  }
                out << ");\n";
                // Declare Native_init method for callback classes
                if (cls->has_callbacks && initial_constructor)
                  {
                      out << "  private native void Native_init();\n";
                  }
                out << "  public " << cls->java_name << "(";
                nargs = func->num_jni_args;
                for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
                  {
                    arg->type.write_as_java(out,false);
                    out << " _" << arg->name;
                    if (nargs > 1)
                      out << ", ";
                  }
                out << ") {\n";
                out << "    this(Native_create(";
                nargs = func->num_jni_args;
                for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
                  {
                    out << "_";
                    out << arg->name;
                    if (nargs > 1)
                      out << ", ";
                  }
                out << "));\n";
                // Call Native_init method to save reference to this
                // for callback classes
                if (cls->has_callbacks)
                  {
                      out << "    this.Native_init();\n";
                  }
                out << "  }\n";
                
                generate_java_methods_from_defaults(func,out);
                initial_constructor = false;
              }
        }

      // Declare regular native functions
      for (func=cls->functions; func != NULL; func=func->next)
        if ((func->java_name != NULL) &&
            !(func->is_constructor || func->is_destructor))
          {
            if ((func->overrides != NULL) && func->is_virtual &&
                (func->binding == func->overrides->binding))
              continue; // Existing function in base class does the job
            out << "  public ";
            if (func->binding != KDD_BIND_CALLBACK)
              {
                out << "native ";
                if (cls->binding == KDD_BIND_GLOBALS)
                  out << "static ";
              }
            func->return_type.write_as_java(out,true);
            out << " " << func->java_name << "(";
            int nargs = func->num_jni_args;
            for (arg=func->args; nargs > 0; nargs--, arg=arg->next)
              {
                arg->type.write_as_java(out,false);
                out << " _" << arg->name;
                if (nargs > 1)
                  out << ", ";
              }
            out << ") throws KduException";
            if (func->binding == KDD_BIND_CALLBACK)
              {
                out << "\n  {\n"
                    << "    // Override in a derived class to respond "
                       "to the callback\n";
                kdd_type &type = func->return_type;
                if (type.known_class != NULL)
                  out << "    return null;\n";
                else if (type.known_primitive != NULL)
                  {
                    if (type.known_primitive->string_type)
                      out << "    return null;\n";
                    else if (strcmp(type.known_primitive->name,"bool") == 0)
                      out << "    return false;\n";
                    else if (strcmp(type.known_primitive->java_name,"void")!=0)  
                      out << "    return "
                             "("<<type.known_primitive->java_name<<") 0;\n";
                    else
                      out << "    return;\n";
                  }
                out << "  }\n";
              }
            else
              out << ";\n";

            generate_java_methods_from_defaults(func,out);
          }

      // Declare constants as public final member variables
      if (cls->constants != NULL)
        out  << "\n";
      for (cst=cls->constants; cst != NULL; cst=cst->next)
        {
          out << "  public static final " << cst->type->java_name
              << " " << cst->name << " = (" << cst->type->java_name
              << ") " << cst->value << ";\n";
        }
      out << "}\n";
      out.close();
    }
  delete[] path;
}

/*****************************************************************************/
/* STATIC                     generate_jni_header                            */
/*****************************************************************************/

static void
  generate_jni_header(kdd_class *classes, const char *jni_dir)
{
  char *path = new char[strlen(jni_dir)+strlen("kdu_jni.h")+2];
  strcpy(path,jni_dir);
  strcat(path,"/");
  strcat(path,"kdu_jni.h");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the JNI source file, \"" << path << "\"."; }
  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n\n";
  out << "#ifndef KDU_JNI_H\n"
         "#define KDU_JNI_H\n\n";
  out << "#include <jni.h> // If the compiler trips up here, you need to\n";
  out << "     // modify the makefile/workspace to add the correct path to\n";
  out << "     // the Java SDK \"include\" directory and machine-specific\n";
  out << "     // sub-directory (e.g., \"include/linux\") on your\n";
  out << "     // platform.  You may first need to install the Java SDK.\n";
  for (kdd_class *cls=classes; cls != NULL; cls=cls->next)
    {
      kdd_func *func;

      if (cls->java_name == NULL)
        continue;
      assert(cls->binding != KDD_BIND_NONE);
      print_banner(cls->java_name,out);

      // Declare class initializer
      out << "extern \"C\"\n";
      generate_jni_func_name("void",cls->java_name,"Native_init_class",
                             NULL,true,false,out);
      out << ";\n";

      if ((cls->binding == KDD_BIND_REF) || (cls->binding == KDD_BIND_COPY))
        { // Class has destructor and derivation-safe constructors
          if (!cls->destroy_private)
            {
              out << "extern \"C\"\n";
              generate_jni_func_name("void",cls->java_name,"Native_destroy",
                                     NULL,false,false,out);
              out << ";\n";
            }
          bool initial_constructor = true;
          for (func=cls->functions; func != NULL; func=func->next)
            if (func->is_constructor && (func->java_name != NULL))
              { // Each constructor has Java constructor and static JNI creator
                out << "extern \"C\"\n";
                generate_jni_func_name("jlong",cls->java_name,"Native_create",
                                       func->args,true,
                                       (func->overload_next!=NULL),out);
                out << ";\n";
                // Declare Native_init method for callback classes
                if (cls->has_callbacks && initial_constructor)
                  {
                    out << "extern \"C\"\n";
                    generate_jni_func_name("void",cls->java_name,"Native_init",
                                           NULL, false, false, out);
                    out << ";\n";
                  }
                initial_constructor = false;
              }
        }

      // Declare regular native functions
      for (func=cls->functions; func != NULL; func=func->next)
        if ((func->java_name != NULL) &&
            (func->binding != KDD_BIND_CALLBACK) &&
            !(func->is_constructor || func->is_destructor))
          {
            if ((func->overrides != NULL) && func->is_virtual &&
                (func->binding == func->overrides->binding))
              continue; // Existing function in base class does the job
            out << "extern \"C\"\n";
            generate_jni_func_name(func->return_type.get_jni_name(),
                                   cls->java_name,func->java_name,
                                   func->args,func->is_static,
                                   (func->overload_next!=NULL),out);
            out << ";\n";
          }

        if (cls->has_callbacks)
          {
            out << "\n";
            generate_derived_delegator(cls,out);
          }
    }

  out << "\n#endif // KDU_JNI_H\n";
  out.close();
}

/*****************************************************************************/
/* STATIC                 generate_jni_implementation                        */
/*****************************************************************************/

static void
  generate_jni_implementation(kdd_file *header_files,
                              kdd_class *classes, const char *jni_dir,
                              const char *aux_dir)
{
  char *path = new char[strlen(jni_dir)+strlen("kdu_jni.cpp")+2];
  strcpy(path,jni_dir);
  strcat(path,"/");
  strcat(path,"kdu_jni.cpp");
  ofstream out;
  if (!create_file(path,out))
    { kdu_error e;
      e << "Unable to create the JNI source file, \"" << path << "\"."; }
  out << "// This file has been automatically generated by \"kdu_hyperdoc\"\n";
  out << "// Do not edit manually.\n";
  out << "// Copyright 2001, David Taubman, "
         "The University of New South Wales (UNSW)\n\n";
  out << "#include <new>\n";
  out << "#include <assert.h>\n";
  out << "#include \"";
  write_relative_path(aux_dir,jni_dir,"kdu_aux.h",out);
  out << "\"\n";
  out << "#include \"kdu_jni.h\"\n";

  // Create global mutex for absolutely safe class initialization in
  // multi-threaded code.
  print_banner("Global mutex used to guard class loading logic",out);

  out << "\n";
  out << "class kdjni__LOADER_lock {\n"
      << "  public: // Member functions\n"
      << "    kdjni__LOADER_lock() { mutex.create(); }\n"
      << "    ~kdjni__LOADER_lock() { mutex.destroy(); }\n"
      << "    void acquire() { mutex.lock(); }\n"
      << "    void release() { mutex.unlock(); }\n"
      << "  private: // Data members\n"
      << "    kdu_mutex mutex;\n"
      << "  };\n";
  out << "\n";
  out << "static kdjni__LOADER_lock __class_LOADER_lock;\n";
  out << "\n";

  // Utility functions: checkForJavaException ensures that a java exception is
  // set whenever a C++ exception is caught. If badAlloc is true, then OutOfMemoryException
  // is thrown, otherwise a KduException is thrown. If this function is called with badAlloc
  // set to false, then it should only be called in a context where the calling java
  // method is declared as throwing KduException (i.e. regular methods, and not 
  // Native_create, Native_destroy, Native_init or Native_init_class methods).
  print_banner("Utility functions",out);

  out << "\n";
  out << "static void checkForJavaException(JNIEnv *__env, bool badAlloc, kdu_exception exc)\n";
  out << "{\n";
  out << "  if (__env->ExceptionOccurred() == NULL)\n";
  out << "    {\n";
  out << "        const char *exception_class_name;\n";
  out << "        if (badAlloc)\n";
  out << "            exception_class_name = \"java/lang/OutOfMemoryError\";\n";
  out << "        else\n";
  out << "            exception_class_name = \"kdu_jni/KduException\";\n";
  out << "        jclass exception_class = __env->FindClass(exception_class_name);\n";
  out << "        if (exception_class==NULL)\n";
  out << "            return;\n";
  out << "        if (badAlloc)\n";
  out << "          __env->ThrowNew(exception_class,\n"
         "                          \"std::bad_alloc exception in Kdu library\");\n";
  out << "        else\n";
  out << "          __env->Throw((jthrowable)\n"
         "                       __env->NewObject(exception_class,\n"
         "                                        __env->GetMethodID(exception_class,\n"
         "                                        \"<init>\",\"(I)V\"),(int) exc));\n";
  out << "    }\n";
  out << "}\n";
  out << "\n";

  // Generate the static class description discovery code which is called on
  // demand, at most once, for each class.
  kdd_class *cls;
  print_banner("Class Loading Logic",out);
  for (cls=classes; cls != NULL; cls=cls->next)
    {
      if ((cls->java_name == NULL) || ( cls->binding == KDD_BIND_GLOBALS ))
        continue;
      out << "\n";
      out << "static jclass " << cls->java_name << "_CLS = NULL;\n";
      out << "static jfieldID " << cls->java_name << "_PTR = NULL;\n";
      out << "static void " << cls->java_name << "_LOADER(JNIEnv *__env)\n{\n";
      out << "  jclass kdu_class = __env->FindClass(\"kdu_jni/"
          << cls->java_name << "\");\n";
      out << "  if (kdu_class==NULL) throw KDU_NULL_EXCEPTION;\n";
      out << "  kdu_class = (jclass) __env->NewGlobalRef(kdu_class);\n";
      out << "  jfieldID kdu_field = __env->GetFieldID(kdu_class,\"_native_ptr\",\"J\");\n";
      out << "  if (kdu_field==NULL) throw KDU_NULL_EXCEPTION;\n";
      out << "  __class_LOADER_lock.acquire();\n";
      out << "  if (" << cls->java_name << "_CLS == NULL)\n";
      out << "    {\n";
      out << "      " << cls->java_name << "_PTR = kdu_field;\n";
      out << "      " << cls->java_name << "_CLS = kdu_class;\n";
      out << "      kdu_class = NULL;\n";
      out << "    }\n";
      out << "  __class_LOADER_lock.release();\n";
      out << "  if (kdu_class != NULL)\n";
      out << "    __env->DeleteGlobalRef(kdu_class);\n";
      out << "}\n";
    }

  // Generate the function implementations
  for (cls=classes; cls != NULL; cls=cls->next)
    {
      kdd_func *func;

      if (cls->java_name == NULL)
        continue;
      assert(cls->binding != KDD_BIND_NONE);

      print_banner(cls->java_name,out);

      out << "\n";
      generate_jni_func_name("void",cls->java_name,"Native_init_class",
                             NULL,true,false,out);
      out << "\n{\n";
      if ( cls->binding != KDD_BIND_GLOBALS )
        {
          out << "  if (" << cls->java_name << "_CLS != NULL) return;\n";
          out << "  try {\n";
          out << "    " << cls->java_name << "_LOADER(__env);\n";
          out << "  } catch(...) {};\n";
        }
      out << "}\n";

      if ((cls->binding == KDD_BIND_REF) || (cls->binding == KDD_BIND_COPY))
        {
          if (!cls->destroy_private)
            { // Implement object destruction function
              out << "\n";
              generate_jni_func_name("void",cls->java_name,"Native_destroy",
                                     NULL,false,false,out);
              out << "\n{\n";
              implement_class_loading(cls,false,NULL,NULL,out);
              out << "    jlong self_ref = "
                     "__env->GetLongField(_self,"<<cls->java_name<<"_PTR);\n";
              out << "    if (self_ref == 0) return;\n";
              const char *prefix = (cls->has_callbacks)?AUX_EXTENSION_PREFIX:"";
              out << "    __env->SetLongField(_self,"<<cls->java_name<<"_PTR,0);\n";
              out << "    if ((self_ref & ((jlong) 1)) != 0)\n"
                  << "      {\n"
                  << "        " << prefix << cls->name << " *self = ("
                  << prefix << cls->name
                  << " *) _kdu_long_to_addr(self_ref - ((jlong) 1));\n";
              if (cls->has_callbacks)
                {
                  out << "        " JNI_DELEGATOR_PREFIX << cls->name
                      << " *self_delegator ="
                         "(" JNI_DELEGATOR_PREFIX << cls->name << " *)(self->_delegator);\n";
                  out << "        delete self;\n";
                  out << "        if (self_delegator != NULL)\n"
                         "          delete self_delegator;\n";
                }
              else
                out << "        delete self;\n";
              out << "      }\n";
              out << "}\n";
            }

          // Implement object creation functions
          bool initial_constructor = true;
          for (func=cls->functions; func != NULL; func=func->next)
            if (func->is_constructor && (func->java_name != NULL))
              {
                out << "\n";
                generate_jni_func_name("jlong",cls->java_name,"Native_create",
                                       func->args,true,
                                       (func->overload_next!=NULL),out);
                out << "\n{\n";
                out << "  try {\n";
                implement_class_loading(cls,true,func->args,NULL,out);
                implement_args_in(func->args,out);
                if (!cls->has_callbacks)
                  {
                    out << "    " << cls->name << " *self =\n"
                        << "      new " << cls->name << "(";
                    implement_args_pass(func->args,out);
                    out << ");\n";
                  }
                else
                  {
                    out << "    " AUX_EXTENSION_PREFIX << cls->name << " *self =\n"
                        << "      new " AUX_EXTENSION_PREFIX << cls->name << "(";
                    implement_args_pass(func->args,out);
                    out << ");\n";
                    out << "    self->_delegator = new " JNI_DELEGATOR_PREFIX
                        << cls->name << "(__env);\n";
                  }

                implement_args_out(func->args,out);
                implement_args_cleanup(func->args,out);
                out << "    return ((jlong) _addr_to_kdu_long(self)) | "
                       "((jlong) 1);\n";
                out << "  } catch(std::bad_alloc &)\n"
                       "  { checkForJavaException(__env,true,KDU_NULL_EXCEPTION); }\n";
                out << "  catch(...) {};\n";
                out << "  return (jlong) 0;\n";
                out << "}\n";

                // Add Native_init method to save a reference to the java
                // object for callback classes
                if (cls->has_callbacks && initial_constructor)
                  {
                    out << "\n";
                    generate_jni_func_name("void",cls->java_name,"Native_init",
                                           NULL,false,false,out);
                    out << "\n{\n";
                    implement_class_loading(cls,false,NULL,NULL,out);
                    out << "    " AUX_EXTENSION_PREFIX << cls->name << " *self = ("
                        << AUX_EXTENSION_PREFIX << cls->name << " *)\n";
                    out << "      _kdu_long_to_addr(__env->GetLongField(_self,"
                        << cls->java_name << "_PTR) & ~((jlong) 1));\n";
                    out << "    " JNI_DELEGATOR_PREFIX << cls->name << " *self_delegator = "
                           "(" JNI_DELEGATOR_PREFIX << cls->name << " *)(self->_delegator);\n";
                    out << "    self_delegator->_init(__env,_self);\n";
                    out << "}\n";
                  }
                initial_constructor = false;
              }
        }

      // Implement regular native functions
      for (func=cls->functions; func != NULL; func=func->next)
        if ((func->java_name != NULL) &&
            (func->binding != KDD_BIND_CALLBACK) &&
            !(func->is_constructor || func->is_destructor))
          {
            if ((func->overrides != NULL) && func->is_virtual &&
                (func->binding == func->overrides->binding))
              continue; // Existing function in base class does the job
            out << "\n";
            generate_jni_func_name(func->return_type.get_jni_name(),
                                   cls->java_name,func->java_name,
                                   func->args,func->is_static,
                                   (func->overload_next!=NULL),out);
            out << "\n{\n";
            out << "  try {\n";
            implement_class_loading(cls,func->is_static,func->args,
                                    func->return_type.known_class,out);
            bool need_cleanup = implement_args_in(func->args,out);
            implement_result_declare(func->return_type,out);
            if (need_cleanup)
              out << "    try { // So we can clean up temporary arrays\n";
            if (cls->name == NULL)
              {
                implement_result_assignment(func->return_type,out);
                out << func->name << "(";
                implement_args_pass(func->args,out);
                out << ");\n";
              }
            else if (func->is_static)
              {
                implement_result_assignment(func->return_type,out);
                out << cls->name << "::" << func->name << "(";
                implement_args_pass(func->args,out);
                out << ");\n";
              }
            else if (cls->binding == KDD_BIND_IFC)
              {
                implement_get_native(cls,"_self","self",out);
                implement_result_assignment(func->return_type,out);
                out << "self." << func->name << "(";
                implement_args_pass(func->args,out);
                out << ");\n";
                implement_set_native(cls,"self","_self",out);
              }
            else
              {
                implement_get_native(cls,"_self","self",out);
                implement_result_assignment(func->return_type,out);
                out << "self->" << func->name << "(";
                implement_args_pass(func->args,out);
                out << ");\n";
                if (func->binding == KDD_BIND_DONATE)
                  implement_set_native(cls,"self","_self",out);
              }
            if (need_cleanup)
              {
                out << "    } catch (...) {\n";
                implement_args_cleanup(func->args,out);
                out << "    throw;\n";
                out << "    }\n";
              }
            implement_args_out(func->args,out);
            implement_args_cleanup(func->args,out);
            implement_return_result(func,out);
            out << "  } catch(std::bad_alloc &)\n"
                   "  { checkForJavaException(__env,true,KDU_NULL_EXCEPTION); }\n";
            out << "  catch (kdu_exception exc)\n"
                   "  { checkForJavaException(__env,false,exc); }\n";
            out << "  catch (...)\n"
                   "  { checkForJavaException(__env,false,KDU_CONVERTED_EXCEPTION); }\n";
            implement_return_default(func->return_type,out);
            out << "}\n";
          }

      if (!cls->has_callbacks)
        continue;

      // Implement callback functions
      for (func=cls->functions; func != NULL; func=func->next)
        if ((func->binding == KDD_BIND_CALLBACK) &&
            (func->java_name != NULL) && func->is_virtual)
          {
            out << "\n";
            implement_callback_binding(func,out);
          }
    }

  out.close();
}


/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/*                             make_java_bindings                            */
/*****************************************************************************/

void make_java_bindings(kdd_file *header_files, kdd_class *classes,
                        const char *java_dir, const char *jni_dir,
                        const char *aux_dir)
{
  kdd_class *cls;
  for (cls = classes; cls != NULL; cls=cls->next)
    {
      if (cls->binding == KDD_BIND_NONE)
        continue;

      const char *name = cls->name;
      if (name == NULL)
        name = "Kdu_global"; // Name of Globals class
      else
        cls->file->has_bindings = true;
      cls->java_name = new char[strlen(name)+1];
      strcpy(cls->java_name,name);
      cls->java_name[0] = (char) toupper(cls->java_name[0]);
      find_bindable_functions(cls);
    }
  generate_java_classes(classes,java_dir);
  generate_jni_header(classes,jni_dir);
  generate_jni_implementation(header_files,classes,jni_dir,aux_dir);
}
