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
cmd_enter_the_fray(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("\nPlease specify the stat you'd like to use in this move\n\n");
		printf("heart\t- You are facing off against your foe\n");
		printf("shadow \t- You are moving into position against or strike without warning\n");
		printf("wits\t- You are ambushed\n");
		printf("Example: enterthefray wits\n\n");
		return;
	} else if (ret <= -20) {
		return;
	}

	if (strcasecmp(stat, "wits") == 0) {
		ival[0] = curchar->wits;
	} else if (strcasecmp(stat, "shadow") == 0) {
		ival[0] = curchar->shadow;
	} else if (strcasecmp(stat, "heart") == 0) {
		ival[0] = curchar->heart;
	} else
		goto info;

	if (curchar->fight_active == 0) {
		ask_for_fight_difficulty();
		curchar->fight_active = 1;
	} else {
		printf("You are already in a fight\n");
		return;
	}

	ret = action_roll(ival);
	if (ret == 8) {
		change_char_value("momentum", INCREASE, 2);
		set_initiative(1);
		printf("You have initiative\n");
	} else if (ret == 4) {
		printf("You may choose one boost -> Rulebook\n");
	} else
		printf("Pay the price -> Rulebook\n");

	update_prompt();
}

void
cmd_end_the_fight(char *cmd)
{
	struct character *curchar = get_current_character();
	double dval[2] = { -1.0, -1.0 };
	int ret;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	if (curchar->fight_active == 0) {
		printf("You are not in a fight.  Enter one with enterthefray\n");
		return;
	}

	dval[0] = curchar->fight->progress;
	dval[1] = get_int_from_cmd(cmd);
	ret = progress_roll(dval);
	if (ret == 8) {
		printf("The foe is no longer in the fight -> Rulebook\n");
	} else if (ret == 4) {
		printf("The foe is no longer in the fight, but you must chose one option -> Rulebook\n");
	} else {
		printf("You lost the fight.  Pay the price -> Rulebook\n");
	}
	curchar->fight_active = 0;
	curchar->fight->progress = 0;
	delete_fight(curchar->id);
	update_prompt();
}

void
cmd_endure_harm(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret, hr, suffer;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	suffer = 0;

	/* We are in a fight, so we can suffer harm equal to our foe's rank */
	if (curchar->fight_active == 1) {
		hr = curchar->health - curchar->fight->difficulty;
		suffer = curchar->fight->difficulty;
	} else {
		/* We are not in a fight, so the player can specify the amount of
		 * harm to suffer */
		ival[1] = get_int_from_cmd(cmd);
		if (ival[1] == -1) {
			/* We are not in a fight and there is not argument provided */
			printf("Please specify the amount of harm you want to suffer\n\n");
			printf("Example: endureharm 2\n");
			return;
		}

		hr = curchar->health - ival[1];
		suffer = ival[1];
		log_debug("Arg provided %d, hr: %d\n", ival[1], hr);
		/* Reset ival[1] since we don't need a bonus */
		ival[1] = -1;
	}

	if (hr >= 0) {
		curchar->health -= suffer;
		printf("You suffer %d harm and your health is down to %d\n",
			suffer, curchar->health);
	} else if (hr < 0) {
		/* Health is 0, so suffer -momentum equal to remaining health */
		log_debug("hr < 0: %d\n", hr);
		curchar->health = 0;
		curchar->momentum -= (hr * (-1));
		printf("You suffer %d harm and since your health is %d, your "\
			"momentum is down to %d\n",
			suffer, curchar->health,
			curchar->momentum);
	}

	ival[0] = curchar->iron;
	if (curchar->heart > curchar->iron) {
		ival[0] = curchar->heart;
	}

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You need to choose one option -> Rulebook\n");
	} else if (ret == 4) {
		printf("You press on\n");
	} else {
		change_char_value("momentum", DECREASE, 1);
		if (curchar->health == 0)
			printf("Mark either maimed or wounded or on the oracle table -> Rulebook\n");
	}
}

