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

#include "isscrolls.h"

#include <json-c/json.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void
read_oracle_from_json(int focus, int generate)
{
	char path[_POSIX_PATH_MAX], temp_name[255];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	long die;
	int what, ret, id, id_temp, max = 100, action;

	switch(focus) {
		case ORACLE_CHAR_ROLE:
		case ORACLE_CHAR_GOAL:
		case ORACLE_CHAR_DESC:
		case ORACLE_CHAR_DISPOSITION:
		case ORACLE_CHAR_ACTIVITY:
			action = JSON_CHARACTERS;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_character.json", PATH_SHARE_DIR);
			break;
		case ORACLE_IS_NAMES:
		case ORACLE_ELF_NAMES:
		case ORACLE_VAROU_NAMES:
		case ORACLE_TROLL_NAMES:
			action = JSON_NAMES;
			max = 200;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_names.json", PATH_SHARE_DIR);
			break;
		case ORACLE_PAYTHEPRICE:
		case ORACLE_DELVE_THE_DEPTHS_EDGE:
		case ORACLE_DELVE_THE_DEPTHS_SHADOW:
		case ORACLE_DELVE_THE_DEPTHS_WITS:
		case ORACLE_DELVE_OPPORTUNITY:
		case ORACLE_DELVE_DANGER:
			action = JSON_MOVES;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_move_oracles.json", PATH_SHARE_DIR);
			break;
		case ORACLE_ACTIONS:
		case ORACLE_THEMES:
			action = JSON_ACTION;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_prompts.json", PATH_SHARE_DIR);
			break;
		case ORACLE_RANKS:
		case ORACLE_COMBAT_ACTIONS:
		case ORACLE_PLOT_TWISTS:
		case ORACLE_MYSTIC_BACKSLASH:
			action = JSON_TURNING;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_turning_point.json", PATH_SHARE_DIR);
			break;
		case ORACLE_REGION:
		case ORACLE_LOCATION:
		case ORACLE_COASTAL:
		case ORACLE_DESCRIPTION:
			action = JSON_PLACES;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_place.json", PATH_SHARE_DIR);
			break;
		case ORACLE_SETTLEMENT_TROUBLE:
			action = JSON_SETTLEMENT;
			ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_settlement.json", PATH_SHARE_DIR);
			break;
		default:
			log_debug("Unknown focus.  Abort\n");
			return;
	}

	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	memset(temp_name, 0, sizeof(temp_name));

again:
	die = roll_oracle_die();
	if (die < 0 || die > max)
		goto again;

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %lu\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

		switch (action) {
			case JSON_CHARACTERS:
				if ((strcmp(json_object_get_string(name), "Role") == 0))
					what = ORACLE_CHAR_ROLE;
				else if ((strcmp(json_object_get_string(name), "Goal") == 0))
					what = ORACLE_CHAR_GOAL;
				else if ((strcmp(json_object_get_string(name), "Descriptor") == 0))
					what = ORACLE_CHAR_DESC;
				else if ((strcmp(json_object_get_string(name), "Disposition") == 0))
					what = ORACLE_CHAR_DISPOSITION;
				else if ((strcmp(json_object_get_string(name), "Activity") == 0))
					what = ORACLE_CHAR_ACTIVITY;
				else {
					what = -1;
					continue;
				}
				break;
			case JSON_MOVES:
				if ((strcmp(json_object_get_string(name), "Pay the Price") == 0))
					what = ORACLE_PAYTHEPRICE;
				else if ((strcmp(json_object_get_string(name), "Delve the Depths - Edge") == 0))
					what = ORACLE_DELVE_THE_DEPTHS_EDGE;
				else if ((strcmp(json_object_get_string(name), "Delve the Depths - Shadow") == 0))
					what = ORACLE_DELVE_THE_DEPTHS_SHADOW;
				else if ((strcmp(json_object_get_string(name), "Delve the Depths - Wits") == 0))
					what = ORACLE_DELVE_THE_DEPTHS_WITS;
				else if ((strcmp(json_object_get_string(name), "Find an Opportunity") == 0))
					what = ORACLE_DELVE_OPPORTUNITY;
				else if ((strcmp(json_object_get_string(name), "Reveal a Danger") == 0))
					what = ORACLE_DELVE_DANGER;
				else {
					what = -1;
					continue;
				}
				break;
			case JSON_ACTION:
				if ((strcmp(json_object_get_string(name), "Action") == 0))
					what = ORACLE_ACTIONS;
				else if ((strcmp(json_object_get_string(name), "Theme") == 0))
					what = ORACLE_THEMES;
				/* The following oracles are not yet supported */
				else if ((strcmp(json_object_get_string(name), "Feature") == 0))
					continue;
				else if ((strcmp(json_object_get_string(name), "Focus") == 0))
					continue;
				else if ((strcmp(json_object_get_string(name), "Trap") == 0))
					continue;
				else if ((strcmp(json_object_get_string(name), "Combat Event") == 0))
					continue;
				else {
					what = -1;
					continue;
				}
				break;
			case JSON_TURNING:
				if ((strcmp(json_object_get_string(name), "Challenge Rank") == 0))
					what = ORACLE_RANKS;
				else if ((strcmp(json_object_get_string(name), "Combat Action") == 0))
					what = ORACLE_COMBAT_ACTIONS;
				else if ((strcmp(json_object_get_string(name), "Major Plot Twist") == 0))
					what = ORACLE_PLOT_TWISTS;
				else if ((strcmp(json_object_get_string(name), "Mystic Backlash") == 0))
					what = ORACLE_MYSTIC_BACKSLASH;
				else {
					what = -1;
					continue;
				}
				break;
			case JSON_PLACES:
				if ((strcmp(json_object_get_string(name), "Region") == 0))
					what = ORACLE_REGION;
				else if ((strcmp(json_object_get_string(name), "Location") == 0))
					what = ORACLE_LOCATION;
				else if ((strcmp(json_object_get_string(name), "Coastal Waters Location") == 0))
					what = ORACLE_COASTAL;
				else if ((strcmp(json_object_get_string(name), "Location Descriptors") == 0))
					what = ORACLE_DESCRIPTION;
				else {
					what = -1;
					continue;
				}
				break;
			case JSON_NAMES:
				if ((strcmp(json_object_get_string(name), "Ironlander Names") == 0))
					what = ORACLE_IS_NAMES;
				else if ((strcmp(json_object_get_string(name), "Elf Names") == 0))
					what = ORACLE_ELF_NAMES;
				else if ((strcmp(json_object_get_string(name), "Giant Names") == 0))
					what = ORACLE_GIANT_NAMES;
				else if ((strcmp(json_object_get_string(name), "Varou Names") == 0))
					what = ORACLE_VAROU_NAMES;
				else if ((strcmp(json_object_get_string(name), "Troll Names") == 0))
					what = ORACLE_TROLL_NAMES;
				else {
					what = -1;
					continue;
				}
				break;
			case JSON_SETTLEMENT:
				if ((strcmp(json_object_get_string(name), "Settlement Trouble") == 0))
					what = ORACLE_SETTLEMENT_TROUBLE;
				else {
					what = -1;
					continue;
				}
				break;
			default:
				log_debug("Unknown action.  Abort\n");
				return;
		}

		if (what != focus)
			continue;

		id_temp = 0;
		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
			id = json_object_get_int(chance);
			log_debug("%s %d <%ld>\n", json_object_get_string(desc), id, die);

			/* Save the current string for later use, since some oracle tables don't
			 * have a value for every key (die roll from 1 to 100.  In case there is a
			 * gap, use the save value */
			snprintf(temp_name, sizeof(temp_name), "%s", json_object_get_string(desc));

			/* Die roll matches exactly the oracle key */
			if (id == die) {
				/* User called 'generatenpc' so avoid newlines */
				if (generate) {
					convert_to_lowercase(temp_name);
					printf("%s", temp_name);
				} else
					printf("%s <%ld>\n", temp_name, die);
				goto out;
			/* We have a gap */
			} else if (id > die) {
				log_debug("ID %d larger than the last one %d, we have a gap (die %ld)\n",
					id, id_temp, die);
				/* User called 'generatenpc' so avoid newlines */
				if (generate) {
					convert_to_lowercase(temp_name);
					printf("%s", temp_name);
				} else
					printf("%s <%ld>\n", temp_name, die);
				goto out;
			}
			id_temp = id;
		}
	}

