/*
 * parser.h -- shell parser
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

#ifndef PARSER_H
#define PARSER_H


typedef struct job_s {
	char **tokens;
	int    stdin;
	int    stdout;
	int    stderr;
	int    prv_ret;
} job_t;


char **argv_gen(job_t *job, int argc, char **argv);
void   free_tokens(char **tokens);
char **tokenize(char *input);
int    parse_tokens(char **tokens);


#endif /* PARSER_H */
