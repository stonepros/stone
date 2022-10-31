#ifndef STONE_CRUSH_TYPES_H
#define STONE_CRUSH_TYPES_H

#ifdef KERNEL
# define free(x) kfree(x)
#else
# include <stdlib.h>
#endif


#include <linux/types.h>  /* just for int types */

#ifndef BUG_ON
# define BUG_ON(x) stone_assert(!(x))
#endif

#endif
