#include "auth/stonex/StonexKeyServer.h"
#include "common/stone_argparse.h"
#include "global/global_init.h"
#include "common/config.h"
#include "common/debug.h"

#define dout_context g_stone_context

#define AES_KEY_LEN	16

int main(int argc, const char **argv)
{
  vector<const char*> args;
  argv_to_vec(argc, argv, args);

  auto cct = global_init(NULL, args, STONE_ENTITY_TYPE_CLIENT,
                         CODE_ENVIRONMENT_UTILITY,
			 CINIT_FLAG_NO_DEFAULT_CONFIG_FILE);
  common_init_finish(g_stone_context);
  KeyRing extra;
  KeyServer server(g_stone_context, &extra);

  generic_dout(0) << "server created" << dendl;

  getchar();

#if 0
  char aes_key[AES_KEY_LEN];
  memset(aes_key, 0x77, sizeof(aes_key));
  bufferptr keybuf(aes_key, sizeof(aes_key));
  CryptoKey key(STONE_CRYPTO_AES, stone_clock_now(), keybuf);

  const char *msg="hello! this is a message\n";
  char pad[16];
  memset(pad, 0, 16);
  bufferptr ptr(msg, strlen(msg));
  bufferlist enc_in;
  enc_in.append(ptr);
  enc_in.append(msg, strlen(msg));

  bufferlist enc_out;
  if (key.encrypt(enc_in, enc_out) < 0) {
    derr(0) << "couldn't encode!" << dendl;
    exit(1);
  }

  const char *enc_buf = enc_out.c_str();
  for (unsigned i=0; i<enc_out.length(); i++) {
    std::cout << hex << (int)(unsigned char)enc_buf[i] << dec << " ";
    if (i && !(i%16))
      std::cout << std::endl;
  }

  bufferlist dec_in, dec_out;

  dec_in = enc_out;

  if (key.decrypt(dec_in, dec_out) < 0) {
    derr(0) << "couldn't decode!" << dendl;
  }

  dout(0) << "decoded len: " << dec_out.length() << dendl;
  dout(0) << "decoded msg: " << dec_out.c_str() << dendl;

  return 0;
#endif
}

