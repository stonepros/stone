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

#include "include/compat.h"
#include "gtest/gtest.h"
#include "include/stonefs/libstonefs.h"
#include "mds/mdstypes.h"
#include "include/stat.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "common/Clock.h"

#ifdef __linux__
#include <limits.h>
#include <sys/xattr.h>
#endif

#include <fmt/format.h>
#include <map>
#include <vector>
#include <thread>

#ifndef ALLPERMS
#define ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

TEST(LibStoneFS, OpenEmptyComponent) {

  pid_t mypid = getpid();
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  char c_dir[1024];
  sprintf(c_dir, "/open_test_%d", mypid);
  struct stone_dir_result *dirp;

  ASSERT_EQ(0, stone_mkdirs(cmount, c_dir, 0777));

  ASSERT_EQ(0, stone_opendir(cmount, c_dir, &dirp));

  char c_path[1024];
  sprintf(c_path, "/open_test_%d//created_file_%d", mypid, mypid);
  int fd = stone_open(cmount, c_path, O_RDONLY|O_CREAT, 0666);
  ASSERT_LT(0, fd);

  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_closedir(cmount, dirp));
  stone_shutdown(cmount);

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));

  ASSERT_EQ(0, stone_mount(cmount, "/"));

  fd = stone_open(cmount, c_path, O_RDONLY, 0666);
  ASSERT_LT(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  // cleanup
  ASSERT_EQ(0, stone_unlink(cmount, c_path));
  ASSERT_EQ(0, stone_rmdir(cmount, c_dir));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, OpenReadTruncate) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  auto path = fmt::format("test_open_rdt_{}", getpid());
  int fd = stone_open(cmount, path.c_str(), O_WRONLY|O_CREAT, 0666);
  ASSERT_LE(0, fd);

  auto data = std::string("hello world");
  ASSERT_EQ(stone_write(cmount, fd, data.c_str(), data.size(), 0), (int)data.size());
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, path.c_str(), O_RDONLY, 0);
  ASSERT_LE(0, fd);
  ASSERT_EQ(stone_ftruncate(cmount, fd, 0), -EBADF);
  ASSERT_EQ(stone_ftruncate(cmount, fd, 1), -EBADF);
  ASSERT_EQ(0, stone_close(cmount, fd));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, OpenReadWrite) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  char c_path[1024];
  sprintf(c_path, "test_open_rdwr_%d", getpid());
  int fd = stone_open(cmount, c_path, O_WRONLY|O_CREAT, 0666);
  ASSERT_LT(0, fd);

  const char *out_buf = "hello world";
  size_t size = strlen(out_buf);
  char in_buf[100];
  ASSERT_EQ(stone_write(cmount, fd, out_buf, size, 0), (int)size);
  ASSERT_EQ(stone_read(cmount, fd, in_buf, sizeof(in_buf), 0), -EBADF);
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, c_path, O_RDONLY, 0);
  ASSERT_LT(0, fd);
  ASSERT_EQ(stone_write(cmount, fd, out_buf, size, 0), -EBADF);
  ASSERT_EQ(stone_read(cmount, fd, in_buf, sizeof(in_buf), 0), (int)size);
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, c_path, O_RDWR, 0);
  ASSERT_LT(0, fd);
  ASSERT_EQ(stone_write(cmount, fd, out_buf, size, 0), (int)size);
  ASSERT_EQ(stone_read(cmount, fd, in_buf, sizeof(in_buf), 0), (int)size);
  ASSERT_EQ(0, stone_close(cmount, fd));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, MountNonExist) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_NE(0, stone_mount(cmount, "/non-exist"));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, MountDouble) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));
  ASSERT_EQ(-EISCONN, stone_mount(cmount, "/"));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, MountRemount) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));

  StoneContext *cct = stone_get_mount_context(cmount);
  ASSERT_EQ(0, stone_mount(cmount, "/"));
  ASSERT_EQ(0, stone_unmount(cmount));

  ASSERT_EQ(0, stone_mount(cmount, "/"));
  ASSERT_EQ(cct, stone_get_mount_context(cmount));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, UnmountUnmounted) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(-ENOTCONN, stone_unmount(cmount));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, ReleaseUnmounted) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_release(cmount));
}

TEST(LibStoneFS, ReleaseMounted) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));
  ASSERT_EQ(-EISCONN, stone_release(cmount));
  ASSERT_EQ(0, stone_unmount(cmount));
  ASSERT_EQ(0, stone_release(cmount));
}

TEST(LibStoneFS, UnmountRelease) {

  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));
  ASSERT_EQ(0, stone_unmount(cmount));
  ASSERT_EQ(0, stone_release(cmount));
}

TEST(LibStoneFS, Mount) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);
  stone_shutdown(cmount);

  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, OpenLayout) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  /* valid layout */
  char test_layout_file[256];
  sprintf(test_layout_file, "test_layout_%d_b", getpid());
  int fd = stone_open_layout(cmount, test_layout_file, O_CREAT|O_WRONLY, 0666, (1<<20), 7, (1<<20), NULL);
  ASSERT_GT(fd, 0);
  char poolname[80];
  ASSERT_LT(0, stone_get_file_pool_name(cmount, fd, poolname, sizeof(poolname)));
  ASSERT_LT(0, stone_get_file_pool_name(cmount, fd, poolname, 0));

  /* on already-written file (ENOTEMPTY) */
  stone_write(cmount, fd, "hello world", 11, 0);
  stone_close(cmount, fd);

  char xattrk[128];
  char xattrv[128];
  sprintf(xattrk, "stone.file.layout.stripe_unit");
  sprintf(xattrv, "65536");
  ASSERT_EQ(-ENOTEMPTY, stone_setxattr(cmount, test_layout_file, xattrk, (void *)xattrv, 5, 0));

  /* invalid layout */
  sprintf(test_layout_file, "test_layout_%d_c", getpid());
  fd = stone_open_layout(cmount, test_layout_file, O_CREAT, 0666, (1<<20), 1, 19, NULL);
  ASSERT_EQ(fd, -EINVAL);

  /* with data pool */
  sprintf(test_layout_file, "test_layout_%d_d", getpid());
  fd = stone_open_layout(cmount, test_layout_file, O_CREAT, 0666, (1<<20), 7, (1<<20), poolname);
  ASSERT_GT(fd, 0);
  stone_close(cmount, fd);

  /* with metadata pool (invalid) */
  sprintf(test_layout_file, "test_layout_%d_e", getpid());
  fd = stone_open_layout(cmount, test_layout_file, O_CREAT, 0666, (1<<20), 7, (1<<20), "metadata");
  ASSERT_EQ(fd, -EINVAL);

  /* with metadata pool (does not exist) */
  sprintf(test_layout_file, "test_layout_%d_f", getpid());
  fd = stone_open_layout(cmount, test_layout_file, O_CREAT, 0666, (1<<20), 7, (1<<20), "asdfjasdfjasdf");
  ASSERT_EQ(fd, -EINVAL);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, DirLs) {

  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  struct stone_dir_result *ls_dir = NULL;
  char foostr[256];
  sprintf(foostr, "dir_ls%d", mypid);
  ASSERT_EQ(stone_opendir(cmount, foostr, &ls_dir), -ENOENT);

  ASSERT_EQ(stone_mkdir(cmount, foostr, 0777), 0);
  struct stone_statx stx;
  ASSERT_EQ(stone_statx(cmount, foostr, &stx, 0, 0), 0);
  ASSERT_NE(S_ISDIR(stx.stx_mode), 0);

  char barstr[256];
  sprintf(barstr, "dir_ls2%d", mypid);
  ASSERT_EQ(stone_statx(cmount, barstr, &stx, 0, AT_SYMLINK_NOFOLLOW), -ENOENT);

  // insert files into directory and test open
  char bazstr[256];
  int i = 0, r = rand() % 4096;
  if (getenv("LIBSTONEFS_RAND")) {
    r = atoi(getenv("LIBSTONEFS_RAND"));
  }
  printf("rand: %d\n", r);
  for(; i < r; ++i) {

    sprintf(bazstr, "dir_ls%d/dirf%d", mypid, i);
    int fd  = stone_open(cmount, bazstr, O_CREAT|O_RDONLY, 0666);
    ASSERT_GT(fd, 0);
    ASSERT_EQ(stone_close(cmount, fd), 0);

    // set file sizes for readdirplus
    stone_truncate(cmount, bazstr, i);
  }

  ASSERT_EQ(stone_opendir(cmount, foostr, &ls_dir), 0);

  // not guaranteed to get . and .. first, but its a safe assumption in this case
  struct dirent *result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");

  std::vector<std::string> entries;
  std::map<std::string, int64_t> offset_map;
  int64_t offset = stone_telldir(cmount, ls_dir);
  for(i = 0; i < r; ++i) {
    result = stone_readdir(cmount, ls_dir);
    ASSERT_TRUE(result != NULL);
    entries.push_back(result->d_name);
    offset_map[result->d_name] = offset;
    offset = stone_telldir(cmount, ls_dir);
  }

  ASSERT_TRUE(stone_readdir(cmount, ls_dir) == NULL);
  offset = stone_telldir(cmount, ls_dir);

  ASSERT_EQ(offset_map.size(), entries.size());
  for(i = 0; i < r; ++i) {
    sprintf(bazstr, "dirf%d", i);
    ASSERT_TRUE(offset_map.count(bazstr) == 1);
  }

  // test seekdir
  stone_seekdir(cmount, ls_dir, offset);
  ASSERT_TRUE(stone_readdir(cmount, ls_dir) == NULL);

  for (auto p = offset_map.begin(); p != offset_map.end(); ++p) {
    stone_seekdir(cmount, ls_dir, p->second);
    result = stone_readdir(cmount, ls_dir);
    ASSERT_TRUE(result != NULL);
    std::string d_name(result->d_name);
    ASSERT_EQ(p->first, d_name);
  }

  // test rewinddir
  stone_rewinddir(cmount, ls_dir);

  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");

  stone_rewinddir(cmount, ls_dir);

  int t = stone_telldir(cmount, ls_dir);
  ASSERT_GT(t, -1);

  ASSERT_TRUE(stone_readdir(cmount, ls_dir) != NULL);

  // test seekdir - move back to the beginning
  stone_seekdir(cmount, ls_dir, t);

  // test getdents
  struct dirent *getdents_entries;
  size_t getdents_entries_len = (r + 2) * sizeof(*getdents_entries);
  getdents_entries = (struct dirent *)malloc(getdents_entries_len);

  int count = 0;
  std::vector<std::string> found;
  while (true) {
    int len = stone_getdents(cmount, ls_dir, (char *)getdents_entries, getdents_entries_len);
    if (len == 0)
      break;
    ASSERT_GT(len, 0);
    ASSERT_TRUE((len % sizeof(*getdents_entries)) == 0);
    int n = len / sizeof(*getdents_entries);
    int j;
    if (count == 0) {
      ASSERT_STREQ(getdents_entries[0].d_name, ".");
      ASSERT_STREQ(getdents_entries[1].d_name, "..");
      j = 2;
    } else {
      j = 0;
    }
    count += n;
    for(; j < n; ++i, ++j) {
      const char *name = getdents_entries[j].d_name;
      found.push_back(name);
    }
  }
  ASSERT_EQ(found, entries);
  free(getdents_entries);

  // test readdir_r
  stone_rewinddir(cmount, ls_dir);

  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");

  found.clear();
  while (true) {
    struct dirent rdent;
    int len = stone_readdir_r(cmount, ls_dir, &rdent);
    if (len == 0)
      break;
    ASSERT_EQ(len, 1);
    found.push_back(rdent.d_name);
  }
  ASSERT_EQ(found, entries);

  // test readdirplus
  stone_rewinddir(cmount, ls_dir);

  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");

  found.clear();
  while (true) {
    struct dirent rdent;
    struct stone_statx stx;
    int len = stone_readdirplus_r(cmount, ls_dir, &rdent, &stx,
				 STONE_STATX_SIZE, AT_NO_ATTR_SYNC, NULL);
    if (len == 0)
      break;
    ASSERT_EQ(len, 1);
    const char *name = rdent.d_name;
    found.push_back(name);
    int size;
    sscanf(name, "dirf%d", &size);
    ASSERT_TRUE(stx.stx_mask & STONE_STATX_SIZE);
    ASSERT_EQ(stx.stx_size, (size_t)size);
    ASSERT_EQ(stx.stx_ino, rdent.d_ino);
    //ASSERT_EQ(st.st_mode, (mode_t)0666);
  }
  ASSERT_EQ(found, entries);

  ASSERT_EQ(stone_closedir(cmount, ls_dir), 0);

  // cleanup
  for(i = 0; i < r; ++i) {
    sprintf(bazstr, "dir_ls%d/dirf%d", mypid, i);
    ASSERT_EQ(0, stone_unlink(cmount, bazstr));
  }
  ASSERT_EQ(0, stone_rmdir(cmount, foostr));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, ManyNestedDirs) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  const char *many_path = "a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a";
  ASSERT_EQ(stone_mkdirs(cmount, many_path, 0755), 0);

  int i = 0;

  for(; i < 39; ++i) {
    ASSERT_EQ(stone_chdir(cmount, "a"), 0);

    struct stone_dir_result *dirp;
    ASSERT_EQ(stone_opendir(cmount, "a", &dirp), 0);
    struct dirent *dent = stone_readdir(cmount, dirp);
    ASSERT_TRUE(dent != NULL);
    ASSERT_STREQ(dent->d_name, ".");
    dent = stone_readdir(cmount, dirp);
    ASSERT_TRUE(dent != NULL);
    ASSERT_STREQ(dent->d_name, "..");
    dent = stone_readdir(cmount, dirp);
    ASSERT_TRUE(dent != NULL);
    ASSERT_STREQ(dent->d_name, "a");
    ASSERT_EQ(stone_closedir(cmount, dirp), 0);
  }

  ASSERT_STREQ(stone_getcwd(cmount), "/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a");

  ASSERT_EQ(stone_chdir(cmount, "a/a/a"), 0);

  for(i = 0; i < 39; ++i) {
    ASSERT_EQ(stone_chdir(cmount, ".."), 0);
    ASSERT_EQ(stone_rmdir(cmount, "a"), 0);
  }

  ASSERT_EQ(stone_chdir(cmount, "/"), 0);

  ASSERT_EQ(stone_rmdir(cmount, "a/a/a"), 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Xattrs) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_xattr_file[256];
  sprintf(test_xattr_file, "test_xattr_%d", getpid());
  int fd = stone_open(cmount, test_xattr_file, O_CREAT, 0666);
  ASSERT_GT(fd, 0);

  // test removing non-existent xattr
  ASSERT_EQ(-ENODATA, stone_removexattr(cmount, test_xattr_file, "user.nosuchxattr"));

  char i = 'a';
  char xattrk[128];
  char xattrv[128];
  for(; i < 'a'+26; ++i) {
    sprintf(xattrk, "user.test_xattr_%c", i);
    int len = sprintf(xattrv, "testxattr%c", i);
    ASSERT_EQ(stone_setxattr(cmount, test_xattr_file, xattrk, (void *) xattrv, len, XATTR_CREATE), 0);
  }

  // zero size should return required buffer length
  int len_needed = stone_listxattr(cmount, test_xattr_file, NULL, 0);
  ASSERT_GT(len_needed, 0);

  // buffer size smaller than needed should fail
  char xattrlist[128*26];
  ASSERT_GT(sizeof(xattrlist), (size_t)len_needed);
  int len = stone_listxattr(cmount, test_xattr_file, xattrlist, len_needed - 1);
  ASSERT_EQ(-ERANGE, len);

  len = stone_listxattr(cmount, test_xattr_file, xattrlist, sizeof(xattrlist));
  ASSERT_EQ(len, len_needed);
  char *p = xattrlist;
  char *n;
  i = 'a';
  while (len > 0) {
    // stone.* xattrs should not be listed
    ASSERT_NE(strncmp(p, "stone.", 5), 0);

    sprintf(xattrk, "user.test_xattr_%c", i);
    ASSERT_STREQ(p, xattrk);

    char gxattrv[128];
    std::cout << "getting attr " << p << std::endl;
    int alen = stone_getxattr(cmount, test_xattr_file, p, (void *) gxattrv, 128);
    ASSERT_GT(alen, 0);
    sprintf(xattrv, "testxattr%c", i);
    ASSERT_TRUE(!strncmp(xattrv, gxattrv, alen));

    n = index(p, '\0');
    n++;
    len -= (n - p);
    p = n;
    ++i;
  }

  i = 'a';
  for(i = 'a'; i < 'a'+26; ++i) {
    sprintf(xattrk, "user.test_xattr_%c", i);
    ASSERT_EQ(stone_removexattr(cmount, test_xattr_file, xattrk), 0);
  }

  stone_close(cmount, fd);
  stone_shutdown(cmount);

}

