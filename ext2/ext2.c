#define _FILE_OFFSET_BITS 64
#include "ext2.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint16_t ext2_le16(uint16_t x) {
#if EXT2_HOST_BIG_ENDIAN
  return (uint16_t)((x >> 8) | (x << 8));
#else
  return x;
#endif
}

uint32_t ext2_le32(uint32_t x) {
#if EXT2_HOST_BIG_ENDIAN
  return ((x & 0x000000FFu) << 24) | ((x & 0x0000FF00u) << 8) |
         ((x & 0x00FF0000u) >> 8) | ((x & 0xFF000000u) >> 24);
#else
  return x;
#endif
}

static int pread_all(int fd, void *buf, size_t n, off_t off) {
  size_t done = 0;
  while (done < n) {
    ssize_t r = pread(fd, (char *)buf + done, n - done, off + (off_t)done);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (r == 0)
      return -1;
    done += (size_t)r;
  }
  return 0;
}

int ext2_open(ext2_ctx *c, const char *path) {
  c->fd = open(path, O_RDONLY);
  if (c->fd < 0) {
    perror("open");
    return -1;
  }
  if (pread_all(c->fd, &c->sb, sizeof(c->sb), EXT2_SUPERBLOCK_OFFSET) < 0) {
    fprintf(stderr, "cannot read superblock\n");
    close(c->fd);
    c->fd = -1;
    return -1;
  }
  if (ext2_le16(c->sb.s_magic) != EXT2_SUPER_MAGIC) {
    fprintf(stderr, "bad magic 0x%x, not ext2\n", ext2_le16(c->sb.s_magic));
    close(c->fd);
    c->fd = -1;
    return -1;
  }
  c->block_size = 1024u << ext2_le32(c->sb.s_log_block_size);
  if (ext2_le32(c->sb.s_rev_level) >= 1) {
    c->inode_size = ext2_le16(c->sb.s_inode_size);
  } else {
    c->inode_size = 128;
  }
  c->inodes_per_group = ext2_le32(c->sb.s_inodes_per_group);
  c->blocks_per_group = ext2_le32(c->sb.s_blocks_per_group);
  uint32_t bc = ext2_le32(c->sb.s_blocks_count);
  c->groups_count = (bc + c->blocks_per_group - 1) / c->blocks_per_group;
  return 0;
}

void ext2_close(ext2_ctx *c) {
  if (c->fd >= 0) {
    close(c->fd);
    c->fd = -1;
  }
}

int ext2_read_block(const ext2_ctx *c, uint32_t bnum, void *buf) {
  if (bnum == 0) {
    memset(buf, 0, c->block_size);
    return 0;
  }
  return pread_all(c->fd, buf, c->block_size,
                   (off_t)bnum * (off_t)c->block_size);
}

int ext2_read_gd(const ext2_ctx *c, uint32_t group,
                 struct ext2_group_desc *gd) {
  uint32_t gdt_block = (c->block_size == 1024) ? 2 : 1;
  off_t off = (off_t)gdt_block * (off_t)c->block_size +
              (off_t)group * (off_t)sizeof(struct ext2_group_desc);
  return pread_all(c->fd, gd, sizeof(*gd), off);
}

int ext2_read_inode(const ext2_ctx *c, uint32_t ino, struct ext2_inode *out) {
  if (ino == 0) {
    fprintf(stderr, "inode 0 is invalid\n");
    return -1;
  }
  uint32_t group = (ino - 1) / c->inodes_per_group;
  uint32_t idx = (ino - 1) % c->inodes_per_group;
  if (group >= c->groups_count) {
    fprintf(stderr, "inode %u out of range\n", ino);
    return -1;
  }
  struct ext2_group_desc gd;
  if (ext2_read_gd(c, group, &gd) < 0)
    return -1;
  off_t tbl = (off_t)ext2_le32(gd.bg_inode_table) * (off_t)c->block_size;
  off_t off = tbl + (off_t)idx * (off_t)c->inode_size;
  return pread_all(c->fd, out, sizeof(*out), off);
}

uint64_t ext2_inode_size(const struct ext2_inode *in) {
  uint16_t mode = ext2_le16(in->i_mode);
  uint64_t lo = ext2_le32(in->i_size);
  if ((mode & 0xF000) == 0x8000) {
    uint64_t hi = ext2_le32(in->i_dir_acl);
    return lo | (hi << 32);
  }
  return lo;
}
