/*
 * Stone 'frag' type
 */
#include "include/types.h"

int stone_frag_compare(__u32 a, __u32 b)
{
	unsigned va = stone_frag_value(a);
	unsigned vb = stone_frag_value(b);
	if (va < vb)
		return -1;
	if (va > vb)
		return 1;
	va = stone_frag_bits(a);
	vb = stone_frag_bits(b);
	if (va < vb)
		return -1;
	if (va > vb)
		return 1;
	return 0;
}
