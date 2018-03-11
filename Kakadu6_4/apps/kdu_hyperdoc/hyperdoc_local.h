/*****************************************************************************/
// File: hyperdoc_local.h [scope = APPS/HYPERDOC]
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
   Local definitions used by "kdu_hyperdoc.cpp".
******************************************************************************/

#ifndef HYPERDOC_LOCAL_H
#define HYPERDOC_LOCAL_H

#define AUX_DELEGATOR_PREFIX "_aux_delegator__"
#define AUX_EXTENSION_PREFIX "_aux_extended__"
#define MNI_DELEGATOR_PREFIX "_mni_delegator__"
#define JNI_DELEGATOR_PREFIX "_jni_delegator__"

#include <iostream>
#include <fstream>
using namespace std;

// Defined here:
struct kdd_bind_specific;
class kdd_string;
struct kdd_primitive;
struct kdd_constant;
struct kdd_type;
struct kdd_variable;
struct kdd_func;
struct kdd_class;
struct kdd_file;
struct kdd_index;


#define KDD_BIND_NONE      0 // For classes not bound to JNI
#define KDD_BIND_IFC       1 // Objects which acts as an interface
#define KDD_BIND_REF       2 // Objects must be constructed on heap
#define KDD_BIND_COPY      3 // Object may be copied across func boundaries
#define KDD_BIND_GLOBALS   4 // For the "globals" class

#define KDD_BIND_TRANSIENT 7 // Normal binding for functions & variables
#define KDD_BIND_DONATE    8 // For args which donate the object they reference
                                // or functions which donate their object
#define KDD_BIND_CALLBACK  9 // For virtual functions to be implemented (thru
                                // overriding) in the foreign language binding
#define KDD_BIND_NO_BIND   10// For functions which should have no Java binding

#define KDD_TAG_SYNOPSIS       0x101
#define KDD_TAG_RETURNS        0x102
#define KDD_TAG_ARG            0x103
#define KDD_TAG_PAR            0x104
#define KDD_TAG_BULLET         0x105

/*****************************************************************************/
/*                            kdd_bind_specific                              */
/*****************************************************************************/

struct kdd_bind_specific {
  /* This structure is used to build a list of the class/function names
     specified in "-bind" arguments. */
  public: // member functions
    kdd_bind_specific(const char *string);
    ~kdd_bind_specific()
      {
        if (full_name != NULL) delete[] full_name;
        if (class_name != NULL) delete[] class_name;
        if (func_name != NULL) delete[] func_name;
      }
    bool apply(kdd_class *classes);
      /* Searches for the class/function referenced by this object, setting
         its `bind_specific' member to true if found.  Returns false if
         the class/function could not be found. */
  public: // data
    char *full_name;
    char *class_name;
    char *func_name;
    kdd_bind_specific *next;
  };

/*****************************************************************************/
/*                                kdd_string                                 */
/*****************************************************************************/

class kdd_string {
  public: // Member functions
    kdd_string()
      { buffer_start = buffer_next = new char[16];
        buffer_lim = buffer_start+15; *buffer_start = '\0'; }
    ~kdd_string() { delete[] buffer_start; }
    void append(char *from, char *to,
                bool new_paragraph=false, bool new_bullet=false);
      /* Appends a range of text to the current description, adding a space,
         if necessary between the new existing text and the new characters.
         Words must be fully contained within the supplied range of text.
         The `from' address is included in the range, while the `to'
         address points just beyond the range of text.  It is legal to
         append empty ranges of text, in which case there will be no effect.
         Superfluous spaces will be eliminated.
            If the line begins a new paragraph, the `new_paragraph' argument
         should be set to true.  It is also possible to signal the start of
         a bulleted list item by setting `new_bullet' to true, in which case
         `new_paragraph' will be ignored.
            The function may be used to accumulate comment strings or
         declaration strings. */
    char *get() { return buffer_start; } // Returns a NULL terminated string.
    void reset() { buffer_next = buffer_start; *buffer_next = '\0'; }
    void generate_html(ofstream &out, kdd_variable *reference_args,
                       kdd_func *reference_funcs, kdd_variable *reference_vars,
                       kdd_class *reference_classes);
      /* Generates HTML text containing the string, including hyperlinks
         to any quoted argument names from the `reference_args' list,
         any quoted function names from the `reference_funcs' list,
         any quoted variable names from the `reference_vars' list and any
         fully qualified function or variable names which may be found in the
         `reference_classes' list.  Any or all of these lists may be NULL. */
  private: // Data
    char *buffer_start; // Points to the start of the buffer.
    char *buffer_next; // Points to next location to be written in buffer
    char *buffer_lim; // Points just beyond the last location in the buffer.
  };

