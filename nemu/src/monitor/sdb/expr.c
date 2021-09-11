#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, NUM
  // TK_PLUS, TK_SUB, TK_MUL, TK_DIV,
  // TK_LP, TK_RP
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {"[0-9]+",NUM},         // 数字
  {"\\(", '('},         // 左括号
  {"\\)", ')'},         // 右括号
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // sub
  {"\\*", '*'},         // mul
  {"\\/", '/'},         // divide
  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
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
        
        if (substr_len > 32){
          assert(0);
        }

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
        
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case NUM:
            memcpy(tokens[nr_token].str, e + position, (substr_len) * sizeof(char));
            tokens[nr_token].str[substr_len] = '\0';
            // IFDEF(CONFIG_DEBUG, Log("[DEBUG ]读入了一个数字%s", tokens[nr_token].str));
          default: 
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
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

int eval(int p, int q, bool *success, int *position);

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  int position = 0;
  int ans = eval(0, nr_token - 1, success, &position);
  if (!success){
    printf("some problem happens at position %d\n%s\n%*.s^\n", position, e, position, "");
  }
  return ans;
}

#define STACK_SIZE 32
bool check_parentheses(int p, int q, int *position){
  //char *stack = calloc(STACK_SIZE, sizeof(char));
  char stack[STACK_SIZE];
  *position = -1;
  int top = -1, index = p;
  while (index <= q){
    if (tokens[index].type == '('){
      stack[++top] = '(';
    }else if (tokens[index].type == ')'){
      if (top < 0 || stack[top] != ')'){
        *position = p;
        return false;
      }else {
        top--;
      }
    }
    index++;
  }
  if (top != -1){ //栈空
    *position = p;
    return false;
  }
  return tokens[p].type == '(';
}

int prio(char type){
  switch (type)
  {
  case '+':
  case '-':
    return 1;
  
  case '*':
  case '/':
    return 2;

  default:
    return -1;
  }
}

int eval(int p, int q, bool *success, int *position) {
  if (p > q) {
    *success = false;
    return 0;
  } else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    int buffer = 0;
    sscanf(tokens[p].str, "%d", &buffer);
    IFDEF(CONFIG_DEBUG, Log("读取数据 %d", buffer));
    return buffer;
  }
  else if (check_parentheses(p, q, position) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success, position);
  } else {
    if (*position != -1){
      *success = false;
      return 0;
    }
    int op = -1;
    for (int i = p; i <= q; ++i){
      if (prio(tokens[i].type) >= 0){//说明是运算符
        if (op == -1 || prio(tokens[i].type) <= prio(op)){
          op = i;
        }
      }
    }

    int val1 = eval(p, op - 1, success, position);
    int val2 = eval(op + 1, q, success, position);
    IFDEF(CONFIG_DEBUG, Log("主运算符 %c", tokens[op].type));
    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: assert(0);
    }
  }
  return 0;
}