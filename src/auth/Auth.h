// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2004-2009 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef STONE_AUTHTYPES_H
#define STONE_AUTHTYPES_H

#include "Crypto.h"
#include "common/entity_name.h"

// The _MAX values are a bit wonky here because we are overloading the first
// byte of the auth payload to identify both the type of authentication to be
// used *and* the encoding version for the authenticator.  So, we define a
// range.
enum {
  AUTH_MODE_NONE = 0,
  AUTH_MODE_AUTHORIZER = 1,
  AUTH_MODE_AUTHORIZER_MAX = 9,
  AUTH_MODE_MON = 10,
  AUTH_MODE_MON_MAX = 19,
};


struct EntityAuth {
  CryptoKey key;
  std::map<std::string, stone::buffer::list> caps;

  void encode(stone::buffer::list& bl) const {
    __u8 struct_v = 2;
    using stone::encode;
    encode(struct_v, bl);
    encode((uint64_t)STONE_AUTH_UID_DEFAULT, bl);
    encode(key, bl);
    encode(caps, bl);
  }
  void decode(stone::buffer::list::const_iterator& bl) {
    using stone::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    if (struct_v >= 2) {
      uint64_t old_auid;
      decode(old_auid, bl);
    }
    decode(key, bl);
    decode(caps, bl);
  }
};
WRITE_CLASS_ENCODER(EntityAuth)

inline std::ostream& operator<<(std::ostream& out, const EntityAuth& a) {
  return out << "auth(key=" << a.key << ")";
}

struct AuthCapsInfo {
  bool allow_all;
  stone::buffer::list caps;

  AuthCapsInfo() : allow_all(false) {}

  void encode(stone::buffer::list& bl) const {
    using stone::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    __u8 a = (__u8)allow_all;
    encode(a, bl);
    encode(caps, bl);
  }
  void decode(stone::buffer::list::const_iterator& bl) {
    using stone::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    __u8 a;
    decode(a, bl);
    allow_all = (bool)a;
    decode(caps, bl);
  }
};
WRITE_CLASS_ENCODER(AuthCapsInfo)

/*
 * The ticket (if properly validated) authorizes the principal use
 * services as described by 'caps' during the specified validity
 * period.
 */
struct AuthTicket {
  EntityName name;
  uint64_t global_id; /* global instance id */
  utime_t created, renew_after, expires;
  AuthCapsInfo caps;
  __u32 flags;

  AuthTicket() : global_id(0), flags(0){}

  void init_timestamps(utime_t now, double ttl) {
    created = now;
    expires = now;
    expires += ttl;
    renew_after = now;
    renew_after += ttl / 2.0;
  }

  void encode(stone::buffer::list& bl) const {
    using stone::encode;
    __u8 struct_v = 2;
    encode(struct_v, bl);
    encode(name, bl);
    encode(global_id, bl);
    encode((uint64_t)STONE_AUTH_UID_DEFAULT, bl);
    encode(created, bl);
    encode(expires, bl);
    encode(caps, bl);
    encode(flags, bl);
  }
  void decode(stone::buffer::list::const_iterator& bl) {
    using stone::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(name, bl);
    decode(global_id, bl);
    if (struct_v >= 2) {
      uint64_t old_auid;
      decode(old_auid, bl);
    }
    decode(created, bl);
    decode(expires, bl);
    decode(caps, bl);
    decode(flags, bl);
  }
};
WRITE_CLASS_ENCODER(AuthTicket)


/*
 * abstract authorizer class
 */
struct AuthAuthorizer {
  __u32 protocol;
  stone::buffer::list bl;
  CryptoKey session_key;

  explicit AuthAuthorizer(__u32 p) : protocol(p) {}
  virtual ~AuthAuthorizer() {}
  virtual bool verify_reply(stone::buffer::list::const_iterator& reply,
			    std::string *connection_secret) = 0;
  virtual bool add_challenge(StoneContext *cct,
			     const stone::buffer::list& challenge) = 0;
};

struct AuthAuthorizerChallenge {
  virtual ~AuthAuthorizerChallenge() {}
};

