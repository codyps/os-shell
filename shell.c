/* 198:416 - Assignment 2 - Shell
 * Authors: Cody Schafer, Michael Koval
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define ARGNUM  16
#define BUFSIZE 4096
#define DELIMS  " \t\n"
#define PROMPT  "% "
#define TIMEOUT 5

typedef int (command_t)(int argc, char *const *argv);
typedef struct builtin_s {
	char const *name;
	command_t  *func;
} builtin_t;

static pid_t pid;

static void sig_alarm(int sig)
{
	if (pid) {
		kill(pid, SIGKILL);
	}
}

static int cmd_cd(int argc, char *const *argv)
{
	char const *n = NULL;
	int x;
	if (argc < 2) {
		n = getenv("HOME");
		if (!n)
			n = "/";
	} else if (argc == 2) {
		n = argv[1];
	} else {
		fprintf(stderr, "err: too many arguments\n");
		return -1;
	}
	x = chdir(n);

	if (!x)
		return 0;

	fprintf(stderr, "%s: %s: %s\n", argv[0], strerror(errno), n);
	return -1;
}

static int cmd_exit(int argc, char *const *argv)
{
	if (argc < 2) {
		exit(0);
	} else {
		int ret;

		if (argc > 3) {
			fprintf(stderr, "err: too many arguments\n");
			return -1;
		}

		ret = 0;
		sscanf(argv[1], "%d", &ret);

		/* if sscanf fails, we exit with 0 */
		exit(ret);
	}
	return 0;
}

static int cmd_default(int argc, char *const *argv)
{
	pid = fork();
	if (pid == 0) {
		/* child */
		int x = execvp(argv[0], argv);
		if (x) {
			fprintf(stderr, "err: %s: %s.\n", argv[0], strerror(errno));
			fflush(stderr);
			exit(1);
		}
	} else {
		struct rusage stats;
		int status;

		/* kill the child when a timeout occurs */
		signal(SIGALRM, sig_alarm);
		alarm(TIMEOUT);

		for (;;) {
			int ret;

			errno = 0;
			ret   = wait4(pid, &status, WUNTRACED, &stats);
			alarm(0);

			/* signal interrupted the wait; try again */
			if (ret == -1 && errno == EINTR) {
				continue;
			} else if (ret == -1) {
				perror("wait4");
				return 1;
			} else {
				break;
			}
		}

		if (WIFEXITED(status)) {
			/* exited normally */
			printf("exit status = %d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			/* was killed by signal */
			printf("killed by signal %d\n", WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
			/* was stopped, not dead */
			printf("stopped by signal %d\n", WSTOPSIG(status));
		} else {
			printf("wonky status\n");
		}

		printf("user mode time = %ld.%06ld sec\n",
		       (long)stats.ru_utime.tv_sec, (long)stats.ru_utime.tv_usec);
		printf("kernel mode time = %ld.%06ld sec\n",
		       (long)stats.ru_stime.tv_sec, (long)stats.ru_stime.tv_usec);
	}
	return 0;
}

static const builtin_t builtins[] = {
	{ "exit", cmd_exit    },
	{ "cd",   cmd_cd      },
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

		/* Lookup commands in the built-in table. Fall back on exec. */
		if (tok_num > 0) {
			tok_val[tok_num] = NULL;
			builtin_get(builtins, tok_val[0])(tok_num, tok_val);
		}
	}
}
