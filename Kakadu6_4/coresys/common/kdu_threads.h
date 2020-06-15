/*****************************************************************************/
// File: kdu_threads.h [scope = CORESYS/COMMON]
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
   This header defines the core multi-threaded processing architecture used
by Kakadu.  Multi-processing is not required to build the Kakadu core system,
but these definitions allow you to exploit the availability of multiple
physical processors without having to worry about any of the details.
   The objects defined here are implemented in "kdu_threads.cpp".
******************************************************************************/

#ifndef KDU_THREADS_H
#define KDU_THREADS_H

#include <assert.h>
#include <string.h>
#include "kdu_elementary.h"

// Objects defined here
struct kd_thread_lock;
struct kd_thread_grouperr;
class kdu_thread_entity;
class kdu_worker;

// Objects defined elsewhere
struct kd_thread_group;
struct kdu_thread_queue;

#define KDU_MAX_THREADS 64 // Max threads in any one thread group
#define KDU_MAX_SYNC_NESTING 8


/*****************************************************************************/
/*                               kd_thread_lock                              */
/*****************************************************************************/

struct kd_thread_lock {
	kdu_mutex mutex;
	kdu_thread_entity *holder; // Entity which currently holds the lock
};

/*****************************************************************************/
/*                            kd_thread_grouperr                             */
/*****************************************************************************/

struct kd_thread_grouperr {
	bool failed;
	kdu_exception failure_code;
};

/*****************************************************************************/
/*                                 kdu_worker                                */
/*****************************************************************************/

class kdu_worker {
	/* [BIND: reference]
	   [SYNOPSIS]
		 Pure abstract base class, which exists only to define the `do_job'
		 function.  Actual work which is to be supplied as jobs to the
		 `kdu_thread_entity::add_jobs' function must be described by a class
		 derived from this one.
	*/
public:
	virtual ~kdu_worker() { return; } // Prevent compiler warnings
	virtual void do_job(kdu_thread_entity *ent, int job_idx) = 0;
	/* [SYNOPSIS]
		 This function is called with a negative `job_idx' only if
		 the containing object was registered against a queue using
		 the `kdu_thread_entity::register_synchronized_job' function.
		 Otherwise, the `job_idx' argument represents the ordinal
		 position at which the job was added to its queue, starting
		 from 0 for the first job in the queue.
	*/
};

/*****************************************************************************/
/*                             kdu_thread_entity                             */
/*****************************************************************************/

