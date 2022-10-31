// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#ifndef STONE_OSDC_OBJECTERWRITEBACKHANDLER_H
#define STONE_OSDC_OBJECTERWRITEBACKHANDLER_H

#include "osdc/Objecter.h"
#include "osdc/WritebackHandler.h"

class ObjecterWriteback : public WritebackHandler {
 public:
  ObjecterWriteback(Objecter *o, Finisher *fin, stone::mutex *lock)
    : m_objecter(o),
      m_finisher(fin),
      m_lock(lock) { }
  ~ObjecterWriteback() override {}

  void read(const object_t& oid, uint64_t object_no,
		    const object_locator_t& oloc, uint64_t off, uint64_t len,
		    snapid_t snapid, bufferlist *pbl, uint64_t trunc_size,
		    __u32 trunc_seq, int op_flags,
                    const ZTracer::Trace &parent_trace,
                    Context *onfinish) override {
    m_objecter->read_trunc(oid, oloc, off, len, snapid, pbl, 0,
			   trunc_size, trunc_seq,
			   new C_OnFinisher(new C_Lock(m_lock, onfinish),
					    m_finisher));
  }

  bool may_copy_on_write(const object_t& oid, uint64_t read_off,
				 uint64_t read_len, snapid_t snapid) override {
    return false;
  }

  stone_tid_t write(const object_t& oid, const object_locator_t& oloc,
			   uint64_t off, uint64_t len,
			   const SnapContext& snapc, const bufferlist &bl,
			   stone::real_time mtime, uint64_t trunc_size,
			   __u32 trunc_seq, stone_tid_t journal_tid,
                           const ZTracer::Trace &parent_trace,
			   Context *oncommit) override {
    return m_objecter->write_trunc(oid, oloc, off, len, snapc, bl, mtime, 0,
				   trunc_size, trunc_seq,
				   new C_OnFinisher(new C_Lock(m_lock,
							       oncommit),
						    m_finisher));
  }

  bool can_scattered_write() override { return true; }
  using WritebackHandler::write;
  stone_tid_t write(const object_t& oid, const object_locator_t& oloc,
                           vector<pair<uint64_t, bufferlist> >& io_vec,
			   const SnapContext& snapc, stone::real_time mtime,
			   uint64_t trunc_size, __u32 trunc_seq,
			   Context *oncommit) override {
    ObjectOperation op;
    for (vector<pair<uint64_t, bufferlist> >::iterator p = io_vec.begin();
	 p != io_vec.end();
	 ++p)
      op.write(p->first, p->second, trunc_size, trunc_seq);

    return m_objecter->mutate(oid, oloc, op, snapc, mtime, 0,
			      new C_OnFinisher(new C_Lock(m_lock, oncommit),
					       m_finisher));
  }

 private:
  Objecter *m_objecter;
  Finisher *m_finisher;
  stone::mutex *m_lock;
};

#endif
