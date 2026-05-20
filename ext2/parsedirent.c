#include <signal.h>
#define _FILE_OFFSET_BITS 64
#include "ext2.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *ft_str(uint8_t t) {
  switch (t) {
  case EXT2_FT_UNKNOWN:
    return "?";
  case EXT2_FT_REG_FILE:
    return "file";
  case EXT2_FT_DIR:
    return "dir";
  case EXT2_FT_CHRDEV:
    return "chr";
  case EXT2_FT_BLKDEV:
    return "blk";
  case EXT2_FT_FIFO:
    return "fifo";
  case EXT2_FT_SOCK:
    return "sock";
  case EXT2_FT_SYMLINK:
    return "link";
  default:
    return "?";
  }
}

static int read_all_stdin(unsigned char **out, size_t *out_len) {
  size_t cap = 65536;
  size_t len = 0;
  unsigned char *buf = malloc(cap);
  if (!buf)
    return -1;
  for (;;) {
    if (len == cap) {
      cap *= 2;
      unsigned char *nb = realloc(buf, cap);
      if (!nb) {
        free(buf);
        return -1;
      }
      buf = nb;
    }
    ssize_t r = read(STDIN_FILENO, buf + len, cap - len);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      free(buf);
      return -1;
    }
    if (r == 0)
      break;
    len += (size_t)r;
  }
  *out = buf;
  *out_len = len;
  return 0;
}

int main(int argc, char **argv) {
  size_t block_size = 0;
  int with_type = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      block_size = (size_t)strtoul(argv[++i], NULL, 10);
    } else if (strcmp(argv[i], "--no-type") == 0) {
      with_type = 0;
    } else {
      fprintf(stderr, "usage: %s [-b block_size] [--no-type] < dir_data\n",
              argv[0]);
      return 1;
    }
  }

  unsigned char *data = NULL;
  size_t len = 0;
  if (read_all_stdin(&data, &len) < 0) {
    perror("read");
    return 1;
  }

  printf("%-10s %-6s %s\n", "inode", "type", "name");
  printf("---------- ------ ----\n");

  size_t pos = 0;
  while (pos + 8 <= len) {
    struct ext2_dir_entry e;
    memcpy(&e, data + pos, 8);
    uint32_t inode = ext2_le32(e.inode);
    uint16_t rec_len = ext2_le16(e.rec_len);
    uint8_t name_len = e.name_len;
    uint8_t file_type = e.file_type;

    if (rec_len < 8 || pos + rec_len > len) {
      fprintf(stderr, "broken entry at offset %zu (rec_len=%u)\n", pos,
              rec_len);
      break;
    }

    if (inode != 0 && name_len > 0) {
      if (pos + 8 + name_len > len) {
        fprintf(stderr, "truncated name at %zu\n", pos);
        break;
      }
      char name[256];
      memcpy(name, data + pos + 8, name_len);
      name[name_len] = '\0';
      const char *t = with_type ? ft_str(file_type) : "-";
      printf("%-10u %-6s %s\n", inode, t, name);
    }

    if (block_size > 0) {
      size_t in_block = pos % block_size;
      if (in_block + rec_len > block_size) {
        fprintf(stderr, "entry at %zu crosses block boundary\n", pos);
        break;
      }
    }

    pos += rec_len;
  }

  free(data);
  return 0;
}
