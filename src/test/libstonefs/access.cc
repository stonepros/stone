// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2011 New Dream Network
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include "gtest/gtest.h"
#include "common/stone_argparse.h"
#include "include/buffer.h"
#include "include/stringify.h"
#include "include/stonefs/libstonefs.h"
#include "include/rados/librados.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/uio.h>
#include <iostream>
#include <vector>
#include "json_spirit/json_spirit.h"

#ifdef __linux__
#include <limits.h>
#include <sys/xattr.h>
#endif


rados_t cluster;

string key;

int do_mon_command(string s, string *key)
{
  char *outs, *outbuf;
  size_t outs_len, outbuf_len;
  const char *ss = s.c_str();
  int r = rados_mon_command(cluster, (const char **)&ss, 1,
			    0, 0,
			    &outbuf, &outbuf_len,
			    &outs, &outs_len);
  if (outbuf_len) {
    string s(outbuf, outbuf_len);
    std::cout << "out: " << s << std::endl;

    // parse out the key
    json_spirit::mValue v, k;
    json_spirit::read_or_throw(s, v);
    k = v.get_array()[0].get_obj().find("key")->second;
    *key = k.get_str();
    std::cout << "key: " << *key << std::endl;
    free(outbuf);
  } else {
    return -EINVAL;
  }
  if (outs_len) {
    string s(outs, outs_len);
    std::cout << "outs: " << s << std::endl;
    free(outs);
  }
  return r;
}

string get_unique_dir()
{
  return string("/stone_test_libstonefs_access.") + stringify(rand());
}

TEST(AccessTest, Foo) {
  string dir = get_unique_dir();
  string user = "libstonefs_foo_test." + stringify(rand());
  // admin mount to set up test
  struct stone_mount_info *admin;
  ASSERT_EQ(0, stone_create(&admin, NULL));
  ASSERT_EQ(0, stone_conf_read_file(admin, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(admin, NULL));
  ASSERT_EQ(0, stone_mount(admin, "/"));
  ASSERT_EQ(0, stone_mkdir(admin, dir.c_str(), 0755));

  // create access key
  string key;
  ASSERT_EQ(0, do_mon_command(
      "{\"prefix\": \"auth get-or-create\", \"entity\": \"client." + user + "\", "
      "\"caps\": [\"mon\", \"allow *\", \"osd\", \"allow rw\", "
      "\"mds\", \"allow rw\""
      "], \"format\": \"json\"}", &key));

  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, user.c_str()));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_conf_set(cmount, "key", key.c_str()));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  stone_shutdown(cmount);

  // clean up
  ASSERT_EQ(0, stone_rmdir(admin, dir.c_str()));
  stone_shutdown(admin);
}

