// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "include/crc32c.h"
#include "arch/probe.h"
#include "arch/intel.h"
#include "arch/arm.h"
#include "arch/ppc.h"
#include "common/sctp_crc32.h"
#include "common/crc32c_intel_fast.h"
#include "common/crc32c_aarch64.h"
#include "common/crc32c_ppc.h"

/*
 * choose best implementation based on the CPU architecture.
 */
stone_crc32c_func_t stone_choose_crc32(void)
{
  // make sure we've probed cpu features; this might depend on the
  // link order of this file relative to arch/probe.cc.
  stone_arch_probe();

  // if the CPU supports it, *and* the fast version is compiled in,
  // use that.
#if defined(__i386__) || defined(__x86_64__)
  if (stone_arch_intel_sse42 && stone_crc32c_intel_fast_exists()) {
    return stone_crc32c_intel_fast;
  }
#elif defined(__arm__) || defined(__aarch64__)
# if defined(HAVE_ARMV8_CRC)
  if (stone_arch_aarch64_crc32){
    return stone_crc32c_aarch64;
  }
# endif
#elif defined(__powerpc__) || defined(__ppc__)
  if (stone_arch_ppc_crc32) {
    return stone_crc32c_ppc;
  }
#endif
  // default
  return stone_crc32c_sctp;
}

/*
 * static global
 *
 * This is a bit of a no-no for shared libraries, but we don't care.
 * It is effectively constant for the executing process as the value
 * depends on the CPU architecture.
 *
 * We initialize it during program init using the magic of C++.
 */
stone_crc32c_func_t stone_crc32c_func = stone_choose_crc32();


/*
 * Look: http://crcutil.googlecode.com/files/crc-doc.1.0.pdf
 * Here is implementation that goes 1 logical step further,
 * it splits calculating CRC into jumps of length 1, 2, 4, 8, ....
 * Each jump is performed on single input bit separately, xor-ed after that.
 *
 * This function is unused. It is here to show how crc_turbo_table was obtained.
 */
void create_turbo_table(uint32_t table[32][32])
{
  //crc_turbo_struct table;
  for (int bit = 0 ; bit < 32 ; bit++) {
    table[0][bit] = stone_crc32c_sctp(1UL << bit, nullptr, 1);
  }
  for (int range = 1; range <32 ; range++) {
    for (int bit = 0 ; bit < 32 ; bit++) {
      uint32_t crc_x = table[range-1][bit];
      uint32_t crc_y = 0;
      for (int b = 0 ; b < 32 ; b++) {
        if ( (crc_x & (1UL << b)) != 0 ) {
          crc_y = crc_y ^ table[range-1][b];
        }
      }
      table[range][bit] = crc_y;
    }
  }
}

