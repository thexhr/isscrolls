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

#include <json-c/json.h>

#include <errno.h>
#include <limits.h>
#include <string.h>

#include "isscrolls.h"

void
cmd_discover_a_site(__attribute__((unused))char *cmd)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		ask_for_delve_difficulty();
		curchar->delve_active = 1;
	}

	update_prompt();
}

void
ask_for_delve_difficulty()
{
	struct character *curchar = get_current_character();

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	printf("Please set a rank for your site\n\n");
	printf("1\t - Troublesome site (3 progress per waypoint)\n");
	printf("2\t - Dangerous site (2 progress per waypoint)\n");
	printf("3\t - Formidable site (2 progress per waypoint)\n");
	printf("4\t - Extreme site (2 ticks per waypoint)\n");
	printf("5\t - Epic site (1 tick per waypoint)\n\n");

	curchar->delve->difficulty = ask_for_value("Enter a value between 1 and 5: ", 5);
}

void
cmd_delve_the_depths(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret, usedstat = 0;

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		printf("You haven't discovered a site yet. Use 'discoverasite' first\n");
		return;
	}

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("\nPlease specify the stat you'd like to use in this move\n\n");
		printf("edge\t- You are navigating the area with haste\n");
		printf("shadow\t- You are navigating the area with stealth or trickery\n");
		printf("wits\t- You are navigating the area with observation, intuition,"\
			"or expertise\n");
		printf("Example: delvethedepths wits\n\n");
		return;
	} else if (ret <= -20) {
		return;
	}

	if (strcasecmp(stat, "wits") == 0) {
		ival[0] = curchar->wits;
		usedstat = 1;
	} else if (strcasecmp(stat, "shadow") == 0) {
		ival[0] = curchar->shadow;
		usedstat = 2;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->heart;
		usedstat = 3;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You mark progress, delve deeper and find an opportunity:\n");
		mark_delve_progress();
		show_info_from_oracle(ORACLE_DELVE_OPPORTUNITY, 100);
	} else if (ret == 4) {
		printf("Rolling on the delve table with %s\n", stat);
		if (usedstat == 1)
			show_info_from_oracle(ORACLE_DELVE_THE_DEPTHS_WITS, 100);
		else if (usedstat == 2)
			show_info_from_oracle(ORACLE_DELVE_THE_DEPTHS_SHADOW, 100);
		else if (usedstat == 3)
			show_info_from_oracle(ORACLE_DELVE_THE_DEPTHS_EDGE, 100);
	} else {
		printf("You reveal a danger:\n");
		show_info_from_oracle(ORACLE_DELVE_DANGER, 100);
	}

	update_prompt();
}

void
cmd_check_your_gear(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		printf("You must start a delve with 'delvethedepths' first\n");
		return;
	}

	if (curchar->supply <= 0) {
		printf("You don't have any supply left.  You cannot make this move\n");
		return;
	}

	ival[0] = curchar->supply;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You have the needed gear\n");
		change_char_value("momentum", INCREASE, 1);
	} else if (ret == 4) {
		printf("You have the needed gear, but suffer -1 supply\n");
		change_char_value("momentum", INCREASE, 1);
		change_char_value("supply", DECREASE, 1);
	} else {
		printf("You don't have the needed gear and the situation grows more "\
			"perilous -> Rulebook\n");
	}
}


void
cmd_escape_the_depths(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		printf("You must start a delve with 'delvethedepths' first\n");
		return;
	}

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("\nPlease specify the stat you'd like to use in this move\n\n");
		printf("edge\t- If you find the fastest way out\n");
		printf("heart\t- If you steel yourself against the horrors\n");
		printf("iron\t- If you fight your way out\n");
		printf("wits\t- If you find retrace the steps or locate an alternate path\n");
		printf("shadow\t- If you keep out of sight\n");
		printf("Example: escapethedepths wits\n\n");
		return;
	} else if (ret <= -20) {
		return;
	}

	ival[0] = return_char_stat(stat);
	if (ival[0] == -1)
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You make your way safely out\n");
		change_char_value("momentum", INCREASE, 1);
		curchar->delve_active = 0;
		curchar->delve->progress = 0;
		delete_delve(curchar->id);
	} else if (ret == 4) {
		printf("You make your way out, but this place exacts its price.\n");
		printf("Choose one from the Rulebook\n");
		curchar->delve_active = 0;
		curchar->delve->progress = 0;
		delete_delve(curchar->id);
	} else {
		printf("A dire threat or imposing obstacle stands in your way\n");
		printf("Reveal a danger and if you success, you make your way out!\n");
		show_info_from_oracle(ORACLE_DELVE_DANGER, 100);
	}

	update_prompt();
}
void
cmd_locate_your_objective(char *cmd)
{
	struct character *curchar = get_current_character();
	double dval[2] = { -1.0, -1.0 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		printf("You must start a delve with 'delvethedepths' first\n");
		return;
	}

	dval[0] = curchar->delve->progress;
	dval[1] = get_int_from_cmd(cmd);

	ret = progress_roll(dval);
	if (ret == 8) {
		printf("You locate your objective and the situation favors you -> "\
			"Rulebook\n");
		curchar->delve_active = 0;
		curchar->delve->progress = 0;
		delete_delve(curchar->id);
	} else if (ret == 4) {
		printf("You locate your objective but face an unforeseen complication "\
			"-> Rulebook\n");
		curchar->delve_active = 0;
		curchar->delve->progress = 0;
		delete_delve(curchar->id);
	} else {
		locate_your_objective_failed();
	}

	update_prompt();
}

