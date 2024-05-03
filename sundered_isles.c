/*
 * Copyright (c) 2024 Matthias Schmidt <xhr@giessen.ccc.de>
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
cmd_sacrifice_resources(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int hr;

	CURCHAR_CHECK();

	ival[1] = get_int_from_cmd(cmd);
	if (ival[1] == -1) {
		printf("Please provide a number as argument\n\n");
		printf("The number is the amount of supply you loose (1-3)\n");
		printf("Example: sacrificeresources 2\n");
		return;
	}

	hr = curchar->supply - ival[1];
	if (hr >= 0) {
		curchar->supply -= ival[1];
		printf("You suffer -%d supply and it is down to %d\n",
			ival[1], curchar->supply);
	} else {
		log_debug("supply < 0: %d\n", hr);
		curchar->supply = 0;
		printf("Your supply is exhausted, mark unprepared\n");
		cmd_toggle("unprepared");
	}

}


void
cmd_set_a_course(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->supply;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8) { /* strong hit */
		change_char_value("momentum", INCREASE, 1);
		printf("You reach your destination and the situation favours you.\n");
	} else if (ret == 4) { /* weak hit */
		printf("You arrive, but face a cost or complication.  Choose one:\n");
		printf(" - Suffer costs en route. Make one or two suffer moves\n");
		printf(" - Face a complication at the destination. Envision what you "\
			"encounter\n");
	} else
		printf("You are waylaid by a threat.  Pay the price -> Rulebook\n");
}

void
cmd_explore_a_waypoint(__attribute__((unused)) char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->wits;

	ret = action_roll(ival);
	if (ret == 18) { /* strong hit with a match */
		printf("Strong hit with a match.  You make a discovery\n");
	} else if (ret == 12) { /* miss with a match */
		printf("Miss with a match. You may confront chaos\n");
	} else if (ret == 8) { /* strong hit */
		printf("Choose one:\n");
		printf(" - Find an opportunity, envision it and take 2 momentum\n");
		printf(" - Mark progress on your expedition\n");
	} else if (ret == 4) { /* weak hit */
		change_char_value("momentum", INCREASE, 1);
		printf("You uncover something interesting, but it is bound up in a peril\n");
	} else if (ret == 2) { /* miss */
		printf("You encounter a hardship or threat.  Pay the price -> Rulebook\n");
	}
}

void
cmd_hearten(__attribute__((unused)) char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->heart;

	ret = action_roll(ival);
	if (ret == 8) { /* strong hit */
		printf("You find companionship or comfort -> Rulebook\n");
	} else if (ret == 4) { /* weak hit */
		change_char_value("momentum", DECREASE, 1);
		printf("You find companionship or comfort, but this indulgence is "\
				"fleeting\n");
	} else if (ret == 2) { /* miss */
		printf("You take no comfort and the situation worsens.  Pay the "\
			"price -> Rulebook\n");
	}
}

static void
ask_for_expedition_difficulty(void)
{
	struct character *curchar = get_current_character();

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	printf("Please set a rank for your expedition\n\n");
	printf("1\t - Troublesome expedition (3 progress per waypoint)\n");
	printf("2\t - Dangerous expedition (2 progress per waypoint)\n");
	printf("3\t - Formidable expedition (2 progress per waypoint)\n");
	printf("4\t - Extreme expedition (2 ticks per waypoint)\n");
	printf("5\t - Epic expedition (1 tick per waypoint)\n\n");

	curchar->expedition->difficulty = ask_for_value("Enter a value between 1 and 5: ", 5);
}

void
cmd_react_under_fire(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("\nPlease specify the stat you'd like to use in this move\n\n");
		printf("edge\t- You are in pursuit, dodging, getting back in position\n");
		printf("heart\t- You remain stalwart against fear or temptation\n");
		printf("iron\t- You block or divert with force, or take the hit\n");
		printf("shadow\t- You move into hiding or create a distraction\n");
		printf("wits\t- You change the plan or find a way out\n");
		printf("Example: reactunderfire wits\n\n");
		return;
	} else if (ret <= -20) {
		return;
	}

	if (strcasecmp(stat, "wits") == 0) {
		ival[0] = curchar->wits;
	} else if (strcasecmp(stat, "shadow") == 0) {
		ival[0] = curchar->shadow;
	} else if (strcasecmp(stat, "iron") == 0) {
		ival[0] = curchar->iron;
	} else if (strcasecmp(stat, "heart") == 0) {
		ival[0] = curchar->heart;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->edge;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		change_char_value("momentum", INCREASE, 1);
		printf("You success and are in control.\n");
	} else if (ret == 4) {
		printf("You avoid the worst of the danger or overcome the obstacle, but\n");
		printf("not without a cost. Make a suffer move and stay in a bad spot.\n");
	} else if (ret == 2) {
		printf("You stay in a bad spot.  Pay the price.\n");
	}
}

