/*****************************************************************************/
// File: kdu_elementary.h [scope = CORESYS/COMMON]
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
   Elementary data types and extensively used constants.  It is possible that
the definitions here might need to be changed for some architectures.
******************************************************************************/

#ifndef KDU_ELEMENTARY_H
#define KDU_ELEMENTARY_H
#include <new>
#include <limits.h>

#define KDU_EXPORT      // This one is used for core system exports

#define KDU_AUX_EXPORT  // This one is used by managed/kdu_aux to build
// a DLL containing exported auxiliary classes for
// linking to managed code (e.g., C# or Visual Basic)

#if (defined WIN32) || (defined _WIN32) || (defined _WIN64)
#  define KDU_WINDOWS_OS
#  undef KDU_EXPORT
#  if defined CORESYS_EXPORTS
#    define KDU_EXPORT __declspec(dllexport)
#  elif defined CORESYS_IMPORTS
#    define KDU_EXPORT __declspec(dllimport)
#  else
#    define KDU_EXPORT
#  endif // CORESYS_EXPORTS

#  undef KDU_AUX_EXPORT
#  if defined KDU_AUX_EXPORTS
#    define KDU_AUX_EXPORT __declspec(dllexport)
#  elif defined KDU_AUX_IMPORTS
#    define KDU_AUX_EXPORT __declspec(dllimport)
#  else
#    define KDU_AUX_EXPORT
#  endif // KDU_AUX_EXPORTS

#endif // _WIN32 || _WIN64

/*****************************************************************************/
/*                                 8-bit Scalars                             */
/*****************************************************************************/

typedef unsigned char kdu_byte;

/*****************************************************************************/
/*                                 16-bit Scalars                            */
/*****************************************************************************/

typedef short int kdu_int16;
typedef unsigned short int kdu_uint16;

#define KDU_INT16_MAX ((kdu_int16) 0x7FFF)
#define KDU_INT16_MIN ((kdu_int16) 0x8000)

/*****************************************************************************/
/*                                 32-bit Scalars                            */
/*****************************************************************************/

#if (INT_MAX == 2147483647)
typedef int kdu_int32;
typedef unsigned int kdu_uint32;
#else
# error "Platform does not appear to support 32 bit integers!"
#endif

#define KDU_INT32_MAX ((kdu_int32) 0x7FFFFFFF)
#define KDU_INT32_MIN ((kdu_int32) 0x80000000)

/*****************************************************************************/
/*                                 64-bit Scalars                            */
/*****************************************************************************/

#ifdef KDU_WINDOWS_OS
typedef __int64 kdu_int64;
# define KDU_INT64_MIN 0x8000000000000000
# define KDU_INT64_MAX 0x7FFFFFFFFFFFFFFF
#elif (defined __GNUC__) || (defined __sparc) || (defined __APPLE__)
typedef long long kdu_int64;
# define KDU_INT64_MIN 0x8000000000000000
# define KDU_INT64_MAX 0x7FFFFFFFFFFFFFFF
#endif // Definitions for `kdu_int64'

/*****************************************************************************/
/*                           Best Effort Scalars                             */
/*****************************************************************************/

#if (defined _WIN64)
#        define KDU_POINTERS64
typedef __int64 kdu_long;
#        define KDU_LONG_MAX 0x7FFFFFFFFFFFFFFF
#        define KDU_LONG_HUGE (((kdu_long) 1) << 52)
#        define KDU_LONG64 // Defined whenever kdu_long is 64 bits wide
#elif (defined WIN32) || (defined _WIN32)
#    define WIN32_64 // Comment this out if you do not want support for very
					 // large images and compressed file sizes.  Support for
					 // these may slow down execution a little bit and add to
					 // some memory costs.
#    ifdef WIN32_64
typedef __int64 kdu_long;
#        define KDU_LONG_MAX 0x7FFFFFFFFFFFFFFF
#        define KDU_LONG_HUGE (((kdu_long) 1) << 52)
#        define KDU_LONG64 // Defined whenever kdu_long is 64 bits wide
#    endif
#elif (defined __GNUC__) || (defined __sparc) || (defined __APPLE__)
#    define UNX_64 // Comment this out if you do not want support for very
				   // large images.
