#include <cstdarg>
#include <iostream>

#include <seastar/util/backtrace.hh>
#include <seastar/core/reactor.hh>

#include "include/stone_assert.h"

#include "crimson/common/log.h"

namespace stone {
  [[gnu::cold]] void __stone_assert_fail(const stone::assert_data &ctx)
  {
    __stone_assert_fail(ctx.assertion, ctx.file, ctx.line, ctx.function);
  }

  [[gnu::cold]] void __stone_assert_fail(const char* assertion,
                                        const char* file, int line,
                                        const char* func)
  {
    seastar::logger& logger = crimson::get_logger(0);
    logger.error("{}:{} : In function '{}', stone_assert(%s)\n"
                 "{}",
                 file, line, func, assertion,
                 seastar::current_backtrace());
    std::cout << std::flush;
    abort();
  }
  [[gnu::cold]] void __stone_assertf_fail(const char *assertion,
                                         const char *file, int line,
                                         const char *func, const char* msg,
                                         ...)
  {
    char buf[8096];
    va_list args;
    va_start(args, msg);
    std::vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);

    seastar::logger& logger = crimson::get_logger(0);
    logger.error("{}:{} : In function '{}', stone_assert(%s)\n"
                 "{}\n{}\n",
                 file, line, func, assertion,
                 buf,
                 seastar::current_backtrace());
    std::cout << std::flush;
    abort();
  }

  [[gnu::cold]] void __stone_abort(const char* file, int line,
                                  const char* func, const std::string& msg)
  {
    seastar::logger& logger = crimson::get_logger(0);
    logger.error("{}:{} : In function '{}', abort(%s)\n"
                 "{}",
                 file, line, func, msg,
                 seastar::current_backtrace());
    std::cout << std::flush;
    abort();
  }

  [[gnu::cold]] void __stone_abortf(const char* file, int line,
                                   const char* func, const char* fmt,
                                   ...)
  {
    char buf[8096];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    seastar::logger& logger = crimson::get_logger(0);
    logger.error("{}:{} : In function '{}', abort()\n"
                 "{}\n{}\n",
                 file, line, func,
                 buf,
                 seastar::current_backtrace());
    std::cout << std::flush;
    abort();
  }
}