void
cmd_undertake_an_expedition(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("\nPlease specify the stat you'd like to use in this move\n\n");
		printf("edge\t- You are navigating with speed\n");
		printf("shadow\t- You are keeping a low profile\n");
		printf("wits\t- You are staying vigilant\n");
		printf("Example: undertakeanexpedition wits\n\n");
		return;
	} else if (ret <= -20) {
		return;
	}

	/* Undertake a new expedition, ask for the difficulty here since the user
	 * might not have provided a stat for the move. */
	if (curchar->expedition_active == 0) {
		ask_for_expedition_difficulty();
		curchar->expedition_active = 1;
	}

	if (strcasecmp(stat, "wits") == 0) {
		ival[0] = curchar->wits;
	} else if (strcasecmp(stat, "shadow") == 0) {
		ival[0] = curchar->shadow;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->edge;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You reach a waypoint and mark progress.\n");
		mark_expedition_progress(INCREASE);
	} else if (ret == 4) {
		printf("You reach a waypoint and mark progress but with a cost.  Choose one:\n\n");
		printf(" - Suffer costs en route: Make a suffer move (-2) or two suffer moves (-1).\n");
		printf(" - Face peril on a waypoint.\n");
		mark_expedition_progress(INCREASE);
	} else if (ret == 2) {
		printf("You are waylaid by a crisis.  Pay the price.\n");
	}

	update_prompt();
}

void
cmd_finish_an_expedition(char *cmd)
{
	struct character *curchar = get_current_character();
	double dval[2] = { -1.0, -1.0 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->expedition_active == 0) {
		printf("You must undertake an expedition with 'undertakeanexpedition' first\n");
		return;
	}

	dval[0] = curchar->expedition->progress;
	dval[1] = get_int_from_cmd(cmd);

	ret = progress_roll(dval);
	if (ret == 8) {
		printf("You reach your destination or complete your survey -> Rulebook\n");
	} else if (ret == 4) {
		printf("You reach your destination or complete your survey "\
			"but face an unforeseen complication -> Rulebook\n");
	} else {
		printf("Your destination is lost to you, or you come to understand "\
			"the true nature or cost of the expedition -> Rulebook\n");
	}

	curchar->expedition_active = 0;
	curchar->expedition->progress = 0;
	delete_expedition(curchar->id);

	update_prompt();
}

void
mark_expedition_progress(int what)
{
	struct character *curchar = get_current_character();
	double amount = 0;

	CURCHAR_CHECK();

	if (curchar->expedition_active == 0) {
		printf("You need undertake an expedition before you can mark progress\n");
		return;
	}

	switch (curchar->expedition->difficulty) {
	case 1:
		amount = 3;
		break;
	case 2:
		amount = 2;
		break;
	case 3:
		amount = 1;
		break;
	case 4:
		amount = 0.5;
		break;
	case 5:
		amount = 0.25;
		break;
	default:
		curchar->expedition->difficulty = 1;
		log_errx(1, "Unknown difficulty.  This should not happen.  Set it to 1\n");
	}

	if (what == INCREASE)
		curchar->expedition->progress += amount;
	else
		curchar->expedition->progress -= amount;

	if (curchar->expedition->progress > 10) {
		printf("Your reached all waypoints of your expedition.  Consider finishing it\n");
		curchar->expedition->progress = 10;
	} else if (curchar->expedition->progress < 0)
		curchar->expedition->progress = 0;

	update_prompt();
}

void
save_expedition(void)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *items, *id;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("No character loaded.  No expedition to save.\n");
		return;
	}

	if (curchar->expedition_active == 0) {
		log_debug("No active expedition to save.\n");
		return;
	}

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "difficulty",
		json_object_new_int(curchar->expedition->difficulty));
	json_object_object_add(cobj, "progress",
		json_object_new_double(curchar->expedition->progress));

	ret = snprintf(path, sizeof(path), "%s/expedition.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No expedition JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create expedition JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "expedition", items);
	} else {
		/* Get existing character array from JSON */
		if (!json_object_object_get_ex(root, "expedition", &items)) {
			log_debug("Cannot find a [expedition] array in %s\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "expedition", items);
		}

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->id == json_object_get_int(id)) {
				log_debug("Update expedition entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No expedition entry for %s found, adding new one\n", curchar->name);
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
delete_expedition(int id)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	size_t temp_n, i;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/expedition.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No expedition JSON file found\n");
		return;
	}

	json_object *expedition;
	if (!json_object_object_get_ex(root, "expedition", &expedition)) {
		log_debug("Cannot find a [expedition] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(expedition);
	for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(expedition, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			json_object_array_del_idx(expedition, i, 1);
			log_debug("Deleted expedition entry for %d\n", id);
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

void
load_expedition(int id)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	ret = snprintf(path, sizeof(path), "%s/expedition.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No expedition JSON file found\n");
		return;
	}

	json_object *expedition;
	if (!json_object_object_get_ex(root, "expedition", &expedition)) {
		log_debug("Cannot find a [expedition] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(expedition);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(expedition, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			log_debug("Loading expedition for id: %d\n", json_object_get_int(lid));

			curchar->expedition->difficulty = validate_int(temp, "difficulty", 0, 5, 1);
			curchar->expedition->progress   = validate_double(temp, "progress", 0, 10, 0);
		}
	}

	json_object_put(root);
}

