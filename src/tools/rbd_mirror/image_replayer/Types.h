// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_RBD_MIRROR_IMAGE_REPLAYER_TYPES_H
#define STONE_RBD_MIRROR_IMAGE_REPLAYER_TYPES_H

namespace rbd {
namespace mirror {
namespace image_replayer {

enum HealthState {
  HEALTH_STATE_OK,
  HEALTH_STATE_WARNING,
  HEALTH_STATE_ERROR
};

} // namespace image_replayer
} // namespace mirror
} // namespace rbd

#endif // STONE_RBD_MIRROR_IMAGE_REPLAYER_TYPES_H
