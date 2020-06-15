/*****************************************************************************/
// File: params_local.h [scope = CORESYS/PARAMETERS]
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
   Local definitions for use by "params.cpp".  These should not be included
from any other scope.
******************************************************************************/

#ifndef PARAMS_LOCAL_H
#define PARAMS_LOCAL_H

// Defined here:

struct att_val;
struct kd_attribute;

/*****************************************************************************/
/*                                    att_val                                */
/*****************************************************************************/

struct att_val {
	/* Stores a single attribute value.  If `pattern' points to a string
	   whose first character is 'F', the value is a floating point quantity.
	   Otherwise, the value is an integer. */
	att_val()
	{
		is_set = false; pattern = NULL;
	}
	union {
		int ival;
		float fval;
	};
	char const *pattern;
	bool is_set;
};

/*****************************************************************************/
/*                                  kd_attribute                             */
/*****************************************************************************/

struct kd_attribute {
	/* Objects of this class are used to build a linked list of attributes,
	   which are managed by the kdu_params class. An attribute may contain
	   one or more parameter records, each of which may contain one or more
	   fields. Each field may have a different data type and  interpretation. */
public: // Member functions
	kd_attribute(const char *name, const char *comment,
		int flags, const char *pattern);
	/* See the definition of `kdu_params::define_attribute'. */
	~kd_attribute()
	{
		delete[](values);
	}
	void augment_records(int new_records);
	void describe(kdu_message &output, bool allow_tiles, bool allow_comps,
		bool treat_instances_like_components, bool include_comments);
public: // Data
	const char *name; // See constructor.
	const char *comment; // See constructor.
	int flags; // See constructor.
	const char *pattern; // See constructor.
	int num_fields; // Number of fields in each record (i.e., in `pattern').
	int num_records; // Number of records which have actually been written.
	att_val *values; // Array of `max_records'*`num_fields' values.
	bool derived; // Set using the `kdu_params::set_derived' function.
	bool parsed; // Set if the information was obtained by string parsing.
	kd_attribute *next; // Used to build linked list within kdu_params class
private:
	int max_records; // Maximum storage available in `values' array.
};

#endif // PARAMS_LOCAL_H
