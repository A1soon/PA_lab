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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, 
  TK_AND ,
  TK_OR ,
  TK_EQ ,
  TK_NEQ ,
  TK_LEQ ,
  TK_GEQ ,
  TK_DREFER ,
  TK_NUM ,
  TK_REGISTER ,
  TK_HEX ,
  TK_LEFT_PAR ,
  TK_RIGHT_PAR ,

  /* TODO: Add more token types */

};

enum priority{
  LOGIC = 1,
  LEQ_GEQ ,
  NEQ_EQ ,
  PLUS_SUB ,
  MULTI_DIV ,
  DREFER,
  OTHER, 
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
 // {"==", TK_EQ,		//equal
  {"\\-",'-'},          //sub
  {"\\*",'*'},          //mul 
  {"/",'/'},     	//div
			
  {"\\(",TK_LEFT_PAR},//left parenthese
  {"\\)",TK_RIGHT_PAR}, //right parenthe
			
  {"\\!\\=",TK_NEQ},	 // != not equal
  {"==",TK_EQ},	 // == equal
 
  {"\\!",'!'},		 //operator
  {"\\&\\&",TK_AND},	 //operator && and 
  {"\\|\\|",TK_OR},	 //operator || or
	
  {"[0-9]*",TK_NUM},		 //num
  {"0[xX][0-9a-fA-F]+",TK_HEX},  //hex 0x...
  {"\\$[a-zA-Z]*[0-9]",TK_REGISTER},  //register $register
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

