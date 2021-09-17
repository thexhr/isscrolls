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
struct listhead *headp;
LIST_HEAD(listhead, entry) head = LIST_HEAD_INITIALIZER(head);

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

	if (character_exists(name) == 0) {
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
cmd_delete_character(__attribute__((unused)) char *unused)
{
	struct entry *np = NULL;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

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
		free_character();
		curchar = NULL;
	} else if (strlen(character) == 0 && curchar == NULL) {
		/* We got no argument and there is no character loaded */
		printf("Provide the name of a character as argument\n\n");
		printf("Example: cd Delkash - load the character named Delkash\n");
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
cmd_toogle(char *value)
{
	CURCHAR_CHECK();

	if (value == NULL || strlen(value) == 0) {
		printf("Please specify the stat you want to toogle\n");
		printf("\nExample: toggle wounded\n");
		printf("\nYou can toggle the following values:\n\n");
		printf("-Wounded\n-Unprepared\n-Shaken\n-Encumbered\n-Maimed\n-Cursed\n");
		printf("-Corrupted\n-Tormented\n");
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
	} else if (strcasecmp(value, "maimed") == 0) {
		if (curchar->maimed == 1) {
			printf("Maimed is a permanent bane and cannot be changed\n");
			return;
		}
		toggle_value(value, &curchar->maimed);
	} else if (strcasecmp(value, "cursed") == 0) {
		toggle_value(value, &curchar->cursed);
	} else if (strcasecmp(value, "corrupted") == 0) {
		if (curchar->corrupted == 1) {
			printf("Corrupted is a permanent bane and cannot be changed\n");
			return;
		}
		toggle_value(value, &curchar->corrupted);
	} else if (strcasecmp(value, "tormented") == 0) {
		toggle_value(value, &curchar->tormented);
	}

	set_max_momentum();
}


void
update_prompt()
{
	char p[MAX_PROMPT_LEN];
	char j[MAX_PROMPT_LEN];
	char f[MAX_PROMPT_LEN];
	char i[5];

	CURCHAR_CHECK();

	memset(j, 0, sizeof(j));
	memset(f, 0, sizeof(f));
	memset(i, 0, sizeof(i));

	if (curchar->journey_active == 1) {
		if (curchar->j->difficulty < 4)
			snprintf(j, sizeof(j), "Journey %.0f/10 > ",
				curchar->j->progress);
		else
			snprintf(j, sizeof(j), "Journey %.2f/10 > ",
				curchar->j->progress);
	}

	if (curchar->fight_active == 1) {
		if (curchar->fight->initiative == 1)
			snprintf(i, 5, "%s", " [I]");

		if (curchar->fight->difficulty < 4)
			snprintf(f, sizeof(f), "Fight %.0f/10%s > ",
				curchar->fight->progress, i);
		else
			snprintf(f, sizeof(f), "Fight %.2f/10%s > ",
				curchar->fight->progress, i);
	}

	snprintf(p, sizeof(p), "%s > %s%s", curchar->name, j, f);

	set_prompt(p);
}

void
toggle_value(const char *desc, int *value)
{
	int new = !(*value);

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	printf("Toggle %s from %d to %d\n", desc, *value, new);
	*value = new;
}

void
set_max_momentum()
{
	int mm;

	if (curchar == NULL)
		return;

	/* Max momentum is 10 minus all the debilities */
	mm = 10 - curchar->wounded - curchar->unprepared - curchar->shaken -
		curchar->encumbered - curchar->maimed -
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
		curchar->encumbered - curchar->maimed -
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

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	if (value == NULL || strlen(value) == 0) {
		printf("Please specify the stat you want to %s\n", event[what]);
		printf("\nExample: %s wits\t- %s 'wits' by 1\n", event[what], event[what]);
		printf("\nYou can change the following values:\n\n");
		printf("-Edge\n-Heart\n-Iron\n-Shadow\n-Wits\n-Momentum\n-Health\n-Spirit\n");
		printf("-Supply\n-Exp\n");
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
	} else if (strcasecmp(value, "momentum") == 0) {
		modify_value(value, &curchar->momentum, curchar->max_momentum, -6,
			howmany, what);
		return;
	} else if (strcasecmp(value, "health") == 0) {
		if (curchar->wounded == 1) {
			printf("You are wounded, you cannot increase health\n");
			return;
		}
		modify_value(value, &curchar->health, 5, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "spirit") == 0) {
		if (curchar->shaken == 1) {
			printf("You are shaken, you cannot increase spirit\n");
			return;
		}
		modify_value(value, &curchar->spirit, 5, 0, howmany, what);
		return;
	} else if (strcasecmp(value, "supply") == 0) {
		if (curchar->unprepared == 1) {
			printf("You are unprepared, you cannot increase supply\n");
			return;
		}
		modify_value(value, &curchar->supply, 5, 0, howmany, what);
		return;
	} else {
		printf("Unknown value\n");
		return;
	}

}

void
modify_value(const char *str, int *value, int max, int min, int howmany,
	int what)
{
	if (what == 0) {
		if (*value >= max) {
			return;
		}
		*value += howmany;
		printf("Increasing %s from %d to %d\n", str, *value - howmany, *value);
	} else {
		if (*value <= min) {
			return;
		}
		*value -= howmany;
		printf("Decreasing %s from %d to %d\n", str, *value + howmany, *value);
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
save_current_character()
{
	save_character();
}

void
save_character()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *items;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("Nothing to save here\n");
		return;
	}

	save_journey();
	save_fight();

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "name", json_object_new_string(curchar->name));
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
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
	json_object_object_add(cobj, "cursed", json_object_new_int(curchar->cursed));
	json_object_object_add(cobj, "dead", json_object_new_int(curchar->dead));
	json_object_object_add(cobj, "corrupted",
		json_object_new_int(curchar->corrupted));
	json_object_object_add(cobj, "tormented",
		json_object_new_int(curchar->tormented));
	json_object_object_add(cobj, "exp_used",
		json_object_new_int(curchar->exp_used));
	json_object_object_add(cobj, "bonds",
		json_object_new_double(curchar->bonds));
	json_object_object_add(cobj, "journey_active",
		json_object_new_int(curchar->journey_active));
	json_object_object_add(cobj, "fight_active",
		json_object_new_int(curchar->fight_active));

	snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No character JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "characters", items);
	} else {
		/* Get existing character array from JSON */
		if (!json_object_object_get_ex(root, "characters", &items)) {
			log_debug("Cannot find a [characters] array in %s\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "characters", items);
		}

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
delete_saved_character(int id)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	LIST_INIT(&head);

	snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
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

void
load_characters_list()
{
	struct entry *e;
	char path[_POSIX_PATH_MAX];
	json_object *root;
	json_object *lid, *name;
	int temp_n, i;

	LIST_INIT(&head);

	snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
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
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(characters, i);
		json_object_object_get_ex(temp, "id", &lid);
		json_object_object_get_ex(temp, "name", &name);
		log_debug("Add %s to list with id: %d\n", json_object_get_string(name), json_object_get_int(lid));

		if ((e = malloc(sizeof(struct entry))) == NULL)
			log_errx(1, "cannot allocate memory\n");

		e->id = json_object_get_int(lid);
		snprintf(e->name, sizeof(e->name), "%s", json_object_get_string(name));
		LIST_INSERT_HEAD(&head, e, entries);
	}

	json_object_put(root);
}

int
load_character(int id)
{
	struct character *c;
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid, *name;
	int temp_n, i;

	if (id <= 0)
		return -1;

	snprintf(path, sizeof(path), "%s/characters.json", get_isscrolls_dir());
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

	json_object *characters;
	if (!json_object_object_get_ex(root, "characters", &characters)) {
		log_debug("Cannot find a [characters] array in %s\n", path);
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
			json_object *cval;
			json_object_object_get_ex(temp, "edge", &cval);
			c->edge = json_object_get_int(cval);
			json_object_object_get_ex(temp, "heart", &cval);
			c->heart = json_object_get_int(cval);
			json_object_object_get_ex(temp, "iron", &cval);
			c->iron = json_object_get_int(cval);
			json_object_object_get_ex(temp, "shadow", &cval);
			c->shadow = json_object_get_int(cval);
			json_object_object_get_ex(temp, "wits", &cval);
			c->wits	= json_object_get_int(cval);
			json_object_object_get_ex(temp, "exp", &cval);
			c->exp = json_object_get_int(cval);
			json_object_object_get_ex(temp, "health", &cval);
			c->health = json_object_get_int(cval);
			json_object_object_get_ex(temp, "spirit", &cval);
			c->spirit = json_object_get_int(cval);
			json_object_object_get_ex(temp, "supply", &cval);
			c->supply = json_object_get_int(cval);
			json_object_object_get_ex(temp, "wounded", &cval);
			c->wounded = json_object_get_int(cval);
			json_object_object_get_ex(temp, "shaken", &cval);
			c->shaken = json_object_get_int(cval);
			json_object_object_get_ex(temp, "maimed", &cval);
			c->maimed = json_object_get_int(cval);
			json_object_object_get_ex(temp, "cursed", &cval);
			c->cursed = json_object_get_int(cval);
			json_object_object_get_ex(temp, "dead", &cval);
			c->dead  = json_object_get_int(cval);
			json_object_object_get_ex(temp, "bonds", &cval);
			c->bonds = json_object_get_double(cval);
			json_object_object_get_ex(temp, "corrupted", &cval);
			c->corrupted = json_object_get_int(cval);
			json_object_object_get_ex(temp, "tormented", &cval);
			c->tormented = json_object_get_int(cval);
			json_object_object_get_ex(temp, "exp_used", &cval);
			c->exp_used = json_object_get_int(cval);
			json_object_object_get_ex(temp, "unprepared", &cval);
			c->unprepared= json_object_get_int(cval);
			json_object_object_get_ex(temp, "momentum", &cval);
			c->momentum = json_object_get_int(cval);
			json_object_object_get_ex(temp, "encumbered", &cval);
			c->encumbered = json_object_get_int(cval);
			json_object_object_get_ex(temp, "max_momentum", &cval);
			c->max_momentum = json_object_get_int(cval);
			json_object_object_get_ex(temp, "momentum_reset", &cval);
			c->momentum_reset = json_object_get_int(cval);
			json_object_object_get_ex(temp, "journey_active", &cval);
			c->journey_active = json_object_get_int(cval);
			json_object_object_get_ex(temp, "fight_active", &cval);
			c->fight_active = json_object_get_int(cval);
		}
	}

	curchar = c;

	load_journey(c->id);
	load_fight(c->id);
	update_prompt();
	print_character();

	json_object_put(root);

	return 0;
}

