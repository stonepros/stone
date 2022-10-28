/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <jni.h>

#include "ScopedLocalRef.h"
#include "JniConstants.h"

#include "include/stonefs/libstonefs.h"
#include "common/dout.h"

#define dout_subsys stone_subsys_javaclient

#include "com_stone_fs_StoneMount.h"

#define STONE_STAT_CP "com/stone/fs/StoneStat"
#define STONE_STAT_VFS_CP "com/stone/fs/StoneStatVFS"
#define STONE_FILE_EXTENT_CP "com/stone/fs/StoneFileExtent"
#define STONE_MOUNT_CP "com/stone/fs/StoneMount"
#define STONE_NOTMOUNTED_CP "com/stone/fs/StoneNotMountedException"
#define STONE_FILEEXISTS_CP "com/stone/fs/StoneFileAlreadyExistsException"
#define STONE_ALREADYMOUNTED_CP "com/stone/fs/StoneAlreadyMountedException"
#define STONE_NOTDIR_CP "com/stone/fs/StoneNotDirectoryException"

/*
 * Flags to open(). must be synchronized with StoneMount.java
 *
 * There are two versions of flags: the version in Java and the version in the
 * target library (e.g. libc or libstonefs). We control the Java values and map
 * to the target value with fixup_* functions below. This is much faster than
 * keeping the values in Java and making a cross-JNI up-call to retrieve them,
 * and makes it easy to keep any platform specific value changes in this file.
 */
#define JAVA_O_RDONLY    1
#define JAVA_O_RDWR      2
#define JAVA_O_APPEND    4
#define JAVA_O_CREAT     8
#define JAVA_O_TRUNC     16
#define JAVA_O_EXCL      32
#define JAVA_O_WRONLY    64
#define JAVA_O_DIRECTORY 128

/*
 * Whence flags for seek(). sync with StoneMount.java if changed.
 *
 * Mapping of SEEK_* done in seek function.
 */
#define JAVA_SEEK_SET 1
#define JAVA_SEEK_CUR 2
#define JAVA_SEEK_END 3

/*
 * File attribute flags. sync with StoneMount.java if changed.
 */
#define JAVA_SETATTR_MODE  1
#define JAVA_SETATTR_UID   2
#define JAVA_SETATTR_GID   4
#define JAVA_SETATTR_MTIME 8
#define JAVA_SETATTR_ATIME 16

/*
 * Setxattr flags. sync with StoneMount.java if changed.
 */
#define JAVA_XATTR_CREATE   1
#define JAVA_XATTR_REPLACE  2
#define JAVA_XATTR_NONE     3

/*
 * flock flags. sync with StoneMount.java if changed.
 */
#define JAVA_LOCK_SH 1
#define JAVA_LOCK_EX 2
#define JAVA_LOCK_NB 4
#define JAVA_LOCK_UN 8

/* Map JAVA_O_* open flags to values in libc */
static inline int fixup_open_flags(jint jflags)
{
	int ret = 0;

#define FIXUP_OPEN_FLAG(name) \
	if (jflags & JAVA_##name) \
		ret |= name;

	FIXUP_OPEN_FLAG(O_RDONLY)
	FIXUP_OPEN_FLAG(O_RDWR)
	FIXUP_OPEN_FLAG(O_APPEND)
	FIXUP_OPEN_FLAG(O_CREAT)
	FIXUP_OPEN_FLAG(O_TRUNC)
	FIXUP_OPEN_FLAG(O_EXCL)
	FIXUP_OPEN_FLAG(O_WRONLY)
	FIXUP_OPEN_FLAG(O_DIRECTORY)

#undef FIXUP_OPEN_FLAG

	return ret;
}

/* Map JAVA_SETATTR_* to values in stone lib */
static inline int fixup_attr_mask(jint jmask)
{
	int mask = 0;

#define FIXUP_ATTR_MASK(name) \
	if (jmask & JAVA_##name) \
		mask |= STONE_##name;

	FIXUP_ATTR_MASK(SETATTR_MODE)
	FIXUP_ATTR_MASK(SETATTR_UID)
	FIXUP_ATTR_MASK(SETATTR_GID)
	FIXUP_ATTR_MASK(SETATTR_MTIME)
	FIXUP_ATTR_MASK(SETATTR_ATIME)

#undef FIXUP_ATTR_MASK

	return mask;
}

/* Cached field IDs for com.stone.fs.StoneStat */
static jfieldID stonestat_mode_fid;
static jfieldID stonestat_uid_fid;
static jfieldID stonestat_gid_fid;
static jfieldID stonestat_size_fid;
static jfieldID stonestat_blksize_fid;
static jfieldID stonestat_blocks_fid;
static jfieldID stonestat_a_time_fid;
static jfieldID stonestat_m_time_fid;
static jfieldID stonestat_is_file_fid;
static jfieldID stonestat_is_directory_fid;
static jfieldID stonestat_is_symlink_fid;

/* Cached field IDs for com.stone.fs.StoneStatVFS */
static jfieldID stonestatvfs_bsize_fid;
static jfieldID stonestatvfs_frsize_fid;
static jfieldID stonestatvfs_blocks_fid;
static jfieldID stonestatvfs_bavail_fid;
static jfieldID stonestatvfs_files_fid;
static jfieldID stonestatvfs_fsid_fid;
static jfieldID stonestatvfs_namemax_fid;

/* Cached field IDs for com.stone.fs.StoneMount */
static jfieldID stonemount_instance_ptr_fid;

/* Cached field IDs for com.stone.fs.StoneFileExtent */
static jclass stonefileextent_cls;
static jmethodID stonefileextent_ctor_fid;

/*
 * Exception throwing helper. Adapted from Apache Hadoop header
 * org_apache_hadoop.h by adding the do {} while (0) construct.
 */
#define THROW(env, exception_name, message) \
	do { \
		jclass ecls = env->FindClass(exception_name); \
		if (ecls) { \
			int ret = env->ThrowNew(ecls, message); \
			if (ret < 0) { \
				printf("(StoneFS) Fatal Error\n"); \
			} \
			env->DeleteLocalRef(ecls); \
		} \
	} while (0)


static void stoneThrowNullArg(JNIEnv *env, const char *msg)
{
	THROW(env, "java/lang/NullPointerException", msg);
}

static void stoneThrowOutOfMemory(JNIEnv *env, const char *msg)
{
	THROW(env, "java/lang/OutOfMemoryError", msg);
}

static void stoneThrowInternal(JNIEnv *env, const char *msg)
{
	THROW(env, "java/lang/InternalError", msg);
}

static void stoneThrowIndexBounds(JNIEnv *env, const char *msg)
{
	THROW(env, "java/lang/IndexOutOfBoundsException", msg);
}

static void stoneThrowIllegalArg(JNIEnv *env, const char *msg)
{
	THROW(env, "java/lang/IllegalArgumentException", msg);
}

static void stoneThrowFNF(JNIEnv *env, const char *msg)
{
	THROW(env, "java/io/FileNotFoundException", msg);
}

static void stoneThrowFileExists(JNIEnv *env, const char *msg)
{
	THROW(env, STONE_FILEEXISTS_CP, msg);
}

static void stoneThrowNotDir(JNIEnv *env, const char *msg)
{
	THROW(env, STONE_NOTDIR_CP, msg);
}

static void handle_error(JNIEnv *env, int rc)
{
	switch (rc) {
		case -ENOENT:
			stoneThrowFNF(env, "");
			return;
		case -EEXIST:
			stoneThrowFileExists(env, "");
			return;
		case -ENOTDIR:
			stoneThrowNotDir(env, "");
			return;
		default:
			break;
	}

	THROW(env, "java/io/IOException", strerror(-rc));
}

#define CHECK_ARG_NULL(v, m, r) do { \
	if (!(v)) { \
		stoneThrowNullArg(env, (m)); \
		return (r); \
	} } while (0)

#define CHECK_ARG_BOUNDS(c, m, r) do { \
	if ((c)) { \
		stoneThrowIndexBounds(env, (m)); \
		return (r); \
	} } while (0)

#define CHECK_MOUNTED(_c, _r) do { \
	if (!stone_is_mounted((_c))) { \
		THROW(env, STONE_NOTMOUNTED_CP, "not mounted"); \
		return (_r); \
	} } while (0)

/*
 * Cast a jlong to stone_mount_info. Each JNI function is expected to pass in
 * the class instance variable instance_ptr. Passing a parameter is faster
 * than reaching back into Java via an upcall to retrieve this pointer.
 */
static inline struct stone_mount_info *get_stone_mount(jlong j_mntp)
{
	return (struct stone_mount_info *)j_mntp;
}

