/*
 * Copyright (c) 2021-22 Matthias Schmidt <xhr@giessen.ccc.de>
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

#include <sys/queue.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "isscrolls.h"

static struct character *curchar = NULL;
static LIST_HEAD(listhead, entry) head = LIST_HEAD_INITIALIZER(head);

void
cmd_create_character(char *name)
{
	struct entry *e;
	struct character *c;
	char p[MAX_PROMPT_LEN];

	/* There is already a character loaded, so save and free it */
	if (curchar != NULL) {
		save_character();
		free_character();
		curchar = NULL;
	}

	if (character_exists(name)) {
		printf("Sorry, there is already a character named %s\n", name);
		return;
	}

	log_debug("Attempt to create a character named %s\n", name);
	if ((c = create_character(name)) != NULL) {
		curchar = c;
		print_character();
		snprintf(p, sizeof(p), "%s > ", c->name);
		set_prompt(p);

		if ((e = malloc(sizeof(struct entry))) == NULL)
			log_errx(1, "cannot allocate memory\n");

		e->id = c->id;
		snprintf(e->name, sizeof(e->name), "%s", c->name);
		LIST_INSERT_HEAD(&head, e, entries);
	}
}

void
cmd_ls(__attribute__((unused)) char *unused)
{
	struct entry *np;

	LIST_FOREACH(np, &head, entries) {
		printf("%s\n", np->name);
	}
}

void
cmd_save(__attribute__((unused)) char *unused)
{
	save_character();
}

void
cmd_delete_character(__attribute__((unused)) char *unused)
{
	struct entry *np = NULL;

	CURCHAR_CHECK();

	/* Save list entry of the to be deleted char in np */
	LIST_FOREACH(np, &head, entries) {
		if (np->id == curchar->id)
			break;
	}

	delete_saved_character(curchar->id);

	free_character();
	curchar = NULL;

	if (np != NULL) {
		LIST_REMOVE(np, entries);
		free(np);
	} else
		log_debug("Found a list entry but cannot delete it\n");

	set_prompt("> ");
}

void
cmd_cds(char *character)
{
	cmd_cd(character);
	cmd_show_all_vows(NULL);
}

void
cmd_cd(char *character)
{
	int id = -1;

	if (character == NULL)
		return;

	if (strlen(character) == 0 && curchar != NULL) {
		/* We got no argument and there is a character loaded */
		log_debug("Switch to ~ and save character %s\n", character);
		set_prompt("> ");
		save_character();
		unset_last_loaded_character();
		free_character();
		curchar = NULL;
	} else if (strlen(character) == 0 && curchar == NULL) {
		/* We got no argument and there is no character loaded */
		printf("Provide the name of a character as argument\n\n");
		printf("You currently have the following characters:\n");
		cmd_ls(NULL);
		return;
	} else if (strlen(character) != 0 && curchar == NULL) {
		/* We got an argument and there is no character loaded */
		id = return_character_id(character);
		if (id != -1) {
			if (load_character(id) == -1) {
				log_debug("No character object for %s with ID %d\n", character, id);
				return;
			}
		} else
			printf("No character named %s found.\n", character);
	} else if (strlen(character) != 0 && curchar != NULL) {
		/* We got an argument and there is a character loaded */
		id = return_character_id(character);
		if (id != -1) {
			save_character();
			free_character();
			curchar = NULL;

			if (load_character(id) == -1) {
				log_debug("No character object for %s with ID %d\n", character, id);
				return;
			}
		} else
			printf("No character named %s found.\n", character);
	}
}

void
cmd_increase_value(char *value)
{
	change_char_value(value, INCREASE, 1);
}

void
cmd_decrease_value(char *value)
{
	change_char_value(value, DECREASE, 1);
}

void
cmd_toggle(char *value)
{
	CURCHAR_CHECK();

	if (value == NULL || strlen(value) == 0) {
info:
		printf("Please specify the stat you want to toggle\n");
		printf("\nExample: toggle wounded\n");
		printf("\nYou can toggle the following values:\n\n");
		printf("-Wounded\n-Unprepared\n-Shaken\n-Encumbered\n-Maimed\n-Cursed\n");
		printf("-Corrupted\n-Tormented\n-Battlescarred\n");
		return;
	}

	if (strcasecmp(value, "wounded") == 0) {
		toggle_value(value, &curchar->wounded);
	} else if (strcasecmp(value, "unprepared") == 0) {
		toggle_value(value, &curchar->unprepared);
	} else if (strcasecmp(value, "shaken") == 0) {
		toggle_value(value, &curchar->shaken);
	} else if (strcasecmp(value, "encumbered") == 0) {
		toggle_value(value, &curchar->encumbered);
	} else if (strcasecmp(value, "battlescarred") == 0) {
		/* The battle scarred resource can only be used if the maimed condition
		 * was set to true in the past */
		if (!curchar->maimed) {
			printf("You need to be 'maimed' to use 'Battle scarred'\n");
			return;
		}
		toggle_value(value, &curchar->battle_scarred);
	} else if (strcasecmp(value, "maimed") == 0) {
		if (curchar->maimed) {
			printf("Maimed is a permanent bane and cannot be changed\n");
			return;
		}
		toggle_value(value, &curchar->maimed);
	} else if (strcasecmp(value, "cursed") == 0) {
		toggle_value(value, &curchar->cursed);
	} else if (strcasecmp(value, "corrupted") == 0) {
		if (curchar->corrupted) {
			printf("Corrupted is a permanent bane and cannot be changed\n");
			return;
		}
		toggle_value(value, &curchar->corrupted);
	} else if (strcasecmp(value, "tormented") == 0) {
		toggle_value(value, &curchar->tormented);
	} else
		goto info;

	set_max_momentum();
}