void
cmd_print_current_character(__attribute__((unused)) char *unused)
{
	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
	} else
		print_character();
}

void
print_character()
{
	if (curchar == NULL) {
		log_debug("Nothing to print here\n");
		return;
	}

	log_debug("Character ID: %d\n", curchar->id);
	printf("Name: %s (Exp: %d/30) Saved exp: %d ", curchar->name,
		curchar->exp, curchar->exp_used);
	if (curchar->dead == 1)
		printf("[DECEASED]\n");
	else
		printf("\n");

	printf("\nEdge: %d Heart: %d Iron: %d Shadow: %d Wits %d\n\n",
		curchar->edge, curchar->heart, curchar->iron, curchar->shadow, curchar->wits);
	printf("Momentum: %d/%d [%d] Health: %d/5 Spirit: %d/5 Supply: %d/5\n",
		curchar->momentum, curchar-> max_momentum, curchar->momentum_reset,
		curchar->health, curchar->spirit, curchar->supply);

	printf("\nWounded:\t%d Unprepared:\t%d Encumbered:\t%d Shaken:\t%d\n",
		curchar->wounded, curchar->unprepared, curchar->encumbered, curchar->shaken);
	printf("Corrupted:\t%d Tormented:\t%d Corrupted:\t%d Maimed:\t%d\n",
		curchar->corrupted, curchar->tormented, curchar->corrupted, curchar->maimed);

	printf("\nBonds: %.2f\n", curchar->bonds);

	if (curchar->journey_active == 1) {
		printf("\nActive Journey: Difficulty: %d Progress: %.2f/10\n",
			curchar->j->difficulty, curchar->j->progress);
	}
	if (curchar->fight_active == 1) {
		printf("\nActive Fight: Difficulty: %d Progress: %.2f/10\n",
			curchar->fight->difficulty, curchar->fight->progress);
	}
}

