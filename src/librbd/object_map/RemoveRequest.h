// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_OBJECT_MAP_REMOVE_REQUEST_H
#define STONE_LIBRBD_OBJECT_MAP_REMOVE_REQUEST_H

#include "include/buffer.h"
#include "common/stone_mutex.h"
#include <map>
#include <string>

class Context;

namespace librbd {

class ImageCtx;

namespace object_map {

template <typename ImageCtxT = ImageCtx>
class RemoveRequest {
public:
  static RemoveRequest *create(ImageCtxT *image_ctx, Context *on_finish) {
    return new RemoveRequest(image_ctx, on_finish);
  }

  void send();

private:
  /**
   * @verbatim
   *
   * <start>
   *    |          .  .  .
   *    v          v     .
   * REMOVE_OBJECT_MAP   . (for every snapshot)
   *    |          .     .
   *    v          .  .  .
   * <finis>
   *
   * @endverbatim
   */

  RemoveRequest(ImageCtxT *image_ctx, Context *on_finish);

  ImageCtxT *m_image_ctx;
  Context *m_on_finish;

  int m_error_result = 0;
  int m_ref_counter = 0;
  mutable stone::mutex m_lock =
    stone::make_mutex("object_map::RemoveRequest::m_lock");

  void send_remove_object_map();
  Context *handle_remove_object_map(int *result);
};

} // namespace object_map
} // namespace librbd

extern template class librbd::object_map::RemoveRequest<librbd::ImageCtx>;

#endif // STONE_LIBRBD_OBJECT_MAP_REMOVE_REQUEST_H
