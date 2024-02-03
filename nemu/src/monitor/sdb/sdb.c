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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include<memory/paddr.h>
#include "sdb.h"


static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void test_expr();

/*the static value in watchpoint.h*/
#define NR_WP 32
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  word_t old_val;
  char expr[1024];

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

static WP* new_wp();
static void free_wp(WP *wp);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

/* function test_expr to test the function expr */
void test_expr(){
  FILE *fp = fopen("/home/aison/ics2023/nemu/tools/gen-expr/build/input","r");
  if(fp == NULL){
  printf("test_expr error\n");
  return;
  }
  char *e = NULL;
  word_t correct_res;
  size_t len = 0;
  ssize_t read;
  bool success = false;

  while(true){
  if(fscanf(fp,"%u",&correct_res) == -1)break;
  read = getline(&e,&len,fp);
  e[read - 1] = '\0';

  word_t res = expr(e,&success);

  if(!success){
  printf("failed to expr\n");
  //assert(0);
     }
  if(res != correct_res){
  puts(e);
  printf("expected: %u ,got : %u \n",correct_res,res);
  //assert(0);  
     }
  }

  fclose(fp);
  if(e) free(e);

  Log("expr test pass");

}

/*the implementation of sdb*/

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args){
  char *arg = strtok(NULL," ");
  int num = 1;
  if(!(args == NULL)){
 	num = atoi(arg);
  }
  cpu_exec(num);
  return 0;
}

void wp_iterate();

static int cmd_info(char *args){
 // char *arg = strtok(NULL," ");
  if(args == NULL){
  printf("no arguments\n");
  return 0;
  }
  if(strcmp(args,"r") == 0){
	  isa_reg_display();
  }else if(strcmp(args,"w") == 0)
	  wp_iterate();
  else
	  printf("Unkonwn command please retype info_command\n");
  return 0;
}

static int cmd_x(char *args){
  char *n = strtok(NULL," ");
  char *expr = strtok(NULL," ");
  int len;
  paddr_t addr;
  sscanf(n,"%d",&len);
  sscanf(expr,"%x",&addr);
  for(int i = 0;i < len;i++){
	printf("%#x\n",paddr_read(addr,4));
	addr =addr + 4;
  }
  return 0;
}

static int cmd_p(char *args){
  args = strtok(NULL,"\n");
  if(args == NULL){
	  printf("no EXPR\n");
	  return 0;
  }
  bool success = true;
  word_t ans;
  ans = expr(args,&success);
  if(success){
  printf("%u\n",ans);
  }
  else{
  printf("bad expression! please retype\n");
  }
  return 0;
}

void wp_watch(char *expr,word_t res);
void wp_remove(int no);

static int cmd_w(char *args){
  char *arg = strtok(NULL,"\n");
  if(arg == NULL){
  printf("Usage:w EXPR\n");
  return 0;
  }
  bool success;
  word_t res = expr(arg,&success);
  if(success){
  wp_watch(arg,res);
  }else puts("invalid expression\n");

  return 0;
}

static int cmd_d(char *args){
  char *arg = strtok(NULL," ");
  if(arg == NULL){
  printf("Usage: d N\n");
  return 0;
  }
  int NO = strtol(arg,NULL,10);
  wp_remove(NO);
  return 0;
}

void wp_watch(char *expr,word_t res){
  WP *wp = new_wp();
  strcpy(wp->expr,expr);
  wp->old_val = res;
  printf("Watchpoint %d: %s\n",wp->NO,expr);
}

void wp_remove(int no){
  assert(no < NR_WP);
  WP *wp = &wp_pool[no];
  free_wp(wp);
  printf("Delete watchpoint %d: %s\n",wp->NO,wp->expr);
}

void wp_iterate(){
  WP *h = head;
  if(h == NULL){
  printf("no WatchPoints\n");
  return ;
  }
  printf("%-8s%-8s\n","Num","EXPR");
  while(h != NULL){
  printf("%-8d%-8s\n",h->NO,h->expr);
  h = h->next;
  }
}

// watchpoint.c function

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

static WP* new_wp(){
  assert(free_);
  WP* ret = free_;
  free_ = free_ -> next;
  ret->next = head;
  head = ret;
  return ret;
}

static void free_wp(WP *wp){
  WP *h = head;
  if(h == wp)head = NULL;
  else {
  while(h && h->next != wp)h = h->next;
  assert(h);
  h->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

void wp_difftest(){
  WP *h = head;
  while(h){
  bool success;
  word_t new_val = expr(h->expr,&success);
  if(h->old_val != new_val){
  printf("WatchPoint %d: %s\n"
 	"old_val = %u\n"
        "new_val = %u\n",h->NO,h->expr,
	h->old_val,new_val);
      }
  h = h->next;
  }
}



static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","Type the number to execute $i instructions",cmd_si},
  {"info","Type [info r] to check the value of all registers",cmd_info},
  {"x","TYpe x N EXPR to check the value of menmory",cmd_x},
  {"p","Type p $EXPR to evaluate the value of your expression",cmd_p},
  {"w","Usage:w EXPR. Watch for the variation of the result of EXPR,pause at variation point",cmd_w},
  {"d","Usage:d N.Delete watchpoint of wp. ",cmd_d},
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  
  /* Initialize the expression test function*/
  //  test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