/*
 * Setup cached field IDs
 */
static void setup_field_ids(JNIEnv *env, jclass clz)
{
	jclass stonestat_cls;
	jclass stonestatvfs_cls;
	jclass tmp_stonefileextent_cls;

/*
 * Get a fieldID from a class with a specific type
 *
 * clz: jclass
 * field: field in clz
 * type: integer, long, etc..
 *
 * This macro assumes some naming convention that is used
 * only in this file:
 *
 *   GETFID(stonestat, mode, I) gets translated into
 *     stonestat_mode_fid = env->GetFieldID(stonestat_cls, "mode", "I");
 */
#define GETFID(clz, field, type) do { \
	clz ## _ ## field ## _fid = env->GetFieldID(clz ## _cls, #field, #type); \
	if ( ! clz ## _ ## field ## _fid ) \
		return; \
	} while (0)

	/* Cache StoneStat fields */

	stonestat_cls = env->FindClass(STONE_STAT_CP);
	if (!stonestat_cls)
		return;

	GETFID(stonestat, mode, I);
	GETFID(stonestat, uid, I);
	GETFID(stonestat, gid, I);
	GETFID(stonestat, size, J);
	GETFID(stonestat, blksize, J);
	GETFID(stonestat, blocks, J);
	GETFID(stonestat, a_time, J);
	GETFID(stonestat, m_time, J);
	GETFID(stonestat, is_file, Z);
	GETFID(stonestat, is_directory, Z);
	GETFID(stonestat, is_symlink, Z);

	/* Cache StoneStatVFS fields */

	stonestatvfs_cls = env->FindClass(STONE_STAT_VFS_CP);
	if (!stonestatvfs_cls)
		return;

	GETFID(stonestatvfs, bsize, J);
	GETFID(stonestatvfs, frsize, J);
	GETFID(stonestatvfs, blocks, J);
	GETFID(stonestatvfs, bavail, J);
	GETFID(stonestatvfs, files, J);
	GETFID(stonestatvfs, fsid, J);
	GETFID(stonestatvfs, namemax, J);

	/* Cache StoneFileExtent fields */

	tmp_stonefileextent_cls = env->FindClass(STONE_FILE_EXTENT_CP);
	if (!tmp_stonefileextent_cls)
		return;

	stonefileextent_cls = (jclass)env->NewGlobalRef(tmp_stonefileextent_cls);
	env->DeleteLocalRef(tmp_stonefileextent_cls);

	stonefileextent_ctor_fid = env->GetMethodID(stonefileextent_cls, "<init>", "(JJ[I)V");
	if (!stonefileextent_ctor_fid)
		return;

  JniConstants::init(env);

#undef GETFID

	stonemount_instance_ptr_fid = env->GetFieldID(clz, "instance_ptr", "J");
}


