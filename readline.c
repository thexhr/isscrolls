/*
 * Most of this file is copied from the official GNU readline
 * documentation: https://tiswww.case.edu/php/chet/readline/readline.html
 *
 * The documentation was written by
 * Lionel Cons <Lionel.Cons@cern.ch> (original author)
 * Karl Berry  <karl@freefriends.org>
 * Olaf Bachmann <obachman@mathematik.uni-kl.de>
 * and many others.
 *
 * LICENSE: GNU GPL v2
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "isscrolls.h"

static struct command commands[] = {
	{ "cd", cmd_cd, "Switch to or from a character", 0, 0 },
	{ "cds", cmd_cds, "Switch to a character and show all vows", 0, 0 },
	{ "help", cmd_usage, "Show help", 0, 0 },
	{ "ls", cmd_ls, "List all characters", 0, 0 },
	{ "quit", cmd_quit, "Quit the program", 0, 0 },
	{ "q", cmd_quit, "Quit the program", 1, 0 },
	{ "save", cmd_save, "Save the current character", 0, 0 },
	{ "journal", cmd_journal, "Write text to the journal", 0, 0},
	{ "j", cmd_journal, "Write text to the journal", 1, 0},
	{ "--- DICE ROLLS ---", NULL, "", 0, 0 },
	{ "action", cmd_roll_action_dice, "Perform an action roll", 0, 0 },
	{ "actionoracle", cmd_show_action, "Show a random action oracle", 0, 0 },
	{ "burnmomentum", cmd_burn_momentum, "Burn your character's momentum", 0, 0 },
	{ "challenge", cmd_roll_challenge_die, "Roll a challenge die", 0, 0 },
	{ "oracle", cmd_roll_oracle_die, "Roll two challenge dice as oracle", 0, 0 },
	{ "yesorno", cmd_yes_or_no, "Roll oracle to answer a yes/no question", 0, 0 },
	{ "--- CHARACTER COMMANDS ---", NULL, "", 0, 0 },
	{ "create", cmd_create_character, "Create a new character", 0, 0 },
	{ "decrease", cmd_decrease_value, "Decrease a character's value", 0, 0 },
	{ "delete", cmd_delete_character, "Delete currently loaded character", 0, 0 },
	{ "increase", cmd_increase_value, "Increase a character's value", 0, 0 },
	{ "learnfromyourfailures", cmd_learn_from_your_failures, "Roll a 'learn from your failures' move", 0, 0 },
	{ "markabond", cmd_mark_a_bond, "Mark a bond", 0, 0 },
	{ "markprogress", cmd_mark_progress, "Mark progress in your current endeavour", 0, 0 },
	{ "print", cmd_print_current_character, "Print current character sheet", 0, 0 },
	{ "p", cmd_print_current_character, "Print current character sheet", 1, 0 },
	{ "toggle", cmd_toggle, "Toggle character's stats", 0, 0 },
	{ "--- GAME MOVES ---", NULL, "", 0, 0 },
	{ "battle", cmd_battle, "Roll a 'battle' move", 0, 0 },
	{ "clash", cmd_clash, "Roll a 'clash' move", 0, 0 },
	{ "compel", cmd_compel, "Roll a 'compel' move", 0, 0 },
	{ "drawthecircle", cmd_draw_the_circle, "Roll a 'draw the circle' move", 0, 0 },
	{ "endureharm", cmd_endure_harm, "Roll a 'endure harm' move", 0, 0 },
	{ "endurestress", cmd_endure_stress, "Roll a 'endure stress' move", 0, 0 },
	{ "endthefight", cmd_end_the_fight, "Roll a 'end the fight' move", 0, 0 },
	{ "enterthefray", cmd_enter_the_fray, "Roll a 'enter the fray' move", 0, 0 },
	{ "facedanger", cmd_face_danger, "Roll a 'face danger' move", 0, 0 },
	{ "facedesolation", cmd_face_desolation, "Roll a 'face desolation' move", 0, 0 },
	{ "facedeath", cmd_face_death, "Roll a 'face death' move", 0, 0 },
	{ "forgeabond", cmd_forge_a_bond, "Roll a 'forge a bond' move", 0, 0 },
	{ "gatherinformation", cmd_gather_information, "Roll a 'gather information' move", 0, 0 },
	{ "heal", cmd_heal, "Roll a 'heal' move", 0, 0 },
	{ "makecamp", cmd_make_camp, "Roll a 'make camp' move", 0, 0 },
	{ "reachamilestone", cmd_reach_a_milestone, "Roll a 'reach a milestone' move", 0, 0 },
	{ "reachyourdestination", cmd_reach_your_destination, "Roll a 'reach your destination' move", 0, 0 },
	{ "resupply", cmd_resupply, "Roll a 'resupply' move", 0, 0 },
	{ "secureanadvantage", cmd_secure_an_advantage, "Roll a 'secure an advantage' move", 0, 0 },
	{ "sojourn", cmd_sojourn, "Roll a 'sojourn' move", 0, 0 },
	{ "strike", cmd_strike, "Roll a 'strike' move", 0, 0 },
	{ "takedecisiveaction", cmd_take_decisive_action, "Roll a 'take decisive action' move", 0, 1 },
	{ "testyourbond", cmd_test_your_bond, "Roll a 'test your bond' move", 0, 0 },
	{ "undertakeajourney", cmd_undertake_a_journey, "Roll a 'undertake a journey' move", 0, 0 },
	{ "writeyourepilogue", cmd_write_your_epilogue, "Roll a 'write your epilogue' move", 0, 0 },
	{ "--- DELVE MOVES ---", NULL, "", 0, 0 },
	{ "checkyourgear", cmd_check_your_gear, "Roll a 'check your gear' move", 0, 0 },
	{ "delvethedepths", cmd_delve_the_depths, "Roll a 'delve the depths' move", 0, 0 },
	{ "discoverasite", cmd_discover_a_site, "Roll a 'discover a site' move", 0, 0 },
	{ "escapethedepths", cmd_escape_the_depths, "Roll a 'escape the depths' move", 0, 0 },
	{ "locateyourobjective", cmd_locate_your_objective, "Roll a 'locate your objective' move", 0, 0 },
	{ "--- WORK WITH VOWS ---", NULL, "", 0, 0 },
	{ "fulfillyourvow", cmd_fulfill_your_vow, "Roll a 'fulfill your vow' move", 0, 0 },
	{ "swearanironvow", cmd_swear_an_iron_vow, "Roll a 'swear an iron vow' move", 0, 0 },
	{ "vownew", cmd_create_new_vow, "Create a new vow", 0, 0 },
	/*{ "vowmarkprogress", cmd_mark_vow_progress, "Mark progress for the current vow", 0, 0 },*/
	{ "vowactivate", cmd_activate_vow, "Activate an existing vow", 0, 0 },
	{ "vowdeactivate", cmd_deactivate_vow, "Deactivate current vow", 0, 0 },
	{ "vowshow", cmd_show_all_vows, "Show all vows of the current character", 0, 0 },
	/*{ "vowdelete", cmd_delete_vow, "Irrecoverably delete the active vow", 0, 0 },*/
	{ "--- WORK WITH NOTES ---", NULL, "", 0, 0 },
	{ "notenew", cmd_create_new_note, "Create a new note", 0, 0 },
	{ "noteedit", cmd_edit_note, "Edit a note", 0, 0 },
	{ "noteshow", cmd_show_all_notes, "Show all notes of the current character", 0, 0 },
	{ "notedelete", cmd_delete_note, "Irrecoverably delete a note", 0, 0 },
	{ "--- STARFORGED MOVES ---", NULL, "", 0, 1 },
	{ "undertakeanexpedition", cmd_undertake_an_expedition, "Roll a 'undertake an expedition ' move", 0, 1 },
	{ "finishanexpedition", cmd_finish_an_expedition, "Roll a 'finish an expedition ' move", 0, 1 },
	{ "exploreawaypoint", cmd_explore_a_waypoint, "Roll an 'explore a waypoint' move", 0, 1 },
	{ "gainground", cmd_gain_ground, "Roll a 'gain ground' move", 0, 1 },
	{ "hearten", cmd_hearten, "Roll a 'hearten' move", 0, 1 },
	{ "makeaconnection", cmd_make_a_connection, "Roll a 'make a connection' move", 0, 1 },
	{ "reactunderfire", cmd_react_under_fire, "Roll a 'react under fire' move", 0, 1 },
	{ "setacourse", cmd_set_a_course, "Roll a 'set a course' move", 0, 1 },
	{ "sacrificeresources", cmd_sacrifice_resources, "Roll a 'sacrifice resources' move", 0, 1 },
	{ "testyourrelationship", cmd_test_your_relationship, "Roll a 'test your relationship' move", 0, 1 },
	{ "--- ORACLE TABLE ROLLS ---", NULL, "", 0, 0 },
	{ "combataction", cmd_show_combat_action, "Show a random combat action move", 0, 0 },
	{ "coastalwaterlocation", cmd_show_coastal_location, "Show a random coastal water location", 0, 0 },
	{ "elfname", cmd_show_elf_name, "Show a random Elf name", 0, 0 },
	{ "findanopportunity", cmd_find_an_opportunity, "Show a random opportunity", 0, 0 },
	{ "generatenpc", cmd_generate_npc, "Generate a random NPC", 0, 0 },
	{ "giantname", cmd_show_giant_name, "Show a random Giant name", 0, 0 },
	{ "ironlandername", cmd_show_iron_name, "Show a random Ironlander name", 0, 0 },
	{ "location", cmd_show_location, "Show a random location", 0, 0 },
	{ "locationdescription", cmd_show_location_description, "Show a random location description", 0, 0 },
	{ "moonoracle", cmd_moon_oracle, "Show moon phases from Sundered Isles ", 0, 0 },
	{ "mysticbackslash", cmd_show_mystic_backshlash, "Show a random mystic backlash", 0, 0 },
	{ "paytheprice", cmd_show_pay_the_price, "Show a random pay the price result", 0, 0 },
	{ "plottwist", cmd_show_plot_twist, "Show a random major plot twist", 0, 0 },
	{ "rank", cmd_show_rank, "Show a random challenge rank", 0, 0 },
	{ "region", cmd_show_region, "Show a random region", 0, 0 },
	{ "revealadanger", cmd_reveal_a_danger, "Show a random danger", 0, 0 },
	{ "settlementtrouble", cmd_show_settlement_trouble, "Show a random settlement trouble", 0, 0 },
	{ "theme", cmd_show_theme, "Show a random theme oracle", 0, 0 },
	{ "trollname", cmd_show_troll_name, "Show a random Troll name", 0, 0 },
	{ "varouname", cmd_show_varou_name, "Show a random Varou name", 0, 0 },
	{ (char *)NULL, NULL, (char *)NULL, 0, 0 }
};

