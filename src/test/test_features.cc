// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#include <stdio.h>

#include "global/global_init.h"
#include "common/stone_argparse.h"
#include "common/stone_releases.h"
#include "common/stone_strings.h"
#include "global/global_context.h"
#include "gtest/gtest.h"
#include "include/stone_features.h"


TEST(features, release_features)
{
  for (int r = 1; r < STONE_RELEASE_MAX; ++r) {
    const char *name = stone_release_name(r);
    ASSERT_NE(string("unknown"), name);
    ASSERT_EQ(stone_release_t{static_cast<uint8_t>(r)},
	      stone_release_from_name(name));
    uint64_t features = stone_release_features(r);
    int rr = stone_release_from_features(features);
    cout << r << " " << name << " features 0x" << std::hex << features
	 << std::dec << " looks like " << stone_release_name(rr) << std::endl;
    EXPECT_LE(rr, r);
  }
}

TEST(features, release_from_features) {
  ASSERT_EQ(STONE_RELEASE_JEWEL, stone_release_from_features(575862587619852283));
  ASSERT_EQ(STONE_RELEASE_LUMINOUS,
	    stone_release_from_features(1152323339925389307));
}

int main(int argc, char **argv)
{
  vector<const char*> args;
  argv_to_vec(argc, (const char **)argv, args);

  auto cct = global_init(NULL, args, STONE_ENTITY_TYPE_CLIENT,
			 CODE_ENVIRONMENT_UTILITY,
			 CINIT_FLAG_NO_DEFAULT_CONFIG_FILE);
  common_init_finish(g_stone_context);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
