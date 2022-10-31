// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

#ifndef STONE_RGW_SYNC_MODULE_LOG_H
#define STONE_RGW_SYNC_MODULE_LOG_H

#include "rgw_sync_module.h"

class RGWLogSyncModule : public RGWSyncModule {
public:
  RGWLogSyncModule() {}
  bool supports_data_export() override {
    return false;
  }
  int create_instance(StoneContext *cct, const JSONFormattable& config, RGWSyncModuleInstanceRef *instance) override;
};

#endif