void
cmd_strike(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	if (curchar->fight_active == 0) {
		printf("You are not in a fight.  Enter one with enterthefray\n");
		return;
	}

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("Please specify the stat you'd like to use in this move\n\n");
		printf("iron\t- You attack in close quarters\n");
		printf("edge\t- You attack at range\n");
		printf("Example: strike iron\n");
		return;
	} else if (ret <= -20)
		return;

	if (strcasecmp(stat, "iron") == 0) {
		ival[0] = curchar->iron;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->edge;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You inflict +1 harm and retain initiative\n");
		set_initiative(1);
		mark_fight_progress();
		mark_fight_progress();
	} else if (ret == 4) {
		printf("You inflict harm and lose initiative\n");
		set_initiative(0);
		mark_fight_progress();
	} else {
		printf("Pay the price -> Rulebook\n");
		set_initiative(0);
		update_prompt();
	}
}

void
cmd_clash(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	if (curchar->fight_active == 0) {
		printf("You are not in a fight.  Enter one with enterthefray\n");
		return;
	}

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("Please specify the stat you'd like to use in this move\n\n");
		printf("iron\t- You fight in close quarters\n");
		printf("edge\t- You fight at range\n");
		printf("Example: clash iron\n");
		return;
	} else if (ret <= -20)
		return;

	if (strcasecmp(stat, "iron") == 0) {
		ival[0] = curchar->iron;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->edge;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		printf("You inflict harm, regain initiative and can choose one option -> Rulebook\n");
		set_initiative(1);
		mark_fight_progress();
	} else if (ret == 4) {
		printf("You inflict harm and lose initiative. Pay the price -> Rulebook\n");
		set_initiative(0);
		mark_fight_progress();
	} else {
		printf("Pay the price -> Rulebook\n");
		set_initiative(0);
		update_prompt();
	}
}

void
cmd_battle(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	if (curchar == NULL) {
		printf("No character loaded.  Use 'cd' to load a character\n");
		return;
	}

	if (curchar->fight_active == 0) {
		printf("You are not in a fight.  Enter one with enterthefray\n");
		return;
	}

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("Please specify the stat you'd like to use in this move\n\n");
		printf("edge\t- Fight at range, or using your speed and the terrain\n");
		printf("heart\t- Fight depending on your courage, allies, or companions\n");
		printf("iron\t- Fight in close to overpower your opponents\n");
		printf("shadow\t- Fight using trickery to befuddle your opponents\n");
		printf("wits\t- Fight using careful tactics to outsmart your opponents\n\n");
		printf("Example: battle iron\n");
		return;
	} else if (ret <= -20)
		return;

	if (strcasecmp(stat, "iron") == 0) {
			ival[0] = curchar->iron;
	} else if (strcasecmp(stat, "wits") == 0) {
		ival[0] = curchar->wits;
	} else if (strcasecmp(stat, "edge") == 0) {
		ival[0] = curchar->edge;
	} else if (strcasecmp(stat, "shadow") == 0) {
		ival[0] = curchar->shadow;
	} else if (strcasecmp(stat, "heart") == 0) {
		ival[0] = curchar->heart;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8) {
		change_char_value("momentum", INCREASE, 2);
		printf("You achieve your objective unconditionally\n");
	} else if (ret == 4) /* weak hit */
		printf("You achieve your objective, but not without a cost -> Rulebook\n");
	else
		printf("Pay the price -> Rulebook\n");
}

void
set_initiative(int what)
{
	struct character *curchar = get_current_character();

	if (curchar == NULL) {
		log_debug("No character loaded.  Cannot set initiative\n");
		return;
	}

	if (curchar->fight_active == 0) {
		printf("You need start a fight before you can mark progress\n");
		return;
	}

	if (what == 1)
		curchar->fight->initiative = 1;
	else
		curchar->fight->initiative = 0;
}

