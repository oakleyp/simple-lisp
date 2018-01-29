#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int main(int argc, char** argv) {
	while(1) {
		char* input = readline("lisp> ");
		add_history(input);

		printf("You input %s\n", input);
		free(input);
	}

	return 0;
}



 