out:
	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);
}

void
cmd_show_iron_name(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_IS_NAMES, 0);
}

void
cmd_show_elf_name(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_ELF_NAMES, 0);
}

void
cmd_show_giant_name(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_GIANT_NAMES, 0);
}

void
cmd_show_varou_name(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_VAROU_NAMES, 0);
}

void
cmd_show_troll_name(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_TROLL_NAMES, 0);
}

void
cmd_show_action(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_ACTIONS, 0);
}

void
cmd_show_theme(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_THEMES, 0);
}

void
cmd_show_rank(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_RANKS, 0);
}

void
cmd_show_combat_action(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_COMBAT_ACTIONS, 0);
}

void
cmd_show_plot_twist(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_PLOT_TWISTS, 0);
}

void
cmd_show_mystic_backshlash(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_MYSTIC_BACKSLASH, 0);
}

void
cmd_show_location(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_LOCATION, 0);
}

void
cmd_show_location_description(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_DESCRIPTION, 0);
}

void
cmd_show_coastal_location(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_COASTAL, 0);
}

void
cmd_show_region(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_REGION, 0);
}

void
cmd_show_pay_the_price(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_PAYTHEPRICE, 0);
}

void
cmd_find_an_opportunity(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_DELVE_OPPORTUNITY, 0);
}

void
cmd_reveal_a_danger(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_DELVE_DANGER, 0);
}

void
cmd_show_settlement_trouble(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_SETTLEMENT_TROUBLE, 0);
}

void
cmd_generate_npc(__attribute__((unused))char *unused)
{
	read_oracle_from_json(ORACLE_IS_NAMES, 1);
	printf(" the ");
	read_oracle_from_json(ORACLE_CHAR_ROLE, 1);
	printf(" is a ");
	read_oracle_from_json(ORACLE_CHAR_DESC, 1);
	printf(" person whose goal is to ");
	read_oracle_from_json(ORACLE_CHAR_GOAL, 1);
	printf(".\n");
}

void
convert_to_lowercase(char *buffer)
{
	char *p;

	if (buffer == NULL)
		return;

	p = buffer;

	do {
		*p = tolower(*p);
	} while (*p++ != '\0');
}
