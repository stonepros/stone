
#include "stone_ver.h"

#define CONCAT_VER_SYMBOL(x) stone_ver__##x

#define DEFINE_VER_SYMBOL(x) int CONCAT_VER_SYMBOL(x)

DEFINE_VER_SYMBOL(STONE_GIT_VER);



