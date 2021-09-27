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

#include "isscrolls.h"

#include <json-c/json.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static char oracle_is_names[200][MAX_NAME_LEN];
static char oracle_elf_names[100][MAX_NAME_LEN];
static char oracle_giant_names[100][MAX_NAME_LEN];
static char oracle_varou_names[100][MAX_NAME_LEN];
static char oracle_troll_names[100][MAX_NAME_LEN];

static char oracle_action[100][MAX_NAME_LEN];
static char oracle_theme[100][MAX_NAME_LEN];

static char oracle_rank[100][MAX_RANK_LEN];
static char oracle_combat_action[100][MAX_PLOT_LEN];
static char oracle_plot_twist[100][MAX_PLOT_LEN];
static char oracle_mystic_backslash[100][MAX_MYSTIC_LEN];

static char oracle_regions[100][MAX_PLACES_LEN];
static char oracle_locations[100][MAX_PLACES_LEN];
static char oracle_location_descriptions[100][MAX_PLACES_LEN];
static char oracle_coastal_locations[100][MAX_PLACES_LEN];

static char oracle_pay_the_price[100][MAX_PTP_LEN];

static char oracle_delve_edge[100][MAX_DELVE_LEN];
static char oracle_delve_shadow[100][MAX_DELVE_LEN];
static char oracle_delve_wits[100][MAX_DELVE_LEN];
static char oracle_delve_opportunity[100][MAX_CHAR_LEN];
static char oracle_delve_danger[100][MAX_CHAR_LEN];

static char oracle_char_role[100][MAX_ROLE_LEN];
static char oracle_char_goal[100][MAX_GOAL_LEN];
static char oracle_char_desc[100][MAX_DESC_LEN];
static char oracle_char_disposition[100][MAX_DISP_LEN];
static char oracle_char_activity[100][MAX_ACTIVITY_LEN];

static int read_names   = 0;
static int read_action  = 0;
static int read_turning = 0;
static int read_places  = 0;
static int read_moves   = 0;
static int read_chars 	= 0;

static void read_names_from_json(void);
static void read_moves_from_json(void);
static void read_action_from_json(void);
static void read_turning_from_json(void);
static void read_places_from_json(void);
static void read_chars_from_json(void);