TEST(AccessTest, Path) {
  string good = get_unique_dir();
  string bad = get_unique_dir();
  string user = "libstonefs_path_test." + stringify(rand());
  struct stone_mount_info *admin;
  ASSERT_EQ(0, stone_create(&admin, NULL));
  ASSERT_EQ(0, stone_conf_read_file(admin, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(admin, NULL));
  ASSERT_EQ(0, stone_mount(admin, "/"));
  ASSERT_EQ(0, stone_mkdir(admin, good.c_str(), 0755));
  ASSERT_EQ(0, stone_mkdir(admin, string(good + "/p").c_str(), 0755));
  ASSERT_EQ(0, stone_mkdir(admin, bad.c_str(), 0755));
  ASSERT_EQ(0, stone_mkdir(admin, string(bad + "/p").c_str(), 0755));
  int fd = stone_open(admin, string(good + "/q").c_str(), O_CREAT|O_WRONLY, 0755);
  stone_close(admin, fd);
  fd = stone_open(admin, string(bad + "/q").c_str(), O_CREAT|O_WRONLY, 0755);
  stone_close(admin, fd);
  fd = stone_open(admin, string(bad + "/z").c_str(), O_CREAT|O_WRONLY, 0755);
  stone_write(admin, fd, "TEST FAILED", 11, 0);
  stone_close(admin, fd);

  string key;
  ASSERT_EQ(0, do_mon_command(
      "{\"prefix\": \"auth get-or-create\", \"entity\": \"client." + user + "\", "
      "\"caps\": [\"mon\", \"allow r\", \"osd\", \"allow rwx\", "
      "\"mds\", \"allow r, allow rw path=" + good + "\""
      "], \"format\": \"json\"}", &key));

  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, user.c_str()));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_conf_set(cmount, "key", key.c_str()));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  // allowed
  ASSERT_GE(stone_mkdir(cmount, string(good + "/x").c_str(), 0755), 0);
  ASSERT_GE(stone_rmdir(cmount, string(good + "/p").c_str()), 0);
  ASSERT_GE(stone_unlink(cmount, string(good + "/q").c_str()), 0);
  fd = stone_open(cmount, string(good + "/y").c_str(), O_CREAT|O_WRONLY, 0755);
  ASSERT_GE(fd, 0);
  stone_write(cmount, fd, "bar", 3, 0);
  stone_close(cmount, fd);
  ASSERT_GE(stone_unlink(cmount, string(good + "/y").c_str()), 0);
  ASSERT_GE(stone_rmdir(cmount, string(good + "/x").c_str()), 0);

  fd = stone_open(cmount, string(bad + "/z").c_str(), O_RDONLY, 0644);
  ASSERT_GE(fd, 0);
  stone_close(cmount, fd);

  // not allowed
  ASSERT_LT(stone_mkdir(cmount, string(bad + "/x").c_str(), 0755), 0);
  ASSERT_LT(stone_rmdir(cmount, string(bad + "/p").c_str()), 0);
  ASSERT_LT(stone_unlink(cmount, string(bad + "/q").c_str()), 0);
  fd = stone_open(cmount, string(bad + "/y").c_str(), O_CREAT|O_WRONLY, 0755);
  ASSERT_LT(fd, 0);

  // unlink open file
  fd = stone_open(cmount, string(good + "/unlinkme").c_str(), O_CREAT|O_WRONLY, 0755);
  stone_unlink(cmount, string(good + "/unlinkme").c_str());
  ASSERT_GE(stone_write(cmount, fd, "foo", 3, 0), 0);
  ASSERT_GE(stone_fchmod(cmount, fd, 0777), 0);
  ASSERT_GE(stone_ftruncate(cmount, fd, 0), 0);
  ASSERT_GE(stone_fsetxattr(cmount, fd, "user.any", "bar", 3, 0), 0);
  stone_close(cmount, fd);

  // rename open file
  fd = stone_open(cmount, string(good + "/renameme").c_str(), O_CREAT|O_WRONLY, 0755);
  ASSERT_EQ(stone_rename(admin, string(good + "/renameme").c_str(),
			string(bad + "/asdf").c_str()), 0);
  ASSERT_GE(stone_write(cmount, fd, "foo", 3, 0), 0);
  ASSERT_GE(stone_fchmod(cmount, fd, 0777), -EACCES);
  ASSERT_GE(stone_ftruncate(cmount, fd, 0), -EACCES);
  ASSERT_GE(stone_fsetxattr(cmount, fd, "user.any", "bar", 3, 0), -EACCES);
  stone_close(cmount, fd);

  stone_shutdown(cmount);
  ASSERT_EQ(0, stone_unlink(admin, string(bad + "/q").c_str()));
  ASSERT_EQ(0, stone_unlink(admin, string(bad + "/z").c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, string(bad + "/p").c_str()));
  ASSERT_EQ(0, stone_unlink(admin, string(bad + "/asdf").c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, good.c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, bad.c_str()));
  stone_shutdown(admin);
}

TEST(AccessTest, ReadOnly) {
  string dir = get_unique_dir();
  string dir2 = get_unique_dir();
  string user = "libstonefs_readonly_test." + stringify(rand());
  struct stone_mount_info *admin;
  ASSERT_EQ(0, stone_create(&admin, NULL));
  ASSERT_EQ(0, stone_conf_read_file(admin, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(admin, NULL));
  ASSERT_EQ(0, stone_mount(admin, "/"));
  ASSERT_EQ(0, stone_mkdir(admin, dir.c_str(), 0755));
  int fd = stone_open(admin, string(dir + "/out").c_str(), O_CREAT|O_WRONLY, 0755);
  stone_write(admin, fd, "foo", 3, 0);
  stone_close(admin,fd);

  string key;
  ASSERT_EQ(0, do_mon_command(
      "{\"prefix\": \"auth get-or-create\", \"entity\": \"client." + user + "\", "
      "\"caps\": [\"mon\", \"allow r\", \"osd\", \"allow rw\", "
      "\"mds\", \"allow r\""
      "], \"format\": \"json\"}", &key));

  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, user.c_str()));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_conf_set(cmount, "key", key.c_str()));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  // allowed
  fd = stone_open(cmount, string(dir + "/out").c_str(), O_RDONLY, 0644);
  ASSERT_GE(fd, 0);
  stone_close(cmount,fd);

  // not allowed
  fd = stone_open(cmount, string(dir + "/bar").c_str(), O_CREAT|O_WRONLY, 0755);
  ASSERT_LT(fd, 0);
  ASSERT_LT(stone_mkdir(cmount, dir2.c_str(), 0755), 0);

  stone_shutdown(cmount);
  ASSERT_EQ(0, stone_unlink(admin, string(dir + "/out").c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, dir.c_str()));
  stone_shutdown(admin);
}

