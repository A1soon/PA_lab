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

#include "sdb.h"
/*
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}
*/
/* TODO: Implement the functionality of watchpoint 

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

*/


















