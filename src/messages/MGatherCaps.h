#ifndef STONE_MGATHERCAPS_H
#define STONE_MGATHERCAPS_H

#include "messages/MMDSOp.h"


class MGatherCaps final : public MMDSOp {
  static constexpr int HEAD_VERSION = 1;
  static constexpr int COMPAT_VERSION = 1;

public:
  inodeno_t ino;

protected:
  MGatherCaps() :
    MMDSOp{MSG_MDS_GATHERCAPS, HEAD_VERSION, COMPAT_VERSION} {}
  ~MGatherCaps() final {}

public:
  std::string_view get_type_name() const override { return "gather_caps"; }
  void print(std::ostream& o) const override {
    o << "gather_caps(" << ino << ")";
  }

  void encode_payload(uint64_t features) override {
    using stone::encode;
    encode(ino, payload);
  }
  void decode_payload() override {
    using stone::decode;
    auto p = payload.cbegin();
    decode(ino, p);
  }
private:
  template<class T, typename... Args>
  friend boost::intrusive_ptr<T> stone::make_message(Args&&... args);
};

#endif