static uint32_t crc_turbo_table[32][32] =
{
    {0xf26b8303, 0xe13b70f7, 0xc79a971f, 0x8ad958cf, 0x105ec76f, 0x20bd8ede, 0x417b1dbc, 0x82f63b78,
     0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
     0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
     0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000},
    {0x13a29877, 0x274530ee, 0x4e8a61dc, 0x9d14c3b8, 0x3fc5f181, 0x7f8be302, 0xff17c604, 0xfbc3faf9,
     0xf26b8303, 0xe13b70f7, 0xc79a971f, 0x8ad958cf, 0x105ec76f, 0x20bd8ede, 0x417b1dbc, 0x82f63b78,
     0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
     0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000},
    {0xdd45aab8, 0xbf672381, 0x7b2231f3, 0xf64463e6, 0xe964b13d, 0xd725148b, 0xaba65fe7, 0x52a0c93f,
     0xa541927e, 0x4f6f520d, 0x9edea41a, 0x38513ec5, 0x70a27d8a, 0xe144fb14, 0xc76580d9, 0x8b277743,
     0x13a29877, 0x274530ee, 0x4e8a61dc, 0x9d14c3b8, 0x3fc5f181, 0x7f8be302, 0xff17c604, 0xfbc3faf9,
     0xf26b8303, 0xe13b70f7, 0xc79a971f, 0x8ad958cf, 0x105ec76f, 0x20bd8ede, 0x417b1dbc, 0x82f63b78},
    {0x493c7d27, 0x9278fa4e, 0x211d826d, 0x423b04da, 0x847609b4, 0x0d006599, 0x1a00cb32, 0x34019664,
     0x68032cc8, 0xd0065990, 0xa5e0c5d1, 0x4e2dfd53, 0x9c5bfaa6, 0x3d5b83bd, 0x7ab7077a, 0xf56e0ef4,
     0xef306b19, 0xdb8ca0c3, 0xb2f53777, 0x6006181f, 0xc00c303e, 0x85f4168d, 0x0e045beb, 0x1c08b7d6,
     0x38116fac, 0x7022df58, 0xe045beb0, 0xc5670b91, 0x8f2261d3, 0x1ba8b557, 0x37516aae, 0x6ea2d55c},
    {0xf20c0dfe, 0xe1f46d0d, 0xc604aceb, 0x89e52f27, 0x162628bf, 0x2c4c517e, 0x5898a2fc, 0xb13145f8,
     0x678efd01, 0xcf1dfa02, 0x9bd782f5, 0x3243731b, 0x6486e636, 0xc90dcc6c, 0x97f7ee29, 0x2a03aaa3,
     0x54075546, 0xa80eaa8c, 0x55f123e9, 0xabe247d2, 0x5228f955, 0xa451f2aa, 0x4d4f93a5, 0x9a9f274a,
     0x30d23865, 0x61a470ca, 0xc348e194, 0x837db5d9, 0x03171d43, 0x062e3a86, 0x0c5c750c, 0x18b8ea18},
    {0x3da6d0cb, 0x7b4da196, 0xf69b432c, 0xe8daf0a9, 0xd45997a3, 0xad5f59b7, 0x5f52c59f, 0xbea58b3e,
     0x78a7608d, 0xf14ec11a, 0xe771f4c5, 0xcb0f9f7b, 0x93f34807, 0x220ae6ff, 0x4415cdfe, 0x882b9bfc,
     0x15bb4109, 0x2b768212, 0x56ed0424, 0xadda0848, 0x5e586661, 0xbcb0ccc2, 0x7c8def75, 0xf91bdeea,
     0xf7dbcb25, 0xea5be0bb, 0xd15bb787, 0xa75b19ff, 0x4b5a450f, 0x96b48a1e, 0x288562cd, 0x510ac59a},
    {0x740eef02, 0xe81dde04, 0xd5d7caf9, 0xae43e303, 0x596bb0f7, 0xb2d761ee, 0x6042b52d, 0xc0856a5a,
     0x84e6a245, 0x0c21327b, 0x184264f6, 0x3084c9ec, 0x610993d8, 0xc21327b0, 0x81ca3991, 0x067805d3,
     0x0cf00ba6, 0x19e0174c, 0x33c02e98, 0x67805d30, 0xcf00ba60, 0x9bed0231, 0x32367293, 0x646ce526,
     0xc8d9ca4c, 0x945fe269, 0x2d53b223, 0x5aa76446, 0xb54ec88c, 0x6f71e7e9, 0xdee3cfd2, 0xb82be955},
    {0x6992cea2, 0xd3259d44, 0xa3a74c79, 0x42a2ee03, 0x8545dc06, 0x0f67cefd, 0x1ecf9dfa, 0x3d9f3bf4,
     0x7b3e77e8, 0xf67cefd0, 0xe915a951, 0xd7c72453, 0xaa623e57, 0x51280a5f, 0xa25014be, 0x414c5f8d,
     0x8298bf1a, 0x00dd08c5, 0x01ba118a, 0x03742314, 0x06e84628, 0x0dd08c50, 0x1ba118a0, 0x37423140,
     0x6e846280, 0xdd08c500, 0xbffdfcf1, 0x7a178f13, 0xf42f1e26, 0xedb24abd, 0xde88e38b, 0xb8fdb1e7},
    {0xdcb17aa4, 0xbc8e83b9, 0x7cf17183, 0xf9e2e306, 0xf629b0fd, 0xe9bf170b, 0xd69258e7, 0xa8c8c73f,
     0x547df88f, 0xa8fbf11e, 0x541b94cd, 0xa837299a, 0x558225c5, 0xab044b8a, 0x53e4e1e5, 0xa7c9c3ca,
     0x4a7ff165, 0x94ffe2ca, 0x2c13b365, 0x582766ca, 0xb04ecd94, 0x6571edd9, 0xcae3dbb2, 0x902bc195,
     0x25bbf5db, 0x4b77ebb6, 0x96efd76c, 0x2833d829, 0x5067b052, 0xa0cf60a4, 0x4472b7b9, 0x88e56f72},
    {0xbd6f81f8, 0x7f337501, 0xfe66ea02, 0xf921a2f5, 0xf7af331b, 0xeab210c7, 0xd088577f, 0xa4fcd80f,
     0x4c15c6ef, 0x982b8dde, 0x35bb6d4d, 0x6b76da9a, 0xd6edb534, 0xa8371c99, 0x55824fc3, 0xab049f86,
     0x53e549fd, 0xa7ca93fa, 0x4a795105, 0x94f2a20a, 0x2c0932e5, 0x581265ca, 0xb024cb94, 0x65a5e1d9,
     0xcb4bc3b2, 0x937bf195, 0x231b95db, 0x46372bb6, 0x8c6e576c, 0x1d30d829, 0x3a61b052, 0x74c360a4},
    {0xfe314258, 0xf98ef241, 0xf6f19273, 0xe80f5217, 0xd5f2d2df, 0xae09d34f, 0x59ffd06f, 0xb3ffa0de,
     0x6213374d, 0xc4266e9a, 0x8da0abc5, 0x1ead217b, 0x3d5a42f6, 0x7ab485ec, 0xf5690bd8, 0xef3e6141,
     0xdb90b473, 0xb2cd1e17, 0x60764adf, 0xc0ec95be, 0x84355d8d, 0x0d86cdeb, 0x1b0d9bd6, 0x361b37ac,
     0x6c366f58, 0xd86cdeb0, 0xb535cb91, 0x6f87e1d3, 0xdf0fc3a6, 0xbbf3f1bd, 0x720b958b, 0xe4172b16},
    {0xf7506984, 0xeb4ca5f9, 0xd3753d03, 0xa3060cf7, 0x43e06f1f, 0x87c0de3e, 0x0a6dca8d, 0x14db951a,
     0x29b72a34, 0x536e5468, 0xa6dca8d0, 0x48552751, 0x90aa4ea2, 0x24b8ebb5, 0x4971d76a, 0x92e3aed4,
     0x202b2b59, 0x405656b2, 0x80acad64, 0x04b52c39, 0x096a5872, 0x12d4b0e4, 0x25a961c8, 0x4b52c390,
     0x96a58720, 0x28a778b1, 0x514ef162, 0xa29de2c4, 0x40d7b379, 0x81af66f2, 0x06b2bb15, 0x0d65762a},
    {0xc2a5b65e, 0x80a71a4d, 0x04a2426b, 0x094484d6, 0x128909ac, 0x25121358, 0x4a2426b0, 0x94484d60,
     0x2d7cec31, 0x5af9d862, 0xb5f3b0c4, 0x6e0b1779, 0xdc162ef2, 0xbdc02b15, 0x7e6c20db, 0xfcd841b6,
     0xfc5cf59d, 0xfd559dcb, 0xff474d67, 0xfb62ec3f, 0xf329ae8f, 0xe3bf2bef, 0xc292212f, 0x80c834af,
     0x047c1faf, 0x08f83f5e, 0x11f07ebc, 0x23e0fd78, 0x47c1faf0, 0x8f83f5e0, 0x1aeb9d31, 0x35d73a62},
    {0xe040e0ac, 0xc56db7a9, 0x8f3719a3, 0x1b8245b7, 0x37048b6e, 0x6e0916dc, 0xdc122db8, 0xbdc82d81,
     0x7e7c2df3, 0xfcf85be6, 0xfc1cc13d, 0xfdd5f48b, 0xfe479fe7, 0xf963493f, 0xf72ae48f, 0xebb9bfef,
     0xd29f092f, 0xa0d264af, 0x4448bfaf, 0x88917f5e, 0x14ce884d, 0x299d109a, 0x533a2134, 0xa6744268,
     0x4904f221, 0x9209e442, 0x21ffbe75, 0x43ff7cea, 0x87fef9d4, 0x0a118559, 0x14230ab2, 0x28461564},
    {0xc7cacead, 0x8a79ebab, 0x111fa1a7, 0x223f434e, 0x447e869c, 0x88fd0d38, 0x14166c81, 0x282cd902,
     0x5059b204, 0xa0b36408, 0x448abee1, 0x89157dc2, 0x17c68d75, 0x2f8d1aea, 0x5f1a35d4, 0xbe346ba8,
     0x7984a1a1, 0xf3094342, 0xe3fef075, 0xc211961b, 0x81cf5ac7, 0x0672c37f, 0x0ce586fe, 0x19cb0dfc,
     0x33961bf8, 0x672c37f0, 0xce586fe0, 0x995ca931, 0x37552493, 0x6eaa4926, 0xdd54924c, 0xbf455269},
    {0x04fcdcbf, 0x09f9b97e, 0x13f372fc, 0x27e6e5f8, 0x4fcdcbf0, 0x9f9b97e0, 0x3adb5931, 0x75b6b262,
     0xeb6d64c4, 0xd336bf79, 0xa3810803, 0x42ee66f7, 0x85dccdee, 0x0e55ed2d, 0x1cabda5a, 0x3957b4b4,
     0x72af6968, 0xe55ed2d0, 0xcf51d351, 0x9b4fd053, 0x3373d657, 0x66e7acae, 0xcdcf595c, 0x9e72c449,
     0x3909fe63, 0x7213fcc6, 0xe427f98c, 0xcda385e9, 0x9eab7d23, 0x38ba8cb7, 0x7175196e, 0xe2ea32dc},
    {0x6bafcc21, 0xd75f9842, 0xab534675, 0x534afa1b, 0xa695f436, 0x48c79e9d, 0x918f3d3a, 0x26f20c85,
     0x4de4190a, 0x9bc83214, 0x327c12d9, 0x64f825b2, 0xc9f04b64, 0x960ce039, 0x29f5b683, 0x53eb6d06,
     0xa7d6da0c, 0x4a41c2e9, 0x948385d2, 0x2ceb7d55, 0x59d6faaa, 0xb3adf554, 0x62b79c59, 0xc56f38b2,
     0x8f320795, 0x1b8879db, 0x3710f3b6, 0x6e21e76c, 0xdc43ced8, 0xbd6beb41, 0x7f3ba073, 0xfe7740e6},
    {0x140441c6, 0x2808838c, 0x50110718, 0xa0220e30, 0x45a86a91, 0x8b50d522, 0x134ddcb5, 0x269bb96a,
     0x4d3772d4, 0x9a6ee5a8, 0x3131bda1, 0x62637b42, 0xc4c6f684, 0x8c619bf9, 0x1d2f4103, 0x3a5e8206,
     0x74bd040c, 0xe97a0818, 0xd71866c1, 0xabdcbb73, 0x52550017, 0xa4aa002e, 0x4cb876ad, 0x9970ed5a,
     0x370dac45, 0x6e1b588a, 0xdc36b114, 0xbd8114d9, 0x7eee5f43, 0xfddcbe86, 0xfe550bfd, 0xf946610b},
    {0x68175a0a, 0xd02eb414, 0xa5b11ed9, 0x4e8e4b43, 0x9d1c9686, 0x3fd55bfd, 0x7faab7fa, 0xff556ff4,
     0xfb46a919, 0xf36124c3, 0xe32e3f77, 0xc3b0081f, 0x828c66cf, 0x00f4bb6f, 0x01e976de, 0x03d2edbc,
     0x07a5db78, 0x0f4bb6f0, 0x1e976de0, 0x3d2edbc0, 0x7a5db780, 0xf4bb6f00, 0xec9aa8f1, 0xdcd92713,
     0xbc5e38d7, 0x7d50075f, 0xfaa00ebe, 0xf0ac6b8d, 0xe4b4a1eb, 0xcc853527, 0x9ce61cbf, 0x3c204f8f},
    {0xe1ff3667, 0xc6121a3f, 0x89c8428f, 0x167cf3ef, 0x2cf9e7de, 0x59f3cfbc, 0xb3e79f78, 0x62234801,
     0xc4469002, 0x8d6156f5, 0x1f2edb1b, 0x3e5db636, 0x7cbb6c6c, 0xf976d8d8, 0xf701c741, 0xebeff873,
     0xd2338617, 0xa18b7adf, 0x46fa834f, 0x8df5069e, 0x1e067bcd, 0x3c0cf79a, 0x7819ef34, 0xf033de68,
     0xe58bca21, 0xcefbe2b3, 0x981bb397, 0x35db11df, 0x6bb623be, 0xd76c477c, 0xab34f809, 0x538586e3},
    {0x8b7230ec, 0x13081729, 0x26102e52, 0x4c205ca4, 0x9840b948, 0x356d0461, 0x6ada08c2, 0xd5b41184,
     0xae8455f9, 0x58e4dd03, 0xb1c9ba06, 0x667f02fd, 0xccfe05fa, 0x9c107d05, 0x3dcc8cfb, 0x7b9919f6,
     0xf73233ec, 0xeb881129, 0xd2fc54a3, 0xa014dfb7, 0x45c5c99f, 0x8b8b933e, 0x12fb508d, 0x25f6a11a,
     0x4bed4234, 0x97da8468, 0x2a597e21, 0x54b2fc42, 0xa965f884, 0x572787f9, 0xae4f0ff2, 0x59726915},
    {0x56175f20, 0xac2ebe40, 0x5db10a71, 0xbb6214e2, 0x73285f35, 0xe650be6a, 0xc94d0a25, 0x977662bb,
     0x2b00b387, 0x5601670e, 0xac02ce1c, 0x5de9eac9, 0xbbd3d592, 0x724bddd5, 0xe497bbaa, 0xccc301a5,
     0x9c6a75bb, 0x3d389d87, 0x7a713b0e, 0xf4e2761c, 0xec289ac9, 0xddbd4363, 0xbe96f037, 0x78c1969f,
     0xf1832d3e, 0xe6ea2c8d, 0xc8382feb, 0x959c2927, 0x2ed424bf, 0x5da8497e, 0xbb5092fc, 0x734d5309},
    {0xb9a3dcd0, 0x76abcf51, 0xed579ea2, 0xdf434bb5, 0xbb6ae19b, 0x7339b5c7, 0xe6736b8e, 0xc90aa1ed,
     0x97f9352b, 0x2a1e1ca7, 0x543c394e, 0xa878729c, 0x551c93c9, 0xaa392792, 0x519e39d5, 0xa33c73aa,
     0x439491a5, 0x8729234a, 0x0bbe3065, 0x177c60ca, 0x2ef8c194, 0x5df18328, 0xbbe30650, 0x722a7a51,
     0xe454f4a2, 0xcd459fb5, 0x9f67499b, 0x3b22e5c7, 0x7645cb8e, 0xec8b971c, 0xdcfb58c9, 0xbc1ac763},
    {0xdd2d789e, 0xbfb687cd, 0x7a81796b, 0xf502f2d6, 0xefe9935d, 0xda3f504b, 0xb192d667, 0x66c9da3f,
     0xcd93b47e, 0x9ecb1e0d, 0x387a4aeb, 0x70f495d6, 0xe1e92bac, 0xc63e21a9, 0x899035a3, 0x16cc1db7,
     0x2d983b6e, 0x5b3076dc, 0xb660edb8, 0x692dad81, 0xd25b5b02, 0xa15ac0f5, 0x4759f71b, 0x8eb3ee36,
     0x188baa9d, 0x3117553a, 0x622eaa74, 0xc45d54e8, 0x8d56df21, 0x1f41c8b3, 0x3e839166, 0x7d0722cc},
    {0x44036c4a, 0x8806d894, 0x15e1c7d9, 0x2bc38fb2, 0x57871f64, 0xaf0e3ec8, 0x5bf00b61, 0xb7e016c2,
     0x6a2c5b75, 0xd458b6ea, 0xad5d1b25, 0x5f5640bb, 0xbeac8176, 0x78b5741d, 0xf16ae83a, 0xe739a685,
     0xcb9f3bfb, 0x92d20107, 0x204874ff, 0x4090e9fe, 0x8121d3fc, 0x07afd109, 0x0f5fa212, 0x1ebf4424,
     0x3d7e8848, 0x7afd1090, 0xf5fa2120, 0xee1834b1, 0xd9dc1f93, 0xb65449d7, 0x6944e55f, 0xd289cabe},
    {0x4612657d, 0x8c24cafa, 0x1da5e305, 0x3b4bc60a, 0x76978c14, 0xed2f1828, 0xdfb246a1, 0xba88fbb3,
     0x70fd8197, 0xe1fb032e, 0xc61a70ad, 0x89d897ab, 0x165d59a7, 0x2cbab34e, 0x5975669c, 0xb2eacd38,
     0x6039ec81, 0xc073d902, 0x850bc4f5, 0x0ffbff1b, 0x1ff7fe36, 0x3feffc6c, 0x7fdff8d8, 0xffbff1b0,
     0xfa939591, 0xf0cb5dd3, 0xe47acd57, 0xcd19ec5f, 0x9fdfae4f, 0x3a532a6f, 0x74a654de, 0xe94ca9bc},
    {0x584d5569, 0xb09aaad2, 0x64d92355, 0xc9b246aa, 0x9688fba5, 0x28fd81bb, 0x51fb0376, 0xa3f606ec,
     0x42007b29, 0x8400f652, 0x0ded9a55, 0x1bdb34aa, 0x37b66954, 0x6f6cd2a8, 0xded9a550, 0xb85f3c51,
     0x75520e53, 0xeaa41ca6, 0xd0a44fbd, 0xa4a4e98b, 0x4ca5a5e7, 0x994b4bce, 0x377ae16d, 0x6ef5c2da,
     0xddeb85b4, 0xbe3b7d99, 0x799a8dc3, 0xf3351b86, 0xe38641fd, 0xc2e0f50b, 0x802d9ce7, 0x05b74f3f},
    {0xe8cd33e2, 0xd4761135, 0xad00549b, 0x5fecdfc7, 0xbfd9bf8e, 0x7a5f09ed, 0xf4be13da, 0xec905145,
     0xdcccd47b, 0xbc75de07, 0x7d07caff, 0xfa0f95fe, 0xf1f35d0d, 0xe60acceb, 0xc9f9ef27, 0x961fa8bf,
     0x29d3278f, 0x53a64f1e, 0xa74c9e3c, 0x4b754a89, 0x96ea9512, 0x28395cd5, 0x5072b9aa, 0xa0e57354,
     0x44269059, 0x884d20b2, 0x15763795, 0x2aec6f2a, 0x55d8de54, 0xabb1bca8, 0x528f0fa1, 0xa51e1f42},
    {0x82f63b78, 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040,
     0x00000080, 0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000,
     0x00008000, 0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000,
     0x00800000, 0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000},
    {0x417b1dbc, 0x82f63b78, 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020,
     0x00000040, 0x00000080, 0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000,
     0x00004000, 0x00008000, 0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000,
     0x00400000, 0x00800000, 0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000},
    {0x105ec76f, 0x20bd8ede, 0x417b1dbc, 0x82f63b78, 0x00000001, 0x00000002, 0x00000004, 0x00000008,
     0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x00000100, 0x00000200, 0x00000400, 0x00000800,
     0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00010000, 0x00020000, 0x00040000, 0x00080000,
     0x00100000, 0x00200000, 0x00400000, 0x00800000, 0x01000000, 0x02000000, 0x04000000, 0x08000000},
    {0xf26b8303, 0xe13b70f7, 0xc79a971f, 0x8ad958cf, 0x105ec76f, 0x20bd8ede, 0x417b1dbc, 0x82f63b78,
     0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
     0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
     0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000}
};

uint32_t stone_crc32c_zeros(uint32_t crc, unsigned len)
{
  int range = 0;
  unsigned remainder = len & 15;
  len = len >> 4;
  range = 4;
  while (len != 0) {
    if ((len & 1) == 1) {
      uint32_t crc1 = 0;
      uint32_t* ptr = crc_turbo_table/*.val*/[range];
      while (crc != 0) {
        uint32_t mask = ~((crc & 1) - 1);
        crc1 = crc1 ^ (mask & *ptr);
        crc = crc >> 1;
        ptr++;
      }
      crc = crc1;
    }
    len = len >> 1;
    range++;
  }
  if (remainder > 0)
    crc = stone_crc32c(crc, nullptr, remainder);
  return crc;
}
