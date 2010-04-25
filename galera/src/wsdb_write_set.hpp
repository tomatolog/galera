//
// Copyright (C) 2010 Codership Oy <info@codership.com>
//


#ifndef GALERA_WSDB_WRITE_SET_HPP
#define GALERA_WSDB_WRITE_SET_HPP

extern "C"
{
#include "wsdb_api.h"
}

#include <time.h>

namespace galera
{
    class WsdbWriteSet : public WriteSet
    {
    public:
        WsdbWriteSet(wsrep_trx_id_t trx_id = -1) 
            : 
            trx_id_(trx_id),
            write_set_(0),
            rbr_()
        { }
        
        enum wsdb_ws_type get_type() const
        {
            assert(write_set_ != 0);
            return write_set_->type;
        }

        enum wsdb_ws_level get_level() const
        {
            assert(write_set_ != 0);
            return write_set_->level;
        }
        
        wsrep_seqno_t get_last_seen_trx() const
        {
            assert(write_set_ != 0);
            return write_set_->last_seen_trx;
        }
        
        const gu::Buffer& get_rbr() const
        {
            return rbr_;
        }
        
        bool empty() const
        {
            return rbr_.empty();
        }
        
        void serialize(gu::Buffer& buf) const
        {
            XDR xdrs;
            size_t data_max(xdr_sizeof((xdrproc_t)xdr_wsdb_write_set, 
                                       (void *)write_set_) + 1);
            buf.resize(data_max);
            xdrmem_create(&xdrs, (char*)&buf[0], data_max, XDR_ENCODE);
            if (!xdr_wsdb_write_set(&xdrs, write_set_)) 
            {
                gu_throw_fatal << "xdr failed";
            }
        }

    private:
        friend class WsdbTrxHandle;
        WsdbWriteSet(const WsdbWriteSet&);
        void operator=(const WsdbWriteSet&);
        
        wsrep_trx_id_t trx_id_;
        struct wsdb_write_set* write_set_;
        gu::Buffer rbr_;
    };
}

#endif // GALERA_WSDB_WRITE_SET_HPP
