/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
        struct semaphore *sem;

        sem = kmalloc(sizeof(*sem));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
        struct lock *lock;

        lock = kmalloc(sizeof(*lock));
        if (lock == NULL) {
                return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }

        // add stuff here as needed
        #if LOCK_WITH_SEMAPHORE
        lock->binarySem=sem_create(name,1);
        lock->owner=NULL;
        #endif
        #if LOCK_WITHOUT_SEMAPHORE
        lock->owner=NULL;
        lock->value=true;
        lock->lock_wchan= wchan_create(name);
	if (lock->lock_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

	spinlock_init(&lock->lock_lock);
        #endif
        return lock;
}

void
lock_destroy(struct lock *lock)
{
        KASSERT(lock != NULL);

        // add stuff here as needed

        kfree(lock->lk_name);
        #if LOCK_WITH_SEMAPHORE
        sem_destroy(lock->binarySem);
        //chiaramente non elimino il thread owner 
        #endif
        #if LOCK_WITHOUT_SEMAPHORE
        /* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&lock->lock_lock);
	wchan_destroy(lock->lock_wchan);
        #endif
        kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
        // Write this
        #if LOCK_WITH_SEMAPHORE
        KASSERT(lock != NULL);
        P(lock->binarySem);
        lock->owner=curthread;
        #endif
        #if LOCK_WITHOUT_SEMAPHORE
        KASSERT(lock != NULL);
        

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&lock->lock_lock);
        while (lock->value == false) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(lock->lock_wchan, &lock->lock_lock);
        }
        KASSERT(lock->value==true);
        lock->value=false;
        
	spinlock_release(&lock->lock_lock);
        lock->owner=curthread;

        #endif
        (void)lock;  // suppress warning until code gets written
}

void
lock_release(struct lock *lock)
{
        // Write this
        #if LOCK_WITH_SEMAPHORE
        KASSERT(lock_do_i_hold(lock)==true);
        lock->owner=NULL;
        V(lock->binarySem);
        #endif
        #if LOCK_WITHOUT_SEMAPHORE
        KASSERT(lock!= NULL);
        //spinlock_acquire(&lock->lock_lock);
        KASSERT(lock->owner==curthread);
        //spinlock_release(&lock->lock_lock);
        spinlock_acquire(&lock->lock_lock);
        lock->value=true;
        KASSERT(lock->value==true);
	wchan_wakeone(lock->lock_wchan, &lock->lock_lock);
        lock->owner=NULL;
	spinlock_release(&lock->lock_lock);
        #endif
        (void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
        // Write this
        #if LOCK_WITH_SEMAPHORE
        spinlock_acquire(&lock->binarySem->sem_lock);
        if (lock->owner==curthread){
                spinlock_release(&lock->binarySem->sem_lock);
                return true;
        }
        else{ 
                spinlock_release(&lock->binarySem->sem_lock);
                return false;
        }
        #endif
        #if LOCK_WITHOUT_SEMAPHORE
        spinlock_acquire(&lock->lock_lock);
        if (lock->owner==curthread){
                spinlock_release(&lock->lock_lock);
                return true;
        }
        else{ 
                spinlock_release(&lock->lock_lock);
                return false;
        }
        #endif

        (void)lock;  // suppress warning until code gets written

        return true; // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(*cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }

        // add stuff here as needed
        #if CV
        cv->wc=wchan_create("wchan_cv");

        spinlock_init(&cv->splk_wc);
        //cv->lk=lock_create("lock_cv");
        #endif
        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed

        kfree(cv->cv_name);
        #if CV
        wchan_destroy(cv->wc);
        spinlock_cleanup(&cv->splk_wc);
        //lock_destroy(cv->lk);
        #endif
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        // Write this
        #if CV
        KASSERT(cv!=NULL);
        KASSERT(lock!=NULL);
        KASSERT(lock->owner==curthread);
        spinlock_acquire(&cv->splk_wc);
        lock_release(lock);
        wchan_sleep(cv->wc,&cv->splk_wc);
        spinlock_release(&cv->splk_wc);
        lock_acquire(lock);
        #endif
        (void)cv;    // suppress warning until code gets written
        (void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
        // Write this
        #if CV
        KASSERT(cv!=NULL);
        KASSERT(lock!=NULL);
        KASSERT(lock->owner==curthread);
        spinlock_acquire(&cv->splk_wc);
        wchan_wakeone(cv->wc,&cv->splk_wc);
        spinlock_release(&cv->splk_wc);
        #endif
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
        #if CV
        KASSERT(cv!=NULL);
        KASSERT(lock!=NULL);
        spinlock_acquire(&cv->splk_wc);
        if (lock_do_i_hold(lock))
                wchan_wakeall(cv->wc,&cv->splk_wc);
        spinlock_release(&cv->splk_wc);
        #endif
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
}