/*****************************************************************************/
/*                             kdd_primitive                                 */
/*****************************************************************************/

struct kdd_primitive {
  public: // Member functions
    kdd_primitive()
      { name=NULL; java_name=jni_name=NULL; alias=next=NULL; }
    ~kdd_primitive()
      { if (name != NULL) delete[] name; }
    kdd_primitive *
      init_base(kdd_primitive *list, const char *name, const char *java_name,
                const char *jni_name, const char *java_signature,
                bool java_size_compatible, const char *csharp_name,
                const char *vbasic_name, const char *mni_name,
                bool mni_size_compatible, bool allow_arrays);
      /* Call this function only after creating a new object on the heap.
         The present object is linked into the list headed by `list', and the
         function returns the new head of the list.
            The `java_name', `jni_name' and `java_signature' strings must all
         be supplied, representing compatible primitives in Java, for use as
         C-types in JNI implementations, and the corresponding Java language
         signature string.  You may not create primitive types which have no
         Java equivalents, although the Java equivalents need not necessarily
         have the same size.  The `java_size_compatible' argument should be
         set to true only if it is known that the `name' type and the
         `jni_name' type will always have the same size.  Data types with
         incompatible sizes will automatically be copied across interfaces,
         which may cause some inefficiency when passing arrays of primitives.
            The `csharp_name' and `vbasic_name' arguments supply compatible
         names for the primitive in the C# and Visual Basic languages,
         respectively, while `mni_name' supplies the name to be used in
         Microsoft's "Managed Extensions for C++" where we build bindings
         to C# and Visual Basic.  The `mni_size_compatible' argument
         indicates whether these types are known to have exactly the same
         size as the `name' type in C/C++.  You may not create primitive types
         which have no managed equivalents, although this should not be a
         problem, since there are managed equivalents for all Java types.
            The `allow_arrays' argument is true if arrays of this type are
         allowed.
            The `bit_depth' field should be 0 for non-integer types.  For
         integer-valued types, it should hold the number of bits (8, 16, 32
         or 64), with signed integers having a negative rather than a positive
         value.
      */
    kdd_primitive *new_alias(const char *name, const char *alias_name);
      /* Unlike `init_base', this function creates a new `kdd_primitive'
         record on the heap, linking it into the list headed by the object
         associated with the caller.  If the list already contains an element
         with the supplied `name', the function simply returns a pointer to
         the existing element, without creating a new one.  If `alias_name'
         does not correspond to any element in the list, the function returns
         NULL, which will usually trigger an error condition in the caller.
         Otherwise, a new element is created and fully linked into the
         existing list (linking the `alias' field as well), with the function
         returning a pointer to the new element. */
  public: // Data
    char *name; // Name of the primitive type
    const char *java_name; // Name of compatible Java primitive
    const char *jni_name; // Compatible primitive for JNI implementations
    const char *java_signature; // Java language signature string
    bool java_size_compatible; // True if `name' and `jni_name' are same size
    const char *csharp_name; // Name of compatible C# primitive
    const char *vbasic_name; // Name of compatible Visual Basic primitive
    const char *mni_name; // Name in Microsoft's Managed Extensions for C++
    bool string_type; // True if `name' contains "char"+whitespace+"*"
    bool mni_size_compatible; // True if `name' and `mni_name' are same size
    bool allow_arrays; // True if the type allows arrays
    int bit_depth;
    kdd_primitive *alias; // If non-NULL this is an alias for `alias' primitive
    kdd_primitive *next; // Points to next in list of primitives.
  };

/*****************************************************************************/
/*                                kdd_type                                   */
/*****************************************************************************/

