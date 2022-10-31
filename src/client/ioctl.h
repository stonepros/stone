#ifndef FS_STONE_IOCTL_H
#define FS_STONE_IOCTL_H

#include "include/int_types.h"

#if defined(__linux__)
#include <linux/ioctl.h>
#include <linux/types.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

#define STONE_IOCTL_MAGIC 0x97

/* just use u64 to align sanely on all archs */
struct stone_ioctl_layout {
	__u64 stripe_unit, stripe_count, object_size;
	__u64 data_pool;
	__s64 unused;
};

#define STONE_IOC_GET_LAYOUT _IOR(STONE_IOCTL_MAGIC, 1,		\
				   struct stone_ioctl_layout)
#define STONE_IOC_SET_LAYOUT _IOW(STONE_IOCTL_MAGIC, 2,		\
				   struct stone_ioctl_layout)
#define STONE_IOC_SET_LAYOUT_POLICY _IOW(STONE_IOCTL_MAGIC, 5,	\
				   struct stone_ioctl_layout)

/*
 * Extract identity, address of the OSD and object storing a given
 * file offset.
 */
struct stone_ioctl_dataloc {
	__u64 file_offset;           /* in+out: file offset */
	__u64 object_offset;         /* out: offset in object */
	__u64 object_no;             /* out: object # */
	__u64 object_size;           /* out: object size */
	char object_name[64];        /* out: object name */
	__u64 block_offset;          /* out: offset in block */
	__u64 block_size;            /* out: block length */
	__s64 osd;                   /* out: osd # */
	struct sockaddr_storage osd_addr; /* out: osd address */
};

#define STONE_IOC_GET_DATALOC _IOWR(STONE_IOCTL_MAGIC, 3,	\
				   struct stone_ioctl_dataloc)

#define STONE_IOC_LAZYIO _IO(STONE_IOCTL_MAGIC, 4)

#endif
