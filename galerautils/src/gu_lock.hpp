/*
 * Copyright (C) 2009-2017 Codership Oy <info@codership.com>
 *
 */

#ifndef __GU_LOCK__
#define __GU_LOCK__

#include "gu_exception.hpp"
#include "gu_logger.hpp"
#include "gu_mutex.hpp"
#include "gu_cond.hpp"
#include "gu_datetime.hpp"

#include <cerrno>
#include <cassert>

namespace gu
{
    class Lock
    {
        const Mutex* mtx_;

#ifdef HAVE_PSI_INTERFACE
        MutexWithPFS* pfs_mtx_;
#endif /* HAVE_PSI_INTERFACE */

        Lock (const Lock&);
        Lock& operator=(const Lock&);

    public:

        Lock (const Mutex& mtx) : mtx_(&mtx)
#if HAVE_PSI_INTERFACE
            , pfs_mtx_()
#endif
        {
            mtx_->lock();
        }

        virtual ~Lock ()
        {
#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_ != NULL)
            {
                pfs_mtx_->unlock();
                return;
            }
#endif /* HAVE_PSI_INTERFACE */
           mtx_->unlock();
        }

        inline void wait (const Cond& cond)
        {

#ifdef GU_MUTEX_DEBUG
#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_)
                pfs_mtx_->locked_ = false;
            else
                mtx_->locked_ = false;
#else
            mtx_->locked_ = false;
#endif /* HAVE_PSI_INTERFACE */
#endif /* GU_MUTEX_DEBUG */


            cond.ref_count++;

#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_)
                gu_cond_wait (&(cond.cond), pfs_mtx_->value);
            else
                gu_cond_wait (&(cond.cond), &(mtx_->impl()));
#else
            gu_cond_wait (&(cond.cond), &(mtx_->impl()));
#endif /* HAVE_PSI_INTERFACE */

            cond.ref_count--;

#ifdef GU_MUTEX_DEBUG
#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_)
            {
                pfs_mtx_->locked_ = true;
                pfs_mtx_->owned_ = gu_thread_self();
            }
            else
            {
                mtx_->locked_ = true;
                mtx_->owned_  = gu_thread_self();
            }
#else
            mtx_->locked_ = true;
            mtx_->owned_  = gu_thread_self();
#endif /* HAVE_PSI_INTERFACE */
#endif /* GU_MUTEX_DEBUG */

        }

        inline void wait (const Cond& cond, const datetime::Date& date)
        {
            timespec ts;

            date._timespec(ts);


#ifdef GU_MUTEX_DEBUG
#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_)
                pfs_mtx_->locked_ = false;
            else
                mtx_->locked_ = false;
#else
            mtx_->locked_ = false;
#endif /* HAVE_PSI_INTERFACE */
#endif /* GU_MUTEX_DEBUG */

            cond.ref_count++;

            int ret;
#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_)
                ret = gu_cond_timedwait (&(cond.cond), pfs_mtx_->value, &ts);
            else
#endif /* HAVE_PSI_INTERFACE */
                ret = gu_cond_timedwait (&(cond.cond), &(mtx_->impl()), &ts);

            cond.ref_count--;

#ifdef GU_MUTEX_DEBUG
#ifdef HAVE_PSI_INTERFACE
            if (pfs_mtx_)
            {
                pfs_mtx_->locked_ = true;
                pfs_mtx_->owned_ = gu_thread_self();
            }
            else
            {
                mtx_->locked_ = true;
                mtx_->owned_  = gu_thread_self();
            }
#else
            mtx_->locked_ = true;
            mtx_->owned_  = gu_thread_self();
#endif /* HAVE_PSI_INTERFACE */
#endif /* GU_MUTEX_DEBUG */

            if (gu_unlikely(ret)) gu_throw_error(ret);
        }

#ifdef HAVE_PSI_INTERFACE
        Lock (const MutexWithPFS& pfs_mtx)
            :
            mtx_(),
            pfs_mtx_(const_cast<gu::MutexWithPFS*>(&pfs_mtx))
        {
            pfs_mtx_->lock();
        }

        inline void wait (const CondWithPFS& cond)
        {
            cond.ref_count++;
            pfs_instr_callback(
                WSREP_PFS_INSTR_TYPE_CONDVAR,
                WSREP_PFS_INSTR_OPS_WAIT,
                cond.m_tag,
                reinterpret_cast<void**>(const_cast<gu_cond_t**>(&(cond.cond))),
                reinterpret_cast<void**>(const_cast<pthread_mutex_t**>(
                                         &(pfs_mtx_->value))),
                NULL);
            cond.ref_count--;
        }

        inline void wait (const CondWithPFS& cond, const datetime::Date& date)
        {
            timespec ts;

            date._timespec(ts);
            cond.ref_count++;
            pfs_instr_callback(
                WSREP_PFS_INSTR_TYPE_CONDVAR,
                WSREP_PFS_INSTR_OPS_WAIT,
                cond.m_tag,
                reinterpret_cast<void**>(const_cast<gu_cond_t**>(&(cond.cond))),
                reinterpret_cast<void**>(const_cast<pthread_mutex_t**>(
                                         &(pfs_mtx_->value))),
                &ts);
            cond.ref_count--;
        }
#endif /* HAVE_PSI_INTERFACE */
    };
}

#endif /* __GU_LOCK__ */