#    if (defined _LP64)
#      define KDU_POINTERS64
#      ifndef UNX_64
#        define UNX_64 // Need 64-bit kdu_long with 64-bit pointers
#      endif
#    endif

#    ifdef UNX_64
typedef long long int kdu_long;
#        define KDU_LONG_MAX 0x7FFFFFFFFFFFFFFFLL
#        define KDU_LONG_HUGE (1LL << 52)
#        define KDU_LONG64 // Defined whenever kdu_long is 64 bits wide
#    endif
#endif

#ifndef KDU_LONG64
typedef long int kdu_long;
#   define KDU_LONG_MAX LONG_MAX
#   define KDU_LONG_HUGE LONG_MAX
#endif

/*****************************************************************************/
/*                        Other Architecture Constants                       */
/*****************************************************************************/

#ifdef KDU_POINTERS64
#  define KDU_POINTER_BYTES 8 // Length of an address, in bytes
#else
#  define KDU_POINTER_BYTES 4 // Length of an address, in bytes
#endif // !KDU_POINTERS64

/*****************************************************************************/
/*                    Pointer to/from Integer Conversions                    */
/*****************************************************************************/
#if (defined _MSC_VER && (_MSC_VER >= 1300))
#  define _kdu_long_to_addr(_val) ((void *)((INT_PTR)(_val)))
#  define _addr_to_kdu_long(_addr) ((kdu_long)((INT_PTR)(_addr)))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)(PtrToLong(_addr)))
#elif defined KDU_POINTERS64
#  define _kdu_long_to_addr(_val) ((void *)(_val))
#  define _addr_to_kdu_long(_addr) ((kdu_long)(_addr))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)((kdu_long)(_addr)))
#else // !KDU_POINTERS64
#  define _kdu_long_to_addr(_val) ((void *)((kdu_uint32)(_val)))
#  define _addr_to_kdu_long(_addr) ((kdu_long)((kdu_uint32)(_addr)))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)(_addr))
#endif // !KDU_POINTERS64

/*****************************************************************************/
/*                              Subband Identifiers                          */
/*****************************************************************************/

#define LL_BAND ((int) 0) // DC subband
#define HL_BAND ((int) 1) // Horizontally high-pass subband
#define LH_BAND ((int) 2) // Vertically high-pass subband
#define HH_BAND ((int) 3) // High-pass subband


/* ========================================================================= */
/*                        Multi-Threading Primitives                         */
/* ========================================================================= */

#ifdef KDU_WINDOWS_OS
#  include <windows.h>
#  ifndef KDU_NO_THREADS // Disable multi-threading by defining KDU_NO_THREADS
#    define KDU_THREADS
#    define KDU_WIN_THREADS
#  endif // !KDU_NO_THREADS
#elif (defined __GNUC__) || (defined __sparc) || (defined __APPLE__)
#  include <unistd.h>
#  ifndef KDU_NO_THREADS // Disable multi-threading by defining KDU_NO_THREADS
#    define KDU_THREADS
#    define KDU_PTHREADS
#    ifdef HAVE_CLOCK_GETTIME
#      include <time.h>
#    else // Assume we HAVE_GETTIMEOFDAY
#      include <sys/time.h>
#    endif // HAVE_CLODK_GETTIME
struct kdu_timespec : public timespec {
	bool get_time()
	{
#        ifdef HAVE_CLOCK_GETTIME
		return (clock_gettime(this) == 0);
#        else // Assume we HAVE_GETTIMEOFDAY
		struct timeval val;
		if (gettimeofday(&val, NULL) != 0) return false;
		tv_sec = val.tv_sec; tv_nsec = val.tv_usec * 1000; return true;
#        endif // HAVE_CLOCK_GETIME
	}
};
#    include <sys/time.h>
#    include <pthread.h>
#  endif // !KDU_NO_THREADS
#endif

