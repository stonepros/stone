// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_DEEP_COPY_UTILS_H
#define STONE_LIBRBD_DEEP_COPY_UTILS_H

#include "include/common_fwd.h"
#include "include/rados/librados.hpp"
#include "librbd/Types.h"
#include "librbd/deep_copy/Types.h"

#include <boost/optional.hpp>

namespace librbd {
namespace deep_copy {
namespace util {

void compute_snap_map(StoneContext* cct,
                      librados::snap_t src_snap_id_start,
                      librados::snap_t src_snap_id_end,
                      const SnapIds& dst_snap_ids,
                      const SnapSeqs &snap_seqs,
                      SnapMap *snap_map);

} // namespace util
} // namespace deep_copy
} // namespace librbd

#endif // STONE_LIBRBD_DEEP_COPY_UTILS_H