void
cmd_learn_from_your_failures(__attribute__((unused)) char *unused)
{
	double dval[2] = { -1.0, -1.0 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->failure_track < 6) {
		printf("You didn't fail enough, yet!\n");
		return;
	}

	dval[0] = curchar->failure_track;
	dval[1] = -1;
	toggle_output();
	ret = progress_roll(dval);
	toggle_output();
	if (ret == 8 || ret == 18) {
		change_char_value("exp", INCREASE, 3);
		printf("You commit to make a dramatic change. Choose one option -> "\
			"Rulebook\n");
	} else if (ret == 4 || ret == 14) {
		change_char_value("exp", INCREASE, 2);
		printf("You learn from your mistakes\n");
	} else if (ret == 2 || ret == 12) {
		change_char_value("exp", INCREASE, 1);
		printf("Envision on how you set off an ill-fated path\n");
	}

	curchar->failure_track = 0.0;
	printf("Failure track reset\n");
}

int
return_char_stat(const char *stat, int mask)
{
	if (curchar == NULL) {
		log_debug("No character loaded.\n");
		return -1;
	}

	if (mask == 0) {
		log_debug("Empty mask. This should not happen\n");
		return -1;
	}

	if (strcasecmp(stat, "wits") == 0 && (mask & STAT_WITS)) {
		return curchar->wits;
	} else if (strcasecmp(stat, "shadow") == 0 && (mask & STAT_SHADOW)) {
		return curchar->shadow;
	} else if (strcasecmp(stat, "edge") == 0 && (mask & STAT_EDGE)) {
		return curchar->edge;
	} else if (strcasecmp(stat, "iron") == 0 && (mask & STAT_IRON)) {
		return curchar->iron;
	} else if (strcasecmp(stat, "heart") == 0 && (mask & STAT_HEART)) {
		return curchar->heart;
	} else
		return -1;
}

void
update_prompt(void)
{
	char p[MAX_PROMPT_LEN];
	char j[MAX_PROMPT_LEN];
	char f[MAX_PROMPT_LEN];
	char d[MAX_PROMPT_LEN];
	char v[MAX_PROMPT_LEN];
	char e[MAX_PROMPT_LEN];
	char i[5];

	CURCHAR_CHECK();

	j[0] = f[0] = d[0] = i[0] = v[0] = e[0] = '\0';

	/* Only show the vow's title in color mode.  Less noise for braille
	 * displays and screen readers */
	if (get_color()) {
		if (curchar->vow_active) {
			if (curchar->vow->difficulty < 4)
				snprintf(v, sizeof(v), " [%s %.0f]",
					curchar->vow->title, curchar->vow->progress);
			else
				snprintf(v, sizeof(v), " [%s %.2f]",
					curchar->vow->title, curchar->vow->progress);
		}
	} else {
		if (curchar->vow_active) {
				if (curchar->vow->difficulty < 4)
					snprintf(v, sizeof(v), " [%d, %.0f]",
						curchar->vow->vid, curchar->vow->progress);
				else
					snprintf(v, sizeof(v), " [%d, %.2f]",
						curchar->vow->vid, curchar->vow->progress);
			}
	}

	if (curchar->journey_active) {
		if (curchar->j->difficulty < 4)
			snprintf(j, sizeof(j), "Journey %.0f > ",
				curchar->j->progress);
		else
			snprintf(j, sizeof(j), "Journey %.2f > ",
				curchar->j->progress);
	}

	if (curchar->expedition_active) {
		if (curchar->expedition->difficulty < 4)
			snprintf(e, sizeof(e), "Expedition %.0f > ",
				curchar->expedition->progress);
		else
			snprintf(e, sizeof(e), "Expedition %.2f > ",
				curchar->expedition->progress);
	}

	if (curchar->delve_active) {
		if (curchar->delve->difficulty < 4)
			snprintf(d, sizeof(d), "Delve %.0f > ",
				curchar->delve->progress);
		else
			snprintf(d, sizeof(d), "Delve %.2f > ",
				curchar->delve->progress);
	}

	if (curchar->fight_active) {
		if (curchar->fight->initiative)
			snprintf(i, 5, "%s", " [I]");

		if (curchar->fight->difficulty < 4)
			snprintf(f, sizeof(f), "Fight %.0f%s > ",
				curchar->fight->progress, i);
		else
			snprintf(f, sizeof(f), "Fight %.2f%s > ",
				curchar->fight->progress, i);
	}

	snprintf(p, sizeof(p), "%s%s > %s%s%s%s", curchar->name, v, j, e, d, f);

	set_prompt(p);
}

