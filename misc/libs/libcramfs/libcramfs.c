/*
 * cramfsck - check a cramfs file system
/* compile-time options */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/cramfs_fs.h>
#include <zlib.h>

#define PAD_SIZE 512
#define PAGE_CACHE_SIZE (4096)

int cramfs_crc(char *filename)
{
	struct cramfs_super super;	/* just find the cramfs superblock once */
	int start = 0;
	int fd;
	size_t length;
	struct stat st;
	void *buf;
	u32 crc;

	/* find the physical size of the file or block device */
	if (stat(filename, &st) < 0) {
		return -2;
	}
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		return -2;
	}
	if (S_ISBLK(st.st_mode)) {
		if (ioctl(fd, BLKGETSIZE, length) < 0) {
			fprintf(stderr, "unable to determine device size: %s\n", filename);
			return -2;
		}
		length = length * 512;
	}
	else if (S_ISREG(st.st_mode)) {
		length = st.st_size;
	}
	else {
		fprintf(stderr, "not a block device or file: %s\n", filename);
		return -3;
	}

	if (length < sizeof(struct cramfs_super)) {
		fprintf(stderr, "file length too short\n");
		return -3;
	}

	/* find superblock */
	if (read(fd, &super, sizeof(super)) != sizeof(super)) {
		fprintf(stderr, "read failed: %s\n", filename);
		return -3;
	}
	if (super.magic == CRAMFS_32(CRAMFS_MAGIC)) {
		start = 0;
	}
	else if (length >= (PAD_SIZE + sizeof(super))) {
		lseek(fd, PAD_SIZE, SEEK_SET);
		if (read(fd, &super, sizeof(super)) != sizeof(super)) {
			fprintf(stderr, "read failed: %s\n", filename);
			return -3;
		}
		if (super.magic == CRAMFS_32(CRAMFS_MAGIC)) {
			start = PAD_SIZE;
		}
	}

	/* superblock tests */
	if (super.magic != CRAMFS_32(CRAMFS_MAGIC)) {
		fprintf(stderr, "superblock magic not found\n");
		return -3;
	}
#if __BYTE_ORDER == __BIG_ENDIAN
	super.size = CRAMFS_32(super.size);
	super.flags = CRAMFS_32(super.flags);
	super.future = CRAMFS_32(super.future);
	super.fsid.crc = CRAMFS_32(super.fsid.crc);
	super.fsid.edition = CRAMFS_32(super.fsid.edition);
	super.fsid.blocks = CRAMFS_32(super.fsid.blocks);
	super.fsid.files = CRAMFS_32(super.fsid.files);
#endif /* __BYTE_ORDER == __BIG_ENDIAN */
	if (super.flags & ~CRAMFS_SUPPORTED_FLAGS) {
	fprintf(stderr, "unsupported filesystem features\n");
		return -3;
	}
	if (super.size < PAGE_CACHE_SIZE) {
		fprintf(stderr, "superblock size (%d) too small\n", super.size);
		return -3;
	}
	if (super.flags & CRAMFS_FLAG_FSID_VERSION_2) {
		if (super.fsid.files == 0) {
			fprintf(stderr, "zero file count\n");
			return -3;
		}
		if (length < super.size) {
			fprintf(stderr, "file length too short\n");
			return -3;
		}
		else if (length > super.size) {
			fprintf(stderr, "warning: file extends past end of filesystem\n");
		}
	}
	else {
		fprintf(stderr, "warning: old cramfs format\n");
	}

	if (!(super.flags & CRAMFS_FLAG_FSID_VERSION_2)) {
		fprintf(stderr, "unable to test CRC: old cramfs format\n");
		return -3;
	}

// start crc
	crc = crc32(0L, Z_NULL, 0);

	buf = mmap(NULL, super.size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (buf == MAP_FAILED) {
		buf = mmap(NULL, super.size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (buf != MAP_FAILED) {
			lseek(fd, 0, SEEK_SET);
			read(fd, buf, super.size);
		}
	}
	if (buf != MAP_FAILED) {
		((struct cramfs_super *) (buf+start))->fsid.crc = crc32(0L, Z_NULL, 0);
		crc = crc32(crc, buf+start, super.size-start);
		munmap(buf, super.size);
	}
	else {
		int retval;
		size_t length = 0;

		buf = malloc(4096);
		if (!buf) {
			printf("malloc failed");
			return -3;
		}
		lseek(fd, start, SEEK_SET);
		for (;;) {
			retval = read(fd, buf, 4096);
			if (retval < 0) {
				printf("read failed: %s", filename);
				return -3;
			}
			else if (retval == 0) {
				break;
			}
			if (length == 0) {
				((struct cramfs_super *) buf)->fsid.crc = crc32(0L, Z_NULL, 0);
			}
			length += retval;
			if (length > (super.size-start)) {
				crc = crc32(crc, buf, retval - (length - (super.size-start)));
				break;
			}
			crc = crc32(crc, buf, retval);
		}
		free(buf);
	}

//printf("Comp. CRC: %x\nRead CRC: %x\n", crc, super.fsid.crc);
	if (crc != super.fsid.crc) {
		return -4;
	}

	return 1;
}
