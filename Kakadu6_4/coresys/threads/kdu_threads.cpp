/*****************************************************************************/
// File: kdu_threads.cpp [scope = CORESYS/THREADS]
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
   Implements the multi-threaded architecture described in "kdu_threads.h".
******************************************************************************/

#include <stdlib.h>
#include "threads_local.h"

/* ========================================================================= */
/*                            INTERNAL FUNCTIONS                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                      kd_set_threadname                             */
/*****************************************************************************/

#if (defined _WIN32 && defined _DEBUG && defined _MSC_VER && (_MSC_VER>=1300))
static void
  kd_set_threadname(LPCSTR thread_name)
{
  struct {
      DWORD dwType; // must be 0x1000
      LPCSTR szName; // pointer to name (in user addr space)
      DWORD dwThreadID; // thread ID (-1=caller thread)
      DWORD dwFlags; // reserved for future use, must be zero
    } info;
  info.dwType = 0x1000;
  info.szName = thread_name;
  info.dwThreadID = -1;
  info.dwFlags = 0;
  __try {
      RaiseException(0x406D1388,0,sizeof(info)/sizeof(DWORD),
                     (ULONG_PTR *) &info );
    }
  __except(EXCEPTION_CONTINUE_EXECUTION) { }
}
#endif // .NET debug compilation

/*****************************************************************************/
/*                             worker_startproc                              */
/*****************************************************************************/

kdu_thread_startproc_result
  KDU_THREAD_STARTPROC_CALL_CONVENTION worker_startproc(void *param)
{
  kdu_thread_entity *ent = (kdu_thread_entity *) param;
#if (defined _WIN32 && defined _DEBUG && defined _MSC_VER && (_MSC_VER>=1300))
  kd_set_threadname("Kakadu Worker Thread");
#endif // .NET debug compilation
  if (ent->group->cpu_affinity != 0)
    ent->thread.set_cpu_affinity(ent->group->cpu_affinity);
  bool result = ent->process_jobs(NULL,false,false);
  if (result)
    assert(0);
  ent->pre_destroy();
  return KDU_THREAD_STARTPROC_ZERO_RESULT;
}


/* ========================================================================= */
/*                              kd_thread_group                              */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_thread_group::get_queue                         */
/*****************************************************************************/

kdu_thread_queue *
  kd_thread_group::get_queue()
{
  if (free_queues == NULL)
    {
      int mask = KDU_MAX_L2_CACHE_LINE-1;
      assert(!(mask & (mask+1))); // KDU_MAX_L2_CACHE_LINE must be power of 2!
      int queue_size = sizeof(kd_thread_group);
      int aligned_queue_size=KDU_MAX_L2_CACHE_LINE;
      while (aligned_queue_size >= 2*queue_size)
        aligned_queue_size <<= 1;
      if (aligned_queue_size < queue_size)
        aligned_queue_size *= 1+((queue_size-1)/aligned_queue_size);
      assert((aligned_queue_size >= queue_size) &&
             (aligned_queue_size < 2*queue_size));
      int block_bytes = mask + aligned_queue_size*32;
      kd_thread_queue_alloc *alloc = (kd_thread_queue_alloc *)
        malloc(sizeof(kd_thread_queue_alloc *)+(size_t) block_bytes);
      if (alloc == NULL)
        throw std::bad_alloc();
      alloc->next = queue_blocks;
      queue_blocks = alloc;
      kdu_byte *block = alloc->block;
      memset(block,0,(size_t) block_bytes);
      int offset = (- _addr_to_kdu_int32(block)) & mask;
      block += offset;
      block_bytes -= offset;
      for (; block_bytes >= aligned_queue_size;
           block+=aligned_queue_size, block_bytes-=aligned_queue_size)
        {
          kdu_thread_queue *elt = (kdu_thread_queue *) block;
          elt->free_next = free_queues;
          free_queues = elt;
        }
    }
  kdu_thread_queue *result = free_queues;
  free_queues = result->free_next;
  result->free_next = NULL;
  result->group = this;
  result->thread_awaiting_sync = -1;
  result->thread_awaiting_complete = -1;
  result->prescheduled_job_idx = -1;
  return result;
}

/*****************************************************************************/
/*                     kd_thread_group::release_queues                       */
/*****************************************************************************/

void
  kd_thread_group::release_queues(kdu_thread_queue *root, bool leave_root)
{
  kdu_thread_queue *child;
  while ((child=root->children) != NULL)
    { // Recursively release children
      root->children = child->sibling_next;
      release_queues(child,false);
    }
  if (!leave_root)
    {
      memset(root,0,sizeof(kdu_thread_queue));
      root->free_next = free_queues;
      free_queues = root;
    }
}

/*****************************************************************************/
/*                 kd_thread_group::activate_dormant_queues                  */
/*****************************************************************************/

void
  kd_thread_group::activate_dormant_queues()
{
  int num_new_jobs = 0;
  while ((dormant_head != NULL) &&
         ((queue_base.subtree_working_leaves < num_threads) ||
          (dormant_head->bank_idx < min_dormant_bank_idx)))
    {
      kdu_thread_queue *queue = dormant_head;
      min_dormant_bank_idx = queue->bank_idx+1;
      if ((dormant_head = queue->sibling_next) == NULL)
        dormant_tail = NULL;
      else
        dormant_head->sibling_prev = NULL;
      queue->sibling_prev = NULL;
      if ((queue->sibling_next = queue_base.children) != NULL)
        queue->sibling_next->sibling_prev = queue;
      queue->parent = &queue_base;
      queue_base.children = queue;
      queue_base.subtree_unassigned_jobs += queue->subtree_unassigned_jobs;
      queue_base.subtree_primary_jobs += queue->subtree_primary_jobs;
      queue_base.subtree_runnable_jobs += queue->subtree_runnable_jobs;
      if (queue_base.subtree_secondary_pref < queue->subtree_secondary_pref)
        queue_base.subtree_secondary_pref = queue->subtree_secondary_pref;
      if (queue->subtree_working_leaves > 0)
        queue_base.subtree_working_leaves += queue->subtree_working_leaves;
      else if (queue->worker != NULL)
        queue_base.subtree_working_leaves++;
      num_new_jobs += queue->subtree_unassigned_jobs;
    }
  if (num_idle_threads > 0)
    { // Wake up idle threads to run newly available jobs
      for (int n=0; (n < num_threads) && (num_new_jobs > 0); n++)
        if (thread_activity[n] == NULL)
          {
            thread_events[n].set();
            num_new_jobs--;
          }
    }
}


/* ========================================================================= */
/*                             kdu_thread_queue                              */
/* ========================================================================= */

/*****************************************************************************/
/*                   kdu_thread_queue::find_unassigned_job                   */
/*****************************************************************************/

