#include <errno.h>
#include <stdio.h>
#include <string.h>

#define ARGNUM  16
#define BUFSIZE 1024
#define PROMPT  "% "
#define DELIMS  " "

int cmd_cd(int argc, char **argv)
{
	printf("built-in: cd\n");
	return 0;
}

int cmd_exit(int argc, char **argv)
{
	printf("built-in: exit\n");
	return 0;
}

int cmd_default(int argc, char **argv)
{
	printf("built-in: !!NO!!\n");
	return 0;
}

typedef int (*command_t)(int argc, char **argv);
typedef struct builtin_s {
	char     *name;
	command_t func;
} builtin_t;

static builtin_t builtins[] = {
	{ "cd",   cmd_cd      },
	{ "exit", cmd_exit    },
	{ NULL,   cmd_default }
};

command_t builtin_get(builtin_t *dict, char *name)
{
	while (dict->name != NULL) {
		if (!strcmp(dict->name, name)) break;
		++dict;
	}
	return dict->func;
}

int strempty(char *str) {
	while (*str != '\0') {
		if (*str != ' ' && *str != '\n') return 0;
		str++;
	}
	return 1;
}

int main(int argc, char **argv)
{
	char line[BUFSIZE];
	char *tok_val[ARGNUM];
	int   tok_num = 0;

	for (;;) {
		char *tok;

		/* Prompt the user and wait for input. */
		printf("%s", PROMPT);
		if (fgets(line, BUFSIZE, stdin) == NULL) {
			printf("\n");
			return 0;
		}
		/* Ignore empty lines. */
		else if (strempty(line)) {
			continue;
		}

		/* Tokenize the line to find command-line parameters. */
		tok     = strtok(line, DELIMS);
		tok_num = 0;
		while (tok != NULL) {
			/* Don't overflow the static-sized buffer. */
			if (tok_num >= ARGNUM) {
				fprintf(stderr, "err: too many arguments\n");
				return 1;
			}

			tok_val[tok_num] = tok;
			tok = strtok(NULL, DELIMS);
			tok_num++;
		}

		/* Check if the command is a shell built-in. */
		builtin_get(builtins, tok_val[0])(tok_num, tok_val);
	}
}