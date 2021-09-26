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

#ifndef ISSCROLLS_H
#define ISSCROLLS_H

#include <sys/queue.h>

#include <json-c/json.h>

#include <stdio.h>

#define VERSION "2021.c"
#define PATH_SHARE_DIR "/usr/local/share/isscrolls"

#define MAX_NAME_LEN 12
#define MAX_RANK_LEN 13
#define MAX_PLOT_LEN 63
#define MAX_MYSTIC_LEN 127
#define MAX_PLACES_LEN 18
#define MAX_PROMPT_LEN 255
#define MAX_PTP_LEN 201
#define MAX_CHAR_LEN 100
#define MAX_PROGRESS 10
#define MAX_STAT_LEN 20
#define MAX_DELVE_LEN 50

#define STAT_WITS 	0x00001
#define STAT_EDGE 	0x00010
#define STAT_HEART 	0x00100
#define STAT_SHADOW 0x01000
#define STAT_IRON 	0x10000

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define CURCHAR_CHECK() do { 											\
	if (curchar == NULL) { 												\
		printf("No character loaded.  Use 'cd' to load a character\n"); \
		return; 														\
	} 																\
} while(0)

/* oracle.c */
void cmd_show_iron_name(char *);
void cmd_show_elf_name(char *);
void cmd_show_giant_name(char *);
void cmd_show_varou_name(char *);
void cmd_show_troll_name(char *);
void cmd_show_action(char *);
void cmd_show_theme(char *);
void cmd_show_rank(char *);
void cmd_show_combat_action(char *);
void cmd_show_plot_twist(char *);
void cmd_show_mystic_backshlash(char *);
void cmd_show_region(char *);
void cmd_show_location_description(char *);
void cmd_show_location(char *);
void cmd_show_coastal_location(char *);
void cmd_show_pay_the_price(char *);
void cmd_reveal_a_danger(char *);
void cmd_find_an_opportunity(char *);
void show_info_from_oracle(int, int);

/* readline.c */
char ** my_completion(const char *, int, int);
char* command_generator(const char *, int);
void initialize_readline(const char *);
void execute_command(char *);
char* stripwhite (char *);
struct command* find_command(char *);
void cmd_cd(char *);

/* rolls.c */
void cmd_roll_action_dice(char *);
void cmd_roll_challenge_die(char *);
void cmd_roll_oracle_die(char *);
void cmd_yes_or_no(char *);
void cmd_usage(char *);
void cmd_create_character(char *);
void cmd_resupply(char *);
void cmd_face_danger(char *);
void cmd_secure_an_advantage(char *);
void cmd_make_camp(char *);
void cmd_compel(char *);
void cmd_sojourn(char *);
void cmd_draw_the_circle(char *);
void cmd_forge_a_bond(char *);
void cmd_test_your_bond(char *);
void cmd_swear_an_iron_vow(char *);
void cmd_face_death(char *);
void cmd_endure_stress(char *);
long roll_action_die(void);
long roll_challenge_die(void);
long roll_oracle_die(void);
void yes_or_no(int);
int action_roll(int[2]);
int progress_roll(double[2]);
void ask_for_journey_difficulty(void);
int get_int_from_cmd(const char *);
int get_args_from_cmd(char *, char *, int*);

/* isscrolls.c */
void cmd_quit(char *);
void show_banner(char *);
void log_debug(const char *, ...);
void log_errx(int, const char *, ...);
void pm(int, const char *, ...);
void setup_base_dir(void);
void shutdown(int) __attribute__((noreturn));
void sandbox(const char *);
void set_prompt(const char *);
const char * get_isscrolls_dir(void);

