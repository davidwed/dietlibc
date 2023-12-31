#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <sys/cdefs.h>
#include <sys/types.h>
#define _LINUX_CONFIG_H

__BEGIN_DECLS

#define BLOCK_SIZE	1024
#define BLOCK_SIZE_BITS	10


/* These are the fs-independent mount-flags: up to 16 flags are
   supported  */
enum
{
  MS_RDONLY = 1,		/* Mount read-only.  */
#define MS_RDONLY	MS_RDONLY
  MS_NOSUID = 2,		/* Ignore suid and sgid bits.  */
#define MS_NOSUID	MS_NOSUID
  MS_NODEV = 4,			/* Disallow access to device special files.  */
#define MS_NODEV	MS_NODEV
  MS_NOEXEC = 8,		/* Disallow program execution.  */
#define MS_NOEXEC	MS_NOEXEC
  MS_SYNCHRONOUS = 16,		/* Writes are synced at once.  */
#define MS_SYNCHRONOUS	MS_SYNCHRONOUS
  MS_REMOUNT = 32,		/* Alter flags of a mounted FS.  */
#define MS_REMOUNT	MS_REMOUNT
  MS_MANDLOCK = 64,		/* Allow mandatory locks on an FS.  */
#define MS_MANDLOCK	MS_MANDLOCK
  S_WRITE = 128,		/* Write on file/directory/symlink.  */
#define S_WRITE		S_WRITE
  S_APPEND = 256,		/* Append-only file.  */
#define S_APPEND	S_APPEND
  S_IMMUTABLE = 512,		/* Immutable file.  */
#define S_IMMUTABLE	S_IMMUTABLE
  MS_NOATIME = 1024,		/* Do not update access times.  */
#define MS_NOATIME	MS_NOATIME
  MS_NODIRATIME = 2048,		/* Do not update directory access times.  */
#define MS_NODIRATIME	MS_NODIRATIME
  MS_BIND = 4096,		/* Bind directory at different place.  */
#define MS_BIND		MS_BIND

  MS_MOVE = 8192,
#define MS_MOVE		MS_MOVE

  MS_REC = 16384,
#define MS_REC		MS_REC

  MS_SILENT = 32768,
#define MS_VERBOSE	MS_SILENT
#define MS_SILENT	MS_SILENT

  MS_POSIXACL = (1<<16),	/* VFS does not apply the umask */
#define MS_POSIXACL	MS_POSIXACL

  MS_UNBINDABLE = (1<<17),	/* change to unbindable */

  MS_PRIVATE = (1<<18),		/* change to private */
#define MS_PRIVATE	MS_PRIVATE

  MS_SLAVE = (1<<19),	/* change to slave */
#define MS_SLAVE	MS_SLAVE

  MS_SHARED = (1<<20),	/* change to shared */
#define MS_SHARED	MS_SHARED

  MS_RELATIME = (1<<21),	/* Update atime relative to mtime/ctime. */
#define MS_RELATIME	MS_RELATIME

  MS_KERNMOUNT = (1<<22),	/* this is a kern_mount call */
#define MS_KERNMOUNT	MS_KERNMOUNT

  MS_I_VERSION = (1<<23), /* Update inode I_version field */
#define MS_I_VERSION	MS_I_VERSION
  MS_STRICTATIME = (1<<24), /* Always perform atime updates */
#define MS_STRICTATIME	MS_STRICTATIME
  MS_NOSEC = (1<<28),
#define MS_NOSEC	MS_NOSEC
  MS_BORN = (1<<29),
#define MS_BORN		MS_BORN
  MS_ACTIVE = (1<<30),
#define MS_ACTIVE	MS_ACTIVE
  MS_NOUSER = (1<<31),
#define MS_NOUSER	MS_NOUSER

};

/* Flags that can be altered by MS_REMOUNT  */
#define MS_RMT_MASK	(MS_RDONLY|MS_SYNCHRONOUS|MS_MANDLOCK|MS_I_VERSION)


/* Magic mount flag number. Has to be or-ed to the flag values.  */

#define MS_MGC_VAL 0xc0ed0000	/* Magic flag number to indicate "new" flags */
#define MS_MGC_MSK 0xffff0000	/* Magic flag number mask */


/* The read-only stuff doesn't really belong here, but any other place
   is probably as bad and I don't want to create yet another include
   file.  */

#define BLKROSET   _IO(0x12, 93) /* Set device read-only (0 = read-write).  */
#define BLKROGET   _IO(0x12, 94) /* Get read-only status (0 = read_write).  */
#define BLKRRPART  _IO(0x12, 95) /* Re-read partition table.  */
#define BLKGETSIZE _IO(0x12, 96) /* Return device size.  */
#define BLKFLSBUF  _IO(0x12, 97) /* Flush buffer cache.  */
#define BLKRASET   _IO(0x12, 98) /* Set read ahead for block device.  */
#define BLKRAGET   _IO(0x12, 99) /* Get current read ahead setting.  */
#define BLKFRASET  _IO(0x12,100)/* set filesystem (mm/filemap.c) read-ahead */
#define BLKFRAGET  _IO(0x12,101)/* get filesystem (mm/filemap.c) read-ahead */
#define BLKSECTSET _IO(0x12,102)/* set max sectors per request (ll_rw_blk.c) */
#define BLKSECTGET _IO(0x12,103)/* get max sectors per request (ll_rw_blk.c) */
#define BLKSSZGET  _IO(0x12,104)/* get block device sector size */