/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_stone_fs_StoneMount_native_1initialize
	(JNIEnv *env, jclass clz)
{
	setup_field_ids(env, clz);
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_create
 * Signature: (Lcom/stone/fs/StoneMount;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1create
	(JNIEnv *env, jclass clz, jobject j_stonemount, jstring j_id)
{
	struct stone_mount_info *cmount;
	const char *c_id = NULL;
	int ret;

	CHECK_ARG_NULL(j_stonemount, "@mount is null", -1);

	if (j_id) {
		c_id = env->GetStringUTFChars(j_id, NULL);
		if (!c_id) {
			stoneThrowInternal(env, "Failed to pin memory");
			return -1;
		}
	}

	ret = stone_create(&cmount, c_id);

	if (c_id)
		env->ReleaseStringUTFChars(j_id, c_id);

	if (ret) {
		THROW(env, "java/lang/RuntimeException", "failed to create Stone mount object");
		return ret;
	}

	env->SetLongField(j_stonemount, stonemount_instance_ptr_fid, (long)cmount);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_mount
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1mount
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_root)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_root = NULL;
	int ret;

	/*
	 * Toss a message up if we are already mounted.
	 */
	if (stone_is_mounted(cmount)) {
		THROW(env, STONE_ALREADYMOUNTED_CP, "");
		return -1;
	}

	if (j_root) {
		c_root = env->GetStringUTFChars(j_root, NULL);
		if (!c_root) {
			stoneThrowInternal(env, "Failed to pin memory");
			return -1;
		}
	}

	ldout(cct, 10) << "jni: stone_mount: " << (c_root ? c_root : "<NULL>") << dendl;

	ret = stone_mount(cmount, c_root);

	ldout(cct, 10) << "jni: stone_mount: exit ret " << ret << dendl;

	if (c_root)
		env->ReleaseStringUTFChars(j_root, c_root);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_unmount
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1unmount
  (JNIEnv *env, jclass clz, jlong j_mntp)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	ldout(cct, 10) << "jni: stone_unmount enter" << dendl;

	CHECK_MOUNTED(cmount, -1);

	ret = stone_unmount(cmount);

	ldout(cct, 10) << "jni: stone_unmount exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_release
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1release
  (JNIEnv *env, jclass clz, jlong j_mntp)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	ldout(cct, 10) << "jni: stone_release called" << dendl;

	ret = stone_release(cmount);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_conf_set
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1conf_1set
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_opt, jstring j_val)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_opt, *c_val;
	int ret;

	CHECK_ARG_NULL(j_opt, "@option is null", -1);
	CHECK_ARG_NULL(j_val, "@value is null", -1);

	c_opt = env->GetStringUTFChars(j_opt, NULL);
	if (!c_opt) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	c_val = env->GetStringUTFChars(j_val, NULL);
	if (!c_val) {
		env->ReleaseStringUTFChars(j_opt, c_opt);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: conf_set: opt " << c_opt << " val " << c_val << dendl;

	ret = stone_conf_set(cmount, c_opt, c_val);

	ldout(cct, 10) << "jni: conf_set: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_opt, c_opt);
	env->ReleaseStringUTFChars(j_val, c_val);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_conf_get
 * Signature: (JLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_stone_fs_StoneMount_native_1stone_1conf_1get
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_opt)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_opt;
	jstring value = NULL;
	int ret, buflen;
	char *buf;

	CHECK_ARG_NULL(j_opt, "@option is null", NULL);

	c_opt = env->GetStringUTFChars(j_opt, NULL);
	if (!c_opt) {
		stoneThrowInternal(env, "failed to pin memory");
		return NULL;
	}

	buflen = 128;
	buf = new (std::nothrow) char[buflen];
	if (!buf) {
		stoneThrowOutOfMemory(env, "head allocation failed");
		goto out;
	}

	while (1) {
		memset(buf, 0, sizeof(char)*buflen);
		ldout(cct, 10) << "jni: conf_get: opt " << c_opt << " len " << buflen << dendl;
		ret = stone_conf_get(cmount, c_opt, buf, buflen);
		if (ret == -ENAMETOOLONG) {
			buflen *= 2;
			delete [] buf;
			buf = new (std::nothrow) char[buflen];
			if (!buf) {
				stoneThrowOutOfMemory(env, "head allocation failed");
				goto out;
			}
		} else
			break;
	}

	ldout(cct, 10) << "jni: conf_get: ret " << ret << dendl;

	if (ret == 0)
		value = env->NewStringUTF(buf);
	else if (ret != -ENOENT)
		handle_error(env, ret);

	delete [] buf;

out:
	env->ReleaseStringUTFChars(j_opt, c_opt);
	return value;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_conf_read_file
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1conf_1read_1file
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: conf_read_file: path " << c_path << dendl;

	ret = stone_conf_read_file(cmount, c_path);

	ldout(cct, 10) << "jni: conf_read_file: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_statfs
 * Signature: (JLjava/lang/String;Lcom/stone/fs/StoneStatVFS;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1statfs
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jobject j_stonestatvfs)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	struct statvfs st;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_stonestatvfs, "@stat is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: statfs: path " << c_path << dendl;

	ret = stone_statfs(cmount, c_path, &st);

	ldout(cct, 10) << "jni: statfs: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret) {
		handle_error(env, ret);
		return ret;
	}

	env->SetLongField(j_stonestatvfs, stonestatvfs_bsize_fid, st.f_bsize);
	env->SetLongField(j_stonestatvfs, stonestatvfs_frsize_fid, st.f_frsize);
	env->SetLongField(j_stonestatvfs, stonestatvfs_blocks_fid, st.f_blocks);
	env->SetLongField(j_stonestatvfs, stonestatvfs_bavail_fid, st.f_bavail);
	env->SetLongField(j_stonestatvfs, stonestatvfs_files_fid, st.f_files);
	env->SetLongField(j_stonestatvfs, stonestatvfs_fsid_fid, st.f_fsid);
	env->SetLongField(j_stonestatvfs, stonestatvfs_namemax_fid, st.f_namemax);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_getcwd
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_stone_fs_StoneMount_native_1stone_1getcwd
	(JNIEnv *env, jclass clz, jlong j_mntp)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_cwd;

	CHECK_MOUNTED(cmount, NULL);

	ldout(cct, 10) << "jni: getcwd: enter" << dendl;

	c_cwd = stone_getcwd(cmount);
	if (!c_cwd) {
		stoneThrowOutOfMemory(env, "stone_getcwd");
		return NULL;
	}

	ldout(cct, 10) << "jni: getcwd: exit ret " << c_cwd << dendl;

	return env->NewStringUTF(c_cwd);
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_chdir
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1chdir
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: chdir: path " << c_path << dendl;

	ret = stone_chdir(cmount, c_path);

	ldout(cct, 10) << "jni: chdir: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_listdir
 * Signature: (JLjava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_stone_fs_StoneMount_native_1stone_1listdir
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	struct stone_dir_result *dirp;
	list<string>::iterator it;
	list<string> contents;
	const char *c_path;
	jobjectArray dirlist;
	string *ent;
	int ret, buflen, bufpos, i;
	jstring name;
	char *buf;

	CHECK_ARG_NULL(j_path, "@path is null", NULL);
	CHECK_MOUNTED(cmount, NULL);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return NULL;
	}

	ldout(cct, 10) << "jni: listdir: opendir: path " << c_path << dendl;

	/* ret < 0 also includes -ENOTDIR which should return NULL */
	ret = stone_opendir(cmount, c_path, &dirp);
	if (ret) {
		env->ReleaseStringUTFChars(j_path, c_path);
		handle_error(env, ret);
		return NULL;
	}

	ldout(cct, 10) << "jni: listdir: opendir: exit ret " << ret << dendl;

	/* buffer for stone_getdnames() results */
	buflen = 256;
	buf = new (std::nothrow) char[buflen];
	if (!buf)  {
		stoneThrowOutOfMemory(env, "heap allocation failed");
		goto out;
	}

	while (1) {
		ldout(cct, 10) << "jni: listdir: getdnames: enter" << dendl;
		ret = stone_getdnames(cmount, dirp, buf, buflen);
		if (ret == -ERANGE) {
			delete [] buf;
			buflen *= 2;
			buf = new (std::nothrow) char[buflen];
			if (!buf)  {
				stoneThrowOutOfMemory(env, "heap allocation failed");
				goto out;
			}
			continue;
		}

		ldout(cct, 10) << "jni: listdir: getdnames: exit ret " << ret << dendl;

		if (ret <= 0)
			break;

		/* got at least one name */
		bufpos = 0;
		while (bufpos < ret) {
			ent = new (std::nothrow) string(buf + bufpos);
			if (!ent) {
				delete [] buf;
				stoneThrowOutOfMemory(env, "heap allocation failed");
				goto out;
			}

			/* filter out dot files: xref: java.io.File::list() */
			if (ent->compare(".") && ent->compare("..")) {
				contents.push_back(*ent);
				ldout(cct, 20) << "jni: listdir: take path " << *ent << dendl;
			}

			bufpos += ent->size() + 1;
			delete ent;
		}
	}

	delete [] buf;

	if (ret < 0) {
		handle_error(env, ret);
		goto out;
	}

	/* directory list */
	dirlist = env->NewObjectArray(contents.size(), env->FindClass("java/lang/String"), NULL);
	if (!dirlist)
		goto out;

	/*
	* Fill directory listing array.
	*
	* FIXME: how should a partially filled array be cleaned-up properly?
	*/
	for (i = 0, it = contents.begin(); it != contents.end(); ++it) {
		name = env->NewStringUTF(it->c_str());
		if (!name)
			goto out;
		env->SetObjectArrayElement(dirlist, i++, name);
		if (env->ExceptionOccurred())
			goto out;
		env->DeleteLocalRef(name);
	}

	env->ReleaseStringUTFChars(j_path, c_path);
	stone_closedir(cmount, dirp);

	return dirlist;

out:
	env->ReleaseStringUTFChars(j_path, c_path);
	stone_closedir(cmount, dirp);
	return NULL;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_link
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1link
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_oldpath, jstring j_newpath)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_oldpath, *c_newpath;
	int ret;

	CHECK_ARG_NULL(j_oldpath, "@oldpath is null", -1);
	CHECK_ARG_NULL(j_newpath, "@newpath is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_oldpath = env->GetStringUTFChars(j_oldpath, NULL);
	if (!c_oldpath) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	c_newpath = env->GetStringUTFChars(j_newpath, NULL);
	if (!c_newpath) {
		env->ReleaseStringUTFChars(j_oldpath, c_oldpath);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: link: oldpath " << c_oldpath <<
		" newpath " << c_newpath << dendl;

	ret = stone_link(cmount, c_oldpath, c_newpath);

	ldout(cct, 10) << "jni: link: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_oldpath, c_oldpath);
	env->ReleaseStringUTFChars(j_newpath, c_newpath);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_unlink
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1unlink
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: unlink: path " << c_path << dendl;

	ret = stone_unlink(cmount, c_path);

	ldout(cct, 10) << "jni: unlink: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_rename
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1rename
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_from, jstring j_to)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_from, *c_to;
	int ret;

	CHECK_ARG_NULL(j_from, "@from is null", -1);
	CHECK_ARG_NULL(j_to, "@to is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_from = env->GetStringUTFChars(j_from, NULL);
	if (!c_from) {
		stoneThrowInternal(env, "Failed to pin memory!");
		return -1;
	}

	c_to = env->GetStringUTFChars(j_to, NULL);
	if (!c_to) {
		env->ReleaseStringUTFChars(j_from, c_from);
		stoneThrowInternal(env, "Failed to pin memory.");
		return -1;
	}

	ldout(cct, 10) << "jni: rename: from " << c_from << " to " << c_to << dendl;

	ret = stone_rename(cmount, c_from, c_to);

	ldout(cct, 10) << "jni: rename: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_from, c_from);
	env->ReleaseStringUTFChars(j_to, c_to);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_mkdir
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1mkdir
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jint j_mode)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: mkdir: path " << c_path << " mode " << (int)j_mode << dendl;

	ret = stone_mkdir(cmount, c_path, (int)j_mode);

	ldout(cct, 10) << "jni: mkdir: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_mkdirs
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1mkdirs
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jint j_mode)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: mkdirs: path " << c_path << " mode " << (int)j_mode << dendl;

	ret = stone_mkdirs(cmount, c_path, (int)j_mode);

	ldout(cct, 10) << "jni: mkdirs: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_rmdir
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1rmdir
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: rmdir: path " << c_path << dendl;

	ret = stone_rmdir(cmount, c_path);

	ldout(cct, 10) << "jni: rmdir: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_readlink
 * Signature: (JLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_stone_fs_StoneMount_native_1stone_1readlink
  (JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	char *linkname;
	struct stone_statx stx;
	jstring j_linkname;

	CHECK_ARG_NULL(j_path, "@path is null", NULL);
	CHECK_MOUNTED(cmount, NULL);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "failed to pin memory");
		return NULL;
	}

	for (;;) {
		ldout(cct, 10) << "jni: readlink: lstatx " << c_path << dendl;
		int ret = stone_statx(cmount, c_path, &stx, STONE_STATX_SIZE,
				     AT_SYMLINK_NOFOLLOW);
		ldout(cct, 10) << "jni: readlink: lstat exit ret " << ret << dendl;
		if (ret) {
			env->ReleaseStringUTFChars(j_path, c_path);
			handle_error(env, ret);
			return NULL;
		}

		linkname = new (std::nothrow) char[stx.stx_size + 1];
		if (!linkname) {
			env->ReleaseStringUTFChars(j_path, c_path);
			stoneThrowOutOfMemory(env, "head allocation failed");
			return NULL;
		}

		ldout(cct, 10) << "jni: readlink: size " << stx.stx_size << " path " << c_path << dendl;

		ret = stone_readlink(cmount, c_path, linkname, stx.stx_size + 1);

		ldout(cct, 10) << "jni: readlink: exit ret " << ret << dendl;

		if (ret < 0) {
			delete [] linkname;
			env->ReleaseStringUTFChars(j_path, c_path);
			handle_error(env, ret);
			return NULL;
		}

		/* re-stat and try again */
		if (ret > (int)stx.stx_size) {
			delete [] linkname;
			continue;
		}

		linkname[ret] = '\0';
		break;
	}

	env->ReleaseStringUTFChars(j_path, c_path);

	j_linkname = env->NewStringUTF(linkname);
	delete [] linkname;

	return j_linkname;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_symlink
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1symlink
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_oldpath, jstring j_newpath)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_oldpath, *c_newpath;
	int ret;

	CHECK_ARG_NULL(j_oldpath, "@oldpath is null", -1);
	CHECK_ARG_NULL(j_newpath, "@newpath is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_oldpath = env->GetStringUTFChars(j_oldpath, NULL);
	if (!c_oldpath) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	c_newpath = env->GetStringUTFChars(j_newpath, NULL);
	if (!c_newpath) {
		env->ReleaseStringUTFChars(j_oldpath, c_oldpath);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: symlink: oldpath " << c_oldpath <<
		" newpath " << c_newpath << dendl;

	ret = stone_symlink(cmount, c_oldpath, c_newpath);

	ldout(cct, 10) << "jni: symlink: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_oldpath, c_oldpath);
	env->ReleaseStringUTFChars(j_newpath, c_newpath);

	if (ret)
		handle_error(env, ret);

	return ret;
}

#define STONE_J_STONESTAT_MASK (STONE_STATX_UID|STONE_STATX_GID|STONE_STATX_SIZE|STONE_STATX_BLOCKS|STONE_STATX_MTIME|STONE_STATX_ATIME)

static void fill_stonestat(JNIEnv *env, jobject j_stonestat, struct stone_statx *stx)
{
	env->SetIntField(j_stonestat, stonestat_mode_fid, stx->stx_mode);
	env->SetIntField(j_stonestat, stonestat_uid_fid, stx->stx_uid);
	env->SetIntField(j_stonestat, stonestat_gid_fid, stx->stx_gid);
	env->SetLongField(j_stonestat, stonestat_size_fid, stx->stx_size);
	env->SetLongField(j_stonestat, stonestat_blksize_fid, stx->stx_blksize);
	env->SetLongField(j_stonestat, stonestat_blocks_fid, stx->stx_blocks);

	long long time = stx->stx_mtime.tv_sec;
	time *= 1000;
	time += stx->stx_mtime.tv_nsec / 1000000;
	env->SetLongField(j_stonestat, stonestat_m_time_fid, time);

	time = stx->stx_atime.tv_sec;
	time *= 1000;
	time += stx->stx_atime.tv_nsec / 1000000;
	env->SetLongField(j_stonestat, stonestat_a_time_fid, time);

	env->SetBooleanField(j_stonestat, stonestat_is_file_fid,
			S_ISREG(stx->stx_mode) ? JNI_TRUE : JNI_FALSE);

	env->SetBooleanField(j_stonestat, stonestat_is_directory_fid,
			S_ISDIR(stx->stx_mode) ? JNI_TRUE : JNI_FALSE);

	env->SetBooleanField(j_stonestat, stonestat_is_symlink_fid,
			S_ISLNK(stx->stx_mode) ? JNI_TRUE : JNI_FALSE);
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_lstat
 * Signature: (JLjava/lang/String;Lcom/stone/fs/StoneStat;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1lstat
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jobject j_stonestat)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	struct stone_statx stx;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_stonestat, "@stat is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: lstat: path " << c_path << dendl;

	ret = stone_statx(cmount, c_path, &stx, STONE_J_STONESTAT_MASK, AT_SYMLINK_NOFOLLOW);

	ldout(cct, 10) << "jni: lstat exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret) {
	    handle_error(env, ret);
	    return ret;
	}

	fill_stonestat(env, j_stonestat, &stx);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_stat
 * Signature: (JLjava/lang/String;Lcom/stone/fs/StoneStat;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1stat
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jobject j_stonestat)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	struct stone_statx stx;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_stonestat, "@stat is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: stat: path " << c_path << dendl;

	ret = stone_statx(cmount, c_path, &stx, STONE_J_STONESTAT_MASK, 0);

	ldout(cct, 10) << "jni: stat exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret) {
		handle_error(env, ret);
		return ret;
	}

	fill_stonestat(env, j_stonestat, &stx);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_setattr
 * Signature: (JLjava/lang/String;Lcom/stone/fs/StoneStat;I)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1setattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jobject j_stonestat, jint j_mask)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	struct stone_statx stx;
	int ret, mask = fixup_attr_mask(j_mask);

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_stonestat, "@stat is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	memset(&stx, 0, sizeof(stx));

	stx.stx_mode = env->GetIntField(j_stonestat, stonestat_mode_fid);
	stx.stx_uid = env->GetIntField(j_stonestat, stonestat_uid_fid);
	stx.stx_gid = env->GetIntField(j_stonestat, stonestat_gid_fid);
	long mtime_msec = env->GetLongField(j_stonestat, stonestat_m_time_fid);
	long atime_msec = env->GetLongField(j_stonestat, stonestat_a_time_fid);
	stx.stx_mtime.tv_sec = mtime_msec / 1000;
	stx.stx_mtime.tv_nsec = (mtime_msec % 1000) * 1000000;
	stx.stx_atime.tv_sec = atime_msec / 1000;
	stx.stx_atime.tv_nsec = (atime_msec % 1000) * 1000000;
	
	ldout(cct, 10) << "jni: setattr: path " << c_path << " mask " << mask << dendl;

	ret = stone_setattrx(cmount, c_path, &stx, mask, 0);

	ldout(cct, 10) << "jni: setattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_chmod
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1chmod
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jint j_mode)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: chmod: path " << c_path << " mode " << (int)j_mode << dendl;

	ret = stone_chmod(cmount, c_path, (int)j_mode);

	ldout(cct, 10) << "jni: chmod: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_fchmod
 * Signature: (JII)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1fchmod
  (JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jint j_mode)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: fchmod: fd " << (int)j_fd << " mode " << (int)j_mode << dendl;

	ret = stone_fchmod(cmount, (int)j_fd, (int)j_mode);

	ldout(cct, 10) << "jni: fchmod: exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_truncate
 * Signature: (JLjava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1truncate
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jlong j_size)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: truncate: path " << c_path << " size " << (loff_t)j_size << dendl;

	ret = stone_truncate(cmount, c_path, (loff_t)j_size);

	ldout(cct, 10) << "jni: truncate: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_open
 * Signature: (JLjava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1open
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jint j_flags, jint j_mode)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	int ret, flags = fixup_open_flags(j_flags);

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: open: path " << c_path << " flags " << flags
		<< " mode " << (int)j_mode << dendl;

	ret = stone_open(cmount, c_path, flags, (int)j_mode);

	ldout(cct, 10) << "jni: open: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);

	if (ret < 0)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_open_layout
 * Signature: (JLjava/lang/String;IIIIILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1open_1layout
  (JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jint j_flags, jint j_mode,
   jint stripe_unit, jint stripe_count, jint object_size, jstring j_data_pool)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path, *c_data_pool = NULL;
	int ret, flags = fixup_open_flags(j_flags);

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	if (j_data_pool) {
		c_data_pool = env->GetStringUTFChars(j_data_pool, NULL);
		if (!c_data_pool) {
			env->ReleaseStringUTFChars(j_path, c_path);
			stoneThrowInternal(env, "Failed to pin memory");
			return -1;
		}
	}

	ldout(cct, 10) << "jni: open_layout: path " << c_path << " flags " << flags
		<< " mode " << (int)j_mode << " stripe_unit " << stripe_unit
		<< " stripe_count " << stripe_count << " object_size " << object_size
		<< " data_pool " << (c_data_pool ? c_data_pool : "<NULL>") << dendl;

	ret = stone_open_layout(cmount, c_path, flags, (int)j_mode,
			(int)stripe_unit, (int)stripe_count, (int)object_size, c_data_pool);

	ldout(cct, 10) << "jni: open_layout: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	if (j_data_pool)
		env->ReleaseStringUTFChars(j_data_pool, c_data_pool);

	if (ret < 0)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_close
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1close
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: close: fd " << (int)j_fd << dendl;

	ret = stone_close(cmount, (int)j_fd);

	ldout(cct, 10) << "jni: close: ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_lseek
 * Signature: (JIJI)J
 */
JNIEXPORT jlong JNICALL Java_com_stone_fs_StoneMount_native_1stone_1lseek
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jlong j_offset, jint j_whence)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int whence;
	jlong ret;

	CHECK_MOUNTED(cmount, -1);

	switch (j_whence) {
	case JAVA_SEEK_SET:
		whence = SEEK_SET;
		break;
	case JAVA_SEEK_CUR:
		whence = SEEK_CUR;
		break;
	case JAVA_SEEK_END:
		whence = SEEK_END;
		break;
	default:
		stoneThrowIllegalArg(env, "Unknown whence value");
		return -1;
	}

	ldout(cct, 10) << "jni: lseek: fd " << (int)j_fd << " offset "
		<< (long)j_offset << " whence " << whence << dendl;

	ret = stone_lseek(cmount, (int)j_fd, (long)j_offset, whence);

	ldout(cct, 10) << "jni: lseek: exit ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_read
 * Signature: (JI[BJJ)J
 */
JNIEXPORT jlong JNICALL Java_com_stone_fs_StoneMount_native_1stone_1read
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jbyteArray j_buf, jlong j_size, jlong j_offset)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	jsize buf_size;
	jbyte *c_buf;
	long ret;

	CHECK_ARG_NULL(j_buf, "@buf is null", -1);
	CHECK_ARG_BOUNDS(j_size < 0, "@size is negative", -1);
	CHECK_MOUNTED(cmount, -1);

	buf_size = env->GetArrayLength(j_buf);
	CHECK_ARG_BOUNDS(j_size > buf_size, "@size > @buf.length", -1);

	c_buf = env->GetByteArrayElements(j_buf, NULL);
	if (!c_buf) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: read: fd " << (int)j_fd << " len " << (long)j_size <<
		" offset " << (long)j_offset << dendl;

	ret = stone_read(cmount, (int)j_fd, (char*)c_buf, (long)j_size, (long)j_offset);

	ldout(cct, 10) << "jni: read: exit ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, (int)ret);
	else
		env->ReleaseByteArrayElements(j_buf, c_buf, 0);

	return (jlong)ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_write
 * Signature: (JI[BJJ)J
 */
JNIEXPORT jlong JNICALL Java_com_stone_fs_StoneMount_native_1stone_1write
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jbyteArray j_buf, jlong j_size, jlong j_offset)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	jsize buf_size;
	jbyte *c_buf;
	long ret;

	CHECK_ARG_NULL(j_buf, "@buf is null", -1);
	CHECK_ARG_BOUNDS(j_size < 0, "@size is negative", -1);
	CHECK_MOUNTED(cmount, -1);

	buf_size = env->GetArrayLength(j_buf);
	CHECK_ARG_BOUNDS(j_size > buf_size, "@size > @buf.length", -1);

	c_buf = env->GetByteArrayElements(j_buf, NULL);
	if (!c_buf) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: write: fd " << (int)j_fd << " len " << (long)j_size <<
		" offset " << (long)j_offset << dendl;

	ret = stone_write(cmount, (int)j_fd, (char*)c_buf, (long)j_size, (long)j_offset);

	ldout(cct, 10) << "jni: write: exit ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, (int)ret);
	else
		env->ReleaseByteArrayElements(j_buf, c_buf, JNI_ABORT);

	return ret;
}


