#include "stone_releases.h"

#include <ostream>

#include "stone_ver.h"

std::ostream& operator<<(std::ostream& os, const stone_release_t r)
{
  return os << stone_release_name(static_cast<int>(r));
}

stone_release_t stone_release()
{
  return stone_release_t{STONE_RELEASE};
}

stone_release_t stone_release_from_name(std::string_view s)
{
  stone_release_t r = stone_release_t::max;
  while (--r != stone_release_t::unknown) {
    if (s == to_string(r)) {
      return r;
    }
  }
  return stone_release_t::unknown;
}

bool can_upgrade_from(stone_release_t from_release,
                      std::string_view from_release_name,
                      std::ostream& err)
{
  if (from_release == stone_release_t::unknown) {
    // cannot tell, but i am optimistic
    return true;
  }
  const stone_release_t cutoff{static_cast<uint8_t>(static_cast<uint8_t>(from_release) + 2)};
  const auto to_release = stone_release();
  if (cutoff < to_release) {
    err << "recorded " << from_release_name << " "
        << to_integer<int>(from_release) << " (" << from_release << ") "
        << "is more than two releases older than installed "
        << to_integer<int>(to_release) << " (" << to_release << "); "
        << "you can only upgrade 2 releases at a time\n"
        << "you should first upgrade to ";
    auto release = from_release;
    while (++release <= cutoff) {
      err << to_integer<int>(release) << " (" << release << ")";
      if (release < cutoff) {
        err << " or ";
      } else {
        err << "\n";
      }
    }
    return false;
  } else {
    return true;
  }
}