kdu_thread_queue *
  kdu_thread_queue::find_unassigned_job(int thread_idx)
{
  kdu_thread_queue *scan, *next=NULL, *best_secondary=NULL;
  kdu_uint32 best_secondary_pref=0;
  for (scan=this; (scan->subtree_runnable_jobs == 0); scan=next)
    {
      if (scan->subtree_secondary_pref > best_secondary_pref)
        {
          best_secondary = scan;
          best_secondary_pref = scan->subtree_secondary_pref;
        }
      int n;
      for (n=0; n < scan->num_sync_points; n++)
        if (scan->sync_points[n].synchronizing_thread_idx == thread_idx)
          break;
      next = scan->parent;
      if ((n < scan->num_sync_points) || (next == NULL))
        { // Can't go above this point and haven't found any runnable jobs
          if (scan->subtree_primary_jobs > 0)
            { // Make all primary jobs in the tree runnable and scan again.
              scan->make_primary_jobs_runnable();
              next = this;
            }
          else if (best_secondary != NULL)
            {
              assert(best_secondary->subtree_unassigned_jobs > 0);
              return best_secondary->make_secondary_job_runnable();
            }
          else
            {
              assert(scan->subtree_unassigned_jobs == 0);
              return NULL; // No unassigned jobs available anywhere
            }
        }
    }

  while (scan->subtree_runnable_jobs > scan->num_runnable_jobs)
    { // Push down into the descendants as far as we can.
      kdu_thread_queue *child, *first_runnable_child=NULL;
      for (child = scan->children; child != NULL; child=child->sibling_next)
        if (child->subtree_runnable_jobs > 0)
          {
            first_runnable_child = child; // We can always follow this child
            if (child->num_active_jobs == 0)
              break; // Immediately follow this child if it has no active jobs;
                     // this helps to keep threads working in disjoint queues
                     // where possible.
          }
      assert(first_runnable_child != NULL);
      scan = (child==NULL)?first_runnable_child:child;
    }
  return scan;
}

/*****************************************************************************/
/*                kdu_thread_queue::make_subtree_jobs_runnable               */
/*****************************************************************************/

void
  kdu_thread_queue::make_subtree_jobs_runnable()
{
  if ((subtree_unassigned_jobs - subtree_runnable_jobs) >
      (num_unassigned_jobs - num_runnable_jobs))
    { // One or more children must have more unassigned than runnable jobs
      kdu_thread_queue *child;
      for (child=children; child != NULL; child=child->sibling_next)
        if (child->subtree_unassigned_jobs > child->subtree_runnable_jobs)
          child->make_subtree_jobs_runnable();
    }
  int n = num_unassigned_jobs - num_runnable_jobs;
  if (n > 0)
    {
      int p = num_primary_jobs;
      kdu_thread_queue *qscn;
      num_runnable_jobs += n;
      num_primary_jobs = 0;
      secondary_pref = 0;
      for (qscn=this; qscn != NULL; qscn=qscn->parent)
        {
          qscn->subtree_runnable_jobs += n;
          qscn->subtree_primary_jobs -= p;
          if ((qscn->subtree_runnable_jobs+qscn->subtree_primary_jobs) ==
              qscn->subtree_unassigned_jobs)
            qscn->subtree_secondary_pref = 0;
        }
    }
  assert((subtree_runnable_jobs == subtree_unassigned_jobs) &&
         (num_runnable_jobs == num_unassigned_jobs) &&
         (subtree_primary_jobs == 0));
}

/*****************************************************************************/
/*                kdu_thread_queue::make_primary_jobs_runnable               */
/*****************************************************************************/

void
  kdu_thread_queue::make_primary_jobs_runnable()
{
  if (subtree_primary_jobs > num_primary_jobs)
    { // One or more children must have non-runnable primary jobs
      kdu_thread_queue *child;
      for (child=children; child != NULL; child=child->sibling_next)
        if (child->subtree_primary_jobs > 0)
          child->make_primary_jobs_runnable();
    }
  int n = num_primary_jobs;
  if (n > 0)
    {
      kdu_thread_queue *qscn;
      num_runnable_jobs += n;
      num_primary_jobs = 0;
      for (qscn=this; qscn != NULL; qscn=qscn->parent)
        {
          qscn->subtree_runnable_jobs += n;
          qscn->subtree_primary_jobs -= n;
        }
    }
  assert(subtree_primary_jobs == 0);
}

/*****************************************************************************/
/*               kdu_thread_queue::make_secondary_job_runnable               */
/*****************************************************************************/

kdu_thread_queue *
  kdu_thread_queue::make_secondary_job_runnable()
{
  kdu_thread_queue *scan;

  assert((subtree_unassigned_jobs > 0) && (subtree_secondary_pref != 0) &&
         (subtree_runnable_jobs == 0) && (subtree_primary_jobs == 0));
  if (subtree_unassigned_jobs == num_unassigned_jobs)
    { // This is the queue we want
      assert(secondary_pref != 0);
      num_runnable_jobs++;
      if (num_runnable_jobs == num_unassigned_jobs)
        secondary_pref = 0; // No more secondary jobs
      for (scan=this; scan != NULL; scan=scan->parent)
        {
          scan->subtree_runnable_jobs++;
          if ((scan->subtree_runnable_jobs+scan->subtree_primary_jobs) ==
              scan->subtree_unassigned_jobs)
            scan->subtree_secondary_pref = 0;
        }
      return this;
    }

  // If we get here, the queue we seek must be one of our children
  kdu_uint32 sp, best_pref=0, second_best_pref=0;
  kdu_thread_queue *best_child=NULL;
  for (scan=children; scan != NULL; scan=scan->sibling_next)
    {
      if ((sp = scan->subtree_secondary_pref) == 0)
        continue;
      if (scan->subtree_unassigned_jobs == 0)
        { // Secondary_pref values should all be 0.
          scan->subtree_secondary_pref = 0;
          continue;
        }
      assert((scan->subtree_runnable_jobs == 0) &&
             (scan->subtree_primary_jobs == 0) &&
             (scan->num_runnable_jobs == 0) && (scan->num_primary_jobs == 0));
      if ((sp > best_pref) || (best_child == NULL))
        {
          second_best_pref = best_pref;  best_pref = sp;
          best_child = scan;
        }
      else if (sp > second_best_pref)
        second_best_pref = sp;
    }
  assert(best_child != NULL);
  kdu_thread_queue *result = best_child->make_secondary_job_runnable();
  best_pref = best_child->subtree_secondary_pref;
  if (second_best_pref > best_pref)
    best_pref = second_best_pref;
  if (this->secondary_pref != 0)
    { // current node itself has secondary jobs; check them as well.
      assert(this->num_unassigned_jobs >
             (this->num_runnable_jobs+this->num_primary_jobs));
      if (this->secondary_pref > best_pref)
        best_pref = this->secondary_pref;
    }
  this->subtree_secondary_pref = best_pref;
  assert(result != NULL);
  return result;
}

/*****************************************************************************/
/*                        kdu_thread_queue::finalize                         */
/*****************************************************************************/

void
  kdu_thread_queue::finalize(kd_thread_group *group)
{
  if (worker == NULL)
    return;
  max_jobs_in_queue = first_unassigned_job_idx+num_unassigned_jobs;
  if ((num_active_jobs > 0) || (max_jobs_in_queue > first_unassigned_job_idx))
    return; // Queue is still active
  worker = NULL;
  if (subtree_working_leaves > 0)
    return; // Queue was not a working leaf
  for (kdu_thread_queue *scan=parent; scan != NULL; scan=scan->parent)
    {
      assert(scan->subtree_working_leaves > 0);
      scan->subtree_working_leaves--;
      if ((scan->subtree_working_leaves == 0) && (scan->worker != NULL))
        break; // Num working leaves along this branch is still exactly 1,
               // because `scan' is now a working leaf.
    }
  if ((group->dormant_head != NULL) &&
      (group->queue_base.subtree_working_leaves < group->num_threads))
    group->activate_dormant_queues();
}

/*****************************************************************************/
/*              kdu_thread_queue::install_synchronization_point              */
/*****************************************************************************/

