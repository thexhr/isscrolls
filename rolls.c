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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isscrolls.h"

#define MAXTOKENS 3

static const char *odds[] = {
	"Almost certain",
	"Likely",
	"50/50",
	"Unlikely",
	"Small chance",
	NULL
};

void
cmd_gather_information(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->wits;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) { /* strong hit */
		change_char_value("momentum", INCREASE, 2);
		printf("You discover something helpful and specific\n");
	} else if (ret == 4 || ret == 14) { /* weak hit */
		change_char_value("momentum", INCREASE, 1);
		printf("The information complicates your quest or introduces a new danger\n");
	} else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_sojourn(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) { /* strong hit */
		printf("You may choose two options -> Rulebook\n");
	} else if (ret == 4 || ret == 14) { /* weak hit */
		printf("You may choose one option -> Rulebook\n");
	} else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_draw_the_circle(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		change_char_value("momentum", INCREASE, 1);
		printf("You may choose even more boasts -> Rulebook\n");
	} else if (ret == 4 || ret == 14) {
		printf("You may choose one boast -> Rulebook\n");
	} else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_swear_an_iron_vow(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	cmd_create_new_vow(NULL);

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		change_char_value("momentum", INCREASE, 2);
		printf("You are emboldened and know what you must do next\n");
	} else if (ret == 4 || ret == 14) {
		change_char_value("momentum", INCREASE, 1);
		printf("You are determined but begin your quest with questions\n");
	} else if (ret == 2 || ret == 12)
		printf("You face a significant obstacle -> Rulebook\n");
}

void
cmd_forge_a_bond(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		printf("You forge a bond and choose one option -> Rulebook\n");
		curchar->bonds += 0.25;
	} else if (ret == 4 || ret == 14) {
		printf("They ask something from you first -> Rulebook\n");
	} else if (ret == 2 || ret == 12)
		printf("You are refused.  Pay the price -> Rulebook\n");
}

void
cmd_mark_a_bond(__attribute__((unused))char *cmd)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->bonds <= 30) {
		printf("You mark a bond\n");
		curchar->bonds += 0.25;
	}
}

void
cmd_burn_momentum(__attribute__((unused))char *cmd)
{
	struct character *curchar = get_current_character();

	CURCHAR_CHECK();

	if (curchar->momentum > curchar->momentum_reset) {
		curchar->momentum = curchar->momentum_reset;
		printf("You burn your momentum and reset it to %d\n",
			curchar->momentum_reset);
	} else {
		printf("Your momentum is lower than your reset momentum.  "
			"Nothing to burn.\n");
	}
}

void
cmd_test_your_bond(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	if (curchar->bonds == 0) {
		printf("You have no bonds forged.  Please do so first\n");
		return;
	}

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		printf("This test has strengthened your bond. Choose one -> Rulebook\n");
	} else if (ret == 4 || ret == 14) {
		printf("Your bond is fragile -> Rulebook\n");
	} else if (ret == 2 || ret == 12) {
		printf("Your bond is cleared.  Pay the price -> Rulebook\n");
		curchar->bonds -= 0.25;
	}
}

void
cmd_endure_stress(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret, hr;

	CURCHAR_CHECK();

	ival[1] = get_int_from_cmd(cmd);
	if (ival[1] == -1) {
		printf("Please provide a number as argument\n\n");
		printf("The number is the amount of stress you suffer\n");
		printf("Example: endurestress 2\n");
		return;
	}

	hr = curchar->spirit - ival[1];
	if (hr >= 0) {
		curchar->spirit -= ival[1];
		printf("You suffer -%d spirit and it is down to %d\n",
			ival[1], curchar->spirit);
	} else {
		/* Spirit is 0, so suffer -momentum equal to remaining health */
		log_debug("hr < 0: %d\n", hr);
		curchar->spirit = 0;
		curchar->momentum -= (hr * (-1));
		printf("You suffer -%d spirit and since your spirit is 0, your "\
			"momentum is down to %d\n",	ival[1], curchar->momentum);
	}

	/* Reset ival[1] since we need no bonus for the roll */
	ival[1] = -1;
	ival[0] = curchar->heart;
	if (curchar->spirit > curchar->heart) {
		ival[0] = curchar->spirit;
	}

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		printf("You shake it off or embrace the darkness -> Rulebook\n");
	} else if (ret == 4 || ret == 14) {
		printf("You press on\n");
	} else if (ret == 2 || ret == 12) {
		change_char_value("momentum", DECREASE, 1);
		if (curchar->health == 0)
			printf("Mark either shaken or corrupted or roll on the oracle table -> Rulebook\n");
	}
}

