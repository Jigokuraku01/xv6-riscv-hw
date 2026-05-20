#define _FILE_OFFSET_BITS 64
#include "ext2.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int write_all(int fd, const void *buf, size_t n) {
  size_t done = 0;
  while (done < n) {
    ssize_t w = write(fd, (const char *)buf + done, n - done);
    if (w < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    done += (size_t)w;
  }
  return 0;
}

static int emit_block(const ext2_ctx *c, uint32_t bnum, uint64_t *remaining,
                      void *blkbuf) {
  if (*remaining == 0)
    return 0;
  if (ext2_read_block(c, bnum, blkbuf) < 0)
    return -1;
  size_t to_write = c->block_size;
  if ((uint64_t)to_write > *remaining)
    to_write = (size_t)*remaining;
  if (write_all(STDOUT_FILENO, blkbuf, to_write) < 0)
    return -1;
  *remaining -= to_write;
  return 0;
}

static int emit_indirect(const ext2_ctx *c, uint32_t bnum, int level,
                         uint64_t *remaining, void *blkbuf) {
  if (*remaining == 0)
    return 0;
  uint32_t n = c->block_size / 4;

  if (bnum == 0) {
    uint64_t covered_blocks = 1;
    for (int l = 0; l < level; l++)
      covered_blocks *= n;
    uint64_t bytes = covered_blocks * c->block_size;
    if (bytes > *remaining)
      bytes = *remaining;
    char zeros[4096];
    memset(zeros, 0, sizeof(zeros));
    while (bytes > 0) {
      size_t chunk = bytes > sizeof(zeros) ? sizeof(zeros) : (size_t)bytes;
      if (write_all(STDOUT_FILENO, zeros, chunk) < 0)
        return -1;
      bytes -= chunk;
      *remaining -= chunk;
    }
    return 0;
  }

  uint32_t *table = malloc(c->block_size);
  if (!table)
    return -1;
  if (ext2_read_block(c, bnum, table) < 0) {
    free(table);
    return -1;
  }

  for (uint32_t i = 0; i < n && *remaining > 0; i++) {
    uint32_t b = ext2_le32(table[i]);
    int rc;
    if (level == 1) {
      rc = emit_block(c, b, remaining, blkbuf);
    } else {
      rc = emit_indirect(c, b, level - 1, remaining, blkbuf);
    }
    if (rc < 0) {
      free(table);
      return -1;
    }
  }
  free(table);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s <image> <inode>\n", argv[0]);
    return 1;
  }
  char *end;
  unsigned long ino = strtoul(argv[2], &end, 10);
  if (*end != '\0' || ino == 0) {
    fprintf(stderr, "bad inode number: %s\n", argv[2]);
    return 1;
  }

  ext2_ctx c;
  if (ext2_open(&c, argv[1]) < 0)
    return 1;

  struct ext2_inode in;
  if (ext2_read_inode(&c, (uint32_t)ino, &in) < 0) {
    ext2_close(&c);
    return 1;
  }

  uint16_t mode = ext2_le16(in.i_mode);
  uint64_t size = ext2_inode_size(&in);

  if ((mode & 0xA000) == 0xA000 && size > 0 && size < 60) {
    if (write_all(STDOUT_FILENO, in.i_block, (size_t)size) < 0) {
      perror("write");
      ext2_close(&c);
      return 1;
    }
    ext2_close(&c);
    return 0;
  }

  void *blkbuf = malloc(c.block_size);
  if (!blkbuf) {
    fprintf(stderr, "out of memory\n");
    ext2_close(&c);
    return 1;
  }

  uint64_t remaining = size;

  for (int i = 0; i < EXT2_NDIR_BLOCKS && remaining > 0; i++) {
    uint32_t b = ext2_le32(in.i_block[i]);
    if (emit_block(&c, b, &remaining, blkbuf) < 0) {
      fprintf(stderr, "read error at direct block %d\n", i);
      free(blkbuf);
      ext2_close(&c);
      return 1;
    }
  }

  if (remaining > 0) {
    uint32_t b = ext2_le32(in.i_block[EXT2_IND_BLOCK]);
    if (emit_indirect(&c, b, 1, &remaining, blkbuf) < 0) {
      free(blkbuf);
      ext2_close(&c);
      return 1;
    }
  }
  if (remaining > 0) {
    uint32_t b = ext2_le32(in.i_block[EXT2_DIND_BLOCK]);
    if (emit_indirect(&c, b, 2, &remaining, blkbuf) < 0) {
      free(blkbuf);
      ext2_close(&c);
      return 1;
    }
  }
  if (remaining > 0) {
    uint32_t b = ext2_le32(in.i_block[EXT2_TIND_BLOCK]);
    if (emit_indirect(&c, b, 3, &remaining, blkbuf) < 0) {
      free(blkbuf);
      ext2_close(&c);
      return 1;
    }
  }

  free(blkbuf);
  ext2_close(&c);

  if (remaining > 0) {
    fprintf(stderr, "warning: %" PRIu64 " bytes were not produced\n",
            remaining);
    return 2;
  }
  return 0;
}