bool
  kdu_thread_queue::install_synchronization_point(kdu_worker *job,
                                                  bool run_deferred,
                                                  int thread_idx,
                                                  bool finalize_children,
                                                  bool finalize_current,
                                                  kd_thread_group *group)
{
  if (subtree_runnable_jobs < subtree_unassigned_jobs)
    make_subtree_jobs_runnable();
  assert(num_sync_points < KDU_MAX_SYNC_NESTING);
  kd_thread_sync_point *sp = sync_points + (num_sync_points++);
  sp->num_unsynchronized_children = 0;
  sp->synchronization_threshold =
    first_unassigned_job_idx + num_unassigned_jobs;
  sp->synchronization_downcounter = num_active_jobs + num_unassigned_jobs;
  sp->synchronizing_thread_idx = thread_idx;
  sp->synchronized_worker = job;
  sp->synchronized_job_deferred = run_deferred;
  sp->synchronized_job_in_progress = false;
  sp->finalize_children = finalize_children;
  sp->finalize_current = finalize_current;
  if ((sp->synchronization_downcounter > 0) || (sp != sync_points))
    return true;

  if (finalize_current && (worker != NULL))
    this->finalize(group);

  kdu_thread_queue *child;
  for (child=children; child != NULL; child=child->sibling_next)
    if (child->install_synchronization_point(NULL,false,-1,
                       finalize_children,finalize_children,group))
      sp->num_unsynchronized_children++;
  if ((parent == NULL) && (this == &group->queue_base))
    { // Consider dormant queues as though they belonged to the root of the
      // queue hierarchy, for the purpose of installing synchronization
      // conditions.
      for (child=group->dormant_head; child != NULL; child=child->sibling_next)
        if (child->install_synchronization_point(NULL,false,-1,
                        finalize_children,finalize_children,group))
          sp->num_unsynchronized_children++;
    }
  if (sp->num_unsynchronized_children > 0)
    return true;

  // Synchronization condition already reached; uninstall condition
  num_sync_points = 0;
  return false;
}

/*****************************************************************************/
/*                   kdu_thread_queue::handle_exception                      */
/*****************************************************************************/

void
  kdu_thread_queue::handle_exception(int calling_thread_idx)
{
  for (kdu_thread_queue *child=children;
       child != NULL; child=child->sibling_next)
    child->handle_exception(calling_thread_idx);

  secondary_pref = 0;
  first_unassigned_job_idx += num_unassigned_jobs;
  num_unassigned_jobs = num_primary_jobs = num_runnable_jobs = 0;
  num_active_jobs = 0;
  prescheduled_job_idx = -1;
  max_jobs_in_queue = first_unassigned_job_idx;
  worker = NULL;
  subtree_secondary_pref = 0;
  subtree_working_leaves = 0;
  subtree_unassigned_jobs = 0;
  subtree_primary_jobs = 0;
  subtree_runnable_jobs = 0;

  int n, k;
  for (n=0; n < num_sync_points; n++)
    {
      kd_thread_sync_point *sp = sync_points + n;
      sp->num_unsynchronized_children = 0;
      sp->synchronization_downcounter = 0;
      sp->synchronized_worker = NULL;
      if ((sp->synchronizing_thread_idx < 0) ||
          (sp->synchronizing_thread_idx == calling_thread_idx))
        { // Completely remove this `sync_point'
          for (k=n+1; k < num_sync_points; k++)
            *sp = sync_points[k];
          n--;
          num_sync_points--;
        }
    }

  if (thread_awaiting_sync >= 0)
    {
      if (thread_awaiting_sync != calling_thread_idx)
        group->thread_events[thread_awaiting_sync].set();
      thread_awaiting_sync = -1;
    }
  if (thread_awaiting_complete >= 0)
    {
      if (thread_awaiting_complete != calling_thread_idx)
        group->thread_events[thread_awaiting_complete].set();
      thread_awaiting_complete = -1;
    }
}


/* ========================================================================= */
/*                             kdu_thread_entity                             */
/* ========================================================================= */

/*****************************************************************************/
/*                   kdu_thread_entity::kdu_thread_entity                    */
/*****************************************************************************/

kdu_thread_entity::kdu_thread_entity()
{
  thread_idx=0;
  group=NULL;
  locks=NULL;
  grouperr=NULL;
  num_locks=0;
  finished=false;
  recent_queue=NULL;
}

/*****************************************************************************/
/*                     kdu_thread_entity::operator new                       */
/*****************************************************************************/

void *
  kdu_thread_entity::operator new(size_t size)
{
  size += sizeof(kdu_byte *); // Leave room to store deallocation pointer
  int mask = KDU_MAX_L2_CACHE_LINE-1;
  assert(!(mask & (mask+1))); // KDU_MAX_L2_CACHE_LINE must be power of 2!
  int offset = (KDU_MAX_L2_CACHE_LINE - (int) size) & mask;
  size += offset; // Pad to nearest multiple of max L2 cache line size.
  size += KDU_MAX_L2_CACHE_LINE; // Allow for incorrectly aligned result
  kdu_byte *handle = (kdu_byte *) malloc(size);
  if (handle == NULL)
    throw std::bad_alloc();
  kdu_byte *base = handle + sizeof(kdu_byte *);
  offset = (- _addr_to_kdu_int32(base)) & mask;
  base += offset; // Aligned base of object
  ((kdu_byte **) base)[-1] = handle;
  return (void *) base;
}

/*****************************************************************************/
/*                   kdu_thread_entity::operator delete                      */
/*****************************************************************************/

void
  kdu_thread_entity::operator delete(void *base)
{
  kdu_byte *handle = ((kdu_byte **) base)[-1];
  free(handle);
}

/*****************************************************************************/
/*               kdu_thread_entity::get_current_thread_entity                */
/*****************************************************************************/

kdu_thread_entity *
  kdu_thread_entity::get_current_thread_entity()
{
  kdu_thread me;
  if (!me.set_to_self())
    return this; // No multi-threading.  Caller might as well use the same
                 // thread entity.
  if (group == NULL)
    return NULL; // `kdu_thread_entity::create' not yet called; we may be
                 // executing in a different thread from that which will create
                 // it, if anyone does.
  kdu_thread_entity *result = NULL;
  group->mutex.lock();
  for (int n=0; n < group->num_threads; n++)
    if (group->threads[n]->thread.equals(me))
      {
        result = group->threads[n];
        break;
      }
  group->mutex.unlock();
  return result;
}

/*****************************************************************************/
/*                         kdu_thread_entity::create                         */
/*****************************************************************************/

void
  kdu_thread_entity::create(kdu_long cpu_affinity)
{
  assert(!exists());
  thread_idx = 0;
  thread.set_to_self();
  num_locks = get_num_locks();
  group = new kd_thread_group;

  group->cpu_affinity = cpu_affinity;
  group->num_threads = 1;
  group->threads[0] = this;
  group->thread_events[0].create(false);
  group->thread_activity[0] = KD_THREAD_ACTIVE;
  group->mutex.create();
  group->num_locks = num_locks;
  if (num_locks < 8)
    group->locks = group->lock_store;
  else
    group->locks = new kd_thread_lock[num_locks];
  for (int n=0; n < num_locks; n++)
    {
      group->locks[n].holder = NULL;
      group->locks[n].mutex.create();
    }

  this->locks = group->locks;
  this->grouperr = &(group->grouperr);

  if (cpu_affinity != 0)
    thread.set_cpu_affinity(cpu_affinity);
}

/*****************************************************************************/
/*                        kdu_thread_entity::destroy                         */
/*****************************************************************************/

