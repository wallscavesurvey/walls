/*****************************************************************************/
// File: threads_local.h [scope = CORESYS/THREADS]
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
   Provides local definitions common to both the DWT analysis and the DWT
synthesis implementations in "analysis.cpp" and "synthesis.cpp".
******************************************************************************/

#ifndef THREADS_LOCAL_H
#define THREADS_LOCAL_H

#include <assert.h>
#include "kdu_threads.h"
#include "kdu_arch.h"

// Objects defined here:
struct kd_thread_sync_point;
struct kd_thread_queue_alloc;
struct kdu_thread_queue;
struct kd_thread_group;

/*****************************************************************************/
/*                           kd_thread_queue_alloc                           */
/*****************************************************************************/

struct kd_thread_queue_alloc {
  // Used to allocate blocks of aligned thread queues.
    kd_thread_queue_alloc *next;
    kdu_byte block[1]; // Actually allocated much larger
  };

/*****************************************************************************/
/*                           kd_thread_sync_point                            */
/*****************************************************************************/

struct kd_thread_sync_point {
    int synchronization_threshold; // See `kdu_thread_queue'
    int synchronization_downcounter; // See `kdu_thread_queue'
    int num_unsynchronized_children; // See `kdu_thread_queue'
    int synchronizing_thread_idx; // -ve if no thread waiting in `synchronize'
    kdu_worker *synchronized_worker; // NULL if no synchronized job registered
    bool synchronized_job_deferred; // True if job was registered as deferred
    bool synchronized_job_in_progress; // See `kdu_thread_queue'
    bool finalize_children; // See `kdu_thread_queue'
    bool finalize_current; // See `kdu_thread_queue'
  };
  /* The members in this structure are explained in the comments which
     follow the `kdu_thread_queue' structure below. */

/*****************************************************************************/
/*                             kdu_thread_queue                              */
/*****************************************************************************/