struct AuthConnectionMeta {
  uint32_t auth_method = STONE_AUTH_UNKNOWN;  //< STONE_AUTH_*

  /// client: initial empty, but populated if server said bad method
  std::vector<uint32_t> allowed_methods;

  int auth_mode = AUTH_MODE_NONE;  ///< AUTH_MODE_*

  int con_mode = 0;  ///< negotiated mode

  bool is_mode_crc() const {
    return con_mode == STONE_CON_MODE_CRC;
  }
  bool is_mode_secure() const {
    return con_mode == STONE_CON_MODE_SECURE;
  }

  CryptoKey session_key;           ///< per-ticket key

  size_t get_connection_secret_length() const {
    switch (con_mode) {
    case STONE_CON_MODE_CRC:
      return 0;
    case STONE_CON_MODE_SECURE:
      return 16 * 4;
    }
    return 0;
  }
  std::string connection_secret;   ///< per-connection key

  std::unique_ptr<AuthAuthorizer> authorizer;
  std::unique_ptr<AuthAuthorizerChallenge> authorizer_challenge;

  ///< set if msgr1 peer doesn't support STONEX_V2
  bool skip_authorizer_challenge = false;
};

/*
 * Key management
 */ 
#define KEY_ROTATE_NUM 3   /* prev, current, next */

struct ExpiringCryptoKey {
  CryptoKey key;
  utime_t expiration;

  void encode(stone::buffer::list& bl) const {
    using stone::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(key, bl);
    encode(expiration, bl);
  }
  void decode(stone::buffer::list::const_iterator& bl) {
    using stone::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(key, bl);
    decode(expiration, bl);
  }
};
WRITE_CLASS_ENCODER(ExpiringCryptoKey)

inline std::ostream& operator<<(std::ostream& out, const ExpiringCryptoKey& c)
{
  return out << c.key << " expires " << c.expiration;
}

struct RotatingSecrets {
  std::map<uint64_t, ExpiringCryptoKey> secrets;
  version_t max_ver;

  RotatingSecrets() : max_ver(0) {}

  void encode(stone::buffer::list& bl) const {
    using stone::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(secrets, bl);
    encode(max_ver, bl);
  }
  void decode(stone::buffer::list::const_iterator& bl) {
    using stone::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(secrets, bl);
    decode(max_ver, bl);
  }
  
  uint64_t add(ExpiringCryptoKey& key) {
    secrets[++max_ver] = key;
    while (secrets.size() > KEY_ROTATE_NUM)
      secrets.erase(secrets.begin());
    return max_ver;
  }
  
  bool need_new_secrets() const {
    return secrets.size() < KEY_ROTATE_NUM;
  }
  bool need_new_secrets(const utime_t& now) const {
    return secrets.size() < KEY_ROTATE_NUM || current().expiration <= now;
  }

  ExpiringCryptoKey& previous() {
    return secrets.begin()->second;
  }
  ExpiringCryptoKey& current() {
    auto p = secrets.begin();
    ++p;
    return p->second;
  }
  const ExpiringCryptoKey& current() const {
    auto p = secrets.begin();
    ++p;
    return p->second;
  }
  ExpiringCryptoKey& next() {
    return secrets.rbegin()->second;
  }
  bool empty() {
    return secrets.empty();
  }

  void dump();
};
WRITE_CLASS_ENCODER(RotatingSecrets)



class KeyStore {
public:
  virtual ~KeyStore() {}
  virtual bool get_secret(const EntityName& name, CryptoKey& secret) const = 0;
  virtual bool get_service_secret(uint32_t service_id, uint64_t secret_id,
				  CryptoKey& secret) const = 0;
};

inline bool auth_principal_needs_rotating_keys(EntityName& name)
{
  uint32_t ty(name.get_type());
  return ((ty == STONE_ENTITY_TYPE_OSD)
      || (ty == STONE_ENTITY_TYPE_MDS)
      || (ty == STONE_ENTITY_TYPE_MGR));
}

#endif
