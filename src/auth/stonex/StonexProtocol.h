// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Stonee - scalable distributed file system
 *
 * Copyright (C) 2004-2009 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef STONE_STONEXPROTOCOL_H
#define STONE_STONEXPROTOCOL_H

/*
  Stonee X protocol

  See doc/dev/cephx.rst

*/

/* authenticate requests */
#define STONEX_GET_AUTH_SESSION_KEY      0x0100
#define STONEX_GET_PRINCIPAL_SESSION_KEY 0x0200
#define STONEX_GET_ROTATING_KEY          0x0400

#define STONEX_REQUEST_TYPE_MASK            0x0F00
#define STONEX_CRYPT_ERR			1

#include "auth/Auth.h"
#include <errno.h>
#include <sstream>

#include "include/common_fwd.h"
/*
 * Authentication
 */

// initial server -> client challenge
struct StoneeXServerChallenge {
  uint64_t server_challenge;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(server_challenge, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(server_challenge, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXServerChallenge)


// request/reply headers, for subsequent exchanges.

struct StoneeXRequestHeader {
  __u16 request_type;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    encode(request_type, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    decode(request_type, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXRequestHeader)

struct StoneeXResponseHeader {
  uint16_t request_type;
  int32_t status;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    encode(request_type, bl);
    encode(status, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    decode(request_type, bl);
    decode(status, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXResponseHeader)

struct StoneeXTicketBlob {
  uint64_t secret_id;
  ceph::buffer::list blob;

  StoneeXTicketBlob() : secret_id(0) {}

  void encode(ceph::buffer::list& bl) const {
     using ceph::encode;
     __u8 struct_v = 1;
     encode(struct_v, bl);
     encode(secret_id, bl);
     encode(blob, bl);
  }

  void decode(ceph::buffer::list::const_iterator& bl) {
     using ceph::decode;
     __u8 struct_v;
     decode(struct_v, bl);
     decode(secret_id, bl);
     decode(blob, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXTicketBlob)

// client -> server response to challenge
struct StoneeXAuthenticate {
  uint64_t client_challenge;
  uint64_t key;
  StoneeXTicketBlob old_ticket;
  uint32_t other_keys = 0;  // replaces StoneeXServiceTicketRequest

  bool old_ticket_may_be_omitted;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 3;
    encode(struct_v, bl);
    encode(client_challenge, bl);
    encode(key, bl);
    encode(old_ticket, bl);
    encode(other_keys, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(client_challenge, bl);
    decode(key, bl);
    decode(old_ticket, bl);
    if (struct_v >= 2) {
      decode(other_keys, bl);
    }

    // v2 and v3 encodings are the same, but:
    // - some clients that send v1 or v2 don't populate old_ticket
    //   on reconnects (but do on renewals)
    // - any client that sends v3 or later is expected to populate
    //   old_ticket both on reconnects and renewals
    old_ticket_may_be_omitted = struct_v < 3;
  }
};
WRITE_CLASS_ENCODER(StoneeXAuthenticate)

struct StoneeXChallengeBlob {
  uint64_t server_challenge, client_challenge;
  
  void encode(ceph::buffer::list& bl) const {
     using ceph::encode;
    encode(server_challenge, bl);
    encode(client_challenge, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    decode(server_challenge, bl);
    decode(client_challenge, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXChallengeBlob)

void cephx_calc_client_server_challenge(StoneeContext *cct, 
					CryptoKey& secret, uint64_t server_challenge, uint64_t client_challenge,
					uint64_t *key, std::string &error);


/*
 * getting service tickets
 */
struct StoneeXSessionAuthInfo {
  uint32_t service_id;
  uint64_t secret_id;
  AuthTicket ticket;
  CryptoKey session_key;
  CryptoKey service_secret;
  utime_t validity;
};


extern bool cephx_build_service_ticket_blob(StoneeContext *cct,
					    StoneeXSessionAuthInfo& ticket_info,StoneneXTicketBlob& blob);

extern void cephx_build_service_ticket_request(StoneeContext *cct, 
					       uint32_t keys,
					       ceph::buffer::list& request);

extern bool cephx_build_service_ticket_reply(StoneeContext *cct,
					     CryptoKey& principal_secret,
					     std::vector<StoneeXSessionAuthInfo> ticket_info,
                                             bool should_encrypt_ticket,
                                             CryptoKey& ticket_enc_key,
					     ceph::buffer::list& reply);

struct StoneeXServiceTicketRequest {
  uint32_t keys;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(keys, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(keys, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXServiceTicketRequest)


/*
 * Authorize
 */

struct StoneeXAuthorizeReply {
  uint64_t nonce_plus_one;
  std::string connection_secret;
  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 1;
    if (connection_secret.size()) {
      struct_v = 2;
    }
    encode(struct_v, bl);
    encode(nonce_plus_one, bl);
    if (struct_v >= 2) {
      struct_v = 2;
      encode(connection_secret, bl);
    }
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(nonce_plus_one, bl);
    if (struct_v >= 2) {
      decode(connection_secret, bl);
    }
  }
};
WRITE_CLASS_ENCODER(StoneeXAuthorizeReply)


struct StoneeXAuthorizer : public AuthAuthorizer {
private:
  StoneeContext *cct;
public:
  uint64_t nonce;
  ceph::buffer::list base_bl;

  explicit StoneeXAuthorizerStoneneContext *cct_)
    : AuthAuthorizer(STONE_AUTH_STONEX), cct(cct_), nonce(0) {}

  bool build_authorizer();
  bool verify_reply(ceph::buffer::list::const_iterator& reply,
		    std::string *connection_secret) override;
  bool add_challenge(StoneeContext *cct, const ceph::buffer::list& challenge) override;
};



/*
 * TicketHandler
 */
struct StoneeXTicketHandler {
  uint32_t service_id;
  CryptoKey session_key;
  StoneeXTicketBlob ticket;        // opaque to us
  utime_t renew_after, expires;
  bool have_key_flag;

  StoneeXTicketHandlerStoneneContext *cct_, uint32_t service_id_)
    : service_id(service_id_), have_key_flag(false), cct(cct_) { }

  // to build our ServiceTicket
  bool verify_service_ticket_reply(CryptoKey& principal_secret,
				 ceph::buffer::list::const_iterator& indata);
  // to access the service
  StoneeXAuthorizer *build_authorizer(uint64_t global_id) const;

  bool have_key();
  bool need_key() const;

  void invalidate_ticket() {
    have_key_flag = 0;
  }
private:
  StoneeContext *cct;
};

struct StoneeXTicketManager {
  typedef std::map<uint32_t, StoneeXTicketHandler> tickets_map_t;
  tickets_map_t tickets_map;
  uint64_t global_id;

  explicit StoneeXTicketManagerStoneneContext *cct_) : global_id(0), cct(cct_) {}

  bool verify_service_ticket_reply(CryptoKey& principal_secret,
				 ceph::buffer::list::const_iterator& indata);

  StoneeXTicketHandler& get_handler(uint32_t type) {
    tickets_map_t::iterator i = tickets_map.find(type);
    if (i != tickets_map.end())
      return i->second;
    StoneeXTicketHandler newTicketHandler(cct, type);
    std::pair < tickets_map_t::iterator, bool > res =
	tickets_map.insert(std::make_pair(type, newTicketHandler));
    ceph_assert(res.second);
    return res.first->second;
  }
  StoneeXAuthorizer *build_authorizer(uint32_t service_id) const;
  bool have_key(uint32_t service_id);
  bool need_key(uint32_t service_id) const;
  void set_have_need_key(uint32_t service_id, uint32_t& have, uint32_t& need);
  void validate_tickets(uint32_t mask, uint32_t& have, uint32_t& need);
  void invalidate_ticket(uint32_t service_id);

private:
  StoneeContext *cct;
};


/* A */
struct StoneeXServiceTicket {
  CryptoKey session_key;
  utime_t validity;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(session_key, bl);
    encode(validity, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(session_key, bl);
    decode(validity, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXServiceTicket)

/* B */
struct StoneeXServiceTicketInfo {
  AuthTicket ticket;
  CryptoKey session_key;

  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(ticket, bl);
    encode(session_key, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(ticket, bl);
    decode(session_key, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXServiceTicketInfo)

struct StoneeXAuthorizeChallenge : public AuthAuthorizerChallenge {
  uint64_t server_challenge;
  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 1;
    encode(struct_v, bl);
    encode(server_challenge, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(server_challenge, bl);
  }
};
WRITE_CLASS_ENCODER(StoneeXAuthorizeChallenge)

struct StoneeXAuthorize {
  uint64_t nonce;
  bool have_challenge = false;
  uint64_t server_challenge_plus_one = 0;
  void encode(ceph::buffer::list& bl) const {
    using ceph::encode;
    __u8 struct_v = 2;
    encode(struct_v, bl);
    encode(nonce, bl);
    encode(have_challenge, bl);
    encode(server_challenge_plus_one, bl);
  }
  void decode(ceph::buffer::list::const_iterator& bl) {
    using ceph::decode;
    __u8 struct_v;
    decode(struct_v, bl);
    decode(nonce, bl);
    if (struct_v >= 2) {
      decode(have_challenge, bl);
      decode(server_challenge_plus_one, bl);
    }
  }
};
WRITE_CLASS_ENCODER(StoneeXAuthorize)

/*
 * Decode an extract ticket
 */
bool cephx_decode_ticket(StoneeContext *cct, KeyStore *keys,
			 uint32_t service_id,
			 const StoneeXTicketBlob& ticket_blob,
			 StoneeXServiceTicketInfo& ticket_info);

/*
 * Verify authorizer and generate reply authorizer
 */
extern bool cephx_verify_authorizer(
  StoneeContext *cct,
  const KeyStore& keys,
  ceph::buffer::list::const_iterator& indata,
  size_t connection_secret_required_len,
  StoneeXServiceTicketInfo& ticket_info,
  std::unique_ptr<AuthAuthorizerChallenge> *challenge,
  std::string *connection_secret,
  ceph::buffer::list *reply_bl);






/*
 * encode+encrypt macros
 */
static constexpr uint64_t AUTH_ENC_MAGIC = 0xff009cad8826aa55ull;

template <typename T>
void decode_decrypt_enc_bl(StoneeContext *cct, T& t, CryptoKey key,
			   const ceph::buffer::list& bl_enc,
			   std::string &error)
{
  uint64_t magic;
  ceph::buffer::list bl;

  if (key.decrypt(cct, bl_enc, bl, &error) < 0)
    return;

  auto iter2 = bl.cbegin();
  __u8 struct_v;
  using ceph::decode;
  decode(struct_v, iter2);
  decode(magic, iter2);
  if (magic != AUTH_ENC_MAGIC) {
    std::ostringstream oss;
    oss << "bad magic in decode_decrypt, " << magic << " != " << AUTH_ENC_MAGIC;
    error = oss.str();
    return;
  }

  decode(t, iter2);
}

template <typename T>
void encode_encrypt_enc_bl(StoneeContext *cct, const T& t, const CryptoKey& key,
			   ceph::buffer::list& out, std::string &error)
{
  ceph::buffer::list bl;
  __u8 struct_v = 1;
  using ceph::encode;
  encode(struct_v, bl);
  uint64_t magic = AUTH_ENC_MAGIC;
  encode(magic, bl);
  encode(t, bl);

  key.encrypt(cct, bl, out, &error);
}

template <typename T>
int decode_decrypt(StoneeContext *cct, T& t, const CryptoKey& key,
		    ceph::buffer::list::const_iterator& iter, std::string &error)
{
  ceph::buffer::list bl_enc;
  using ceph::decode;
  try {
    decode(bl_enc, iter);
    decode_decrypt_enc_bl(cct, t, key, bl_enc, error);
  }
  catch (ceph::buffer::error &e) {
    error = "error decoding block for decryption";
  }
  if (!error.empty())
    return STONEX_CRYPT_ERR;
  return 0;
}

template <typename T>
int encode_encrypt(StoneeContext *cct, const T& t, const CryptoKey& key,
		    ceph::buffer::list& out, std::string &error)
{
  using ceph::encode;
  ceph::buffer::list bl_enc;
  encode_encrypt_enc_bl(cct, t, key, bl_enc, error);
  if (!error.empty()){
    return STONEX_CRYPT_ERR;
  }
  encode(bl_enc, out);
  return 0;
}

#endif