/*****************************************************************************/
/*                           kdu_thread_startproc                            */
/*****************************************************************************/

#ifdef KDU_WIN_THREADS
typedef DWORD kdu_thread_startproc_result;
#   define KDU_THREAD_STARTPROC_CALL_CONVENTION WINAPI
#   define KDU_THREAD_STARTPROC_ZERO_RESULT ((DWORD) 0)
#else
typedef void * kdu_thread_startproc_result;
#   define KDU_THREAD_STARTPROC_CALL_CONVENTION
#   define KDU_THREAD_STARTPROC_ZERO_RESULT NULL
#endif

typedef kdu_thread_startproc_result
(KDU_THREAD_STARTPROC_CALL_CONVENTION *kdu_thread_startproc)(void *);

/*****************************************************************************/
/*                                EXCEPTIONS                                 */
/*****************************************************************************/

typedef int kdu_exception;
/* [SYNOPSIS]
	 Best practice is to include this type in catch clauses, in case we
	 feel compelled to migrate to a non-integer exception type at some
	 point in the future.
*/

#define KDU_NULL_EXCEPTION ((int) 0)
/* [SYNOPSIS]
	 You can use this value in your applications, for a default
	 initializer for exception codes that you do not intend to throw.
	 There is no inherent reason why a part of an application cannot
	 throw this exception code, but it is probably best not to do so.
*/
#define KDU_ERROR_EXCEPTION ((int) 0x6b647545)
/* [SYNOPSIS]
	 Best practice is to throw this exception within a derived error
	 handler, whenver the end-of-message flush call is received -- see
	 `kdu_error' and `kdu_customize_errors' and `kdu_message'.
	 [//]
	 Conversely, in a catch statement, you may compare the value of a
	 caught exception with this value to determine whether or not the
	 exception was generated upon handling an error message dispatched
	 via `kdu_error'.
*/
#define KDU_MEMORY_EXCEPTION ((int) 0x6b64754d)
/* [SYNOPSIS]
	 You should avoid using this exception code when throwing your own
	 exceptions from anywhere.  This value is used primarily by the
	 system to record the occurrence of a `std::bad_alloc' exception
	 and pass it across programming interfaces which can only accept
	 a single exception type -- e.g., when passing exceptions between
	 threads in a multi-threaded processing environment.  When rethrowing
	 exceptions across such interfaces, this value is checked and used
	 to rethrow a `std::bad_alloc' exception.
	 [//]
	 To facilitate the rethrowing of exceptions with this value as
	 `std::bad_alloc', Kakadu provides the `kdu_rethrow' function,
	 which leaves open the possibility that other types of exceptions
	 may be passed across programming interfaces using special
	 values in the future.
*/
#define KDU_CONVERTED_EXCEPTION ((int) 0x6b647543)
/* [SYNOPSIS]
	 Best practice is to throw this exception code if you need to
	 convert an exception of a different type (e.g., exceptions caught
	 by a catch-all "catch (...)" statement) into an exception of type
	 `kdu_exception' (e.g., so as to pass it to
	 `kdu_thread_entity::handle_exception', or when passing an exception
	 across language boundaries).
*/

static inline void kdu_rethrow(kdu_exception exc)
{
	if (exc == KDU_MEMORY_EXCEPTION) throw std::bad_alloc(); else throw exc;
}
/* [SYNOPSIS]
	 You should ideally use this function whenever you need to rethrow
	 an exception of type `kdu_exception'; at a minimum, this will cause
	 exceptions of type `KDU_MEMORY_EXCEPTION' to be rethrown as
	 `std::bad_alloc' which will help maintain consistency within your
	 application when exceptions are converted and passed across
	 programming interfaces as `kdu_exception'.  It also provides a path
	 to future catching and conversion of a wider range of exception
	 types.
*/

/*****************************************************************************/
/*                                kdu_thread                                 */
/*****************************************************************************/