static void
add_to_array(int what, int id, const char *value)
{
	log_debug("%d, %d, %s\n", what, id, value);

	switch (what) {
	case -1:
		log_debug("Unknown array\n");
		return;
	case ORACLE_IS_NAMES:
		if (id < 0 || id > 200)
			return;
		snprintf(oracle_is_names[id], MAX_NAME_LEN, "%s", value);
		break;
	case ORACLE_ELF_NAMES:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_elf_names[id], MAX_NAME_LEN, "%s", value);
		break;
	case ORACLE_GIANT_NAMES:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_giant_names[id], MAX_NAME_LEN, "%s", value);
		break;
	case ORACLE_VAROU_NAMES:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_varou_names[id], MAX_NAME_LEN, "%s", value);
		break;
	case ORACLE_TROLL_NAMES:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_troll_names[id], MAX_NAME_LEN, "%s", value);
		break;
	/* -------------------------------------------------------------------- */
	case ORACLE_ACTIONS:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_action[id], MAX_NAME_LEN, "%s", value);
		break;
	case ORACLE_THEMES:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_theme[id], MAX_NAME_LEN, "%s", value);
		break;
	/* -------------------------------------------------------------------- */
	case ORACLE_RANKS:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_rank[id], MAX_RANK_LEN, "%s", value);
		break;
	case ORACLE_COMBAT_ACTIONS:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_combat_action[id], MAX_PLOT_LEN, "%s", value);
		break;
	case ORACLE_PLOT_TWISTS:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_plot_twist[id], MAX_PLOT_LEN, "%s", value);
		break;
	case ORACLE_MYSTIC_BACKSLASH:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_mystic_backslash[id], MAX_MYSTIC_LEN, "%s", value);
		break;
	/* -------------------------------------------------------------------- */
	case ORACLE_REGION:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_regions[id], MAX_PLACES_LEN, "%s", value);
		break;
	case ORACLE_LOCATION:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_locations[id], MAX_PLACES_LEN, "%s", value);
		break;
	case ORACLE_COASTAL:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_coastal_locations[id], MAX_PLACES_LEN, "%s", value);
		break;
	case ORACLE_DESCRIPTION:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_location_descriptions[id], MAX_PLACES_LEN, "%s", value);
		break;
	/* -------------------------------------------------------------------- */
	case ORACLE_PAYTHEPRICE:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_pay_the_price[id], MAX_PTP_LEN, "%s", value);
		break;
	/* -------------------------------------------------------------------- */
	case ORACLE_DELVE_THE_DEPTHS_EDGE:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_delve_edge[id], MAX_DELVE_LEN, "%s", value);
		break;
	case ORACLE_DELVE_THE_DEPTHS_WITS:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_delve_wits[id], MAX_DELVE_LEN, "%s", value);
		break;
	case ORACLE_DELVE_THE_DEPTHS_SHADOW:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_delve_shadow[id], MAX_DELVE_LEN, "%s", value);
		break;
	case ORACLE_DELVE_OPPORTUNITY:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_delve_opportunity[id], MAX_CHAR_LEN, "%s", value);
		break;
	case ORACLE_DELVE_DANGER:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_delve_danger[id], MAX_CHAR_LEN, "%s", value);
		break;
	/* -------------------------------------------------------------------- */
	case ORACLE_CHAR_ROLE:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_char_role[id], MAX_ROLE_LEN, "%s", value);
		break;
	case ORACLE_CHAR_GOAL:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_char_goal[id], MAX_GOAL_LEN, "%s", value);
		break;
	case ORACLE_CHAR_DESC:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_char_desc[id], MAX_DESC_LEN, "%s", value);
		break;
	case ORACLE_CHAR_DISPOSITION:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_char_disposition[id], MAX_DISP_LEN, "%s", value);
		break;
	case ORACLE_CHAR_ACTIVITY:
		if (id < 0 || id > 100)
			return;
		snprintf(oracle_char_activity[id], MAX_ACTIVITY_LEN, "%s", value);
		break;
	default:
		log_errx(1, "add_to_array: This should not happen\n");
	}
}


static void
read_chars_from_json()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	int what, ret;

	ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_character.json", PATH_SHARE_DIR);
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happended.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %d\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

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

		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
				add_to_array(what, json_object_get_int(chance), json_object_get_string(desc));
		}
	}

	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);

	read_chars = 1;
}

static void
read_names_from_json()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	int what, ret;

	ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_names.json", PATH_SHARE_DIR);
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happended.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %d\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

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

		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
				add_to_array(what, json_object_get_int(chance), json_object_get_string(desc));
		}
	}

	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);

	read_names = 1;
}

static void
read_moves_from_json()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	int what, ret;

	ret = snprintf(path, sizeof(path), "%s/ironsworn_move_oracles.json", PATH_SHARE_DIR);
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happended.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %d\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

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

		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
			add_to_array(what, json_object_get_int(chance), json_object_get_string(desc));
		}
	}

	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);

	read_moves = 1;
}

static void
read_action_from_json()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	int what, ret;

	ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_prompts.json", PATH_SHARE_DIR);
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happended.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %d\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

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

		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
			add_to_array(what, json_object_get_int(chance), json_object_get_string(desc));
		}
	}

	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);

	read_action = 1;
}

static void
read_turning_from_json()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	int what, ret;

	ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_turning_point.json", PATH_SHARE_DIR);
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happended.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %d\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

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

		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
			add_to_array(what, json_object_get_int(chance), json_object_get_string(desc));
		}
	}

	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);

	read_turning = 1;
}

