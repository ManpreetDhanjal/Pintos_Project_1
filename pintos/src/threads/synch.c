/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/
#include "threads/malloc.h"
#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "inttypes.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

void
sema_down_with_compare (struct semaphore *sema, int64_t ticks) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());
  thread_current()->sleep_time = ticks;
  old_level = intr_disable ();
  if(sema->value >0){
	thread_yield();
  }
  while (sema->value == 0) 
    {
	
      list_insert_ordered (&sema->waiters, &thread_current ()->elem,
                     (list_less_func*)&compare_thread, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}
/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_insert_ordered (&sema->waiters, &thread_current()->elem,(list_less_func*)&compare_priority, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

void
sema_up_with_compare (struct semaphore *sema, int64_t ticks) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  struct thread* t;
  old_level = intr_disable ();

  if(!list_empty(&sema->waiters)){
  	while(!list_empty(&sema->waiters) && list_entry(list_begin(&sema->waiters), struct thread, elem)->sleep_time <= ticks){
  		t = list_entry(list_pop_front(&sema->waiters), struct thread, elem);
  		thread_unblock(t);
  		sema->value++;
  	}
	
  }
   intr_set_level(old_level);
  
}
/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  int thread_priority = -1;
  if (!list_empty (&sema->waiters)) {
	struct thread* t = list_entry (list_pop_front (&sema->waiters),struct thread, elem);
	thread_priority = t->priority;
    	thread_unblock (t);
  }
  sema->value++;
  intr_set_level (old_level);
  if(thread_current()->priority < thread_priority){
	thread_yield();  
  }
  
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  
  if(lock->holder!=NULL && lock->holder->priority>0 && lock->holder->priority < thread_current()->priority){
    // insert in thread priority list
    struct thread_priority *tp = (struct thread_priority*)malloc(sizeof(struct thread_priority));
    tp->val = thread_current()->priority;

    list_insert_ordered (&lock->holder->priority_list, &tp->elem,(list_less_func*)&compare_priority_elem, NULL);
    int xx = list_size(&lock->holder->priority_list);
    printf("inserted %zd\n", xx);
    int priority = lock->holder->priority;
    if(lock->holder->origPriority == -1){
    	lock->holder->origPriority = priority; 
    }
    lock->holder->priority = thread_current()->priority;
    update_lock_hold_priority(lock->holder);
  }
  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  // when thread's priority list is empty, rever to original
  if(!list_empty(&thread_current()->priority_list)){
       	struct list* priority_list = &thread_current()->priority_list;
 	struct list waiter_list = lock->semaphore.waiters;
	// get first waiter of lock
   	//printf("list size:%d\n", list_size(&waiter_list));
     	struct thread* waiter = list_entry(list_begin(&waiter_list),struct thread, elem);
	int highest_waiter_priority = waiter->priority;

        //int highest_waiter_priority = list_entry(list_begin(&waiter_list), struct thread, elem)->priority;
	//printf("pp:%d\n", highest_waiter_priority);
	if(thread_current()->priority == highest_waiter_priority){ // if loc
		// pop lock holder from thread priority list
		list_pop_front(priority_list);
	}else{
     		// loop over the list to remove the thread
		struct list_elem* e;
		int xx = list_size(&thread_current()->priority_list);
		printf("size:%zd\n", xx);
		for (e = list_begin(priority_list); e != list_end(priority_list); e = list_next(e)){
			struct thread_priority *tp = list_entry(e, struct thread_priority, elem);
 			
			if(thread_current()->priority == tp->val){
				//printf("curr:%d tp:%d\n", thread_current()->priority, tp->val);
				list_remove(&tp->elem);	
				break;		
			}
		}
	}
	// get the new priority, the thread at the front of the list
	if(list_empty(&thread_current()->priority_list)){
		//printf("here\n");
		thread_current()->priority = thread_current()->origPriority;
		thread_current()->origPriority = -1;
	}else{
		struct thread_priority* tp = list_entry(list_begin(priority_list), struct thread_priority, elem);	
		thread_current()->priority = tp->val;
		//printf("p in else %d\n", tp->val);
	}
	//printf("dd%d\n", thread_current()->priority);
  }
  lock->holder = NULL;
  
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) {
    struct list_elem *e;
    struct semaphore_elem *curr_ele;
    struct list sema_list;
    struct thread *t;
    struct semaphore_elem *higher_priority_waiter;
    int maxPriority = -1;
    
    for (e = list_begin (&cond->waiters); e != list_end (&cond->waiters); e = list_next (e)){
      curr_ele=  list_entry (e,struct semaphore_elem, elem);
      sema_list = curr_ele->semaphore.waiters;
      t = list_entry(list_begin(&sema_list),struct thread,elem);
      
      if(maxPriority < t->priority){
        maxPriority = t->priority;
        higher_priority_waiter = curr_ele;
      }

    }
    list_remove(&higher_priority_waiter->elem);
    sema_up (&higher_priority_waiter->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
