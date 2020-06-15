/*****************************************************************************/
// File: kdu_arch.cpp [scope = CORESYS/COMMON]
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
  Defines and evaluates the architecture-specific variables defined in
"kdu_arch.h".
******************************************************************************/

#include "kdu_arch.h"

/* ========================================================================= */
/*                           SIMD Support Testing                            */
/* ========================================================================= */

#if ((defined KDU_PENTIUM_MSVC) || (defined KDU_PENTIUM_GCC) || \
     (defined KDU_X86_INTRINSICS))
#  define __KDU_X86__
#endif

//--------------------------- KDU_NO_CPUID_TEST -------------------------------
#if ((defined KDU_NO_CPUID_TEST) && (defined __KDU_X86__))
		// Define KDU_NO_CPUID_TEST if the CPUID instruction has been disabled
		// on your platform -- this is done by default on some Linux platforms.
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = false;
# ifdef KDU_POINTERS64
bool kdu_pentium_cmov_exists = true; // Assume 64-bit X86 CPU's have CMOV
int kdu_mmx_level = 2; // Assume 64-bit X86 CPU's have at least SSE2
# else // !defined KDU_POINTERS64
int kdu_mmx_level = 1; // Assume process has at least MMX support
bool kdu_pentium_cmov_exists = false;
# endif // !defined KDU_POINTERS64

//---------------------- Microsoft System, 64-bit X86 -------------------------
#elif (defined __KDU_X86__) && (defined _WIN64)
extern "C" int x64_get_mmx_level();     // These functions are implemented
extern "C" bool x64_get_cmov_exists();  // in "arch_masm64.asm".

int kdu_mmx_level = x64_get_mmx_level();
bool kdu_pentium_cmov_exists = x64_get_cmov_exists();
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = false;

//---------------------- Microsoft System, 32-bit X86 -------------------------
#elif (defined __KDU_X86__) && (defined _WIN32)
static int get_mmx_level();
static bool get_cmov_exists();

int kdu_mmx_level = get_mmx_level();
bool kdu_pentium_cmov_exists = get_cmov_exists();
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = false;

