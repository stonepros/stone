// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_CACHE_OBJECT_CACHER_WRITEBACK_H
#define STONE_LIBRBD_CACHE_OBJECT_CACHER_WRITEBACK_H

#include "common/snap_types.h"
#include "osd/osd_types.h"
#include "osdc/WritebackHandler.h"
#include <queue>

class Context;

namespace librbd {

struct ImageCtx;

namespace cache {

class ObjectCacherWriteback : public WritebackHandler {
public:
  static const int READ_FLAGS_MASK  = 0xF000;
  static const int READ_FLAGS_SHIFT = 24;

  ObjectCacherWriteback(ImageCtx *ictx, stone::mutex& lock);

  // Note that oloc, trunc_size, and trunc_seq are ignored
  void read(const object_t& oid, uint64_t object_no,
            const object_locator_t& oloc, uint64_t off, uint64_t len,
            snapid_t snapid, bufferlist *pbl, uint64_t trunc_size,
            __u32 trunc_seq, int op_flags,
            const ZTracer::Trace &parent_trace, Context *onfinish) override;

  // Determine whether a read to this extent could be affected by a
  // write-triggered copy-on-write
  bool may_copy_on_write(const object_t& oid, uint64_t read_off,
                         uint64_t read_len, snapid_t snapid) override;

  // Note that oloc, trunc_size, and trunc_seq are ignored
  stone_tid_t write(const object_t& oid, const object_locator_t& oloc,
                   uint64_t off, uint64_t len,
                   const SnapContext& snapc, const bufferlist &bl,
                   stone::real_time mtime, uint64_t trunc_size,
                   __u32 trunc_seq, stone_tid_t journal_tid,
                   const ZTracer::Trace &parent_trace,
                   Context *oncommit) override;
  using WritebackHandler::write;

  void overwrite_extent(const object_t& oid, uint64_t off,
                        uint64_t len, stone_tid_t original_journal_tid,
                        stone_tid_t new_journal_tid) override;

  struct write_result_d {
    bool done;
    int ret;
    std::string oid;
    Context *oncommit;
    write_result_d(const std::string& oid, Context *oncommit) :
      done(false), ret(0), oid(oid), oncommit(oncommit) {}
  private:
    write_result_d(const write_result_d& rhs);
    const write_result_d& operator=(const write_result_d& rhs);
  };

private:
  void complete_writes(const std::string& oid);

  stone_tid_t m_tid;
  stone::mutex& m_lock;
  librbd::ImageCtx *m_ictx;
  stone::unordered_map<std::string, std::queue<write_result_d*> > m_writes;
  friend class C_OrderedWrite;
};

} // namespace cache
} // namespace librbd

#endif // STONE_LIBRBD_CACHE_OBJECT_CACHER_WRITEBACK_H
