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

#ifndef STONE_AUTH_CRYPTO_H
#define STONE_AUTH_CRYPTO_H

#include "include/common_fwd.h"
#include "include/types.h"
#include "include/utime.h"
#include "include/buffer.h"

#include <string>

class CryptoKeyContext;
namespace stone { class Formatter; }

/*
 * Random byte stream generator suitable for cryptographic use
 */
class CryptoRandom {
public:
  CryptoRandom(); // throws on failure
  ~CryptoRandom();
  /// copy up to 256 random bytes into the given buffer. throws on failure
  void get_bytes(char *buf, int len);
private:
  static int open_urandom();
  const int fd;
};

/*
 * some per-key context that is specific to a particular crypto backend
 */
class CryptoKeyHandler {
public:
  // The maximum size of a single block for all descendants of the class.
  static constexpr std::size_t MAX_BLOCK_SIZE {16};

  // A descendant pick-ups one from these and passes it to the ctor template.
  typedef std::integral_constant<std::size_t,  0> BLOCK_SIZE_0B;
  typedef std::integral_constant<std::size_t, 16> BLOCK_SIZE_16B;

  struct in_slice_t {
    const std::size_t length;
    const unsigned char* const buf;
  };

  struct out_slice_t {
    const std::size_t max_length;
    unsigned char* const buf;
  };

  stone::bufferptr secret;

  template <class BlockSizeT>
  CryptoKeyHandler(BlockSizeT) {
    static_assert(BlockSizeT::value <= MAX_BLOCK_SIZE);
  }

  virtual ~CryptoKeyHandler() {}

  virtual int encrypt(const stone::buffer::list& in,
		      stone::buffer::list& out, std::string *error) const = 0;
  virtual int decrypt(const stone::buffer::list& in,
		      stone::buffer::list& out, std::string *error) const = 0;

  // TODO: provide nullptr in the out::buf to get/estimate size requirements?
  // Or maybe dedicated methods?
  virtual std::size_t encrypt(const in_slice_t& in,
			      const out_slice_t& out) const;
  virtual std::size_t decrypt(const in_slice_t& in,
			      const out_slice_t& out) const;

  sha256_digest_t hmac_sha256(const stone::bufferlist& in) const;
};

/*
 * match encoding of struct stone_secret
 */
class CryptoKey {
protected:
  __u16 type;
  utime_t created;
  stone::buffer::ptr secret;   // must set this via set_secret()!

  // cache a pointer to the implementation-specific key handler, so we
  // don't have to create it for every crypto operation.
  mutable std::shared_ptr<CryptoKeyHandler> ckh;

  int _set_secret(int type, const stone::buffer::ptr& s);

public:
  CryptoKey() : type(0) { }
  CryptoKey(int t, utime_t c, stone::buffer::ptr& s)
    : created(c) {
    _set_secret(t, s);
  }
  ~CryptoKey() {
  }

  void encode(stone::buffer::list& bl) const;
  void decode(stone::buffer::list::const_iterator& bl);

  int get_type() const { return type; }
  utime_t get_created() const { return created; }
  void print(std::ostream& out) const;

  int set_secret(int type, const stone::buffer::ptr& s, utime_t created);
  const stone::buffer::ptr& get_secret() { return secret; }
  const stone::buffer::ptr& get_secret() const { return secret; }

  bool empty() const { return ckh.get() == nullptr; }

  void encode_base64(std::string& s) const {
    stone::buffer::list bl;
    encode(bl);
    stone::bufferlist e;
    bl.encode_base64(e);
    e.append('\0');
    s = e.c_str();
  }
  std::string encode_base64() const {
    std::string s;
    encode_base64(s);
    return s;
  }
  void decode_base64(const std::string& s) {
    stone::buffer::list e;
    e.append(s);
    stone::buffer::list bl;
    bl.decode_base64(e);
    auto p = std::cbegin(bl);
    decode(p);
  }

  void encode_formatted(std::string label, stone::Formatter *f,
			stone::buffer::list &bl);
  void encode_plaintext(stone::buffer::list &bl);

  // --
  int create(StoneContext *cct, int type);
  int encrypt(StoneContext *cct, const stone::buffer::list& in,
	      stone::buffer::list& out,
	      std::string *error) const {
    stone_assert(ckh); // Bad key?
    return ckh->encrypt(in, out, error);
  }
  int decrypt(StoneContext *cct, const stone::buffer::list& in,
	      stone::buffer::list& out,
	      std::string *error) const {
    stone_assert(ckh); // Bad key?
    return ckh->decrypt(in, out, error);
  }

  using in_slice_t = CryptoKeyHandler::in_slice_t;
  using out_slice_t = CryptoKeyHandler::out_slice_t;

  std::size_t encrypt(StoneContext*, const in_slice_t& in,
		      const out_slice_t& out) {
    stone_assert(ckh);
    return ckh->encrypt(in, out);
  }
  std::size_t decrypt(StoneContext*, const in_slice_t& in,
		      const out_slice_t& out) {
    stone_assert(ckh);
    return ckh->encrypt(in, out);
  }

  sha256_digest_t hmac_sha256(StoneContext*, const stone::buffer::list& in) {
    stone_assert(ckh);
    return ckh->hmac_sha256(in);
  }

  static constexpr std::size_t get_max_outbuf_size(std::size_t want_size) {
    return want_size + CryptoKeyHandler::MAX_BLOCK_SIZE;
  }

  void to_str(std::string& s) const;
};
WRITE_CLASS_ENCODER(CryptoKey)

inline std::ostream& operator<<(std::ostream& out, const CryptoKey& k)
{
  k.print(out);
  return out;
}


/*
 * Driver for a particular algorithm
 *
 * To use these functions, you need to call stone::crypto::init(), see
 * common/stone_crypto.h. common_init_finish does this for you.
 */
class CryptoHandler {
public:
  virtual ~CryptoHandler() {}
  virtual int get_type() const = 0;
  virtual int create(CryptoRandom *random, stone::buffer::ptr& secret) = 0;
  virtual int validate_secret(const stone::buffer::ptr& secret) = 0;
  virtual CryptoKeyHandler *get_key_handler(const stone::buffer::ptr& secret,
					    std::string& error) = 0;

  static CryptoHandler *create(int type);
};


#endif
