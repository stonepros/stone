#pragma once

#include "common/perf_counters.h"
#include "common/stone_mutex.h"
#include "include/common_fwd.h"

namespace stone::common {
class PerfCountersCollection
{
  StoneContext *m_cct;

  /** Protects perf_impl->m_loggers */
  mutable stone::mutex m_lock;
  PerfCountersCollectionImpl perf_impl;
public:
  PerfCountersCollection(StoneContext *cct);
  ~PerfCountersCollection();
  void add(PerfCounters *l);
  void remove(PerfCounters *l);
  void clear();
  bool reset(const std::string &name);

  void dump_formatted(stone::Formatter *f, bool schema,
                      const std::string &logger = "",
                      const std::string &counter = "");
  void dump_formatted_histograms(stone::Formatter *f, bool schema,
                                 const std::string &logger = "",
                                 const std::string &counter = "");

  void with_counters(std::function<void(const PerfCountersCollectionImpl::CounterMap &)>) const;

  friend class PerfCountersCollectionTest;
};

class PerfCountersDeleter {
  StoneContext* cct;

public:
  PerfCountersDeleter() noexcept : cct(nullptr) {}
  PerfCountersDeleter(StoneContext* cct) noexcept : cct(cct) {}
  void operator()(PerfCounters* p) noexcept;
};
}
using PerfCountersRef = std::unique_ptr<stone::common::PerfCounters, stone::common::PerfCountersDeleter>;
