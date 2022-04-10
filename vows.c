/*
 * Copyright (c) 2022 Matthias Schmidt <xhr@giessen.ccc.de>
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include <json-c/json.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "isscrolls.h"

void
cmd_create_new_vow(char *title)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->vow_active == 1) {
		/* Deactivate and save the current, active vow */
		cmd_deactivate_vow(NULL);
	}

	ask_for_vow_difficulty();
	curchar->vow_active = 1;

	if (title != NULL && strlen(title) > 0) {
		curchar->vow->title = calloc(1, MAX_VOW_TITLE+1);
		if (curchar->vow->title == NULL)
			log_errx(1, "calloc vow title\n");
		snprintf(curchar->vow->title, MAX_VOW_TITLE, "%s", title);
	} else {
again:
		printf("Enter a tile for your vow [max 25 chars]: ");
		curchar->vow->title = readline(NULL);
		if (strlen(curchar->vow->title) == 0) {
			printf("The title must contain at least one character\n");
			free(curchar->vow->title);
			goto again;
		}
	}

	log_debug("New vow titled '%s'\n", curchar->vow->title);

descagain:
	printf("Enter a description for your vow [max 255 chars]: ");
	curchar->vow->description = readline(NULL);
	if (strlen(curchar->vow->description) == 0) {
		printf("The description must contain at least one character\n");
		free(curchar->vow->description);
		goto descagain;
	}

	/* Every vow has an ID (vid) ... */
	curchar->vow->vid = get_max_vow_id();

	/* If we don't have any vows yet, we'll get a -1 back here */
	if (curchar->vow->vid == -1)
		curchar->vow->vid = 1;
	else
		curchar->vow->vid++;

	curchar->vid = curchar->vow->vid;
	/* ... and belongs to one character (id) */
	curchar->vow->id = curchar->id;

	curchar->vow->fulfilled = 0;

	save_vow();

	update_prompt();
}

void
cmd_deactivate_vow(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->vow_active == 0) {
		printf("No vow active.  Load one or create a new one\n");
		return;
	}

	save_vow();

	reset_vow(curchar);

	update_prompt();
}

void
cmd_activate_vow(char *cmd)
{
	struct character *curchar = get_current_character();
	char *ep;
	long lval;
	int vid = -1;

	CURCHAR_CHECK();

	lval = strtol(cmd, &ep, 10);
	if (cmd[0] == '\0' || *ep != '\0') {
		printf("Please provide a number as argument\n");
		return;
	}
	if ((errno == ERANGE || lval <= 0 || lval > MAX_VOWS)) {
		printf("Please provide a number between 1 and %d\n", MAX_VOWS);
		return;
	}

	vid = lval;

	/* Only redraw the prompt and set the vow as active if there is one */
	if (load_vow(vid) != -1) {
		curchar->vow_active = 1;
		update_prompt();
	}
}

void
cmd_forsake_your_vow(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	printf("Your vow is cleared and you have to endure stress -> Rulebook\n");
	change_char_value("spirit", DECREASE, curchar->vow->difficulty);
	cmd_delete_vow(NULL);
}

void
cmd_delete_vow(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->vow_active == 0) {
		printf("No vow active.  Nothing to forsake\n");
		return;
	}

	delete_vow(curchar->vid);
	reset_vow(curchar);

	update_prompt();
}

void
cmd_mark_vow_progress(__attribute__((unused)) char *unused)
{
	mark_vow_progress(INCREASE);
	save_vow();
}

void
cmd_reach_a_milestone(__attribute__((unused)) char *unused)
{
	cmd_mark_vow_progress(NULL);
}

void
cmd_show_all_vows(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *progress, *title, *difficulty, *vid, *id, *ff;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	ret = snprintf(path, sizeof(path), "%s/vows.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No vow JSON file found\n");
		return;
	}

	json_object *vow;
	if (!json_object_object_get_ex(root, "vow", &vow)) {
		log_debug("Cannot find a [vow] array in %s\n", path);
		return;
	}

	printf("%s's vows\n\n", curchar->name);
	printf("%-3s %-25s Progress Difficulty Fulfilled\n", "ID", "Title");
	temp_n = json_object_array_length(vow);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(vow, i);
		json_object_object_get_ex(temp, "id", &id);
		if (json_object_get_int(id) != curchar->id)
			continue;

		json_object_object_get_ex(temp, "vid", &vid);
		json_object_object_get_ex(temp, "title", &title);
		json_object_object_get_ex(temp, "progress", &progress);
		json_object_object_get_ex(temp, "difficulty", &difficulty);
		json_object_object_get_ex(temp, "fulfilled", &ff);
		printf("%-3d %-25s %.2f/10  %-10d %d\n",
			json_object_get_int(vid),
			json_object_get_string(title),
			json_object_get_double(progress),
			json_object_get_int(difficulty),
			json_object_get_int(ff));
	}

	json_object_put(root);
}