void
toggle_value(const char *desc, int *value)
{
	int new = !(*value);

	CURCHAR_CHECK();

	printf("Toggle %s from %d to %d\n", desc, *value, new);
	*value = new;
}

void
set_max_momentum(void)
{
	int mm, bs;

	if (curchar == NULL)
		return;

	/* The second option of the Battle scarred resources neutralizes the
	 * maimed debility */
	if (curchar->battle_scarred)
		bs = 0;
	else
		bs = curchar->maimed;

	/* Max momentum is 10 minus all the debilities */
	mm = 10 - curchar->wounded - curchar->unprepared - curchar->shaken -
		curchar->encumbered - bs -
		curchar->cursed - curchar->corrupted - curchar->tormented;

	if (mm != curchar->max_momentum) {
		printf("Your max momentum changed from %d to %d\n",
			curchar->max_momentum, mm);
		curchar->max_momentum = mm;
	}

	/* Reset momentum is +2 and reduced by 1 for each debility.  It cannot fall
	 * lower than 0
	 */
	mm = 2 - curchar->wounded - curchar->unprepared - curchar->shaken -
		curchar->encumbered - bs -
		curchar->cursed - curchar->corrupted - curchar->tormented;

	if (mm < 0)
		mm = 0;
	if (mm != curchar->momentum_reset) {
		printf("Your reset momentum changed from %d to %d\n",
			curchar->momentum_reset, mm);
		curchar->momentum_reset = mm;
	}

}

void
change_char_value(const char *value, int what, int howmany)
{
	const char *event[2] = { "increase", "decrease" };

	CURCHAR_CHECK();

	if (value == NULL || strlen(value) == 0) {
change_info:
		printf("Please specify the stat you want to %s\n", event[what]);
		printf("\nExample: %s wits\t- %s 'wits' by 1\n", event[what], event[what]);
		printf("\nYou can change the following values:\n\n");
		printf("-Edge\n-Heart\n-Iron\n-Shadow\n-Wits\n-Momentum\n-Health\n-Spirit\n");
		printf("-Supply\n-Exp\n-expspent\n-Weapon\n");
		return;
	}

	if (strcasecmp(value, "edge") == 0) {
		modify_value(value, &curchar->edge, 4, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "heart") == 0) {
		modify_value(value, &curchar->heart, 4, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "iron") == 0) {
		modify_value(value, &curchar->iron, 4, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "shadow") == 0) {
		modify_value(value, &curchar->shadow, 4, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "wits") == 0) {
		modify_value(value, &curchar->wits, 4, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "exp") == 0) {
		modify_value(value, &curchar->exp, 30, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "expspent") == 0) {
		modify_value(value, &curchar->exp_used, curchar->exp, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "weapon") == 0) {
		modify_value(value, &curchar->weapon, 2, 1, howmany, what);
		return;
	} else if (strcasecmp(value, "failure") == 0) {
		modify_double(value, &curchar->failure_track, 10.0, 0.0, 0.25, what);
		return;
	} else if (strcasecmp(value, "momentum") == 0) {
		if (curchar->momentum == -6 && what == DECREASE) {
			printf("You cannot decrease your momentum since you're at the minimum\n");
			printf("You must roll the \'Face a Setback\' move\n");
			return;
		}
		modify_value(value, &curchar->momentum, curchar->max_momentum, -6,
			howmany, what);
		return;
	} else if (strcasecmp(value, "health") == 0) {
		if (curchar->wounded) {
			printf("You are wounded, you cannot increase health\n");
			return;
		}
		modify_value(value, &curchar->health, 5, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "spirit") == 0) {
		if (curchar->shaken) {
			printf("You are shaken, you cannot increase spirit\n");
			return;
		}
		modify_value(value, &curchar->spirit, 5, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "supply") == 0) {
		if (curchar->unprepared) {
			printf("You are unprepared, you cannot increase supply\n");
			return;
		}
		modify_value(value, &curchar->supply, 5, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "progress") == 0) {
		/* Order of increasing progress is as follows:
		 *
		 * fight > delve > journey
		 *
		 * That's the reason for the returns here.
		 */
		if (curchar->fight_active) {
			mark_fight_progress(what);
			return;
		}
		if (curchar->delve_active) {
			mark_delve_progress(what);
			return;
		}
		if (curchar->journey_active)
			mark_journey_progress(what);
	} else {
		goto change_info;
	}

}

