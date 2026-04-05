#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

static int failures = 0;

int g_single = 7;
char g_array[256];

static void
check(int cond, char *name)
{
  if(cond){
    printf("[PASS] %s\n", name);
  } else {
    printf("[FAIL] %s\n", name);
    failures++;
  }
}

static void
print_flags(char *name, void *buf, uint64 len, int exp_a, int exp_d)
{
  int a = pageflagscheck(buf, len, PTE_A);
  int d = pageflagscheck(buf, len, PTE_D);

  if(a < 0 || d < 0){
    check(0, name);
    return;
  }

  check(a == exp_a && d == exp_d, name);
  printf("%s: A=%d D=%d\n", name, a, d);
}

static void
dump_stage(char *title)
{
  printf("\n=== %s ===\n", title);
  pagetableprint();
}

static int
do_reads(void)
{
  volatile int sink = 0;
  int st_single = 11;
  int st_array[512];
  int *heap_single;
  char *heap_array;
  uint64 sz;

  dump_stage("start");

  heap_single = (int *)malloc(sizeof(int));
  heap_array = (char *)malloc(3 * PGSIZE);
  if(heap_single == 0 || heap_array == 0){
    check(0, "malloc for read test");
    return -1;
  }
  check(1, "malloc for read test");

  *heap_single = 13;
  for(int i = 0; i < sizeof(st_array) / sizeof(st_array[0]); i++)
    st_array[i] = i;
  for(int i = 0; i < 3 * PGSIZE; i++)
    heap_array[i] = (char)(i & 0x7f);
  g_array[0] = 1;

  dump_stage("after allocation");

  sz = (uint64)sbrk(0);
    int clear_ok = pageflagsclear((void *)0, sz, PTE_A | PTE_D);
    check(clear_ok == 0, "pageflagsclear(all A|D)");
    if(clear_ok < 0){
    free(heap_single);
    free(heap_array);
    return -1;
  }

  dump_stage("after clear A/D for all pages");

  sink += g_single;
  sink += g_array[0];
  sink += st_single;
  sink += st_array[5];
  sink += *heap_single;
  sink += heap_array[0];
  sink += heap_array[PGSIZE];
  sink += heap_array[2 * PGSIZE];

  print_flags("read: global single", &g_single, sizeof(g_single), 1, 0);
  print_flags("read: global array elem", &g_array[0], sizeof(g_array[0]), 1, 0);
  print_flags("read: stack single", &st_single, sizeof(st_single), 1, 1);
  print_flags("read: stack array elem", &st_array[5], sizeof(st_array[5]), 1, 1);
  print_flags("read: heap single", heap_single, sizeof(*heap_single), 1, 0);
  print_flags("read: heap array elem[0]", &heap_array[0], sizeof(heap_array[0]), 1, 0);
  print_flags("read: heap array elem[PGSIZE]", &heap_array[PGSIZE], sizeof(heap_array[0]), 1, 0);
  print_flags("read: heap array elem[2*PGSIZE]", &heap_array[2 * PGSIZE], sizeof(heap_array[0]), 1, 0);

  dump_stage("after reads");

  g_single += 1;
  g_array[0] += 1;
  st_single += 1;
  st_array[5] += 1;
  *heap_single += 1;
  heap_array[0] += 1;
  heap_array[PGSIZE] += 1;
  heap_array[2 * PGSIZE] += 1;

  print_flags("write: global single", &g_single, sizeof(g_single), 1, 1);
  print_flags("write: global array elem", &g_array[0], sizeof(g_array[0]), 1, 1);
  print_flags("write: stack single", &st_single, sizeof(st_single), 1, 1);
  print_flags("write: stack array elem", &st_array[5], sizeof(st_array[5]), 1, 1);
  print_flags("write: heap single", heap_single, sizeof(*heap_single), 1, 1);
  print_flags("write: heap array elem[0]", &heap_array[0], sizeof(heap_array[0]), 1, 1);
  print_flags("write: heap array elem[PGSIZE]", &heap_array[PGSIZE], sizeof(heap_array[0]), 1, 1);
  print_flags("write: heap array elem[2*PGSIZE]", &heap_array[2 * PGSIZE], sizeof(heap_array[0]), 1, 1);

  dump_stage("after writes");

  free(heap_single);
  free(heap_array);

  dump_stage("after free");

  if(sink == 0)
    printf("sink=%d\n", sink);

  return 0;
}

static void
test_invalid_mask(void)
{
  int x = 1;
  check(pageflagscheck(&x, sizeof(x), PTE_A | (1L << 9)) < 0,
        "pageflagscheck invalid mask");
  check(pageflagsclear(&x, sizeof(x), PTE_D | (1L << 9)) < 0,
        "pageflagsclear invalid mask");
}

int
main(void)
{
  printf("hw_task5_cases: start\n");

  test_invalid_mask();
  if(do_reads() < 0)
    failures++;

  if(failures == 0){
    printf("hw_task5_cases: ALL PASS\n");
    exit(0);
  }

  printf("hw_task5_cases: FAILURES=%d\n", failures);
  exit(1);
}