static void
read_places_from_json()
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *oracles, *temp, *table, *name, *desc, *chance;
	size_t n_oracles, n_entries, i, j;
	int what, ret;

	ret = snprintf(path, sizeof(path), "%s/ironsworn_oracles_place.json", PATH_SHARE_DIR);
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happended.  Buffer to short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_errx(1, "Cannot open %s\n", path);
	}

	if (!json_object_object_get_ex(root, "Oracles", &oracles)) {
		log_debug("Cannot find a [Oracles] array in %s\n", path);
		return;
	}

	n_oracles = json_object_array_length(oracles);

	log_debug("number of oracles: %d\n", n_oracles);
	for (i = 0; i < n_oracles; i++) {
		temp = json_object_array_get_idx(oracles, i);
		json_object_object_get_ex(temp, "Oracle Table", &table);
		json_object_object_get_ex(temp, "Name", &name);
		log_debug("Name %s\n", json_object_get_string(name));

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

		n_entries = json_object_array_length(table);
		for (j = 0; j < n_entries; j++) {
			temp = json_object_array_get_idx(table, j);
			json_object_object_get_ex(temp, "Description", &desc);
			json_object_object_get_ex(temp, "Chance", &chance);
			add_to_array(what, json_object_get_int(chance), json_object_get_string(desc));
		}
	}

	/* Decrement the reference count of json_object and free if it reaches zero. */
	json_object_put(root);

	read_places = 1;
}

void
cmd_show_iron_name(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_IS_NAMES, 200);
}

void
cmd_show_elf_name(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_ELF_NAMES, 100);
}

void
cmd_show_giant_name(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_GIANT_NAMES, 100);
}

void
cmd_show_varou_name(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_VAROU_NAMES, 100);
}

void
cmd_show_troll_name(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_TROLL_NAMES, 100);
}

void
cmd_show_action(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_ACTIONS, 100);
}

void
cmd_show_theme(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_THEMES, 100);
}

void
cmd_show_rank(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_RANKS, 100);
}

void
cmd_show_combat_action(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_COMBAT_ACTIONS, 100);
}

void
cmd_show_plot_twist(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_PLOT_TWISTS, 100);
}

void
cmd_show_mystic_backshlash(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_MYSTIC_BACKSLASH, 100);
}

void
cmd_show_location(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_LOCATION, 100);
}

void
cmd_show_location_description(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_DESCRIPTION, 100);
}

void
cmd_show_coastal_location(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_COASTAL, 100);
}

void
cmd_show_region(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_REGION, 100);
}

void
cmd_show_pay_the_price(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_PAYTHEPRICE, 100);
}

void
cmd_find_an_opportunity(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_DELVE_OPPORTUNITY, 100);
}

void
cmd_reveal_a_danger(__attribute__((unused))char *unused)
{
	show_info_from_oracle(0, ORACLE_DELVE_DANGER, 100);
}

void
cmd_generate_npc(__attribute__((unused))char *unused)
{
	show_info_from_oracle(1, ORACLE_IS_NAMES, 100);
	printf(" the ");
	show_info_from_oracle(1, ORACLE_CHAR_ROLE, 100);
	printf(" is a ");
	show_info_from_oracle(1, ORACLE_CHAR_DESC, 100);
	printf(" person whose goal is to ");
	show_info_from_oracle(1, ORACLE_CHAR_GOAL, 100);
	printf(".\n");
}

