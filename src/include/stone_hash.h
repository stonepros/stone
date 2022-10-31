#ifndef FS_STONE_HASH_H
#define FS_STONE_HASH_H

#define STONE_STR_HASH_LINUX      0x1  /* linux dcache hash */
#define STONE_STR_HASH_RJENKINS   0x2  /* robert jenkins' */

extern unsigned stone_str_hash_linux(const char *s, unsigned len);
extern unsigned stone_str_hash_rjenkins(const char *s, unsigned len);

extern unsigned stone_str_hash(int type, const char *s, unsigned len);
extern const char *stone_str_hash_name(int type);
extern bool stone_str_hash_valid(int type);

#endif
