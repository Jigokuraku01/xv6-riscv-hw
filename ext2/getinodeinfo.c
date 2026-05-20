#include <signal.h>
#define _FILE_OFFSET_BITS 64
#include "ext2.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *mode_type_str(uint16_t mode) {
  switch (mode & 0xF000) {
  case 0x1000:
    return "FIFO";
  case 0x2000:
    return "character device";
  case 0x4000:
    return "directory";
  case 0x6000:
    return "block device";
  case 0x8000:
    return "regular file";
  case 0xA000:
    return "symbolic link";
  case 0xC000:
    return "socket";
  default:
    return "unknown";
  }
}

static void mode_to_str(uint16_t mode, char out[11]) {
  const char *t = "?";
  switch (mode & 0xF000) {
  case 0x1000:
    t = "p";
    break;
  case 0x2000:
    t = "c";
    break;
  case 0x4000:
    t = "d";
    break;
  case 0x6000:
    t = "b";
    break;
  case 0x8000:
    t = "-";
    break;
  case 0xA000:
    t = "l";
    break;
  case 0xC000:
    t = "s";
    break;
  }
  out[0] = t[0];
  out[1] = (mode & 0400) ? 'r' : '-';
  out[2] = (mode & 0200) ? 'w' : '-';
  out[3] = (mode & 0100) ? ((mode & 04000) ? 's' : 'x')
                         : ((mode & 04000) ? 'S' : '-');
  out[4] = (mode & 0040) ? 'r' : '-';
  out[5] = (mode & 0020) ? 'w' : '-';
  out[6] = (mode & 0010) ? ((mode & 02000) ? 's' : 'x')
                         : ((mode & 02000) ? 'S' : '-');
  out[7] = (mode & 0004) ? 'r' : '-';
  out[8] = (mode & 0002) ? 'w' : '-';
  out[9] = (mode & 0001) ? ((mode & 01000) ? 't' : 'x')
                         : ((mode & 01000) ? 'T' : '-');
  out[10] = '\0';
}

static void fmt_time(uint32_t t, char *buf, size_t n) {
  if (t == 0) {
    snprintf(buf, n, "-");
    return;
  }
  time_t tt = (time_t)t;
  struct tm tm;
  gmtime_r(&tt, &tm);
  strftime(buf, n, "%Y-%m-%d %H:%M:%S UTC", &tm);
}

static int print_indirect(ext2_ctx *c, uint32_t bnum, int level, int *count) {
  if (bnum == 0) {
    printf("  [hole indirect L%d]\n", level);
    return 0;
  }
  printf("  indirect L%d block: %u\n", level, bnum);
  uint32_t *buf = malloc(c->block_size);
  if (!buf)
    return -1;
  if (ext2_read_block(c, bnum, buf) < 0) {
    free(buf);
    return -1;
  }
  uint32_t n = c->block_size / 4;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t b = ext2_le32(buf[i]);
    if (level == 1) {
      if (b != 0) {
        printf("    [%d] block %u\n", (*count), b);
      } else {
        printf("    [%d] hole\n", (*count));
      }
      (*count)++;
    } else {
      if (b != 0) {
        if (print_indirect(c, b, level - 1, count) < 0) {
          free(buf);
          return -1;
        }
      } else {
        uint32_t skip = 1;
        for (int l = 1; l < level; l++)
          skip *= n;
        *count += (int)skip;
      }
    }
  }
  free(buf);
  return 0;
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
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

  printf("filesystem: %s\n", argv[1]);
  printf("block size: %u\n", c.block_size);
  printf("inode size: %u\n", c.inode_size);
  printf("inodes per group: %u\n", c.inodes_per_group);
  printf("blocks per group: %u\n", c.blocks_per_group);
  printf("groups: %u\n", c.groups_count);
  printf("host endianness: %s\n", EXT2_HOST_BIG_ENDIAN ? "big" : "little");
  printf("\n");

  struct ext2_inode in;
  if (ext2_read_inode(&c, (uint32_t)ino, &in) < 0) {
    ext2_close(&c);
    return 1;
  }

  uint16_t mode = ext2_le16(in.i_mode);
  char ms[11];
  mode_to_str(mode, ms);
  char ta[64], tm[64], tc[64], td[64];
  fmt_time(ext2_le32(in.i_atime), ta, sizeof(ta));
  fmt_time(ext2_le32(in.i_mtime), tm, sizeof(tm));
  fmt_time(ext2_le32(in.i_ctime), tc, sizeof(tc));
  fmt_time(ext2_le32(in.i_dtime), td, sizeof(td));

  printf("inode:       %lu\n", ino);
  printf("type:        %s\n", mode_type_str(mode));
  printf("mode:        %s (0%o)\n", ms, mode & 0xFFF);
  printf("uid:         %u\n", ext2_le16(in.i_uid));
  printf("gid:         %u\n", ext2_le16(in.i_gid));
  printf("links:       %u\n", ext2_le16(in.i_links_count));
  printf("size:        %" PRIu64 " bytes\n", ext2_inode_size(&in));
  printf("blocks(512): %u\n", ext2_le32(in.i_blocks));
  printf("flags:       0x%x\n", ext2_le32(in.i_flags));
  printf("generation:  %u\n", ext2_le32(in.i_generation));
  printf("atime:       %s\n", ta);
  printf("mtime:       %s\n", tm);
  printf("ctime:       %s\n", tc);
  printf("dtime:       %s\n", td);
  printf("\n");

  printf("direct blocks:\n");
  for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
    uint32_t b = ext2_le32(in.i_block[i]);
    if (b != 0) {
      printf("  [%d] %u\n", i, b);
    } else {
      printf("  [%d] hole\n", i);
    }
  }

  uint32_t ib1 = ext2_le32(in.i_block[EXT2_IND_BLOCK]);
  uint32_t ib2 = ext2_le32(in.i_block[EXT2_DIND_BLOCK]);
  uint32_t ib3 = ext2_le32(in.i_block[EXT2_TIND_BLOCK]);

  printf("\nsingly indirect:\n");
  if (ib1 != 0) {
    int cnt = 0;
    print_indirect(&c, ib1, 1, &cnt);
  } else {
    printf("  (none)\n");
  }

  printf("\ndoubly indirect:\n");
  if (ib2 != 0) {
    int cnt = 0;
    print_indirect(&c, ib2, 2, &cnt);
  } else {
    printf("  (none)\n");
  }

  printf("\ntriply indirect:\n");
  if (ib3 != 0) {
    int cnt = 0;
    print_indirect(&c, ib3, 3, &cnt);
  } else {
    printf("  (none)\n");
  }

  ext2_close(&c);
  return 0;
}