class kdu_thread_entity {
	/* [BIND: reference]
	   [SYNOPSIS]
		 This object represents the state of a running thread, which may call
		 into the `kdu_codestream' interface or any of its derivatives.  Its
		 `acquire_lock' and `release_lock' functions are used by the internal
		 machinery to block access to critical sections of code which are
		 sensitive to concurrent access from multiple threads.  Since it is
		 possible that an error is generated in one of these calls, which
		 subsequently throws an exception without terminating the process, it
		 is important that any locks are released when such an exception is
		 caught.  The `handle_exception' function is provided for this purpose.
		 [//]
		 Threads may belong to groups of cooperating workers.  Each such group
		 has an owner, whose `kdu_thread_entity' object was directly created
		 using `create'.  The other working threads are all created using
		 the owner's `add_thread' member.  When the owner is destroyed, the
		 entire group is destroyed along with it -- after waiting for all
		 workers to go idle.  When one thread in a group generates an error
		 through `kdu_error', resulting in a call to `handle_exception', the
		 remaining threads in the group will also throw exceptions when they
		 next call the `acquire_lock' function.  This helps ensure that the
		 group behaves as a single working entity.  In each case, exception
		 catching and rethrowing is restricted to exceptions of type
		 `kdu_exception'.
		 [//]
		 The purpose of thread groups is to use multiple physical processors
		 efficiently.  This is done by associating each working thread with a
		 separate processor and scheduling jobs onto the theads through a
		 collection of job queues.  Separate job queues typically involve
		 disjoint blocks of memory so that processor resources are used most
		 efficiently by keeping threads local to queues, as much as possible.
		 The scheduling mechanism is implemented through two functions:
		 `process_jobs' and `add_jobs'.
		 [//]
		 Thread queues are organized into a hierarchy, wherein new queues
		 may be created as top-level queues or as sub-queues of other queues.
		 At the bottom of this hierarchy are the leaf queues.  A number of
		 powerful features offered by this object may be defined to operate
		 only within sub-trees of the hierarchy.  Among these are the ability
		 to wait for completion of all jobs within a given sub-tree, the
		 ability to register a synchronized job to be run automatically when
		 all jobs in a sub-tree reach a defined point, and the ability to
		 selectively delete sub-trees from the hierarchy.
		 [//]
		 This object is designed to be sub-classed.  When you do so, your
		 derived object will be used by every thread within the same working
		 group.  Your derived object also specifies the number of distinct
		 locks which are to be provided to the `acquire_lock' and `release_lock'
		 functions, once `create' is called.  This is done by overriding the
		 `get_num_locks' function.
	*/
public: // Members
	KDU_EXPORT kdu_thread_entity();
	/* [SYNOPSIS]
		 The constructor is not a completely safe place for meaningful
		 construction.  For this reason, you must call `create' to
		 render the constructed object functional.  Prior to this point,
		 the `exists' function will return false.
	*/
	void pre_destroy() { if (!is_group_owner()) group = NULL; }
	/* This function is called automatically when a non-group-owner
	   thread finishes so that the safety check in the destructor will
	   not fail.  This is done only for code verification purposes.  You
	   should not call the function yourself from an application. */
	virtual ~kdu_thread_entity()
	{
		if (is_group_owner()) destroy();
		assert(group == NULL);
	}
	/* [SYNOPSIS]
		 While the destructor will perform all required cleanup, the
		 caller may be suspended for some time while waiting for worker
		 threads to complete.  For this reason, you are encouraged to
		 explicitly call the `destroy' function first.  This also provides
		 you with information on whether all threads terminated normally
		 or a failure was caught.
	*/
	KDU_EXPORT void *operator new(size_t size);
	/* This function allocates the thread object in such a way that it
	   occupies a whole number of L2 cache lines, thereby maximizing
	   cache utilization efficiency. */
	KDU_EXPORT void operator delete(void *ptr);
	/* This function deletes the memory allocated using the custom new
	   operator defined above. */
	virtual kdu_thread_entity *new_instance() { return new kdu_thread_entity; }
	/* [SYNOPSIS]
		 The `add_thread' function uses this function to create new
		 `kdu_thread_entity' objects for each new worker thread.  You
		 can override this virtual function to create objects of your own
		 derived class, thereby ensuring that all worker threads will
		 also use the derived class.  This is particularly convenient if
		 you want each thread in a group to manage additional
		 thread-specific data.  By careful implementation of the function
		 override, you can also arrange to inherit parameters from the
		 thread which creates new workers.
	*/
	bool exists() { return (group != NULL); }
	/* [SYNOPSIS]
		 Returns true only between calls to `create' and `destroy'.
		 You should not use a `kdu_thread_entity' object until it has
		 been created.
	*/
	bool operator!() { return !exists(); }
	/* [SYNOPSIS]
		 Opposite of `exists'.
	*/
	bool is_group_owner() { return (group != NULL) && (thread_idx == 0); }
	/* [SYNOPSIS]
		 As explained in the introduction to this object, each thread
		 entity belongs to a group which has exactly one owner.  This
		 function indicates whether or not this object is the owner for
		 its group.  The `kdu_thread_entity' objects passed into the
		 `kdu_worker::do_job' interface may well not be the owner of
		 their group.
	*/
	KDU_EXPORT kdu_thread_entity *get_current_thread_entity();
	/* [SYNOPSIS]
		 Returns a pointer to the `kdu_thread_entity' object belonging to
		 the calling thread.  The function may be invoked from the
		 `kdu_thread_entity' object belonging to any thread in the same
		 group.  If the caller does not have any membership of this thread
		 group, the function returns NULL.
	*/
	virtual int get_num_locks() { return (group == NULL) ? 1 : this->num_locks; }
	/* [SYNOPSIS]
		 Override this function in a derived class if your want your
		 created thread groups to offer more than the default of 1 lock,
		 accessible via the `acquire_lock' and `release_lock' functions.
	*/
	KDU_EXPORT void create(kdu_long cpu_affinity = 0);
	/* [SYNOPSIS]
		 You must call this function after construction to prepare the
		 thread entity for use.  Prior to this point `exists' will return
		 false.  After creation, the present object will become the owner
		 of a group, to which workers may be added using the `add_thread'
		 function.
	   [ARG: cpu_affinity]
		 This argument can be used to associate the threads which
		 collaborate in this group with one or more specific logical
		 CPU's.  If the value is zero, threads can run on any CPU.
		 Otherwise, the operating system is requested to schedule the
		 threads only on those logical CPU's n, such that bit n is set
		 in the `cpu_affinity' mask.  The function effectively causes
		 `kdu_thread::set_cpu_affinity' to be invoked on the current
		 thread and then each thread added to the group using
		 `add_thread'.  Whether or not this argument actually affects
		 thread scheduling depends on the specific platform and the support
		 offered by `kdu_thread::set_cpu_affinity' for that platform.
	*/
	KDU_EXPORT bool destroy();
	/* [SYNOPSIS]
		 This function does nothing unless the object has been created.
		 In that case, it returns it to the non-created state, allowing
		 `create' to be called again if desired.  The function essentially
		 calls `terminate' after first invoking `handle_exception' to
		 force termination as quickly as possible, then it destroys all
		 worker threads and ultimately the common group resources.
		 [//]
		 Only the group owner may call `destroy'.
	   [RETURNS]
		 False if any thread in the group failed (through `kdu_error'),
		 throwing an exception prior to the call to `destroy'.
	*/
	KDU_EXPORT int get_num_threads();
	/* [SYNOPSIS]
		 You may use this function to determine the total number of
		 threads which are currently associated with the thread group.
		 Of course, this value is changed by calls to `add_thread'.  If
		 the object has not yet been created, the present function returns
		 0, even through there must of course be at least one thread of
		 execution in your program.  If the object has been created, but
		 `add_thread' has not yet been called, the function returns 1,
		 since there is always an owner thread.
	*/
	KDU_EXPORT bool add_thread(int thread_concurrency = 0);
	/* [SYNOPSIS]
		 This function is used to add worker threads to the group owned
		 by the current thread.  The caller must, therefore, be the
		 group owner.  The new theads are automatically launched with
		 their own `kdu_thread_entity' objects.  They will not terminate
		 until `destroy' is called.
		 [//]
		 A reasonable policy is to add a total of P-1 workers, where
		 P is the number of physical processors (CPU's) on the platform.
		 The value of P can be difficult to recover consistently across
		 different platforms, since POSIX deliberately (for some good
		 reasons) refrains from providing a standard mechanism for
		 discovering the number of processors across UNIX platforms.
		 However, the `kdu_get_num_processors' function makes an attempt
		 to discover the value, returning 0 if it cannot.
		 [//]
		 The role of the `thread_concurrency' argument may seem a little
		 more mysterious.  This argument is provided to deal with
		 operating systems (notably Solaris), which do not automatically
		 create at least as many kernel threads as there are processors,
		 thereby preventing the process from exploiting the availability
		 of multiple physical CPU's through multiple threads.  This
		 problem can be resolved by using `pthread_setconcurrency' to
		 set the number of kernel threads (threads associated with
		 distinct processors).  However, it is unlikely that application
		 level programmers will remember to do this or know what value
		 to set.  For this reason, the default behaviour of the present
		 function, when operating in a PTHREADS environment, is to adjust
		 the thread concurrency to match the number of threads which
		 are added into the `kdu_thread_entity' object.  This is a fine
		 solution when there is only one `kdu_thread_entity' object, which
		 is certainly the most common scenario. When there are multiple
		 objects, however, they may compete with each other to set the
		 thread concurrency level for the process.  You may avoid this
		 problem by supplying a non-zero `thread_concurrency' argument,
		 in which case that value will be passed to `pthread_setconcurrency'
		 each time.  For operating systems which do not pay any
		 attention to `pthread_concurrency', it does not matter what
		 value you supply for this argument.  For consistency, however,
		 the best approach is either to set the argument either to 0, -1
		 or a value equal to the number of processors you hope to be able
		 to utilize concurrently.  The special value -1 means that the
		 existing thread concurrency level should not be changed.
	   [RETURNS]
		 False if insufficient resources are available to offer
		 a new thread of execution, or if a multi-threading implementation
		 for the present architecture does not exist in the definitions
		 provided in "kdu_elementary.h".  You may wish to check them for
		 your platform.
	   [ARG: thread_concurrency]
		 As explained at length above, this argument is best set to 0, -1
		 or the number of physical processors you would like to
		 be able to use concurrently in your application (not just in
		 this instance of the `kdu_thread_entity' object, if there are
		 multiple instances of it).  Since it is common to call the
		 `add_thread' function P-1 times, where P is the number of
		 processors, a reasonable policy would be to supply P for the
		 value of `thread_concurrency', each time the function is called.
		 If you leave the `thread_concurrency' argument at its default value
		 of 0, the actual thread concurrency will be set equal to the
		 number of threads associated with this object, including the owner
		 thread.  If you pass -1 to this function, the thread concurrency
		 will be left unchanged, which can be useful if you don't want to
		 alter a configuration optimized elsewhere.
	*/
	KDU_EXPORT kdu_thread_queue *
		add_queue(kdu_worker *worker, kdu_thread_queue *super_queue,
			const char *name = NULL, kdu_long queue_bank_idx = 0);
	/* [SYNOPSIS]
		 Creates a new job queue, returning an opaque handle to the
		 created queue.  You cannot directly access any members of the
		 job queue, but you can pass it to `add_jobs', or as a
		 super-queue in further calls to `add_queue', or as a
		 synchronization or termination handle to `synchronize', `terminate'
		 or `register_synchronized_job'.
		 [//]
		 The association between jobs and queues is foundational to the
		 thread scheduling algorithm implemented internally.  You should
		 create one queue for each distinct block of memory in which
		 workers might be required to perform jobs.  The scheduler will
		 then attempt to keep threads working in blocks of memory (queues)
		 which they have used before, while also keeping all threads
		 occupied whenever there is work to be done.
		 [//]
		 The provision of a non-NULL `super_queue' argument means that
		 the newly added queue is to be considered a "sub-queue".
		 If the scheduling algorithm cannot keep a thread working within
		 the same queue, it attempts to keep it working within the same
		 super-queue, or that queue's super-queue, and so forth.  Thus,
		 the use of super-queues provides the scheduler with more clues
		 regarding jobs which are likely to involve closely related
		 resources.  The use of super-queues has three important additional
		 advantages:
		 [>>] Where a large number of queues is involved, partitioning
			  them hierarchically into super-queues can reduce the number
			  of queues which the scheduler must search to find a free one.
		 [>>] It is possible to selectively wait for and terminate just
			  those queues which belong to a given super-queue.
		 [>>] It is possible to schedule special "synchronized jobs" to be
			  run within a super-queue, whenever its sub-queues reach a
			  declared state.  By providing multiple super-queues, it is
			  possible to have multiple outstanding synchronized jobs
			  simultaneously.
		 [//]
		 A typical way to use super-queues in the Kakadu environment is
		 as follows: 1) each tile processor may be associated with a
		 single top-level queue; 2) each tile-component processor may be
		 associated with a sub-queue of the corresponding tile queue; and
		 3) each subband may be associated with a sub-queue of the
		 corresponding tile-component queue.  In this context, code-blocks
		 are processed within the leaf queues.
		 [//]
		 To make the best use of multiple physical processors, you should
		 have at least one queue for each thread, with at least one thread
		 for each processor.
		 [//]
		 It is worth noting that any thread may create queues, not just the
		 group owner.
		 [//]
		 From Kakadu version 5.2.4, queues can be additionally organized
		 into so-called "queue banks".  Queue banks allow for the deferral
		 of all jobs on some queues until the system can be reasonably
		 sure that threads about to become permanently idle.  The idea is
		 that banks should be processed as sequentially as possible while
		 keeping all threads active as much of the time as possible.
		 [//]
		 The system uses the information provided in calls to the `add_jobs'
		 function to determine when the system is likely to be
		 "under-utilized," meaning that there are less working leaf queues
		 in the queue hierarchy than the number of threads.  A working
		 leaf queue is any queue to which jobs can still be added, which
		 has no working descendants.  For more on working queues, see
		 the `add_jobs' function.  The system also maintains a
		 list of "dormant" queue banks.  Once the system becomes
		 under-utilized, dormant banks are moved onto the active list
		 until full utilization can be achieved.  If the system is
		 under-utilized when a new queue is added, that queue (and its
		 bank) are immediately made active.
		 [//]
		 If any queue belongs to a particular bank, all of its sub-queues
		 must belong to the same bank.  For this reason, the
		 `queue_bank_idx' argument is ignored whenever `super_queue'
		 is non-NULL.  When super-queue is non-NULL, however, the
		 `queue_bank_idx' argument is used to determine whether or not
		 new banks need to be created.
	   [ARG: worker]
		 Pointer to a `kdu_worker' object whose `kdu_worker::do_job'
		 function is used to process jobs for this queue.  This argument can
		 be NULL if you do not intend to add jobs to this queue -- in this
		 case, the queue is being used only to organize sub-queues in the
		 overall queue hierarchy.  A working queue is one which has a
		 non-NULL `worker', for which not all potential jobs have yet
		 been processed.  The `add_jobs' function informs the system of
		 the point beyond which no more jobs will be added to a queue, so
		 that its working status can readily be determined.
	   [ARG: super_queue]
		 If NULL, a new top-level queue is created.  Otherwise, a new
		 sub-queue is added to the supplied super-queue.  The queue
		 hierarchy may extend to any desired number of levels.  See the
		 comments above for a discussion of the merits of queue
		 hierarchies.
	   [ARG: name]
		 If non-NULL, this argument must point to a constant text string
		 whose contents will not change over the life of the queue.  The
		 name is currently only used for internal debugging purposes.
	   [ARG: queue_bank_idx]
		 If you don't want any dormant banks, it is best to leave this
		 argument set to 0.  Otherwise, you can use it to add queues
		 to initially dormant banks, which become active only once the
		 system becomes under-utilized.  A typical example of queue banks
		 would be to use one bank for each frame in a video sequence.
		 This allows you to add queues (and add jobs to the queues) for
		 frames which are not currently being processed, confident in the
		 fact that these will not start to consume processing resources
		 until the system becomes under-utilized, which means that the
		 processing of the current frame (or codestream) is nearing
		 completion.
	*/
	KDU_EXPORT void
		add_jobs(kdu_thread_queue *queue, int num_jobs,
			bool finalize_queue, kdu_uint32 secondary_seq = 0);
	/* [SYNOPSIS]
		 Schedules one or more discrete jobs onto the indicated `queue'.
		 When a thread becomes available to service these jobs, it will
		 call the `kdu_worker::do_job' function, associated with the
		 `kdu_worker' object which was installed with the queue during
		 the original call to `add_queue'.  In calling `kdu_worker::do_job',
		 it will pass in its own `kdu_thread_entity' identity, along with
		 the index of the job.  The `kdu_worker' object is expected to be
		 able to figure out what must be done, based on the job index alone.
		 [//]
		 Job indices start from 0 (the first job added after creation of
		 the queue has this index) and increment consecutively thereafter.
		 [//]
		 The `finalize_queue' argument plays an important role in
		 minimizing thread idle time in certain applications.  The caller
		 should set this argument to false, unless it knows that
		 there will be no further jobs added to the present queue.  This
		 allows the system to determine the point at which the system is
		 likely to become under-utilized, meaning that there may be
		 insufficient schedulable queues to keep all threads working.
		 The under-utilization condition is explained more carefully in
		 conjunction with the `add_queue' function.  If you cannot be
		 sure whether or not new jobs might be added to a queue, it is
		 not essential that you call this function with the `finalize_queue'
		 function set to true.  This is because calls to `terminate'
		 automatically finalize the relevant queues before terminating
		 them.  However, relying upon the termination of queues for the
		 detection of under-utilization may lead to increased thread
		 idle time.  In any case, once the last job in a finalized queue
		 has been executed, it ceases to be a "working queue", for the
		 purpose of the description found in connection with the `add_queue'
		 function.
		 [//]
		 To understand the `secondary_seq' argument, it is best to know
		 something about how Kakadu schedules jobs.  During their lifetime,
		 jobs may take on four different scheduling states: "active"
		 (meaning they are actually running); "runnable" (meaning they are
		 ready to become active whenever a thread becomes available);
		 "primary"; and "secondary".  Jobs added by this function start
		 out either as "primary" or as "secondary", depending on whether
		 `secondary_seq' is 0.  Jobs are promoted to "runnable"
		 only when a thread would otherwise become idle due a lack of
		 runnable jobs to process.  When a "primary" job is promoted to
		 runnable, all jobs in the entire queue hierarchy which are currently
		 in the "primary" state are simultaneously promoted to runnable.
		 This provides for a kind of fairness across all queues in the
		 hierarchy, where jobs are advanced from "primary" to "runnable"
		 in batches.  If no "primary" jobs are available and a thread would
		 otherwise have nothing to do, a single "secondary" job is
		 selected to keep threads from becoming idle.
		 [//]
		 The idea is that no dependent tasks should ever need to block on
		 a "secondary" job.  These jobs are provided well in advance of their
		 actual deadlines so as to keep all processing resources in use as
		 much of the time as possible.  Of course, one could expect that
		 "secondary" jobs would eventually become important dependencies for
		 other jobs -- typically after promotion, activation and completion
		 of one or more "primary" jobs.
		 [//]
		 To accommodate the promotion of "secondary" jobs to the "primary"
		 state, we provide the following two policies: a) whenever this
		 function is called, any "secondary" jobs on the queue which have
		 not already become runnable are automatically promoted to the
		 "primary" state; and b) the function may be called with `num_jobs'
		 equal to 0, the only purpose of which is to promote all
		 outstanding "secondary" jobs to the "primary" state.
		 [//]
		 The Kakadu core system uses the "secondary" state when
		 adding block decoding jobs in subbands for which existing
		 decoded subband lines still remain to be consumed by the DWT
		 synthesis engine.  Once the DWT synthesis engine has consumed
		 all previously buffered sample lines, new secondary jobs are
		 added, which advances the existing secondary jobs to the primary
		 state.  A similar thing is done for block encoding jobs.
		 [//]
		 When a secondary job must be promoted directly to runnable in
		 order to keep threads active, an attempt is made to promote first
		 those secondary jobs whose `secondary_seq' argument was smallest
		 when they were added.  The Kakadu core system assigns higher
		 `secondary_seq' values to block decoding/encoding jobs which
		 correspond to higher resolution subbands, so that some higher
		 resolution ones are likely to still be available (as fill in
		 jobs) when the end of the image is reached.
	   [ARG: num_jobs]
		 Number of jobs (with consecutive job indices) being added to the
		 queue.  If this argument is zero, any jobs already on the queue
		 which are currently still in the "secondary" state will be
		 promoted to the "primary" state.
	   [ARG: secondary_seq]
		 If zero, the new jobs will be queued in the "primary" service
		 state.  Otherwise, they are queued in the "secondary" state and
		 an attempt will be made to respect the supplied sequence number,
		 so that whenever a secondary job must be promoted to the runnable
		 state directly (to prevent a thread from idling), jobs with
		 lower `secondary_seq' values will be promoted before jobs with
		 higher `secondary_seq' values.
	*/
	KDU_EXPORT bool
		synchronize(kdu_thread_queue *root_queue,
			bool finalize_descendants_when_synchronized = false,
			bool finalize_root_when_synchronized = false);
	/* [SYNOPSIS]
		 This function waits for all outstanding work on the supplied
		 `root_queue' and all of its descendants (sub-queues, sub-sub-queues,
		 etc.) to complete.  If `root_queue' is NULL, all work on all queues
		 must complete before the function returns.  By the time the
		 function returns, all threads which have done any work within
		 the identified sub-tree of the queue hierarchy are guaranteed
		 also to have invoked their `do_sync' function, since they have
		 either become idle or moved onto other branches in the queue
		 hierarchy -- see `do_sync' for a discussion of the conditions
		 under which that function is called.  The bottom line is that
		 you can be quite sure that all relevant work has been done and
		 that all relevant thread-local state information has been
		 globally synchronized by the time the function returns.
		 [//]
		 It is worth noting that the calling thread itself contributes
		 to the pool of working threads.  This minimizes the performance
		 penalty which can be expected from invoking this function
		 frequently.  Also, the caller will not throw an exception if
		 failure occurs inside the thread group.  Instead, the return
		 value indicates whether or not this has occurred.
		 [//]
		 One important restriction of the present function is that the
		 `root_queue' argument may not refer to a leaf in the queue
		 hierarchy -- i.e., it must refer to a queue which has sub-queues.
		 The only exception to this is where a leaf queue has never been
		 assigned any jobs.  The reason for this restriction is rather
		 technical, but the main reason has to do with the fact that we
		 need to ensure that `do_sync' has been invoked on any thread which
		 has done work in the queue by the time the synchronization condition
		 is achieved, while at the same time we want to avoid calling
		 `do_sync' overly often.  The compromise is to allow this function
		 to be invoked only on super-queues and to insist that whenever a
		 thread moves from processing within one non-leaf queue to
		 processing within a different non-leaf queue, or between leaves
		 with different super-queues, the `do_sync' function will be called
		 first.  For more on this, `do_sync'.
		 [//]
		 It is worth pointing out that `process_jobs' can be used to
		 directly wait for completion of outstanding jobs on a queue, but
		 the behaviour offered by `process_jobs' is quite different
		 (in fact complementary) to that offered by `synchronize'.  While
		 the present function cannot be invoked on leaf queues, the
		 `process_jobs' function will commonly be used to wait for
		 completion of outstanding jobs on leaf queues.  Also, while this
		 function can be invoked over the entire thread group (by setting
		 `root_queue' to NULL), the `process_jobs' function requires a
		 non-NULL queue to wait upon.  Finally, the `process_jobs'
		 function is only interested in synchronizing on a single queue,
		 without regard for the state of its sub-queues.  By contrast,
		 `process_jobs' synchronizes on the entire sub-tree anchored at
		 `root_queue'.
		 [//]
		 Since this function may be invoked from any thread, a natural
		 question to ask is what happens if a thread adds jobs to the
		 queue on which we are waiting or any of its descendants, while
		 we are still waiting.  The behaviour in this case, may be
		 understood in terms of the following recursive description of
		 queue synchronization points:
		 [>>] When `synchronize' is called, a synchronization point, S_q,
			  is immediately installed in the `root_queue', q.  In
			  particular, S_q is set equal to the index of the next job
			  to be added to that queue.
		 [>>] Once the job with index S_q-1 completes (or immediately, if
			  the job has already completed or no job was ever added), a
			  synchronization point, S_b, is installed in each of its
			  sub-queues, b; the value of S_b is set equal to the index of
			  the next job to be added to the corresponding sub-queue, b.
		 [>>] This procedure continues recursively, until we encounter a
			  leaf queue.  Once all leaf queues, e, under the `root_queue'
			  have reached their synchronization points (i.e., once all
			  corresponding jobs S_e-1 complete), the `synchronize' function
			  returns.
		 [//]
		 A second important question to answer here is what happens if
		 a thread attempts to synchronize on a condition while a call to
		 `synchronize' is already in progress.  The implementation here
		 is intended to handle such situations robustly.  It does so by
		 maintaining a list of synchronization points inside each queue.
		 Once any queue reaches a synchronization point, the next element
		 in the list is moved to the head.  There is, however, a limit
		 on the number of outstanding synchronization points which any
		 queue can manage -- this limit is given by the
		 `KDU_MAX_SYNC_NESTING' macro.
		 [//]
		 Note that it is perfectly legal to wait upon queues which
		 currently belong to dormant banks (see `add_queue').  If the
		 condition must be waited upon, it will not occur until after
		 the relevant dormant queue banks have become active, in the
		 normal course of affairs.
		 [//]
		 For additional information on the handling of synchronization
		 conditions, you may wish to consult the documentation for the
		 `register_synchronized_job' function.
	   [RETURNS]
		 False if any thread in the entire thread group has invoked its
		 `handle_exception' function.  In this case, the synchronized state
		 will generally be achieved quickly, since all outstanding jobs
		 on all queues should have been cancelled.
	   [ARG: root_queue]
		 If NULL, we are waiting for all queues in the entire thread group
		 to complete.  Otherwise, we are waiting only for the
		 completion of jobs on the indicated queue and all of its
		 descendant queues.  Note that `queue' must point to a super-queue
		 if non-NULL -- i.e., a queue which has at least one sub-queue.
	   [ARG: finalize_descendants_when_synchronized]
		 This argument is normally set to true only when the function is
		 called from within `terminate'.  It forces all descendants of
		 the `root_queue' to be marked as finalized (as though
		 `add_jobs' where called with its `finalize_queue' argument set to
		 true) once the synchronization condition is reached in those
		 queues.
	   [ARG: finalize_root_when_synchronized]
		 This argument is normally set to true only when the function is
		 called from within `terminate', with its `leave_root' argument set
		 to false.  It forces the `root_queue' to be marked as finalized
		 (as though `add_jobs' were called with its `finalize_queue'
		 argument set to true) once the synchronization condition is
		 reached.
	*/
	KDU_EXPORT bool
		terminate(kdu_thread_queue *root_queue, bool leave_root,
			kdu_exception *exc_code = NULL);
	/* [SYNOPSIS]
		 This function essentially wraps up a call to `synchronize',
		 followed by deletion of the identified `root_queue' and all of its
		 descendants (sub-queues, sub-sub-queues, etc.).  If `leave_root' is
		 true, the supplied `root_queue' will not be deleted, but all of its
		 descendants will be deleted.
		 [//]
		 You should not call this function unless you
		 can be sure that after the internal `synchronize' call returns,
		 no other object in the system will try to use any of the queues
		 which are to be deleted.  In particular, you should be particularly
		 careful that deferred synchronized jobs do not attempt to add
		 or otherwise use any of the queues which are to be deleted, since
		 you cannot be sure when these will be run -- see
		 `register_synchronized_job' for more on this.
		 [//]
		 If called with `root_queue'=NULL, the function does not return until
		 all work on all queues in the thread group is complete.
		 [//]
		 If this call results in the deletion of all queues in the thread
		 group, either because `root_queue' is NULL or because it refers
		 to the last top-level queue in the group and `leave_root' is
		 false, a special processing step is invoked to ensure that all
		 worker threads, along with the calling thread invoke their
		 protected `on_finished' member function, after which the
		 object is prepared for a clean start with new queues and jobs.
		 You must make sure that this only happens when the calling thread
		 is the group owner.  The `on_finished' function may provide
		 final cleanup code to prepare for the destruction of top-level
		 objects with which the threads may have been working.  Once all
		 threads have invoked their `on_finished' function, you can also be
		 sure that all deferred synchronized jobs have been fully run -- see
		 `register_synchronized_job'.  As mentioned, after such a call to
		 `terminate' returns, the object is prepared for a clean start.
		 This includes the removal of any failure code installed by a
		 previous call to `handle_exception'.
		 [//]
		 This function will never throw an exception.  Instead, all
		 internal exceptions are caught.  However, if any thread in the
		 group invokes the `handle_exception' function, either before or
		 after this function is called, the function will produce a
		 return value of false.
		 [//]
		 Once `terminate' has been called, it can be called again as
		 often as you like prior to adding new queues, without imposing
		 any significant computational cost.
	   [RETURNS]
		 False if any thread in the group invoked its `handle_exception'
		 function either before or during the execution of this function.
	   [ARG: root_queue]
		 If NULL, the function waits until all queues in the thread group
		 are complete and then deletes them all.  Otherwise, only those
		 jobs which are related to the supplied `root_queue' must complete
		 and the function only deletes the `root_queue' (if `leave_root' is
		 false) and its descendants.  In either case, if all queues wind up
		 getting deleted, all threads will pass through their protected
		 `on_finished' member functions and the object will be left in
		 a state prepared for the addition of new queues and jobs.  As
		 with the `synchonize' function, `root_queue' may only refer to
		 a super-queue -- one with at least one sub-queue.
	   [ARG: leave_root]
		 If true or `root_queue' is NULL, only those queues which are
		 descendants of `root_queue' will be deleted.  Otherwise, the
		 `root_queue' will also be deleted.
	   [ARG: exc_code]
		 If non-NULL, the referenced variable will be set to the value of
		 the `exc_code' which was passed to `handle_exception' by
		 any failed thread.  This is meaningful only if the function
		 returns false.  Note that a special value of `KDU_MEMORY_EXCEPTION'
		 means that a `std::bad_alloc' exception was caught and saved.  If
		 you intend to rethrow the exception after this function returns,
		 you should ideally check for this value and rethrow the exception
		 as `std::bad_alloc'.
	*/
	KDU_EXPORT void
		register_synchronized_job(kdu_worker *worker, kdu_thread_queue *queue,
			bool run_deferred);
	/* [SYNOPSIS]
		 This function registers a special `worker' object, whose
		 `kdu_worker::do_job' function will not be called until all
		 currently outstanding jobs associated with the identified `queue'
		 and its descendants have been completed.  To be more specific, the
		 conditions required for a synchronized job to become runnable
		 are identical to those described for the `synchronization'
		 function to return, i.e.:
		 [>>] When `register_synchronized_job' is called, a synchronization
			  point, S_q, is immediately installed in the `root_queue', q.
			  In particular, S_q is set equal to the index of the next job
			  to be added to that queue.
		 [>>] Once the job with index S_q-1 completes (or immediately, if it
			  has already completed, or the queue has never had any jobs),
			  a synchronization point, S_b, is installed in each of its
			  sub-queues, b; the value of S_b is set equal to the index of
			  the next job to be added to the corresponding sub-queue, b.
		 [>>] This procedure continues recursively, until we encounter a
			  leaf queue.  Once all leaf queues, e, under the `root_queue'
			  have reached their synchronization points (i.e., once all
			  corresponding jobs S_e-1 complete), the synhronized job
			  becomes runnable.
		 [//]
		 If `queue' is NULL, the `worker' object is effectively
		 registered against an invisible super-queue which is the root
		 of all queues in the thread group.  This means that the
		 synchronized job will become runnable only once all jobs which
		 have been added to any top level queue are completed, and once
		 all jobs added to their sub-queues prior to that point have been
		 completed, and so forth.
		 [//]
		 Once a synchronized job becomes runnable, the `worker->do_job'
		 function is either called immediately from the thread which first
		 caused the synchronization condition to occur, or else it is
		 stored on a queue of runnable synchronized jobs, to be run when
		 resources become available.  Which of these two actions is taken
		 depends on the `run_deferred' argument.
		 [//]
		 If `run_deferred' is true, and there are multiple threads, the
		 synchronized job is placed on a special queue associated with
		 the entire thread group, to be run as soon as any worker thread
		 completes all of its active jobs.  In practice, this means that
		 deferred jobs will only be run by worker threads within the
		 context of their top-level `process_jobs' call -- the one which
		 has no `wait_queue'.  This policy ensures that the processing of
		 deferred jobs cannot block the execution of any other thread.  It
		 thus allows large background jobs to be completed without risking
		 a loss of throughput due to other thread resources (and their
		 CPU's) becoming idle.  The downside of deferred jobs is that you
		 cannot be sure when they will be run.  In fact, it is possible
		 that the `queue' against which they were registered will be
		 terminated before the synchronized job is run -- depending on
		 how the application chooses to call `terminate', of course.
		 The only thing you can be sure of is that all deferred jobs will
		 be run before the return of a call to `terminate' which results
		 in the deletion of all top-level job queues.  This is because
		 such a call returns only once all threads have passed through
		 their `on_finished' function, which will not happen while
		 deferred jobs remain.
		 [//]
		 If you would just like to install a deferred job, without any
		 associated synchronization conditions, you have simply to create
		 a queue which has no jobs of its own and no descendants.  Then
		 register a deferred synchronized job against this queue.  Since
		 synchronization is achieved immediately, it will be added
		 immediately to the queue of deferred jobs.
		 [//]
		 It is worth noting that the `run_deferred' argument has no impact
		 if there is only one thread.  In this case, deferred jobs are run
		 as soon as their synchronization conditions occur.  If these
		 conditions exist already, the function will be run immediately.
		 [//]
		 If two activities (synchronizing threads or synchronized
		 jobs) are registered against the same condition, they will
		 be performed (return from `synchronize' or perform synchronized
		 job) in the same order that they were registered.  If the order
		 is important, then you should request the relevant synchronized
		 activities from the same thread.  A typical example is the
		 registration of a sychronized job, followed by a call to
		 `synchronize' (e.g., through `terminate').  If both are done
		 from the same thread, and the synchronized job is not deferred,
		 you can be sure that the synchronized job will be run (completely)
		 before `synchronize' returns, which means that you will not
		 terminate any of the queues associated with a synchronized job
		 until after the job is complete.  As already mentioned, this
		 cannot be guaranteed for deferred jobs (those registered with
		 `run_deferred'=true).  The only thing you can be sure of with
		 such jobs is that a `terminate' call which specifies a NULL
		 `root_queue' (i.e., one which terminates the entire queue
		 hierarchy) will wait until the comletion of all deferred jobs
		 before returning.
		 [//]
		 As with `synchronize', each queue is able to manage multiple
		 outstanding synchronization points arising from any combination
		 of synchronized jobs and/or calls to `synchronize', with an
		 upper limit for each queue set by `KDU_MAX_SYNC_NESTING'.  Since
		 you cannot predict definitively when a synchronized job will
		 be executed, it is possible that you could overflow this limit
		 by registering repeated instances of a synchronized job without
		 first checking to see if previous instances have already
		 been dispatched.  For this reason, if you intend to regularly
		 schedule synchronized jobs (e.g., calls to
		 `kdu_codestream::flush' for incremental codestream generation),
		 you should first check that previously scheduled jobs have
		 been dispatched.  There are many ways to do this.
		 [//]
		 Note that it is perfectly legal to register synchronized jobs
		 against queues which currently belong to dormant banks (see
		 `add_queue').  If the condition must be waited upon, it will not
		 occur until after the relevant dormant queue banks have become
		 active, in the normal course of affairs.
	   [ARG: worker]
		 Object whose `kdu_worker::do_job' function will be called at
		 the appropriate time, as described above.
	   [ARG: queue]
		 If non-NULL, the synchronized jobs will become runnable once
		 all jobs on the specified `queue' have completed, once all
		 jobs added to any of its sub-queues prior to that point have
		 completed, and so on.  If NULL, the job will become runnable
		 only once this condition is achieved by all top-level queues.
		 As with `synchronize' and `terminate', this argument may only
		 refer to a super-queue -- i.e., one which has at least one
		 sub-queue -- or to a leaf queue which has never been assigned
		 any jobs.  The reasons for this have to do with the conditions
		 under which `do_sync' and `need_sync' are called.
	   [ARG: run_deferred]
		 If false, or there is only one thread in the system, the
		 `worker->do_job' function will be called immediately after the
		 job becomes runnable.  Moreover, no further synchronization
		 points associated with the `queue' concerned will be processed
		 until after the `worker->do_job' function returns.  On the other
		 hand, if `run_deferred' is true and there are at least two threads
		 (i.e., at least one worker thread, in addition to the group owner),
		 runnable jobs are entered on their own special queue, to be
		 processed by the next worker thread (not the group owner) which
		 has no active jobs, as explained above.
	*/
	KDU_EXPORT bool
		process_jobs(kdu_thread_queue *wait_queue, bool waiting_for_sync = false,
			bool throw_group_failure = true);
	/* [SYNOPSIS]
		 This function is responsible for the scheduling and execution
		 of queued jobs.  If `wait_queue' is NULL, the function processes
		 jobs indefinitely, blocking as required when there are no jobs
		 to process, returning only if some thread in the group calls
		 `destroy'.  In this case, the return value will be false.  You
		 would not normally use the function in this way from your own
		 application; rather, this is the way the function is used in
		 the entry-point function associated with each distinct thread
		 of execution which is added using `add_thread'.
		 [//]
		 If `wait_queue' is non-NULL, the function schedules and executes
		 jobs as above, but only until some condition is reached on the
		 indicated queue.  Two types of conditions may be of interest:
		 [>>] If `waiting_for_sync' is false, you are waiting for all
			  outstanding jobs on the `wait_queue' to complete, with the
			  exception of "secondary" jobs.  To understand "secondary"
			  consult the interface documentation for `add_jobs'; it
			  is sufficient here to point out that you can readily
			  promote all secondary jobs to the primary state by invoking
			  `add_jobs' with a `num_jobs' argument of 0.  After doing
			  this, the function will wait for the completion of all jobs
			  added to the queue.  The calling thread may participate in
			  the processing of any outstanding jobs on the queue, or it
			  may process jobs on other queues while waiting.
		 [>>] If `waiting_for_sync' is false, the caller is waiting for
			  an installed synchronization point to be reached, where
			  the calling thread is explicitly identified as a synchronizing
			  thread in the list of synchronization points currently
			  found in the `wait_queue'.  This mode of invocation is
			  appopriate only when the function is called from within
			  the `synchronize' function, so you would not use it in this
			  way from your own application.
		 [//]
		 In both the above cases, the function returns true if the wait
		 condition occurs.  If the condition holds on entry, the function
		 returns true immediately.
		 [//]
		 Note that this function may throw an exception, if an error
		 occurs while executing a queued job, or if another thread is
		 found to have done so.  In either case, an execption will be
		 thrown only if `throw_group_failure' is true.  Exceptions, if
		 thrown, have values of type `kdu_exception'.
		 [//]
		 If `throw_group_failure' is false, the function catches exceptions
		 of type `kdu_exception', thrown while processing jobs, and
		 automatically invokes the `handle_exception' function for you.
		 Moreover, with `throw_group_failure' false, the function neither
		 throws exceptions nor returns, unless either a wait condition is
		 satisfied (returns true) or destruction is requested (returns
		 false).  This means that errors can go undetected, which is not
		 normally desirable.  It is desirable, however, when the function
		 is executed from within the thread entry-point function, since it
		 ensures that the thread scheduling algorithm continues to operate
		 across error conditions until such time as the entire
		 multi-threaded environment is to be destroyed.
		 [//]
		 It is worth noting that this function may be (and often is) invoked
		 from within the `kdu_worker::do_job' function itself.  That is,
		 the thread can process new jobs while it is still processing an
		 existing one.  When this happens, the embedded call to
		 `process_jobs' must be issued with a non-NULL `wait_queue' argument,
		 meaning that the thread is making itself available to process new
		 jobs only while waiting for all processing to complete on a queue,
		 presumably so that it can continue to process its existing open
		 job.  This nested rescheduling can be used effectively to keep
		 physical processors occupied close to 100% of the time, even
		 in the face of complex dependencies.
	   [RETURNS]
		 True if `wait_queue' is non-NULL and the condition on which the
		 caller is waiting occurs.  The function returns false only if
		 destruction is requested and `throw_group_failure' is false.
	   [ARG: wait_queue]
		 If non-NULL, the caller is interested in one of two possible
		 conditions associated with the indicated queue, as determined
		 by the `waiting_for_sync' argument.
	   [ARG: waiting_for_sync]
		 This argument is ignored unless `wait_queue' is non-NULL.  In
		 that event, a false `waiting_for_sync' argument means that the
		 caller is waiting for the completion of all outstanding jobs on
		 the `wait_queue', with the possible exception of jobs added in
		 the "secondary" service state (see `add_jobs'). A true
		 `waiting_for_sync' argument means that the caller is waiting for
		 a synchronization point to be reached, where the synchronization
		 point has already been installed in the `wait_queue' and the
		 current thread is identified as its `synchronizing_thread_idx'.
		 The function is used in this way only when invoked from within
		 the `synchronize' function.
	   [ARG: throw_group_failure]
		 Normally you will want this argument to be true, so that
		 unexpected failures cause an exception of type `kdu_exception' to
		 be thrown.  The internal machinery, however, sets this argument
		 to false in certain circumstances.
	*/
	virtual void handle_exception(kdu_exception exc_code);
	/* [SYNOPSIS]
		 As explained in the introductory comments, this function
		 is required to handle errors generated through `kdu_error'.
		 These errors either terminate the entire process, or else they
		 throw an exception of type `kdu_exception'.  In the former case,
		 no cleanup is required.  When an exception is caught, however, two
		 housekeeping operations are required: 1) any locks currently held
		 by the thread must be released so as to avoid deadlocking other
		 threads; 2) the thread group must be marked as having failed so
		 that the exception can be replicated in other threads within the
		 group when they next call `acquire_lock' or `process_jobs'.  The
		 `exc_code' supplied here will be used when replicating
		 the exception in other threads.
	*/
	void acquire_lock(int lock_id, bool allow_exceptions = true)
	{
		kd_thread_lock *lock = locks + lock_id;
		assert((lock_id >= 0) && (lock_id < num_locks) && (lock->holder != this));
		if (allow_exceptions && grouperr->failed)
		{
			if (grouperr->failure_code == KDU_MEMORY_EXCEPTION)
				throw std::bad_alloc();
			else
				throw grouperr->failure_code;
		}
		lock->mutex.lock(); lock->holder = this;
	}
	/* [SYNOPSIS]
		 This function is used by the `kdu_codestream' interface and its
		 descendants.  You will not normally invoke it directly from an
		 application.
		 [//]
		 The mutexes which are locked by this function are fast,
		 non-recursive mutexes.  You must not try to acquire the same
		 lock a second time, without first releasing it.
		 [//]
		 Note that this function may throw an exception of type
		 `kdu_exception' or `std::bad_alloc' (converted from
		 `KDU_MEMORY_EXCEPTION'), if any other thread in the
		 group terminates unexpectedly, invoking its `handle_exception'
		 function.  To avoid this (e.g., when performing cleanup operations
		 after an exception has already been thrown), set the
		 `allow_exceptions' argument to false.
	   [ARG: lock_id]
		 Must be in the range 0 to L-1, where L is the value returned by
		 `get_num_locks'.
	   [ARG: allow_exceptions]
		 If true, an exception will be thrown if the thread group is found
		 to have failed somewhere.  This behaviour ensures that all
		 participating threads in the group will eventually throw
		 exceptions.
	*/
	bool try_lock(int lock_id, bool allow_exceptions = true)
	{
		kd_thread_lock *lock = locks + lock_id;
		assert((lock_id >= 0) && (lock_id < num_locks) && (lock->holder != this));
		if (allow_exceptions && grouperr->failed)
		{
			if (grouperr->failure_code == KDU_MEMORY_EXCEPTION)
				throw std::bad_alloc();
			else
				throw grouperr->failure_code;
		}
		if (!lock->mutex.try_lock()) return false;
		lock->holder = this;  return true;
	}
	/* [SYNOPSIS]
		 Same as `acquire_lock', except that the function does not block.
		 If the mutex is already locked by another thread, the function
		 returns false immediately.
	*/
	void release_lock(int lock_id)
	{
		kd_thread_lock *lock = locks + lock_id;
		assert((lock_id >= 0) && (lock_id < num_locks) && (lock->holder == this));
		lock->holder = NULL;  lock->mutex.unlock();
	}
	/* [SYNOPSIS]
		 This function is used by the `kdu_codestream' interface and its
		 descendants.  You will not normally invoke it directly from an
		 application.
	*/
protected:
	virtual void
		do_sync(bool exception_handled) { return; }
	/* [SYNOPSIS]
		 This function is called under any of the following conditions:
		 [>>] When a thread is about to go idle.
		 [>>] When a thread is scheduled to work in a different queue to
			  the one in which it has just been working, except where it
			  moves between two leaf queues with a common parent, or
			  between a leaf queue and its parent.
		 [>>] When a thread finishes a job which lies at or before
			  the synchronization threshold associated with some queue
			  and is about to be assigned to a job which lies beyond
			  the synchronization threshold of that or any other queue --
			  see `synchonize' and `register_synchronized_job' for a
			  discussion of synchronization thresholds.
		 [>>] Before `synchronize' returns, the calling thread's
			  `need_sync' function is called.
	   [ARG: exception_handled]
		 If true, one or more threads in the working group have already
		 invoked their `handle_exception' function.  This may affect
		 the amount of work which the `do_sync' function chooses to do.
		 Also, if cleaning up requires that a lock be acquired, you will
		 probably want to do so in a manner which does not cause further
		 exceptions to be thrown -- see `acquire_lock'.
	*/
	virtual bool need_sync() { return true; }
	/* [SYNOPSIS]
		 This function may or may not be called prior to `do_sync' to
		 determine whether `do_sync' would do any work.  The reason for
		 this is that `do_sync' must be called with the group mutex
		 unlocked, so that it can do a substantial amount of work if
		 need be and so that it can itself call `acquire_lock' and
		 `release_lock', if required.
		 [//]
		 The present function, however, is called with the internal
		 group mutex locked.  It must not do any significant amount
		 of work and must not acquire any locks, but it can save
		 the caller from having to unlock the shared group mutex
		 and re-lock it, in the event that `do_sync' would not do
		 anything.
		 [//]
		 If in doubt, the safest thing is to have this function
		 return true, so that `do_sync' is called whenever it would
		 otherwise need to be called.  This is what the default
		 implementation does.
	*/
	virtual void
		on_finished(bool exception_handled) { return; }
	/* [SYNOPSIS]
		 This function is invoked on each thread within the group after
		 a call to `terminate' which deletes all queues in the thread
		 group.  If the thread group is already empty, in the sense of
		 having no queues, when `terminate' is called, the caller's
		 `on_finished' function will be called for good measure.
	   [ARG: exception_handled]
		 If true, one or more threads in the working group have already
		 invoked their `handle_exception' function.  This may affect
		 the amount of work which the `do_sync' function chooses to do.
		 Also, if cleaning up requires that a lock be acquired, you will
		 probably want to do so in a manner which does not cause further
		 exceptions to be thrown -- see `acquire_lock'.
	*/
private: // Helper functions
	void wake_idle_thread(kdu_thread_queue *run_queue);
	/* This function is called from `add_jobs' if it is determined that
	   there is now more work to do and an idle thread is available to do
	   that work.  The function finds the most appropriate idle thread
	   to do the work and wakes it up after setting up after prescheduling
	   it to work on a runnable job in the `run_queue' -- this argument
	   must be non-NULL.  The function should be called while holding the
	   group mutex. */
	bool process_outstanding_sync_points(kdu_thread_queue *queue);
	/* This function scans any outstanding `sync_points' on the `queue'
	   to determine whether or not they are currently satisfied.  If so,
	   the function processes any synchronized job which is required and
	   propagates conditions vertically to parents as required.  To do
	   this in a comprehensive manner, the function relies upon recursion.
	   The function does not process past synchronization points which
	   have associated synchronizing threads (non-negative
	   `synchronizing_thread_idx'); however, it does set these threads'
	   events, if they need to be woken up.  It also does not process
	   past synchronization points which have a synchronized job in
	   progress -- these synchronization points are being processed by
	   another thread which is also in the process of calling this
	   function.  Returns true if any synchronization point was removed.
	   This function should be called while holding the group mutex. */
private: // Data
	friend kdu_thread_startproc_result
		KDU_THREAD_STARTPROC_CALL_CONVENTION worker_startproc(void *);
	int thread_idx; // Index within `group->threads'
	kdu_thread thread;
	kd_thread_group *group; // Created by the owning thread entity.
	kd_thread_grouperr *grouperr; // Access to group failure conditions
	int num_locks; // Copy of the value stored in `group'
	kd_thread_lock *locks; // Pointer to the array stored in `group'
	kdu_thread_queue *recent_queue; // Last queue processed before going idle
	bool finished; // Set when `on_finished' is called
};

#endif // KDU_THREADS_H
