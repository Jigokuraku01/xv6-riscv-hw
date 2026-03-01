#include "kernel/types.h"
#include "user/user.h"

#define BUFFERSIZE 130

int read_ln(char* buffer){
  int i = 0;
  char c;
  while(i < BUFFERSIZE - 1){
    int read_res = read(0, &c, 1);
    if(read_res < 0){
      return read_res;
    }
    if(read_res == 0){
      break;
    }
    if(c == '\n'){
      break;
    }
    buffer[i] = c;
    i++;
  }
  if(i >= BUFFERSIZE - 1){
    return -1;
  }
  buffer[i] = '\0';
  return i;
}

int my_atoi(char* ptr, int* ans){
  int ans_val = 0;
  int is_neg = 0;
  if(*ptr == '-'){
    ptr++;
    is_neg = 1;
  }
  
  char* num_start = ptr;

  while('0' <= *ptr && *ptr <= '9'){
    ans_val = ans_val*10 + *ptr - '0';
    ptr++;
  }

  if(ptr == num_start){
    return -1;
  }

  if(is_neg){
    ans_val = -ans_val;
  }
  *ans = ans_val;
  return 0;
}

int parse_int(char **ptr, int* num){
  *num = 0;
  while(**ptr == ' '){
    (*ptr)++;
  }
  if(**ptr == '\0'){
    return -1;
  }

  char* num_start = *ptr;
  while(**ptr != ' ' && **ptr != '\0'){
    (*ptr)++;
  }
  char* num_end = *ptr;
  if(num_end == num_start){
    return -1;
  }
  int ans = 0;
  int my_atoi_res = my_atoi(num_start, &ans);
  if(my_atoi_res < 0){
    return my_atoi_res;
  }
  *num = ans;
  return 0;
}


int main(int argc, char *argv[])
{
  char buffer[BUFFERSIZE] = {0};

  int read_ln_res = read_ln(buffer);
  if(read_ln_res < 0){
    fprintf(2, "ERROR IN LINE READING: %d\n", read_ln_res);
    exit(1);
  }
  printf("|%s|\n", buffer);
  char* ptr = buffer;
  
  int fst;
  int parse_res = parse_int(&ptr, &fst);
  if(parse_res < 0){
    fprintf(2, "ERROR IN PARSING INTEGER: %d\n", parse_res);
    exit(1);
  }


  int snd;
  parse_res = parse_int(&ptr, &snd);
  if(parse_res < 0){
    fprintf(2, "ERROR IN PARSING INTEGER: %d\n", parse_res);
    exit(1);
  }

  while(*ptr == ' '){
    ptr++;
  }
  if(*ptr != '\0'){
    fprintf(2, "ERROR: EXTRA CHARACTERS IN INPUT\n");
    exit(1);
  }

  printf("%d + %d = %d\n", fst, snd, add(fst, snd));
  exit(0);
}
  
  
