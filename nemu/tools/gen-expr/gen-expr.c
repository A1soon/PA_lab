/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static char* operations[] = {
  "+",
  "-",
  "*",
  "/",
};

#define NR_OPS sizeof(operations) / sizeof(char*)

uint32_t choose(uint32_t n){
  return rand() % n;
}

char *p = NULL;
int size = 5 ;
static void do_gen_rand_expr(int depth){
  int options = depth > size ? 0 : choose(3);
  switch(options){
    case 0 :
	    uint32_t num = rand() % 100 ;
	    char strnum[32] = {'\0'};
	    sprintf(strnum,"%d",num);
	    strcpy(p,strnum);
	    p += strlen(strnum);
	    break;
    case 1 :
	    strcpy(p,"(");
	    p += strlen("(");
	    do_gen_rand_expr(depth + 1);
	    strcpy(p,")");
	    p += strlen(")");
	    break;
/*    case  2:
	    strcpy(p," ");
	    p += strlen(" ");
	    break;*/
    default :
	    do_gen_rand_expr(depth + 1);
	    char *op = operations[choose(NR_OPS)];
	    strcpy(p,op);
	    p += strlen(op);
	    do_gen_rand_expr(depth + 1);
	    break;
  }
}

static void gen_rand_expr() {
  p = buf;
  do_gen_rand_expr(0);
  p[0] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