void
cmd_fulfill_your_vow(char *cmd)
{
	struct character *curchar = get_current_character();
	double dval[2] = { -1.0, -1.0 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->vow_active == 0) {
		printf("You have no active vow.  Either create or activate one\n");
		return;
	}

	dval[0] = curchar->vow->progress;
	dval[1] = get_int_from_cmd(cmd);
	ret = progress_roll(dval);
	if (ret == 8) {
		printf("Your quest is complete\n");
		change_char_value("exp", INCREASE, curchar->vow->difficulty);
	} else if (ret == 4) {
		printf("There is more to be done or you realize the truth of your quest"\
			"-> Rulebook\n");
		change_char_value("exp", INCREASE, curchar->vow->difficulty-1);
	} else {
		printf("Your quest is undone -> Rulebook\n");
	}

	curchar->vow->fulfilled = 1;

	/* Prompt will be updated in the following function */
	cmd_deactivate_vow(NULL);
}

int
get_max_vow_id()
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *vid, *id;
	size_t temp_n, i;
	int ret, tvid, max;
	tvid = max = 0;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return max;
	}

	ret = snprintf(path, sizeof(path), "%s/vows.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No vow JSON file found\n");
		return -1;
	}

	json_object *vow;
	if (!json_object_object_get_ex(root, "vow", &vow)) {
		log_debug("Cannot find a [vow] array in %s\n", path);
		return -1;
	}

	temp_n = json_object_array_length(vow);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(vow, i);
		json_object_object_get_ex(temp, "id", &id);

		/* Count only IDs for the current character */
		if (json_object_get_int(id) != curchar->id)
			continue;

		json_object_object_get_ex(temp, "vid", &vid);
		tvid = json_object_get_int(vid);
		if (tvid > max)
			max = tvid;
	}

	log_debug("Max vid is %d\n", max);

	json_object_put(root);

	return max;
}


void
reset_vow(struct character *curchar)
{
	if (curchar == NULL) {
		log_debug("curchar in reset_vow is NULL.  This should not happen\n");
		return;
	}

	if (curchar->vow->title != NULL) {
		free(curchar->vow->title);
		curchar->vow->title = NULL;
	}
	if (curchar->vow->description != NULL) {
		free(curchar->vow->description);
		curchar->vow->description = NULL;
	}

	curchar->vow->difficulty = -1;
	curchar->vow->progress = 0.0;
	curchar->vow->fulfilled = 0;
	curchar->vow->vid = 0;
	curchar->vow_active = 0;
	curchar->vid = -1;
}

