/*
 * jksh.c -- Jacob Koziej's basic shell
 * Copyright (C) 2022  Jacob Koziej <jacobkoziej@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtin.h"
#include "parser.h"


#define DEFAULT_HOMEDIR "/"


static char   *homedir;
static char   *input;
static int     prv_ret;
static char   *ps1;
static char  **tokens;
static job_t   job;


static void child(int argc, char **argv)
{
	job.tokens  = tokens,
	job.stdin   = -1;
	job.stdout  = -1;
	job.stderr  = -1;
	job.prv_ret = prv_ret;

	char **args = argv_gen(&job, argc, argv);
	if (!args) exit(127);

	if (job.stdin > -1) {
		if (dup2(job.stdin, STDIN_FILENO) < 0) {
			perror("can't dup2() to stdin");
			goto error;
		}

		if (close(job.stdin) < 0) {
			perror("can't close existing stdin file descriptor");
			goto error;
		}
	}

	if (job.stdout > -1) {
		if (dup2(job.stdout, STDOUT_FILENO) < 0) {
			perror("can't dup2() to stdout");
			goto error;
		}

		if (close(job.stdout) < 0) {
			perror("can't close existing stdout file descriptor");
			goto error;
		}
	}

	if (job.stderr > -1) {
		if (dup2(job.stderr, STDERR_FILENO) < 0) {
			perror("can't dup2() to stderr");
			goto error;
		}

		if (close(job.stderr) < 0) {
			perror("can't close existing stderr file descriptor");
			goto error;
		}
	}

	execvp(args[0], args);

	perror(args[0]);

error:
	for (char **tmp = args; *tmp; tmp++)
		free(*tmp);

	free(args);

	exit(127);
}

static void cleanup(void)
{
	if (homedir) free(homedir);
	if (input)   free(input);
	if (ps1)     free(ps1);
	if (tokens)  free_tokens(tokens);
}

int main(int argc, char **argv)
{
	atexit(cleanup);

	char *tmp;

	// prep PS1
	if ((tmp = getenv("PS1")) && !(ps1 = strdup(tmp))) {
		perror("couldn't allocate buffer for `PS1`");
		return EXIT_FAILURE;
	}
	if (!ps1 && !(ps1 = strdup(""))) {
		perror("couldn't allocate buffer for `PS1`");
		return EXIT_FAILURE;
	}

	// set home directory
	if ((tmp = getenv("HOME")) && !(homedir = strdup(tmp))) {
		perror("couldn't allocate buffer for `HOME`");
		return EXIT_FAILURE;
	}
	if (!homedir && !(homedir = strdup(DEFAULT_HOMEDIR))) {
		perror("couldn't allocate buffer for `HOME`");
		return EXIT_FAILURE;
	}

	while (1) {
		// update PS1
		tmp = getenv("PS1");
		if (tmp && strcmp(ps1, tmp)) {
			free(ps1);
			ps1 = strdup(tmp);
			if (!ps1) return EXIT_FAILURE;
		}

		printf("%s", ps1);

		size_t inputsiz;

		if (errno = 0, getline(&input, &inputsiz, stdin) < 0) {
			// EOF reached
			if (!errno) break;

			perror("failed to read shell input");
			return EXIT_FAILURE;
		}

		// remove trailing newline (if found)
		size_t end = strlen(input) - 1;
		if (input[end] == '\n') input[end] = '\0';

		tokens = tokenize(input);
		if (!tokens) {
			perror("failed to tokenize shell input");
			return EXIT_FAILURE;
		}

		if (!*tokens || **tokens == '#') goto done;

		if (!strcmp("cd", *tokens)) {
			prv_ret = cd(tokens + 1, homedir);
			goto done;
		}
		if (!strcmp("exit", *tokens)) {
			shexit(tokens + 1);
			goto done;
		}
		if (!strcmp("export", *tokens)) {
			export(tokens + 1);
			goto done;
		}
		if (!strcmp("pwd", *tokens)) {
			prv_ret = pwd(tokens + 1);
			goto done;
		}

		pid_t pid;

		switch (pid = fork()) {
			case -1:
				perror("couldn't fork");
				goto done;

			case 0:
				child(argc, argv);
				break;
		}

		int status;
		waitpid(pid, &status, 0);
		prv_ret = WEXITSTATUS(status);

done:
		free(input);
		free_tokens(tokens);

		input  = NULL;
		tokens = NULL;
	}

	return EXIT_SUCCESS;
}
