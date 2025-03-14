/*
 * Copyright (c) 2021 Matthias Schmidt <xhr@giessen.ccc.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "isscrolls.h"

static char prompt[MAX_PROMPT_LEN];
static char isscrolls_dir[_POSIX_PATH_MAX];

static int debug = 0;
static int color = 0;
static int cursed = 0;
static int banner = 1;
static int output = 1;

static volatile sig_atomic_t sflag = 0;

FILE *journal_file = NULL;

static void
signal_handler(int signal)
{
	switch (signal) {
		case SIGINT:
		case SIGTERM:
			sflag = 1;
			break;
	}
}

void
show_banner(__attribute__((unused)) char *unused)
{
	pm(GREEN, "\n██▓  ██████   ██████  ▄████▄   ██▀███   ▒█████   ██▓     ██▓      ██████\n");
	pm(GREEN, "▓██▒▒██    ▒ ▒██    ▒ ▒██▀ ▀█  ▓██ ▒ ██▒▒██▒  ██▒▓██▒    ▓██▒    ▒██    ▒\n");
	pm(GREEN, "▒██▒░ ▓██▄   ░ ▓██▄   ▒▓█    ▄ ▓██ ░▄█ ▒▒██░  ██▒▒██░    ▒██░    ░ ▓██▄\n");
	pm(GREEN, "░██░  ▒   ██▒  ▒   ██▒▒▓▓▄ ▄██▒▒██▀▀█▄  ▒██   ██░▒██░    ▒██░      ▒   ██▒\n");
	pm(GREEN, "░██░▒██████▒▒▒██████▒▒▒ ▓███▀ ░░██▓ ▒██▒░ ████▓▒░░██████▒░██████▒▒██████▒▒\n");
	pm(GREEN, "░▓  ▒ ▒▓▒ ▒ ░▒ ▒▓▒ ▒ ░░ ░▒ ▒  ░░ ▒▓ ░▒▓░░ ▒░▒░▒░ ░ ▒░▓  ░░ ▒░▓  ░▒ ▒▓▒ ▒ ░\n");
	pm(GREEN, " ▒ ░░ ░▒  ░ ░░ ░▒  ░ ░  ░  ▒     ░▒ ░ ▒░  ░ ▒ ▒░ ░ ░ ▒  ░░ ░ ▒  ░░ ░▒  ░ ░\n");
	pm(GREEN, " ▒ ░░  ░  ░  ░  ░  ░  ░          ░░   ░ ░ ░ ░ ▒    ░ ░     ░ ░   ░  ░  ░\n");
	pm(GREEN, " ░        ░        ░  ░ ░         ░         ░ ░      ░  ░    ░  ░      ░\n");
	pm(GREEN, "                      ░\n");
	printf("                                                            Version %s\n\n", VERSION);

	printf("\tPlayer toolkit for the Ironsworn RPG family\n");
	printf("\tBy Matthias Schmidt - https://cybervillains.com/@_xhr_\n\n");
	printf("Enter 'help' for available commands\n\n");
}

int
main(int argc, char **argv)
{
	char *line, *res;
	int ch;

	/*
	 * Seed the PRNG with the current time of the day.  This is not fine
	 * for a security critical application, for rolling dice it is
	 */
	srandom(time(NULL) ^ getpid());

	while ((ch = getopt(argc, argv, "cdbx")) != -1) {
		switch (ch) {
		case 'b':
			banner = 0;
			break;
		case 'c':
			color = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'x':
			cursed = 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	setup_base_dir();

	initialize_readline(isscrolls_dir);

	if (banner)
		show_banner(NULL);

	if (signal(SIGINT, signal_handler) == SIG_ERR)
		log_errx(1, "signal");
	if (signal(SIGTERM, signal_handler) == SIG_ERR)
		log_errx(1, "signal");

	sandbox(isscrolls_dir);

	if (load_characters_list() == -1)
		set_prompt("> ");

	while (!sflag) {
		line = readline(prompt);
		if (line == NULL)
			continue;
		res = stripwhite(line);

		if (*res) {
			add_history(res);
			execute_command(res);
		}

		free(line);
	}

	initiate_shutdown(0);

	return 0;
}

void
set_prompt(const char *p)
{
	if (p == NULL || strlen(p) == 0)
		return;

	if (color)
		snprintf(prompt, sizeof(prompt), "%s%s%s", ANSI_COLOR_BOLD, p, ANSI_COLOR_RESET);
	else
		snprintf(prompt, sizeof(prompt), "%s", p);
}

#ifdef __OpenBSD__
void
sandbox(const char *dir)
{
	if (unveil(PATH_SHARE_DIR, "r") == -1)
		log_errx(1, "unveil");
	if (unveil(dir, "rwc") == -1)
		log_errx(1, "unveil");
	if (unveil(NULL, NULL) == -1)
		log_errx(1, "unveil");

	if (pledge("stdio rpath wpath cpath tty", NULL) == -1)
		log_errx(1, "pledge");
}
#else
void sandbox(__attribute__((unused)) const char *dir)
{
}
#endif /* __OpenBSD__ */

void
cmd_quit(__attribute__((unused)) char *unused)
{
	initiate_shutdown(0);
}

void
initiate_shutdown(int exit_code)
{
	char hist_path[_POSIX_PATH_MAX];
	int ret;

	save_current_character();

	ret = snprintf(hist_path, sizeof(hist_path), "%s/history", isscrolls_dir);
	if (ret < 0 || (size_t)ret >= sizeof(hist_path)) {
		printf("Path truncation happened.  Buffer too short to fit %s\n", hist_path);
	}

	log_debug("Writing history to %s\n", hist_path);
	write_history(hist_path);

	close_journal_file();

	exit(exit_code);
}

void
setup_base_dir(void)
{
	struct stat sb;
	char *home, *xdg_home;
	int ret;

	if ((xdg_home = getenv("XDG_CONFIG_HOME")) != NULL) {
		ret = snprintf(isscrolls_dir, sizeof(isscrolls_dir), "%s/isscrolls", xdg_home);
		if (ret < 0 || (size_t)ret >= sizeof(isscrolls_dir)) {
			log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", isscrolls_dir);
		}
	} else if ((home = getenv("HOME")) != NULL) {
		ret = snprintf(isscrolls_dir, sizeof(isscrolls_dir), "%s/.config/isscrolls", home);
		if (ret < 0 || (size_t)ret >= sizeof(isscrolls_dir)) {
			log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", isscrolls_dir);
		}
	} else {
		log_errx(1, "Neither $XDG_CONFIG_HOME nor $HOME is set!\n");
	}

	if (stat(isscrolls_dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		log_debug("%s already exists\n", isscrolls_dir);
	} else {
		log_debug("%s does not exists.  Attempt to create it\n", isscrolls_dir);
		if (mkdir(isscrolls_dir, 0755) == -1) {
			log_errx(1, "Cannot create %s directory\n", isscrolls_dir);
		}
	}
}

void
log_debug(const char *fmt, ...)
{
	va_list ap;

	if (debug == 0)
		return;

	va_start(ap, fmt);
	fprintf(stdout, "[*] ");
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void
log_errx(int prio, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	initiate_shutdown(prio);
}

void
pm(int what, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (color) {
		switch (what) {
		case RED:
			fprintf(stdout, ANSI_COLOR_RED);
			break;
		case YELLOW:
			fprintf(stdout, ANSI_COLOR_YELLOW);
			break;
		case GREEN:
			fprintf(stdout, ANSI_COLOR_GREEN);
			break;
		case BLUE:
			fprintf(stdout, ANSI_COLOR_CYAN);
			break;
		default:
			break;
		}
	}

	vfprintf(stdout, fmt, ap);
	if (color)
		fprintf(stdout, ANSI_COLOR_RESET);
	va_end(ap);
}

const char*
get_isscrolls_dir(void)
{
	return isscrolls_dir;
}

void
toggle_output(void)
{
	output = !output;
}

int
get_output(void)
{
	return output;
}

int
get_color(void)
{
	return color;
}

int
get_cursed(void)
{
	return cursed;
}


static void
print_uncolored(FILE* out_file, char const * const in_string)
{
	if (out_file == NULL) {
		log_errx(1, "attempt to write to closed journal file");
		return;
	}
	fprintf(out_file, "%s\n", in_string);
}

void
write_journal_entry(char const * const what)
{
	char path[_POSIX_PATH_MAX];
	time_t t;
	struct tm *tm_ptr;
	if (what[0] == '\0')
		return;
	if (journal_file == NULL) {
		journal_file_name(path);
		journal_file = fopen(path, "a");
		if (journal_file == NULL) {
			printf("Could not open journal file (%s): %s\n", path, strerror(errno));
			return;
		}
	}
	t = time(NULL);
	tm_ptr = localtime(&t);
	if (tm_ptr == NULL) {
		log_errx(1, "localtime returned null");
		return;
	}
	fprintf(journal_file, "[%d-%02d-%02d %02d:%02d:%02d] ", tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday, tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
	print_uncolored(journal_file, what);
}

void
close_journal_file(void)
{
	if (journal_file != NULL) {
		fclose(journal_file);
		journal_file = NULL;
	}
}

