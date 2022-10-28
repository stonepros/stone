// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "gtest/gtest.h"
#include "common/stone_argparse.h"
#include "common/stone_crypto.h"
#include "common/config_proxy.h"
#include "global/global_context.h"
#include "global/global_init.h"
#include <vector>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  std::vector<const char*> args;
  argv_to_vec(argc, (const char **)argv, args);

  auto cct = global_init(NULL, args, STONE_ENTITY_TYPE_OSD,
			 CODE_ENVIRONMENT_UTILITY,
			 CINIT_FLAG_NO_MON_CONFIG);
  g_conf().set_val("lockdep", "true");
  common_init_finish(g_stone_context);

  int r = RUN_ALL_TESTS();
  return r;
}