TEST(LibStoneFS, Xattrs_ll) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_xattr_file[256];
  sprintf(test_xattr_file, "test_xattr_%d", getpid());
  int fd = stone_open(cmount, test_xattr_file, O_CREAT, 0666);
  ASSERT_GT(fd, 0);
  stone_close(cmount, fd);

  Inode *root = NULL;
  Inode *existent_file_handle = NULL;

  int res = stone_ll_lookup_root(cmount, &root);
  ASSERT_EQ(res, 0);

  UserPerm *perms = stone_mount_perms(cmount);
  struct stone_statx stx;

  res = stone_ll_lookup(cmount, root, test_xattr_file, &existent_file_handle,
		       &stx, 0, 0, perms);
  ASSERT_EQ(res, 0);

  const char *valid_name = "user.attrname";
  const char *value = "attrvalue";
  char value_buf[256] = { 0 };

  res = stone_ll_setxattr(cmount, existent_file_handle, valid_name, value, strlen(value), 0, perms);
  ASSERT_EQ(res, 0);

  res = stone_ll_getxattr(cmount, existent_file_handle, valid_name, value_buf, 256, perms);
  ASSERT_EQ(res, (int)strlen(value));

  value_buf[res] = '\0';
  ASSERT_STREQ(value_buf, value);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, LstatSlashdot) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  struct stone_statx stx;
  ASSERT_EQ(stone_statx(cmount, "/.", &stx, 0, AT_SYMLINK_NOFOLLOW), 0);
  ASSERT_EQ(stone_statx(cmount, ".", &stx, 0, AT_SYMLINK_NOFOLLOW), 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, StatDirNlink) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_dir1[256];
  sprintf(test_dir1, "dir1_symlinks_%d", getpid());
  ASSERT_EQ(stone_mkdir(cmount, test_dir1, 0700), 0);

  int fd = stone_open(cmount, test_dir1, O_DIRECTORY|O_RDONLY, 0);
  ASSERT_GT(fd, 0);
  struct stone_statx stx;
  ASSERT_EQ(stone_fstatx(cmount, fd, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
  ASSERT_EQ(stx.stx_nlink, 2u);

  {
    char test_dir2[296];
    sprintf(test_dir2, "%s/.", test_dir1);
    ASSERT_EQ(stone_statx(cmount, test_dir2, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
    ASSERT_EQ(stx.stx_nlink, 2u);
  }

  {
    char test_dir2[296];
    sprintf(test_dir2, "%s/1", test_dir1);
    ASSERT_EQ(stone_mkdir(cmount, test_dir2, 0700), 0);
    ASSERT_EQ(stone_statx(cmount, test_dir2, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
    ASSERT_EQ(stx.stx_nlink, 2u);
      ASSERT_EQ(stone_statx(cmount, test_dir1, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
      ASSERT_EQ(stx.stx_nlink, 3u);
    sprintf(test_dir2, "%s/2", test_dir1);
    ASSERT_EQ(stone_mkdir(cmount, test_dir2, 0700), 0);
      ASSERT_EQ(stone_statx(cmount, test_dir1, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
      ASSERT_EQ(stx.stx_nlink, 4u);
    sprintf(test_dir2, "%s/1/1", test_dir1);
    ASSERT_EQ(stone_mkdir(cmount, test_dir2, 0700), 0);
      ASSERT_EQ(stone_statx(cmount, test_dir1, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
      ASSERT_EQ(stx.stx_nlink, 4u);
    ASSERT_EQ(stone_rmdir(cmount, test_dir2), 0);
      ASSERT_EQ(stone_statx(cmount, test_dir1, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
      ASSERT_EQ(stx.stx_nlink, 4u);
    sprintf(test_dir2, "%s/1", test_dir1);
    ASSERT_EQ(stone_rmdir(cmount, test_dir2), 0);
      ASSERT_EQ(stone_statx(cmount, test_dir1, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
      ASSERT_EQ(stx.stx_nlink, 3u);
    sprintf(test_dir2, "%s/2", test_dir1);
    ASSERT_EQ(stone_rmdir(cmount, test_dir2), 0);
      ASSERT_EQ(stone_statx(cmount, test_dir1, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
      ASSERT_EQ(stx.stx_nlink, 2u);
  }

  ASSERT_EQ(stone_rmdir(cmount, test_dir1), 0);
  ASSERT_EQ(stone_fstatx(cmount, fd, &stx, STONE_STATX_NLINK, AT_SYMLINK_NOFOLLOW), 0);
  ASSERT_EQ(stx.stx_nlink, 0u);

  stone_close(cmount, fd);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, DoubleChmod) {

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  sprintf(test_file, "test_perms_%d", getpid());

  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  // write some stuff
  const char *bytes = "foobarbaz";
  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));

  stone_close(cmount, fd);

  // set perms to read but can't write
  ASSERT_EQ(stone_chmod(cmount, test_file, 0400), 0);

  fd = stone_open(cmount, test_file, O_RDWR, 0);
  ASSERT_EQ(fd, -EACCES);

  fd = stone_open(cmount, test_file, O_RDONLY, 0);
  ASSERT_GT(fd, -1);

  char buf[100];
  int ret = stone_read(cmount, fd, buf, 100, 0);
  ASSERT_EQ(ret, (int)strlen(bytes));
  buf[ret] = '\0';
  ASSERT_STREQ(buf, bytes);

  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), -EBADF);

  stone_close(cmount, fd);

  // reset back to writeable
  ASSERT_EQ(stone_chmod(cmount, test_file, 0600), 0);

  // ensure perms are correct
  struct stone_statx stx;
  ASSERT_EQ(stone_statx(cmount, test_file, &stx, STONE_STATX_MODE, AT_SYMLINK_NOFOLLOW), 0);
  ASSERT_EQ(stx.stx_mode, 0100600U);

  fd = stone_open(cmount, test_file, O_RDWR, 0);
  ASSERT_GT(fd, 0);

  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));
  stone_close(cmount, fd);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Fchmod) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  sprintf(test_file, "test_perms_%d", getpid());

  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  // write some stuff
  const char *bytes = "foobarbaz";
  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));

  // set perms to read but can't write
  ASSERT_EQ(stone_fchmod(cmount, fd, 0400), 0);

  char buf[100];
  int ret = stone_read(cmount, fd, buf, 100, 0);
  ASSERT_EQ(ret, (int)strlen(bytes));
  buf[ret] = '\0';
  ASSERT_STREQ(buf, bytes);

  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));

  stone_close(cmount, fd);

  ASSERT_EQ(stone_open(cmount, test_file, O_RDWR, 0), -EACCES);

  // reset back to writeable
  ASSERT_EQ(stone_chmod(cmount, test_file, 0600), 0);

  fd = stone_open(cmount, test_file, O_RDWR, 0);
  ASSERT_GT(fd, 0);

  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));
  stone_close(cmount, fd);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Lchmod) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  sprintf(test_file, "test_perms_lchmod_%d", getpid());

  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  // write some stuff
  const char *bytes = "foobarbaz";
  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));
  stone_close(cmount, fd);

  // Create symlink
  char test_symlink[256];
  sprintf(test_symlink, "test_lchmod_sym_%d", getpid());
  ASSERT_EQ(stone_symlink(cmount, test_file, test_symlink), 0);

  // get symlink stat - lstat
  struct stone_statx stx_orig1;
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx_orig1, STONE_STATX_ALL_STATS, AT_SYMLINK_NOFOLLOW), 0);

  // Change mode on symlink file
  ASSERT_EQ(stone_lchmod(cmount, test_symlink, 0400), 0);
  struct stone_statx stx_orig2;
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx_orig2, STONE_STATX_ALL_STATS, AT_SYMLINK_NOFOLLOW), 0);

  // Compare modes
  ASSERT_NE(stx_orig1.stx_mode, stx_orig2.stx_mode);
  static const int permbits = S_IRWXU|S_IRWXG|S_IRWXO;
  ASSERT_EQ(permbits&stx_orig1.stx_mode, 0777);
  ASSERT_EQ(permbits&stx_orig2.stx_mode, 0400);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Fchown) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  sprintf(test_file, "test_fchown_%d", getpid());

  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  // set perms to readable and writeable only by owner
  ASSERT_EQ(stone_fchmod(cmount, fd, 0600), 0);

  // change ownership to nobody -- we assume nobody exists and id is always 65534
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "0"), 0);
  ASSERT_EQ(stone_fchown(cmount, fd, 65534, 65534), 0);
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "1"), 0);

  stone_close(cmount, fd);

  fd = stone_open(cmount, test_file, O_RDWR, 0);
  ASSERT_EQ(fd, -EACCES);

  stone_shutdown(cmount);
}

#if defined(__linux__) && defined(O_PATH)
TEST(LibStoneFS, FlagO_PATH) {
  struct stone_mount_info *cmount;

  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, NULL));

  char test_file[PATH_MAX];
  sprintf(test_file, "test_oflag_%d", getpid());

  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR|O_PATH, 0666);
  ASSERT_EQ(-ENOENT, fd);

  fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);
  ASSERT_EQ(0, stone_close(cmount, fd));

  // ok, the file has been created. perform real checks now
  fd = stone_open(cmount, test_file, O_CREAT|O_RDWR|O_PATH, 0666);
  ASSERT_GT(fd, 0);

  char buf[128];
  ASSERT_EQ(-EBADF, stone_read(cmount, fd, buf, sizeof(buf), 0));
  ASSERT_EQ(-EBADF, stone_write(cmount, fd, buf, sizeof(buf), 0));

  // set perms to readable and writeable only by owner
  ASSERT_EQ(-EBADF, stone_fchmod(cmount, fd, 0600));

  // change ownership to nobody -- we assume nobody exists and id is always 65534
  ASSERT_EQ(-EBADF, stone_fchown(cmount, fd, 65534, 65534));

  // try to sync
  ASSERT_EQ(-EBADF, stone_fsync(cmount, fd, false));

  struct stone_statx stx;
  ASSERT_EQ(0, stone_fstatx(cmount, fd, &stx, 0, 0));

  ASSERT_EQ(0, stone_close(cmount, fd));
  stone_shutdown(cmount);
}
#endif /* __linux */