void
locate_your_objective_failed()
{
	struct character *curchar = get_current_character();
	int a;

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		log_debug("No active delve.\n");
		return;
	}

	printf("Please decide what to do\n\n");
	printf("1\t - End your delve and pay the price -> Rulebook\n");
	printf("2\t - Continue your delve -> progress is lost, difficulty +1\n");

	a = ask_for_value("Enter a value between 1 and 2: ", 2);
	if (a == 1) {
		curchar->delve_active = 0;
		curchar->delve->progress = 0;
		delete_delve(curchar->id);
	} else {
		curchar->delve->progress = 0;
		if (curchar->delve->difficulty < 5)
			curchar->delve->difficulty += 1;
	}
}

void
mark_delve_progress()
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->delve_active == 0) {
		printf("You need start a delve before you can mark progress\n");
		return;
	}

	switch (curchar->delve->difficulty) {
	case 1:
		curchar->delve->progress += 3;
		break;
	case 2:
		curchar->delve->progress += 2;
		break;
	case 3:
		curchar->delve->progress += 1;
		break;
	case 4:
		curchar->delve->progress += 0.5;
		break;
	case 5:
		curchar->delve->progress += 0.25;
		break;
	default:
		curchar->delve->difficulty = 1;
		log_errx(1, "Unknown difficulty.  This should not happen.  Set it to 1\n");
	}

	if (curchar->delve->progress > 10) {
		printf("Your reached all milestones of your delve.  Consider ending it\n");
		curchar->delve->progress = 10;
	}

	update_prompt();
}

void
save_delve()
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *items, *id;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("No character loaded.  No delve to save.\n");
		return;
	}

	if (curchar->delve_active == 0) {
		log_debug("No active delve to save.\n");
		return;
	}

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "difficulty",
		json_object_new_int(curchar->delve->difficulty));
	json_object_object_add(cobj, "progress",
		json_object_new_double(curchar->delve->progress));

	snprintf(path, sizeof(path), "%s/delve.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No delve JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create delve JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "delve", items);
	} else {
		/* Get existing character array from JSON */
		if (!json_object_object_get_ex(root, "delve", &items)) {
			log_debug("Cannot find a [delve] array in %s\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "delve", items);
		}

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->id == json_object_get_int(id)) {
				log_debug("Update delve entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No delve entry for %s found, adding new one\n", curchar->name);
		json_object_array_add(items, cobj);
	}

out:
	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

void
delete_delve(int id)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	snprintf(path, sizeof(path), "%s/delve.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No delve JSON file found\n");
		return;
	}

	json_object *delve;
	if (!json_object_object_get_ex(root, "delve", &delve)) {
		log_debug("Cannot find a [delve] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(delve);
	for (i = 0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(delve, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			json_object_array_del_idx(delve, i, 1);
			log_debug("Deleted delve entry for %d\n", id);
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

void
load_delve(int id)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	snprintf(path, sizeof(path), "%s/delve.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No delve JSON file found\n");
		return;
	}

	json_object *delve;
	if (!json_object_object_get_ex(root, "delve", &delve)) {
		log_debug("Cannot find a [delve] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(delve);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(delve, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			log_debug("Loading delve for id: %d\n", json_object_get_int(lid));

			json_object *cval;
			json_object_object_get_ex(temp, "difficulty", &cval);
			curchar->delve->difficulty = json_object_get_int(cval);
			json_object_object_get_ex(temp, "progress", &cval);
			curchar->delve->progress   = json_object_get_double(cval);
		}
	}

	json_object_put(root);
}

