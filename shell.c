/* 198:416 - Assignment 2 - Shell
 * Authors: Cody Schafer, Michael Koval
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define ARGNUM  16
#define BUFSIZE 1024
#define PROMPT  "% "
#define DELIMS  " \t\n"

static int cmd_pwd(int argc, char *const *argv)
{
	char *p = getcwd(NULL, 0);
	puts(p);
	free(p);
	return 0;
}

static int cmd_cd(int argc, char *const *argv)
{
	char const *n = NULL;
	int x;
	if (argc < 2) {
		n = getenv("HOME");
		if (!n)
			n = "/";
	} else {
		n = argv[1];
	}
	x = chdir(n);

	if (!x)
		return 0;

	fprintf(stderr, "%s: %s: %s\n", argv[0],
			strerror(errno), n);

	return -1;
}

static int cmd_exit(int argc, char *const *argv)
{
	if (argc < 2) {
		exit(0);
	} else {
		int ret = 0;
		sscanf(argv[1], "%d", &ret);

		/* if sscanf fails, we exit with 0 */
		exit(ret);
	}
	return 0;
}

static int cmd_default(int argc, char *const *argv)
{
	pid_t p = fork();
	if (p == 0) {
		/* child */
		int x = execvp(argv[0], argv);
		if (x) {
			fprintf(stderr, "execvp fail.");
			return -1;
		}
	} else {
		int status;
		pid_t p2 = wait4(p, &status, 1, NULL);
		printf("wait done. %d\n", (int)p2);
	}

	return 0;
}

typedef int (command_t)(int argc, char *const *argv);
typedef struct builtin_s {
	char const *name;
	command_t  *func;
} builtin_t;

static const builtin_t builtins[] = {
	{ "exit", cmd_exit    },
	{ "cd",   cmd_cd      },
	{ "pwd",  cmd_pwd     },
	{ NULL,   cmd_default }
};

static command_t *builtin_get(builtin_t const *dict, char const *name)
{
	while (dict->name != NULL) {
		if (!strcmp(dict->name, name))
			break;
		++dict;
	}
	return dict->func;
}

static int strempty(char const *str) {
	while (*str != '\0') {
		if (*str != ' ' && *str != '\n')
			return 0;
		str++;
	}
	return 1;
}

int main(int argc, char **argv)
{
	char line[BUFSIZE];

	/* extra space for NULL */
	char *tok_val[ARGNUM + 1];
	int   tok_num = 0;

	for (;;) {
		char *tok;

next_command:
		/* Prompt the user and wait for input. */
		printf("%s", PROMPT);
		if (fgets(line, BUFSIZE, stdin) == NULL) {
			printf("\n");
			return 0;
		} else if (strempty(line)) {
			/* Ignore empty lines. */
			continue;
		}

		/* Tokenize the line to find command-line parameters. */
		tok     = strtok(line, DELIMS);
		tok_num = 0;
		while (tok != NULL) {
			/* Don't overflow the static-sized buffer. */
			if (tok_num >= ARGNUM) {
				fprintf(stderr, "err: too many arguments\n");
				goto next_command;
			}

			tok_val[tok_num] = tok;
			tok = strtok(NULL, DELIMS);
			tok_num++;
		}

		/* make execvp happy. */
		tok_val[tok_num] = NULL;

		/* Check if the command is a shell built-in. */
		{
			command_t *func = builtin_get(builtins, tok_val[0]);

			char *const *tmp1 = tok_val;
			func(tok_num, tmp1);
		}
	}
}
