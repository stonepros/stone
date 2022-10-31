#ifndef TEST_STONE_TIME_H
#define TEST_STONE_TIME_H

#include <list>

#include "include/encoding.h"
#include "common/stone_time.h"
#include "common/Formatter.h"

// wrapper for stone::real_time that implements the dencoder interface
template <typename Clock>
class time_point_wrapper {
  using time_point = typename Clock::time_point;
  time_point t;
 public:
  time_point_wrapper() = default;
  explicit time_point_wrapper(const time_point& t) : t(t) {}

  void encode(bufferlist& bl) const {
    using stone::encode;
    encode(t, bl);
  }
  void decode(bufferlist::const_iterator &p) {
    using stone::decode;
    decode(t, p);
  }
  void dump(Formatter* f) {
    auto epoch_time = Clock::to_time_t(t);
    f->dump_string("time", std::ctime(&epoch_time));
  }
  static void generate_test_instances(std::list<time_point_wrapper*>& ls) {
    constexpr time_t t{455500800}; // Ghostbusters release date
    ls.push_back(new time_point_wrapper(Clock::from_time_t(t)));
  }
};

using real_time_wrapper = time_point_wrapper<stone::real_clock>;
WRITE_CLASS_ENCODER(real_time_wrapper)

using coarse_real_time_wrapper = time_point_wrapper<stone::coarse_real_clock>;
WRITE_CLASS_ENCODER(coarse_real_time_wrapper)

// wrapper for stone::timespan that implements the dencoder interface
class timespan_wrapper {
  stone::timespan d;
 public:
  timespan_wrapper() = default;
  explicit timespan_wrapper(const stone::timespan& d) : d(d) {}

  void encode(bufferlist& bl) const {
    using stone::encode;
    encode(d, bl);
  }
  void decode(bufferlist::const_iterator &p) {
    using stone::decode;
    decode(d, p);
  }
  void dump(Formatter* f) {
    f->dump_int("timespan", d.count());
  }
  static void generate_test_instances(std::list<timespan_wrapper*>& ls) {
    constexpr std::chrono::seconds d{7377}; // marathon world record (2:02:57)
    ls.push_back(new timespan_wrapper(d));
  }
};
WRITE_CLASS_ENCODER(timespan_wrapper)

#endif