void
mark_fight_progress()
{
	struct character *curchar = get_current_character();

	if (curchar == NULL) {
		log_debug("No character loaded.  Cannot calculate progress\n");
		return;
	}

	if (curchar->fight_active == 0) {
		printf("You need start a fight before you can mark progress\n");
		return;
	}

	switch (curchar->fight->difficulty) {
	case 1:
		curchar->fight->progress += 3;
		break;
	case 2:
		curchar->fight->progress += 2;
		break;
	case 3:
		curchar->fight->progress += 1;
		break;
	case 4:
		curchar->fight->progress += 0.5;
		break;
	case 5:
		curchar->fight->progress += 0.25;
		break;
	default:
		curchar->fight->difficulty = 1;
		log_errx(1, "Unknown difficulty.  This should not happen.  Set to 1\n");
	}

	if (curchar->fight->progress > 10) {
		printf("Your fight is successful.  Consider ending it\n");
		curchar->fight->progress = 10;
	}

	update_prompt();
}

void
save_fight()
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *items, *id;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("No character loaded.  No fight to save.\n");
		return;
	}

	if (curchar->fight_active == 0) {
		log_debug("No active fight to save.\n");
		return;
	}

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "difficulty", json_object_new_int(curchar->fight->difficulty));
	json_object_object_add(cobj, "progress", json_object_new_double(curchar->fight->progress));
	json_object_object_add(cobj, "initiative", json_object_new_int(curchar->fight->initiative));

	snprintf(path, sizeof(path), "%s/fight.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No fight JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create fight JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "fight", items);
	} else {
		/* Get existing character array from JSON */
		if (!json_object_object_get_ex(root, "fight", &items)) {
			log_debug("Cannot find a [fight] array in %s. Create one\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "fight", items);
		}

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->id == json_object_get_int(id)) {
				log_debug("Update fight entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No fight entry for %s found, adding new one\n", curchar->name);
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
delete_fight(int id)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	snprintf(path, sizeof(path), "%s/fight.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No fight JSON file found\n");
		return;
	}

	json_object *fight;
	if (!json_object_object_get_ex(root, "fight", &fight)) {
		log_debug("Cannot find a [fight] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(fight);
	for (i = 0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(fight, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			json_object_array_del_idx(fight, i, 1);
			log_debug("Deleted fight entry for %d\n", id);
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}

void
load_fight(int id)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	int temp_n, i;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	snprintf(path, sizeof(path), "%s/fight.json", get_isscrolls_dir());
	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No fight JSON file found\n");
		return;
	}

	json_object *fight;
	if (!json_object_object_get_ex(root, "fight", &fight)) {
		log_debug("Cannot find a [fight] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(fight);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(fight, i);
		json_object_object_get_ex(temp, "id", &lid);
		if (id == json_object_get_int(lid)) {
			log_debug("Loading fight for id: %d\n", json_object_get_int(lid));

			json_object *cval;
			json_object_object_get_ex(temp, "difficulty", &cval);
			curchar->fight->difficulty = json_object_get_int(cval);
			json_object_object_get_ex(temp, "progress", &cval);
			curchar->fight->progress   = json_object_get_double(cval);
			json_object_object_get_ex(temp, "initiative", &cval);
			curchar->fight->initiative = json_object_get_int(cval);
		}
	}

	json_object_put(root);
}

void
ask_for_fight_difficulty()
{
	struct character *curchar = get_current_character();

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	printf("Please set a difficulty for your fight\n\n");
	printf("1\t - Troublesome foe (3 progress per harm)\n");
	printf("2\t - Dangerous foe (2 progress per harm)\n");
	printf("3\t - Formidable foe (2 progress per harm)\n");
	printf("4\t - Extreme foe (2 ticks per harm)\n");
	printf("5\t - Epic foe (1 tick per harm)\n\n");

	curchar->fight->difficulty = ask_for_value("Enter a value between 1 and 5: ", 5);
}

