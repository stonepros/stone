#include <string>
#include <unistd.h>
#include <include/fs_types.h>
#include <mds/mdstypes.h>
#include <include/stonefs/libstonefs.h>

#define MAX_STONE_FILES	1000
#define DIRNAME		"ino_release_cb"

static std::atomic<bool> cb_done = false;

static void cb(void *hdl, vinodeno_t vino)
{
	cb_done = true;
}

int main(int argc, char *argv[])
{
	inodeno_t inos[MAX_STONE_FILES];
	struct stone_mount_info *cmount = NULL;

	stone_create(&cmount, "admin");
	stone_conf_read_file(cmount, NULL);
	stone_init(cmount);

	[[maybe_unused]] int ret = stone_mount(cmount, NULL);
	assert(ret >= 0);
	ret = stone_mkdir(cmount, DIRNAME, 0755);
	assert(ret >= 0);
	ret = stone_chdir(cmount, DIRNAME);
	assert(ret >= 0);

	/* Create a bunch of files, get their inode numbers and close them */
	int i;
	for (i = 0; i < MAX_STONE_FILES; ++i) {
		int fd;
		struct stone_statx stx;

		string name = std::to_string(i);

		fd = stone_open(cmount, name.c_str(), O_RDWR|O_CREAT, 0644);
		assert(fd >= 0);

		ret = stone_fstatx(cmount, fd, &stx, STONE_STATX_INO, 0);
		assert(ret >= 0);

		inos[i] = stx.stx_ino;
		stone_close(cmount, fd);
	}

	/* Remount */
	stone_unmount(cmount);
	stone_release(cmount);
	stone_create(&cmount, "admin");
	stone_conf_read_file(cmount, NULL);
	stone_init(cmount);

	struct stone_client_callback_args args = { 0 };
	args.ino_release_cb = cb;
	stone_ll_register_callbacks(cmount, &args);

	ret = stone_mount(cmount, NULL);
	assert(ret >= 0);

	Inode	*inodes[MAX_STONE_FILES];

	for (i = 0; i < MAX_STONE_FILES; ++i) {
		/* We can stop if we got a callback */
		if (cb_done)
			break;

		ret = stone_ll_lookup_inode(cmount, inos[i], &inodes[i]);
		assert(ret >= 0);
	}
    sleep(45);

	assert(cb_done);
	stone_unmount(cmount);
	stone_release(cmount);
	return 0;
}
