#ifndef parsing_h
#define parsing_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpc.h"

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

enum { LERR_DB_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval {
	int type;
	long num;

	char* err;
	char* sym;

	int count;
	struct lval** cell;
} lval;

lval* lval_num(long);
lval* lval_sym(char*);
lval* lval_sexpr(void);
lval* lval_err(char*);

lval* lval_add(lval*, lval*);
lval* lval_read_num(mpc_ast_t*);
lval* lval_read(mpc_ast_t*);

char* readline(char*);

void add_history(char*);

void lval_del(lval*);
void lval_expr_print(lval*, char, char);
void lval_print(lval*);
void lval_println(lval*);

lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);
lval* builtin_op(lval*, char*);
lval* lval_eval(lval*);
lval* lval_eval_sexpr(lval*);

#endif