struct kdu_thread_queue {
  public: // Member functions
    bool check_condition(bool waiting_for_sync, int thread_idx)
      { // Returns true if a condition on which the indicated `thread' is
        // waiting has been achieved.  This is either a synchronization point,
        // installed in the current queue, or completion of all outstanding
        // (non-secondary) jobs on the queue.
        if (waiting_for_sync)
          {
            assert(num_sync_points > 0);
            return (sync_points[0].synchronizing_thread_idx == thread_idx) &&
                   (sync_points[0].synchronization_downcounter == 0) &&
                   (sync_points[0].num_unsynchronized_children == 0);
          }
        assert(num_primary_jobs == 0);
        return ((num_runnable_jobs+num_active_jobs) == 0);
      }
    kdu_thread_queue *find_unassigned_job(int thread_idx);
      /* This potentially recursive function looks for a job inside the
         current queue (or any of its descendants) which has an unassigned
         job.  If one cannot be found, it works up toward the root of the
         queue hierarchy looking for queues with unassigned jobs.  The
         search stops when the function reaches the root, or when it reaches
         a queue on which the identified thread is trying to synchronize,
         as identified by the `synchronizing_thread_idx' member of any
         active entry in the `sync_points' array.  If nothing is found,
         the function returns NULL.  The function is executed while the
         group mutex is locked. */
    void make_subtree_jobs_runnable();
      /* Makes all unassigned jobs in this queue and all of its descendants
         runnable. */
    void make_primary_jobs_runnable();
      /* Same as `make_subtree_jobs_runnable', except only those jobs within
         the sub-tree which are marked as primary are promoted to the
         runnable state. */
    kdu_thread_queue *make_secondary_job_runnable();
      /* This function selects exactly one job from within the current
         sub-tree to make runnable and returns a pointer to the corresponding
         queue.  It is called from `find_unassigned_job' if no runnable or
         primary jobs can be found, but there exists at least one secondary
         unassigned job in the sub-tree.  The function uses and updates the
         `subtree_secondary_pref' member to locate a queue whose
         secondary jobs have the highest preference (although selection of
         the queue with absolute maximum preference is not always
         guaranteed). */
    void finalize(kd_thread_group *group);
      /* This function is called once we can be sure that no more jobs will
         be added to a queue.  If this leaves the number of working queues
         smaller than the number of threads in the system, the system is
         deemed to be under-utilized and so `group->activate_dormant_queues'
         is called to bring more work into focus. */
    bool install_synchronization_point(kdu_worker *job, bool run_deferred,
                                       int thread_idx, bool finalize_children,
                                       bool finalize_current,
                                       kd_thread_group *group);
      /* This function is only called while the group mutex is locked.
         If `job' is non-NULL it installs a synchronized job into the
         indicated queue, as a deferred or immediate job, depending on the
         `run_deferred' argument; otherwise, if `thread_idx' is non-negative,
         it installs a `synchronizing_thread_idx' into the indicated thread.
         At most one of the above conditions may apply.  If neither apply,
         the function is being used to install a subordinate condition into
         one of a queue's children.  If the queue has no outstanding jobs,
         the function is called recursively on each of the queue's children,
         setting the `num_unsynchronized_children' member as appropriate.
         Prior to returning, the function checks to see whether the
         `synchronization_downcounter' and `num_unsynchronized_children'
         values associated with the new synchronization point are both zero.
         If so, the synchronization point is uninstalled and the function
         returns false, meaning that the synchronization event on which
         we are waiting has occurred.  Otherwise, the function returns true,
         leaving the synchronization point installed.
            In the special case where the object on which this function is
         executed happens to be the root of the queue hierarchy, the
         domant queues found in `group' are also considered as descendants
         of the root object for the purpose of installing synchronization
         conditions. */
    void handle_exception(int calling_thread_idx);
      /* This function is invoked from `kdu_thread_entity::handle_exception'.
         It passes recursively through the queue hierarchy, doing the
         following:
         1) Remove any unassigned or active jobs.
         2) Remove any prescheduled jobs.
         3) Remove any synchronized workers from all job queues.
         4) Remove all synchronization points except for those which have a
            valid `synchronizing_thread_idx' value, which is not equal to
            `calling_thread_idx' -- for these queues, the synchronization
            condition is set to have occurred.
         5) Signal all waiting threads, as identified by valid
            `thread_awaiting_sync' and `thread_awaiting_complete' members which
            are encountered.
         Note that the group mutex is always locked while this function is
         called. */
  //--------------------------------------------------------------------------
  public: // Links and identification
    int depth; // Depth in the queue hierarchy
    const char *name; // Textual description used for debugging -- may be NULL
    kdu_long bank_idx; // See `kdu_thread_entity::add_queue'
    kd_thread_group *group; // NULL if the queue is on the free list
    kdu_thread_queue *parent; // Super-queue, if any -- else NULL
    kdu_thread_queue *sibling_next; // Doubly-linked list of queues with the
    kdu_thread_queue *sibling_prev; // same parent.
    kdu_thread_queue *children; // Head of child list
    kdu_thread_queue *free_next;
  //--------------------------------------------------------------------------
  public: // Members used to manage jobs in this queue
    kdu_worker *worker;
    kdu_uint32 secondary_pref; // See below
    int num_active_jobs; // Num jobs currently being worked on by threads
    int num_unassigned_jobs; // Num jobs waiting to be assigned to threads
    int num_primary_jobs; // See below
    int num_runnable_jobs; // See below
    int first_unassigned_job_idx; // Index of next job to be assigned
    int max_jobs_in_queue; // 0 unless we know the limit
    int prescheduled_job_idx; // -ve except if `kdu_thread_entity::add_jobs'
       // has scheduled a job to be run by a specific thread, writing a
       // pointer to this queue into the relevant entry in the
       // `kd_thread_group::thread_activity' array for the thread to find
       // when it wakes up from being idle.  Prescheduled jobs are treated
       // as assigned and active.
  //--------------------------------------------------------------------------
  public: // Members used to manage jobs in sub-queues
    int subtree_unassigned_jobs; // See below
    int subtree_primary_jobs; // See below
    int subtree_runnable_jobs; // See below
    kdu_uint32 subtree_secondary_pref; // See below
    int subtree_working_leaves; // See below
  //--------------------------------------------------------------------------
  public: // Members used for synchronization
    int num_sync_points;
    kd_thread_sync_point sync_points[KDU_MAX_SYNC_NESTING];
  //--------------------------------------------------------------------------
  public: // Other members used to implement `kdu_thread_entity::get_job'
    int thread_awaiting_sync; // -ve if no thread is blocked on a sync point
    int thread_awaiting_complete; // -ve if no thread is blocked on the
                        // completion of all outstanding jobs in this queue.
  };
  /* Notes:
       This object manages the state of a single queue, as well as its
     descendants.  We now explain some of the less obvious members:
       * `num_unassigned_jobs' identifies the total number of jobs in this
     queue which are still waiting to be assigned to threads.  Of these,
     `num_runnable_jobs' identifies the total number of jobs which are
     ready to be run right away, while `num_primary_jobs' tells us the
     number of jobs which are in the primary state.  The number of jobs
     which are in the secondary state is given by `num_unassigned_jobs' -
     `num_runnable_jobs' - `num_primary_jobs'.  The runnable, primary and
     secondary states are explained in connection with `kdu_thread::add_jobs'.
       * `subtree_unassigned_jobs', `subtree_runnable_jobs' and
     `subtree_primary_jobs' hold totals for the `num_unassigned_jobs',
     `num_runnable_jobs' and `num_primary_jobs' members of all queues which
     belong to the sub-tree anchored at this queue.  For leaf queues,
     therefore, `subtree_unassigned_jobs'=`num_unassigned_jobs' and so forth.
       * `secondary_pref' should be 0 unless the queue has jobs in the
     "secondary" service state, in which case it holds 2^32 minus the
     `secondary_seq' argument supplied to `kdu_thread_entity::add_jobs'.
       * `subtree_secondary_pref' holds a fairly tight upper bound on the
     `secondary_pref' fields found in the queue's sub-tree.  If
     there are any jobs in the secondary service state within this queue or
     any of its descendants, therefore, the value of this member should
     be non-zero.  The value of this member is affected by calls to
     `kdu_thread_entity::add_jobs' which add secondary jobs, and by calls
     to `kdu_thread_queue::find_unassigned_job' which promote a secondary
     job.  This latter role is actually performed by calls to the
     `make_secondary_job_runnable' function.
       * `subtree_working_leaves' holds the total number of working leaf
     queues which are descended from the current queue, not including itself.
     Thus, leaf queues always have a value of 0 in this field.  A working
     leaf queue is one whose `worker' member is non-NULL, which has no
     descendants with a non-NULL `worker' member.  That is, a working leaf
     is any queue which has its `subtree_working_leaves' and `worker' members
     equal to 0 and non-NULL, respectively.  Once the last job for a queue has
     been executed, as determined by information supplied to
     `kdu_thread_entity::add_jobs', its `worker' member is set to NULL and
     the `subtree_working_leaves' member is adjusted in each of its ancestors.
     This allows the determination of system under-utilization, in which
     case dormant queues may be added to the `kd_thread_group::queue_base'
     object, which manages all schedulable queues.
       * `num_sync_points' identifies the number of outstanding
     synchronization points in the `sync_points' array.  The first of these
     is the one which will fall due first.
       * Elements of the `sync_points' array have the following interpretation:
         -- `synchronization_threshold' is set to the index of the next job
            to be added, at the time when synchronization point was installed.
         -- `synchronization_downcounter' is initialized to the sum of the
            `num_active_jobs' and `num_unassigned_jobs' members, at the time
            the synchronization point was installed.  Each time a job
            completes, the complete list of `sync_points' is scanned,
            and each element's `synchronization_downcounter' member is
            decremented if the completing job number is less than the
            corresponding `synchronization_threshold'.
            Once this member reaches 0, the queue is considered synchronized,
            but its children might not be synchronized yet.  When this
            happens, a synchronization point is installed in each child, and
            `num_unsynchronized_children' is set accordingly -- see below.
         -- `num_unsynchronized_children' indicates the number of immediate
            sub-queues of the current queue whose first `sync_points' entry
            is active and has no `synchronized_worker' or
            non-negative `synchronizing_thread_idx' of its own.  The
            `num_unsynchronized_children' member is valid only in the first
            element of the `sync_queues' array.  Once it becomes 0 and the
            `synchronization_downcounter' member also becomes 0, the
            synchronization point is deemed to have been reached.  At that
            point, any `synchronized_worker' is run, then any non-negative
            `synchronizing_thread_idx' member is used to wake up a thread
            which is waiting for synchronization.  If there is no synchronized
            worker and no synchronizing thread, the parent queue's
            `num_unsynchronized_children' member is decremented and the
            testing for synchronization points continues recursively from
            that queue.  In either case, the synchronization point is removed
            and the next one, if any, is moved to the front of the
            `sync_points' array.
         -- `synchronized_worker' is non-NULL if a special job is to be
            run when the synchronization point is reached.  The job will be
            returned by the `get_job' call which finds the synchronization
            point to be reached.
         -- `synchronizing_thread_idx' is non-negative if the corresponding
            thread is waiting inside `kdu_thread_entity::synchronize' for
            this synchronization condition to occur.  This thread's event will
            be signalled once the `synchronization_downcounter' and
            `num_unsynchronized_children' members both reach 0.  The thread
            which is sitting inside `synchronize' is responsible for clearing
            the synchronization point and advancing to the next one in the
            `sync_points' array, if any.
         -- `finalize_current' is set to true if we are waiting to terminate
            the current queue.  As soon as the synchronization condition
            propagates to the head of the `sync_points' array and the
            `synchronization_downcounter' reaches 0, the queue will be
            marked as finalized, meaning that no new jobs can be added.
            This allows system under-utilization to be recognized as
            quickly as possible, so that dormant queues can be activated
            in a timely manner.
         -- `finalize_children' plays the same role as `finalize_current'
            except that it refers to our intent to finalize the queue's
            children once the synchronization condition propagates to
            the head of their respective `sync_points' array and their
            respective `synchronization_downcounter' values reach 0.
       * `thread_awaiting_sync' and `thread_awaiting_complete' hold the indices
     of threads whose event may need to be set when a job completes on the
     queue.  The former is used for threads which have written their index
     into the `synchronizing_thread_idx' member of an entry in the
     `sync_points' array.  The latter is used for threads which are waiting
     for the completion of all jobs in the current queue, without any
     associated synchronization point.
  */