void
cmd_usage(__attribute__((unused)) char *unused)
{
	int i;

	printf("%-20s %s\n", "COMMAND", "DESCRIPTION");
	for (i=0; commands[i].doc; i++) {
		if (commands[i].alias == 0)
			printf("%-20s %s\n", commands[i].name, commands[i].doc);
		else
			printf("%-20s\n", commands[i].name);
	}

	printf("\nFor more detailed information consult the man page: $ man isscrolls\n\n");
}

char *
stripwhite (char *string)
{
	register char *s, *t;

	for (s = string; isspace (*s); s++)
		;

	if (*s == 0)
		return (s);

	t = s + strlen (s) - 1;
	while (t > s && isspace (*t))
		t--;

	*++t = '\0';

	return s;
}

struct command *
find_command(char *line)
{
	int i;

	for (i=0; commands[i].name; i++) {
		/* Skip over --- which is a separator for the cmd_usage() */
		if (strncasecmp(line, "---", 3) == 0)
			continue;
		if (strcasecmp(line, commands[i].name) == 0)
			return &commands[i];
	}

	return NULL;
}

void
initialize_readline(const char *base_path)
{
	char hist_path[_POSIX_PATH_MAX];

	rl_readline_name = "issrolls";

	rl_attempted_completion_function = my_completion;

	using_history();

	snprintf(hist_path, _POSIX_PATH_MAX, "%s/history", base_path);

	log_debug("Reading history from %s\n", hist_path);
	read_history(hist_path);
}

