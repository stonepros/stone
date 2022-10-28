/*
 * stone_fs.cc - Some Stone functions that are shared between kernel space and
 * user space.
 *
 */

/*
 * Some non-inline stone helpers
 */
#include "include/types.h"

int stone_flags_to_mode(int flags)
{
	/* because STONE_FILE_MODE_PIN is zero, so mode = -1 is error */
	int mode = -1;

	if ((flags & STONE_O_DIRECTORY) == STONE_O_DIRECTORY)
		return STONE_FILE_MODE_PIN;

	switch (flags & O_ACCMODE) {
	case STONE_O_WRONLY:
		mode = STONE_FILE_MODE_WR;
		break;
	case STONE_O_RDONLY:
		mode = STONE_FILE_MODE_RD;
		break;
	case STONE_O_RDWR:
	case O_ACCMODE: /* this is what the VFS does */
		mode = STONE_FILE_MODE_RDWR;
		break;
	}

	if (flags & STONE_O_LAZY)
		mode |= STONE_FILE_MODE_LAZY;

	return mode;
}

int stone_caps_for_mode(int mode)
{
	int caps = STONE_CAP_PIN;

	if (mode & STONE_FILE_MODE_RD)
		caps |= STONE_CAP_FILE_SHARED |
			STONE_CAP_FILE_RD | STONE_CAP_FILE_CACHE;
	if (mode & STONE_FILE_MODE_WR)
		caps |= STONE_CAP_FILE_EXCL |
			STONE_CAP_FILE_WR | STONE_CAP_FILE_BUFFER |
			STONE_CAP_AUTH_SHARED | STONE_CAP_AUTH_EXCL |
			STONE_CAP_XATTR_SHARED | STONE_CAP_XATTR_EXCL;
	if (mode & STONE_FILE_MODE_LAZY)
		caps |= STONE_CAP_FILE_LAZYIO;

	return caps;
}

int stone_flags_sys2wire(int flags)
{
       int wire_flags = 0;

       switch (flags & O_ACCMODE) {
       case O_RDONLY:
               wire_flags |= STONE_O_RDONLY;
               break;
       case O_WRONLY:
               wire_flags |= STONE_O_WRONLY;
               break;
       case O_RDWR:
               wire_flags |= STONE_O_RDWR;
               break;
       }
       flags &= ~O_ACCMODE;

#define stone_sys2wire(a) if (flags & a) { wire_flags |= STONE_##a; flags &= ~a; }

       stone_sys2wire(O_CREAT);
       stone_sys2wire(O_EXCL);
       stone_sys2wire(O_TRUNC);

       #ifndef _WIN32
       stone_sys2wire(O_DIRECTORY);
       stone_sys2wire(O_NOFOLLOW);
       // In some cases, FILE_FLAG_BACKUP_SEMANTICS may be used instead
       // of O_DIRECTORY. We may need some workarounds in order to handle
       // the fact that those flags are not available on Windows.
       #endif

#undef stone_sys2wire

       return wire_flags;
}
