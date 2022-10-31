// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_CRYPTO_CRYPTO_CONTEXT_POOL_H
#define STONE_LIBRBD_CRYPTO_CRYPTO_CONTEXT_POOL_H

#include "librbd/crypto/DataCryptor.h"
#include "common/allocator.h"
#include "include/stone_assert.h"
#include <boost/lockfree/queue.hpp>

namespace librbd {
namespace crypto {

template <typename T>
class CryptoContextPool : public DataCryptor<T>  {

public:
    CryptoContextPool(DataCryptor<T>* data_cryptor, uint32_t pool_size);
    ~CryptoContextPool();

    T* get_context(CipherMode mode) override;
    void return_context(T* ctx, CipherMode mode) override;

    inline uint32_t get_block_size() const override {
      return m_data_cryptor->get_block_size();
    }
    inline uint32_t get_iv_size() const override {
      return m_data_cryptor->get_iv_size();
    }
    inline int get_key_length() const override {
      return m_data_cryptor->get_key_length();
    }
    inline const unsigned char* get_key() const override {
      return m_data_cryptor->get_key();
    }
    inline int init_context(T* ctx, const unsigned char* iv,
                            uint32_t iv_length) const override {
      return m_data_cryptor->init_context(ctx, iv, iv_length);
    }
    inline int update_context(T* ctx, const unsigned char* in,
                              unsigned char* out,
                              uint32_t len) const override {
      return m_data_cryptor->update_context(ctx, in, out, len);
    }

    typedef boost::lockfree::queue<
            T*,
            boost::lockfree::allocator<stone::allocator<void>>> ContextQueue;

private:
    DataCryptor<T>* m_data_cryptor;
    ContextQueue m_encrypt_contexts;
    ContextQueue m_decrypt_contexts;

    inline ContextQueue& get_contexts(CipherMode mode) {
      switch(mode) {
        case CIPHER_MODE_ENC:
          return m_encrypt_contexts;
        case CIPHER_MODE_DEC:
          return m_decrypt_contexts;
        default:
          stone_assert(false);
      }
    }
};

} // namespace crypto
} // namespace librbd

#endif // STONE_LIBRBD_CRYPTO_CRYPTO_CONTEXT_POOL_H