void
cmd_face_death(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		printf("Death rejects you.\n");
	} else if (ret == 4 || ret == 14) {
		printf("Your must choose one option -> Rulebook\n");
	} else if (ret == 2 || ret == 12) {
		printf("You are dead\n");
		curchar->dead = 1;
	}
}

void
cmd_heal(char *who)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	if (strlen(who) == 0) {
info:
		printf("Please specify who to heal\n\n");
		printf("me\t- heal yourself (roll against Iron or Wits (whatever is lower))\n");
		printf("others\t- heal others (roll against Wits)\n\n");
		printf("Example: heal me\n");
		return;
	}

	if (strcasecmp(who, "me") == 0) {
		if (curchar->iron < curchar->wits)
			ival[0] = curchar->iron;
		else if (curchar->iron > curchar->wits)
			ival[0] = curchar->wits;
		else
			ival[0] = curchar->wits;
	} else if (strcasecmp(who, "others") == 0) {
		ival[0] = curchar->wits;
	} else
		goto info;

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) { /* strong hit */
		change_char_value("health", INCREASE, 2);
		printf("Your care is helpful\n");
	} else if (ret == 4 || ret == 14) { /* weak hit */
		change_char_value("health", INCREASE, 1);
		printf("You healing is successful, but you have to suffer -1 supply or momentum\n");
	} else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_roll_action_dice(char *cmd)
{
	char *ep;
	char *tokens[MAXTOKENS];
	char *p, *last;
	int i = 0;
	long lval[2];
	int ival[2] = { -1, -1 };

	if (cmd == NULL || strlen(cmd) == 0) {
		printf("Please provide at least one attribute value\n\n");
		printf("> action <attribute value> [bonus value]\n\n");
		printf("Examples:\n");
		printf("> action 3\t- Add 3 to the D6 die\n");
		printf("> action 4 1\t- Add 4 and additionally 1 to the D6 die\n\n");

		return;
	}

	log_debug("cmd: %s\n", cmd);

	/* Parse the argument line into 2 tokens, separated by space */
	for ((p = strtok_r(cmd, " ", &last)); p;
		(p = strtok_r(NULL, " ", &last))) {
		if (i < MAXTOKENS - 1)
			tokens[i++] = p;
	}
	tokens[i] = NULL;

	for (i=0; tokens[i] != NULL; i++) {
		log_debug("token %d: %s\n", i, tokens[i]);

		errno = 0;
		lval[i] = strtol(tokens[i], &ep, 10);
		if (cmd[0] == '\0' || *ep != '\0') {
			printf("Please provide a number as argument\n");
			return;
		}
		if ((errno == ERANGE || lval[i] <= 0 || lval[i] > 10)) {
			printf("Please provide a number between 1 and 10\n");
			return;
		}
		ival[i] = lval[i];
	}

	action_roll(ival);
}

void
cmd_resupply(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->wits;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) { /* strong hit */
		change_char_value("supply", INCREASE, 2);
	} else if (ret == 4 || ret == 14) { /* weak hit */
		printf("Take up to +2 supply, but suffer -1 momentum for each\n");
	} else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");

}

void
cmd_face_desolation(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->heart;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) { /* strong hit */
		printf("You resist and press on\n");
	} else if (ret == 4 || ret == 14) { /* weak hit */
		printf("Choose one option -> Rulebook\n");
	} else if (ret == 2 || ret == 12)
		printf("You succumb to despair and horror and are lost -> Rulebook\n");

}

