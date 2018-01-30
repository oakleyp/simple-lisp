#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpc.h"

static char buffer[2048];
 
char* readline(char *prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

void add_history(char* unused) {}

long eval(mpc_ast_t* t) {
	if(strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	char* op = t->children[1].contents;

	long x = eval(t->children[2]);

	for(int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
		x = eval_op(x, op, eval(t->children[i]));
	}

	return x;
}

long eval_op(long x, char* op, long y) {
	long result;
	
	switch(op) {
		case '*': 
			result = x*y;
			break;
		case "/":
			result = x/y;
			break;
		case "+":
			result = x+y;
			break;
		case "-":
			result = x-y;
			break;
	}

	return result;
}

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lisp = mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
	"                                                     \
		number   : /-?[0-9]+/ ;                             \
		operator : '+' | '-' | '*' | '/' ;                  \
		expr     : <number> | '(' <operator> <expr>+ ')' ;  \
		lisp    : /^/ <operator> <expr>+ /$/ ;             \
	",
	Number, Operator, Expr, Lisp);

	while(1) {
		char* input = readline("lisp> ");
		add_history(input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lisp, &r)) {
			/* Print AST on success */
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);

			long result = eval(r.output);
			printf("RESULT: %li\n", result);
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



 
