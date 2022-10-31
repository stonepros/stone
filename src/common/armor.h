#ifndef STONE_ARMOR_H
#define STONE_ARMOR_H

#ifdef __cplusplus
extern "C" {
#endif

int stone_armor(char *dst, const char *dst_end,
	       const char *src, const char *end);

int stone_armor_linebreak(char *dst, const char *dst_end,
	       const char *src, const char *end,
	       int line_width);
int stone_unarmor(char *dst, const char *dst_end,
		 const char *src, const char *end);
#ifdef __cplusplus
}
#endif

#endif
