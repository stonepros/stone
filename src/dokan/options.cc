/*
 * Copyright (C) 2021 SUSE LINUX GmbH
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
*/

#include "include/compat.h"
#include "include/stonefs/libstonefs.h"

#include "stone_dokan.h"
#include "utils.h"

#include "common/stone_argparse.h"
#include "common/config.h"

#include "global/global_init.h"

void print_usage() {
  const char* usage_str = R"(
Usage: stone-dokan.exe -l <mountpoint>
                      map -l <mountpoint>    Map a StoneFS filesystem
                      unmap -l <mountpoint>  Unmap a StoneFS filesystem

Map options:
  -l [ --mountpoint ] arg     mountpoint (path or drive letter) (e.g -l x)
  -x [ --root-path ] arg      mount a Stone filesystem subdirectory

  -t [ --thread-count] arg    thread count
  --operation-timeout arg     Dokan operation timeout. Default: 120s.

  --debug                     enable debug output
  --dokan-stderr              enable stderr Dokan logging

  --read-only                 read-only mount
  -o [ --win-mount-mgr]       use the Windows mount manager
  --current-session-only      expose the mount only to the current user session
  --removable                 use a removable drive
  --win-vol-name arg          The Windows volume name. Default: Stone - <fs_name>.

Unmap options:
  -l [ --mountpoint ] arg     mountpoint (path or drive letter) (e.g -l x).
                              It has to be the exact same mountpoint that was
                              used when the mapping was created.

Common Options:
)";

  std::cout << usage_str;
  generic_client_usage();
}


int parse_args(
  std::vector<const char*>& args,
  std::ostream *err_msg,
  Command *command, Config *cfg)
{
  if (args.empty()) {
    std::cout << "stone-dokan: -h or --help for usage" << std::endl;
    return -EINVAL;
  }

  std::string conf_file_list;
  std::string cluster;
  StoneInitParameters iparams = stone_argparse_early_args(
    args, STONE_ENTITY_TYPE_CLIENT, &cluster, &conf_file_list);

  ConfigProxy config{false};
  config->name = iparams.name;
  config->cluster = cluster;
  if (!conf_file_list.empty()) {
    config.parse_config_files(conf_file_list.c_str(), nullptr, 0);
  } else {
    config.parse_config_files(nullptr, nullptr, 0);
  }
  config.parse_env(STONE_ENTITY_TYPE_CLIENT);
  config.parse_argv(args);

  std::vector<const char*>::iterator i;
  std::ostringstream err;
  std::string mountpoint;
  std::string win_vol_name;

  for (i = args.begin(); i != args.end(); ) {
    if (stone_argparse_flag(args, i, "-h", "--help", (char*)NULL)) {
      *command = Command::Help;
      return 0;
    } else if (stone_argparse_flag(args, i, "-v", "--version", (char*)NULL)) {
      *command = Command::Version;
    } else if (stone_argparse_witharg(args, i, &mountpoint,
                                     "--mountpoint", "-l", (char *)NULL)) {
      cfg->mountpoint = to_wstring(mountpoint);
    } else if (stone_argparse_witharg(args, i, &cfg->root_path,
                                     "--root-path", "-x", (char *)NULL)) {
    } else if (stone_argparse_flag(args, i, "--debug", (char *)NULL)) {
      cfg->debug = true;
    } else if (stone_argparse_flag(args, i, "--dokan-stderr", (char *)NULL)) {
      cfg->dokan_stderr = true;
    } else if (stone_argparse_flag(args, i, "--read-only", (char *)NULL)) {
      cfg->readonly = true;
    } else if (stone_argparse_flag(args, i, "--removable", (char *)NULL)) {
      cfg->removable = true;
    } else if (stone_argparse_flag(args, i, "--win-mount-mgr", "-o", (char *)NULL)) {
      cfg->use_win_mount_mgr = true;
    } else if (stone_argparse_witharg(args, i, &win_vol_name,
                                     "--win-vol-name", (char *)NULL)) {
      cfg->win_vol_name = to_wstring(win_vol_name);
    } else if (stone_argparse_flag(args, i, "--current-session-only", (char *)NULL)) {
      cfg->current_session_only = true;
    } else if (stone_argparse_witharg(args, i, (int*)&cfg->thread_count,
                                     err, "--thread-count", "-t", (char *)NULL)) {
      if (!err.str().empty()) {
        *err_msg << "stone-dokan: " << err.str();
        return -EINVAL;
      }
      if (cfg->thread_count < 0) {
        *err_msg << "stone-dokan: Invalid argument for thread-count";
        return -EINVAL;
      }
    } else if (stone_argparse_witharg(args, i, (int*)&cfg->operation_timeout,
                                     err, "--operation-timeout", (char *)NULL)) {
      if (!err.str().empty()) {
        *err_msg << "stone-dokan: " << err.str();
        return -EINVAL;
      }
      if (cfg->operation_timeout < 0) {
        *err_msg << "stone-dokan: Invalid argument for operation-timeout";
        return -EINVAL;
      }
    } else {
      ++i;
    }
  }

  if (cfg->use_win_mount_mgr && cfg->current_session_only) {
    *err_msg << "stone-dokan: The mount manager always mounts the drive "
             << "for all user sessions.";
    return -EINVAL;
  }

  Command cmd = Command::None;
  if (args.begin() != args.end()) {
    if (strcmp(*args.begin(), "help") == 0) {
      cmd = Command::Help;
    } else if (strcmp(*args.begin(), "version") == 0) {
      cmd = Command::Version;
    } else if (strcmp(*args.begin(), "map") == 0) {
      cmd = Command::Map;
    } else if (strcmp(*args.begin(), "unmap") == 0) {
      cmd = Command::Unmap;
    } else {
      *err_msg << "stone-dokan: unknown command: " <<  *args.begin();
      return -EINVAL;
    }
    args.erase(args.begin());
  }
  if (cmd == Command::None) {
    // The default command.
    cmd = Command::Map;
  }

  switch (cmd) {
    case Command::Map:
    case Command::Unmap:
      if (cfg->mountpoint.empty()) {
        *err_msg << "stone-dokan: missing mountpoint.";
        return -EINVAL;
      }
      break;
    default:
      break;
  }

  if (args.begin() != args.end()) {
    *err_msg << "stone-dokan: unknown args: " << *args.begin();
    return -EINVAL;
  }

  *command = cmd;
  return 0;
}

int set_dokan_options(Config *cfg, PDOKAN_OPTIONS dokan_options) {
  ZeroMemory(dokan_options, sizeof(DOKAN_OPTIONS));
  dokan_options->Version = DOKAN_VERSION;
  dokan_options->ThreadCount = cfg->thread_count;
  dokan_options->MountPoint = cfg->mountpoint.c_str();
  dokan_options->Timeout = cfg->operation_timeout * 1000;

  if (cfg->removable)
    dokan_options->Options |= DOKAN_OPTION_REMOVABLE;
  if (cfg->use_win_mount_mgr)
    dokan_options->Options |= DOKAN_OPTION_MOUNT_MANAGER;
  if (cfg->current_session_only)
    dokan_options->Options |= DOKAN_OPTION_CURRENT_SESSION;
  if (cfg->readonly)
    dokan_options->Options |= DOKAN_OPTION_WRITE_PROTECT;
  if (cfg->debug)
    dokan_options->Options |= DOKAN_OPTION_DEBUG;
  if (cfg->dokan_stderr)
    dokan_options->Options |= DOKAN_OPTION_STDERR;

  return 0;
}
