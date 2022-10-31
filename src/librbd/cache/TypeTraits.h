// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_CACHE_TYPE_TRAITS_H
#define STONE_LIBRBD_CACHE_TYPE_TRAITS_H

namespace stone {
namespace immutable_obj_cache {

class CacheClient;

} // namespace immutable_obj_cache
} // namespace stone

namespace librbd {
namespace cache {

template <typename ImageCtxT>
struct TypeTraits {
  typedef stone::immutable_obj_cache::CacheClient CacheClient;    
};

} // namespace librbd
} // namespace cache

#endif