bool
  kdu_thread_entity::destroy()
{
  if (!exists())
    return true;
  assert(is_group_owner());

  int n;
  bool result = !grouperr->failed;
  handle_exception(-1); // Forces threads to terminate as soon as possible.
  terminate(NULL,false,NULL); // Wait until all work is done and delete queues.
  assert(group->queue_base.children == NULL);
  group->mutex.lock();
  group->destruction_requested = true;
  for (n=0; n < group->num_threads; n++)
    group->thread_events[n].set();
  group->mutex.unlock();
  for (n=1; n < group->num_threads; n++)
    group->threads[n]->thread.destroy(); // Waits for the thread to exit
  for (n=0; n < group->num_threads; n++)
    {
      if (n > 0)
        delete group->threads[n];
      group->threads[n] = NULL;
      group->thread_events[n].destroy();
    }
  group->num_threads = group->num_idle_threads = 0;
  for (n=0; n < group->num_locks; n++)
    {
      assert(group->locks[n].holder == NULL);
      group->locks[n].mutex.destroy();
    }
  if ((group->locks != NULL) && (group->locks != group->lock_store))
    delete[] group->locks;
  group->num_locks = 0;  group->locks = NULL;
  group->mutex.destroy();
  delete group;
  
  group = NULL;
  thread_idx = 0;
  recent_queue = NULL;

  return result;
}

/*****************************************************************************/
/*                    kdu_thread_entity::get_num_threads                     */
/*****************************************************************************/

int
  kdu_thread_entity::get_num_threads()
{
  if (!exists())
    return 0;
  return group->num_threads;
}

/*****************************************************************************/
/*                       kdu_thread_entity::add_thread                       */
/*****************************************************************************/

bool
  kdu_thread_entity::add_thread(int thread_concurrency)
{
  if (!exists())
    return false;

  group->mutex.lock();

  int thrd_idx = group->num_threads;
  bool success = (thrd_idx < KDU_MAX_THREADS);
  if (success)
    {
      success = group->thread_events[thrd_idx].create(false);
      if (success)
        {
          if ((group->threads[thrd_idx] = this->new_instance()) == NULL)
            {
              success = false;
              group->thread_events[thrd_idx].destroy();
            }
        }
      if (success)
        {
          kdu_thread_entity *thrd = group->threads[thrd_idx];
          group->num_threads = thrd_idx+1;
          thrd->thread_idx = thrd_idx;
          thrd->group = this->group;
          thrd->grouperr = this->grouperr;
          thrd->num_locks = this->num_locks;
          thrd->locks = this->locks;
          group->thread_activity[thrd_idx] = KD_THREAD_ACTIVE;
          success = thrd->thread.create(worker_startproc,(void *) thrd);
          if (!success)
            { // Unable to create the new thread
              assert(group->thread_activity[thrd_idx]==KD_THREAD_ACTIVE);
              group->thread_events[thrd_idx].destroy();
              thrd->group = NULL; // So `delete' passes verification test
              delete thrd;
              group->threads[thrd_idx] = NULL;
              group->num_threads--;
            }
        }
    }

#ifdef KDU_PTHREADS
  if (thread_concurrency > 0)
    pthread_setconcurrency(thread_concurrency);
  else if ((thread_concurrency == 0) && success)
    pthread_setconcurrency(group->num_threads);
                 // Make sure we have one kernel thread for each user thread
#endif // KDU_PTHREADS

  group->mutex.unlock();

  return success;
}

/*****************************************************************************/
/*                       kdu_thread_entity::add_queue                        */
/*****************************************************************************/

kdu_thread_queue *
  kdu_thread_entity::add_queue(kdu_worker *worker,
                               kdu_thread_queue *parent,
                               const char *name,
                               kdu_long queue_bank_idx)
{
  if (!exists())
    return NULL;
  int depth = 1;
  if (parent != NULL)
    {
      depth = parent->depth+1;
      queue_bank_idx = parent->bank_idx;
    }
  else if (queue_bank_idx < group->min_dormant_bank_idx)
    parent = &(group->queue_base);
  group->mutex.lock();
  kdu_thread_queue *queue = group->get_queue();
  queue->depth = depth;
  queue->name = name;
  queue->worker = worker;
  queue->parent = parent;
  queue->bank_idx = queue_bank_idx;
  if (worker != NULL)
    {
      kdu_thread_queue *scan;
      for (scan=parent; scan != NULL; scan=scan->parent)
        {
          scan->subtree_working_leaves++;
          if ((scan->subtree_working_leaves == 1) && (scan->worker != NULL))
            break; // Number of working leaves along this branch is still 1,
                   // since `scan' was previously a working leaf.
        }
    }
  if (parent == NULL)
    { // Add to the end of the dormant list
      queue->sibling_next = NULL;
      if ((queue->sibling_prev = group->dormant_tail) != NULL)
        queue->sibling_prev->sibling_next = queue;
      else
        group->dormant_head = queue;
      group->dormant_tail = queue;
      if (group->queue_base.subtree_working_leaves < group->num_threads)
        group->activate_dormant_queues(); // This will not make any new jobs
             // schedulable, since we have not added jobs yet to the new queue.
    }
  else
    {
      if ((queue->sibling_next = parent->children) != NULL)
        queue->sibling_next->sibling_prev = queue;
      parent->children = queue;
    }
  group->mutex.unlock();
  return queue;
}

/*****************************************************************************/
/*                       kdu_thread_entity::add_jobs                         */
/*****************************************************************************/

void
  kdu_thread_entity::add_jobs(kdu_thread_queue *queue, int num_jobs,
                              bool finalize_queue, kdu_uint32 secondary_seq)
{
  if (queue->worker == NULL)
    {
      assert(num_jobs == 0);
      return;
    }
  group->mutex.lock();
  if (queue->worker == NULL)
    { // We have to check again while the mutex is locked, just to be sure.
      group->mutex.unlock();
      return;
    }
  if (grouperr->failed)
    { // We don't want to go adding jobs to a queue after a thread has
      // already called its `handle_exception' function, since beyond that
      // point no queue should have any unassigned jobs.
      group->mutex.unlock();
      kdu_rethrow(grouperr->failure_code);
    }
  if (finalize_queue)
    {
      if (num_jobs == 0)
        queue->finalize(group);
      else
        queue->max_jobs_in_queue = num_jobs +
          queue->first_unassigned_job_idx + queue->num_unassigned_jobs;
    }
  else
    assert(queue->max_jobs_in_queue == 0);

  kdu_thread_queue *scan;
  int delta_runnable_jobs = 0;
  int delta_secondary_jobs = 0;
  int delta_primary_jobs = queue->num_unassigned_jobs -
        (queue->num_primary_jobs + queue->num_runnable_jobs);
            // Make any existing secondary jobs primary.
  assert(delta_primary_jobs >= 0);
  if ((delta_primary_jobs == 0) && (num_jobs == 0))
    { // Nothing more to do
      group->mutex.unlock();
      return;
    }

  if (secondary_seq != 0)
    delta_secondary_jobs = num_jobs;
  else
    delta_primary_jobs += num_jobs;
  queue->num_unassigned_jobs += num_jobs;
  queue->num_primary_jobs += delta_primary_jobs;
  bool is_dormant = (queue->bank_idx >= group->min_dormant_bank_idx);
  if ((group->num_idle_threads > 0) && (queue->num_unassigned_jobs > 0) &&
      !is_dormant)
    { // Make the appropriate number of jobs runnable
      delta_runnable_jobs = queue->num_primary_jobs;
      delta_primary_jobs -= delta_runnable_jobs;
      queue->num_primary_jobs = 0;
      if (delta_runnable_jobs == 0)
        {
          assert(delta_secondary_jobs > 0);
          delta_secondary_jobs--;
          delta_runnable_jobs++;
        }
      queue->num_runnable_jobs += delta_runnable_jobs;
    }

  if (delta_secondary_jobs > 0)
    {
      assert(secondary_seq != 0);
      kdu_uint32 sp, secondary_pref = (~secondary_seq) + 1;
      queue->secondary_pref = secondary_pref;
      for (scan=queue; scan != NULL; scan=scan->parent)
        {
          scan->subtree_unassigned_jobs += num_jobs;
          scan->subtree_primary_jobs += delta_primary_jobs;
          scan->subtree_runnable_jobs += delta_runnable_jobs;
          if ((sp = scan->subtree_secondary_pref) < secondary_pref)
            scan->subtree_secondary_pref = secondary_pref;
          else
            secondary_pref = sp;
        }
    }
  else
    {
      assert((queue->num_runnable_jobs + queue->num_primary_jobs) ==
             queue->num_unassigned_jobs);
      queue->secondary_pref = 0;
      for (scan=queue; scan != NULL; scan=scan->parent)
        {
          scan->subtree_unassigned_jobs += num_jobs;
          scan->subtree_primary_jobs += delta_primary_jobs;
          scan->subtree_runnable_jobs += delta_runnable_jobs;
          if ((scan->subtree_runnable_jobs + scan->subtree_primary_jobs) ==
              scan->subtree_unassigned_jobs)
            scan->subtree_secondary_pref = 0;
        }
    }

  if ((group->num_idle_threads > 0) && (queue->num_runnable_jobs > 0) &&
      (queue->num_active_jobs == 0) && (queue->prescheduled_job_idx < 0) &&
      !is_dormant)
    { // Wake up an idle worker thread now that there is more work to do.
      wake_idle_thread(queue);
    }
  group->mutex.unlock();
}