class kdu_thread {
	/* [SYNOPSIS]
		 The `kdu_thread' object provides you with a platform independent
		 mechanism for creating, destroying and manipulating threads of
		 execution on Windows, Unix/Linux and MAC operating systems at
		 least -- basically, any operating system which supports
		 the "pthreads" or Windows threads interfaces, assuming the necessary
		 definitions are set up in "kdu_elementary.h".
	*/
public: // Member functions
	kdu_thread()
	{ /* [SYNOPSIS] You need to call `create' explicitly, or else the
					`exists' function will continue to return false. */
#       if defined KDU_WIN_THREADS
		thread = NULL; thread_id = 0;
#       elif defined KDU_PTHREADS
		thread_valid = false;
#       endif
	}
	bool exists()
	{ /* [SYNOPSIS]
		   Returns true if the thread has been successfully created by
		   a call to `create' that has not been matched by a completed
		   call to `destroy', or if a successful call to
		   `set_to_self' has been processed. */
#       if defined KDU_WIN_THREADS
		return (thread != NULL);
#       elif defined KDU_PTHREADS
		return thread_valid;
#       else
		return false;
#       endif
	}
	bool operator!() { return !exists(); }
	/* [SYNOPSIS] Opposite of `exists'. */
	bool equals(kdu_thread &rhs)
	{ /* [SYNOPSIS]
		   You can use this function to reliably determine whether or
		   not two `kdu_thread' objects refer to the same underlying
		   threads. */
#       if defined KDU_WIN_THREADS
		return ((rhs.thread != NULL) && (this->thread != NULL) &&
			(rhs.thread_id == this->thread_id));
#       elif defined KDU_PTHREADS
		return (rhs.thread_valid && this->thread_valid &&
			(pthread_equal(rhs.thread, this->thread) != 0));
#       else
		return false;
#       endif
	}
	bool operator==(kdu_thread &rhs) { return equals(rhs); }
	/* [SYNOPSIS] Same as `equals'. */
	bool operator!=(kdu_thread &rhs) { return !equals(rhs); }
	/* [SYNOPSIS] Opposite of `equals'. */
	bool set_to_self()
	{ /* [SYNOPSIS]
		   You can use this function to create a reference to the
		   caller's own thread.  The resulting object can be passed to
		   `equals', `get_priority', `set_priority' and `set_cpu_affinity'.
		   Do not invoke the `destroy' function on a `kdu_thread' object
		   created in this way.
		 [RETURNS]
		   False if thread functionality is not supported on the current
		   platform.  This problem can usually be resolved by
		   creating the appropriate definitions in "kdu_elementary.h".
	  */
#       if defined KDU_WIN_THREADS
		thread = GetCurrentThread();  thread_id = GetCurrentThreadId();
		return true;
#       elif defined KDU_PTHREADS
		thread = pthread_self();
		return thread_valid = true;
#       else
		return false;
#       endif
	}
	bool create(kdu_thread_startproc start_proc, void *start_arg)
	{ /* [SYNOPSIS]
		   Creates a new thread of execution, with `start_proc' as its
		   entry-point and `start_arg' as the parameter passed to
		   `start_proc' on entry.
		 [RETURNS]
		   False if thread creation is not supported on the current platform
		   or if the operating system is unwilling to allocate any more
		   threads to the process.  The former problem may be resolved by
		   creating the appropriate definitions in "kdu_elementary.h".
	  */
#       if defined KDU_WIN_THREADS
		thread = CreateThread(NULL, 0, start_proc, start_arg, 0, &thread_id);
#       elif defined KDU_PTHREADS
		thread_valid =
			(pthread_create(&thread, NULL, start_proc, start_arg) == 0);
#       endif
		return exists();
	}
	bool destroy()
	{ /* [SYNOPSIS]
		   Suspends the caller until the thread has terminated.
		 [RETURNS]
		   False if the thread has not been successfully created.
	  */
		bool result = false;
#       if defined KDU_WIN_THREADS
		result = ((thread != NULL) &&
			(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0) &&
			(CloseHandle(thread) == TRUE));
		thread = NULL;
#       elif defined KDU_PTHREADS
		result = (thread_valid && (pthread_join(thread, NULL) == 0));
		thread_valid = false;
#       endif
		return result;
	}
	bool set_cpu_affinity(kdu_long affinity_mask)
	{ /* [SYNOPSIS]
		   If supported, this function requests the scheduler to run the
		   thread only on one of the CPU's whose bit is set in the supplied
		   `cpu_mask'.  CPU n is included if bit n is set in `affinity_mask'.
		   The function returns true if the request is accepted.  Although
		   this function should be executable from any thread in the system,
		   experience shows that some platforms do not behave correctly
		   unless the function is invoked from within the thread whose
		   processor affinity is being modified.
		 [RETURNS]
		   False if the thread has not been successfully created, or the
		   `affinity_mask' is invalid, or the calling thread does not
		   have permission to set the CPU affinity of another thread, or
		   operating support has not yet been extended to offer this
		   specific feature.
	  */
#       if (defined KDU_WIN_THREADS)
#         if (defined _WIN64)
		ULONG_PTR result =
			SetThreadAffinityMask(thread, (ULONG_PTR)affinity_mask);
#         else
		int result = (int)
			SetThreadAffinityMask(thread, (DWORD)affinity_mask);
#         endif
		return (result != 0);
#       else
		return false;
#       endif
	}
	int get_priority(int &min_priority, int &max_priority)
	{ /* [SYNOPSIS]
		   Returns the current priority of the thread and sets `min_priority'
		   and `max_priority' to the minimum and maximum priorities that
		   should be used in successful calls to `set_priority'.
		 [RETURNS]
		   False if the thread has not been successfully created.
	  */
#       if (defined KDU_WIN_THREADS)
		min_priority = -2;  max_priority = 2;
		return GetThreadPriority(thread);
#       elif (defined KDU_NO_SCHED_SUPPORT)
		return (min_priority = max_priority = 0);
#       elif (defined KDU_PTHREADS)
		sched_param param;  int policy;
		if (pthread_getschedparam(thread, &policy, &param) != 0)
			return (min_priority = max_priority = 0);
		min_priority = sched_get_priority_min(policy);
		max_priority = sched_get_priority_max(policy);
		return param.sched_priority;
#       else
		return (min_priority = max_priority = 0);
#       endif
	}
	bool set_priority(int priority)
	{ /* [SYNOPSIS]
		   Returns true if the threads priority was successfully changed.
		   Use `get_priority' to find the current priority and a reasonable
		   range of priority values to use in calls to this function.
		 [RETURNS]
		   False if the thread has not been successfully created, or the
		   `priority' value is invalid, or the calling thread does not
		   have permission to set the priority of another thread.
	  */
#       if (defined KDU_WIN_THREADS)
		if (priority < -2) priority = -2;
		if (priority > 2) priority = 2;
		return (SetThreadPriority(thread, priority) != FALSE);
#       elif (defined KDU_NO_SCHED_SUPPORT)
		return false;
#       elif (defined KDU_PTHREADS)
		sched_param param;  int policy;
		if (pthread_getschedparam(thread, &policy, &param) != 0)
			return false;
		param.sched_priority = priority;
		return (pthread_setschedparam(thread, policy, &param) == 0);
#       else
		return false;
#       endif
	}
private: // Data
#   if defined KDU_WIN_THREADS
	HANDLE thread; // NULL if empty
	DWORD thread_id;
#   elif defined KDU_PTHREADS
	pthread_t thread;
	bool thread_valid; // False if empty
#   endif
};