void
cmd_make_camp(char *cmd)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ival[0] = curchar->supply;
	ival[1] = get_int_from_cmd(cmd);

	ret = action_roll(ival);
	if (ret == 8 || ret == 18)
		printf("Choose two options-> Rulebook\n");
	else if (ret == 4 || ret == 14)
		printf("Choose one option-> Rulebook\n");
	else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_face_danger(char *stat)
{
	struct character *curchar = get_current_character();
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	if (strlen(stat) == 0) {
info:
		printf("Please specify the stat you'd like to use in this move\n\n");
		printf("edge\t- Act with speed, agility, or precision\n");
		printf("heart\t- Act with charm, loyalty, or courage\n");
		printf("iron\t- Act with aggressive action, forceful defense, strength\n");
		printf("shadow\t- Act with deception, stealth, or trickery\n");
		printf("wits\t- Act with expertise, insight, or observation\n\n");
		printf("Example: facedanger iron\n");
		return;
	}

	ival[0] = return_char_stat(stat,
		STAT_EDGE|STAT_HEART|STAT_IRON|STAT_WITS|STAT_SHADOW);
	if (ival[0] == -1)
		goto info;

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) /* strong hit */
		change_char_value("momentum", INCREASE, 1);
	else if (ret == 4 || ret == 14) /* weak hit */
		printf("Face a troublesome cost -> Rulebook\n");
	else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_compel(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("Please specify the stat you'd like to use in this move\n\n");
		printf("heart\t- You charm, pacify, barter, or convince\n");
		printf("iron\t- You threaten or incite\n");
		printf("shadow\t- You lie or swindle\n");
		printf("Example: compel iron\n");
		return;
	} else if (ret <= -20)
		return;

	ival[0] = return_char_stat(stat, STAT_HEART|STAT_IRON|STAT_SHADOW);
	if (ival[0] == -1)
		goto info;

	ret = action_roll(ival);
	if (ret == 8 || ret == 18) {
		change_char_value("momentum", INCREASE, 1);
		printf("You might get +1 for your next move -> Rulebook\n");
	} else if (ret == 4 || ret == 14) {
		change_char_value("momentum", INCREASE, 1);
		printf("You might be asked for something in return -> Rulebook\n");
	} else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_secure_an_advantage(char *cmd)
{
	struct character *curchar = get_current_character();
	char stat[MAX_STAT_LEN];
	int ival[2] = { -1, -1 };
	int ret;

	CURCHAR_CHECK();

	ret = get_args_from_cmd(cmd, stat, &ival[1]);
	if (ret >= 10) {
info:
		printf("Please specify the stat you'd like to use in this move\n\n");
		printf("edge\t- Act with speed, agility, or precision\n");
		printf("heart\t- Act with charm, loyalty, or courage\n");
		printf("iron\t- Act with aggressive action, forceful defense, strength\n");
		printf("shadow\t- Act with deception, stealth, or trickery\n");
		printf("wits\t- Act with expertise, insight, or observation\n\n");
		printf("Example: secureanadvantage iron\n");
		return;
	} else if (ret <= -20)
		return;

	ival[0] = return_char_stat(stat,
		STAT_EDGE|STAT_HEART|STAT_IRON|STAT_WITS|STAT_SHADOW);
	if (ival[0] == -1)
		goto info;

	ret = action_roll(ival);
	if (ret == 8 || ret == 18)
		printf("Gain an advantage -> Rulebook\n");
	else if (ret == 4 || ret == 14)
		change_char_value("momentum", INCREASE, 1);
	else if (ret == 2 || ret == 12)
		printf("Pay the price -> Rulebook\n");
}

void
cmd_write_your_epilogue(char *cmd)
{
	struct character *curchar = get_current_character();
	double dval[2] = { -1.0, -1.0 };
	int ret;

	CURCHAR_CHECK();

	dval[0] = curchar->bonds;
	dval[1] = get_int_from_cmd(cmd);

	ret = progress_roll(dval);
	if (ret == 8 || ret == 18) {
		printf("Things come to pass as you hoped\n");
	} else if (ret == 4 || ret == 14) {
		printf("Your life takes an unexpected turn, but not necessary for the worse"\
			" -> Rulebook\n");
	} else if (ret == 2 || ret == 12) {
		printf("Your fears are realized\n");
	}
}

void
cmd_roll_challenge_die(__attribute__((unused)) char *unused)
{
	if (get_color())
		printf("<%ld>\n", roll_challenge_die());
	else
		printf("%ld\n", roll_challenge_die());
}