	Token tmp_token;
        switch (rules[i].token_type) {
        	case '+':
			tmp_token.type = '+';
			tokens[nr_token ++] = tmp_token;
			break;
		case '-':
			tmp_token.type = '-';
			tokens[nr_token ++] = tmp_token;
			break;
		case '*':
			// dereference type	
			if(nr_token == 0 || 
			(tokens[nr_token - 1].type != TK_NUM && tokens[nr_token - 1].type != TK_HEX
			  && tokens[nr_token - 1].type != ')' 
			  && tokens[nr_token - 1].type != TK_REGISTER)){
				tokens[nr_token].type = TK_DREFER;
				strncpy(tokens[nr_token].str,substr_start,substr_len);
				tokens[nr_token].str[substr_len] = '\0';
				nr_token ++;
				break;
			}
			else {
	        		tmp_token.type ='*';
				tokens[nr_token ++] = tmp_token;
				break;
			}
		case '/':
			tmp_token.type = '/';
			tokens[nr_token ++] = tmp_token;
			break;
		case 256:
			break;
		case TK_EQ:		//equal
			tokens[nr_token].type = TK_EQ;
			strcpy(tokens[nr_token].str,"==");
			nr_token ++;
			break;
		case TK_NUM:		//num
			tokens[nr_token].type = TK_NUM;
			strncpy(tokens[nr_token].str,substr_start,substr_len);
			tokens[nr_token].str[substr_len] = '\0';
			nr_token++;
			break;
		case TK_REGISTER:		//register
			tokens[nr_token].type = TK_REGISTER;
			strncpy(tokens[nr_token].str,substr_start,substr_len);
			tokens[nr_token].str[substr_len] = '\0';
			nr_token ++;
			break;
		case TK_NEQ:		//not equal
			tokens[nr_token].type = TK_NEQ;
			strcpy(tokens[nr_token].str,"!=");
			nr_token ++;
			break;
		case TK_HEX:		//HEX
			tokens[nr_token].type = TK_HEX;
			strncpy(tokens[nr_token].str,substr_start,substr_len);
			tokens[nr_token].str[substr_len] = '\0';
			nr_token ++;
			break;
		case TK_AND:		// && and
			tokens[nr_token].type = TK_AND;
			strcpy(tokens[nr_token].str,"&&");
			nr_token ++;
			break;
		case TK_OR:		// || or
			tokens[nr_token].type = TK_OR;
			strcpy(tokens[nr_token].str,"||");
			nr_token ++;
			break;
		case TK_LEFT_PAR:		// ( 
			tmp_token.type = '(';
			tokens[nr_token ++] = tmp_token;
			break;
		case TK_RIGHT_PAR:
			tmp_token.type = ')';
			tokens[nr_token ++] = tmp_token;
			break;
		case TK_LEQ:
			tokens[nr_token].type = TK_LEQ;
		        strcpy(tokens[nr_token].str,"<=");
			nr_token ++;
			break;
		case TK_GEQ:
			tokens[nr_token].type = TK_GEQ;
			strcpy(tokens[nr_token].str,">=");
			nr_token ++;
			break;
		default:
			printf("error! no regular expressions matches i = %d\n",i);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool bad_expr = false;

static int priority_of_op(int type){
	switch(type){
		case TK_AND:
		case TK_OR:
			return LOGIC;
		case TK_LEQ:
		case TK_GEQ:
			return LEQ_GEQ;
		case TK_EQ:
		case TK_NEQ:
			return NEQ_EQ;
		case '+':
		case '-':
			return PLUS_SUB;
		case '*':
		case '/':
			return MULTI_DIV;
		case TK_DREFER:
			return DREFER;
		default :
			return OTHER;
	}
}



bool check_parentheses(int left,int right){
	if(tokens[left].type !='(' || tokens[right].type != ')')
		return false;

	int l = left,r = right;
	int num = 0;
	while(l < r){
		if(tokens[l].type == '('){
			num ++;
			if(tokens[r].type == ')'){
				l++;
				r--;
				num --;
				continue;
			}
			else r--;

		} 
		else if(tokens[l].type == ')'){
			bad_expr = true;
			printf("no matched parentheses\n");
			return false;
		}

		else l++; 
	}
	
	if(num != 0){
		bad_expr = true;
		printf("no matched parentheses\n");
		return false;
	}
	return true;
}

static int primary_operator(int p,int q){
	int position = p,prior = OTHER;
	for(;p <= q;p++){
		int type = tokens[p].type;
		int cur_prior = priority_of_op(type);

		if(type == '('){
			while(tokens[p].type !=')' ) p++;
		}

		if(cur_prior <= prior){
			position = p;
			prior = cur_prior;
		}
	}
	return position;
}

static word_t str2int(char *s){
	int base = 10;
	if(s[0] == '$'){
		bool success;
		word_t ans;
		ans = isa_reg_str2val(s + 1,&success);
		if(success) return ans;
		else {
		printf("register error \n");
		return ans;
		}

	}

	if(strlen(s) < 2){
		base = 10;
	}
	else{
		base  = s[1] == 'x' ? 16 : 10 ;
	}

	return strtol(s,NULL,base);
}

word_t eval(int p,int q){
  if(bad_expr) return 0;
  if(p > q){
	  //bad expression
	  bad_expr = true;
	  printf("bad expression\n");
	  return 0;
  }
  else if(p == q){
	  return str2int(tokens[p].str);
  }
  else if(check_parentheses(p,q)){
	  return eval(p + 1,q - 1);
  }
  else {

	int position = primary_operator(p,q);
	int op_type = tokens[position].type;
	word_t val1 = eval(p,position - 1);
	word_t val2 = eval(position + 1,q);

	switch(op_type){
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			if(val2 == 0){
				printf("error:A / 0\n");
				return 0;
			}
			return val1 / val2;
		case TK_AND :
			return val1 && val2;
		case TK_OR :
			return val1 || val2;
		case TK_EQ :
			return val1 == val2;
		case TK_NEQ :
			return val1 != val2;
		case TK_LEQ :
			return val1 <= val2;
		case TK_GEQ :
			return val1 >= val2;
		default:
			printf("no operation type\n");
			return 0;	
	}
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();
  bad_expr = false;
  int beg_str = 0;
  int end_str = nr_token - 1;
  word_t ans = eval(beg_str,end_str);
  nr_token = 0;
  if(bad_expr){
  *success = false;
  return 0;
  }
  *success = true;
  return ans;
}