/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_ftruncate
 * Signature: (JIJ)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1ftruncate
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jlong j_size)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: ftruncate: fd " << (int)j_fd <<
		" size " << (loff_t)j_size << dendl;

	ret = stone_ftruncate(cmount, (int)j_fd, (loff_t)j_size);

	ldout(cct, 10) << "jni: ftruncate: exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_fsync
 * Signature: (JIZ)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1fsync
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jboolean j_dataonly)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	ldout(cct, 10) << "jni: fsync: fd " << (int)j_fd <<
		" dataonly " << (j_dataonly ? 1 : 0) << dendl;

	ret = stone_fsync(cmount, (int)j_fd, j_dataonly ? 1 : 0);

	ldout(cct, 10) << "jni: fsync: exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_flock
 * Signature: (JIZ)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1flock
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jint j_operation, jlong j_owner)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	ldout(cct, 10) << "jni: flock: fd " << (int)j_fd <<
		" operation " << j_operation << " owner " << j_owner << dendl;

	int operation = 0;

#define MAP_FLOCK_FLAG(JNI_MASK, NATIVE_MASK) do {	\
	if ((j_operation & JNI_MASK) != 0) {		\
		operation |= NATIVE_MASK; 		\
		j_operation &= ~JNI_MASK;		\
	} 						\
	} while(0)
	MAP_FLOCK_FLAG(JAVA_LOCK_SH, LOCK_SH);
	MAP_FLOCK_FLAG(JAVA_LOCK_EX, LOCK_EX);
	MAP_FLOCK_FLAG(JAVA_LOCK_NB, LOCK_NB);
	MAP_FLOCK_FLAG(JAVA_LOCK_UN, LOCK_UN);
	if (j_operation != 0) {
		stoneThrowIllegalArg(env, "flock flags");
		return -EINVAL;
	}
