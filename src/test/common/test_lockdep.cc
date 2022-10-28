// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "gtest/gtest.h"

#include "common/stone_argparse.h"
#include "common/stone_context.h"
#include "common/stone_mutex.h"
#include "common/common_init.h"
#include "common/lockdep.h"
#include "include/util.h"
#include "include/coredumpctl.h"
#include "log/Log.h"

class lockdep : public ::testing::Test
{
protected:
  void SetUp() override {
#ifndef STONE_DEBUG_MUTEX
    GTEST_SKIP() << "WARNING: STONE_DEBUG_MUTEX is not defined, lockdep will not work";
#endif
    StoneInitParameters params(STONE_ENTITY_TYPE_CLIENT);
    cct = common_preinit(params, CODE_ENVIRONMENT_UTILITY,
			 CINIT_FLAG_NO_DEFAULT_CONFIG_FILE);
    cct->_conf->cluster = "stone";
    cct->_conf.set_val("lockdep", "true");
    cct->_conf.apply_changes(nullptr);
    ASSERT_TRUE(g_lockdep);
  }
  void TearDown() final
  {
    if (cct) {
      cct->put();
      cct = nullptr;
    }
  }
protected:
  StoneContext *cct = nullptr;
};

TEST_F(lockdep, abba)
{
  stone::mutex a(stone::make_mutex("a")), b(stone::make_mutex("b"));
  a.lock();
  ASSERT_TRUE(stone_mutex_is_locked(a));
  ASSERT_TRUE(stone_mutex_is_locked_by_me(a));
  b.lock();
  ASSERT_TRUE(stone_mutex_is_locked(b));
  ASSERT_TRUE(stone_mutex_is_locked_by_me(b));
  a.unlock();
  b.unlock();

  b.lock();
  PrCtl unset_dumpable;
  EXPECT_DEATH(a.lock(), "");
  b.unlock();
}

TEST_F(lockdep, recursive)
{
  stone::mutex a(stone::make_mutex("a"));
  a.lock();
  PrCtl unset_dumpable;
  EXPECT_DEATH(a.lock(), "");
  a.unlock();

  stone::recursive_mutex b(stone::make_recursive_mutex("b"));
  b.lock();
  ASSERT_TRUE(stone_mutex_is_locked(b));
  ASSERT_TRUE(stone_mutex_is_locked_by_me(b));
  b.lock();
  b.unlock();
  b.unlock();
}
