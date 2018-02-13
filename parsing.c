#include "parsing.h"

static char buffer[2048];

lval* lval_num(long n) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = n;
	return v;
};

lval* lval_sym(char *s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
}

lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m)+1);
	strcpy(v->err, m);
	return v;
}

lval* lval_add(lval* x, lval* y) {
	x->count++;
	x->cell = realloc(x->cell, sizeof(lval*) * x->count);
	x->cell[x->count-1] = y;
	return x;
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
} 

lval* lval_read(mpc_ast_t* t) {
	if(strstr(t->tag, "number")) { return lval_read_num(t); }
	if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	lval* x = NULL;
	if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if(strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

	for(int i = 0; i < t->children_num; i++) {
		if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if(strcmp(t->children[i]->tag, "regex") == 0)  { continue; }

		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}
 
char* readline(char *prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

void add_history(char* unused) {}

void lval_del(lval* v) {
	switch(v->type) {
		case LVAL_NUM: break;

		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		case LVAL_SEXPR:
			for(int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}

			free(v->cell);
			break;
	}

	free(v);
}

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for(int i = 0; i < v->count; i++) {
		lval_print(v->cell[i]);

		if(i != v->count-1) {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) {
	switch(v->type) {
		case LVAL_NUM: printf("%li", v->num); break;
		case LVAL_ERR: printf("Error: %s.", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
	}
}

void lval_println(lval *v) {
	lval_print(v); putchar('\n');
}

lval* lval_pop(lval* v, int i) {
	printf("Made it lval_pop i = %i \n", i);
	lval* x = v->cell[i];
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

	v->count--;

	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	printf("Made it lval_take - i = %i \n", i);
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

lval* builtin_op(lval* v, char* op) {
	printf("Made it builtin_op \n");
	//Check that all args are numbers
	for(int i = 0; i < v->count; i++) {
		if(v->cell[i]->type != LVAL_NUM) {
			lval_del(v);
			return lval_err("Cannot operate on non-number");
		}
	}

	lval* x = lval_pop(v, 0);

	if(strcmp(op, "-") == 0 && v->count == 0) {
		x->num = -(x->num);
	}

	while(v->count > 0) {
		lval* y = lval_pop(v, 0);

		if(strcmp("*", op) == 0) { x->num *= y->num; }
		if(strcmp("-", op) == 0) { x->num -= y->num; }
		if(strcmp("+", op) == 0) { x->num += y->num; }
		if(strcmp("/", op) == 0) { 
			if(y->num == 0) {
				lval_del(x); lval_del(y);
				x = lval_err("Division by zero");
			}
			
			x->num /= y->num;
		}

		lval_del(y);
	}

	lval_del(v); return x;
}

lval* lval_eval(lval* v) {
	printf("Made it lval_eval \n");
	if(v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
	return v;
}

lval* lval_eval_sexpr(lval* v) {
	printf("Made it lval_sexpr \n");
	for(int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	for(int i = 0; i < v->count; i++) {
		if(v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
	}

	if(v->count == 0) { return v; }
	if(v->count == 1) { return lval_take(v, 0); }

	lval* first = lval_pop(v, 0);
	if(first->type != LVAL_SYM) {
		lval_del(first); lval_del(v);
		return lval_err("S-expression does not start with symbol"); 
	}

	lval* result = builtin_op(v, first->sym);
	lval_del(first);
	return result;
}

// lval eval_op(lval x, char* op, lval y) {
// 	if(x.type == LVAL_ERR) { return x; }
// 	if(y.type == LVAL_ERR) { return y; }

// 	if(strcmp("*", op) == 0) { return lval_num(x.num * y.num); }
// 	if(strcmp("-", op) == 0) { return lval_num(x.num - y.num); }
// 	if(strcmp("+", op) == 0) { return lval_num(x.num + y.num); }
// 	if(strcmp("%", op) == 0) { return lval_num(fmod(x.num, y.num)); }
// 	if(strcmp("/", op) == 0) { 
// 		return y.num == 0
// 			? lval_err(LERR_DB_ZERO)
// 			: lval_num(x.num / y.num);
// 	}

// 	return lval_err(LERR_BAD_OP);
// }

// lval eval(mpc_ast_t* t) {
// 	if(strstr(t->tag, "number")) {
// 		errno = 0;
// 		long x = atof(t->contents);
// 		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
// 	}

// 	char* op = t->children[1]->contents;

// 	lval x = eval(t->children[2]);

// 	for(int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
// 		x = eval_op(x, op, eval(t->children[i]));
// 	}

// 	return x;
// }

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lisp = mpc_new("lisp");

	mpca_lang(MPCA_LANG_DEFAULT,
	"                                          \
		number : /-?[0-9]+/ ;                    \
		symbol : '+' | '-' | '*' | '/' ;         \
		sexpr  : '(' <expr>* ')' ;               \
		expr   : <number> | <symbol> | <sexpr> ; \
		lisp   : /^/ <expr>* /$/ ;               \
	",
	Number, Symbol, Sexpr, Expr, Lisp);

	while(1) {
		char* input = readline("lisp> ");
		add_history(input);

		printf("You input %s\n", input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lisp, &r)) {
			/* Print AST on success */
			mpc_ast_print(r.output); putchar('\n');

			lval* result = lval_read(r.output);
			lval_println(result); putchar('\n');
			lval_println(lval_eval(result));

			//lval_del(result);

			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		free(input);
	}

	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lisp);
	return 0;
}



 
