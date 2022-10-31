// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef STONE_LIBRBD_CRYPTO_LUKS_HEADER_H
#define STONE_LIBRBD_CRYPTO_LUKS_HEADER_H

#include <libcryptsetup.h>
#include "common/stone_context.h"
#include "include/buffer.h"

namespace librbd {
namespace crypto {
namespace luks {

class Header {
public:
    Header(StoneContext* cct);
    ~Header();
    int init();

    int write(const stone::bufferlist& bl);
    ssize_t read(stone::bufferlist* bl);

    int format(const char* type, const char* alg, const char* key,
               size_t key_size, const char* cipher_mode, uint32_t sector_size,
               uint32_t data_alignment, bool insecure_fast_mode);
    int add_keyslot(const char* passphrase, size_t passphrase_size);
    int load(const char* type);
    int read_volume_key(const char* passphrase, size_t passphrase_size,
                        char* volume_key, size_t* volume_key_size);

    int get_sector_size();
    uint64_t get_data_offset();
    const char* get_cipher();
    const char* get_cipher_mode();

private:
    void libcryptsetup_log(int level, const char* msg);
    static void libcryptsetup_log_wrapper(int level, const char* msg,
                                          void* header);

    StoneContext* m_cct;
    int m_fd;
    struct crypt_device *m_cd;
};

} // namespace luks
} // namespace crypto
} // namespace librbd

#endif // STONE_LIBRBD_CRYPTO_LUKS_HEADER_H
