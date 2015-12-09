/*
 * ceph_fs.cc - Some Ceph functions that are shared between kernel space and
 * user space.
 *
 */

#include <errno.h>

/*
 * Some non-inline ceph helpers
 */
#include "include/types.h"

/*
 * return true if @layout appears to be valid
 */
int ceph_file_layout_is_valid(const struct ceph_file_layout *layout)
{
	__u32 su = le32_to_cpu(layout->fl_stripe_unit);
	__u32 sc = le32_to_cpu(layout->fl_stripe_count);
	__u32 os = le32_to_cpu(layout->fl_object_size);

	/* stripe unit, object size must be non-zero, 64k increment */
	if (!su || (su & (CEPH_MIN_STRIPE_UNIT-1)))
		return 0;
	if (!os || (os & (CEPH_MIN_STRIPE_UNIT-1)))
		return 0;
	/* object size must be a multiple of stripe unit */
	if (os < su || os % su)
		return 0;
	/* stripe count must be non-zero */
	if (!sc)
		return 0;
	return 1;
}


int ceph_flags_to_mode(int flags)
{
	/* because CEPH_FILE_MODE_PIN is zero, so mode = -1 is error */
	int mode = -1;

#ifdef O_DIRECTORY  /* fixme */
	if ((flags & O_DIRECTORY) == O_DIRECTORY)
		return CEPH_FILE_MODE_PIN;
#endif

	switch (flags & O_ACCMODE) {
	case O_WRONLY:
		mode = CEPH_FILE_MODE_WR;
		break;
	case O_RDONLY:
		mode = CEPH_FILE_MODE_RD;
		break;
	case O_RDWR:
	case O_ACCMODE: /* this is what the VFS does */
		mode = CEPH_FILE_MODE_RDWR;
		break;
	}

	return mode;
}

int ceph_caps_for_mode(int mode)
{
	int caps = CEPH_CAP_PIN;

	if (mode & CEPH_FILE_MODE_RD)
		caps |= CEPH_CAP_FILE_SHARED |
			CEPH_CAP_FILE_RD | CEPH_CAP_FILE_CACHE;
	if (mode & CEPH_FILE_MODE_WR)
		caps |= CEPH_CAP_FILE_EXCL |
			CEPH_CAP_FILE_WR | CEPH_CAP_FILE_BUFFER |
			CEPH_CAP_AUTH_SHARED | CEPH_CAP_AUTH_EXCL |
			CEPH_CAP_XATTR_SHARED | CEPH_CAP_XATTR_EXCL;
	if (mode & CEPH_FILE_MODE_LAZY)
		caps |= CEPH_CAP_FILE_LAZYIO;

	return caps;
}

struct ceph_file_layout file_layout_legacy(const file_layout_t& fl)
{
	struct ceph_file_layout old_fl;
	old_fl.fl_stripe_unit = fl.fl_stripe_unit;
	old_fl.fl_stripe_count = fl.fl_stripe_count;
	old_fl.fl_object_size = fl.fl_object_size;
	old_fl.fl_cas_hash = 0;
	old_fl.fl_object_stripe_unit = 0;
	old_fl.fl_unused = 0;
	old_fl.fl_pg_pool = fl.fl_pg_pool;
	return old_fl;
}
