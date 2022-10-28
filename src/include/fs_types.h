// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#ifndef STONE_INCLUDE_FS_TYPES_H
#define STONE_INCLUDE_FS_TYPES_H

#include "types.h"
class JSONObj;

#define STONEFS_EBLOCKLISTED    108
#define STONEFS_EPERM           1
#define STONEFS_ESTALE          116
#define STONEFS_ENOSPC          28
#define STONEFS_ETIMEDOUT       110
#define STONEFS_EIO             5
#define STONEFS_ENOTCONN        107
#define STONEFS_EEXIST          17
#define STONEFS_EINTR           4
#define STONEFS_EINVAL          22
#define STONEFS_EBADF           9
#define STONEFS_EROFS           30
#define STONEFS_EAGAIN          11
#define STONEFS_EACCES          13
#define STONEFS_ELOOP           40
#define STONEFS_EISDIR          21
#define STONEFS_ENOENT          2
#define STONEFS_ENOTDIR         20
#define STONEFS_ENAMETOOLONG    36
#define STONEFS_EBUSY           16
#define STONEFS_EDQUOT          122
#define STONEFS_EFBIG           27
#define STONEFS_ERANGE          34
#define STONEFS_ENXIO           6
#define STONEFS_ECANCELED       125
#define STONEFS_ENODATA         61
#define STONEFS_EOPNOTSUPP      95
#define STONEFS_EXDEV           18
#define STONEFS_ENOMEM          12
#define STONEFS_ENOTRECOVERABLE 131
#define STONEFS_ENOSYS          38
#define STONEFS_EWOULDBLOCK     STONEFS_EAGAIN
#define STONEFS_ENOTEMPTY       39
#define STONEFS_EDEADLK         35
#define STONEFS_EDEADLOCK       STONEFS_EDEADLK
#define STONEFS_EDOM            33
#define STONEFS_EMLINK          31
#define STONEFS_ETIME           62
#define STONEFS_EOLDSNAPC       85

// taken from linux kernel: include/uapi/linux/fcntl.h
#define STONEFS_AT_FDCWD        -100    /* Special value used to indicate
                                          openat should use the current
                                          working directory. */

// --------------------------------------
// ino

typedef uint64_t _inodeno_t;

struct inodeno_t {
  _inodeno_t val;
  inodeno_t() : val(0) {}
  // cppcheck-suppress noExplicitConstructor
  inodeno_t(_inodeno_t v) : val(v) {}
  inodeno_t operator+=(inodeno_t o) { val += o.val; return *this; }
  operator _inodeno_t() const { return val; }

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    encode(val, bl);
  }
  void decode(ceph::buffer::list::const_iterator& p) {
    using ceph::decode;
    decode(val, p);
  }
} __attribute__ ((__may_alias__));
WRITE_CLASS_ENCODER(inodeno_t)

template<>
struct denc_traits<inodeno_t> {
  static constexpr bool supported = true;
  static constexpr bool featured = false;
  static constexpr bool bounded = true;
  static constexpr bool need_contiguous = true;
  static void bound_encode(const inodeno_t &o, size_t& p) {
    denc(o.val, p);
  }
  static void encode(const inodeno_t &o, ceph::buffer::list::contiguous_appender& p) {
    denc(o.val, p);
  }
  static void decode(inodeno_t& o, ceph::buffer::ptr::const_iterator &p) {
    denc(o.val, p);
  }
};

inline std::ostream& operator<<(std::ostream& out, const inodeno_t& ino) {
  return out << std::hex << "0x" << ino.val << std::dec;
}

namespace std {
template<>
struct hash<inodeno_t> {
  size_t operator()( const inodeno_t& x ) const {
    static rjhash<uint64_t> H;
    return H(x.val);
  }
};
} // namespace std


// file modes

inline bool file_mode_is_readonly(int mode) {
  return (mode & STONE_FILE_MODE_WR) == 0;
}


// dentries
#define MAX_DENTRY_LEN 255

// --
namespace ceph {
  class Formatter;
}
void dump(const ceph_file_layout& l, ceph::Formatter *f);
void dump(const ceph_dir_layout& l, ceph::Formatter *f);



// file_layout_t

struct file_layout_t {
  // file -> object mapping
  uint32_t stripe_unit;   ///< stripe unit, in bytes,
  uint32_t stripe_count;  ///< over this many objects
  uint32_t object_size;   ///< until objects are this big

  int64_t pool_id;        ///< rados pool id
  std::string pool_ns;         ///< rados pool namespace

  file_layout_t(uint32_t su=0, uint32_t sc=0, uint32_t os=0)
    : stripe_unit(su),
      stripe_count(sc),
      object_size(os),
      pool_id(-1) {
  }

  static file_layout_t get_default() {
    return file_layout_t(1<<22, 1, 1<<22);
  }

  uint64_t get_period() const {
    return static_cast<uint64_t>(stripe_count) * object_size;
  }

  void from_legacy(const ceph_file_layout& fl);
  void to_legacy(ceph_file_layout *fl) const;

  bool is_valid() const;

  void encode(ceph::buffer::list& bl, uint64_t features) const;
  void decode(ceph::buffer::list::const_iterator& p);
  void dump(ceph::Formatter *f) const;
  void decode_json(JSONObj *obj);
  static void generate_test_instances(std::list<file_layout_t*>& o);
};
WRITE_CLASS_ENCODER_FEATURES(file_layout_t)

WRITE_EQ_OPERATORS_5(file_layout_t, stripe_unit, stripe_count, object_size, pool_id, pool_ns);

std::ostream& operator<<(std::ostream& out, const file_layout_t &layout);

#endif