TEST(LibStoneFS, Symlinks) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  sprintf(test_file, "test_symlinks_%d", getpid());

  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  stone_close(cmount, fd);

  char test_symlink[256];
  sprintf(test_symlink, "test_symlinks_sym_%d", getpid());

  ASSERT_EQ(stone_symlink(cmount, test_file, test_symlink), 0);

  // test the O_NOFOLLOW case
  fd = stone_open(cmount, test_symlink, O_NOFOLLOW, 0);
  ASSERT_EQ(fd, -ELOOP);

  // stat the original file
  struct stone_statx stx_orig;
  ASSERT_EQ(stone_statx(cmount, test_file, &stx_orig, STONE_STATX_ALL_STATS, 0), 0);
  // stat the symlink
  struct stone_statx stx_symlink_orig;
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx_symlink_orig, STONE_STATX_ALL_STATS, 0), 0);
  // ensure the statx bufs are equal
  ASSERT_EQ(memcmp(&stx_orig, &stx_symlink_orig, sizeof(stx_orig)), 0);

  sprintf(test_file, "/test_symlinks_abs_%d", getpid());

  fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  stone_close(cmount, fd);

  sprintf(test_symlink, "/test_symlinks_abs_sym_%d", getpid());

  ASSERT_EQ(stone_symlink(cmount, test_file, test_symlink), 0);

  // stat the original file
  ASSERT_EQ(stone_statx(cmount, test_file, &stx_orig, STONE_STATX_ALL_STATS, 0), 0);
  // stat the symlink
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx_symlink_orig, STONE_STATX_ALL_STATS, 0), 0);
  // ensure the statx bufs are equal
  ASSERT_TRUE(!memcmp(&stx_orig, &stx_symlink_orig, sizeof(stx_orig)));

  // test lstat
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx_orig, STONE_STATX_ALL_STATS, AT_SYMLINK_NOFOLLOW), 0);
  ASSERT_TRUE(S_ISLNK(stx_orig.stx_mode));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, DirSyms) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_dir1[256];
  sprintf(test_dir1, "dir1_symlinks_%d", getpid());

  ASSERT_EQ(stone_mkdir(cmount, test_dir1, 0700), 0);

  char test_symdir[256];
  sprintf(test_symdir, "symdir_symlinks_%d", getpid());

  ASSERT_EQ(stone_symlink(cmount, test_dir1, test_symdir), 0);

  char test_file[256];
  sprintf(test_file, "/symdir_symlinks_%d/test_symdir_file", getpid());
  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0600);
  ASSERT_GT(fd, 0);
  stone_close(cmount, fd);

  struct stone_statx stx;
  ASSERT_EQ(stone_statx(cmount, test_file, &stx, 0, AT_SYMLINK_NOFOLLOW), 0);

  // ensure that its a file not a directory we get back
  ASSERT_TRUE(S_ISREG(stx.stx_mode));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, LoopSyms) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_dir1[256];
  sprintf(test_dir1, "dir1_loopsym_%d", getpid());

  ASSERT_EQ(stone_mkdir(cmount, test_dir1, 0700), 0);

  char test_dir2[256];
  sprintf(test_dir2, "/dir1_loopsym_%d/loop_dir", getpid());

  ASSERT_EQ(stone_mkdir(cmount, test_dir2, 0700), 0);

  // symlink it itself:  /path/to/mysym -> /path/to/mysym
  char test_symdir[256];
  sprintf(test_symdir, "/dir1_loopsym_%d/loop_dir/symdir", getpid());

  ASSERT_EQ(stone_symlink(cmount, test_symdir, test_symdir), 0);

  char test_file[256];
  sprintf(test_file, "/dir1_loopsym_%d/loop_dir/symdir/test_loopsym_file", getpid());
  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0600);
  ASSERT_EQ(fd, -ELOOP);

  // loop: /a -> /b, /b -> /c, /c -> /a
  char a[264], b[264], c[264];
  sprintf(a, "/%s/a", test_dir1);
  sprintf(b, "/%s/b", test_dir1);
  sprintf(c, "/%s/c", test_dir1);
  ASSERT_EQ(stone_symlink(cmount, a, b), 0);
  ASSERT_EQ(stone_symlink(cmount, b, c), 0);
  ASSERT_EQ(stone_symlink(cmount, c, a), 0);
  ASSERT_EQ(stone_open(cmount, a, O_RDWR, 0), -ELOOP);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, HardlinkNoOriginal) {

  int mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir[256];
  sprintf(dir, "/test_rmdirfail%d", mypid);
  ASSERT_EQ(stone_mkdir(cmount, dir, 0777), 0);

  ASSERT_EQ(stone_chdir(cmount, dir), 0);

  int fd = stone_open(cmount, "f1", O_CREAT, 0644);
  ASSERT_GT(fd, 0);

  stone_close(cmount, fd);

  // create hard link
  ASSERT_EQ(stone_link(cmount, "f1", "hardl1"), 0);

  // remove file link points to
  ASSERT_EQ(stone_unlink(cmount, "f1"), 0);

  stone_shutdown(cmount);

  // now cleanup
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);
  ASSERT_EQ(stone_chdir(cmount, dir), 0);
  ASSERT_EQ(stone_unlink(cmount, "hardl1"), 0);
  ASSERT_EQ(stone_rmdir(cmount, dir), 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, BadArgument) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  int fd = stone_open(cmount, "test_file", O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);
  char buf[100];
  ASSERT_EQ(stone_write(cmount, fd, buf, sizeof(buf), 0), (int)sizeof(buf));
  ASSERT_EQ(stone_read(cmount, fd, buf, 0, 5), 0);
  stone_close(cmount, fd);
  ASSERT_EQ(stone_unlink(cmount, "test_file"), 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, BadFileDesc) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  ASSERT_EQ(stone_fchmod(cmount, -1, 0655), -EBADF);
  ASSERT_EQ(stone_close(cmount, -1), -EBADF);
  ASSERT_EQ(stone_lseek(cmount, -1, 0, SEEK_SET), -EBADF);

  char buf[0];
  ASSERT_EQ(stone_read(cmount, -1, buf, 0, 0), -EBADF);
  ASSERT_EQ(stone_write(cmount, -1, buf, 0, 0), -EBADF);

  ASSERT_EQ(stone_ftruncate(cmount, -1, 0), -EBADF);
  ASSERT_EQ(stone_fsync(cmount, -1, 0), -EBADF);

  struct stone_statx stx;
  ASSERT_EQ(stone_fstatx(cmount, -1, &stx, 0, 0), -EBADF);

  struct sockaddr_storage addr;
  ASSERT_EQ(stone_get_file_stripe_address(cmount, -1, 0, &addr, 1), -EBADF);

  ASSERT_EQ(stone_get_file_stripe_unit(cmount, -1), -EBADF);
  ASSERT_EQ(stone_get_file_pool(cmount, -1), -EBADF);
  char poolname[80];
  ASSERT_EQ(stone_get_file_pool_name(cmount, -1, poolname, sizeof(poolname)), -EBADF);
  ASSERT_EQ(stone_get_file_replication(cmount, -1), -EBADF);
  ASSERT_EQ(stone_get_file_object_size(cmount, -1), -EBADF);
  int stripe_unit, stripe_count, object_size, pg_pool;
  ASSERT_EQ(stone_get_file_layout(cmount, -1, &stripe_unit, &stripe_count, &object_size, &pg_pool), -EBADF);
  ASSERT_EQ(stone_get_file_stripe_count(cmount, -1), -EBADF);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, ReadEmptyFile) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  // test the read_sync path in the client for zero files
  ASSERT_EQ(stone_conf_set(cmount, "client_debug_force_sync_read", "true"), 0);

  int mypid = getpid();
  char testf[256];

  sprintf(testf, "test_reademptyfile%d", mypid);
  int fd = stone_open(cmount, testf, O_CREAT|O_TRUNC|O_WRONLY, 0644);
  ASSERT_GT(fd, 0);

  stone_close(cmount, fd);

  fd = stone_open(cmount, testf, O_RDONLY, 0);
  ASSERT_GT(fd, 0);

  char buf[4096];
  ASSERT_EQ(stone_read(cmount, fd, buf, 4096, 0), 0);

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, PreadvPwritev) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  int mypid = getpid();
  char testf[256];

  sprintf(testf, "test_preadvpwritevfile%d", mypid);
  int fd = stone_open(cmount, testf, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  char out0[] = "hello ";
  char out1[] = "world\n";
  struct iovec iov_out[2] = {
	{out0, sizeof(out0)},
	{out1, sizeof(out1)},
  };
  char in0[sizeof(out0)];
  char in1[sizeof(out1)];
  struct iovec iov_in[2] = {
	{in0, sizeof(in0)},
	{in1, sizeof(in1)},
  };
  ssize_t nwritten = iov_out[0].iov_len + iov_out[1].iov_len; 
  ssize_t nread = iov_in[0].iov_len + iov_in[1].iov_len; 

  ASSERT_EQ(stone_pwritev(cmount, fd, iov_out, 2, 0), nwritten);
  ASSERT_EQ(stone_preadv(cmount, fd, iov_in, 2, 0), nread);
  ASSERT_EQ(0, strncmp((const char*)iov_in[0].iov_base, (const char*)iov_out[0].iov_base, iov_out[0].iov_len));
  ASSERT_EQ(0, strncmp((const char*)iov_in[1].iov_base, (const char*)iov_out[1].iov_base, iov_out[1].iov_len));

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, LlreadvLlwritev) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  int mypid = getpid();
  char filename[256];

  sprintf(filename, "test_llreadvllwritevfile%u", mypid);

  Inode *root, *file;
  ASSERT_EQ(stone_ll_lookup_root(cmount, &root), 0);

  Fh *fh;
  struct stone_statx stx;
  UserPerm *perms = stone_mount_perms(cmount);

  ASSERT_EQ(stone_ll_create(cmount, root, filename, 0666,
		    O_RDWR|O_CREAT|O_TRUNC, &file, &fh, &stx, 0, 0, perms), 0);

  /* Reopen read-only */
  char out0[] = "hello ";
  char out1[] = "world\n";
  struct iovec iov_out[2] = {
	{out0, sizeof(out0)},
	{out1, sizeof(out1)},
  };
  char in0[sizeof(out0)];
  char in1[sizeof(out1)];
  struct iovec iov_in[2] = {
	{in0, sizeof(in0)},
	{in1, sizeof(in1)},
  };
  ssize_t nwritten = iov_out[0].iov_len + iov_out[1].iov_len;
  ssize_t nread = iov_in[0].iov_len + iov_in[1].iov_len;

  ASSERT_EQ(stone_ll_writev(cmount, fh, iov_out, 2, 0), nwritten);
  ASSERT_EQ(stone_ll_readv(cmount, fh, iov_in, 2, 0), nread);
  ASSERT_EQ(0, strncmp((const char*)iov_in[0].iov_base, (const char*)iov_out[0].iov_base, iov_out[0].iov_len));
  ASSERT_EQ(0, strncmp((const char*)iov_in[1].iov_base, (const char*)iov_out[1].iov_base, iov_out[1].iov_len));

  stone_ll_close(cmount, fh);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, StripeUnitGran) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);
  ASSERT_GT(stone_get_stripe_unit_granularity(cmount), 0);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Rename) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  int mypid = getpid();
  char path_src[256];
  char path_dst[256];

  /* make a source file */
  sprintf(path_src, "test_rename_src%d", mypid);
  int fd = stone_open(cmount, path_src, O_CREAT|O_TRUNC|O_WRONLY, 0777);
  ASSERT_GT(fd, 0);
  ASSERT_EQ(0, stone_close(cmount, fd));

  /* rename to a new dest path */
  sprintf(path_dst, "test_rename_dst%d", mypid);
  ASSERT_EQ(0, stone_rename(cmount, path_src, path_dst));

  /* test that dest path exists */
  struct stone_statx stx;
  ASSERT_EQ(0, stone_statx(cmount, path_dst, &stx, 0, 0));

  /* test that src path doesn't exist */
  ASSERT_EQ(-ENOENT, stone_statx(cmount, path_src, &stx, 0, AT_SYMLINK_NOFOLLOW));

  /* rename with non-existent source path */
  ASSERT_EQ(-ENOENT, stone_rename(cmount, path_src, path_dst));

  ASSERT_EQ(0, stone_unlink(cmount, path_dst));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, UseUnmounted) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));

  struct statvfs stvfs;
  EXPECT_EQ(-ENOTCONN, stone_statfs(cmount, "/", &stvfs));
  EXPECT_EQ(-ENOTCONN, stone_get_local_osd(cmount));
  EXPECT_EQ(-ENOTCONN, stone_chdir(cmount, "/"));

  struct stone_dir_result *dirp;
  EXPECT_EQ(-ENOTCONN, stone_opendir(cmount, "/", &dirp));
  EXPECT_EQ(-ENOTCONN, stone_closedir(cmount, dirp));

  stone_readdir(cmount, dirp);
  EXPECT_EQ(ENOTCONN, errno);

  struct dirent rdent;
  EXPECT_EQ(-ENOTCONN, stone_readdir_r(cmount, dirp, &rdent));

  struct stone_statx stx;
  EXPECT_EQ(-ENOTCONN, stone_readdirplus_r(cmount, dirp, &rdent, &stx, 0, 0, NULL));
  EXPECT_EQ(-ENOTCONN, stone_getdents(cmount, dirp, NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_getdnames(cmount, dirp, NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_telldir(cmount, dirp));
  EXPECT_EQ(-ENOTCONN, stone_link(cmount, "/", "/link"));
  EXPECT_EQ(-ENOTCONN, stone_unlink(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_rename(cmount, "/path", "/path"));
  EXPECT_EQ(-ENOTCONN, stone_mkdir(cmount, "/", 0655));
  EXPECT_EQ(-ENOTCONN, stone_mkdirs(cmount, "/", 0655));
  EXPECT_EQ(-ENOTCONN, stone_rmdir(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_readlink(cmount, "/path", NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_symlink(cmount, "/path", "/path"));
  EXPECT_EQ(-ENOTCONN, stone_statx(cmount, "/path", &stx, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_setattrx(cmount, "/path", &stx, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_getxattr(cmount, "/path", "name", NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_lgetxattr(cmount, "/path", "name", NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_listxattr(cmount, "/path", NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_llistxattr(cmount, "/path", NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_removexattr(cmount, "/path", "name"));
  EXPECT_EQ(-ENOTCONN, stone_lremovexattr(cmount, "/path", "name"));
  EXPECT_EQ(-ENOTCONN, stone_setxattr(cmount, "/path", "name", NULL, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_lsetxattr(cmount, "/path", "name", NULL, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_fsetattrx(cmount, 0, &stx, 0));
  EXPECT_EQ(-ENOTCONN, stone_chmod(cmount, "/path", 0));
  EXPECT_EQ(-ENOTCONN, stone_fchmod(cmount, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_chown(cmount, "/path", 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_lchown(cmount, "/path", 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_fchown(cmount, 0, 0, 0));

  struct utimbuf utb;
  EXPECT_EQ(-ENOTCONN, stone_utime(cmount, "/path", &utb));
  EXPECT_EQ(-ENOTCONN, stone_truncate(cmount, "/path", 0));
  EXPECT_EQ(-ENOTCONN, stone_mknod(cmount, "/path", 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_open(cmount, "/path", 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_open_layout(cmount, "/path", 0, 0, 0, 0, 0, "pool"));
  EXPECT_EQ(-ENOTCONN, stone_close(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_lseek(cmount, 0, 0, SEEK_SET));
  EXPECT_EQ(-ENOTCONN, stone_read(cmount, 0, NULL, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_write(cmount, 0, NULL, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_ftruncate(cmount, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_fsync(cmount, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_fstatx(cmount, 0, &stx, 0, 0));
  EXPECT_EQ(-ENOTCONN, stone_sync_fs(cmount));
  EXPECT_EQ(-ENOTCONN, stone_get_file_stripe_unit(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_file_stripe_count(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_file_layout(cmount, 0, NULL, NULL ,NULL ,NULL));
  EXPECT_EQ(-ENOTCONN, stone_get_file_object_size(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_file_pool(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_file_pool_name(cmount, 0, NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_file_replication(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_path_replication(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_get_path_layout(cmount, "/path", NULL, NULL, NULL, NULL));
  EXPECT_EQ(-ENOTCONN, stone_get_path_object_size(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_get_path_stripe_count(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_get_path_stripe_unit(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_get_path_pool(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_get_path_pool_name(cmount, "/path", NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_pool_name(cmount, 0, NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_get_file_stripe_address(cmount, 0, 0, NULL, 0));
  EXPECT_EQ(-ENOTCONN, stone_localize_reads(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_debug_get_fd_caps(cmount, 0));
  EXPECT_EQ(-ENOTCONN, stone_debug_get_file_caps(cmount, "/path"));
  EXPECT_EQ(-ENOTCONN, stone_get_stripe_unit_granularity(cmount));
  EXPECT_EQ(-ENOTCONN, stone_get_pool_id(cmount, "data"));
  EXPECT_EQ(-ENOTCONN, stone_get_pool_replication(cmount, 1));

  stone_release(cmount);
}

TEST(LibStoneFS, GetPoolId) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char name[80];
  memset(name, 0, sizeof(name));
  ASSERT_LE(0, stone_get_path_pool_name(cmount, "/", name, sizeof(name)));
  ASSERT_GE(stone_get_pool_id(cmount, name), 0);
  ASSERT_EQ(stone_get_pool_id(cmount, "weflkjwelfjwlkejf"), -ENOENT);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, GetPoolReplication) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  /* negative pools */
  ASSERT_EQ(stone_get_pool_replication(cmount, -10), -ENOENT);

  /* valid pool */
  int pool_id;
  int stripe_unit, stripe_count, object_size;
  ASSERT_EQ(0, stone_get_path_layout(cmount, "/", &stripe_unit, &stripe_count,
				    &object_size, &pool_id));
  ASSERT_GE(pool_id, 0);
  ASSERT_GT(stone_get_pool_replication(cmount, pool_id), 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, GetExtentOsds) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);

  EXPECT_EQ(-ENOTCONN, stone_get_file_extent_osds(cmount, 0, 0, NULL, NULL, 0));

  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  int stripe_unit = (1<<18);

  /* make a file! */
  char test_file[256];
  sprintf(test_file, "test_extent_osds_%d", getpid());
  int fd = stone_open_layout(cmount, test_file, O_CREAT|O_RDWR, 0666,
      stripe_unit, 2, stripe_unit*2, NULL);
  ASSERT_GT(fd, 0);

  /* get back how many osds > 0 */
  int ret = stone_get_file_extent_osds(cmount, fd, 0, NULL, NULL, 0);
  EXPECT_GT(ret, 0);

  int64_t len;
  int osds[ret];

  /* full stripe extent */
  EXPECT_EQ(ret, stone_get_file_extent_osds(cmount, fd, 0, &len, osds, ret));
  EXPECT_EQ(len, (int64_t)stripe_unit);

  /* half stripe extent */
  EXPECT_EQ(ret, stone_get_file_extent_osds(cmount, fd, stripe_unit/2, &len, osds, ret));
  EXPECT_EQ(len, (int64_t)stripe_unit/2);

  /* 1.5 stripe unit offset -1 byte */
  EXPECT_EQ(ret, stone_get_file_extent_osds(cmount, fd, 3*stripe_unit/2-1, &len, osds, ret));
  EXPECT_EQ(len, (int64_t)stripe_unit/2+1);

  /* 1.5 stripe unit offset +1 byte */
  EXPECT_EQ(ret, stone_get_file_extent_osds(cmount, fd, 3*stripe_unit/2+1, &len, osds, ret));
  EXPECT_EQ(len, (int64_t)stripe_unit/2-1);

  /* only when more than 1 osd */
  if (ret > 1) {
    EXPECT_EQ(-ERANGE, stone_get_file_extent_osds(cmount, fd, 0, NULL, osds, 1));
  }

  stone_close(cmount, fd);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, GetOsdCrushLocation) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);

  EXPECT_EQ(-ENOTCONN, stone_get_osd_crush_location(cmount, 0, NULL, 0));

  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  ASSERT_EQ(stone_get_osd_crush_location(cmount, 0, NULL, 1), -EINVAL);

  char path[256];
  ASSERT_EQ(stone_get_osd_crush_location(cmount, 9999999, path, 0), -ENOENT);
  ASSERT_EQ(stone_get_osd_crush_location(cmount, -1, path, 0), -EINVAL);

  char test_file[256];
  sprintf(test_file, "test_osds_loc_%d", getpid());
  int fd = stone_open(cmount, test_file, O_CREAT|O_RDWR, 0666);
  ASSERT_GT(fd, 0);

  /* get back how many osds > 0 */
  int ret = stone_get_file_extent_osds(cmount, fd, 0, NULL, NULL, 0);
  EXPECT_GT(ret, 0);

  /* full stripe extent */
  int osds[ret];
  EXPECT_EQ(ret, stone_get_file_extent_osds(cmount, fd, 0, NULL, osds, ret));

  ASSERT_GT(stone_get_osd_crush_location(cmount, 0, path, 0), 0);
  ASSERT_EQ(stone_get_osd_crush_location(cmount, 0, path, 1), -ERANGE);

  for (int i = 0; i < ret; i++) {
    int len = stone_get_osd_crush_location(cmount, osds[i], path, sizeof(path));
    ASSERT_GT(len, 0);
    int pos = 0;
    while (pos < len) {
      std::string type(path + pos);
      ASSERT_GT((int)type.size(), 0);
      pos += type.size() + 1;

      std::string name(path + pos);
      ASSERT_GT((int)name.size(), 0);
      pos += name.size() + 1;
    }
  }

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, GetOsdAddr) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);

  EXPECT_EQ(-ENOTCONN, stone_get_osd_addr(cmount, 0, NULL));

  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  ASSERT_EQ(-EINVAL, stone_get_osd_addr(cmount, 0, NULL));

  struct sockaddr_storage addr;
  ASSERT_EQ(-ENOENT, stone_get_osd_addr(cmount, -1, &addr));
  ASSERT_EQ(-ENOENT, stone_get_osd_addr(cmount, 9999999, &addr));

  ASSERT_EQ(0, stone_get_osd_addr(cmount, 0, &addr));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, OpenNoClose) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  pid_t mypid = getpid();
  char str_buf[256];
  sprintf(str_buf, "open_no_close_dir%d", mypid);
  ASSERT_EQ(0, stone_mkdirs(cmount, str_buf, 0777));

  struct stone_dir_result *ls_dir = NULL;
  ASSERT_EQ(stone_opendir(cmount, str_buf, &ls_dir), 0);

  sprintf(str_buf, "open_no_close_file%d", mypid);
  int fd = stone_open(cmount, str_buf, O_RDONLY|O_CREAT, 0666);
  ASSERT_LT(0, fd);

  // shutdown should force close opened file/dir
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Nlink) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  Inode *root, *dir, *file;

  ASSERT_EQ(stone_ll_lookup_root(cmount, &root), 0);

  char dirname[32], filename[32], linkname[32];
  sprintf(dirname, "nlinkdir%x", getpid());
  sprintf(filename, "nlinkorig%x", getpid());
  sprintf(linkname, "nlinklink%x", getpid());

  struct stone_statx stx;
  Fh *fh;
  UserPerm *perms = stone_mount_perms(cmount);

  ASSERT_EQ(stone_ll_mkdir(cmount, root, dirname, 0755, &dir, &stx, 0, 0, perms), 0);
  ASSERT_EQ(stone_ll_create(cmount, dir, filename, 0666, O_RDWR|O_CREAT|O_EXCL,
			   &file, &fh, &stx, STONE_STATX_NLINK, 0, perms), 0);
  ASSERT_EQ(stone_ll_close(cmount, fh), 0);
  ASSERT_EQ(stx.stx_nlink, (nlink_t)1);

  ASSERT_EQ(stone_ll_link(cmount, file, dir, linkname, perms), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, file, &stx, STONE_STATX_NLINK, 0, perms), 0);
  ASSERT_EQ(stx.stx_nlink, (nlink_t)2);

  ASSERT_EQ(stone_ll_unlink(cmount, dir, linkname, perms), 0);
  ASSERT_EQ(stone_ll_lookup(cmount, dir, filename, &file, &stx,
			   STONE_STATX_NLINK, 0, perms), 0);
  ASSERT_EQ(stx.stx_nlink, (nlink_t)1);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, SlashDotDot) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  struct stone_statx	stx;
  ASSERT_EQ(stone_statx(cmount, "/.", &stx, STONE_STATX_INO, 0), 0);

  ino_t ino = stx.stx_ino;
  ASSERT_EQ(stone_statx(cmount, "/..", &stx, STONE_STATX_INO, 0), 0);

  /* At root, "." and ".." should be the same inode */
  ASSERT_EQ(ino, stx.stx_ino);

  /* Test accessing the parent of an unlinked directory */
  char dir1[32], dir2[56];
  sprintf(dir1, "/sldotdot%x", getpid());
  sprintf(dir2, "%s/sub%x", dir1, getpid());

  ASSERT_EQ(stone_mkdir(cmount, dir1, 0755), 0);
  ASSERT_EQ(stone_mkdir(cmount, dir2, 0755), 0);

  ASSERT_EQ(stone_chdir(cmount, dir2), 0);

  /* Test behavior when unlinking cwd */
  struct stone_dir_result *rdir;
  ASSERT_EQ(stone_opendir(cmount, ".", &rdir), 0);
  ASSERT_EQ(stone_rmdir(cmount, dir2), 0);

  /* get "." entry */
  struct dirent *result = stone_readdir(cmount, rdir);
  ino = result->d_ino;

  /* get ".." entry */
  result = stone_readdir(cmount, rdir);
  ASSERT_EQ(ino, result->d_ino);
  stone_closedir(cmount, rdir);

  /* Make sure it works same way when mounting subtree */
  ASSERT_EQ(stone_unmount(cmount), 0);
  ASSERT_EQ(stone_mount(cmount, dir1), 0);
  ASSERT_EQ(stone_statx(cmount, "/..", &stx, STONE_STATX_INO, 0), 0);

  /* Test readdir behavior */
  ASSERT_EQ(stone_opendir(cmount, "/", &rdir), 0);
  result = stone_readdir(cmount, rdir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  ino = result->d_ino;
  result = stone_readdir(cmount, rdir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");
  ASSERT_EQ(ino, result->d_ino);

  stone_shutdown(cmount);
}

static inline bool
timespec_eq(timespec const& lhs, timespec const& rhs)
{
  return lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec == rhs.tv_nsec;
}

TEST(LibStoneFS, Btime) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char filename[32];
  sprintf(filename, "/getattrx%x", getpid());

  stone_unlink(cmount, filename);
  int fd = stone_open(cmount, filename, O_RDWR|O_CREAT|O_EXCL, 0666);
  ASSERT_LT(0, fd);

  /* make sure fstatx works */
  struct stone_statx	stx;

  ASSERT_EQ(stone_fstatx(cmount, fd, &stx, STONE_STATX_CTIME|STONE_STATX_BTIME, 0), 0);
  ASSERT_TRUE(stx.stx_mask & (STONE_STATX_CTIME|STONE_STATX_BTIME));
  ASSERT_TRUE(timespec_eq(stx.stx_ctime, stx.stx_btime));
  stone_close(cmount, fd);

  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_CTIME|STONE_STATX_BTIME, 0), 0);
  ASSERT_TRUE(timespec_eq(stx.stx_ctime, stx.stx_btime));
  ASSERT_TRUE(stx.stx_mask & (STONE_STATX_CTIME|STONE_STATX_BTIME));

  struct timespec old_btime = stx.stx_btime;

  /* Now sleep, do a chmod and verify that the ctime changed, but btime didn't */
  sleep(1);
  ASSERT_EQ(stone_chmod(cmount, filename, 0644), 0);
  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_CTIME|STONE_STATX_BTIME, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_BTIME);
  ASSERT_TRUE(timespec_eq(stx.stx_btime, old_btime));
  ASSERT_FALSE(timespec_eq(stx.stx_ctime, stx.stx_btime));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, SetBtime) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char filename[32];
  sprintf(filename, "/setbtime%x", getpid());

  stone_unlink(cmount, filename);
  int fd = stone_open(cmount, filename, O_RDWR|O_CREAT|O_EXCL, 0666);
  ASSERT_LT(0, fd);
  stone_close(cmount, fd);

  struct stone_statx stx;
  struct timespec old_btime = { 1, 2 };

  stx.stx_btime = old_btime;

  ASSERT_EQ(stone_setattrx(cmount, filename, &stx, STONE_SETATTR_BTIME, 0), 0);

  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_BTIME, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_BTIME);
  ASSERT_TRUE(timespec_eq(stx.stx_btime, old_btime));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, LazyStatx) {
  struct stone_mount_info *cmount1, *cmount2;
  ASSERT_EQ(stone_create(&cmount1, NULL), 0);
  ASSERT_EQ(stone_create(&cmount2, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount1, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount2, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount1, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount2, NULL));
  ASSERT_EQ(stone_mount(cmount1, "/"), 0);
  ASSERT_EQ(stone_mount(cmount2, "/"), 0);

  char filename[32];
  sprintf(filename, "lazystatx%x", getpid());

  Inode *root1, *file1, *root2, *file2;
  struct stone_statx stx;
  Fh *fh;
  UserPerm *perms1 = stone_mount_perms(cmount1);
  UserPerm *perms2 = stone_mount_perms(cmount2);

  ASSERT_EQ(stone_ll_lookup_root(cmount1, &root1), 0);
  stone_ll_unlink(cmount1, root1, filename, perms1);
  ASSERT_EQ(stone_ll_create(cmount1, root1, filename, 0666, O_RDWR|O_CREAT|O_EXCL,
			   &file1, &fh, &stx, 0, 0, perms1), 0);
  ASSERT_EQ(stone_ll_close(cmount1, fh), 0);

  ASSERT_EQ(stone_ll_lookup_root(cmount2, &root2), 0);

  ASSERT_EQ(stone_ll_lookup(cmount2, root2, filename, &file2, &stx, STONE_STATX_CTIME, 0, perms2), 0);

  struct timespec old_ctime = stx.stx_ctime;

  /*
   * Now sleep, do a chmod on the first client and the see whether we get a
   * different ctime with a statx that uses AT_NO_ATTR_SYNC
   */
  sleep(1);
  stx.stx_mode = 0644;
  ASSERT_EQ(stone_ll_setattr(cmount1, file1, &stx, STONE_SETATTR_MODE, perms1), 0);

  ASSERT_EQ(stone_ll_getattr(cmount2, file2, &stx, STONE_STATX_CTIME, AT_NO_ATTR_SYNC, perms2), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_CTIME);
  ASSERT_TRUE(stx.stx_ctime.tv_sec == old_ctime.tv_sec &&
	      stx.stx_ctime.tv_nsec == old_ctime.tv_nsec);

  stone_shutdown(cmount1);
  stone_shutdown(cmount2);
}

TEST(LibStoneFS, ChangeAttr) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char filename[32];
  sprintf(filename, "/changeattr%x", getpid());

  stone_unlink(cmount, filename);
  int fd = stone_open(cmount, filename, O_RDWR|O_CREAT|O_EXCL, 0666);
  ASSERT_LT(0, fd);

  struct stone_statx	stx;
  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);

  uint64_t old_change_attr = stx.stx_version;

  /* do chmod, and check whether change_attr changed */
  ASSERT_EQ(stone_chmod(cmount, filename, 0644), 0);
  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);
  ASSERT_NE(stx.stx_version, old_change_attr);
  old_change_attr = stx.stx_version;

  /* now do a write and see if it changed again */
  ASSERT_EQ(3, stone_write(cmount, fd, "foo", 3, 0));
  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);
  ASSERT_NE(stx.stx_version, old_change_attr);
  old_change_attr = stx.stx_version;

  /* Now truncate and check again */
  ASSERT_EQ(0, stone_ftruncate(cmount, fd, 0));
  ASSERT_EQ(stone_statx(cmount, filename, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);
  ASSERT_NE(stx.stx_version, old_change_attr);

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, DirChangeAttr) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dirname[32], filename[56];
  sprintf(dirname, "/dirchange%x", getpid());
  sprintf(filename, "%s/foo", dirname);

  ASSERT_EQ(stone_mkdir(cmount, dirname, 0755), 0);

  struct stone_statx	stx;
  ASSERT_EQ(stone_statx(cmount, dirname, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);

  uint64_t old_change_attr = stx.stx_version;

  int fd = stone_open(cmount, filename, O_RDWR|O_CREAT|O_EXCL, 0666);
  ASSERT_LT(0, fd);
  stone_close(cmount, fd);

  ASSERT_EQ(stone_statx(cmount, dirname, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);
  ASSERT_NE(stx.stx_version, old_change_attr);

  old_change_attr = stx.stx_version;

  ASSERT_EQ(stone_unlink(cmount, filename), 0);
  ASSERT_EQ(stone_statx(cmount, dirname, &stx, STONE_STATX_VERSION, 0), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_VERSION);
  ASSERT_NE(stx.stx_version, old_change_attr);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, SetSize) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char filename[32];
  sprintf(filename, "/setsize%x", getpid());

  stone_unlink(cmount, filename);
  int fd = stone_open(cmount, filename, O_RDWR|O_CREAT|O_EXCL, 0666);
  ASSERT_LT(0, fd);

  struct stone_statx stx;
  uint64_t size = 8388608;
  stx.stx_size = size;
  ASSERT_EQ(stone_fsetattrx(cmount, fd, &stx, STONE_SETATTR_SIZE), 0);
  ASSERT_EQ(stone_fstatx(cmount, fd, &stx, STONE_STATX_SIZE, 0), 0);
  ASSERT_EQ(stx.stx_size, size);

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, ClearSetuid) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  Inode *root;
  ASSERT_EQ(stone_ll_lookup_root(cmount, &root), 0);

  char filename[32];
  sprintf(filename, "clearsetuid%x", getpid());

  Fh *fh;
  Inode *in;
  struct stone_statx stx;
  const mode_t after_mode = S_IRWXU;
  const mode_t before_mode = S_IRWXU | S_ISUID | S_ISGID;
  const unsigned want = STONE_STATX_UID|STONE_STATX_GID|STONE_STATX_MODE;
  UserPerm *usercred = stone_mount_perms(cmount);

  stone_ll_unlink(cmount, root, filename, usercred);
  ASSERT_EQ(stone_ll_create(cmount, root, filename, before_mode,
			   O_RDWR|O_CREAT|O_EXCL, &in, &fh, &stx, want, 0,
			   usercred), 0);

  ASSERT_EQ(stx.stx_mode & (mode_t)ALLPERMS, before_mode);

  // write
  ASSERT_EQ(stone_ll_write(cmount, fh, 0, 3, "foo"), 3);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, usercred), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_MODE);
  ASSERT_EQ(stx.stx_mode & (mode_t)ALLPERMS, after_mode);

  // reset mode
  stx.stx_mode = before_mode;
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_STATX_MODE, usercred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, usercred), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_MODE);
  ASSERT_EQ(stx.stx_mode & (mode_t)ALLPERMS, before_mode);

  // truncate
  stx.stx_size = 1;
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_SETATTR_SIZE, usercred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, usercred), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_MODE);
  ASSERT_EQ(stx.stx_mode & (mode_t)ALLPERMS, after_mode);

  // reset mode
  stx.stx_mode = before_mode;
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_STATX_MODE, usercred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, usercred), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_MODE);
  ASSERT_EQ(stx.stx_mode & (mode_t)ALLPERMS, before_mode);

  // chown  -- for this we need to be "root"
  UserPerm *rootcred = stone_userperm_new(0, 0, 0, NULL);
  ASSERT_TRUE(rootcred);
  stx.stx_uid++;
  stx.stx_gid++;
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_SETATTR_UID|STONE_SETATTR_GID, rootcred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, usercred), 0);
  ASSERT_TRUE(stx.stx_mask & STONE_STATX_MODE);
  ASSERT_EQ(stx.stx_mode & (mode_t)ALLPERMS, after_mode);

  /* test chown with supplementary groups, and chown with/without exe bit */
  uid_t u = 65534;
  gid_t g = 65534;
  gid_t gids[] = {65533,65532};
  UserPerm *altcred = stone_userperm_new(u, g, sizeof gids / sizeof gids[0], gids);
  stx.stx_uid = u;
  stx.stx_gid = g;
  mode_t m = S_ISGID|S_ISUID|S_IRUSR|S_IWUSR;
  stx.stx_mode = m;
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_STATX_MODE|STONE_SETATTR_UID|STONE_SETATTR_GID, rootcred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, altcred), 0);
  ASSERT_EQ(stx.stx_mode&(mode_t)ALLPERMS, m);
  /* not dropped without exe bit */
  stx.stx_gid = gids[0];
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_SETATTR_GID, altcred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, altcred), 0);
  ASSERT_EQ(stx.stx_mode&(mode_t)ALLPERMS, m);
  /* now check dropped with exe bit */
  m = S_ISGID|S_ISUID|S_IRWXU;
  stx.stx_mode = m;
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_STATX_MODE, altcred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, altcred), 0);
  ASSERT_EQ(stx.stx_mode&(mode_t)ALLPERMS, m);
  stx.stx_gid = gids[1];
  ASSERT_EQ(stone_ll_setattr(cmount, in, &stx, STONE_SETATTR_GID, altcred), 0);
  ASSERT_EQ(stone_ll_getattr(cmount, in, &stx, STONE_STATX_MODE, 0, altcred), 0);
  ASSERT_EQ(stx.stx_mode&(mode_t)ALLPERMS, m&(S_IRWXU|S_IRWXG|S_IRWXO));
  stone_userperm_destroy(altcred);

  ASSERT_EQ(stone_ll_close(cmount, fh), 0);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, OperationsOnRoot)
{
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dirname[32];
  sprintf(dirname, "/somedir%x", getpid());

  ASSERT_EQ(stone_mkdir(cmount, dirname, 0755), 0);

  ASSERT_EQ(stone_rmdir(cmount, "/"), -EBUSY);

  ASSERT_EQ(stone_link(cmount, "/", "/"), -EEXIST);
  ASSERT_EQ(stone_link(cmount, dirname, "/"), -EEXIST);
  ASSERT_EQ(stone_link(cmount, "nonExisitingDir", "/"), -ENOENT);

  ASSERT_EQ(stone_unlink(cmount, "/"), -EISDIR);

  ASSERT_EQ(stone_rename(cmount, "/", "/"), -EBUSY);
  ASSERT_EQ(stone_rename(cmount, dirname, "/"), -EBUSY);
  ASSERT_EQ(stone_rename(cmount, "nonExistingDir", "/"), -EBUSY);
  ASSERT_EQ(stone_rename(cmount, "/", dirname), -EBUSY);
  ASSERT_EQ(stone_rename(cmount, "/", "nonExistingDir"), -EBUSY);

  ASSERT_EQ(stone_mkdir(cmount, "/", 0777), -EEXIST);

  ASSERT_EQ(stone_mknod(cmount, "/", 0, 0), -EEXIST);

  ASSERT_EQ(stone_symlink(cmount, "/", "/"), -EEXIST);
  ASSERT_EQ(stone_symlink(cmount, dirname, "/"), -EEXIST);
  ASSERT_EQ(stone_symlink(cmount, "nonExistingDir", "/"), -EEXIST);

  stone_shutdown(cmount);
}

static void shutdown_racer_func()
{
  const int niter = 32;
  struct stone_mount_info *cmount;
  int i;

  for (i = 0; i < niter; ++i) {
    ASSERT_EQ(stone_create(&cmount, NULL), 0);
    ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
    ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
    ASSERT_EQ(stone_mount(cmount, "/"), 0);
    stone_shutdown(cmount);
  }
}

// See tracker #20988
TEST(LibStoneFS, ShutdownRace)
{
  const int nthreads = 32;
  std::thread threads[nthreads];

  // Need a bunch of fd's for this test
  struct rlimit rold, rnew;
  ASSERT_EQ(getrlimit(RLIMIT_NOFILE, &rold), 0);
  rnew = rold;
  rnew.rlim_cur = rnew.rlim_max;
  ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &rnew), 0);

  for (int i = 0; i < nthreads; ++i)
    threads[i] = std::thread(shutdown_racer_func);

  for (int i = 0; i < nthreads; ++i)
    threads[i].join();
  /*
   * Let's just ignore restoring the open files limit,
   * the kernel will defer releasing the file descriptors
   * and then the process will be possibly reachthe open
   * files limit. More detail, please see tracer#43039
   */