/*****************************************************************************/
/*                      kdu_thread_entity::synchronize                       */
/*****************************************************************************/

bool
  kdu_thread_entity::synchronize(kdu_thread_queue *root,
                                 bool finalize_descendants, bool finalize_root)
{
  int n;
  if (!exists())
    return true;

  for (n=0; n < group->num_locks; n++)
    if (group->locks[n].holder == this)
      handle_exception(0); // Must have forgotten to handle an exception

  if (root == NULL)
    {
      root = &(group->queue_base);
      if ((root->children == NULL) && (group->dormant_head == NULL))
        return !grouperr->failed; // Nothing to synchronize
    }

  group->mutex.lock();

  assert((root->children != NULL) ||
         ((root->num_unassigned_jobs+root->first_unassigned_job_idx)==0));
    // Remember: not allowed to synchronize on a leaf node, unless it has
    // never been assigned any jobs.


  if (root->install_synchronization_point(NULL,false,thread_idx,
                                    finalize_descendants,finalize_root,group))
    {
      group->mutex.unlock();
      bool success = process_jobs(root,true,false);
      group->mutex.lock();

      if (!success)
        { // Synchronization condition should have been removed by the
          // call to `handle_exception' from within `process_jobs'
          assert((root->num_sync_points == 0) ||
                 (root->sync_points[0].synchronizing_thread_idx!=thread_idx));
        }
      else
        { // Need to remove the synchronization condition and take a look at
          // any other sync points on the list, possibly running synchronized
          // jobs and/or waking up other synchronizing threads.
          assert((root->num_sync_points > 0) &&
                 (root->sync_points[0].synchronizing_thread_idx==thread_idx) &&
                 root->check_condition(true,thread_idx));
          root->num_sync_points--;
          for (n=0; n < root->num_sync_points; n++)
            root->sync_points[n] = root->sync_points[n+1];
          if (root->num_sync_points > 0)
            process_outstanding_sync_points(root);
        }
    }

  group->mutex.unlock();
  return !grouperr->failed;
}

/*****************************************************************************/
/*               kdu_thread_entity::register_synchronized_job                */
/*****************************************************************************/

void
  kdu_thread_entity::register_synchronized_job(kdu_worker *sync_worker,
                                               kdu_thread_queue *root,
                                               bool run_deferred)
{
  if (!exists())
    return;

  int n;
  for (n=0; n < group->num_locks; n++)
    if (group->locks[n].holder == this)
      handle_exception(0); // Must have forgotten to handle an exception

  if (root == NULL)
    {
      root = &(group->queue_base);
      if (root->children == NULL)
        { // Just execute the synchronized job immediately and return
          if (!grouperr->failed)
            sync_worker->do_job(this,-1);
          return;
        }
    }

  group->mutex.lock();

  assert((root->children != NULL) ||
         ((root->num_unassigned_jobs+root->first_unassigned_job_idx)==0));
    // Remember: not allowed to synchronize on a leaf node, unless it has
    // never been assigned any jobs.

  bool runnable_now =
    !root->install_synchronization_point(sync_worker,run_deferred,-1,
                                         false,false,group);
  if (runnable_now && run_deferred && (group->num_threads > 1) &&
      (group->num_deferred_jobs < KDU_MAX_THREADS))
    {
      group->deferred_jobs[group->num_deferred_jobs++] = sync_worker;
      runnable_now = false;
    }

  group->mutex.unlock();
  if (runnable_now && !grouperr->failed)
    sync_worker->do_job(this,-1);
}

/*****************************************************************************/
/*                       kdu_thread_entity::terminate                        */
/*****************************************************************************/

bool
  kdu_thread_entity::terminate(kdu_thread_queue *root, bool leave_root,
                               kdu_exception *exception_code)
{
  synchronize(root,true,!leave_root);
  bool failed = grouperr->failed;
  if (failed && (exception_code != NULL))
    *exception_code = grouperr->failure_code;
  if (root == NULL)
    {
      root = &(group->queue_base);
      leave_root = true;
      if (root->children == NULL)
        return !failed; // Nothing to do.
    }
  
  group->mutex.lock();
  assert(root->num_unassigned_jobs == root->subtree_unassigned_jobs);

  // Remove all of the queues that we are terminating from the
  // `recent_queue' member of all threads
  int n;
  for (n=0; n < group->num_threads; n++)
    {
      kdu_thread_entity *thrd = group->threads[n];
      kdu_thread_queue *qp = thrd->recent_queue;
      if ((qp == NULL) || ((qp == root) && leave_root))
        continue;
      for (; (qp != NULL); qp=qp->parent)
        if (qp == root)
          break;
      if (qp != NULL)
        thrd->recent_queue = NULL;
    }

  // Release all terminated queues
  if (!leave_root)
    { // Detach root queue from the hierarchy
      assert(root->num_unassigned_jobs == 0);
      if (root->sibling_prev == NULL)
        {
          assert(root->parent->children == root);
          root->parent->children = root->sibling_next;
        }
      else
        root->sibling_prev->sibling_next = root->sibling_next;
      if (root->sibling_next != NULL)
        root->sibling_next->sibling_prev = root->sibling_prev;
    }
  group->release_queues(root,leave_root);

  group->mutex.unlock();

  if (group->queue_base.children == NULL)
    { // Need to get all threads to execute their `on_finished' function.
      assert(this->is_group_owner() && (group->num_finished_threads == 0));
      this->on_finished(grouperr->failed);
      this->finished = true;
      group->mutex.lock();
      group->num_finished_threads = 1;
      group->finish_requested = true;
      for (n=1; n < group->num_threads; n++)
        group->thread_events[n].set();
      while (group->num_finished_threads < group->num_threads)
        group->thread_events[0].wait(group->mutex);
      assert(group->num_deferred_jobs == 0);
      group->finish_requested = 0;
      group->num_finished_threads = 0;
      for (n=0; n < group->num_threads; n++)
        {
          assert(group->threads[n]->finished);
          group->threads[n]->finished = false;
        }
      grouperr->failed = false;
      grouperr->failure_code = KDU_NULL_EXCEPTION;
      group->mutex.unlock();
    }
  return !failed;
}

/*****************************************************************************/
/*                    kdu_thread_entity::handle_exception                     */
/*****************************************************************************/

