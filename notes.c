/*
 * Copyright (c) 2022-25 Matthias Schmidt <xhr@giessen.ccc.de>
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
#include <string.h>

#include <json-c/json.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "isscrolls.h"

void
cmd_create_new_note(char *title)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (title != NULL && strlen(title) > 0) {
		curchar->note->title = calloc(1, MAX_NOTE_TITLE+1); 
		if (curchar->note->title == NULL)
			log_errx(1, "calloc note title\n");
		snprintf(curchar->note->title, MAX_NOTE_TITLE, "%s", title);
	} else {
again:
		printf("Enter a title for your note [max 25 chars]: ");
		curchar->note->title = readline(NULL);
		if (curchar->note->title != NULL && strlen(curchar->note->title) == 0) {
			printf("The title must contain at least one character\n");
			free(curchar->note->title);
			goto again;
		}
	}

	log_debug("New note titled '%s'\n", curchar->note->title);

descagain:
	printf("Enter a description for your note [max 255 chars]: ");
	curchar->note->description = readline(NULL);
	if (curchar->note->description != NULL &&
		strlen(curchar->note->description) == 0) {
		printf("The description must contain at least one character\n");
		free(curchar->note->description);
		goto descagain;
	}

	/* Every new note gets a highest ID (nid) ... */
	curchar->note->nid = get_max_note_id();

	/* If we don't have any notes yet, start with 1 */
	if (curchar->note->nid == -1)
		curchar->note->nid = 1;
	else
		curchar->note->nid++;

	curchar->nid = curchar->note->nid;
	/* ... and belongs to one character (id) */
	curchar->note->id = curchar->id;

	save_note();

	update_prompt();
}

void
cmd_edit_note(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	edit_note(curchar->nid);

	update_prompt();
}

void
edit_note(int nid) 
{
	struct character *curchar = get_current_character();
	if (load_note(nid) == -1) 
		return;
	curchar->note->title = edit_text("Title:", curchar->note->title);
	curchar->note->description = edit_text("Description:", curchar->note->description);
	save_note();
}


void
cmd_delete_note(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	delete_note(curchar->nid);

	update_prompt();
}

void
cmd_show_all_notes(__attribute__((unused)) char *unused)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *title, *description, *nid, *id;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return;
	}

	ret = snprintf(path, sizeof(path), "%s/notes.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No note JSON file found\n");
		return;
	}

	json_object *note;
	if (!json_object_object_get_ex(root, "note", &note)) {
		log_debug("Cannot find a [note] array in %s\n", path);
		return;
	}

	printf("%s's notes\n\n", curchar->name);
	printf("%3s %-25s %s\n", "ID", "Title", "Description");
	temp_n = json_object_array_length(note);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(note, i);
		json_object_object_get_ex(temp, "id", &id);
		if (json_object_get_int(id) != curchar->id)
			continue;

		json_object_object_get_ex(temp, "nid", &nid);
		json_object_object_get_ex(temp, "title", &title);
		json_object_object_get_ex(temp, "description", &description);
		printf("%3d %-25s %s\n",
			json_object_get_int(nid),
			json_object_get_string(title),
			json_object_get_string(description));
	}

	json_object_put(root);
}

int
get_max_note_id(void)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *nid, *id;
	size_t temp_n, i;
	int ret, tvid, max;
	tvid = max = 0;

	if (curchar == NULL) {
		log_debug("No character loaded\n");
		return max;
	}

	ret = snprintf(path, sizeof(path), "%s/notes.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No note JSON file found\n");
		return -1;
	}

	json_object *note;
	if (!json_object_object_get_ex(root, "note", &note)) {
		log_debug("Cannot find a [note] array in %s\n", path);
		return -1;
	}

	temp_n = json_object_array_length(note);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(note, i);
		json_object_object_get_ex(temp, "id", &id);

		/* Count only IDs for the current character */
		if (json_object_get_int(id) != curchar->id)
			continue;

		json_object_object_get_ex(temp, "nid", &nid);
		tvid = json_object_get_int(nid);
		if (tvid > max)
			max = tvid;
	}

	log_debug("Max nid is %d\n", max);

	json_object_put(root);

	return max;
}

