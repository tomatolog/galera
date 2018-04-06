//
// Copyright (C) 2010-2014 Codership Oy <info@codership.com>
//
#ifndef GALERA_WSDB_HPP
#define GALERA_WSDB_HPP

#include "trx_handle.hpp"
#include "wsrep_api.h"
#include "gu_unordered.hpp"

namespace galera
{
    class Wsdb
    {

        class Conn
        {
        public:
            Conn(wsrep_conn_id_t conn_id)
                :
                conn_id_(conn_id),
                trx_()
            { }

            Conn(const Conn& other)
                :
                conn_id_(other.conn_id_),
                trx_(other.trx_)
            { }

            ~Conn() { }

            void assign_trx(TrxHandlePtr& trx)
            {
                trx_ = trx;
            }

            void reset_trx()
            {
                trx_ = TrxHandlePtr();
            }

            TrxHandlePtr get_trx()
            {
                return trx_;
            }

        private:
            void operator=(const Conn&);
            wsrep_conn_id_t conn_id_;
            TrxHandlePtr trx_;
        };


        class TrxHash
        {
        public:
            size_t operator()(const wsrep_trx_id_t& key) const { return key; }
        };

        typedef gu::UnorderedMap<wsrep_trx_id_t, TrxHandlePtr, TrxHash> TrxMap;

        /* TrxMap structure doesn't take into consideration presence of 2 trx
        objects with same trx_id (2^64 - 1 which is default trx_id) belonging
        to 2 different connections.
        This eventually causes same trx object to get shared among 2 different
        unrelated connections which causes state in-consistency leading
        to crash. (RACE CONDITION).
        This problem could be solved by taking into consideration conn-id but
        that would invite interface change to avoid this we maintain a separate
        map of such trx object based on pthread_id. */
        class ConnTrxHash
        {
        public:
            size_t operator()(const pthread_t& key) const { return key; }
        };

        typedef gu::UnorderedMap<pthread_t, TrxHandlePtr, ConnTrxHash> ConnTrxMap;

        class ConnHash
        {
        public:
            size_t operator()(const wsrep_conn_id_t& key) const { return key; }
        };

        typedef gu::UnorderedMap<wsrep_conn_id_t, Conn, ConnHash> ConnMap;

    public:

        TrxHandlePtr get_trx(const TrxHandle::Params& params,
                             const wsrep_uuid_t&      source_id,
                             wsrep_trx_id_t           trx_id,
                             bool                     create = false);

        TrxHandlePtr new_trx(const TrxHandle::Params& params,
                             const wsrep_uuid_t&      source_id,
                             wsrep_trx_id_t           trx_id)
        {
            return TrxHandlePtr(TrxHandle::New(trx_pool_, params,
                                               source_id, -1, trx_id),
                                TrxHandleDeleter());
        }

        void discard_trx(wsrep_trx_id_t trx_id);

        TrxHandlePtr get_conn_query(const TrxHandle::Params&,
                                    const wsrep_uuid_t&,
                                    wsrep_conn_id_t conn_id,
                                    bool create = false);

        void discard_conn(wsrep_conn_id_t conn_id);
        void discard_conn_query(wsrep_conn_id_t conn_id);

        Wsdb();
        ~Wsdb();

        void print(std::ostream& os) const;

    private:
        // Create new trx handle
        TrxHandlePtr create_trx(const TrxHandle::Params& params,
                                const wsrep_uuid_t&      source_id,
                                wsrep_trx_id_t           trx_id);

        Conn*      get_conn(wsrep_conn_id_t conn_id, bool create);

        static const size_t trx_mem_limit_ = 1 << 20;

        TrxHandle::LocalPool trx_pool_;

        TrxMap       trx_map_;
        ConnTrxMap   conn_trx_map_;
#ifdef HAVE_PSI_INTERFACE
        gu::MutexWithPFS
                     trx_mutex_;
        ConnMap      conn_map_;
        gu::MutexWithPFS
                     conn_mutex_;
#else
        gu::Mutex    trx_mutex_;
        ConnMap      conn_map_;
        gu::Mutex    conn_mutex_;
#endif /* HAVE_PSI_INTERFACE */
    };

    inline std::ostream& operator<<(std::ostream& os, const Wsdb& w)
    {
        w.print(os); return os;
    }
}


#endif // GALERA_WSDB_HPP