struct kdd_type {
  public: // Member functions
    kdd_type()
      {
        known_class=NULL; known_primitive=NULL; unknown_name=modifier=NULL;
        by_ref = false; derefs = as_array = 0;
        binding=KDD_BIND_TRANSIENT;  bind_as_opaque_pointer=false;
      }
    ~kdd_type()
      {
        if (unknown_name != NULL) delete[] unknown_name;
        if (modifier != NULL) delete[] modifier;
      }
    bool exists()
      {
        return ((known_class != NULL) || (known_primitive != NULL) ||
                (unknown_name != NULL));
      }
    bool operator!() { return !exists(); }
    bool operator==(kdd_type &ref);
    bool operator!=(kdd_type &ref)
      { return !(*this == ref); }
    void finalize(kdd_class *classes, kdd_primitive *primitives);
       /* Tries to find any unknown type name in the definitions offered
          by the supplied list of classes, or the supplied list of primitive
          types.  Note that some primitive types may include their own
          indirection modifiers or modifiers, in which case the present
          function may decrement the number of indirection levels or
          alter the modifiers. */
    void print(ostream &out, const char *name);
      /* Prints the C++ argument type declaration, including all modifiers and
         dereferences, followed by the actual argument name.  For return
         types, `name' should be NULL. */
    void generate_html(ostream &out);
      /* Generates text describing the data type, having the form
         <pre-modifier> <type name> <indirection modifiers>, where
         the type name string includes any appropriate hyperlinks. */
    bool is_void();
      /* Returns true if this type is plain "void", or if there is no
         function return type at all.  These conditions can only be true
         for empty function return types. */
    bool is_bindable(bool for_return);
      /* Returns true if variables of this type may be bound to Java/C#/VB
         function arguments (`for_return' is false) or Java/C#/VB function
         return values (`for_return' is true). */
    void write_as_java(ostream &out, bool for_return);
      /* Writes the type as a valid Java function return type or argument
         type.  You should never call this function unless `is_bindable' has
         returned true.  Otherwise, an error may be generated.
            If `for_return' is true, the Java type of a function return value
         is being written.  Otherwise, the definition which is written
         corresponds to a function argument. */
    void write_as_csharp(ostream &out, const char *name, bool for_html);
      /* Similar to `write_as_java', but writes the return type or argument
         definition for a C# function prototype.  For return types,
         `name' should be NULL, whereas for argument types, `name'
         should hold the actual name of the argument, to be written as well.
         The `for_html' argument indicates whether the prototype is being
         generated for inclusion in an HTML file, in which case HTML
         typesetting commands can be included in the output stream.
         Currently, this is the only way in which the function is used. */
    void write_as_vbasic(ostream &out, const char *name, bool for_html);
      /* Same as `write_as_csharp'.  Note, however, that for function return
         types (`name'=NULL), the function should be called after the
         function name and arguments have been written. */
    char *get_jni_name();
      /* Generates a string representing the data type to be used for
         return values or arguments of the present type in JNI function
         names.  The string is written into an internal character
         buffer, a pointer to which is returned.  Note carefully that
         there is only one copy of this internal buffer, so a second
         call to this function will generally invalidate the results
         of a previous call. */
    void write_mni_declaration(ostream &out, const char *name,
                               bool old_syntax, const char *prefix="");
      /* Writes the type declaration, followed by the supplied variable name,
         optionally prefixed by the `prefix' string, and followed by any
         required suffix, so as to form a complete argument or variable
         declaration compatible with Microsoft's managed extensions to C++.
         The purpose of the `prefix' argument is to allow for the unique
         construction of novel variable names, by combining the `prefix' and
         `name' strings.  If `name' is NULL, the function is being used to
         write a function return type declaration. */
  public: // Data
    kdd_class *known_class;
    kdd_primitive *known_primitive;
    char *unknown_name; // Non-NULL only if all of the above fields are NULL
    char *modifier; // Prefixes like "static", "const", ...
    bool by_ref; // True if a "&" post-modifier was used
    int derefs; // Number of "*" characters following type name
    int as_array; // True if the '[]' argument modifier was used.
    int binding; // Either KDD_BIND_TRANSIENT or KDD_BIND_DONATE
    bool bind_as_opaque_pointer;
  };

/*****************************************************************************/
/*                              kdd_constant                                 */
/*****************************************************************************/

struct kdd_constant {
  public: // Member functions
    kdd_constant()
      { type=NULL; name=html_link=html_offset=NULL; value=NULL; next=NULL; }
    kdd_constant(char *name, char *value, kdd_primitive *primitive)
      {
        this->name = new char[strlen(name)+1];
        strcpy(this->name,name);
        this->value = new char[strlen(value)+1];
        strcpy(this->value,value);
        type = primitive;
        html_link = html_offset = NULL;
        next = NULL;
      }
    ~kdd_constant()
      {
        if (name != NULL) delete[] name;
        if (html_link != NULL) delete[] html_link;
        if (value != NULL) delete[] value;
      }
    void create_html_link(const char *html_file);
  public: // Data
    kdd_primitive *type;
    char *name;
    char *html_link; // Holds both HTML file and location in that file.
    char *html_offset;
    char *value;
    kdd_constant *next;
  };