void
show_info_from_oracle(int action, int what, int max)
{
	char temp[255];
	long die, die2, saved_die;

roll_again:
	die = saved_die = roll_oracle_die();
	if (die < 0 || die >= max)
		return;

	memset(temp, 0, sizeof(temp));

	if (read_names == 0)
		read_names_from_json();
	if (read_action == 0)
		read_action_from_json();
	if (read_turning == 0)
		read_turning_from_json();
	if (read_places == 0)
		read_places_from_json();
	if (read_moves == 0)
		read_moves_from_json();
	if (read_chars == 0)
		read_chars_from_json();

	switch(what) {
	case ORACLE_IS_NAMES:
		die2 = roll_oracle_die();
		die += die2;
		snprintf(temp, sizeof(temp), "%s", oracle_is_names[die]);
		break;
	case ORACLE_ELF_NAMES:
		while (strlen(oracle_elf_names[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_elf_names[die]);
		break;
	case ORACLE_GIANT_NAMES:
		while (strlen(oracle_giant_names[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_giant_names[die]);
		break;
	case ORACLE_VAROU_NAMES:
		while (strlen(oracle_varou_names[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_varou_names[die]);
		break;
	case ORACLE_TROLL_NAMES:
		while (strlen(oracle_troll_names[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_troll_names[die]);
		break;
	case ORACLE_ACTIONS:
		while (strlen(oracle_action[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_action[die]);
		break;
	case ORACLE_THEMES:
		while (strlen(oracle_theme[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_theme[die]);
		break;
	case ORACLE_RANKS:
		while (strlen(oracle_rank[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_rank[die]);
		break;
	case ORACLE_COMBAT_ACTIONS:
		while (strlen(oracle_combat_action[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_combat_action[die]);
		break;
	case ORACLE_PLOT_TWISTS:
		while (strlen(oracle_plot_twist[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_plot_twist[die]);
		break;
	case ORACLE_MYSTIC_BACKSLASH:
		while (strlen(oracle_mystic_backslash[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_mystic_backslash[die]);
		break;
	case ORACLE_REGION:
		while (strlen(oracle_regions[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_regions[die]);
		break;
	case ORACLE_LOCATION:
		while (strlen(oracle_locations[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_locations[die]);
		break;
	case ORACLE_COASTAL:
		while (strlen(oracle_coastal_locations[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_coastal_locations[die]);
		break;
	case ORACLE_DESCRIPTION:
		while (strlen(oracle_location_descriptions[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_location_descriptions[die]);
		break;
	case ORACLE_PAYTHEPRICE:
		while (strlen(oracle_pay_the_price[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_pay_the_price[die]);
		break;
	case ORACLE_DELVE_THE_DEPTHS_EDGE:
		while (strlen(oracle_delve_edge[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_delve_edge[die]);
		break;
	case ORACLE_DELVE_THE_DEPTHS_WITS:
		while (strlen(oracle_delve_wits[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_delve_wits[die]);
		break;
	case ORACLE_DELVE_THE_DEPTHS_SHADOW:
		while (strlen(oracle_delve_shadow[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_delve_shadow[die]);
		break;
	case ORACLE_DELVE_OPPORTUNITY:
		while (strlen(oracle_delve_opportunity[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_delve_opportunity[die]);
		break;
	case ORACLE_DELVE_DANGER:
		while (strlen(oracle_delve_danger[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_delve_danger[die]);
		break;
	case ORACLE_CHAR_ROLE:
		while (strlen(oracle_char_role[die]) == 0)
			die++;
		if (die == 100)
			goto roll_again;
		snprintf(temp, sizeof(temp), "%s", oracle_char_role[die]);
		break;
	case ORACLE_CHAR_GOAL:
		while (strlen(oracle_char_goal[die]) == 0)
			die++;
		if (die == 100)
			goto roll_again;
		snprintf(temp, sizeof(temp), "%s", oracle_char_goal[die]);
		break;
	case ORACLE_CHAR_DESC:
		while (strlen(oracle_char_desc[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_char_desc[die]);
		break;
	case ORACLE_CHAR_DISPOSITION:
		while (strlen(oracle_char_disposition[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_char_disposition[die]);
		break;
	case ORACLE_CHAR_ACTIVITY:
		while (strlen(oracle_char_activity[die]) == 0)
			die++;
		snprintf(temp, sizeof(temp), "%s", oracle_char_activity[die]);
		break;
	}

	if (action) {
		if (what != ORACLE_IS_NAMES)
			convert_to_lowercase(temp);
		printf("%s", temp);
	} else
		printf("%s <%ld>\n", temp, saved_die);
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