//  ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &rold), 0);
}

static void get_current_time_utimbuf(struct utimbuf *utb)
{
  utime_t t = stone_clock_now();
  utb->actime = t.sec();
  utb->modtime = t.sec();
}

static void get_current_time_timeval(struct timeval tv[2])
{
  utime_t t = stone_clock_now();
  t.copy_to_timeval(&tv[0]);
  t.copy_to_timeval(&tv[1]);
}

static void get_current_time_timespec(struct timespec ts[2])
{
  utime_t t = stone_clock_now();
  t.to_timespec(&ts[0]);
  t.to_timespec(&ts[1]);
}

TEST(LibStoneFS, TestUtime) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  sprintf(test_file, "test_utime_file_%d", getpid());
  int fd = stone_open(cmount, test_file, O_CREAT, 0666);
  ASSERT_GT(fd, 0);

  struct utimbuf utb;
  struct stone_statx stx;

  get_current_time_utimbuf(&utb);

  // stone_utime()
  EXPECT_EQ(0, stone_utime(cmount, test_file, &utb));
  ASSERT_EQ(stone_statx(cmount, test_file, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(utb.actime, 0));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(utb.modtime, 0));

  get_current_time_utimbuf(&utb);

  // stone_futime()
  EXPECT_EQ(0, stone_futime(cmount, fd, &utb));
  ASSERT_EQ(stone_statx(cmount, test_file, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(utb.actime, 0));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(utb.modtime, 0));

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, TestUtimes) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];
  char test_symlink[256];

  sprintf(test_file, "test_utimes_file_%d", getpid());
  sprintf(test_symlink, "test_utimes_symlink_%d", getpid());
  int fd = stone_open(cmount, test_file, O_CREAT, 0666);
  ASSERT_GT(fd, 0);

  ASSERT_EQ(stone_symlink(cmount, test_file, test_symlink), 0);

  struct timeval times[2];
  struct stone_statx stx;

  get_current_time_timeval(times);

  // stone_utimes() on symlink, validate target file time
  EXPECT_EQ(0, stone_utimes(cmount, test_symlink, times));
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(times[0]));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(times[1]));

  get_current_time_timeval(times);

  // stone_lutimes() on symlink, validate symlink time
  EXPECT_EQ(0, stone_lutimes(cmount, test_symlink, times));
  ASSERT_EQ(stone_statx(cmount, test_symlink, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, AT_SYMLINK_NOFOLLOW), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(times[0]));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(times[1]));

  get_current_time_timeval(times);

  // stone_futimes()
  EXPECT_EQ(0, stone_futimes(cmount, fd, times));
  ASSERT_EQ(stone_statx(cmount, test_file, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(times[0]));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(times[1]));

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, TestFutimens) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_file[256];

  sprintf(test_file, "test_futimens_file_%d", getpid());
  int fd = stone_open(cmount, test_file, O_CREAT, 0666);
  ASSERT_GT(fd, 0);

  struct timespec times[2];
  struct stone_statx stx;

  get_current_time_timespec(times);

  // stone_futimens()
  EXPECT_EQ(0, stone_futimens(cmount, fd, times));
  ASSERT_EQ(stone_statx(cmount, test_file, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(times[0]));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(times[1]));

  stone_close(cmount, fd);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, OperationsOnDotDot) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char c_dir[512], c_dir_dot[1024], c_dir_dotdot[1024];
  char c_non_existent_dir[1024], c_non_existent_dirs[1024];
  char c_temp[1024];

  pid_t mypid = getpid();
  sprintf(c_dir, "/oodd_dir_%d", mypid);
  sprintf(c_dir_dot, "/%s/.", c_dir);
  sprintf(c_dir_dotdot, "/%s/..", c_dir);
  sprintf(c_non_existent_dir, "/%s/../oodd_nonexistent/..", c_dir);
  sprintf(c_non_existent_dirs,
          "/%s/../ood_nonexistent1_%d/oodd_nonexistent2_%d", c_dir, mypid, mypid);
  sprintf(c_temp, "/oodd_temp_%d", mypid);

  ASSERT_EQ(0, stone_mkdir(cmount, c_dir, 0777));
  ASSERT_EQ(-EEXIST, stone_mkdir(cmount, c_dir_dot, 0777));
  ASSERT_EQ(-EEXIST, stone_mkdir(cmount, c_dir_dotdot, 0777));
  ASSERT_EQ(0, stone_mkdirs(cmount, c_non_existent_dirs, 0777));

  ASSERT_EQ(-ENOTEMPTY, stone_rmdir(cmount, c_dir_dot));
  ASSERT_EQ(-ENOTEMPTY, stone_rmdir(cmount, c_dir_dotdot));
  // non existent directory should return -ENOENT
  ASSERT_EQ(-ENOENT, stone_rmdir(cmount, c_non_existent_dir));

  ASSERT_EQ(-EBUSY, stone_rename(cmount, c_dir_dot, c_temp));
  ASSERT_EQ(0, stone_chdir(cmount, c_dir));
  ASSERT_EQ(0, stone_mkdir(cmount, c_temp, 0777));
  ASSERT_EQ(-EBUSY, stone_rename(cmount, c_temp, ".."));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, SnapXattrs) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_snap_xattr_file[256];
  char c_temp[PATH_MAX];
  char gxattrv[128];
  char gxattrv2[128];
  int xbuflen = sizeof(gxattrv);
  pid_t mypid = getpid();

  sprintf(test_snap_xattr_file, "test_snap_xattr_%d", mypid);
  int fd = stone_open(cmount, test_snap_xattr_file, O_CREAT, 0666);
  ASSERT_GT(fd, 0);
  stone_close(cmount, fd);

  sprintf(c_temp, "/.snap/test_snap_xattr_snap_%d", mypid);
  ASSERT_EQ(0, stone_mkdir(cmount, c_temp, 0777));

  int alen = stone_getxattr(cmount, c_temp, "stone.snap.btime", (void *)gxattrv, xbuflen);
  // xattr value is secs.nsecs (don't assume zero-term)
  ASSERT_LT(0, alen);
  ASSERT_LT(alen, xbuflen);
  gxattrv[alen] = '\0';
  char *s = strchrnul(gxattrv, '.');
  ASSERT_LT(s, gxattrv + alen);
  ASSERT_EQ('.', *s);
  *s = '\0';
  utime_t btime = utime_t(strtoull(gxattrv, NULL, 10), strtoull(s + 1, NULL, 10));
  *s = '.';  // restore for later strcmp

  // file within the snapshot should carry the same btime
  sprintf(c_temp, "/.snap/test_snap_xattr_snap_%d/%s", mypid, test_snap_xattr_file);

  int alen2 = stone_getxattr(cmount, c_temp, "stone.snap.btime", (void *)gxattrv2, xbuflen);
  ASSERT_EQ(alen, alen2);
  ASSERT_EQ(0, strncmp(gxattrv, gxattrv2, alen));

  // non-snap file shouldn't carry the xattr
  alen = stone_getxattr(cmount, test_snap_xattr_file, "stone.snap.btime", (void *)gxattrv2, xbuflen);
  ASSERT_EQ(-ENODATA, alen);

  // create a second snapshot
  sprintf(c_temp, "/.snap/test_snap_xattr_snap2_%d", mypid);
  ASSERT_EQ(0, stone_mkdir(cmount, c_temp, 0777));

  // check that the btime for the newer snapshot is > older
  alen = stone_getxattr(cmount, c_temp, "stone.snap.btime", (void *)gxattrv2, xbuflen);
  ASSERT_LT(0, alen);
  ASSERT_LT(alen, xbuflen);
  gxattrv2[alen] = '\0';
  s = strchrnul(gxattrv2, '.');
  ASSERT_LT(s, gxattrv2 + alen);
  ASSERT_EQ('.', *s);
  *s = '\0';
  utime_t new_btime = utime_t(strtoull(gxattrv2, NULL, 10), strtoull(s + 1, NULL, 10));
  ASSERT_LT(btime, new_btime);

  // listxattr() shouldn't return snap.btime vxattr
  char xattrlist[512];
  int len = stone_listxattr(cmount, test_snap_xattr_file, xattrlist, sizeof(xattrlist));
  ASSERT_GE(sizeof(xattrlist), (size_t)len);
  char *p = xattrlist;
  int found = 0;
  while (len > 0) {
    if (strcmp(p, "stone.snap.btime") == 0)
      found++;
    len -= strlen(p) + 1;
    p += strlen(p) + 1;
  }
  ASSERT_EQ(found, 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, SnapQuota) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char test_snap_dir_quota_xattr[256];
  char test_snap_subdir_quota_xattr[256];
  char test_snap_subdir_noquota_xattr[256];
  char xattrk[128];
  char xattrv[128];
  char c_temp[PATH_MAX];
  char gxattrv[128];
  int xbuflen = sizeof(gxattrv);
  pid_t mypid = getpid();

  // create dir and set quota
  sprintf(test_snap_dir_quota_xattr, "test_snap_dir_quota_xattr_%d", mypid);
  ASSERT_EQ(0, stone_mkdir(cmount, test_snap_dir_quota_xattr, 0777));

  sprintf(xattrk, "stone.quota.max_bytes");
  sprintf(xattrv, "65536");
  ASSERT_EQ(0, stone_setxattr(cmount, test_snap_dir_quota_xattr, xattrk, (void *)xattrv, 5, XATTR_CREATE));

  // create subdir and set quota
  sprintf(test_snap_subdir_quota_xattr, "test_snap_dir_quota_xattr_%d/subdir_quota", mypid);
  ASSERT_EQ(0, stone_mkdirs(cmount, test_snap_subdir_quota_xattr, 0777));

  sprintf(xattrk, "stone.quota.max_bytes");
  sprintf(xattrv, "32768");
  ASSERT_EQ(0, stone_setxattr(cmount, test_snap_subdir_quota_xattr, xattrk, (void *)xattrv, 5, XATTR_CREATE));

  // create subdir with no quota
  sprintf(test_snap_subdir_noquota_xattr, "test_snap_dir_quota_xattr_%d/subdir_noquota", mypid);
  ASSERT_EQ(0, stone_mkdirs(cmount, test_snap_subdir_noquota_xattr, 0777));

  // snapshot dir
  sprintf(c_temp, "/.snap/test_snap_dir_quota_xattr_snap_%d", mypid);
  ASSERT_EQ(0, stone_mkdirs(cmount, c_temp, 0777));

  // check dir quota under snap
  sprintf(c_temp, "/.snap/test_snap_dir_quota_xattr_snap_%d/test_snap_dir_quota_xattr_%d", mypid, mypid);
  int alen = stone_getxattr(cmount, c_temp, "stone.quota.max_bytes", (void *)gxattrv, xbuflen);
  ASSERT_LT(0, alen);
  ASSERT_LT(alen, xbuflen);
  gxattrv[alen] = '\0';
  ASSERT_STREQ(gxattrv, "65536");

  // check subdir quota under snap
  sprintf(c_temp, "/.snap/test_snap_dir_quota_xattr_snap_%d/test_snap_dir_quota_xattr_%d/subdir_quota", mypid, mypid);
  alen = stone_getxattr(cmount, c_temp, "stone.quota.max_bytes", (void *)gxattrv, xbuflen);
  ASSERT_LT(0, alen);
  ASSERT_LT(alen, xbuflen);
  gxattrv[alen] = '\0';
  ASSERT_STREQ(gxattrv, "32768");

  // ensure subdir noquota xattr under snap
  sprintf(c_temp, "/.snap/test_snap_dir_quota_xattr_snap_%d/test_snap_dir_quota_xattr_%d/subdir_noquota", mypid, mypid);
  EXPECT_EQ(-ENODATA, stone_getxattr(cmount, c_temp, "stone.quota.max_bytes", (void *)gxattrv, xbuflen));

  // listxattr() shouldn't return stone.quota.max_bytes vxattr
  sprintf(c_temp, "/.snap/test_snap_dir_quota_xattr_snap_%d/test_snap_dir_quota_xattr_%d", mypid, mypid);
  char xattrlist[512];
  int len = stone_listxattr(cmount, c_temp, xattrlist, sizeof(xattrlist));
  ASSERT_GE(sizeof(xattrlist), (size_t)len);
  char *p = xattrlist;
  int found = 0;
  while (len > 0) {
    if (strcmp(p, "stone.quota.max_bytes") == 0)
      found++;
    len -= strlen(p) + 1;
    p += strlen(p) + 1;
  }
  ASSERT_EQ(found, 0);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Lseek) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  char c_path[1024];
  sprintf(c_path, "test_lseek_%d", getpid());
  int fd = stone_open(cmount, c_path, O_RDWR|O_CREAT|O_TRUNC, 0666);
  ASSERT_LT(0, fd);

  const char *out_buf = "hello world";
  size_t size = strlen(out_buf);
  ASSERT_EQ(stone_write(cmount, fd, out_buf, size, 0), (int)size);

  /* basic SEEK_SET/END/CUR tests */
  ASSERT_EQ(0, stone_lseek(cmount, fd, 0, SEEK_SET));
  ASSERT_EQ(size, stone_lseek(cmount, fd, 0, SEEK_END));
  ASSERT_EQ(0, stone_lseek(cmount, fd, -size, SEEK_CUR));

  /* Test basic functionality and out of bounds conditions for SEEK_HOLE/DATA */