/*****************************************************************************/
/*                              kdd_variable                                 */
/*****************************************************************************/

struct kdd_variable {
  public: // Member functions
    kdd_variable()
      { name=html_link=html_offset=initializer=NULL;
        next=NULL; owner=NULL; line_number=0; file=NULL;
        property_set = property_get = NULL;
        jni_bindable = mni_bindable = false;
      }
    ~kdd_variable()
      {
        if (name != NULL) delete[] name;
        if (html_link != NULL) delete[] html_link;
        if (initializer != NULL) delete[] initializer;
      }
    void create_html_link(const char *html_file);
  public: // Data
    char *name; // Variable name.
    char *html_link; // Holds both HTML file and location in that file.
    char *html_offset; // Points to the internal location part of `html_link'.
    kdd_file *file; // File in which declaration is found.
    int line_number; // Location in file at which declaration begins
    kdd_string description; // Describes a function arg or class member
    kdd_class *owner; // NULL for function args.
    kdd_type type; // Basic type (modifiers in following two members).
    char *initializer; // Default/initial value of arg/variable, or NULL.
    kdd_variable *next; // For building linked lists.
    kdd_func *property_set; // See below
    kdd_func *property_get; // See below
    bool jni_bindable, mni_bindable;
  };
  /* Notes:
       If `property_set' is non-NULL, this is a public member variable of
     a class/struct which contains an appropriate `set_' function (the one
     referenced by `property_set'.  That function points back to the present
     object through its own `kdd_func::property' member.
       If `property_get' is non-NULL, this is a public member variable of
     a class/struct which contains an appropriate `get_' function (the one
     referenced by `property_get').  That function points back to the
     present object through its own `kdd_func::property' member.
       The `jni_bindable' member is true if and only if this is one
     of the first `kdd_func::num_jni_args' arguments of an owning
     function.  Similarly, `mni_bindable' is true if and only if this
     is one of the first `kdd_func::num_mni_args' arguments of an
     owning function. */

/*****************************************************************************/
/*                                kdd_func                                   */
/*****************************************************************************/