/* character.c */
struct character* init_character_struct(void);
void print_character(void);
struct character* create_character(const char *);
void free_character(void);
int validate_range(int, int);
int ask_for_value(const char *, int);
void cmd_print_current_character(char *);
void cmd_delete_character(char *);
void save_character(void);
void delete_saved_character(int);
int load_character(int);
struct character * get_current_character(void);
int return_character_id(const char *);
int return_char_stat(const char *, int);
void load_characters_list(void);
void save_current_character(void);
void cmd_increase_value(char *);
void cmd_decrease_value(char *);
void cmd_toggle(char *);
void cmd_ls(char *);
void cmd_gather_information(char *);
void cmd_heal(char *);
void cmd_mark_progress(char *);
void change_char_value(const char *, int, int);
void increase_value(const char *, int *, int);
void decrease_value(const char *, int *, int);
void modify_value(const char *, int *, int, int, int, int);
void toggle_value(const char *, int *);
void change_momentum_reset(int);
void set_max_momentum(void);
int validate_int(json_object *, const char *, int, int, int);
double validate_double(json_object *, const char *, double, double, double);
int character_exists(const char *);
void update_prompt(void);

/* journey.c */
void mark_journey_progress(int);
void save_journey(void);
void load_journey(int);
void delete_journey(int);
void reach_your_destination_failed(void);
void cmd_undertake_a_journey(char *);
void cmd_reach_your_destination(char *);

/* fight.c */
void load_fight(int);
void save_fight(void);
void delete_fight(int);
void cmd_enter_the_fray(char *);
void cmd_strike(char *);
void cmd_clash(char *);
void cmd_battle(char *);
void cmd_endure_harm(char *);
void mark_fight_progress(int);
void ask_for_fight_difficulty(void);
void cmd_end_the_fight(char *);
void set_initiative(int);

/* delve.c */
void cmd_discover_a_site(char *);
void cmd_delve_the_depths(char *);
void cmd_locate_your_objective(char *);
void cmd_check_your_gear(char *);
void cmd_escape_the_depths(char *);
void mark_delve_progress(int);
void load_delve(int);
void save_delve(void);
void delete_delve(int);
void ask_for_delve_difficulty(void);
void locate_your_objective_failed(void);

enum oracle_codes {
	ORACLE_IS_NAMES,
	ORACLE_ELF_NAMES,
	ORACLE_GIANT_NAMES,
	ORACLE_VAROU_NAMES,
	ORACLE_TROLL_NAMES,
	ORACLE_ACTIONS,
	ORACLE_THEMES,
	ORACLE_RANKS,
	ORACLE_COMBAT_ACTIONS,
	ORACLE_PLOT_TWISTS,
	ORACLE_MYSTIC_BACKSLASH,
	ORACLE_REGION,
	ORACLE_LOCATION ,
	ORACLE_COASTAL,
	ORACLE_DESCRIPTION,
	ORACLE_PAYTHEPRICE,
	ORACLE_DELVE_THE_DEPTHS_EDGE,
	ORACLE_DELVE_THE_DEPTHS_SHADOW,
	ORACLE_DELVE_THE_DEPTHS_WITS,
	ORACLE_DELVE_OPPORTUNITY,
	ORACLE_DELVE_DANGER,
};

enum how_to_change_values {
	INCREASE,
	DECREASE,
};

enum color_codes {
	RED,
	YELLOW,
	GREEN,
	DEFAULT,
};

struct command {
	const char *name;
	void (*cmd)(char *);
	const char *doc;
	int alias;
};

struct journey {
	double progress;
	int id;
	int difficulty;
};

struct fight {
	double progress;
	int id;
	int difficulty;
	int initiative;
};

struct delve {
	double progress;
	int id;
	int difficulty;
};

struct character {
	struct journey *j;
	struct fight *fight;
	struct delve *delve;
	char *name;
	double bonds;
	int dead;
	int journey_active;
	int fight_active;
	int delve_active;
	int id;
	int edge;
	int heart;
	int iron;
	int shadow;
	int wits;
	int exp;
	int exp_used;
	int momentum;
	int max_momentum;
	int momentum_reset;
	int health;
	int spirit;
	int supply;
	int wounded;
	int unprepared;
	int shaken;
	int encumbered;
	int maimed;
	int cursed;
	int corrupted;
	int tormented;
};

struct entry {
	LIST_ENTRY(entry) entries;
	char name[255];
	int id;
};

#endif

