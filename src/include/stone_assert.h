#ifndef STONE_ASSERT_H
#define STONE_ASSERT_H

#include <cstdlib>
#include <string>

#ifndef __STRING
# define __STRING(x) #x
#endif

#if defined(__linux__)
#include <features.h>

#elif defined(__FreeBSD__)
#include <sys/cdefs.h>
#define	__GNUC_PREREQ(minor, major)	__GNUC_PREREQ__(minor, major)
#elif defined(__sun) || defined(_AIX)
#include "include/compat.h"
#include <assert.h>
#endif

#ifdef __STONE__
# include "acconfig.h"
#endif

#include "include/common_fwd.h"

namespace stone {

struct BackTrace;

/*
 * Select a function-name variable based on compiler tests, and any compiler
 * specific overrides.
 */
#if defined(HAVE_PRETTY_FUNC)
# define __STONE_ASSERT_FUNCTION __PRETTY_FUNCTION__
#elif defined(HAVE_FUNC)
# define __STONE_ASSERT_FUNCTION __func__
#else
# define __STONE_ASSERT_FUNCTION ((__const char *) 0)
#endif

extern void register_assert_context(StoneContext *cct);

struct assert_data {
  const char *assertion;
  const char *file;
  const int line;
  const char *function;
};

extern void __stone_assert_fail(const char *assertion, const char *file, int line, const char *function)
  __attribute__ ((__noreturn__));
extern void __stone_assert_fail(const assert_data &ctx)
  __attribute__ ((__noreturn__));

extern void __stone_assertf_fail(const char *assertion, const char *file, int line, const char *function, const char* msg, ...)
  __attribute__ ((__noreturn__));
extern void __stone_assert_warn(const char *assertion, const char *file, int line, const char *function);

[[noreturn]] void __stone_abort(const char *file, int line, const char *func,
                               const std::string& msg);

[[noreturn]] void __stone_abortf(const char *file, int line, const char *func,
                                const char* msg, ...);

#define _STONE_ASSERT_VOID_CAST static_cast<void>

#define assert_warn(expr)							\
  ((expr)								\
   ? _STONE_ASSERT_VOID_CAST (0)					\
   : ::stone::__stone_assert_warn (__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION))

}

using namespace stone;


/*
 * stone_abort aborts the program with a nice backtrace.
 *
 * Currently, it's the same as assert(0), but we may one day make assert a
 * debug-only thing, like it is in many projects.
 */
#define stone_abort(msg, ...)                                            \
  ::stone::__stone_abort( __FILE__, __LINE__, __STONE_ASSERT_FUNCTION, "abort() called")

#define stone_abort_msg(msg)                                             \
  ::stone::__stone_abort( __FILE__, __LINE__, __STONE_ASSERT_FUNCTION, msg) 

#define stone_abort_msgf(...)                                             \
  ::stone::__stone_abortf( __FILE__, __LINE__, __STONE_ASSERT_FUNCTION, __VA_ARGS__)

#ifdef __SANITIZE_ADDRESS__
#define stone_assert(expr)                           \
  do {                                              \
    ((expr))                                        \
    ? _STONE_ASSERT_VOID_CAST (0)                    \
      : ::stone::__stone_assert_fail(__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION); \
  } while (false)
#else
#define stone_assert(expr)							\
  do { static const stone::assert_data assert_data_ctx = \
   {__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION}; \
   ((expr) \
   ? _STONE_ASSERT_VOID_CAST (0) \
    : ::stone::__stone_assert_fail(assert_data_ctx)); } while(false)
#endif

// this variant will *never* get compiled out to NDEBUG in the future.
// (stone_assert currently doesn't either, but in the future it might.)
#ifdef __SANITIZE_ADDRESS__
#define stone_assert_always(expr)                    \
  do {                                              \
    ((expr))                                        \
    ? _STONE_ASSERT_VOID_CAST (0)                    \
      : ::stone::__stone_assert_fail(__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION); \
  } while(false)
#else
#define stone_assert_always(expr)							\
  do { static const stone::assert_data assert_data_ctx = \
   {__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION}; \
   ((expr) \
   ? _STONE_ASSERT_VOID_CAST (0) \
    : ::stone::__stone_assert_fail(assert_data_ctx)); } while(false)
#endif

// Named by analogy with printf.  Along with an expression, takes a format
// string and parameters which are printed if the assertion fails.
#define assertf(expr, ...)                  \
  ((expr)								\
   ? _STONE_ASSERT_VOID_CAST (0)					\
   : ::stone::__stone_assertf_fail (__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION, __VA_ARGS__))
#define stone_assertf(expr, ...)                  \
  ((expr)								\
   ? _STONE_ASSERT_VOID_CAST (0)					\
   : ::stone::__stone_assertf_fail (__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION, __VA_ARGS__))

// this variant will *never* get compiled out to NDEBUG in the future.
// (stone_assertf currently doesn't either, but in the future it might.)
#define stone_assertf_always(expr, ...)                  \
  ((expr)								\
   ? _STONE_ASSERT_VOID_CAST (0)					\
   : ::stone::__stone_assertf_fail (__STRING(expr), __FILE__, __LINE__, __STONE_ASSERT_FUNCTION, __VA_ARGS__))

#endif