TEST(AccessTest, User) {
  string dir = get_unique_dir();
  string user = "libstonefs_user_test." + stringify(rand());

  // admin mount to set up test
  struct stone_mount_info *admin;
  ASSERT_EQ(0, stone_create(&admin, NULL));
  ASSERT_EQ(0, stone_conf_read_file(admin, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(admin, NULL));
  ASSERT_EQ(0, stone_conf_set(admin, "client_permissions", "0"));
  ASSERT_EQ(0, stone_mount(admin, "/"));
  ASSERT_EQ(0, stone_mkdir(admin, dir.c_str(), 0755));

  // create access key
  string key;
  ASSERT_EQ(0, do_mon_command(
      "{\"prefix\": \"auth get-or-create\", \"entity\": \"client." + user + "\", "
      "\"caps\": [\"mon\", \"allow *\", \"osd\", \"allow rw\", "
      "\"mds\", \"allow rw uid=123 gids=456,789\""
      "], \"format\": \"json\"}", &key));

  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, user.c_str()));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_conf_set(cmount, "key", key.c_str()));
  ASSERT_EQ(-EACCES, stone_mount(cmount, "/"));
  ASSERT_EQ(0, stone_init(cmount));

  UserPerm *perms = stone_userperm_new(123, 456, 0, NULL);
  ASSERT_NE(nullptr, perms);
  ASSERT_EQ(0, stone_mount_perms_set(cmount, perms));
  stone_userperm_destroy(perms);

  ASSERT_EQ(0, stone_conf_set(cmount, "client_permissions", "0"));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  // user bits
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 0700));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 123, 456));
  ASSERT_EQ(0, stone_mkdir(cmount, string(dir + "/u1").c_str(), 0755));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 456));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));

  // group bits
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 0770));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 456));
  ASSERT_EQ(0, stone_mkdir(cmount, string(dir + "/u2").c_str(), 0755));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 2));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));

  // user overrides group
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 0470));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 123, 456));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));

  // other
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 0777));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 1));
  ASSERT_EQ(0, stone_mkdir(cmount, string(dir + "/u3").c_str(), 0755));
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 0770));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));

  // user and group overrides other
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 07));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 456));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 123, 1));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 123, 456));
  ASSERT_EQ(-EACCES, stone_mkdir(cmount, string(dir + "/no").c_str(), 0755));

  // chown and chgrp
  ASSERT_EQ(0, stone_chmod(admin, dir.c_str(), 0700));
  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 123, 456));
  // FIXME: Re-enable these 789 tests once we can set multiple GIDs via libstonefs/config
  // ASSERT_EQ(0, stone_chown(cmount, dir.c_str(), 123, 789));
  ASSERT_EQ(0, stone_chown(cmount, dir.c_str(), 123, 456));
  // ASSERT_EQ(0, stone_chown(cmount, dir.c_str(), -1, 789));
  ASSERT_EQ(0, stone_chown(cmount, dir.c_str(), -1, 456));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), 123, 1));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), 1, 456));

  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 1));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), 123, 456));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), 123, -1));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), -1, 456));

  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 1, 456));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), 123, 456));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), 123, -1));
  ASSERT_EQ(-EACCES, stone_chown(cmount, dir.c_str(), -1, 456));

  ASSERT_EQ(0, stone_chown(admin, dir.c_str(), 123, 1));
  ASSERT_EQ(0, stone_chown(cmount, dir.c_str(), -1, 456));
  // ASSERT_EQ(0, stone_chown(cmount, dir.c_str(), 123, 789));

  stone_shutdown(cmount);

  // clean up
  ASSERT_EQ(0, stone_rmdir(admin, string(dir + "/u1").c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, string(dir + "/u2").c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, string(dir + "/u3").c_str()));
  ASSERT_EQ(0, stone_rmdir(admin, dir.c_str()));
  stone_shutdown(admin);
}

static int update_root_mode()
{
  struct stone_mount_info *admin;
  int r = stone_create(&admin, NULL);
  if (r < 0)
    return r;
  stone_conf_read_file(admin, NULL);
  stone_conf_parse_env(admin, NULL);
  stone_conf_set(admin, "client_permissions", "false");
  r = stone_mount(admin, "/");
  if (r < 0)
    goto out;
  r = stone_chmod(admin, "/", 0777);
out:
  stone_shutdown(admin);
  return r;
}


int main(int argc, char **argv)
{
  int r = update_root_mode();
  if (r < 0)
    exit(1);

  ::testing::InitGoogleTest(&argc, argv);

  srand(getpid());

  r = rados_create(&cluster, NULL);
  if (r < 0)
    exit(1);
  
  r = rados_conf_read_file(cluster, NULL);
  if (r < 0)
    exit(1);

  rados_conf_parse_env(cluster, NULL);
  r = rados_connect(cluster);
  if (r < 0)
    exit(1);

  r = RUN_ALL_TESTS();

  rados_shutdown(cluster);

  return r;
}
