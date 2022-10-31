// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#pragma once

#include <map>
#include <string>
#include <seastar/core/future.hh>
#include "osd/osd_types.h"
#include "crimson/os/futurized_collection.h"

namespace stone::os {
  class Transaction;
}

namespace crimson::os {
  class FuturizedCollection;
  class FuturizedStore;
}

/// metadata shared across PGs, or put in another way,
/// metadata not specific to certain PGs.
class OSDMeta {
  template<typename T> using Ref = boost::intrusive_ptr<T>;

  crimson::os::FuturizedStore* store;
  Ref<crimson::os::FuturizedCollection> coll;

public:
  OSDMeta(Ref<crimson::os::FuturizedCollection> coll,
          crimson::os::FuturizedStore* store)
    : store{store}, coll{coll}
  {}

  auto collection() {
    return coll;
  }
  void create(stone::os::Transaction& t);

  void store_map(stone::os::Transaction& t,
                 epoch_t e, const bufferlist& m);
  seastar::future<bufferlist> load_map(epoch_t e);

  void store_superblock(stone::os::Transaction& t,
                        const OSDSuperblock& sb);
  seastar::future<OSDSuperblock> load_superblock();

  using ec_profile_t = std::map<std::string, std::string>;
  seastar::future<std::tuple<pg_pool_t,
			     std::string,
			     ec_profile_t>> load_final_pool_info(int64_t pool);
private:
  static ghobject_t osdmap_oid(epoch_t epoch);
  static ghobject_t final_pool_info_oid(int64_t pool);
  static ghobject_t superblock_oid();
};