void
modify_value(const char *str, int *value, int max, int min, int howmany,
	int what)
{
	if (what == 0) {
		if (*value >= max)
			return;
		else if ((*value + howmany) >= max)
			*value = max;
		else
			*value += howmany;

		printf("Increasing %s from %d to %d\n", str, *value - howmany, *value);
	} else {
		if (*value <= min)
			return;
		else if ((*value - howmany) <= min)
			*value = min;
		else
			*value -= howmany;
		printf("Decreasing %s from %d to %d\n", str, *value + howmany, *value);
	}
}

void
modify_double(const char *str, double *value, double max, double min, double howmany,
	int what)
{
	if (what == 0) {
		if (*value >= max)
			return;
		else if ((*value + howmany) >= max)
			*value = max;
		else
			*value += howmany;

		if (get_output())
			printf("Increasing %s from %.2f to %.2f\n", str, *value - howmany, *value);
	} else {
		if (*value <= min)
			return;
		else if ((*value - howmany) <= min)
			*value = min;
		else
			*value -= howmany;
		if (get_output())
			printf("Decreasing %s from %.2f to %.2f\n", str, *value + howmany, *value);
	}
}

int
return_character_id(const char *name)
{
	struct entry *np;
	int id = -1;

	LIST_FOREACH(np, &head, entries) {
		if (strcasecmp(np->name, name) == 0) {
			id = np->id;
		}
	}

	return id;
}

void
save_current_character(void)
{
	save_character();
}