void
save_vow()
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *items, *id, *vid;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("No character loaded.  No vow to save.\n");
		return;
	}

	if (curchar->vow_active == 0) {
		log_debug("No active vow to save.\n");
		return;
	}

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "vid", json_object_new_int(curchar->vow->vid));
	json_object_object_add(cobj, "fulfilled", json_object_new_int(curchar->vow->fulfilled));
	json_object_object_add(cobj, "difficulty", json_object_new_int(curchar->vow->difficulty));
	json_object_object_add(cobj, "progress", json_object_new_double(curchar->vow->progress));
	json_object_object_add(cobj, "title", json_object_new_string(curchar->vow->title));
	json_object_object_add(cobj, "description", json_object_new_string(curchar->vow->description));

	ret = snprintf(path, sizeof(path), "%s/vows.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No vow JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create vow JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "vow", items);
	} else {
		/* Get existing array from JSON */
		if (!json_object_object_get_ex(root, "vow", &items)) {
			log_debug("Cannot find a [vow] array in %s. Create one\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "vow", items);
		}

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object_object_get_ex(temp, "vid", &vid);
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->vid == json_object_get_int(vid) &&
				curchar->id == json_object_get_int(id)) {
				log_debug("Update vow entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No vow entry for %s found, adding new one\n", curchar->name);
		json_object_array_add(items, cobj);
	}

out:
	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

int
load_vow(int vid)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid, *title, *desc, *id;
	size_t temp_n, i;
	int rval, ret = -1;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return ret;
	}

	/* ID == -1 means that there is now active vow */
	if (vid == -1) {
		log_debug("Cannot load vow with vid -1\n");
		/* As a precaution we make sure that active_vow is reset */
		curchar->vow_active = 0;
		return ret;
	}

	rval = snprintf(path, sizeof(path), "%s/vows.json", get_isscrolls_dir());
	if (rval < 0 || (size_t)rval >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No vow JSON file found\n");
		return ret;
	}

	json_object *vow;
	if (!json_object_object_get_ex(root, "vow", &vow)) {
		log_debug("Cannot find a [vow] array in %s\n", path);
		return ret;
	}

	temp_n = json_object_array_length(vow);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(vow, i);
		json_object_object_get_ex(temp, "vid", &lid);
		json_object_object_get_ex(temp, "id", &id);

		/* Load only vows belonging to the current character */
		if (vid == json_object_get_int(lid) &&
			curchar->id == json_object_get_int(id)) {
			log_debug("Loading vow for id: %d\n", json_object_get_int(lid));

			curchar->vow->difficulty = validate_int(temp, "difficulty", 0, 5, 1);
			curchar->vow->fulfilled  = validate_int(temp, "fulfilled", 0, 1, 0);
			curchar->vow->progress   = validate_double(temp, "progress", 0, 10, 0);
			curchar->vow->vid 	  	 = curchar->vid = json_object_get_int(lid);

			if (curchar->vow->fulfilled) {
				printf("You cannot activate an already fulfilled vow\n");
				goto out;
			}

			json_object_object_get_ex(temp, "title", &title);
			if ((curchar->vow->title = calloc(1, MAX_VOW_TITLE+1)) == NULL)
				log_errx(1, "calloc\n");
			snprintf(curchar->vow->title, MAX_VOW_TITLE, "%s", json_object_get_string(title));

			json_object_object_get_ex(temp, "description", &desc);
			if ((curchar->vow->description = calloc(1, MAX_VOW_DESC+1)) == NULL)
				log_errx(1, "calloc\n");
			snprintf(curchar->vow->description, MAX_VOW_DESC, "%s", json_object_get_string(desc));
			ret = 1;
			goto out;
		}
	}

out:
	json_object_put(root);

	if (ret != -1)
		log_debug("Successfully loaded vow %d for id: %d\n", vid, curchar->id);

	return ret;
}

void
delete_vow(int vid)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	size_t temp_n, i;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/vows.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No vow JSON file found\n");
		return;
	}

	json_object *vow;
	if (!json_object_object_get_ex(root, "vow", &vow)) {
		log_debug("Cannot find a [vow] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(vow);
	for (i = 0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(vow, i);
		json_object_object_get_ex(temp, "vid", &lid);
		if (vid == json_object_get_int(lid)) {
			json_object_array_del_idx(vow, i, 1);
			log_debug("Deleted vow entry with vid %d\n", vid);
			break;
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

void
ask_for_vow_difficulty()
{
	struct character *curchar = get_current_character();

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	printf("Please select a rank for your vow\n\n");
	printf("1\t - Troublesome vow (3 progress)\n");
	printf("2\t - Dangerous vow (2 progress)\n");
	printf("3\t - Formidable vow (2 progress)\n");
	printf("4\t - Extreme vow (2 ticks)\n");
	printf("5\t - Epic vow (1 tick)\n\n");

	curchar->vow->difficulty = ask_for_value("Enter a value between 1 and 5: ", 5);
}

void
mark_vow_progress(int what)
{
	struct character *curchar = get_current_character();
	double amount = 0;

	if (curchar == NULL) {
		log_debug("No character loaded.  Cannot calculate progress\n");
		return;
	}

	if (curchar->vow_active == 0) {
		printf("You need to have an active vow before you can mark progress\n");
		return;
	}

	switch (curchar->vow->difficulty) {
	case 1:
		amount = 3.0;
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
		curchar->vow->difficulty = 1;
		log_errx(1, "Unknown rank.  This should not happen.  Set to 1\n");
	}

	if (what == INCREASE)
		curchar->vow->progress += amount;
	else
		curchar->vow->progress -= amount;

	if (curchar->vow->progress > 10) {
		printf("Your vow progress is full.  Consider fulfilling it\n");
		curchar->vow->progress = 10;
	} else if (curchar->vow->progress < 0)
		curchar->vow->progress = 0;

	update_prompt();
}

