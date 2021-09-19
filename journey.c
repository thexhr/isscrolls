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
cmd_undertake_a_journey(char *cmd)
{
	struct character *curchar = get_current_character();
	char *ep;
	long lval;
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->wits;
	if (strlen(cmd) > 0) {
		errno = 0;
		lval = strtol(cmd, &ep, 10);
		if (cmd[0] == '\0' || *ep != '\0') {
			printf("Please provide a number as argument\n");
			return;
		}
		if ((errno == ERANGE || lval <= 0 || lval > 10)) {
			printf("Please provide a number between 1 and 10\n");
			return;
		}

		ival[1] = lval;
		log_debug("Arg provided %d\n", ival[1]);
	}

	if (curchar->journey_active == 0) {
		ask_for_journey_difficulty();
		curchar->journey_active = 1;
	}

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You reach a waypoint and can choose one option -> Rulebook\n");
		mark_journey_progress();
	} else if (ret == 4) {
		printf("You reach a waypoint, but suffer -1 supply\n");
		change_char_value("supply", DECREASE, 1);
		mark_journey_progress();
	} else
		printf("Pay the price -> Rulebook\n");

	update_prompt();
}

void
cmd_reach_your_destination(char *cmd)
{
	struct character *curchar = get_current_character();
	double dval[2] = { -1.0, -1.0 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->journey_active == 0) {
		printf("You must start a journey with 'undertakeajourney' first\n");
		return;
	}

	dval[0] = curchar->j->progress;
	dval[1] = get_int_from_cmd(cmd);

	ret = progress_roll(dval);
	if (ret == 8) {
		printf("You reach your destination and the situation favors you -> "\
			"Rulebook\n");
		curchar->journey_active = 0;
		curchar->j->progress = 0;
		delete_journey(curchar->id);
	} else if (ret == 4) {
		printf("You reach your destination but face an unforeseen complication "\
			"-> Rulebook\n");
		curchar->journey_active = 0;
		curchar->j->progress = 0;
		delete_journey(curchar->id);
	} else {
		reach_your_destination_failed();
	}

	update_prompt();
}

void
mark_journey_progress()
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->journey_active == 0) {
		printf("You need start a journey before you can mark progress\n");
		return;
	}

	switch (curchar->j->difficulty) {
	case 1:
		curchar->j->progress += 3;
		break;
	case 2:
		curchar->j->progress += 2;
		break;
	case 3:
		curchar->j->progress += 1;
		break;
	case 4:
		curchar->j->progress += 0.5;
		break;
	case 5:
		curchar->j->progress += 0.25;
		break;
	default:
		curchar->j->difficulty = 1;
		log_errx(1, "Unknown difficulty.  This should not happen.  Set it to 1\n");
	}

	if (curchar->j->progress > 10) {
		printf("Your reached all milestones of your journey.  Consider ending it\n");
		curchar->j->progress = 10;
	}

	update_prompt();
}

void
reach_your_destination_failed()
{
	struct character *curchar = get_current_character();
	int a;

	CURCHAR_CHECK();

	if (curchar->journey_active == 0) {
		log_debug("No active journey.\n");
		return;
	}

	printf("Please decide what to do\n\n");
	printf("1\t - End your journey and pay the price -> Rulebook\n");
	printf("2\t - Continue your journey -> progress is lost, difficulty +1\n");

	a = ask_for_value("Enter a value between 1 and 2: ", 2);
	if (a == 1) {
		curchar->journey_active = 0;
		curchar->j->progress = 0;
		delete_journey(curchar->id);
	} else {
		curchar->j->progress = 0;
		if (curchar->j->difficulty < 5)
			curchar->j->difficulty += 1;
	}
}

void
save_journey()
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *items, *id;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("No character loaded.  No journey to save.\n");
		return;
	}

	if (curchar->journey_active == 0) {
		log_debug("No active journey to save.\n");
		return;
	}

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "difficulty",
		json_object_new_int(curchar->j->difficulty));
	json_object_object_add(cobj, "progress",
		json_object_new_double(curchar->j->progress));

	snprintf(path, sizeof(path), "%s/journey.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No journey JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create journey JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "journey", items);
	} else {
		/* Get existing character array from JSON */
		if (!json_object_object_get_ex(root, "journey", &items)) {
			log_debug("Cannot find a [journey] array in %s\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "journey", items);
		}

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->id == json_object_get_int(id)) {
				log_debug("Update journey entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No journey entry for %s found, adding new one\n", curchar->name);
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
delete_journey(int id)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	snprintf(path, sizeof(path), "%s/journey.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No journey JSON file found\n");
		return;
	}

	json_object *journey;
	if (!json_object_object_get_ex(root, "journey", &journey)) {
		log_debug("Cannot find a [journey] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(journey);
	for (i = 0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(journey, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			json_object_array_del_idx(journey, i, 1);
			log_debug("Deleted journey entry for %d\n", id);
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

void
load_journey(int id)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	snprintf(path, sizeof(path), "%s/journey.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No journey JSON file found\n");
		return;
	}

	json_object *journey;
	if (!json_object_object_get_ex(root, "journey", &journey)) {
		log_debug("Cannot find a [journey] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(journey);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(journey, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			log_debug("Loading journey for id: %d\n", json_object_get_int(lid));

			json_object *cval;
			json_object_object_get_ex(temp, "difficulty", &cval);
			curchar->j->difficulty = json_object_get_int(cval);
			json_object_object_get_ex(temp, "progress", &cval);
			curchar->j->progress   = json_object_get_double(cval);
		}
	}

	json_object_put(root);
}