void
save_character(void)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *items;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("Nothing to save here\n");
		return;
	}

	save_journey();
	save_fight();
	save_delve();
	save_expedition();
	save_vow();

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "name", json_object_new_string(curchar->name));
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "vid", json_object_new_int(curchar->vid));
	json_object_object_add(cobj, "edge", json_object_new_int(curchar->edge));
	json_object_object_add(cobj, "heart", json_object_new_int(curchar->heart));
	json_object_object_add(cobj, "iron", json_object_new_int(curchar->iron));
	json_object_object_add(cobj, "shadow", json_object_new_int(curchar->shadow));
	json_object_object_add(cobj, "wits", json_object_new_int(curchar->wits));
	json_object_object_add(cobj, "exp", json_object_new_int(curchar->exp));
	json_object_object_add(cobj, "momentum",
		json_object_new_int(curchar->momentum));
	json_object_object_add(cobj, "max_momentum",
		json_object_new_int(curchar->max_momentum));
	json_object_object_add(cobj, "momentum_reset",
		json_object_new_int(curchar->momentum_reset));
	json_object_object_add(cobj, "health", json_object_new_int(curchar->health));
	json_object_object_add(cobj, "spirit", json_object_new_int(curchar->spirit));
	json_object_object_add(cobj, "supply", json_object_new_int(curchar->supply));
	json_object_object_add(cobj, "wounded",
		json_object_new_int(curchar->wounded));
	json_object_object_add(cobj, "unprepared",
		json_object_new_int(curchar->unprepared));
	json_object_object_add(cobj, "shaken",
			json_object_new_int(curchar->shaken));
	json_object_object_add(cobj, "encumbered",
		json_object_new_int(curchar->encumbered));
	json_object_object_add(cobj, "maimed", json_object_new_int(curchar->maimed));
	json_object_object_add(cobj, "battle_scarred", json_object_new_int(curchar->battle_scarred));
	json_object_object_add(cobj, "cursed", json_object_new_int(curchar->cursed));
	json_object_object_add(cobj, "dead", json_object_new_int(curchar->dead));
	json_object_object_add(cobj, "weapon", json_object_new_int(curchar->weapon));
	json_object_object_add(cobj, "strong_hit", json_object_new_int(curchar->strong_hit));
	json_object_object_add(cobj, "corrupted",
		json_object_new_int(curchar->corrupted));
	json_object_object_add(cobj, "tormented",
		json_object_new_int(curchar->tormented));
	json_object_object_add(cobj, "exp_used",
		json_object_new_int(curchar->exp_used));
	json_object_object_add(cobj, "bonds",
		json_object_new_double(curchar->bonds));
	json_object_object_add(cobj, "failure_track",
		json_object_new_double(curchar->failure_track));
	json_object_object_add(cobj, "journey_active",
		json_object_new_int(curchar->journey_active));
	json_object_object_add(cobj, "fight_active",
		json_object_new_int(curchar->fight_active));
	json_object_object_add(cobj, "delve_active",
		json_object_new_int(curchar->delve_active));
	json_object_object_add(cobj, "vow_active",
		json_object_new_int(curchar->vow_active));
	json_object_object_add(cobj, "expedition_active",
		json_object_new_int(curchar->expedition_active));

	ret = snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No character JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "characters", items);
		json_object_object_add(root, "last_used", json_object_new_int(curchar->id));
	} else {
		/* Get existing character array from JSON */
		if (!json_object_object_get_ex(root, "characters", &items)) {
			log_debug("Cannot find a [characters] array in %s\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "characters", items);
		}

		json_object_object_add(root, "last_used", json_object_new_int(curchar->id));

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object *id;
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->id == json_object_get_int(id)) {
				log_debug("Update character entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No entry for %s found, adding new one\n", curchar->name);
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
unset_last_loaded_character(void)
{
	char path[_POSIX_PATH_MAX];
	json_object *root;
	int ret;

	if (curchar == NULL) {
		log_debug("Nothing to unset here\n");
		return;
	}

	ret = snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	/* Just set the last_used character to 0 */
	if ((root = json_object_from_file(path)) == NULL)
		return;
	else
		json_object_object_add(root, "last_used", json_object_new_int(0));

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);

	json_object_put(root);
}

void
delete_saved_character(int id)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	size_t temp_n, i;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No character JSON file found\n");
		return;
	}

	json_object *characters;
	if (!json_object_object_get_ex(root, "characters", &characters)) {
		log_debug("Cannot find a [characters] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(characters);
	for (i = 0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(characters, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			json_object_array_del_idx(characters, i, 1);
			log_debug("Deleted character entry for %d\n", id);
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

int
load_characters_list(void)
{
	struct entry *e;
	char path[_POSIX_PATH_MAX];
	json_object *root;
	json_object *lid, *name;
	size_t temp_n, i;
	int ret, last_id = -1, found = 0;

	LIST_INIT(&head);

	ret = snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No character JSON file found\n");
		return -1;
	}

	json_object *last_used;
	if (!json_object_object_get_ex(root, "last_used", &last_used)) {
		log_debug("No previously loaded character\n");
	} else {
		last_id = json_object_get_int(last_used);
		log_debug("Previously loaded character: %d\n", last_id);
	}

	json_object *characters;
	if (!json_object_object_get_ex(root, "characters", &characters)) {
		log_debug("Cannot find a [characters] array in %s\n", path);
		return -1;
	}
	temp_n = json_object_array_length(characters);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(characters, i);
		json_object_object_get_ex(temp, "id", &lid);
		json_object_object_get_ex(temp, "name", &name);
		log_debug("Add %s to list with id: %d\n", json_object_get_string(name), json_object_get_int(lid));

		if ((e = malloc(sizeof(struct entry))) == NULL)
			log_errx(1, "cannot allocate memory\n");

		e->id = json_object_get_int(lid);

		/* If there is a last loaded character in the JSON, make sure it is also
		 * in the list of existing characters.  This prevents that a broken ID
		 * is loaded later */
		if (e->id == last_id)
			found = 1;

		snprintf(e->name, sizeof(e->name), "%s", json_object_get_string(name));
		LIST_INSERT_HEAD(&head, e, entries);
	}

	json_object_put(root);

	if (last_id != -1 && found == 1) {
		if (load_character(last_id) == -1)
			return -1;
	} else
		return -1;

	return 0;
}

int
load_character(int id)
{
	struct character *c;
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid, *name;
	size_t temp_n, i;
	int ret;

	if (id <= 0)
		return -1;

	ret = snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No character JSON file found\n");
		return -1;
	}

	if ((c = calloc(1, sizeof(struct character))) == NULL)
		log_errx(1, "calloc");

	if ((c->name = calloc(1, MAX_CHAR_LEN)) == NULL)
		log_errx(1, "calloc");

	if ((c->j = calloc(1, sizeof(struct journey))) == NULL)
		log_errx(1, "calloc");

	if ((c->fight = calloc(1, sizeof(struct fight))) == NULL)
		log_errx(1, "calloc");

	if ((c->delve = calloc(1, sizeof(struct delve))) == NULL)
		log_errx(1, "calloc");

	if ((c->vow = calloc(1, sizeof(struct vow))) == NULL)
		log_errx(1, "calloc");

	if ((c->expedition = calloc(1, sizeof(struct expedition))) == NULL)
		log_errx(1, "calloc");

	json_object *characters;
	if (!json_object_object_get_ex(root, "characters", &characters)) {
		log_debug("Cannot find a [characters] array in %s\n", path);
		free(c->name);
		c->name = NULL;
		free(c->j);
		c->j = NULL;
		free(c->fight);
		c->fight = NULL;
		free(c->delve);
		c->delve = NULL;
		free(c->vow);
		c->vow = NULL;
		free(c->expedition);
		c->expedition = NULL;
		free(c);
		c = NULL;
		return -1;
	}

	temp_n = json_object_array_length(characters);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(characters, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			json_object_object_get_ex(temp, "name", &name);

			log_debug("Loading character %s, id: %d\n", json_object_get_string(name),
				json_object_get_int(lid));

			snprintf(c->name, MAX_CHAR_LEN, "%s", json_object_get_string(name));
			c->id		 = id;
			c->vid = validate_int(temp, "vid", -1, INT_MAX, -1);
			c->edge = validate_int(temp, "edge", 0, 5, 1);
			c->heart = validate_int(temp, "heart", 0, 5, 1);
			c->iron = validate_int(temp, "iron", 0, 5, 1);
			c->shadow = validate_int(temp, "shadow", 0, 5, 1);
			c->wits = validate_int(temp, "wits", 0, 5, 1);
			c->exp = validate_int(temp, "exp", 0, 30, 0);
			c->health = validate_int(temp, "health", 0, 5, 5);
			c->spirit = validate_int(temp, "spirit", 0, 5, 5);
			c->supply = validate_int(temp, "supply", 0, 5, 5);
			c->wounded = validate_int(temp, "wounded", 0, 1, 0);
			c->shaken = validate_int(temp, "shaken", 0, 1, 0);
			c->maimed = validate_int(temp, "maimed", 0, 1, 0);
			c->battle_scarred = validate_int(temp, "battle_scarred", 0, 1, 0);
			c->cursed = validate_int(temp, "cursed", 0, 1, 0);
			c->dead = validate_int(temp, "dead", 0, 1, 0);
			c->weapon = validate_int(temp, "weapon", 1, 2, 1);
			c->bonds = validate_double(temp, "bonds", 0, 5, 1);
			c->corrupted = validate_int(temp, "corrupted", 0, 1, 0);
			c->tormented = validate_int(temp, "tormented", 0, 1, 0);
			c->exp_used = validate_int(temp, "exp_used", 0, 30, 0);
			c->unprepared = validate_int(temp, "unprepared", 0, 1, 0);
			c->momentum = validate_int(temp, "momentum", -6, 10, 2);
			c->encumbered = validate_int(temp, "encumbered", 0, 1, 0);
			c->max_momentum = validate_int(temp, "max_momentum", -6, 10, 2);
			c->momentum_reset = validate_int(temp, "momentum_reset", -6, 2, 2);
			c->journey_active = validate_int(temp, "journey_active", 0, 1, 0);
			c->fight_active = validate_int(temp, "fight_active", 0, 1, 0);
			c->delve_active = validate_int(temp, "delve_active", 0, 1, 0);
			c->vow_active = validate_int(temp, "vow_active", 0, 1, 0);
			c->expedition_active = validate_int(temp, "expedition_active", 0, 1, 0);
			c->strong_hit = validate_int(temp, "strong_hit", 0, 1, 0);
			c->failure_track = validate_double(temp, "failure_track", 0.0, 10.0, 0.0);
		}
	}

	curchar = c;

	load_journey(c->id);
	load_fight(c->id);
	load_delve(c->id);
	load_expedition(c->id);

	if (load_vow(c->vid) == -1)
		curchar->vow_active = 0;

	update_prompt();
	print_character();

	json_object_put(root);

	return 0;
}

