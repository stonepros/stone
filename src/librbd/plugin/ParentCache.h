// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_PLUGIN_PARENT_CACHE_H
#define STONE_LIBRBD_PLUGIN_PARENT_CACHE_H

#include "librbd/plugin/Types.h"
#include "include/Context.h"

namespace librbd {

struct ImageCtx;

namespace plugin {

template <typename ImageCtxT>
class ParentCache : public Interface<ImageCtxT> {
public:
  ParentCache(StoneContext* cct) : Interface<ImageCtxT>(cct) {
  }

  void init(ImageCtxT* image_ctx, Api<ImageCtxT>& api,
            cache::ImageWritebackInterface& image_writeback,
            PluginHookPoints& hook_points_list,
            Context* on_finish) override;

private:
  void handle_init_parent_cache(int r, Context* on_finish);
  using ceph::Plugin::cct;

};

} // namespace plugin
} // namespace librbd

extern template class librbd::plugin::ParentCache<librbd::ImageCtx>;

#endif // STONE_LIBRBD_PLUGIN_PARENT_CACHE_H