void
cmd_roll_oracle_die(__attribute__((unused)) char *unused)
{
	if (get_color())
		printf("<%ld>\n", roll_oracle_die());
	else
		printf("%ld\n", roll_oracle_die());
}

long
roll_action_die(void)
{
	long ret = random() % 6;

	return ret == 0 ? 6 : ret;
}

long
roll_challenge_die(void)
{
	return random() % 10;
}

long
roll_oracle_die(void)
{
	return random() % 100;
}

void
cmd_yes_or_no(char *args)
{
	int num = atoi(args);
	int i;

	log_debug("Argument %d\n", num);
	if (num <= 0 || num > 5) {
		printf("Provide a number between 1-5 as argument, i.e. yesorno 2\n\n");
		for (i=0; odds[i] != NULL; i++)
			printf("%d - %s\n", i+1, odds[i]);

		return;
	}

	yes_or_no(num);
}

void
yes_or_no(int num)
{
	long a1, c1, c2;
	int match = 0;

	a1 = roll_challenge_die();
	c2 = roll_challenge_die();
	c1 = (a1 * 10) + c2;

	if (a1 == c2) {
		if (get_color())
			printf("<%ld><%ld> match -> ", a1, a1);
		else
			printf("%ld, %ld match -> ", a1, a1);
		match = 1;
	} else {
		if (get_color())
			printf("<%ld><%ld> -> ", a1, c2);
		else
			printf("%ld, %ld ", a1, c2);
	}

	if (num == 1 && c1 >= 11)
		pm(GREEN, "yes");
	else if (num == 2 && c1 >= 26)
		pm(GREEN, "yes");
	else if (num == 3 && c1 >= 51)
		pm(GREEN, "yes");
	else if (num == 4 && c1 >= 76)
		pm(GREEN, "yes");
	else if (num == 5 && c1 >= 91)
		pm(GREEN, "yes");
	else
		pm(RED, "no");

	if (match)
		printf(" (an extreme result or twist has occured)\n");
	else
		printf("\n");
}

int
action_roll(int args[2])
{
	struct character *curchar = get_current_character();
	long c1, c2, a1, b;
	int ret = 0, match = 0;

	log_debug("Action args: %d, %d\n", args[0], args[1]);

	if (args[0] == -1) {
		log_errx(1, "No attribute value provided. This should not happen!");
	}

	a1 = roll_action_die();

	/* We have a character loaded, so no standalone action roll */
	if (curchar != NULL) {
		/* Character has negative momentum which equals action dice roll */
		if (curchar->momentum < 0 && (curchar->momentum * -1) == a1) {
			log_debug("Negative momentum equals die roll: %ld -> 0\n", a1);
			a1 = 0;
		}
	}

	b = a1;
	/* Add attribute and maybe a bonus value */
	b += args[0];
	if (args[1] != -1)
		b += args[1];

	if (args[1] == -1) {
		if (get_color())
			printf("<%ld> + %d = %ld ", a1, args[0], b);
		else
			printf("%ld + %d = %ld ", a1, args[0], b);
	}
	else {
		if (get_color())
			printf("<%ld> + %d + %d = %ld ", a1, args[0], args[1], b);
		else
			printf("%ld + %d + %d = %ld ", a1, args[0], args[1], b);
	}

	/* Roll challenge die and replace a 0 with 10 for both cosmetic and
	 * arithmetic reasons */
	c1 = roll_challenge_die();
	c2 = roll_challenge_die();
	c1 = (c1 == 0 ? 10 : c1);
	c2 = (c2 == 0 ? 10 : c2);

	/* Use a special way to display matches so that they don't get
	 * unnoticed for the player */
	if (c1 == c2) {
		match = 10;
		if (get_color())
			printf("vs <%ld> match ", c1);
		else
			printf("vs %ld match ", c1);
	} else {
		if (get_color())
			printf("vs <%ld><%ld> ", c1, c2);
		else
			printf("vs %ld, %ld ", c1, c2);
	}

	/* Reset strong hit indicator for the loaded character, it will be re-set in
	 * the next code block */
	if (curchar != NULL)
		curchar->strong_hit = 0;

	if (b <= c1 && b <= c2) {
		pm(RED, "miss\n");
		ret = 2;
		/* Increase the failure track by one tick on every miss */
		modify_double("failure", &curchar->failure_track, 10.0, 0.0, 0.25, INCREASE);
	} else if (b <= c1 || b <= c2) {
		pm(YELLOW, "weak hit\n");
		ret = 4;
	} else if (b > c1 && b > c2) {
		pm(GREEN, "strong hit\n");
		if (curchar != NULL)
			curchar->strong_hit = 1;
		ret = 8;
	}

	/* In case of a match, 10 are added */
	return ret + match;
}