struct kdd_func {
  public: // Member functions
    kdd_func()
      { name=html_name=NULL; line_number=overload_index=0; owner=NULL;
        args=NULL; next=prev=overload_next=overrides=NULL;
        is_virtual=is_static=is_pure=has_descriptions=false;
        is_constructor=is_destructor=false; property=NULL; file=NULL;
        java_name = mni_name = NULL;  csharp_name = vbasic_name = NULL;
        binding = KDD_BIND_TRANSIENT; bind_specific=false;
        num_jni_args = num_mni_args = 0;
      }
    ~kdd_func()
      { kdd_variable *vtmp;
        while ((vtmp=args) != NULL)
          { args=vtmp->next; delete vtmp; }
        if (name != NULL) delete[] name;
        if (java_name != NULL) delete[] java_name;
        if (mni_name != NULL) delete[] mni_name;
        if (html_name != NULL) delete[] html_name;
      }
    bool operator==(kdd_func &ref);
      /* Returns true if the two function definitions agree in their
         names and argument types. */
    bool operator!=(kdd_func &ref)
      { return !(*this == ref); }
    void create_html_names(kdd_func *compaction_list);
      /* Determines the name, if any, of the HTML file for this object, as
         well as the HTML links of all of the function's arguments.
         If `compaction_list' is non-NULL, short HTML names are
         generated, having length no larger than 32 characters.  To guarantee
         uniqueness, the function walks through all previously assigned
         function names to check that there is no conflict.  The
         `compaction_list' argument points to the first function for which a
         name may have been assigned; the function should walk through the
         list from this class until it gets to itself. */
    void generate_html(kdd_class *classes, const char *directory);
      /* Generates an HTML file describing this function. */
  public: // Data
    bool has_descriptions; // True if function or any arg has a description
    char *name; // Function name
    char *html_name; // Name of HTML file which describes this function if any.
    char *java_name; // NULL if function does not exist in Java bindings
    char *mni_name; // NULL if function does not exist in managed bindings
    const char *csharp_name; // Non-NULL if and only if `mni_name' non-NULL
    const char *vbasic_name; // Non-NULL if and only if `mni_name' non-NULL
    kdd_file *file; // File in which declaration is found.
    bool is_virtual, is_static, is_pure;
    bool is_constructor, is_destructor;
    kdd_variable *property; // See below
    int line_number; // Line in file at which declaration begins.
    kdd_string description; // Describes the function.
    kdd_class *owner; // Global definitions reference object with NULL name
    kdd_func *overrides; // Function in nearest base class overridden here
    kdd_type return_type;
    kdd_string return_description;
    kdd_variable *args; // Argument list.
    kdd_func *next; // For building linked lists
    kdd_func *prev; // Points to previous member function of class, if any.
    kdd_func *overload_next; // Used to build a ring of overloaded funcs
    int overload_index; // 0 for first function in overload ring.
    int binding; // KDD_BIND_DONATE if the function donates its object to
                 //   another object; Java/MNI bindings may not destroy donated
                 //   objects once all references to them have been lost.
                 // KDD_BIND_TRANSIENT for regular functions.
                 // KDD_BIND_CALLBACK for functions which are to be implemented
                 //   in the foreign language binding.
    bool bind_specific; // See below.
    int num_jni_args; // See below.
    int num_mni_args; // See below.
  };
  /* Notes:
        If `property' is non-NULL, this function has been identified as a
     "get" or "set" interface function whose purpose is to provide access
     to a public member variable of the underlying C++ class or structure
     and `property' points to the relevant member variable.  This information
     is used to build interfaces to managed languages, including C# and
     Visual Basic, allowing application developers to treat the relevant
     member variable in the most natural way.
        The `bind_specific' flag is used only in conjunction with the "-bind"
     argument, although it is always set to false during construction.
     The flag is set to true only for those functions or classes which are
     specified by "-bind" arguments, either directly or implicitly.  If any
     "-bind" arguments were processed, a final pass through the set of all
     classes and functions sets the `binding' member to KDD_BIND_NONE
     if `bind_specific' is false.
        The `num_jni_args' and `num_mni_args' members are filled out during
     the initial search for bindable functions, when building Java native
     interfaces and Microsoft managed native interfaces, respectively.
     These members identify the number of initial arguments which can be
     represented in Java or one of the Microsoft managed languages,
     respectively, where any subsequent arguments must have defaults. */

/*****************************************************************************/
/*                               kdd_class                                   */
/*****************************************************************************/

struct kdd_class {
  public: // Member functions
    kdd_class()
      {
        name=html_name=java_name=mni_name=NULL; line_number = 0; file = NULL;
        next = NULL; functions = NULL; variables = NULL; constants = NULL;
        has_descriptions = is_union = is_struct = is_abstract = false;
        destroy_private = destroy_virtual = has_callbacks = false;
        binding = KDD_BIND_NONE;  bind_specific = false;
      }
    ~kdd_class()
      { kdd_func *ftmp; kdd_variable *vtmp; kdd_constant *ctmp;
        while ((ctmp=constants) != NULL)
          { constants=ctmp->next; delete ctmp; }
        while ((ftmp=functions) != NULL)
          { functions=ftmp->next; delete ftmp; }
        while ((vtmp=variables) != NULL)
          { variables=vtmp->next; delete vtmp; }
        if (name != NULL) delete[] name;
        if (html_name != NULL) delete[] html_name;
        if (java_name != NULL) delete[] java_name;
        if (mni_name != NULL) delete[] mni_name;
      }
    void create_html_names(kdd_class *compaction_list);
      /* Determines the name, if any, of the HTML file for this object, as
         well as the HTML names and links for all member functions and
         variables.  If `compaction_list' is non-NULL, short HTML names are
         generated, having length no larger than 32 characters.  To guarantee
         uniqueness, the function walks through all previously assigned
         class names to check that there is no conflict.  The `compaction_list'
         argument points to the first class for which a name may have been
         assigned; the function should walk through the list from this class
         until it gets to itself. */
    void generate_html(kdd_class *classes, const char *directory);
      /* Generates a single HTML page for the class (if it has descriptions)
         and a single HTML page for each public member function and member
         variable. */
    void find_properties();
      /* This function is called once all member variables and functions
         have been parsed, to determine whether or not any of the public
         member variables are properties, in the sense that they have
         corresponding "get_" and/or "set_" functions.  If so, the
         relevant `kdd_variable::property_get', `kdd_variable::property_set'
         and `kdd_func::property' members are set, as appropriate. */
  public: // Data
    bool has_descriptions; // True if class or any member has a description
    bool is_union;
    bool is_struct;
    bool is_abstract; // True if function has one or more pure virtual funcs
    bool destroy_private; // True if destructor is declared private
    bool destroy_virtual; // True if destructor is virtual
    bool has_callbacks; // True if one or more functions has KDD_BIND_CALLBACK
    char *name; // NULL for global definitions.
    char *html_name; // Name of HTML file which describes this class, if any.
    char *java_name; // Name of class as it appears in Java, if at all.
    char *mni_name; // Name of class, as it appears in Managed code, if at all.
    kdd_file *file; // File containing declaration.
    int line_number; // Location in file at which definition begins.
    kdd_string description; // Describes the class; can be empty.
    kdd_type base; // Name and/or link to base class, if any.
    kdd_constant *constants; // List of constants local to this class
    kdd_func *functions; // List of public function members
    kdd_variable *variables; // List of public data members
    int binding; // KDD_BIND_NONE, KDD_BIND_IFC, KDD_BIND_COPY or KDD_BIND_REF
    bool bind_specific; // See below
    kdd_class *next;
  };
  /* Notes:
        The `bind_specific' flag is used only in conjunction with the "-bind"
     argument, although it is always set to false during construction.
     The flag is set to true only for those functions or classes which are
     specified by "-bind" arguments, either directly or implicitly.  If any
     "-bind" arguments were processed, a final pass through the set of all
     classes and functions sets the `binding' member to KDD_BIND_NONE
     if `bind_specific' is false. */