void
  kdu_thread_entity::handle_exception(kdu_exception exception_code)
{
  if (!exists())
    return;

  int n;
  for (n=0; n < group->num_locks; n++)
    if (group->locks[n].holder == this)
      release_lock(n);

  group->mutex.lock();
  group->num_deferred_jobs = 0;
  group->queue_base.handle_exception(thread_idx);
  for (kdu_thread_queue *dormant=group->dormant_head;
       dormant != NULL; dormant=dormant->sibling_next)
    dormant->handle_exception(thread_idx);
  group->activate_dormant_queues(); // Should leave all queues active
  assert((group->dormant_head == NULL) && (group->dormant_tail == NULL));
  
  for (n=0; n < group->num_threads; n++)
    if ((group->thread_activity[n] != NULL) &&
        (group->thread_activity[n] != KD_THREAD_ACTIVE))
      { // Found a pre-scheduled thread; the prescheduled job has been
        // destroyed already, so return this thread to the idle state.
        assert(n > 0); // Can't preschedule the group owner
        group->thread_activity[n] = NULL;
        group->num_idle_threads++;
        assert(group->num_idle_threads < group->num_threads);
      }
  if (!grouperr->failed)
    { // Failing for the first time
      grouperr->failed = true;
      grouperr->failure_code = exception_code;
    }

  group->mutex.unlock();
}

/*****************************************************************************/
/*                     kdu_thread_entity::wake_idle_thread                   */
/*****************************************************************************/

void
  kdu_thread_entity::wake_idle_thread(kdu_thread_queue *queue)
{
  assert((group->num_idle_threads > 0) && (queue->num_runnable_jobs > 0) &&
         (queue->num_active_jobs == 0) && (queue->prescheduled_job_idx < 0));

  // Find a good thread to wake up; we do this by measuring the "distance"
  // between each idle thread's most recent queue and the current queue.
  // Here, the "distance" of interest is the minimum number of branches
  // in the tree hierarchy which must be traversed, in order to walk up
  // the tree from one queue and back down to the other queue.  The
  // distance between a node and its parent is thus 1, the distance
  // between two sibling nodes is 2, the distance between a nephew and
  // an uncle is 3, etc.
  int n;
  int best_thread_idx = -1;
  int min_distance = INT_MAX;
  for (n=0; n < group->num_threads; n++)
    if (group->thread_activity[n] == NULL)
      {
        kdu_thread_entity *thrd = group->threads[n];
        kdu_thread_queue *scan1=queue, *scan2=thrd->recent_queue;
        int distance=0;
        if (scan2 != NULL)
          {
            for (; scan1->depth > scan2->depth; scan1=scan1->parent)
              distance++;
            for (; scan2->depth > scan1->depth; scan2=scan2->parent)
              distance++;
            for (; scan1!=scan2; scan1=scan1->parent, scan2=scan2->parent)
              distance+=2;
          }
        if (distance < min_distance)
          {
            min_distance = distance;
            best_thread_idx = n;
          }
      }

  // Pre-schedule this thread to do the next job on the present queue.
  assert((best_thread_idx > 0) &&
         (group->thread_activity[best_thread_idx]==NULL));
  queue->num_active_jobs++;
  queue->num_unassigned_jobs--;
  queue->num_runnable_jobs--;
  kdu_thread_queue *scan;
  for (scan=queue; scan != NULL; scan=scan->parent)
    {
      scan->subtree_unassigned_jobs--;
      scan->subtree_runnable_jobs--;
    }
  queue->prescheduled_job_idx = queue->first_unassigned_job_idx++;
  group->thread_activity[best_thread_idx] = queue;
  group->num_idle_threads--; // Pre-scheduled threads are no longer idle
  group->thread_events[best_thread_idx].set();
}

/*****************************************************************************/
/*            kdu_thread_entity::process_outstanding_sync_points             */
/*****************************************************************************/

bool
  kdu_thread_entity::process_outstanding_sync_points(kdu_thread_queue *queue)
{
  bool removed_something = false;
  while (queue->num_sync_points > 0)
    {
      kd_thread_sync_point *sp = queue->sync_points;
      if ((sp->synchronization_downcounter > 0) ||
          (sp->num_unsynchronized_children > 0))
        break;
      if (sp->synchronizing_thread_idx >= 0)
        {
          assert(sp->synchronized_worker == NULL);
          if (sp->synchronizing_thread_idx == queue->thread_awaiting_sync)
            group->thread_events[queue->thread_awaiting_sync].set();
          break;
        }

      int n;
      bool propagate_to_parent = false;
      if (sp->synchronized_worker != NULL)
        {
          if (sp->synchronized_job_in_progress)
            break; // Someone else will take it from here
          if (sp->synchronized_job_deferred && (group->num_threads > 1) &&
              (group->num_deferred_jobs < KDU_MAX_THREADS))
            { // Add to deferred job queue
              group->deferred_jobs[group->num_deferred_jobs++] =
                sp->synchronized_worker;
              if (group->num_idle_threads > 0)
                { // Wake up an idle thread
                  for (n=1; n < group->num_threads; n++)
                    if (group->thread_activity[n] == NULL)
                      break;
                  assert(n < group->num_threads);
                  group->num_idle_threads--;
                  group->thread_activity[n] = &(group->queue_base);
                  group->thread_events[n].set();
                }
            }
          else
            {
              sp->synchronized_job_in_progress = true;
              group->mutex.unlock();
              sp->synchronized_worker->do_job(this,-1);
              if (need_sync())
                do_sync(grouperr->failed);
              group->mutex.lock();
              sp->synchronized_job_in_progress = false;
            }
          sp->synchronized_worker = NULL;
        }
      else
        propagate_to_parent = (queue->parent != NULL);
      queue->num_sync_points--;
      for (n=0; n < queue->num_sync_points; n++)
        queue->sync_points[n] = queue->sync_points[n+1];
      removed_something = true;

      if ((queue->num_sync_points > 0) &&
          (queue->sync_points[0].synchronization_downcounter == 0))
        {
          sp = queue->sync_points;
          if (sp->finalize_current && (queue->worker != NULL))
            queue->finalize(group);

          // Install synchronization requirements into children.
          kdu_thread_queue *qscn;
          assert(sp->num_unsynchronized_children == 0);
          for (qscn=queue->children; qscn != NULL; qscn=qscn->sibling_next)
            if (qscn->install_synchronization_point(NULL,false,-1,
                           sp->finalize_children,sp->finalize_children,group))
              sp->num_unsynchronized_children++;
          if (queue->parent == NULL)
            { // Consider dormant queues as though they belonged to the root of
              // the queue hierarchy, for the purpose of installing
              // synchronization conditions.
              assert(queue == &group->queue_base);
              for (qscn=group->dormant_head;
                   qscn != NULL; qscn=qscn->sibling_next)
                if (qscn->install_synchronization_point(NULL,false,-1,
                          sp->finalize_children,sp->finalize_children,group))
                  sp->num_unsynchronized_children++;
            }
        }

      if (propagate_to_parent)
        {
          kdu_thread_queue *parent = queue->parent;
          assert((parent->num_sync_points > 0) &&
                 (parent->sync_points[0].num_unsynchronized_children > 0) &&
                 (parent->sync_points[0].synchronization_downcounter == 0));
          parent->sync_points[0].num_unsynchronized_children--;
          if (parent->sync_points[0].num_unsynchronized_children == 0)
            process_outstanding_sync_points(parent);
        }
    }
  return removed_something;
}

/*****************************************************************************/
/*                     kdu_thread_entity::process_jobs                       */
/*****************************************************************************/