#ifdef SEEK_HOLE
  ASSERT_EQ(size, stone_lseek(cmount, fd, 0, SEEK_HOLE));
  ASSERT_EQ(-ENXIO, stone_lseek(cmount, fd, -1, SEEK_HOLE));
  ASSERT_EQ(-ENXIO, stone_lseek(cmount, fd, size + 1, SEEK_HOLE));
#endif
#ifdef SEEK_DATA
  ASSERT_EQ(0, stone_lseek(cmount, fd, 0, SEEK_DATA));
  ASSERT_EQ(-ENXIO, stone_lseek(cmount, fd, -1, SEEK_DATA));
  ASSERT_EQ(-ENXIO, stone_lseek(cmount, fd, size + 1, SEEK_DATA));
#endif

  ASSERT_EQ(0, stone_close(cmount, fd));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, SnapInfoOnNonSnapshot) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  struct snap_info info;
  ASSERT_EQ(-EINVAL, stone_get_snap_info(cmount, "/", &info));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, EmptySnapInfo) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_path[64];
  char snap_path[PATH_MAX];
  sprintf(dir_path, "/dir0_%d", getpid());
  sprintf(snap_path, "%s/.snap/snap0_%d", dir_path, getpid());

  ASSERT_EQ(0, stone_mkdir(cmount, dir_path, 0755));
  // snapshot without custom metadata
  ASSERT_EQ(0, stone_mkdir(cmount, snap_path, 0755));

  struct snap_info info;
  ASSERT_EQ(0, stone_get_snap_info(cmount, snap_path, &info));
  ASSERT_GT(info.id, 0);
  ASSERT_EQ(info.nr_snap_metadata, 0);

  ASSERT_EQ(0, stone_rmdir(cmount, snap_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, SnapInfo) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_path[64];
  char snap_name[64];
  char snap_path[PATH_MAX];
  sprintf(dir_path, "/dir0_%d", getpid());
  sprintf(snap_name, "snap0_%d", getpid());
  sprintf(snap_path, "%s/.snap/%s", dir_path, snap_name);

  ASSERT_EQ(0, stone_mkdir(cmount, dir_path, 0755));
  // snapshot with custom metadata
  struct snap_metadata snap_meta[] = {{"foo", "bar"},{"this", "that"},{"abcdefg", "12345"}};
  ASSERT_EQ(0, stone_mksnap(cmount, dir_path, snap_name, 0755, snap_meta, std::size(snap_meta)));

  struct snap_info info;
  ASSERT_EQ(0, stone_get_snap_info(cmount, snap_path, &info));
  ASSERT_GT(info.id, 0);
  ASSERT_EQ(info.nr_snap_metadata, std::size(snap_meta));
  for (size_t i = 0; i < info.nr_snap_metadata; ++i) {
    auto &k1 = info.snap_metadata[i].key;
    auto &v1 = info.snap_metadata[i].value;
    bool found = false;
    for (size_t j = 0; j < info.nr_snap_metadata; ++j) {
      auto &k2 = snap_meta[j].key;
      auto &v2 = snap_meta[j].value;
      if (strncmp(k1, k2, strlen(k1)) == 0 && strncmp(v1, v2, strlen(v1)) == 0) {
        found = true;
        break;
      }
    }
    ASSERT_TRUE(found);
  }
  stone_free_snap_info_buffer(&info);

  ASSERT_EQ(0, stone_rmsnap(cmount, dir_path, snap_name));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, LookupInoMDSDir) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  Inode *inode;
  auto ino = inodeno_t(0x100); /* rank 0 ~mdsdir */
  ASSERT_EQ(-ESTALE, stone_ll_lookup_inode(cmount, ino, &inode));
  ino = inodeno_t(0x600); /* rank 0 first stray dir */
  ASSERT_EQ(-ESTALE, stone_ll_lookup_inode(cmount, ino, &inode));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, LookupVino) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_path[64];
  char snap_name[64];
  char snapdir_path[128];
  char snap_path[256];
  char file_path[PATH_MAX];
  char snap_file[PATH_MAX];
  sprintf(dir_path, "/dir0_%d", getpid());
  sprintf(snap_name, "snap0_%d", getpid());
  sprintf(file_path, "%s/file_%d", dir_path, getpid());
  sprintf(snapdir_path, "%s/.snap", dir_path);
  sprintf(snap_path, "%s/%s", snapdir_path, snap_name);
  sprintf(snap_file, "%s/file_%d", snap_path, getpid());

  ASSERT_EQ(0, stone_mkdir(cmount, dir_path, 0755));
  int fd = stone_open(cmount, file_path, O_WRONLY|O_CREAT, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_mksnap(cmount, dir_path, snap_name, 0755, nullptr, 0));

  // record vinos for all of them
  struct stone_statx stx;
  ASSERT_EQ(0, stone_statx(cmount, dir_path, &stx, STONE_STATX_INO, 0));
  vinodeno_t dir_vino(stx.stx_ino, stx.stx_dev);

  ASSERT_EQ(0, stone_statx(cmount, file_path, &stx, STONE_STATX_INO, 0));
  vinodeno_t file_vino(stx.stx_ino, stx.stx_dev);

  ASSERT_EQ(0, stone_statx(cmount, snapdir_path, &stx, STONE_STATX_INO, 0));
  vinodeno_t snapdir_vino(stx.stx_ino, stx.stx_dev);

  ASSERT_EQ(0, stone_statx(cmount, snap_path, &stx, STONE_STATX_INO, 0));
  vinodeno_t snap_vino(stx.stx_ino, stx.stx_dev);

  ASSERT_EQ(0, stone_statx(cmount, snap_file, &stx, STONE_STATX_INO, 0));
  vinodeno_t snap_file_vino(stx.stx_ino, stx.stx_dev);

  // Remount
  ASSERT_EQ(0, stone_unmount(cmount));
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, NULL));

  // Find them all
  Inode *inode;
  ASSERT_EQ(0, stone_ll_lookup_vino(cmount, dir_vino, &inode));
  stone_ll_put(cmount, inode);
  ASSERT_EQ(0, stone_ll_lookup_vino(cmount, file_vino, &inode));
  stone_ll_put(cmount, inode);
  ASSERT_EQ(0, stone_ll_lookup_vino(cmount, snapdir_vino, &inode));
  stone_ll_put(cmount, inode);
  ASSERT_EQ(0, stone_ll_lookup_vino(cmount, snap_vino, &inode));
  stone_ll_put(cmount, inode);
  ASSERT_EQ(0, stone_ll_lookup_vino(cmount, snap_file_vino, &inode));
  stone_ll_put(cmount, inode);

  // cleanup
  ASSERT_EQ(0, stone_rmsnap(cmount, dir_path, snap_name));
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Openat) {
  pid_t mypid = getpid();
  struct stone_mount_info *cmount;
  ASSERT_EQ(0, stone_create(&cmount, NULL));
  ASSERT_EQ(0, stone_conf_read_file(cmount, NULL));
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_mount(cmount, "/"));

  char c_rel_dir[64];
  char c_dir[128];
  sprintf(c_rel_dir, "open_test_%d", mypid);
  sprintf(c_dir, "/%s", c_rel_dir);
  ASSERT_EQ(0, stone_mkdir(cmount, c_dir, 0777));

  int root_fd = stone_open(cmount, "/", O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, root_fd);

  int dir_fd = stone_openat(cmount, root_fd, c_rel_dir, O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, dir_fd);

  struct stone_statx stx;
  ASSERT_EQ(stone_statxat(cmount, root_fd, c_rel_dir, &stx, 0, 0), 0);
  ASSERT_EQ(stx.stx_mode & S_IFMT, S_IFDIR);

  char c_rel_path[64];
  char c_path[256];
  sprintf(c_rel_path, "created_file_%d", mypid);
  sprintf(c_path, "%s/created_file_%d", c_dir, mypid);
  int file_fd = stone_openat(cmount, dir_fd, c_rel_path, O_RDONLY | O_CREAT, 0666);
  ASSERT_LE(0, file_fd);

  ASSERT_EQ(stone_statxat(cmount, dir_fd, c_rel_path, &stx, 0, 0), 0);
  ASSERT_EQ(stx.stx_mode & S_IFMT, S_IFREG);

  ASSERT_EQ(0, stone_close(cmount, file_fd));
  ASSERT_EQ(0, stone_close(cmount, dir_fd));
  ASSERT_EQ(0, stone_close(cmount, root_fd));

  ASSERT_EQ(0, stone_unlink(cmount, c_path));
  ASSERT_EQ(0, stone_rmdir(cmount, c_dir));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Statxat) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_name[64];
  char rel_file_name_1[128];
  char rel_file_name_2[256];

  char dir_path[512];
  char file_path[1024];

  // relative paths for *at() calls
  sprintf(dir_name, "dir0_%d", getpid());
  sprintf(rel_file_name_1, "file_%d", getpid());
  sprintf(rel_file_name_2, "%s/%s", dir_name, rel_file_name_1);

  sprintf(dir_path, "/%s", dir_name);
  sprintf(file_path, "%s/%s", dir_path, rel_file_name_1);

  ASSERT_EQ(0, stone_mkdir(cmount, dir_path, 0755));
  int fd = stone_open(cmount, file_path, O_WRONLY|O_CREAT, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  struct stone_statx stx;

  // test relative to root
  fd = stone_open(cmount, "/", O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  ASSERT_EQ(stone_statxat(cmount, fd, dir_name, &stx, 0, 0), 0);
  ASSERT_EQ(stx.stx_mode & S_IFMT, S_IFDIR);
  ASSERT_EQ(stone_statxat(cmount, fd, rel_file_name_2, &stx, 0, 0), 0);
  ASSERT_EQ(stx.stx_mode & S_IFMT, S_IFREG);
  ASSERT_EQ(0, stone_close(cmount, fd));

  // test relative to dir
  fd = stone_open(cmount, dir_path, O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  ASSERT_EQ(stone_statxat(cmount, fd, rel_file_name_1, &stx, 0, 0), 0);
  ASSERT_EQ(stx.stx_mode & S_IFMT, S_IFREG);

  // delete the dirtree, recreate and verify
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  ASSERT_EQ(0, stone_mkdir(cmount, dir_path, 0755));
  int fd1 = stone_open(cmount, file_path, O_WRONLY|O_CREAT, 0666);
  ASSERT_LE(0, fd1);
  ASSERT_EQ(0, stone_close(cmount, fd1));
  ASSERT_EQ(stone_statxat(cmount, fd, rel_file_name_1, &stx, 0, 0), -ENOENT);
  ASSERT_EQ(0, stone_close(cmount, fd));

  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, StatxatATFDCWD) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_name[64];
  char rel_file_name_1[128];

  char dir_path[512];
  char file_path[1024];

  // relative paths for *at() calls
  sprintf(dir_name, "dir0_%d", getpid());
  sprintf(rel_file_name_1, "file_%d", getpid());

  sprintf(dir_path, "/%s", dir_name);
  sprintf(file_path, "%s/%s", dir_path, rel_file_name_1);

  ASSERT_EQ(0, stone_mkdir(cmount, dir_path, 0755));
  int fd = stone_open(cmount, file_path, O_WRONLY|O_CREAT, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  struct stone_statx stx;
  // chdir and test with STONEFS_AT_FDCWD
  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  ASSERT_EQ(stone_statxat(cmount, STONEFS_AT_FDCWD, rel_file_name_1, &stx, 0, 0), 0);
  ASSERT_EQ(stx.stx_mode & S_IFMT, S_IFREG);

  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Fdopendir) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char foostr[256];
  sprintf(foostr, "/dir_ls%d", mypid);
  ASSERT_EQ(stone_mkdir(cmount, foostr, 0777), 0);

  char bazstr[512];
  sprintf(bazstr, "%s/elif", foostr);
  int fd  = stone_open(cmount, bazstr, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, foostr, O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  struct stone_dir_result *ls_dir = NULL;
  ASSERT_EQ(stone_fdopendir(cmount, fd, &ls_dir), 0);

  // not guaranteed to get . and .. first, but its a safe assumption in this case
  struct dirent *result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "elif");

  ASSERT_TRUE(stone_readdir(cmount, ls_dir) == NULL);

  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_closedir(cmount, ls_dir));
  ASSERT_EQ(0, stone_unlink(cmount, bazstr));
  ASSERT_EQ(0, stone_rmdir(cmount, foostr));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, FdopendirATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char foostr[256];
  sprintf(foostr, "/dir_ls%d", mypid);
  ASSERT_EQ(stone_mkdir(cmount, foostr, 0777), 0);

  char bazstr[512];
  sprintf(bazstr, "%s/elif", foostr);
  int fd  = stone_open(cmount, bazstr, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  ASSERT_EQ(0, stone_chdir(cmount, foostr));
  struct stone_dir_result *ls_dir = NULL;
  ASSERT_EQ(stone_fdopendir(cmount, STONEFS_AT_FDCWD, &ls_dir), 0);

  // not guaranteed to get . and .. first, but its a safe assumption in this case
  struct dirent *result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "elif");

  ASSERT_TRUE(stone_readdir(cmount, ls_dir) == NULL);

  ASSERT_EQ(0, stone_closedir(cmount, ls_dir));
  ASSERT_EQ(0, stone_unlink(cmount, bazstr));
  ASSERT_EQ(0, stone_rmdir(cmount, foostr));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, FdopendirReaddirTestWithDelete) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char foostr[256];
  sprintf(foostr, "/dir_ls%d", mypid);
  ASSERT_EQ(stone_mkdir(cmount, foostr, 0777), 0);

  char bazstr[512];
  sprintf(bazstr, "%s/elif", foostr);
  int fd  = stone_open(cmount, bazstr, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, foostr, O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  struct stone_dir_result *ls_dir = NULL;
  ASSERT_EQ(stone_fdopendir(cmount, fd, &ls_dir), 0);

  ASSERT_EQ(0, stone_unlink(cmount, bazstr));
  ASSERT_EQ(0, stone_rmdir(cmount, foostr));

  // not guaranteed to get . and .. first, but its a safe assumption
  // in this case. also, note that we may or may not get other
  // entries.
  struct dirent *result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, ".");
  result = stone_readdir(cmount, ls_dir);
  ASSERT_TRUE(result != NULL);
  ASSERT_STREQ(result->d_name, "..");

  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_closedir(cmount, ls_dir));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, FdopendirOnNonDir) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char foostr[256];
  sprintf(foostr, "/dir_ls%d", mypid);
  ASSERT_EQ(stone_mkdir(cmount, foostr, 0777), 0);

  char bazstr[512];
  sprintf(bazstr, "%s/file", foostr);
  int fd  = stone_open(cmount, bazstr, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);

  struct stone_dir_result *ls_dir = NULL;
  ASSERT_EQ(stone_fdopendir(cmount, fd, &ls_dir), -ENOTDIR);
  ASSERT_EQ(0, stone_close(cmount, fd));

  ASSERT_EQ(0, stone_unlink(cmount, bazstr));
  ASSERT_EQ(0, stone_rmdir(cmount, foostr));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Mkdirat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path1[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path1, "/%s", dir_name);

  char dir_path2[512];
  char rel_dir_path2[512];
  sprintf(dir_path2, "%s/dir_%d", dir_path1, mypid);
  sprintf(rel_dir_path2, "%s/dir_%d", dir_name, mypid);

  int fd = stone_open(cmount, "/", O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);

  ASSERT_EQ(0, stone_mkdirat(cmount, fd, dir_name, 0777));
  ASSERT_EQ(0, stone_mkdirat(cmount, fd, rel_dir_path2, 0666));

  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path2));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path1));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, MkdiratATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path1[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path1, "/%s", dir_name);

  char dir_path2[512];
  sprintf(dir_path2, "%s/dir_%d", dir_path1, mypid);

  ASSERT_EQ(0, stone_mkdirat(cmount, STONEFS_AT_FDCWD, dir_name, 0777));

  ASSERT_EQ(0, stone_chdir(cmount, dir_path1));
  ASSERT_EQ(0, stone_mkdirat(cmount, STONEFS_AT_FDCWD, dir_name, 0666));

  ASSERT_EQ(0, stone_rmdir(cmount, dir_path2));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path1));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Readlinkat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512];
  sprintf(rel_file_path, "%s/elif", dir_name);
  sprintf(file_path, "%s/elif", dir_path);
  int fd  = stone_open(cmount, file_path, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  char link_path[128];
  char rel_link_path[64];
  sprintf(rel_link_path, "linkfile_%d", mypid);
  sprintf(link_path, "/%s", rel_link_path);
  ASSERT_EQ(0, stone_symlink(cmount, rel_file_path, link_path));

  fd = stone_open(cmount, "/", O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  size_t target_len = strlen(rel_file_path);
  char target[target_len+1];
  ASSERT_EQ(target_len, stone_readlinkat(cmount, fd, rel_link_path, target, target_len));
  target[target_len] = '\0';
  ASSERT_EQ(0, memcmp(target, rel_file_path, target_len));

  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_unlink(cmount, link_path));
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, ReadlinkatATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "./elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd  = stone_open(cmount, file_path, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  char link_path[PATH_MAX];
  char rel_link_path[1024];
  sprintf(rel_link_path, "./linkfile_%d", mypid);
  sprintf(link_path, "%s/%s", dir_path, rel_link_path);
  ASSERT_EQ(0, stone_symlink(cmount, rel_file_path, link_path));

  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  size_t target_len = strlen(rel_file_path);
  char target[target_len+1];
  ASSERT_EQ(target_len, stone_readlinkat(cmount, STONEFS_AT_FDCWD, rel_link_path, target, target_len));
  target[target_len] = '\0';
  ASSERT_EQ(0, memcmp(target, rel_file_path, target_len));

  ASSERT_EQ(0, stone_unlink(cmount, link_path));
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Symlinkat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512];
  sprintf(rel_file_path, "%s/elif", dir_name);
  sprintf(file_path, "%s/elif", dir_path);

  int fd  = stone_open(cmount, file_path, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  char link_path[128];
  char rel_link_path[64];
  sprintf(rel_link_path, "linkfile_%d", mypid);
  sprintf(link_path, "/%s", rel_link_path);

  fd = stone_open(cmount, "/", O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_symlinkat(cmount, rel_file_path, fd, rel_link_path));

  size_t target_len = strlen(rel_file_path);
  char target[target_len+1];
  ASSERT_EQ(target_len, stone_readlinkat(cmount, fd, rel_link_path, target, target_len));
  target[target_len] = '\0';
  ASSERT_EQ(0, memcmp(target, rel_file_path, target_len));

  ASSERT_EQ(0, stone_close(cmount, fd));
  ASSERT_EQ(0, stone_unlink(cmount, link_path));
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, SymlinkatATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "./elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd  = stone_open(cmount, file_path, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  char link_path[PATH_MAX];
  char rel_link_path[1024];
  sprintf(rel_link_path, "./linkfile_%d", mypid);
  sprintf(link_path, "%s/%s", dir_path, rel_link_path);
  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  ASSERT_EQ(0, stone_symlinkat(cmount, rel_file_path, STONEFS_AT_FDCWD, rel_link_path));

  size_t target_len = strlen(rel_file_path);
  char target[target_len+1];
  ASSERT_EQ(target_len, stone_readlinkat(cmount, STONEFS_AT_FDCWD, rel_link_path, target, target_len));
  target[target_len] = '\0';
  ASSERT_EQ(0, memcmp(target, rel_file_path, target_len));

  ASSERT_EQ(0, stone_unlink(cmount, link_path));
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Unlinkat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);

  int fd  = stone_open(cmount, file_path, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, dir_path, O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  ASSERT_EQ(-ENOTDIR, stone_unlinkat(cmount, fd, rel_file_path, AT_REMOVEDIR));
  ASSERT_EQ(0, stone_unlinkat(cmount, fd, rel_file_path, 0));
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, "/", O_DIRECTORY | O_RDONLY, 0);
  ASSERT_EQ(-EISDIR, stone_unlinkat(cmount, fd, dir_name, 0));
  ASSERT_EQ(0, stone_unlinkat(cmount, fd, dir_name, AT_REMOVEDIR));
  ASSERT_LE(0, fd);

  stone_shutdown(cmount);
}