/*****************************************************************************/
/*                               kdd_file                                    */
/*****************************************************************************/

struct kdd_file {
  public: // Member functions
    kdd_file() { pathname=name=NULL; has_bindings=false; next=NULL; }
    kdd_file(const char *full_path_name);
      /* Copies `full_path_name' to `pathname', sets `name' to point into
         the file name portion of the full path name, and creates the
         HTML file name. */
    void shorten_html_name(kdd_file *list);
      /* Called if HTML names are to be strictly limited to no more than
         32 characters.  In this case, the file name is truncated in an
         appropriate manner.  The `list' argument refers to the head of
         the file list to which the current object belongs.  It is parsed
         to check for uniqueness. */
    ~kdd_file()
      {
        if (pathname != NULL) delete[] pathname;
        if (html_name != NULL) delete[] html_name;
      }
    void generate_html(kdd_class *classes, const char *directory);
      /* Generates a single HTML page describing the classes which belong to
         this file, placing the HTML page in the indicated directory. */
    void copy_file(const char *directory);
      /* Writes an exact copy of the file to the supplied directory. */
  public: // Data
    char *pathname; // Relative path of source header file.
    char *name; // Points into `pathname' to the actual file name.
    char *html_name; // Name of HTML file containing a list of its contents.
    bool has_bindings; // If it has one or more classes or global functions,
                       // with language bindings
    kdd_file *next;
  };

/*****************************************************************************/
/*                               kdd_index                                   */
/*****************************************************************************/

struct kdd_index {
  public: // Member functions
    kdd_index(){ name=link=NULL; next=NULL; }
    ~kdd_index()
      { if (name != NULL) delete[] name;
        if (link != NULL) delete[] link; }
  public: // Data
    char *name;
    char *link;
    kdd_index *next;
  };

/*****************************************************************************/
/*                            External Functions                             */
/*****************************************************************************/

extern void
  write_relative_path(const char *target_dir, const char *ref_dir,
                      const char *target_file, ostream &out);
  /* This function is used to generate relative include file specifications
     so as to avoid the need for complex search paths in the build
     environment.  The function writes the relative pathname formed by
     concatenating `target_dir' with `target_file', separated by the usual
     "/" delimiter, and expressed relative to `ref_dir'.  If the
     relationship between `target_dir' and `ref_dir' does not allow a
     relative path to be formed, `target_file' is written by itself. */

extern bool create_file(char *path, ofstream &out);
  /* This function tries in the first instance just to open the text file
     at the indicated path for writing.  If it fails, the function the
     tries to create the required directories.  This eliminates problems
     encountered with earlier versions of the hyperdoc utility where
     the tool was automatically run from a script which expected certain
     directories to exist on the user's system. */

#endif // HYPERDOC_LOCAL_H