bool
  kdu_thread_entity::process_jobs(kdu_thread_queue *wait_queue,
                                  bool waiting_for_sync,
                                  bool throw_on_failure)
{
  assert((wait_queue != NULL) || !is_group_owner());
        // Owner can only call this function with `wait_queue' non-NULL.

  group->mutex.lock();

  int n;
  kdu_thread_queue *qscn;

  if ((wait_queue != NULL) && (!waiting_for_sync) &&
      ((n=wait_queue->num_primary_jobs) > 0))
    { // Make all primary jobs runnable
      wait_queue->num_runnable_jobs += n;
      wait_queue->num_primary_jobs = 0;
      for (qscn=wait_queue; qscn != NULL; qscn=qscn->parent)
        {
          qscn->subtree_runnable_jobs += n;
          qscn->subtree_primary_jobs -= n;
        }
    }

  kdu_thread_queue *run_queue=NULL; // Queue selected for running the next job
  kdu_thread_queue *last_queue=NULL; // Queue used to run the last job

  while (1)
    {
      // First, find a queue on which to run a new job, unless a condition
      // on which we are waiting occurs in the meantime, in which case, set
      // `run_queue' to NULL.
      kdu_thread_queue *search_start = last_queue;
      if (search_start == NULL)
        search_start = wait_queue;
      if ((search_start == NULL) ||
          (search_start->bank_idx >= group->min_dormant_bank_idx))
        search_start = recent_queue;
      if (search_start == NULL)
        search_start = &(group->queue_base);
      while ((run_queue == NULL) &&
             ((wait_queue != NULL) || (group->num_deferred_jobs == 0)) &&
             ((wait_queue == NULL) ||
              !wait_queue->check_condition(waiting_for_sync,thread_idx)) &&
             ((run_queue=search_start->find_unassigned_job(thread_idx))==NULL))
        { // Can't return true yet and can't find a suitable unassigned job;
          // we will need to block
          if (group->destruction_requested ||
              (grouperr->failed && throw_on_failure))
            {
              group->mutex.unlock();
              if (throw_on_failure)
                kdu_rethrow(grouperr->failure_code);
              else
                return false;
            }
          if (wait_queue != NULL)
            {
              if (waiting_for_sync)
                {
                  assert((wait_queue->num_sync_points > 0) &&
                         (wait_queue->sync_points[0].synchronizing_thread_idx==
                          this->thread_idx)); // Can't have multiple threads
                                   // trying to sync on exactly the same queue
                  wait_queue->thread_awaiting_sync = this->thread_idx;
                }
              else
                wait_queue->thread_awaiting_complete = this->thread_idx;
            }
          else if (group->finish_requested && !this->finished)
            {
              group->mutex.unlock();
              try {
                  on_finished(grouperr->failed);
                }
              catch (kdu_exception exc) {
                  handle_exception(exc);
                }
              catch (std::bad_alloc) {
                  handle_exception(KDU_MEMORY_EXCEPTION);
                }
              catch (...) {
                  handle_exception(KDU_CONVERTED_EXCEPTION);
                }
              group->mutex.lock();
              this->finished = true;
              group->num_finished_threads++;
              if (group->num_finished_threads == group->num_threads)
                group->thread_events[0].set(); // Owner may be waiting for this
              last_queue = NULL;
              search_start = &(group->queue_base);
              continue;
            }
          else
            { // Put ourselves on the idle list; we don't do this if we are
              // waiting on a `wait_queue', in part because the queue must be
              // very nearly ready -- after all, there are no unassigned jobs.
              assert(!is_group_owner());
              assert(group->thread_activity[thread_idx] != NULL);
              group->num_idle_threads++;
              group->thread_activity[thread_idx] = NULL;
            }
          kdu_event *ep = group->thread_events + thread_idx;
          ep->reset();
          ep->wait(group->mutex);
          if (wait_queue == NULL)
            { // We have been woken up from the idle state; let's check to see
              // if a job has been pre-scheduled for us by `add_jobs', or if
              // we have been woken up to run a deferred job.
              if (group->thread_activity[thread_idx] != NULL)
                { // Must be pre-scheduled.
                  run_queue = group->thread_activity[thread_idx];
                  assert(run_queue != KD_THREAD_ACTIVE);
                  if (run_queue == &group->queue_base)
                    { // We have been woken up to run a deferred job; we will
                      // have to check that it has not already been serviced
                      // by another thread.  To do this, we mark ourselves
                      // as active and go back to the beginning of the loop.
                      group->thread_activity[thread_idx] = KD_THREAD_ACTIVE;
                      run_queue = NULL;
                      continue;
                    }
                  assert(run_queue->prescheduled_job_idx >= 0);
                  break;
                }
              else
                { // Thread must still be idle
                  assert(group->num_idle_threads > 0);
                  group->thread_activity[thread_idx] = KD_THREAD_ACTIVE;
                  group->num_idle_threads--;
                  if (group->finish_requested)
                    search_start = &(group->queue_base);
                }
            }
          else if (waiting_for_sync)
            wait_queue->thread_awaiting_sync = -1;
          else
            wait_queue->thread_awaiting_complete = -1;
        }

      if (grouperr->failed && throw_on_failure)
        {
          group->mutex.unlock();
          kdu_rethrow(grouperr->failure_code);
        }

      // At this point, we can either run something or break out of the loop.
      // Start by checking if we can run a deferred job.  If we can, we will
      // have to first run `do_sync' and set `run_queue' to NULL to force a
      // complete re-evaluation (when we return) of jobs which can be run.
      if ((wait_queue == NULL) && (group->num_deferred_jobs > 0) &&
          (group->thread_activity[thread_idx] == KD_THREAD_ACTIVE))
        {
          assert(group->num_threads > 1);
          kdu_worker *job = group->deferred_jobs[0];
          for (int j=1; j < group->num_deferred_jobs; j++)
            group->deferred_jobs[j-1] = group->deferred_jobs[j];
          group->num_deferred_jobs--;
          run_queue = NULL; // Can't trust any job selected by the scheduler
                            // to still be unassigned by the time we return
                            // from running the deferred job.
          group->mutex.unlock();
          try {
              if (need_sync())
                do_sync(grouperr->failed);
              job->do_job(this,-1);
            }
          catch (kdu_exception exc) {
              handle_exception(exc);
            }
          catch (std::bad_alloc) {
              handle_exception(KDU_MEMORY_EXCEPTION);
            }
          catch (...) {
              handle_exception(KDU_CONVERTED_EXCEPTION);
            }
          group->mutex.lock();
          last_queue = NULL;
          continue;
        }

      // At this point `run_queue' is NULL only if a condition on which we
      // are waiting has occurred.  Otherwise, `run_queue' identifies the
      // queue from which the next job is to be scheduled -- it is possible
      // that this is a pre-scheduled job.
      if (run_queue == NULL)
        break; // Break out of the `while' loop and return true

      int job_number=-1;
      if (group->thread_activity[thread_idx] == run_queue)
        {
          assert(run_queue->prescheduled_job_idx >= 0);
          job_number = run_queue->prescheduled_job_idx;
          run_queue->prescheduled_job_idx = -1;
          group->thread_activity[thread_idx] = KD_THREAD_ACTIVE;
        }
      else
        {
          assert((run_queue->num_unassigned_jobs > 0) &&
                 (run_queue->num_runnable_jobs > 0));
          run_queue->num_unassigned_jobs--;
          run_queue->num_runnable_jobs--;
          for (qscn=run_queue; qscn != NULL; qscn=qscn->parent)
            {
              qscn->subtree_unassigned_jobs--;
              qscn->subtree_runnable_jobs--;
            }
          run_queue->num_active_jobs++;
          job_number = run_queue->first_unassigned_job_idx++;
        }

      // Now run the job
      group->mutex.unlock();
      if (!throw_on_failure)
        { // Catch exceptions thrown from `do_job'
          try {
              run_queue->worker->do_job(this,job_number);
            }
          catch (kdu_exception exc) {
              handle_exception(exc);
            }
          catch (std::bad_alloc) {
              handle_exception(KDU_MEMORY_EXCEPTION);
            }
          catch (...) {
              handle_exception(KDU_CONVERTED_EXCEPTION);
            }
        }
      else
        run_queue->worker->do_job(this,job_number);
      group->mutex.lock();

      // Now check for failure
      if (grouperr->failed)
        { // Someone must have called `handle_exception'
          assert(run_queue->num_active_jobs == 0);
          run_queue = NULL; // All active jobs should have been cleared by the
                            // call to `handle_exception'
          if (!throw_on_failure)
            {
              last_queue = NULL;
              continue; // Just keep looping
            }
          group->mutex.unlock();
          kdu_rethrow(grouperr->failure_code);
        }

      // If we get here, a job has been successfully run and must now be
      // cleared from its queue.  This may trigger all sorts of happy
      // occurrences based on installed synchronization points.
      last_queue = run_queue;
      run_queue = NULL; // We may pre-assign a run-queue for the next job here
      assert(last_queue->num_active_jobs > 0);

      // Need to determine whether or not `do_sync' is needed.  If true,
      // it should be called now, before we mark the current job as
      // finished.  Here are the conditions under which `do_sync' may
      // need to be called:
      //   (I) `last_queue' is not a leaf node; or
      //  (II) there are no runnable jobs which can be allocated to the current
      //       thread either from `last_queue', or from its parent or
      //       any of its siblings; or
      //  (III) `last_queue' is a leaf node which is associated with a
      //        synchronization condition (its parent must have the same
      //        synchronization condition) and the synchronization condition
      //        might occur before any further jobs are run on the current
      //        thread.  This can happen if the conditions required for the
      //        synchronization condition with respect to `last_queue' have
      //        just been satisfied and we are not about to run any job in
      //        a sibling queue which is required for the synchronization
      //        condition to be fulfilled.  If this condition is tested and
      //        rejected, `run_queue' is left pointing to the queue from which
      //        the next job must be taken -- it will be picked up in the next
      //        execution of the `while' loop.
      bool need_do_sync =
        (last_queue->children != NULL) || // Case I
        (last_queue->parent == NULL) ||
        (last_queue->parent->subtree_runnable_jobs == 0); // Case II
      if ((!need_do_sync) && (last_queue->num_sync_points > 0) &&
          (job_number < last_queue->sync_points[0].synchronization_threshold))
        { // Considering Case III.
          assert((last_queue->parent->num_sync_points > 0) &&
           (last_queue->parent->sync_points[0].num_unsynchronized_children>0));
               // Above assertion should be true because synchronization
               // points cannot be directly installed in leaf nodes and
               // we have already verified that we are a leaf node.
          if (last_queue->num_runnable_jobs > 0)
            { // The next job will come from the current queue unless we
              // have to execute a synchronization handler, in which case we
              // will run `do_sync' separately at that point.
              need_do_sync =
                (last_queue->first_unassigned_job_idx >=
               last_queue->sync_points[0].synchronization_threshold);
            }
          else if ((wait_queue == last_queue) && (!waiting_for_sync) &&
                   (wait_queue->num_active_jobs == 1))
            { // In this case, we are about to quit the loop and return,
              // since we have just satisfied the simple non-synchronizing
              // wait condition with which we entered.  In this case, there
              // is no point in hunting for a next job to run and we will
              // be running `do_sync' right before returning from the function
              // so there is no need to mark `need_do_sync' as true here.
              need_do_sync = false;
            }
          else
            { // See if there is a sibling which has a job to run, on which
              // our parent is waiting.
              need_do_sync = true;
              for (run_queue=last_queue->parent->children;
                   run_queue != NULL; run_queue=run_queue->sibling_next)
                if ((run_queue->num_sync_points > 0) &&
                    (run_queue->num_runnable_jobs > 0) &&
                    (run_queue->first_unassigned_job_idx <
                     run_queue->sync_points[0].synchronization_threshold))
                  { need_do_sync = false; break; }
            }
        }
      if (need_do_sync && need_sync())
        {
          assert(run_queue == NULL);
          group->mutex.unlock();
          try {
              do_sync(grouperr->failed);
            }
          catch (kdu_exception exc) {
              handle_exception(exc);
            }
          catch (std::bad_alloc) {
              handle_exception(KDU_MEMORY_EXCEPTION);
            }
          catch (...) {
              handle_exception(KDU_CONVERTED_EXCEPTION);
            }
          group->mutex.lock();
          if (grouperr->failed)
            { // Someone must have called `handle_exception'
              assert(last_queue->num_active_jobs == 0);
              last_queue = NULL; // All active jobs should have been cleared by
                                 // the call to `handle_exception'
              last_queue = NULL;
              if (!throw_on_failure)
                continue; // Just keep looping
              group->mutex.unlock();
              kdu_rethrow(grouperr->failure_code);
            }
        }

      last_queue->num_active_jobs--;

      // Check to see if the queue is all finished
      if ((last_queue->num_active_jobs == 0) &&
          (last_queue->first_unassigned_job_idx ==
           last_queue->max_jobs_in_queue))
        {
          last_queue->worker = NULL;
          if (last_queue->subtree_working_leaves == 0)
            { // `last_queue' was a working leaf, which is no longer working
              for (qscn=last_queue->parent; qscn != NULL; qscn=qscn->parent)
                {
                  qscn->subtree_working_leaves--;
                  if ((qscn->subtree_working_leaves == 0) &&
                      (qscn->worker != NULL))
                    break;  // Num working leaves along this branch is still
                      // exactly 1, because `qscn' is now a working leaf.
                }
              if ((group->dormant_head != NULL) &&
                  (group->queue_base.subtree_working_leaves <
                   group->num_threads))
                group->activate_dormant_queues();
            }
        }

      // Check to see if a synchronization condition has been
      // reached for the first time.
      for (n=0; n < last_queue->num_sync_points; n++)
        if (job_number < last_queue->sync_points[n].synchronization_threshold)
          {
            kd_thread_sync_point *sp = last_queue->sync_points+n;
            assert(sp->synchronization_downcounter > 0);
            sp->synchronization_downcounter--;
            if ((n == 0) && (sp->synchronization_downcounter == 0))
              {
                if (sp->finalize_current && (last_queue->worker != NULL))
                  last_queue->finalize(group);
                assert(sp->num_unsynchronized_children == 0);
                for (qscn=last_queue->children;
                     qscn != NULL; qscn=qscn->sibling_next)
                  if (qscn->install_synchronization_point(NULL,false,-1,
                             sp->finalize_children,sp->finalize_children,group))
                    sp->num_unsynchronized_children++;
                if ((sp->num_unsynchronized_children == 0) &&
                    process_outstanding_sync_points(last_queue))
                  { // One or more initial sync points have been removed
                    n = -1; // So the next iteration in the loop starts at n=0
                  }
              }
          }

      if ((last_queue->thread_awaiting_complete >= 0) &&
          last_queue->check_condition(false,-1))
        {
          assert(last_queue->thread_awaiting_complete != this->thread_idx);
                  // We only mark ourselves as `waiting_complete' while we are
                  // blocked on our thread event object.
          group->thread_events[last_queue->thread_awaiting_complete].set();
        }

      if ((wait_queue != NULL) &&
          wait_queue->check_condition(waiting_for_sync,thread_idx))
        {
          break; // Break out of `while' loop to return true
        }

      if ((run_queue == NULL) && (last_queue->num_runnable_jobs > 0))
        run_queue = last_queue; // Move on to next job in the same queue.

      // Mark `last_queue' as our most `recent_queue'.
      this->recent_queue = last_queue;
    }

  // We get here only if a `wait_queue' condition has occurred
  group->mutex.unlock();
  if (need_sync())
    do_sync(grouperr->failed);
  return true;
}