TEST(LibStoneFS, UnlinkatATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);

  int fd  = stone_open(cmount, file_path, O_CREAT|O_RDONLY, 0666);
  ASSERT_LE(0, fd);
  ASSERT_EQ(0, stone_close(cmount, fd));

  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  ASSERT_EQ(-ENOTDIR, stone_unlinkat(cmount, STONEFS_AT_FDCWD, rel_file_path, AT_REMOVEDIR));
  ASSERT_EQ(0, stone_unlinkat(cmount, STONEFS_AT_FDCWD, rel_file_path, 0));

  ASSERT_EQ(0, stone_chdir(cmount, "/"));
  ASSERT_EQ(-EISDIR, stone_unlinkat(cmount, STONEFS_AT_FDCWD, dir_name, 0));
  ASSERT_EQ(0, stone_unlinkat(cmount, STONEFS_AT_FDCWD, dir_name, AT_REMOVEDIR));

  stone_shutdown(cmount);
}

TEST(LibStoneFS, Chownat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd = stone_open(cmount, file_path, O_CREAT|O_RDWR, 0666);
  ASSERT_LE(0, fd);

  // set perms to readable and writeable only by owner
  ASSERT_EQ(stone_fchmod(cmount, fd, 0600), 0);
  stone_close(cmount, fd);

  fd = stone_open(cmount, dir_path, O_DIRECTORY | O_RDONLY, 0);
  // change ownership to nobody -- we assume nobody exists and id is always 65534
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "0"), 0);
  ASSERT_EQ(stone_chownat(cmount, fd, rel_file_path, 65534, 65534, 0), 0);
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "1"), 0);
  stone_close(cmount, fd);

  fd = stone_open(cmount, file_path, O_RDWR, 0);
  ASSERT_EQ(fd, -EACCES);

  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "0"), 0);
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "1"), 0);
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, ChownatATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd = stone_open(cmount, file_path, O_CREAT|O_RDWR, 0666);
  ASSERT_LE(0, fd);

  // set perms to readable and writeable only by owner
  ASSERT_EQ(stone_fchmod(cmount, fd, 0600), 0);
  stone_close(cmount, fd);

  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  // change ownership to nobody -- we assume nobody exists and id is always 65534
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "0"), 0);
  ASSERT_EQ(stone_chownat(cmount, STONEFS_AT_FDCWD, rel_file_path, 65534, 65534, 0), 0);
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "1"), 0);

  fd = stone_open(cmount, file_path, O_RDWR, 0);
  ASSERT_EQ(fd, -EACCES);

  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "0"), 0);
  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(stone_conf_set(cmount, "client_permissions", "1"), 0);
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Chmodat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd = stone_open(cmount, file_path, O_CREAT|O_RDWR, 0666);
  ASSERT_LE(0, fd);
  const char *bytes = "foobarbaz";
  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));
  ASSERT_EQ(0, stone_close(cmount, fd));

  fd = stone_open(cmount, dir_path, O_DIRECTORY | O_RDONLY, 0);

  // set perms to read but can't write
  ASSERT_EQ(stone_chmodat(cmount, fd, rel_file_path, 0400, 0), 0);
  ASSERT_EQ(stone_open(cmount, file_path, O_RDWR, 0), -EACCES);

  // reset back to writeable
  ASSERT_EQ(stone_chmodat(cmount, fd, rel_file_path, 0600, 0), 0);
  int fd2 = stone_open(cmount, file_path, O_RDWR, 0);
  ASSERT_LE(0, fd2);

  ASSERT_EQ(0, stone_close(cmount, fd2));
  ASSERT_EQ(0, stone_close(cmount, fd));

  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, ChmodatATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, "/"), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd = stone_open(cmount, file_path, O_CREAT|O_RDWR, 0666);
  ASSERT_LE(0, fd);
  const char *bytes = "foobarbaz";
  ASSERT_EQ(stone_write(cmount, fd, bytes, strlen(bytes), 0), (int)strlen(bytes));
  ASSERT_EQ(0, stone_close(cmount, fd));

  // set perms to read but can't write
  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  ASSERT_EQ(stone_chmodat(cmount, STONEFS_AT_FDCWD, rel_file_path, 0400, 0), 0);
  ASSERT_EQ(stone_open(cmount, file_path, O_RDWR, 0), -EACCES);

  // reset back to writeable
  ASSERT_EQ(stone_chmodat(cmount, STONEFS_AT_FDCWD, rel_file_path, 0600, 0), 0);
  int fd2 = stone_open(cmount, file_path, O_RDWR, 0);
  ASSERT_LE(0, fd2);
  ASSERT_EQ(0, stone_close(cmount, fd2));

  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, Utimensat) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd = stone_open(cmount, file_path, O_CREAT|O_RDWR, 0666);
  ASSERT_LE(0, fd);

  struct timespec times[2];
  get_current_time_timespec(times);

  fd = stone_open(cmount, dir_path, O_DIRECTORY | O_RDONLY, 0);
  ASSERT_LE(0, fd);
  EXPECT_EQ(0, stone_utimensat(cmount, fd, rel_file_path, times, 0));
  stone_close(cmount, fd);

  struct stone_statx stx;
  ASSERT_EQ(stone_statx(cmount, file_path, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(times[0]));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(times[1]));

  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, UtimensatATFDCWD) {
  pid_t mypid = getpid();

  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  char dir_name[128];
  char dir_path[256];
  sprintf(dir_name, "dir_%d", mypid);
  sprintf(dir_path, "/%s", dir_name);
  ASSERT_EQ(stone_mkdir(cmount, dir_path, 0777), 0);

  char file_path[512];
  char rel_file_path[512] = "elif";
  sprintf(file_path, "%s/elif", dir_path);
  int fd = stone_open(cmount, file_path, O_CREAT|O_RDWR, 0666);
  ASSERT_LE(0, fd);

  struct timespec times[2];
  get_current_time_timespec(times);

  ASSERT_EQ(0, stone_chdir(cmount, dir_path));
  EXPECT_EQ(0, stone_utimensat(cmount, STONEFS_AT_FDCWD, rel_file_path, times, 0));

  struct stone_statx stx;
  ASSERT_EQ(stone_statx(cmount, file_path, &stx,
                       STONE_STATX_MTIME|STONE_STATX_ATIME, 0), 0);
  ASSERT_EQ(utime_t(stx.stx_atime), utime_t(times[0]));
  ASSERT_EQ(utime_t(stx.stx_mtime), utime_t(times[1]));

  ASSERT_EQ(0, stone_unlink(cmount, file_path));
  ASSERT_EQ(0, stone_rmdir(cmount, dir_path));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, SetMountTimeoutPostMount) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  ASSERT_EQ(-EINVAL, stone_set_mount_timeout(cmount, 5));
  stone_shutdown(cmount);
}