/*****************************************************************************/
/*                                 kdu_mutex                                 */
/*****************************************************************************/

class kdu_mutex {
	/* [SYNOPSIS]
		 The `kdu_mutex' object implements a conventional mutex (i.e., a mutual
		 exclusion synchronization object), on Windows, Unix/Linux and MAC
		 operating systems at least -- basically, any operating system which
		 supports the "pthreads" or Windows threads interfaces, assuming the
		 necessary definitions are set up in "kdu_elementary.h".
	*/
public: // Member functions
	kdu_mutex()
	{ /* [SYNOPSIS] You need to call `create' explicitly, or else the
					`exists' function will continue to return false. */
#       if defined KDU_WIN_THREADS
		mutex = NULL;
#       elif defined KDU_PTHREADS
		mutex_valid = false;
#       endif
	}
	bool exists()
	{ /* [SYNOPSIS]
		   Returns true if the object has been successfully created in
		   a call to `create' that has not been matched by a call to
		   `destroy'. */
#       if defined KDU_WIN_THREADS
		return (mutex != NULL);
#       elif defined KDU_PTHREADS
		return mutex_valid;
#       else
		return false;
#       endif
	}
	bool operator!() { return !exists(); }
	/* [SYNOPSIS] Opposite of `exists'. */
	bool create()
	{ /* [SYNOPSIS]
		   You must explicitly call `create' and `destroy'.
		   The constructor and and destructor for the `kdu_mutex' object
		   do not create or destroy the underlying synchronization
		   primitive itself.
		 [RETURNS]
		   False if mutex creation is not supported on the current platform
		   or if the operating system is unwilling to allocate any more
		   synchronization primitives to the process.  The former problem
		   may be resolved by creating the appropriate definitions in
		   "kdu_elementary.h". */
#       if defined KDU_WIN_THREADS
		mutex = CreateMutex(NULL, FALSE, NULL);
#       elif defined KDU_PTHREADS
		mutex_valid = (pthread_mutex_init(&mutex, NULL) == 0);
#       endif
		return exists();
	}
	bool destroy()
	{ /* [SYNOPSIS]
		   You must explicitly call `create' and `destroy'.
		   The constructor and and destructor for the `kdu_mutex' object
		   do not create or destroy the underlying synchronization
		   primitive itself.
		 [RETURNS]
		   False if the mutex has not been successfully created. */
		bool result = false;
#       if defined KDU_WIN_THREADS
		result = ((mutex != NULL) && (CloseHandle(mutex) == TRUE));
		mutex = NULL;
#       elif defined KDU_PTHREADS
		result = (mutex_valid && (pthread_mutex_destroy(&mutex) == 0));
		mutex_valid = false;
#       endif
		return result;
	}
	bool lock()
	{ /* [SYNOPSIS]
		   Blocks the caller until the mutex is available.  You should
		   take steps to avoid blocking on a mutex which you have already
		   locked within the same thread of execution.
		 [RETURNS]
		   False if the mutex has not been successfully created. */
#       if defined KDU_WIN_THREADS
		return ((mutex != NULL) &&
			(WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0));
#       elif defined KDU_PTHREADS
		return (mutex_valid && (pthread_mutex_lock(&mutex) == 0));
#       else
		return false;
#       endif
	}
	bool try_lock()
	{ /* [SYNOPSIS]
		   Same as `lock', except that the call is non-blocking.  If the
		   mutex is already locked by another thread, the function returns
		   false immediately.
		 [RETURNS]
		   False if the mutex is currently locked by another thread, or
		   has not been successfully created. */
#       if defined KDU_WIN_THREADS
		return ((mutex != NULL) &&
			(WaitForSingleObject(mutex, 0) == WAIT_OBJECT_0));
#       elif defined KDU_PTHREADS
		return (mutex_valid && (pthread_mutex_trylock(&mutex) == 0));
#       else
		return false;
#       endif
	}
	bool unlock()
	{ /* [SYNOPSIS]
		   Releases a previously locked mutex and unblocks any other
		   thread which might be waiting on the mutex.
		 [RETURNS]
		   False if the mutex has not been successfully created. */
#       if defined KDU_WIN_THREADS
		return ((mutex != NULL) && (ReleaseMutex(mutex) == TRUE));
#       elif defined KDU_PTHREADS
		return (mutex_valid && (pthread_mutex_unlock(&mutex) == 0));
#       else
		return false;
#       endif
	}
private: // Data
#   if defined KDU_WIN_THREADS
	HANDLE mutex; // NULL if empty
#   elif defined KDU_PTHREADS
	friend class kdu_event;
	pthread_mutex_t mutex;
	bool mutex_valid; // False if empty
#   endif
};