int
validate_int(json_object *jobj, const char *desc, int min, int max, int def)
{
	json_object *cval;
	int value;

	if (jobj == NULL) {
		log_debug("Empty JSON object for %s.  Using default\n", desc);
		return def;
	}

	if (!json_object_object_get_ex(jobj, desc, &cval)) {
		log_debug("Cannot get value for %s from JSON.  Using default\n", desc);
		return def;
	}

	value = json_object_get_int(cval);

	if (value < min || value > max) {
		printf("[-] Error.  Value for %s (%d) is out of range [%d, %d]\n",
			desc, value, min, max);
		printf("[-] Resetting to a default value: %d\n", def);
		printf("\n[-] If you think this is a bug, please open an issue at\n");
		printf("https://github.com/thexhr/isscrolls/issues and describe why\n");
		printf("you think it is a bug\n");
		return def;
	}

	return value;
}

double
validate_double(json_object *jobj, const char *desc,
	double min, double max, double def)
{
	json_object *cval;
	double value;

	if (jobj == NULL) {
		log_debug("Empty JSON object for %s.  Using default\n", desc);
		return def;
	}

	if (!json_object_object_get_ex(jobj, desc, &cval)) {
		log_debug("Cannot get value for %s from JSON.  Using default\n", desc);
		return def;
	}

	value = json_object_get_double(cval);

	if (value < min || value > max) {
		printf("[-] Error.  Value for %s (%.2f) is out of range [%.2f, %.2f]\n",
			desc, value, min, max);
		printf("[-] Resetting to a default value: %.2f\n", def);
		printf("\n[-] If you think this is a bug, please open an issue at\n");
		printf("https://github.com/thexhr/isscrolls/issues and describe why\n");
		printf("you think it is a bug.\n");
		return def;
	}

	return value;
}