TEST(LibStoneFS, SetMountTimeout) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(0, stone_set_mount_timeout(cmount, 5));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);
  stone_shutdown(cmount);
}

TEST(LibStoneFS, LookupMdsPrivateInos) {
  struct stone_mount_info *cmount;
  ASSERT_EQ(stone_create(&cmount, NULL), 0);
  ASSERT_EQ(stone_conf_read_file(cmount, NULL), 0);
  ASSERT_EQ(0, stone_conf_parse_env(cmount, NULL));
  ASSERT_EQ(stone_mount(cmount, NULL), 0);

  Inode *inode;
  for (int ino = 0; ino < MDS_INO_SYSTEM_BASE; ino++) {
    if (MDS_IS_PRIVATE_INO(ino)) {
      ASSERT_EQ(-ESTALE, stone_ll_lookup_inode(cmount, ino, &inode));
    } else if (ino == STONE_INO_ROOT || ino == STONE_INO_GLOBAL_SNAPREALM) {
      ASSERT_EQ(0, stone_ll_lookup_inode(cmount, ino, &inode));
      stone_ll_put(cmount, inode);
    } else if (ino == STONE_INO_LOST_AND_FOUND) {
      // the ino 3 will only exists after the recovery tool ran, so
      // it may return -ESTALE with a fresh fs cluster
      int r = stone_ll_lookup_inode(cmount, ino, &inode);
      if (r == 0) {
        stone_ll_put(cmount, inode);
      } else {
        ASSERT_TRUE(r == -ESTALE);
      }
    } else {
      // currently the ino 0 and 4~99 is not useded yet.
      ASSERT_EQ(-ESTALE, stone_ll_lookup_inode(cmount, ino, &inode));
    }
  }
  stone_shutdown(cmount);
}