void
ask_for_journey_difficulty()
{
	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	printf("Please set a difficulty for your journey\n\n");
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
free_character()
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

	if (strlen(name) == 0)
		return -1;

	LIST_FOREACH(np, &head, entries) {
		if (strcasecmp(name, np->name) == 0) {
			return 0;
		}
	}

	return -1;
}

struct character *
create_character(const char *name)
{
	struct character *c;

	c = init_character_struct();

	if (strlen(name) == 0) {
		printf("Enter a name for your character: ");
		c->name = readline(NULL);
		if (strlen(c->name) == 0) {
			printf("Please provide a longer name\n");
			free_character();
			return NULL;
		}
		if (character_exists(c->name) == 0) {
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

	printf("Now distribute the following values to your attributes: 3,2,2,1,1\n");

	c->edge   = ask_for_value("Edge   : ", 4);
	c->heart  = ask_for_value("Heart  : ", 4);
	c->iron   = ask_for_value("Iron   : ", 4);
	c->wits   = ask_for_value("Wits   : ", 4);
	c->shadow = ask_for_value("Shadow : ", 4);

	return c;
}

struct character *
init_character_struct()
{
	struct character *c;

	if ((c = calloc(1, sizeof(struct character))) == NULL)
		log_errx(1, "calloc");

	if ((c->j = calloc(1, sizeof(struct journey))) == NULL)
		log_errx(1, "calloc");

	if ((c->fight = calloc(1, sizeof(struct fight))) == NULL)
		log_errx(1, "calloc");

	c->id = random();
	c->name = NULL;
	c->edge = c->heart = c->iron = c->shadow = c->wits = c->exp = 0;
	c->momentum = c->momentum_reset = 2;
	c->max_momentum = 10;
	c->health = c->spirit = c->supply = 5;
	c->wounded = c->unprepared = c->shaken = c->encumbered = c->maimed = 0;
	c->cursed = c->corrupted = c->tormented = c->exp_used = c->bonds = 0;
	c->dead = 0;

	c->j->id = c->id;
	c->j->difficulty = -1;
	c->j->progress = 0.0;
	c->journey_active = 0;

	c->fight->id = c->id;
	c->fight->difficulty = -1;
	c->fight->progress = 0.0;
	c->fight->initiative = 0;
	c->fight_active = 0;

	return c;
}

struct character *
get_current_character()
{
	return curchar;
}