void
cmd_mark_progress(__attribute__((unused)) char *unused)
{
	CURCHAR_CHECK();

	if (curchar->fight_active) {
		mark_fight_progress(INCREASE);
		return;
	}
	if (curchar->delve_active) {
		mark_delve_progress(INCREASE);
		return;
	}
	if (curchar->journey_active)
		mark_journey_progress(INCREASE);
}

void
cmd_print_current_character(__attribute__((unused)) char *unused)
{
	CURCHAR_CHECK();

	print_character();
}

void
print_character(void)
{
	static const char *wp;

	CURCHAR_CHECK();

	log_debug("Character ID: %d\n", curchar->id);
	printf("Name: %s (Exp: %d/30) Exp spent: %d ", curchar->name,
		curchar->exp, curchar->exp_used);
	if (curchar->failure_track > 0.0)
		printf("Failure Track: %.2f/10", curchar->failure_track);
	if (curchar->dead)
		printf("[DECEASED]\n");
	else
		printf("\n");

	printf("\nEdge: %d Heart: %d Iron: %d Shadow: %d Wits: %d\n\n",
		curchar->edge, curchar->heart, curchar->iron, curchar->shadow, curchar->wits);
	printf("Momentum: %d/%d [%d] Health: %d/5 Spirit: %d/5 Supply: %d/5\n",
		curchar->momentum, curchar-> max_momentum, curchar->momentum_reset,
		curchar->health, curchar->spirit, curchar->supply);

	printf("\nWounded:\t%d Unprepared:\t%d Encumbered:\t%d Shaken:\t%d\n",
		curchar->wounded, curchar->unprepared, curchar->encumbered, curchar->shaken);
	printf("Corrupted:\t%d Tormented:\t%d Cursed:\t%d Maimed:\t%d\n",
		curchar->corrupted, curchar->tormented, curchar->cursed, curchar->maimed);

	if (curchar->weapon == 2)
		wp = "deadly";
	else
		wp = "simple";

	printf("\nArmed with a %s weapon\n", wp);
	printf("\nBonds: %.2f\n", curchar->bonds);

	if (curchar->journey_active) {
		printf("\nActive Journey: Difficulty: %d Progress: %.2f/10\n",
			curchar->j->difficulty, curchar->j->progress);
	}
	if (curchar->fight_active) {
		printf("\nActive Fight: Difficulty: %d Progress: %.2f/10\n",
			curchar->fight->difficulty, curchar->fight->progress);
	}
	if (curchar->delve_active) {
		printf("\nActive delve: Difficulty: %d Progress: %.2f/10\n",
			curchar->delve->difficulty, curchar->delve->progress);
	}
	if (curchar->vow_active) {
		printf("\nActive vow : %s Difficulty: %d Progress: %.2f/10\n\n"\
			"Description: %s\n",
			curchar->vow->title, curchar->vow->difficulty,
			curchar->vow->progress, curchar->vow->description);
	}
}

void
ask_for_journey_difficulty(void)
{
	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	printf("Please set a rank for your journey\n\n");
	printf("1\t - Troublesome journey (3 progress per waypoint)\n");
	printf("2\t - Dangerous journey (2 progress per waypoint)\n");
	printf("3\t - Formidable journey (2 progress per waypoint)\n");
	printf("4\t - Extreme journey (2 ticks per waypoint)\n");
	printf("5\t - Epic journey (1 tick per waypoint)\n\n");

	curchar->j->difficulty = ask_for_value("Enter a value between 1 and 5: ", 5);
}

int
validate_range(int temp, int max)
{
	if (temp < 1 || temp > max) {
		printf("Invalid range. The value has to be between 1 and %d\n", max);
		return -1;
	}

	return 0;
}

