// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:       types.h
//
// Purpose:    common types for Hyper-G
//
// Created:     1 Sep 92    Gerald Pani
//
// Modified:    3 Aug 93    Gerald Pani
//
//
//
// Description:
//
//</file>

#ifndef hg_utils_types
#define hg_utils_types

#include "../InterViews/boolean.h"

#include "../Dispatch/leave-scope.h"

#ifdef nil
#undef nil
#endif

#define nil 0

#ifdef __osf__
#  define HG_UL32 unsigned int
#else
#  define HG_UL32 unsigned long
#endif

// internet address
#define HG_inet_addr HG_UL32

#endif
