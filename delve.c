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
	int ret;

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
	} else if (strcasecmp(stat, "shadow") == 0) {
		ival[0] = curchar->shadow;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->heart;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You mark progress and delve deeper\n");
		// XXX Find and opportunity
		mark_delve_progress();
	} else if (ret == 4) {
		// XXX Roll table against stat
		printf("XXX TBD\n");
	} else
		// XXX Reveal a danger
		printf("XXX TBD\n");

	update_prompt();
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
		printf("Your reached all milestones of your delve.  Consider ending it\n");
		curchar->j->progress = 10;
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
		json_object_new_int(curchar->j->difficulty));
	json_object_object_add(cobj, "progress",
		json_object_new_double(curchar->j->progress));

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
			curchar->j->difficulty = json_object_get_int(cval);
			json_object_object_get_ex(temp, "progress", &cval);
			curchar->j->progress   = json_object_get_double(cval);
		}
	}

	json_object_put(root);
}

