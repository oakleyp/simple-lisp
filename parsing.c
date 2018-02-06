#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpc.h"

static char buffer[2048];

typedef struct {
	int type;
	long num;
	int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };

enum { LERR_DB_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long n) {
	lval v;
	v.type = LVAL_NUM;
	v.num = n;
	return v;
};

lval lval_err(int n) {
	lval v;
	v.type = LVAL_ERR;
	v.err = n;
	return v;
};
 
char* readline(char *prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

void add_history(char* unused) {}

void lval_print(lval v) {
	switch(v.type) {
		case LVAL_NUM:
			printf("%li", v.num); 
			break;
		case LVAL_ERR:
			if(v.err == LERR_DB_ZERO) {
				printf("Error: Division by 0");
			} else if (v.err == LERR_BAD_OP) {
				printf("Error: Invalid operator in expression");
			} else if (v.err == LERR_BAD_NUM) {
				printf("Error: Invalid number in expression");
			}
			break;
	}
}



long eval_op(long x, char* op, long y) {
	if(strcmp("*", op) == 0) { return x*y; }
	if(strcmp("/", op) == 0) { return x/y; }
	if(strcmp("-", op) == 0) { return x-y; }
	if(strcmp("+", op) == 0) { return x+y; }
	if(strcmp("%", op) == 0) { return x%y; }
	return 0;
}

long eval(mpc_ast_t* t) {
	if(strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	char* op = t->children[1]->contents;

	long x = eval(t->children[2]);

	for(int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
		x = eval_op(x, op, eval(t->children[i]));
	}

	return x;
}

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lisp = mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
	"                                                       \
		number   : /-?[0-9]+/ ;                             \
		operator : '+' | '-' | '*' | '/' ;                  \
		expr     : <number> | '(' <operator> <expr>+ ')' ;  \
		lisp    : /^/ <operator> <expr>+ /$/ ;              \
	",
	Number, Operator, Expr, Lisp);

	while(1) {
		char* input = readline("lisp> ");
		add_history(input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lisp, &r)) {
			/* Print AST on success */
			mpc_ast_print(r.output);

			long result = eval(r.output);
			printf("RESULT: %li\n", result);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		printf("You input %s\n", input);
		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expr, Lisp);
	return 0;
}



 