/*****************************************************************************/
/*                                 kdu_event                                 */
/*****************************************************************************/

class kdu_event {
	/* [SYNOPSIS]
		 The `kdu_event' object provides similar functionality to the Windows
		 Event object, except that you must supply a locked mutex to the
		 `wait' function.  In most cases, this actually simplifies
		 synchronization with Windows Event objects.  Perhaps more importantly,
		 though, it enables the behaviour to be correctly implemented also
		 with pthreads condition objects in Unix.
	*/
public: // Member functions
	kdu_event()
	{ /* [SYNOPSIS] You need to call `create' explicitly, or else the
					`exists' function will continue to return false. */
#       if defined KDU_WIN_THREADS
		event = NULL;
#       elif defined KDU_PTHREADS
		cond_exists = manual_reset = event_is_set = false;
#       endif
	}
	bool exists()
	{ /* [SYNOPSIS]
		   Returns true if the object has been successfully created in
		   a call to `create' that has not been matched by a call to
		   `destroy'. */
#       if defined KDU_WIN_THREADS
		return (event != NULL);
#       elif defined KDU_PTHREADS
		return cond_exists;
#       else
		return false;
#       endif
	}
	bool operator!() { return !exists(); }
	/* [SYNOPSIS] Opposite of `exists'. */
	bool create(bool manual_reset)
		/* [SYNOPSIS]
			   You must explicitly call `create' and `destroy'.
			   The constructor and and destructor for the `kdu_event' object
			   do not create or destroy the underlying synchronization
			   primitive itself.
			   [//]
			   If `manual_reset' is true, the event object remains set from the
			   point at which `set' is called, until `reset' is called.  During
			   this interval, multiple waiting threads may be woken up --
			   depending on whether a waiting thread invokes `reset' before
			   another waiting thread is woken.
			   [//]
			   If `manual_reset' is false, the event is automatically reset once
			   any thread's call to `wait' succeeds.  If there are other
			   threads which are also waiting for the event, they will not
			   be woken up until the `set' function is called again.
			 [RETURNS]
			   False if event creation is not supported on the current platform
			   or if the operating system is unwilling to allocate any more
			   synchronization primitives to the process.  The former problem
			   may be resolved by creating the appropriate definitions in
			   "kdu_elementary.h".
		*/
	{
#       if defined KDU_WIN_THREADS
		event = CreateEvent(NULL, (manual_reset ? TRUE : FALSE), FALSE, NULL);
#       elif defined KDU_PTHREADS
		cond_exists = (pthread_cond_init(&cond, NULL) == 0);
		this->manual_reset = manual_reset;
		this->event_is_set = false;
#       endif
		return exists();
	}
	bool destroy()
	{ /* [SYNOPSIS]
		   You must explicitly call `create' and `destroy'.
		   The constructor and and destructor for the `kdu_mutex' object
		   do not create or destroy the underlying synchronization
		   primitive itself.
		 [RETURNS]
		   False if the event has not been successfully created. */
		bool result = false;
#       if defined KDU_WIN_THREADS
		result = (event != NULL) && (CloseHandle(event) == TRUE);
		event = NULL;
#       elif defined KDU_PTHREADS
		result = (cond_exists && (pthread_cond_destroy(&cond) == 0));
		cond_exists = event_is_set = manual_reset = false;
#       endif
		return result;
	}
	bool set()
	{ /* [SYNOPSIS]
		   Set the synchronization object to the signalled state.  This
		   causes any waiting thread to wake up and return from its call
		   to `wait'.  Any future call to `wait' by this or any other
		   thread will also return immediately, until such point as
		   `reset' is called.  If the `create' function was supplied with
		   `manual_reset' = false, the condition created by this present
		   function is reset as soon as any single thread returns
		   from a current or future `wait' call.  See `create' for more
		   on this.
		 [RETURNS]
		   False if the event has not been successfully created. */
#       if defined KDU_WIN_THREADS
		return ((event != NULL) && (SetEvent(event) == TRUE));
#       elif defined KDU_PTHREADS
		if (event_is_set) return true;
		event_is_set = true;
		if (manual_reset)
			return (pthread_cond_broadcast(&cond) == 0);
		else
			return (pthread_cond_signal(&cond) == 0);
#       else
		return false;
#       endif
	}
	bool reset()
	{ /* [SYNOPSIS]
		   See `set' and `create' for an explanation.
		 [RETURNS]
		   False if the event has not been successfully created. */
#       if defined KDU_WIN_THREADS
		return ((event != NULL) && (ResetEvent(event) == TRUE));
#       elif defined KDU_PTHREADS
		event_is_set = false;  return true;
#       else
		return false;
#       endif
	}
	bool wait(kdu_mutex &mutex)
	{ /* [SYNOPSIS]
		   Blocks the caller until the synchronization object becomes
		   signalled by a prior or future call to `set'.  If the object
		   is already signalled, the function returns immediately.  In
		   the case of an auto-reset object (`create'd with `manual_reset'
		   = false), the function returns the object to the non-singalled
		   state after a successful return.
		   [//]
		   The supplied `mutex' must have been locked by the caller;
		   moreover all threads which call this event's `wait' function
		   must lock the same mutex.  Upon return the `mutex' will again
		   be locked.  If a blocking wait is required, the mutex will
		   be unlocked during the wait.
		 [RETURNS]
		   False if the event has not been successfully created, or if
		   an error (e.g., deadlock) is detected by the kernel. */
#       if defined KDU_WIN_THREADS
		mutex.unlock();
		bool result = (event != NULL) &&
			(WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0);
		mutex.lock();
		return result;
#       elif defined KDU_PTHREADS
		bool result = cond_exists;
		while (result && !event_is_set)
			result = (pthread_cond_wait(&cond, &(mutex.mutex)) == 0);
		if (!manual_reset) event_is_set = false;
		return result;
#       else
		return false;
#       endif
	}
	bool timed_wait(kdu_mutex &mutex, int microseconds)
	{ /* [SYNOPSIS]
		   Blocks the caller until the synchronization object becomes
		   signalled by a prior or future call to `set', or the indicated
		   number of microseconds elapse.  Apart from the time limit, this
		   function is identical to `wait'.  If the available synchronization
		   tools are unable to time their wait with microsecond precision,
		   the function may wait longer than the specified number of
		   microseconds, but it will not perform a poll, unless
		   `microseconds' is equal to 0.
		 [RETURNS]
		   False if `wait' would return false or the time limit expires. */
#       if defined KDU_WIN_THREADS
		mutex.unlock();
		bool result = (event != NULL) &&
			(WaitForSingleObject(event, (microseconds + 999) / 1000) ==
				WAIT_OBJECT_0);
		mutex.lock();
		return result;
#       elif defined KDU_PTHREADS
		bool result = event_is_set;
		if (!result)
		{
			kdu_timespec deadline;
			if ((!cond_exists) || !deadline.get_time()) return false;
			deadline.tv_sec += microseconds / 1000000;
			if ((deadline.tv_nsec += (microseconds % 1000000) * 1000) >= 1000000000)
			{
				deadline.tv_sec++; deadline.tv_nsec -= 1000000000;
			}
			pthread_cond_timedwait(&cond, &(mutex.mutex), &deadline);
			result = event_is_set;
		}
		if (!manual_reset)
			event_is_set = false;
		return result;
#       else
		return false;
#       endif
	}
private: // Data
#   if defined KDU_WIN_THREADS
	HANDLE event; // NULL if empty
#   elif defined KDU_PTHREADS
	pthread_cond_t cond;
	bool event_is_set; // The state of the event variable
	bool manual_reset;
	bool cond_exists; // False if `cond' is meaningless
#   endif
};

#endif // KDU_ELEMENTARY_H