/*****************************************************************************/
/*                              kd_thread_group                              */
/*****************************************************************************/

#define KD_THREAD_ACTIVE ((kdu_thread_queue *) _kdu_long_to_addr(1))
   // Used to mark entries in the `kd_thread_group::thread_activity' array
   // which correspond to currently active threads.  See comments associated
   // with that array for more information.

struct kd_thread_group {
  public: // Data
    kd_thread_group()
      {
        cpu_affinity = 0;
        free_queues = NULL;  queue_blocks = NULL;  num_deferred_jobs = 0;
        num_threads = num_idle_threads = num_finished_threads = 0;
        memset(&queue_base,0,sizeof(queue_base));
        queue_base.name = "ROOT";
        queue_base.group = this;
        queue_base.prescheduled_job_idx=-1;
        queue_base.thread_awaiting_sync=queue_base.thread_awaiting_complete=-1;
        dormant_head = dormant_tail = NULL;  min_dormant_bank_idx = 1;
        grouperr.failed=false;  grouperr.failure_code=KDU_NULL_EXCEPTION;
        finish_requested = destruction_requested = false;
        locks=NULL;  num_locks=0;
      }
    ~kd_thread_group()
      { // Only cleans up private resources
        kd_thread_queue_alloc *tmp;
        while ((tmp=queue_blocks) != NULL)
          { queue_blocks=tmp->next; free(tmp); }
      }
    kdu_thread_queue *get_queue();
      /* Allocates a new queue, with its members initialized.  This function
         should be called only while holding the group lock, since queues
         may be added or released from multiple threads. */
    void release_queues(kdu_thread_queue *root, bool leave_root);
      /* Recycles all descendants of the `root' queue back to the list of
         free queues.  If `leave_root' is false, the `root' queue is also
         recycled; in this case, it is presumed to have been unlinked from
         any sibling list already.  This function should be called only
         while holding the group lock, since queues may be added or
         released from multiple threads. */
    void activate_dormant_queues();
      /* This function is called when the system is under-utilized.  It
         moves queues from the dormant list until the number of working
         queues managed by `kd_thread_group::queue_base' is at least as
         large as the number of threads in the system.  The function
         should be called while holding the group mutex.  If new jobs
         become available for processing, the function also wakes up
         any idle threads. */
  public: // Padding
    kdu_byte _leadin[KDU_MAX_L2_CACHE_LINE];
  public: // Thread/queue state
    kdu_long cpu_affinity; // Provided by `kdu_thread_entity::create'
    int num_threads;
    kdu_thread_entity *threads[KDU_MAX_THREADS]; // 1'st is the group owner
    int num_idle_threads; // Owner is never entered as idle
    kdu_thread_queue *thread_activity[KDU_MAX_THREADS]; // See below
    int num_finished_threads; // Num threads which have called `on_finished'
    kdu_thread_queue queue_base;
    int num_deferred_jobs;
    kdu_worker *deferred_jobs[KDU_MAX_THREADS]; // A reasonable limit
    kdu_thread_queue *dormant_head, *dormant_tail; // See below
    kdu_long min_dormant_bank_idx; // See below
  public: // Communication between threads
    kd_thread_grouperr grouperr;
    bool finish_requested; // Requesting threads to call `on_finished'.
    bool destruction_requested; // Requesting worker threads to exit.
  public: // Synchronization objects
    kdu_mutex mutex; // Guards access to all thread and queue members
    kdu_event thread_events[KDU_MAX_THREADS]; // One for each thread
    int num_locks;
    kd_thread_lock *locks;
    kd_thread_lock lock_store[8]; // Use these unless we need more
  private: // Resources
    kdu_thread_queue *free_queues;
    kd_thread_queue_alloc *queue_blocks;
  public: // Padding
    kdu_byte _trailer[KDU_MAX_L2_CACHE_LINE];
  };
  /* NOTES:
        The `thread_activity' array contains `num_threads' valid entries,
     holding one of three different types of values, as follows.  If
     `thread_activity'[n] = NULL, the n'th thread is idle and is included
     in the `num_idle_threads' count.  If `thread_activity'[n] =
     `KD_THREAD_ACTIVE' (an impossible pointer), the n'th thread is active.
     Otherwise, `thread_activity'[n] points to a queue which contains one
     or more newly added jobs that have been scheduled to run on the n'th
     thread by the `kd_thread_entity::add_jobs' function.  In this case,
     the n'th thread was idle; it is expected to wake from its idle state
     and run jobs from the indicated queue, converting the
     `thread_activity'[n] entry to `KD_THREAD_ACTIVE'.  The corresponding
     queue's `kdu_thread_queue::prescheduled_job_idx' member identifies the
     specific job which has been scheduled to run on the n'th thread.
        The `dormant_head' and `dormant_tail' members point to the head and
     tail of a sibling-linked list of queues whose bank index is
     greater than or equal to the `min_dormant_bank_idx' argument.
     Some or all of these will be moved to the `queue_base' container
     when the system next becomes under-utilized, as defined in connection
     with the `kdu_thread_entity::add_queue' function.
  */

#endif // THREADS_LOCAL_H