/* A jump here: 108-111 have been used for various private purposes. */
#define BLKBSZGET  _IOR(0x12,112,size_t)
#define BLKBSZSET  _IOW(0x12,113,size_t)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)	/* return device size in bytes (u64 *arg) */
#define BLKTRACESETUP _IOWR(0x12,115,struct blk_user_trace_setup)
#define BLKTRACESTART _IO(0x12,116)
#define BLKTRACESTOP _IO(0x12,117)
#define BLKTRACETEARDOWN _IO(0x12,118)
#define BLKDISCARD _IO(0x12,119)
#define BLKIOMIN _IO(0x12,120)
#define BLKIOOPT _IO(0x12,121)
#define BLKALIGNOFF _IO(0x12,122)
#define BLKPBSZGET _IO(0x12,123)
#define BLKDISCARDZEROES _IO(0x12,124)
#define BLKSECDISCARD _IO(0x12,125)
#define BLKROTATIONAL _IO(0x12,126)
#define BLKZEROOUT _IO(0x12,127)
/*
 * A jump here: 130-131 are reserved for zoned block devices
 * (see uapi/linux/blkzoned.h)
 */

#define BMAP_IOCTL 1		/* obsolete - kept for compatibility */
#define FIBMAP	   _IO(0x00,1)	/* bmap access */
#define FIGETBSZ   _IO(0x00,2)	/* get the block size used for bmap */
#define FIFREEZE	_IOWR('X', 119, int)	/* Freeze */
#define FITHAW		_IOWR('X', 120, int)	/* Thaw */
#define FITRIM		_IOWR('X', 121, struct fstrim_range)	/* Trim */
#define FICLONE		_IOW(0x94, 9, int)
#define FICLONERANGE	_IOW(0x94, 13, struct file_clone_range)
#define FIDEDUPERANGE	_IOWR(0x94, 54, struct file_dedupe_range)

#define	FS_IOC_GETFLAGS			_IOR('f', 1, long)
#define	FS_IOC_SETFLAGS			_IOW('f', 2, long)
#define	FS_IOC_GETVERSION		_IOR('v', 1, long)
#define	FS_IOC_SETVERSION		_IOW('v', 2, long)
#define FS_IOC_FIEMAP			_IOWR('f', 11, struct fiemap)
#define FS_IOC32_GETFLAGS		_IOR('f', 1, int)
#define FS_IOC32_SETFLAGS		_IOW('f', 2, int)
#define FS_IOC32_GETVERSION		_IOR('v', 1, int)
#define FS_IOC32_SETVERSION		_IOW('v', 2, int)
#define FS_IOC_FSGETXATTR		_IOR ('X', 31, struct fsxattr)
#define FS_IOC_FSSETXATTR		_IOW ('X', 32, struct fsxattr)

/*
 * File system encryption support
 */
/* Policy provided via an ioctl on the topmost directory */
#define FS_KEY_DESCRIPTOR_SIZE	8

#define FS_POLICY_FLAGS_PAD_4		0x00
#define FS_POLICY_FLAGS_PAD_8		0x01
#define FS_POLICY_FLAGS_PAD_16		0x02
#define FS_POLICY_FLAGS_PAD_32		0x03
#define FS_POLICY_FLAGS_PAD_MASK	0x03
#define FS_POLICY_FLAGS_VALID		0x03

/* Encryption algorithms */
#define FS_ENCRYPTION_MODE_INVALID		0
#define FS_ENCRYPTION_MODE_AES_256_XTS		1
#define FS_ENCRYPTION_MODE_AES_256_GCM		2
#define FS_ENCRYPTION_MODE_AES_256_CBC		3
#define FS_ENCRYPTION_MODE_AES_256_CTS		4
#define FS_ENCRYPTION_MODE_AES_128_CBC		5
#define FS_ENCRYPTION_MODE_AES_128_CTS		6

/* Possible value for FLAGS parameter of `umount2'.  */
enum
{
  MNT_FORCE = 1,		/* Force unmounting.  */
#define MNT_FORCE MNT_FORCE
  MNT_DETACH = 2,		/* Just detach, unmount when last reference dies.  */
#define MNT_DETACH MNT_DETACH
  MNT_EXPIRE = 4,		/* Mark for expiry.  */
#define MNT_EXPIRE MNT_EXPIRE
  UMOUNT_NOFOLLOW = 8		/* Don't follow symlink on umount.  */
#define UMOUNT_NOFOLLOW UMOUNT_NOFOLLOW
};

int  mount(const char* specialfile, const char* dir, const char* filesystemtype,
	   unsigned long rwflag, const void * data) __THROW;

int umount(const char *specialfile) __THROW;
int umount2(const char *specialfile, int mflag) __THROW;

__END_DECLS

#endif