char **
my_completion(const char *text, int start, __attribute__((unused))int end)
{
	char **matches;

	matches = (char **)NULL;

	if (start == 0)
		matches = rl_completion_matches(text, command_generator);

	return matches;
}

char *
command_generator(const char *text, int state)
{
	const char *name;
	static int list_index;
	static size_t len;

	if (!state) {
		list_index = 0;
		len = strlen(text);
	}

	while ((name = commands[list_index].name)) {
		list_index++;

		if (strncmp(name, text, len) == 0) {
			return strdup(name);
		}
	}

	return (char *)NULL;
}

void
execute_command(char *line)
{
	struct command *cmd;
	char *word;
	int i = 0;

	/* Skip over white spaces */
	while (line[i] && isspace(line[i]))
		i++;

	/* Set to char array after last whitespace */
	word = line + i;

	/* Skip over all non-isspace characters */
	while (line[i] && !isspace(line[i]))
		i++;

	/* If line, is still valid, NUL terminate command word */
	if (line[i])
		line[i++] = '\0';

	cmd = find_command(word);

	if (cmd == NULL) {
		printf("Command not found\n");
		return;
	}

	/* Skip over white spaces after command line */
	while (isspace(line[i]))
		i++;

	word = line + i;

	((*(cmd->cmd)) (word));
	return;
}

static char *deftext = NULL;

static int
set_deftext(void)
{
    if (deftext != NULL) {
        rl_insert_text (deftext);
        deftext = (char *) NULL;
        rl_startup_hook = (rl_hook_func_t *) NULL;
    }
	return 0;
}

char *
edit_text(char *prompt, char *orig_text)
{
	deftext = orig_text;
	rl_startup_hook = set_deftext;
	return readline (prompt);
}