int
progress_roll(double args[2])
{
	struct character *curchar = get_current_character();
	double pr_score;
	long c1, c2;
	int ret = 0, match = 0;

	if (args[0] == -1) {
		log_errx(1, "No attribute value provided. This should not happen!");
	}

	/* A progress roll needs a loaded character */
	if (curchar == NULL)
		return -1;

	/* Roll challenge die and replace a 0 with 10 for both cosmetic and
	 * arithmetic reasons */
	c1 = roll_challenge_die();
	c2 = roll_challenge_die();
	c1 = (c1 == 0 ? 10 : c1);
	c2 = (c2 == 0 ? 10 : c2);

	log_debug("args[0] %.2lf args[1] %.2lf\n", args[0], args[1]);

	pr_score = args[0];
	if (args[1] != -1)
		pr_score += args[1];

	if (c1 == c2) {
		match = 10;
		if (get_color())
			printf("<%ld> match vs ", c1);
		else
			printf("%ld match vs ", c1);
	} else {
		if (get_color())
			printf("<%ld><%ld> vs ", c1, c2);
		else
			printf("%ld, %ld vs ", c1, c2);
	}

	printf("Progress: %.2lf ", pr_score);

	if (pr_score <= c1 && pr_score <= c2) {
		pm(RED, "miss\n");
		ret = 2;
		/* Increase the failure track by two ticks on every miss */
		modify_double("failure", &curchar->failure_track, 10.0, 0.0, 0.5, INCREASE);
	} else if (pr_score <= c1 || pr_score <= c2) {
		pm(YELLOW, "weak hit\n");
		ret = 4;
	} else if (pr_score > c1 && pr_score > c2) {
		pm(GREEN, "strong hit\n");
		ret = 8;
	}

	return ret + match;
}

int
get_int_from_cmd(const char *cmd)
{
	char *ep;
	long lval;
	int ival = -1;

	if (strlen(cmd) > 0) {
		errno = 0;
		lval = strtol(cmd, &ep, 10);
		if (cmd[0] == '\0' || *ep != '\0') {
			printf("Please provide a number as argument\n");
			return ival;
		}
		if ((errno == ERANGE || lval <= 0 || lval > 10)) {
			printf("Please provide a number between 1 and 10\n");
			return ival;
		}

		ival = lval;
		log_debug("Arg provided %d\n", ival);
	}

	return ival;
}

int
get_args_from_cmd(char *cmd, char *stat, int *ival)
{
	char *tokens[MAXTOKENS];
	char *ep;
	char *p, *last;
	int i = 0;
	long lval = -1;

	/* Parse the argument line into max tokens, separated by space */
	for ((p = strtok_r(cmd, " ", &last)); p;
		(p = strtok_r(NULL, " ", &last))) {
		if (i < MAXTOKENS - 1)
			tokens[i++] = p;
	}
	tokens[i] = NULL;

	/* First token is a stat */
	i = 0;
	if (tokens[i] == NULL)
		return 10;
	else if (strlen(tokens[i]) == 0)
		return 11;

	log_debug("stat token: %s\n", tokens[i]);
	snprintf(stat, MAX_STAT_LEN, "%s", tokens[i]);

	/* Second token is a bonus value*/
	errno = 0;
	i++;
	/* It's OK to have no bonus, so the token can be NULL and we can return */
	if (tokens[i] == NULL)
		return 0;
	else
		log_debug("bonus token: %s\n", tokens[i]);

	lval = strtol(tokens[i], &ep, 10);
	if (cmd[0] == '\0' || *ep != '\0') {
		printf("Please provide a number as argument\n");
		return -20;
	}
	if ((errno == ERANGE || lval <= 0 || lval > 10)) {
		printf("Please provide a number between 1 and 10\n");
		return -22;
	}
	*ival = lval;

	return 0;
}
