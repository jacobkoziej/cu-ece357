/*
 * catgrepmore.c -- Jeff Hakner's "Swiss Army Knife"
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

#include "catgrepmore.h"

#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


static char buf[READSIZ];

static size_t bytes;
static size_t files;

static sigjmp_buf       jmp;
static struct sigaction sig;


static void sigusr1_handler(int sig)
{
	(void) sig;

	fprintf(stderr, "files: %lu, bytes: %lu\n", files, bytes);
}

static void next_file_handler(int sig_num)
{
	sig.sa_handler = SIG_DFL;

	if (sigaction(SIGUSR2, &sig, NULL) < 0) {
		perror("failed to reset SIGUSR2 signal");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGPIPE, &sig, NULL) < 0) {
		perror("failed to reset SIGPIPE signal");
		exit(EXIT_FAILURE);
	}

	siglongjmp(jmp, sig_num);
}


int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "%s: not enough arguments\n", argv[0]);
		fprintf(stderr, "usage: %s pattern [FILE]...\n", argv[0]);

		return EXIT_FAILURE;
	}

	sig.sa_handler = sigusr1_handler;
	sig.sa_flags   = SA_RESTART;

	if (sigaction(SIGUSR1, &sig, NULL) < 0) {
		perror("failed to setup signal handler for SIGUSR1");
		return EXIT_FAILURE;
	}

	for (int i = 2; i < argc; i++) {
		int fd;
		int pipefd[PIPE_CNT][2];

open_eintr:
		fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			if (errno == EINTR) goto open_eintr;

			perror(argv[i]);
			return EXIT_FAILURE;
		}

		if (pipe(pipefd[GREP_PIPE]) < 0) {
			perror("pipe(pipefd[GREP_PIPE]):");
			return EXIT_FAILURE;
		}

		if (pipe(pipefd[MORE_PIPE]) < 0) {
			perror("pipe(pipefd[MORE_PIPE]):");
			return EXIT_FAILURE;
		}

		switch (sigsetjmp(jmp, 1)) {
		case SIGUSR2:
			fprintf(
				stderr,
				"SIGUSR2 recieved, moving on from file #%lu\n",
				files
			);
			goto sigjmp;
			break;

		case SIGPIPE:
			fprintf(
				stderr,
				"SIGPIPE recieved, moving on from file #%lu\n",
				files
			);
			goto sigjmp;
			break;
		}

		sig.sa_handler = next_file_handler;

		if (sigaction(SIGUSR2, &sig, NULL) < 0) {
			perror("failed to setup signal handler for SIGUSR1");
			return EXIT_FAILURE;
		}
		if (sigaction(SIGPIPE, &sig, NULL) < 0) {
			perror("failed to setup signal handler for SIGPIPE");
			return EXIT_FAILURE;
		}

		pid_t grep_pid;
		pid_t more_pid;

		switch (grep_pid = fork()) {
		case -1:
			goto grep_child_error;

		case 0:
			if (dup2(pipefd[GREP_PIPE][0], STDIN_FILENO) < 0)
				goto grep_child_error;

			if (dup2(pipefd[MORE_PIPE][1], STDOUT_FILENO) < 0)
				goto grep_child_error;

			if (close(fd) < 0) goto grep_child_error;
			for (int i = 0; i < PIPE_CNT; i++)
				for (int j = 0; j < 2; j++)
					if (close(pipefd[i][j]) < 0)
						goto grep_child_error;

			execlp("grep", "grep", argv[1], NULL);

grep_child_error:
			perror("failed to fork grep child");
			return EXIT_FAILURE;
		}

		switch (more_pid = fork()) {
		case -1:
			goto more_child_error;

		case 0:
			if (dup2(pipefd[MORE_PIPE][0], STDIN_FILENO) < 0)
				goto more_child_error;

			if (close(fd) < 0) goto more_child_error;
			for (int i = 0; i < PIPE_CNT; i++)
				for (int j = 0; j < 2; j++)
					if (close(pipefd[i][j]) < 0)
						goto more_child_error;

			execlp("more", "more", NULL);

more_child_error:
			perror("failed to fork more child");
			return EXIT_FAILURE;
		}

		while (1) {
			ssize_t ret;
			size_t  readsiz;
			size_t  writesiz;

			for (readsiz = 0; readsiz < READSIZ; readsiz += ret) {
read_eintr:
				ret = read(fd, buf, READSIZ - readsiz);
				if (ret < 0) {
					if (errno == EINTR)
						goto read_eintr;

					perror("failed to read file");
					return EXIT_FAILURE;
				}

				if (!ret) break;
			}

			if (!readsiz) break;  // EOF

			for (
				writesiz = readsiz;
				writesiz;
				writesiz -= ret, bytes += ret
			) {
write_eintr:
				ret = write(
					pipefd[GREP_PIPE][1],
					buf,
					writesiz
				);
				if (ret < 0) {
					if (errno == EINTR)
						goto write_eintr;

					perror("failed to write to pipe");
					return EXIT_FAILURE;
				}

				if (!ret) break;
			}
		}

sigjmp:
		for (int i = 0; i < PIPE_CNT; i++)
			for (int j = 0; j < 2; j++)
pipe_close_eintr:
				if (close(pipefd[i][j]) < 0) {
					if (errno == EINTR)
						goto pipe_close_eintr;

					perror("failed to close pipes");
					return EXIT_FAILURE;
				}

		wait(NULL);
		wait(NULL);

		sig.sa_handler = SIG_DFL;

		if (sigaction(SIGUSR2, &sig, NULL) < 0) {
			perror("failed to reset SIGUSR2 signal");
			return EXIT_FAILURE;
		}
		if (sigaction(SIGPIPE, &sig, NULL) < 0) {
			perror("failed to reset SIGPIPE signal");
			return EXIT_FAILURE;
		}

close_eintr:
		if (close(fd) < 0) {
			if (errno == EINTR) goto close_eintr;

			perror("close()");
			return EXIT_FAILURE;
		}

		++files;
	}

	return EXIT_SUCCESS;
}