void
free_character(void)
{
	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	if (curchar->name != NULL) {
		free(curchar->name);
		curchar->name = NULL;
	}
	if (curchar->j != NULL) {
		free(curchar->j);
		curchar->j = NULL;
	}
	if (curchar->fight != NULL) {
		free(curchar->fight);
		curchar->fight = NULL;
	}
	if (curchar->delve != NULL) {
		free(curchar->delve);
		curchar->delve = NULL;
	}
	if (curchar->expedition != NULL) {
		free(curchar->expedition);
		curchar->expedition = NULL;
	}
	if (curchar->vow->title != NULL) {
		free(curchar->vow->title);
		curchar->vow->title = NULL;
	}
	if (curchar->vow->description != NULL) {
		free(curchar->vow->description);
		curchar->vow->description = NULL;
	}
	if (curchar->vow != NULL) {
		free(curchar->vow);
		curchar->vow= NULL;
	}
	if (curchar != NULL) {
		free(curchar);
		curchar = NULL;
	}
}

int
ask_for_value(const char *attribute, int max)
{
	char *line;
	int temp = -1;

again:
	line = readline(attribute);
	if (line == NULL)
		goto again;

	temp = atoi(line);
	if (validate_range(temp, max) == -1)	{
		goto again;
	}

	free(line);
	return temp;
}

int
character_exists(const char *name)
{
	struct entry *np;

	if (name == NULL)
		return 0;

	if (strlen(name) == 0)
		return 0;

	LIST_FOREACH(np, &head, entries) {
		if (strcasecmp(name, np->name) == 0) {
			return 1;
		}
	}

	return 0;
}

struct character *
create_character(const char *name)
{
	struct character *c;

	c = init_character_struct();

	if (strlen(name) == 0) {
		printf("Enter a name for your character: ");
		c->name = readline(NULL);
		if (c->name != NULL && strlen(c->name) == 0) {
			printf("Please provide a longer name\n");
			free_character();
			return NULL;
		}
		if (character_exists(c->name)) {
			printf("Sorry, there is already a character named %s\n", c->name);
			free_character();
			return NULL;
		}
	} else {
		if ((c->name = calloc(1, MAX_CHAR_LEN)) == NULL)
			log_errx(1, "calloc");
		snprintf(c->name, MAX_CHAR_LEN, "%s", name);
		printf("Creating a character named %s\n", c->name);
	}

	printf("Now distribute the following values to your attributes:\n");
	printf("  - Challenging difficulty\t: 4,3,3,2,2\n");
	printf("  - Perilous difficulty\t\t: 3,2,2,1,1\n");
	printf("  - Grim difficulty\t\t: 3,2,1,1,0\n");

	c->edge   = ask_for_value("Edge   : ", 4);
	c->heart  = ask_for_value("Heart  : ", 4);
	c->iron   = ask_for_value("Iron   : ", 4);
	c->wits   = ask_for_value("Wits   : ", 4);
	c->shadow = ask_for_value("Shadow : ", 4);

	return c;
}

struct character *
init_character_struct(void)
{
	struct character *c;

	if ((c = calloc(1, sizeof(struct character))) == NULL)
		log_errx(1, "calloc");

	if ((c->j = calloc(1, sizeof(struct journey))) == NULL)
		log_errx(1, "calloc");

	if ((c->fight = calloc(1, sizeof(struct fight))) == NULL)
		log_errx(1, "calloc");

	if ((c->delve = calloc(1, sizeof(struct delve))) == NULL)
		log_errx(1, "calloc");

	if ((c->expedition = calloc(1, sizeof(struct expedition))) == NULL)
		log_errx(1, "calloc");

	if ((c->vow= calloc(1, sizeof(struct vow))) == NULL)
		log_errx(1, "calloc");

	c->id = random();
	c->name = NULL;
	c->edge = c->heart = c->iron = c->shadow = c->wits = c->exp = 0;
	c->momentum = c->momentum_reset = 2;
	c->max_momentum = 10;
	c->health = c->spirit = c->supply = 5;
	c->wounded = c->unprepared = c->shaken = c->encumbered = c->maimed = 0;
	c->cursed = c->corrupted = c->tormented = c->exp_used = c->bonds = 0;
	c->dead = c->battle_scarred = c->strong_hit = 0;
	c->failure_track = 0.0;
	c->weapon = 1;

	c->j->id = c->id;
	c->j->difficulty = -1;
	c->j->progress = 0.0;
	c->journey_active = 0;

	c->fight->id = c->id;
	c->fight->difficulty = -1;
	c->fight->progress = 0.0;
	c->fight->initiative = 0;
	c->fight_active = 0;

	c->delve->id = c->id;
	c->delve->difficulty = -1;
	c->delve->progress = 0.0;
	c->delve_active = 0;

	c->expedition->id = c->id;
	c->expedition->difficulty = -1;
	c->expedition->progress = 0.0;
	c->expedition_active = 0;

	c->vow->id = c->id;
	c->vow->difficulty = -1;
	c->vow->title = NULL;
	c->vow->description = NULL;
	c->vow_active = 0;
	c->vid = -1;

	return c;
}

struct character *
get_current_character(void)
{
	return curchar;
}
