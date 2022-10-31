// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2016 SUSE LINUX GmbH
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONE_JOURNAL_JOURNAL_METADATA_LISTENER_H
#define STONE_JOURNAL_JOURNAL_METADATA_LISTENER_H

namespace journal {

class JournalMetadata;

struct JournalMetadataListener {
  virtual ~JournalMetadataListener() {};
  virtual void handle_update(JournalMetadata *) = 0;
};

} // namespace journal

#endif // STONE_JOURNAL_JOURNAL_METADATA_LISTENER_H