void
save_note(void)
{
	struct character *curchar = get_current_character();
	char path[_POSIX_PATH_MAX];
	json_object *root, *items, *id, *nid;
	size_t temp_n, i;
	int ret;

	if (curchar == NULL) {
		log_debug("No character loaded.  No note to save.\n");
		return;
	}
	if (curchar->note == NULL || curchar->nid == -1) {
		log_debug("No current note found.\n");
		return;
	} 
	if (curchar->note->title == NULL || curchar->note->description == NULL || curchar->nid != curchar->note->nid) {
		log_errx(1, "Badly formed note, character=%d, note=%d (%d)", curchar->id, curchar->nid, curchar->note->nid);
		return;
	}

	json_object *cobj = json_object_new_object();
	json_object_object_add(cobj, "id", json_object_new_int(curchar->id));
	json_object_object_add(cobj, "nid", json_object_new_int(curchar->nid));
	json_object_object_add(cobj, "title",
		json_object_new_string(curchar->note->title));
	json_object_object_add(cobj, "description",
		json_object_new_string(curchar->note->description));

	ret = snprintf(path, sizeof(path), "%s/notes.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No note JSON file found\n");
		root = json_object_new_object();
		if (!root)
			log_errx(1, "Cannot create note JSON object\n");

		items = json_object_new_array();
		json_object_array_add(items, cobj);
		json_object_object_add(root, "note", items);
	} else {
		/* Get existing array from JSON */
		if (!json_object_object_get_ex(root, "note", &items)) {
			log_debug("Cannot find a [note] array in %s. Create one\n", path);
			items = json_object_new_array();
			json_object_object_add(root, "note", items);
		}

		temp_n = json_object_array_length(items);
		for (i = 0; i < temp_n; i++) {
			json_object *temp = json_object_array_get_idx(items, i);
			json_object_object_get_ex(temp, "nid", &nid);
			json_object_object_get_ex(temp, "id", &id);
			if (curchar->nid == json_object_get_int(nid) &&
				curchar->id == json_object_get_int(id)) {
				log_debug("Update note entry for %s\n", curchar->name);
				json_object_array_del_idx(items, i, 1);
				json_object_array_add(items, cobj);
				goto out;
			}
		}
		log_debug("No note entry for %s found, adding new one\n", curchar->name);
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
load_note(int nid)
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

	rval = snprintf(path, sizeof(path), "%s/notes.json", get_isscrolls_dir());
	if (rval < 0 || (size_t)rval >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No note JSON file found\n");
		return ret;
	}

	json_object *note;
	if (!json_object_object_get_ex(root, "note", &note)) {
		log_debug("Cannot find a [note] array in %s\n", path);
		return ret;
	}

	temp_n = json_object_array_length(note);
	for (i=0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(note, i);
		json_object_object_get_ex(temp, "nid", &lid);
		json_object_object_get_ex(temp, "id", &id);

		/* Load only notes belonging to the current character */
		if (nid == json_object_get_int(lid) &&
			curchar->id == json_object_get_int(id)) {
			log_debug("Loading note for id: %d\n", json_object_get_int(lid));

			curchar->note->nid = curchar->nid = json_object_get_int(lid);

			json_object_object_get_ex(temp, "title", &title);
			if ((curchar->note->title = calloc(1, MAX_NOTE_TITLE+1)) == NULL)
				log_errx(1, "calloc\n");
			snprintf(curchar->note->title, MAX_NOTE_TITLE, "%s",
				json_object_get_string(title));

			json_object_object_get_ex(temp, "description", &desc);
			if ((curchar->note->description = calloc(1, MAX_NOTE_DESC+1)) == NULL)
				log_errx(1, "calloc\n");
			snprintf(curchar->note->description, MAX_NOTE_DESC, "%s",
				json_object_get_string(desc));
			ret = 1;
			goto out;
		}
	}

out:
	json_object_put(root);

	if (ret != -1)
		log_debug("Successfully loaded note %d for id: %d\n", nid, curchar->id);

	return ret;
}

void
delete_note(int nid)
{
	char path[_POSIX_PATH_MAX];
	json_object *root, *lid;
	size_t temp_n, i;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/notes.json", get_isscrolls_dir());
	if (ret < 0 || (size_t)ret >= sizeof(path)) {
		log_errx(1, "Path truncation happened.  Buffer too short to fit %s\n", path);
	}

	if ((root = json_object_from_file(path)) == NULL) {
		log_debug("No note JSON file found\n");
		return;
	}

	json_object *note;
	if (!json_object_object_get_ex(root, "note", &note)) {
		log_debug("Cannot find a [note] array in %s\n", path);
		return;
	}

	temp_n = json_object_array_length(note);
	for (i = 0; i < temp_n; i++) {
		json_object *temp = json_object_array_get_idx(note, i);
		json_object_object_get_ex(temp, "nid", &lid);
		if (nid == json_object_get_int(lid)) {
			json_object_array_del_idx(note, i, 1);
			log_debug("Deleted note entry with nid %d\n", nid);
			break;
		}
	}

	if (json_object_to_file(path, root))
		printf("Error saving %s\n", path);
	else
		log_debug("Successfully saved %s\n", path);

	json_object_put(root);
}
