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

#include <limits.h>
#include <regex.h>
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

char message_buffer[BUFFER_LENGTH] = "";
char *message_buffer_pos;
int buffer_chars_left = BUFFER_LENGTH;

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
	clear_message_buffer();
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
	print_and_journal(message_buffer);
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
		// log_debug("setting color to %d\n", what);
		switch (what) {
		case RED:
			add_to_buffer("%s", ANSI_COLOR_RED);
			break;
		case YELLOW:
			add_to_buffer("%s", ANSI_COLOR_YELLOW);
			break;
		case GREEN:
			add_to_buffer("%s", ANSI_COLOR_GREEN);
			break;
		case BLUE:
			add_to_buffer("%s", ANSI_COLOR_CYAN);
			break;
		default:
			break;
		}
	}
	// log_debug("writing string, format '%s'\n", fmt);
	add_to_buffer(fmt, ap);
	if (color) {
		// log_debug("clearing color\n");
		add_to_buffer("%s", ANSI_COLOR_RESET);
	}
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

void
clear_message_buffer(void) {
	message_buffer_pos = &message_buffer[0];
	buffer_chars_left = BUFFER_LENGTH;
	message_buffer[0] = '\0';
}

void 
add_to_buffer(const char *format, ...) {	
	va_list args;
	int chars_written;	
	log_debug("writing to buffer, %d chars left, format is '%s'\n", buffer_chars_left, format);
	// va_start(args, format);  
    // for (int i = 0; i < 1; i++) {
	// 	char *p = va_arg(args, char *);
	// 	while (*p) {log_debug("%c=%x", *p >= 32 ? *p : ' ', *p); p++;}
	// 	log_debug(" done\n");
	// }
    // va_end(args);
	va_start(args, format); 
	// log_debug("to buffer\n");
    chars_written = vsprintf(message_buffer_pos, format, args);
	log_debug("%d chars written\n", chars_written);
	if (chars_written < 0) {
		log_errx(1, "error in formatting message: %s");
		return;
	}
    va_end(args);
	buffer_chars_left -= chars_written;
	message_buffer_pos += chars_written;
	if (buffer_chars_left < 0) {
		log_errx(1, "buffer overflow by %d in add_to_buffer", -buffer_chars_left);
		return;
	}
	message_buffer_pos[0] = '\0';
}

void 
write_journal_entry(char *what) {
    char path[_POSIX_PATH_MAX];
    time_t t;
    struct tm tm;
	if (what[0] == '\0') 
		return;
    if (journal_file == NULL) {
		character_file_name(path, _POSIX_PATH_MAX, "journal");
        journal_file = fopen(path, "a");
        if (journal_file == NULL) {
            log_errx(1, "Could not open journal file (%s)\n", path);
            return;
        }
    }
    t = time(NULL);
    tm = *localtime(&t);
    fprintf(journal_file, "[%d-%02d-%02d %02d:%02d:%02d] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	print_uncolored(journal_file, what);
}

void
close_journal_file(void) {
	if (journal_file != NULL) {
		fclose(journal_file);
		journal_file = NULL;
	}
}

#define MAX_MATCHES 1
#define MAX_ERROR_MSG 256
#define UNCOLOR_REGEX "\\(\x1b[[0-9;]*m\\)"

static regex_t color_control_regex;
static int regex_compiled = 0;
void 
print_uncolored(FILE* out_file, char *in_string) {
	int errcode, start, finish;
	char msg_buffer[MAX_ERROR_MSG];
	regmatch_t matches[MAX_MATCHES];
	char *rest = in_string;

	if (regex_compiled == 0) {
		errcode = regcomp(&color_control_regex, UNCOLOR_REGEX, 0);
		if (errcode != 0) {
			(void) regerror (errcode, &color_control_regex, msg_buffer, MAX_ERROR_MSG);
			log_errx(1, "error compiling regex ('%s'): %s", UNCOLOR_REGEX, msg_buffer);
			return;
		}
		regex_compiled = 1;
	}
	// log_debug("string '%s' to file...\n", in_string);

	while (regexec(&color_control_regex, rest, MAX_MATCHES, matches, 0) == 0) {
		// ix = 1;
		// prev_finish = 0;
		// while (ix < MAX_MATCHES && matches[ix].rm_so != -1) {
		// log_debug("match found %d to %d in '%s'\n, ", matches[0].rm_so, matches[0].rm_eo, rest);
		start = matches[0].rm_so;
		// log_debug("writing part:  '%.*s'", start, rest);
		fprintf(out_file, "%.*s", start, rest);
		finish = matches[0].rm_eo;
			// ix++;
		// }
		rest += finish;
	}
	// log_debug("writing rest: '%s'", rest);
	fprintf(out_file, "%s\n", rest);
	// regfree(&color_control_regex);
}