#undef MAP_FLOCK_FLAG

	ret = stone_flock(cmount, (int)j_fd, operation, (uint64_t) j_owner);

	ldout(cct, 10) << "jni: flock: exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_fstat
 * Signature: (JILcom/stone/fs/StoneStat;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1fstat
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd, jobject j_stonestat)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	struct stone_statx stx;
	int ret;

	CHECK_ARG_NULL(j_stonestat, "@stat is null", -1);
	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: fstat: fd " << (int)j_fd << dendl;

	ret = stone_fstatx(cmount, (int)j_fd, &stx, STONE_J_STONESTAT_MASK, 0);

	ldout(cct, 10) << "jni: fstat exit ret " << ret << dendl;

	if (ret) {
		handle_error(env, ret);
		return ret;
	}

	fill_stonestat(env, j_stonestat, &stx);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_sync_fs
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1sync_1fs
	(JNIEnv *env, jclass clz, jlong j_mntp)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	ldout(cct, 10) << "jni: sync_fs: enter" << dendl;

	ret = stone_sync_fs(cmount);

	ldout(cct, 10) << "jni: sync_fs: exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_getxattr
 * Signature: (JLjava/lang/String;Ljava/lang/String;[B)J
 */
JNIEXPORT jlong JNICALL Java_com_stone_fs_StoneMount_native_1stone_1getxattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jstring j_name, jbyteArray j_buf)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	const char *c_name;
	jsize buf_size;
	jbyte *c_buf = NULL; /* please gcc with goto */
	long ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_name, "@name is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_name = env->GetStringUTFChars(j_name, NULL);
	if (!c_name) {
		env->ReleaseStringUTFChars(j_path, c_path);
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	/* just lookup the size if buf is null */
	if (!j_buf) {
		buf_size = 0;
		goto do_getxattr;
	}

	c_buf = env->GetByteArrayElements(j_buf, NULL);
	if (!c_buf) {
		env->ReleaseStringUTFChars(j_path, c_path);
		env->ReleaseStringUTFChars(j_name, c_name);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	buf_size = env->GetArrayLength(j_buf);

do_getxattr:

	ldout(cct, 10) << "jni: getxattr: path " << c_path << " name " << c_name <<
		" len " << buf_size << dendl;

	ret = stone_getxattr(cmount, c_path, c_name, c_buf, buf_size);
	if (ret == -ERANGE)
		ret = stone_getxattr(cmount, c_path, c_name, c_buf, 0);

	ldout(cct, 10) << "jni: getxattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	env->ReleaseStringUTFChars(j_name, c_name);
	if (j_buf)
		env->ReleaseByteArrayElements(j_buf, c_buf, 0);

	if (ret < 0)
		handle_error(env, (int)ret);

	return (jlong)ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_lgetxattr
 * Signature: (JLjava/lang/String;Ljava/lang/String;[B)I
 */
JNIEXPORT jlong JNICALL Java_com_stone_fs_StoneMount_native_1stone_1lgetxattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jstring j_name, jbyteArray j_buf)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	const char *c_name;
	jsize buf_size;
	jbyte *c_buf = NULL; /* please gcc with goto */
	long ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_name, "@name is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_name = env->GetStringUTFChars(j_name, NULL);
	if (!c_name) {
		env->ReleaseStringUTFChars(j_path, c_path);
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	/* just lookup the size if buf is null */
	if (!j_buf) {
		buf_size = 0;
		goto do_lgetxattr;
	}

	c_buf = env->GetByteArrayElements(j_buf, NULL);
	if (!c_buf) {
		env->ReleaseStringUTFChars(j_path, c_path);
		env->ReleaseStringUTFChars(j_name, c_name);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	buf_size = env->GetArrayLength(j_buf);

do_lgetxattr:

	ldout(cct, 10) << "jni: lgetxattr: path " << c_path << " name " << c_name <<
		" len " << buf_size << dendl;

	ret = stone_lgetxattr(cmount, c_path, c_name, c_buf, buf_size);
	if (ret == -ERANGE)
		ret = stone_lgetxattr(cmount, c_path, c_name, c_buf, 0);

	ldout(cct, 10) << "jni: lgetxattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	env->ReleaseStringUTFChars(j_name, c_name);
	if (j_buf)
		env->ReleaseByteArrayElements(j_buf, c_buf, 0);

	if (ret < 0)
		handle_error(env, (int)ret);

	return (jlong)ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_listxattr
 * Signature: (JLjava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_stone_fs_StoneMount_native_1stone_1listxattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	jobjectArray xattrlist;
	const char *c_path;
	string *ent;
	jstring name;
	list<string>::iterator it;
	list<string> contents;
	int ret, buflen, bufpos, i;
	char *buf;

	CHECK_ARG_NULL(j_path, "@path is null", NULL);
	CHECK_MOUNTED(cmount, NULL);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return NULL;
	}

	buflen = 1024;
	buf = new (std::nothrow) char[buflen];
	if (!buf) {
		stoneThrowOutOfMemory(env, "head allocation failed");
		goto out;
	}

	while (1) {
		ldout(cct, 10) << "jni: listxattr: path " << c_path << " len " << buflen << dendl;
		ret = stone_listxattr(cmount, c_path, buf, buflen);
		if (ret == -ERANGE) {
			delete [] buf;
			buflen *= 2;
			buf = new (std::nothrow) char[buflen];
			if (!buf)  {
				stoneThrowOutOfMemory(env, "heap allocation failed");
				goto out;
			}
			continue;
		}
		break;
	}

	ldout(cct, 10) << "jni: listxattr: ret " << ret << dendl;

	if (ret < 0) {
		delete [] buf;
		handle_error(env, ret);
		goto out;
	}

	bufpos = 0;
	while (bufpos < ret) {
		ent = new (std::nothrow) string(buf + bufpos);
		if (!ent) {
			delete [] buf;
			stoneThrowOutOfMemory(env, "heap allocation failed");
			goto out;
		}
		contents.push_back(*ent);
		bufpos += ent->size() + 1;
		delete ent;
	}

	delete [] buf;

	xattrlist = env->NewObjectArray(contents.size(), env->FindClass("java/lang/String"), NULL);
	if (!xattrlist)
		goto out;

	for (i = 0, it = contents.begin(); it != contents.end(); ++it) {
		name = env->NewStringUTF(it->c_str());
		if (!name)
			goto out;
		env->SetObjectArrayElement(xattrlist, i++, name);
		if (env->ExceptionOccurred())
			goto out;
		env->DeleteLocalRef(name);
	}

	env->ReleaseStringUTFChars(j_path, c_path);
	return xattrlist;

out:
	env->ReleaseStringUTFChars(j_path, c_path);
	return NULL;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_llistxattr
 * Signature: (JLjava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_stone_fs_StoneMount_native_1stone_1llistxattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	jobjectArray xattrlist;
	const char *c_path;
	string *ent;
	jstring name;
	list<string>::iterator it;
	list<string> contents;
	int ret, buflen, bufpos, i;
	char *buf;

	CHECK_ARG_NULL(j_path, "@path is null", NULL);
	CHECK_MOUNTED(cmount, NULL);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return NULL;
	}

	buflen = 1024;
	buf = new (std::nothrow) char[buflen];
	if (!buf) {
		stoneThrowOutOfMemory(env, "head allocation failed");
		goto out;
	}

	while (1) {
		ldout(cct, 10) << "jni: llistxattr: path " << c_path << " len " << buflen << dendl;
		ret = stone_llistxattr(cmount, c_path, buf, buflen);
		if (ret == -ERANGE) {
			delete [] buf;
			buflen *= 2;
			buf = new (std::nothrow) char[buflen];
			if (!buf)  {
				stoneThrowOutOfMemory(env, "heap allocation failed");
				goto out;
			}
			continue;
		}
		break;
	}

	ldout(cct, 10) << "jni: llistxattr: ret " << ret << dendl;

	if (ret < 0) {
		delete [] buf;
		handle_error(env, ret);
		goto out;
	}

	bufpos = 0;
	while (bufpos < ret) {
		ent = new (std::nothrow) string(buf + bufpos);
		if (!ent) {
			delete [] buf;
			stoneThrowOutOfMemory(env, "heap allocation failed");
			goto out;
		}
		contents.push_back(*ent);
		bufpos += ent->size() + 1;
		delete ent;
	}

	delete [] buf;

	xattrlist = env->NewObjectArray(contents.size(), env->FindClass("java/lang/String"), NULL);
	if (!xattrlist)
		goto out;

	for (i = 0, it = contents.begin(); it != contents.end(); ++it) {
		name = env->NewStringUTF(it->c_str());
		if (!name)
			goto out;
		env->SetObjectArrayElement(xattrlist, i++, name);
		if (env->ExceptionOccurred())
			goto out;
		env->DeleteLocalRef(name);
	}

	env->ReleaseStringUTFChars(j_path, c_path);
	return xattrlist;

out:
	env->ReleaseStringUTFChars(j_path, c_path);
	return NULL;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_removexattr
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1removexattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jstring j_name)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	const char *c_name;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_name, "@name is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_name = env->GetStringUTFChars(j_name, NULL);
	if (!c_name) {
		env->ReleaseStringUTFChars(j_path, c_path);
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: removexattr: path " << c_path << " name " << c_name << dendl;

	ret = stone_removexattr(cmount, c_path, c_name);

	ldout(cct, 10) << "jni: removexattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	env->ReleaseStringUTFChars(j_name, c_name);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_lremovexattr
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1lremovexattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jstring j_name)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	const char *c_name;
	int ret;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_name, "@name is null", -1);
	CHECK_MOUNTED(cmount, -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_name = env->GetStringUTFChars(j_name, NULL);
	if (!c_name) {
		env->ReleaseStringUTFChars(j_path, c_path);
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: lremovexattr: path " << c_path << " name " << c_name << dendl;

	ret = stone_lremovexattr(cmount, c_path, c_name);

	ldout(cct, 10) << "jni: lremovexattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	env->ReleaseStringUTFChars(j_name, c_name);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_setxattr
 * Signature: (JLjava/lang/String;Ljava/lang/String;[BJI)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1setxattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jstring j_name,
	 jbyteArray j_buf, jlong j_size, jint j_flags)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	const char *c_name;
	jsize buf_size;
	jbyte *c_buf;
	int ret, flags;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_name, "@name is null", -1);
	CHECK_ARG_NULL(j_buf, "@buf is null", -1);
	CHECK_ARG_BOUNDS(j_size < 0, "@size is negative", -1);
	CHECK_MOUNTED(cmount, -1);

	buf_size = env->GetArrayLength(j_buf);
	CHECK_ARG_BOUNDS(j_size > buf_size, "@size > @buf.length", -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_name = env->GetStringUTFChars(j_name, NULL);
	if (!c_name) {
		env->ReleaseStringUTFChars(j_path, c_path);
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_buf = env->GetByteArrayElements(j_buf, NULL);
	if (!c_buf) {
		env->ReleaseStringUTFChars(j_path, c_path);
		env->ReleaseStringUTFChars(j_name, c_name);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	switch (j_flags) {
	case JAVA_XATTR_CREATE:
		flags = STONE_XATTR_CREATE;
		break;
	case JAVA_XATTR_REPLACE:
		flags = STONE_XATTR_REPLACE;
		break;
	case JAVA_XATTR_NONE:
		flags = 0;
		break;
	default:
		env->ReleaseStringUTFChars(j_path, c_path);
		env->ReleaseStringUTFChars(j_name, c_name);
		env->ReleaseByteArrayElements(j_buf, c_buf, JNI_ABORT);
		stoneThrowIllegalArg(env, "setxattr flag");
		return -1;
	}

	ldout(cct, 10) << "jni: setxattr: path " << c_path << " name " << c_name
		<< " len " << j_size << " flags " << flags << dendl;

	ret = stone_setxattr(cmount, c_path, c_name, c_buf, j_size, flags);

	ldout(cct, 10) << "jni: setxattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	env->ReleaseStringUTFChars(j_name, c_name);
	env->ReleaseByteArrayElements(j_buf, c_buf, JNI_ABORT);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_lsetxattr
 * Signature: (JLjava/lang/String;Ljava/lang/String;[BJI)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1lsetxattr
	(JNIEnv *env, jclass clz, jlong j_mntp, jstring j_path, jstring j_name,
	 jbyteArray j_buf, jlong j_size, jint j_flags)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_path;
	const char *c_name;
	jsize buf_size;
	jbyte *c_buf;
	int ret, flags;

	CHECK_ARG_NULL(j_path, "@path is null", -1);
	CHECK_ARG_NULL(j_name, "@name is null", -1);
	CHECK_ARG_NULL(j_buf, "@buf is null", -1);
	CHECK_ARG_BOUNDS(j_size < 0, "@size is negative", -1);
	CHECK_MOUNTED(cmount, -1);

	buf_size = env->GetArrayLength(j_buf);
	CHECK_ARG_BOUNDS(j_size > buf_size, "@size > @buf.length", -1);

	c_path = env->GetStringUTFChars(j_path, NULL);
	if (!c_path) {
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_name = env->GetStringUTFChars(j_name, NULL);
	if (!c_name) {
		env->ReleaseStringUTFChars(j_path, c_path);
		stoneThrowInternal(env, "Failed to pin memory");
		return -1;
	}

	c_buf = env->GetByteArrayElements(j_buf, NULL);
	if (!c_buf) {
		env->ReleaseStringUTFChars(j_path, c_path);
		env->ReleaseStringUTFChars(j_name, c_name);
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	switch (j_flags) {
	case JAVA_XATTR_CREATE:
		flags = STONE_XATTR_CREATE;
		break;
	case JAVA_XATTR_REPLACE:
		flags = STONE_XATTR_REPLACE;
		break;
	case JAVA_XATTR_NONE:
		flags = 0;
		break;
	default:
		env->ReleaseStringUTFChars(j_path, c_path);
		env->ReleaseStringUTFChars(j_name, c_name);
		env->ReleaseByteArrayElements(j_buf, c_buf, JNI_ABORT);
		stoneThrowIllegalArg(env, "lsetxattr flag");
		return -1;
	}

	ldout(cct, 10) << "jni: lsetxattr: path " << c_path << " name " << c_name
		<< " len " << j_size << " flags " << flags << dendl;

	ret = stone_lsetxattr(cmount, c_path, c_name, c_buf, j_size, flags);

	ldout(cct, 10) << "jni: lsetxattr: exit ret " << ret << dendl;

	env->ReleaseStringUTFChars(j_path, c_path);
	env->ReleaseStringUTFChars(j_name, c_name);
	env->ReleaseByteArrayElements(j_buf, c_buf, JNI_ABORT);

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_file_stripe_unit
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1file_1stripe_1unit
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: get_file_stripe_unit: fd " << (int)j_fd << dendl;

	ret = stone_get_file_stripe_unit(cmount, (int)j_fd);

	ldout(cct, 10) << "jni: get_file_stripe_unit: exit ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_file_replication
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1file_1replication
	(JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: get_file_replication: fd " << (int)j_fd << dendl;

	ret = stone_get_file_replication(cmount, (int)j_fd);

	ldout(cct, 10) << "jni: get_file_replication: exit ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_file_pool_name
 * Signature: (JI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1file_1pool_1name
  (JNIEnv *env, jclass clz, jlong j_mntp, jint j_fd)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	jstring pool = NULL;
	int ret, buflen = 0;
	char *buf = NULL;

	CHECK_MOUNTED(cmount, NULL);

	ldout(cct, 10) << "jni: get_file_pool_name: fd " << (int)j_fd << dendl;

	for (;;) {
		/* get pool name length (len==0) */
		ret = stone_get_file_pool_name(cmount, (int)j_fd, NULL, 0);
		if (ret < 0)
			break;

		/* allocate buffer */
		if (buf)
			delete [] buf;
		buflen = ret;
		buf = new (std::nothrow) char[buflen+1]; /* +1 for '\0' */
		if (!buf) {
			stoneThrowOutOfMemory(env, "head allocation failed");
			goto out;
		}
		memset(buf, 0, (buflen+1)*sizeof(*buf));

		/* handle zero-length pool name!? */
		if (buflen == 0)
			break;

		/* fill buffer */
		ret = stone_get_file_pool_name(cmount, (int)j_fd, buf, buflen);
		if (ret == -ERANGE) /* size changed! */
			continue;
		else
			break;
	}

	ldout(cct, 10) << "jni: get_file_pool_name: ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, ret);
	else
		pool = env->NewStringUTF(buf);

out:
	if (buf)
	delete [] buf;

	return pool;
}

/**
 * Class: com_stone_fs_StoneMount
 * Method: native_stone_get_default_data_pool_name
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1default_1data_1pool_1name
  (JNIEnv *env, jclass clz, jlong j_mntp)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	jstring pool = NULL;
	int ret, buflen = 0;
	char *buf = NULL;
        
	CHECK_MOUNTED(cmount, NULL);
	 
	ldout(cct, 10) << "jni: get_default_data_pool_name" << dendl;

	ret = stone_get_default_data_pool_name(cmount, NULL, 0);
	if (ret < 0)
		goto out;
	buflen = ret;
	buf = new (std::nothrow) char[buflen+1]; /* +1 for '\0' */
	if (!buf) {
		stoneThrowOutOfMemory(env, "head allocation failed");
		goto out;
	}
	memset(buf, 0, (buflen+1)*sizeof(*buf));
	ret = stone_get_default_data_pool_name(cmount, buf, buflen);
	
	ldout(cct, 10) << "jni: get_default_data_pool_name: ret " << ret << dendl;
	
	if (ret < 0)
		handle_error(env, ret);
	else
		pool = env->NewStringUTF(buf);

out:
	if (buf)
	delete [] buf;

	return pool;        
}


/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_localize_reads
 * Signature: (JZ)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1localize_1reads
	(JNIEnv *env, jclass clz, jlong j_mntp, jboolean j_on)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret, val = j_on ? 1 : 0;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: localize_reads: val " << val << dendl;

	ret = stone_localize_reads(cmount, val);

	ldout(cct, 10) << "jni: localize_reads: exit ret " << ret << dendl;

	if (ret)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_stripe_unit_granularity
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1stripe_1unit_1granularity
	(JNIEnv *env, jclass clz, jlong j_mntp)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: get_stripe_unit_granularity" << dendl;

	ret = stone_get_stripe_unit_granularity(cmount);

	ldout(cct, 10) << "jni: get_stripe_unit_granularity: exit ret " << ret << dendl;

	if (ret < 0)
		handle_error(env, ret);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_pool_id
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1pool_1id
  (JNIEnv *env, jclass clz, jlong j_mntp, jstring jname)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	const char *c_name;
	int ret;

	CHECK_MOUNTED(cmount, -1);
	CHECK_ARG_NULL(jname, "@name is null", -1);

	c_name = env->GetStringUTFChars(jname, NULL);
	if (!c_name) {
		stoneThrowInternal(env, "failed to pin memory");
		return -1;
	}

	ldout(cct, 10) << "jni: get_pool_id: name " << c_name << dendl;

	ret = stone_get_pool_id(cmount, c_name);
	if (ret < 0)
		handle_error(env, ret);

	ldout(cct, 10) << "jni: get_pool_id: ret " << ret << dendl;

	env->ReleaseStringUTFChars(jname, c_name);

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_pool_replication
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1pool_1replication
  (JNIEnv *env, jclass clz, jlong j_mntp, jint jpoolid)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
	int ret;

	CHECK_MOUNTED(cmount, -1);

	ldout(cct, 10) << "jni: get_pool_replication: poolid " << jpoolid << dendl;

	ret = stone_get_pool_replication(cmount, jpoolid);
	if (ret < 0)
		handle_error(env, ret);

	ldout(cct, 10) << "jni: get_pool_replication: ret " << ret << dendl;

	return ret;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_file_extent_osds
 * Signature: (JIJ)Lcom/stone/fs/StoneFileExtent;
 */
JNIEXPORT jobject JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1file_1extent_1osds
  (JNIEnv *env, jclass clz, jlong mntp, jint fd, jlong off)
{
  struct stone_mount_info *cmount = get_stone_mount(mntp);
  StoneContext *cct = stone_get_mount_context(cmount);
  jobject extent = NULL;
  int ret, nosds, *osds = NULL;
  jintArray osd_array;
  loff_t len;

	CHECK_MOUNTED(cmount, NULL);

	ldout(cct, 10) << "jni: get_file_extent_osds: fd " << fd << " off " << off << dendl;

  for (;;) {
    /* get pg size */
    ret = stone_get_file_extent_osds(cmount, fd, off, NULL, NULL, 0);
    if (ret < 0)
      break;

    /* alloc osd id array */
    if (osds)
      delete [] osds;
    nosds = ret;
    osds = new int[nosds];

    /* get osd ids */
    ret = stone_get_file_extent_osds(cmount, fd, off, &len, osds, nosds);
    if (ret == -ERANGE)
      continue;
    else
      break;
  }

	ldout(cct, 10) << "jni: get_file_extent_osds: ret " << ret << dendl;

  if (ret < 0) {
    handle_error(env, ret);
    goto out;
  }

  nosds = ret;

  osd_array = env->NewIntArray(nosds);
  if (!osd_array)
    goto out;

  env->SetIntArrayRegion(osd_array, 0, nosds, osds);
  if (env->ExceptionOccurred())
    goto out;

  extent = env->NewObject(stonefileextent_cls, stonefileextent_ctor_fid, off, len, osd_array);
  if (!extent)
    goto out;

out:
  if (osds)
    delete [] osds;

  return extent;
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_osd_crush_location
 * Signature: (JI)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1osd_1crush_1location
  (JNIEnv *env, jclass clz, jlong j_mntp, jint osdid)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
  jobjectArray path = NULL;
  vector<string> str_path;
	int ret, bufpos, buflen = 0;
  char *buf = NULL;

	CHECK_MOUNTED(cmount, NULL);

  ldout(cct, 10) << "jni: osd loc: osd " << osdid << dendl;

  for (;;) {
    /* get length of the location path */
    ret = stone_get_osd_crush_location(cmount, osdid, NULL, 0);
    if (ret < 0)
      break;

    /* alloc path buffer */
    if (buf)
      delete [] buf;
    buflen = ret;
    buf = new char[buflen+1];
    memset(buf, 0, buflen*sizeof(*buf));

    /* empty path */
    if (buflen == 0)
      break;

    /* get the path */
    ret = stone_get_osd_crush_location(cmount, osdid, buf, buflen);
    if (ret == -ERANGE)
      continue;
    else
      break;
  }

  ldout(cct, 10) << "jni: osd loc: osd " << osdid << " ret " << ret << dendl;

  if (ret < 0) {
    handle_error(env, ret);
    goto out;
  }

  bufpos = 0;
  while (bufpos < ret) {
    string type(buf + bufpos);
    bufpos += type.size() + 1;
    string name(buf + bufpos);
    bufpos += name.size() + 1;
    str_path.push_back(type);
    str_path.push_back(name);
  }

  path = env->NewObjectArray(str_path.size(), env->FindClass("java/lang/String"), NULL);
  if (!path)
    goto out;

  for (unsigned i = 0; i < str_path.size(); i++) {
    jstring ent = env->NewStringUTF(str_path[i].c_str());
    if (!ent)
      goto out;
    env->SetObjectArrayElement(path, i, ent);
    if (env->ExceptionOccurred())
      goto out;
    env->DeleteLocalRef(ent);
  }

out:
  if (buf)
    delete [] buf;

  return path;
}

/*
 * sockaddrToInetAddress uses with the following license, and is adapted for
 * use in this project by using Stone JNI exception utilities.
 *
 * ----
 *
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
jobject sockaddrToInetAddress(JNIEnv* env, const sockaddr_storage& ss, jint* port) {
    // Convert IPv4-mapped IPv6 addresses to IPv4 addresses.
    // The RI states "Java will never return an IPv4-mapped address".
    const sockaddr_in6& sin6 = reinterpret_cast<const sockaddr_in6&>(ss);
    if (ss.ss_family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&sin6.sin6_addr)) {
        // Copy the IPv6 address into the temporary sockaddr_storage.
        sockaddr_storage tmp;
        memset(&tmp, 0, sizeof(tmp));
        memcpy(&tmp, &ss, sizeof(sockaddr_in6));
        // Unmap it into an IPv4 address.
        sockaddr_in& sin = reinterpret_cast<sockaddr_in&>(tmp);
        sin.sin_family = AF_INET;
        sin.sin_port = sin6.sin6_port;
        memcpy(&sin.sin_addr.s_addr, &sin6.sin6_addr.s6_addr[12], 4);
        // Do the regular conversion using the unmapped address.
        return sockaddrToInetAddress(env, tmp, port);
    }

    const void* rawAddress;
    size_t addressLength;
    int sin_port = 0;
    int scope_id = 0;
    if (ss.ss_family == AF_INET) {
        const sockaddr_in& sin = reinterpret_cast<const sockaddr_in&>(ss);
        rawAddress = &sin.sin_addr.s_addr;
        addressLength = 4;
        sin_port = ntohs(sin.sin_port);
    } else if (ss.ss_family == AF_INET6) {
        const sockaddr_in6& sin6 = reinterpret_cast<const sockaddr_in6&>(ss);
        rawAddress = &sin6.sin6_addr.s6_addr;
        addressLength = 16;
        sin_port = ntohs(sin6.sin6_port);
        scope_id = sin6.sin6_scope_id;
    } else if (ss.ss_family == AF_UNIX) {
        const sockaddr_un& sun = reinterpret_cast<const sockaddr_un&>(ss);
        rawAddress = &sun.sun_path;
        addressLength = strlen(sun.sun_path);
    } else {
        // We can't throw SocketException. We aren't meant to see bad addresses, so seeing one
        // really does imply an internal error.
        //jniThrowExceptionFmt(env, "java/lang/IllegalArgumentException",
        //                     "sockaddrToInetAddress unsupported ss_family: %i", ss.ss_family);
        stoneThrowIllegalArg(env, "sockaddrToInetAddress unsupposed ss_family");
        return NULL;
    }
    if (port != NULL) {
        *port = sin_port;
    }

    ScopedLocalRef<jbyteArray> byteArray(env, env->NewByteArray(addressLength));
    if (byteArray.get() == NULL) {
        return NULL;
    }
    env->SetByteArrayRegion(byteArray.get(), 0, addressLength,
			    reinterpret_cast<jbyte*>(const_cast<void*>(rawAddress)));

    if (ss.ss_family == AF_UNIX) {
        // Note that we get here for AF_UNIX sockets on accept(2). The unix(7) man page claims
        // that the peer's sun_path will contain the path, but in practice it doesn't, and the
        // peer length is returned as 2 (meaning only the sun_family field was set).
        //
        // Stone Note: this isn't supported. inetUnixAddress appears to just be
        // something in Dalvik/Android stuff.
        stoneThrowInternal(env, "OSD address should never be a UNIX socket");
        return NULL;
        //static jmethodID ctor = env->GetMethodID(JniConstants::inetUnixAddressClass, "<init>", "([B)V");
        //return env->NewObject(JniConstants::inetUnixAddressClass, ctor, byteArray.get());
    }

    if (addressLength == 4) {
      static jmethodID getByAddressMethod = env->GetStaticMethodID(JniConstants::inetAddressClass,
          "getByAddress", "(Ljava/lang/String;[B)Ljava/net/InetAddress;");
      if (getByAddressMethod == NULL) {
        return NULL;
      }
      return env->CallStaticObjectMethod(JniConstants::inetAddressClass, getByAddressMethod,
          NULL, byteArray.get());
    } else if (addressLength == 16) {
      static jmethodID getByAddressMethod = env->GetStaticMethodID(JniConstants::inet6AddressClass,
          "getByAddress", "(Ljava/lang/String;[BI)Ljava/net/Inet6Address;");
      if (getByAddressMethod == NULL) {
        return NULL;
      }
      return env->CallStaticObjectMethod(JniConstants::inet6AddressClass, getByAddressMethod,
          NULL, byteArray.get(), scope_id);
    } else {
      abort();
      return NULL;
    }
}

/*
 * Class:     com_stone_fs_StoneMount
 * Method:    native_stone_get_osd_addr
 * Signature: (JI)Ljava/net/InetAddress;
 */
JNIEXPORT jobject JNICALL Java_com_stone_fs_StoneMount_native_1stone_1get_1osd_1addr
  (JNIEnv *env, jclass clz, jlong j_mntp, jint osd)
{
	struct stone_mount_info *cmount = get_stone_mount(j_mntp);
	StoneContext *cct = stone_get_mount_context(cmount);
  struct sockaddr_storage addr;
  int ret;

	CHECK_MOUNTED(cmount, NULL);

  ldout(cct, 10) << "jni: get_osd_addr: osd " << osd << dendl;

  ret = stone_get_osd_addr(cmount, osd, &addr);

  ldout(cct, 10) << "jni: get_osd_addr: ret " << ret << dendl;

  if (ret < 0) {
    handle_error(env, ret);
    return NULL;
  }

  return sockaddrToInetAddress(env, addr, NULL);
}