static bool get_cmov_exists()
{
	int edx_val = 0;
	__try {
		__asm {
			MOV eax, 1
			CPUID
			MOV edx_val, EDX
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	return ((edx_val & 0x00008000) != 0);
}

static int get_mmx_level()
{
	int edx_val = 0, ecx_val = 0;
	__try {
		__asm {
			MOV eax, 1
			CPUID
			MOV edx_val, edx
			MOV ecx_val, ecx
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
	int result = 0;
	if (edx_val & 0x00800000)
	{
		result = 1; // Basic MMX features exist
#ifndef KDU_NO_SSE
		if ((edx_val & 0x06000000) == 0x06000000)
		{
			result = 2; // SSE and SSE2 features exist
			__try { // Just to be quite certainm, try an SSE2 instruction
				__asm xorpd xmm0, xmm0
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { result = 1; }
		}
#endif // !KDU_NO_SSE
	}
	if ((result == 2) && (ecx_val & 1))
	{
		result = 3; // SSE3 support exists
		if (ecx_val & 0x00000200)
			result = 4; // SSSE3 support exists
	}
	return result;
}

//---------------------- GCC Build, X86 64-bit Platform -----------------------
#elif (defined __KDU_X86__) && (defined KDU_POINTERS64)
static int get_mmx_level();
static bool get_cmov_exists();

int kdu_mmx_level = get_mmx_level();
bool kdu_pentium_cmov_exists = get_cmov_exists();
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = false;

static bool get_cmov_exists()
{
	int edx_val = 0;
	__asm__ volatile ("mov %%rbx,%%rsi\n\t" // Save PIC register (RBX) in RSI
		"mov $1,%%rax\n\t"
		"cpuid\n\t"
		"mov %%edx,%0\n\t"
		"mov %%rsi,%%rbx" // Restore the PIC register (RBX)
		: "=m" (edx_val) : /* no input */
		: "%rax", "%rsi", "%rcx", "%rdx");
	return ((edx_val & 0x00008000) != 0);
}

static int get_mmx_level()
{
	int edx_val = 0, ecx_val = 0;
	__asm__ volatile ("mov %%rbx,%%rsi\n\t" // Save PIC register (RBX) in RSI
		"mov $1,%%rax\n\t"
		"cpuid\n\t"
		"mov %%edx,%0\n\t"
		"mov %%ecx,%1\n\t"
		"mov %%rsi,%%rbx" // Restore the PIC register (RBX)
		: "=m" (edx_val), "=m" (ecx_val) : /* no input */
		: "%rax", "%rsi", "%rcx", "%rdx");
	int result = 0;
	if (edx_val & 0x00800000)
	{
		result = 1; // Basic MMX features exist
		if ((edx_val & 0x06000000) == 0x06000000)
			result = 2;
	}
	if ((result == 2) && (ecx_val & 1))
	{
		result = 3; // SSE3 support exists
		if (ecx_val & 0x00000200)
			result = 4; // SSSE3 support exists
	}
	return result;
}

//----------------------- GCC build, 32-bit X86 -------------------------------
#elif (defined __KDU_X86__)
static bool check_cpuid_exists();
static int get_mmx_level();
static bool get_cmov_exists();

int kdu_mmx_level = get_mmx_level();
bool kdu_pentium_cmov_exists = get_cmov_exists();
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = false;

static bool check_cpuid_exists()
{
	kdu_uint16 orig_flags16, mod_flags16;
	__asm__ volatile ("pushf\n\t"
		"pop %%ax\n\t" // Get flags into AX
		"mov %%ax,%%cx\n\t" // Save copy of flags in CX
		"mov %%cx,%0\n\t"
		"xor $0xF000,%%ax\n\t" // Flip bits 12-15
		"push %%ax\n\t" // Put modified flags back on stack
		"popf\n\t" // Replace flags
		"pushf\n\t" // Push flags back on the stack
		"pop %%ax\n\t" // Get flags back into AX
		"mov %%ax,%1\n\t"
		"push %%cx\n\t" // Put original flags back on stack
		"popf"
		: "=m" (orig_flags16), "=m" (mod_flags16)
		: /* no input */
		: "%ax", "%cx");
	if (orig_flags16 == mod_flags16)
		return false; // Failed to toggle bits 12-15 of flags register.
					  // Must be 8086/88, 80186 or 80286.

	kdu_uint32 orig_flags32, mod_flags32;
	__asm__ volatile ("pushf\n\t" // Push eflags onto stack
		"pop %%eax\n\t" // Get eflags into EAX
		"mov %%eax,%%ecx\n\t" // Save copy of eflags in ECX
		"mov %%ecx,%0\n\t"
		"xor $0x40000,%%eax\n\t" // Flip AC bit
		"push %%eax\n\t" // Put modified eflags back on stack
		"popf\n\t" // Replace eflags
		"pushf\n\t" // Push eflags back on the stack
		"pop %%eax\n\t" // Get eflags back into EAX
		"mov %%eax,%1\n\t"
		"push %%ecx\n\t" // Put original eflags back on stack
		"popf" // Put back original eflags
		: "=m" (orig_flags32), "=m" (mod_flags32)
		: /* no input */
		: "%eax", "%ecx");
	if (orig_flags32 == mod_flags32)
		return false; // Failed to toggle AC bit of eflags register
					  // Must be 80386.

	__asm__ volatile ("pushf\n\t" // Push eflags onto stack
		"pop %%eax\n\t" // Get eflags into EAX
		"mov %%eax,%%ecx\n\t" // Save copy of eflags in ECX
		"mov %%ecx,%0\n\t"
		"xor $0x200000,%%eax\n\t" // Flip ID bit
		"push %%eax\n\t" // Put modified eflags back on stack
		"popf\n\t" // Replace eflags
		"pushf\n\t" // Push eflags back on the stack
		"pop %%eax\n\t" // Get eflags back into EAX
		"mov %%eax,%1\n\t"
		"push %%ecx\n\t" // Put original eflags back on stack
		"popf" // Put back original eflags
		: "=m" (orig_flags32), "=m" (mod_flags32)
		: /* no input */
		: "%eax", "%ecx");
	if (orig_flags32 == mod_flags32)
		return false; // Unable to toggle ID bit of eflags register
					  // Must be 80486
	kdu_uint32 max_eax;
	__asm__ volatile ("mov %%ebx,%%esi\n\t" // Save PIC register (EBX) in ESI
		"xor %%eax,%%eax\n\t" // Zero EAX
		"cpuid\n\t"
		"mov %%eax,%0\n\t"
		"mov %%esi,%%ebx" // Restore the PIC register (EBX)
		: "=m" (max_eax) : /* no input */
		: "%eax", "%ecx", "%edx", "%esi");
	if (max_eax < 1)
		return false; // We won't be able to invoke CPUID with EAX=1 to test
					  // for MMX/SSE/SSE2 support.
	return true;
}

static bool get_cmov_exists()
{
	if (!check_cpuid_exists()) return false;
	int edx_val = 0;
	__asm__ volatile ("mov %%ebx,%%esi\n\t" // Save PIC register (EBX) in ESI
		"mov $1,%%eax\n\t"
		"cpuid\n\t"
		"mov %%edx,%0\n\t"
		"mov %%esi,%%ebx" // Restore the PIC register (EBX)
		: "=m" (edx_val) : /* no input */
		: "%eax", "%ecx", "%edx", "%esi");
	return ((edx_val & 0x00008000) != 0);
}

static int get_mmx_level()
{
	if (!check_cpuid_exists()) return 0;
	int edx_val = 0, ecx_val = 0;
	__asm__ volatile ("mov %%ebx,%%esi\n\t" // Save PIC register (EBX) in ESI
		"mov $1,%%eax\n\t"
		"cpuid\n\t"
		"mov %%edx,%0\n\t"
		"mov %%ecx,%1\n\t"
		"mov %%esi,%%ebx" // Restore the PIC register (EBX)
		: "=m" (edx_val), "=m" (ecx_val) : /* no input */
		: "%eax", "%ecx", "%edx", "%esi");
	int result = 0;
	if (edx_val & 0x00800000)
	{
		result = 1; // Basic MMX features exist
		if ((edx_val & 0x06000000) == 0x06000000)
			result = 2;
	}
	if ((result == 2) && (ecx_val & 1))
	{
		result = 3; // SSE3 support exists
		if (ecx_val & 0x00000200)
			result = 4; // SSSE3 support exists
	}
	return result;
}

//--------------------------- KDU_SPARCVIS_GCC --------------------------------
#elif defined KDU_SPARCVIS_GCC
static bool get_sparcvis_exists();
bool kdu_pentium_cmov_exists = false;
int kdu_mmx_level = 0;
bool kdu_sparcvis_exists = get_sparcvis_exists();
bool kdu_altivec_exists = false;

static bool get_sparcvis_exists()
{
	typedef double kdvis_d64; // Use for 64-bit VIS registers
	union kdvis_4x16 {
		kdvis_d64 d64;
		kdu_int16 v[4];
	};
	try {
		kdvis_4x16 val; val.v[0] = 1;
		register kdvis_d64 reg = val.d64;
		__asm__ volatile ("fpadd16 %1,%2,%0" :"=e"(reg) : "e"(reg), "e"(reg));
		val.d64 = reg;
		return (val.v[0] == 2);
	}
	catch (...)
	{ // Instruction not supported
		return false;
	}
}

//---------------------------- KDU_ALTIVEC_GCC --------------------------------
#elif defined KDU_ALTIVEC_GCC
# include <sys/types.h>
# include <sys/sysctl.h>
static bool get_altivec_exists();
bool kdu_pentium_cmov_exists = false;
int kdu_mmx_level = 0;
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = get_altivec_exists();

static bool get_altivec_exists()
{
	try {
		int vectorunit = 0;
		int mib[2];
		size_t len;
		mib[0] = CTL_HW;
		mib[1] = HW_VECTORUNIT;
		len = sizeof(vectorunit);
		sysctl(mib, 2, &vectorunit, &len, NULL, 0);
		return (vectorunit != 0);
	}
	catch (...)
	{ // Just in case something went wrong in the test
		return false;
	}
}

//-------------------------- Unknown Architecture -----------------------------
#else
bool kdu_pentium_cmov_exists = false;
int kdu_mmx_level = 0;
bool kdu_sparcvis_exists = false;
bool kdu_altivec_exists = false;
#endif // Architecture tests


/* ========================================================================= */
/*                          Multi-Processor Support                          */
/* ========================================================================= */

#ifdef __APPLE__
# include <sys/param.h>
# include <sys/sysctl.h>
#endif // __APPLE__

/*****************************************************************************/
/* EXTERN                    kdu_get_num_processors                          */
/*****************************************************************************/

int
kdu_get_num_processors()
{
#if (defined _WIN64)
	ULONG_PTR proc_mask = 0, sys_mask = 0;
	if (!GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask))
		return 1;
	int b, result = 0;
	for (b = 0; b < 64; b++, proc_mask >>= 1)
		result += (int)(proc_mask & 1);
	return (result > 1) ? result : 1;
#elif (defined _WIN32) || (defined WIN32)
	DWORD proc_mask = 0, sys_mask = 0;
	if (!GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask))
		return 1;
	int b, result = 0;
	for (b = 0; b < 32; b++, proc_mask >>= 1)
		result += (int)(proc_mask & 1);
	return (result > 1) ? result : 1;
#elif defined _SC_NPROCESSORS_ONLN
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined _SC_NPROCESSORS_CONF
	return (int)sysconf(_SC_NPROCESSORS_CONF);
#elif (defined __APPLE__)
	int result = 0;
	size_t result_size = sizeof(result);
	if (sysctlbyname("hw.ncpu", &result, &result_size, NULL, 0) != 0)
		result = 0;
	return result;
#else
	return 0;
#endif
}
