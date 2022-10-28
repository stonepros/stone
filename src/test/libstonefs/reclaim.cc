// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Tests for Stone delegation handling
 *
 * (c) 2017, Jeff Layton <jlayton@redhat.com>
 */

#include "gtest/gtest.h"
#include "include/stonefs/libstonefs.h"
#include "include/stat.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/uio.h>
#include <libgen.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/xattr.h>
#include <limits.h>
#endif

#ifdef __FreeBSD__
#include <sys/wait.h>
#endif


#include <map>
#include <vector>
#include <thread>
#include <atomic>

#define	STONEFS_RECLAIM_TIMEOUT		60

static int dying_client(int argc, char **argv)
{
  struct stone_mount_info *cmount;

  /* Caller must pass in the uuid */
  if (argc < 2)
    return 1;

  if (stone_create(&cmount, nullptr) != 0)
    return 1;

  if (stone_conf_read_file(cmount, nullptr) != 0)
    return 1;

  if (stone_conf_parse_env(cmount, nullptr) != 0)
    return 1;

  if (stone_init(cmount) != 0)
    return 1;

  stone_set_session_timeout(cmount, STONEFS_RECLAIM_TIMEOUT);

  if (stone_start_reclaim(cmount, argv[1], STONE_RECLAIM_RESET) != -ENOENT)
    return 1;

  stone_set_uuid(cmount, argv[1]);

  if (stone_mount(cmount, "/") != 0)
    return 1;

  Inode *root, *file;
  if (stone_ll_lookup_root(cmount, &root) != 0)
    return 1;

  Fh *fh;
  struct stone_statx stx;
  UserPerm *perms = stone_mount_perms(cmount);

  if (stone_ll_create(cmount, root, argv[1], 0666, O_RDWR|O_CREAT|O_EXCL,
		      &file, &fh, &stx, 0, 0, perms) != 0)
    return 1;

  return 0;
}

TEST(LibStoneFS, ReclaimReset) {
  pid_t		pid;
  char		uuid[256];
  const char	*exe = "/proc/self/exe";

  sprintf(uuid, "simplereclaim:%x", getpid());

  pid = fork();
  ASSERT_GE(pid, 0);
  if (pid == 0) {
    errno = 0;
    execl(exe, exe, uuid, nullptr);
    /* It won't be zero of course, which is sort of the point... */
    ASSERT_EQ(errno, 0);
  }

  /* parent - wait for child to exit */
  int ret;
  pid_t wp = wait(&ret);
  ASSERT_GE(wp, 0);
  ASSERT_EQ(WIFEXITED(ret), true);
  ASSERT_EQ(WEXITSTATUS(ret), 0);

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, nullptr), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, nullptr), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, nullptr));
  ASSERT_EQ(stone_init(cmount), 0);
  stone_set_session_timeout(cmount, STONEFS_RECLAIM_TIMEOUT);
  ASSERT_EQ(stone_start_reclaim(cmount, uuid, STONE_RECLAIM_RESET), 0);
  stone_set_uuid(cmount, uuid);
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  Inode *root, *file;
  ASSERT_EQ(stone_ll_lookup_root(cmount, &root), 0);
  UserPerm *perms = stone_mount_perms(cmount);
  struct stone_statx stx;
  ASSERT_EQ(stone_ll_lookup(cmount, root, uuid, &file, &stx, 0, 0, perms), 0);
  Fh *fh;
  ASSERT_EQ(stone_ll_open(cmount, file, O_WRONLY, &fh, perms), 0);

  stone_unmount(cmount);
  stone_release(cmount);
}

static int update_root_mode()
{
  struct stone_mount_info *admin;
  int r = stone_create(&admin, nullptr);
  if (r < 0)
    return r;
  stone_conf_read_file(admin, nullptr);
  stone_conf_parse_env(admin, nullptr);
  stone_conf_set(admin, "client_permissions", "false");
  r = stone_mount(admin, "/");
  if (r < 0)
    goto out;
  r = stone_chmod(admin, "/", 01777);
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

  if (argc > 1)
    return dying_client(argc, argv);

  srand(getpid());

  return RUN_ALL_TESTS();
}
