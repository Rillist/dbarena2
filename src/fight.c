/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*    ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*    ROM has been brought to you by the ROM consortium                      *
*        Russ Taylor (rtaylor@hypercube.org)                                *
*        Gabrielle Taylor (gtaylor@hypercube.org)                           *
*        Brian Moore (zump@rom.org)                                         *
*    By using this code, you have agreed to follow the terms of the         *
*    ROM license, in the file Rom24/doc/rom.license                         *
****************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "interp.h"

/*
 * Local functions.
 */
bool legal_attack args ( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void check_assist args ( ( CHAR_DATA * ch, CHAR_DATA * victim ) );
void check_killer args ( ( CHAR_DATA * ch, CHAR_DATA * victim ) );
void FinishMessage args ( ( CHAR_DATA * pCh, CHAR_DATA * pVictim, long long int llDam, int nDt, bool bImmune ) );
void dam_message  args ( ( CHAR_DATA * ch, CHAR_DATA * victim, long long int dam, int dt, bool immune ) );
void death_cry    args ( ( CHAR_DATA * ch ) );
void group_gain   args ( ( CHAR_DATA * ch, CHAR_DATA * victim ) );
bool is_safe      args ( ( CHAR_DATA * ch, CHAR_DATA * victim ) );
void make_corpse  args ( ( CHAR_DATA * ch ) );
void mob_hit      args ( ( CHAR_DATA * ch, CHAR_DATA * victim, int dt ) );
void set_fighting args ( ( CHAR_DATA * ch, CHAR_DATA * victim ) );
void disarm       args ( ( CHAR_DATA * ch, CHAR_DATA * victim ) );
bool chance       args ( ( int num ) );
bool mobai_warrior args ( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

// Make the character lose some ki after an attack/skill/whatever
// Mod is how much is lost, 1 being the least, higher more
// Use 40    for extremely draining attacks
//     30-39 for most one-shot ki attacks
//     10    for normal combat hits
//     1-10  for charging ki attacks
void ki_loss (CHAR_DATA *ch, sh_int mod) {
    long long int loss;

	if (ch->ki > 0) {
		//loss  = sqrt(ch->nCurPl * ch->llPl / 100);
        // The stronger you are the more effort you use?
        // Not sure if this is the best thing to use
        loss = mod + get_curr_stat(ch, STAT_STR);
        switch (GetTrans(ch)) {
            case TRANS_SSJ1:
            case TRANS_SSJ2:
            case TRANS_ICER2:
            case TRANS_ICER3:
            case TRANS_SUPERH:
            case TRANS_HYPERN:
                loss *= 2;
                break;

            case TRANS_SSJ3:
            case TRANS_SSJ4:
            case TRANS_ICER4:
            case TRANS_ICER5:
            case TRANS_MYSTIC:
            case TRANS_SUPERN:
            case TRANS_SELFFUSE:
                loss *= 3;
                break;

            case TRANS_SSJ5:
                loss /= 4;
                break;
		}
        if (IS_AFFECTED(ch, AFF_KAIOKEN))
            loss *= 2;

        loss = ch->nCurPl * loss / 100;

		// Lower/increase the losses based on stats -- use 10 as a base stat
		//loss = 20 * loss / (get_curr_stat(ch,STAT_WIL)+get_curr_stat(ch,STAT_INT));
        loss /= 25;
        loss = UMAX(1, loss);
		ch->ki = UMAX(0, ch->ki - loss);
	}
	else
		ch->nCurPl = UMAX(0, ch->nCurPl - 10);
}


bool chance (int num) {
    if (number_range(1,100) <= num)
        return TRUE;
    else
        return FALSE;
}

bool legal_attack (CHAR_DATA *ch, CHAR_DATA *victim) {
    if (victim == ch) {
        sendch ("Very funny.\n\r", ch);
        return FALSE;
    }

    if (is_safe (ch, victim))
        return FALSE;

    if (IS_NPC (victim) && victim->fighting != NULL && !is_same_group (ch, victim->fighting)) {
        sendch ("Kill stealing is not permitted.\n\r", ch);
        return FALSE;
    }

    if (IS_AFFECTED (ch, AFF_CHARM) && ch->master == victim) {
        act ("But $N is such a good friend!", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    return TRUE;
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update (void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj_next;
    bool room_trig = FALSE;

    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        ch_next = ch->next;

		// Return balance to normal.
        // Balance will attempt to balance out at 6-7
		if (ch->balance > 7)
			if (number_range(1,7) == 1)
                set_balance (ch, -1, TRUE);
		if (ch->balance < 6)
			if (number_range(1,7) == 1)
                set_balance (ch, 1, TRUE);

        if ((victim = ch->fighting) == NULL || ch->in_room == NULL)
            continue;

		if (!IS_AWAKE (ch) && ch->in_room != victim->in_room)
            stop_fighting (ch, FALSE);
		else if (IS_NPC(ch))
			mob_hit (ch, victim, TYPE_UNDEFINED);

        if ((victim = ch->fighting) == NULL)
            continue;

        /*
         * Fun for the whole family!
         */
        check_assist (ch, victim);

        if (IS_NPC (ch)) {
            if (HAS_TRIGGER_MOB (ch, TRIG_FIGHT))
                p_percent_trigger (ch, NULL, NULL, victim, NULL, NULL, TRIG_FIGHT);
            if (HAS_TRIGGER_MOB (ch, TRIG_HPCNT))
                p_hprct_trigger (ch, victim);
        }

		for ( obj = ch->carrying; obj; obj = obj_next )
		{
			obj_next = obj->next_content;

			if ( obj->wear_loc != WEAR_NONE && HAS_TRIGGER_OBJ( obj, TRIG_FIGHT ) )
			    p_percent_trigger( NULL, obj, NULL, victim, NULL, NULL, TRIG_FIGHT );
		}

		if ( HAS_TRIGGER_ROOM( ch->in_room, TRIG_FIGHT ) && room_trig == FALSE )
		{
			room_trig = TRUE;
			p_percent_trigger( NULL, NULL, ch->in_room, victim, NULL, NULL, TRIG_FIGHT );
		}
    }

    return;
}

/* for auto assisting */
void check_assist (CHAR_DATA * ch, CHAR_DATA * victim)
{
    CHAR_DATA *rch, *rch_next;

    for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
    {
        rch_next = rch->next_in_room;

        if (IS_AWAKE (rch) && rch->fighting == NULL)
        {

            /* quick check for ASSIST_PLAYER */
            if (!IS_NPC (ch) && IS_NPC (rch)
                && IS_SET (rch->off_flags, ASSIST_PLAYERS))
            {
                do_function (rch, &do_emote, "screams and attacks!");
                begin_combat (rch, victim);
                continue;
            }

            /* PCs next */
            if (!IS_NPC (ch) || IS_AFFECTED (ch, AFF_CHARM))
            {
                if (((!IS_NPC (rch) && IS_SET (rch->act, PLR_AUTOASSIST))
                     || IS_AFFECTED (rch, AFF_CHARM))
                    && is_same_group (ch, rch) && !is_safe (rch, victim))
                    begin_combat (rch, victim);

                continue;
            }

            /* now check the NPC cases */

            if (IS_NPC (ch) && !IS_AFFECTED (ch, AFF_CHARM))
            {
                if ((IS_NPC (rch) && IS_SET (rch->off_flags, ASSIST_ALL))
                    || (IS_NPC (rch) && rch->group && rch->group == ch->group)
                    || (IS_NPC (rch) && rch->race == ch->race
                        && IS_SET (rch->off_flags, ASSIST_RACE))
                    || (IS_NPC (rch) && IS_SET (rch->off_flags, ASSIST_ALIGN)
                        && ((IS_GOOD (rch) && IS_GOOD (ch))
                            || (IS_EVIL (rch) && IS_EVIL (ch))
                            || (IS_NEUTRAL (rch) && IS_NEUTRAL (ch))))
                    || (rch->pIndexData == ch->pIndexData
                        && IS_SET (rch->off_flags, ASSIST_VNUM)))
                {
                    CHAR_DATA *vch;
                    CHAR_DATA *target;
                    int number;

                    if (number_bits (1) == 0)
                        continue;

                    target = NULL;
                    number = 0;
                    for (vch = ch->in_room->people; vch; vch = vch->next)
                    {
                        if (can_see (rch, vch)
                            && is_same_group (vch, victim)
                            && number_range (0, number) == 0)
                        {
                            target = vch;
                            number++;
                        }
                    }

                    if (target != NULL)
                    {
                        do_function (rch, &do_emote, "screams and attacks!");
                        begin_combat (rch, target);
                    }
                }
            }
        }
    }
}


// Make the character enter combat with the victim
void begin_combat (CHAR_DATA *ch, CHAR_DATA *victim) {

	// Start the fighting
    if (victim != ch)
    {
        if (is_safe (ch, victim))
            return;
        check_killer (ch, victim);

        if (victim->position > POS_STUNNED)
        {
            if (victim->fighting == NULL)
            {
                set_fighting (victim, ch);
                if (IS_NPC (victim) && HAS_TRIGGER_MOB (victim, TRIG_KILL))
                    p_percent_trigger (victim, NULL, NULL, ch, NULL, NULL, TRIG_KILL);
            }
            if (victim->timer <= 4)
                victim->position = POS_FIGHTING;

            if (ch->fighting == NULL)
                set_fighting (ch, victim);
        }

        /*
         * Charm stuff.
         */
        if (victim->master == ch)
            stop_follower (victim);
    }
}


// AI function for mobs with the ACT_WARRIOR act flag.
// Returns TRUE if the mob did something, FALSE if
// it needs further "processing"
bool mobai_warrior (CHAR_DATA *ch, CHAR_DATA *victim) {
    sh_int off_sn[5] = {gsn_kick, gsn_knee, gsn_throat_shot, gsn_heart_shot, gsn_elbow};
    int n, sn;

    // Below 10% health, 1 in 3 chance
    if (IS_SET (ch->act, ACT_WIMPY) && number_range(1, 3) == 1 && 100 * ch->hit / UMAX(1,ch->max_hit) < 10) {
        if (ch->balance < 7)
            do_function (ch, &do_retreat, "");
        else
            do_function(ch, &do_flee, "");
        return TRUE;
    }
    // Powerlevel
    else if (number_range(1,3) == 1 && ch->nCurPl <= 33) {
		do_function(ch, &do_power, "max");
        return TRUE;
	/*
		if (victim->llPl > ch->llPl * 2) {
            do_function(ch, &do_power, "max");
            return TRUE;
        }
        else {
            do_function(ch, &do_power, "50");
            return TRUE;
        }
		*/
    }
    // Ki getting low?
    else if ((ch->ki < ch->max_ki / 10) && (ch->nCurPl >= 33)) {
        do_function(ch, &do_power, "base");
        return TRUE;
    }

    // Check balance
    if (victim->balance - ch->balance > 5 && number_range(1,2) == 1) {
        do_function(ch, &do_retreat, "");
        return TRUE;
    }
    else if (victim->balance - ch->balance > 3 && number_range(1,2) == 1) {
        do_function(ch, &do_defend, "");
        return TRUE;
    }

    // Try to blind
    if (number_range(1,3) == 1 && !IS_AFFECTED(victim, AFF_BLIND))
        if (get_skill(ch, gsn_eye_gouge) > 1)
            if (skill_driver(ch, "", gsn_eye_gouge))
                return TRUE;

    // Do some offensive thing
    switch (number_range(1,2)) {
        // Balance:
        case 1:
            if (ch->balance - victim->balance <= 2  && number_range(1,2) == 1) {
                n = number_range(1,3);
                // Retreat to gain more, or offensively gain more
                if (get_skill(ch, gsn_sweep) > 1 && n == 1 && !IS_AFFECTED(victim, AFF_FLYING)) {
                    if (skill_driver(ch, "", gsn_sweep))
                        return TRUE;
                }
                else if (get_skill(ch, gsn_bash) > 1 && n == 2) {
                    if (skill_driver(ch, "", gsn_bash))
                        return TRUE;
                }
                else {
                    do_function(ch, &do_defend, "");
                    return TRUE;
                }
            }
            break;

        // Offensive skill
        case 2:
            if (number_range(1,2) == 1) {
                // Find a random, offensive skill
                while (TRUE) {
                    sn = off_sn[number_range(0,4)];
                    if (IS_AFFECTED(victim, AFF_FLYING) && sn == gsn_sweep)
                        continue;
                    else
                        break;
                }
                if (skill_driver(ch, "", sn))
                    return TRUE;
            }
            break;
    }
    // Basic attack
    if (ch->balance > victim->balance && number_range(1,4) > 1) {
        do_function(ch, &do_attack, "heavy");
        return TRUE;
    }
    else if (ch->balance < victim->balance && number_range(1,4) > 1) {
        do_function(ch, &do_attack, "light");
        return TRUE;
    }
    do_function(ch, &do_attack, "");
    return TRUE;
}


// AI function for mobs with the ACT_KIWARRIOR act flag.
// Returns TRUE if the mob did something, FALSE if
// it needs further "processing".
// Also used in damage to help protect link-dead characters
bool mobai_kiwarrior (CHAR_DATA *ch, CHAR_DATA *victim) {
    sh_int off_sn[15] = {gsn_kick, gsn_knee, gsn_throat_shot, gsn_heart_shot, gsn_elbow,
                        gsn_kamehameha, gsn_finalflash, gsn_energy_ball, gsn_energy_beam,
                        gsn_fingerbeam, gsn_mouthbeam, gsn_eyebeam, gsn_masenko, gsn_specialbeam,
                        gsn_destructo_disk};
    int n, sn;

    // Below 10% health, 1 in 3 chance
    if (IS_SET (ch->act, ACT_WIMPY) && number_range(1, 3) == 1 && 100 * ch->hit / UMAX(1,ch->max_hit) < 10) {
        if (ch->balance < 7)
            do_function (ch, &do_retreat, "");
        else
            do_function(ch, &do_flee, "");
        return TRUE;
    }
    // Powerlevel
    else if (number_range(1,3) == 1 && ch->nCurPl <= 33) {
        do_function(ch, &do_power, "max");
        return TRUE;
	    /*
		if (victim->llPl > ch->llPl * 2) {
            do_function(ch, &do_power, "max");
            return TRUE;
        }
        else {
            do_function(ch, &do_power, "50");
            return TRUE;
        }
		*/
    }
    // Ki getting low?
    else if ((ch->ki < ch->max_ki / 10) && ch->nCurPl >= 33) {
        do_function(ch, &do_power, "base");
        return TRUE;
    }

    // Check balance
    if (victim->balance - ch->balance > 5 && number_range(1,2) == 1) {
        do_function(ch, &do_retreat, "");
        return TRUE;
    }
    else if (victim->balance - ch->balance > 3 && number_range(1,2) == 1) {
        do_function(ch, &do_defend, "");
        return TRUE;
    }

    // Try to blind
    if (number_range(1,3) == 1 && !IS_AFFECTED(victim, AFF_BLIND)) {
        if (get_skill(ch, gsn_solarflare) > 1) {
            if (skill_driver(ch, "", gsn_solarflare))
                return TRUE;
        }
        else if (get_skill(ch, gsn_eye_gouge) > 1) {
            if (skill_driver(ch, "", gsn_eye_gouge))
                return TRUE;
        }
    }

    // Do some offensive thing
    switch (number_range(1,2)) {
        // Balance:
        case 1:
            if (ch->balance - victim->balance <= 2  && number_range(1,2) == 1) {
                n = number_range(1,5);
                // Retreat to gain more, or offensively gain more
                if (get_skill(ch, gsn_scattershot) > 1 && (n == 1 || n == 2)) {
                    if (skill_driver(ch, "", gsn_scattershot))
                        return TRUE;
                }
                else if (get_skill(ch, gsn_sweep) > 1 && n == 3 && !IS_AFFECTED(victim, AFF_FLYING)) {
                    if (skill_driver(ch, "", gsn_sweep))
                        return TRUE;
                }
                else if (get_skill(ch, gsn_bash) > 1 && n == 4) {
                    if (skill_driver(ch, "", gsn_bash))
                        return TRUE;
                }
                else {
                    do_function(ch, &do_defend, "");
                    return TRUE;
                }
            }
            break;

        // Offensive skill
        case 2:
            if (number_range(1,2) == 1) {
                // Find a random, offensive skill
                while (TRUE) {
                    sn = off_sn[number_range(0,4)];
                    if (IS_AFFECTED(victim, AFF_FLYING) && sn == gsn_sweep)
                        continue;
                    else
                        break;
                }
                if (skill_driver(ch, "", sn))
                    return TRUE;
            }
            break;
    }
    // Basic attack
    if (ch->balance > victim->balance && number_range(1,4) > 1 ) {
        do_function(ch, &do_attack, "heavy");
        return TRUE;
    }
    else if (ch->balance < victim->balance && number_range(1,4) > 1) {
        do_function(ch, &do_attack, "light");
        return TRUE;
    }
    do_function(ch, &do_attack, "");
    return TRUE;
}


// HACK:
// kamehameha is the first ki skill in the skill table, and ki attacks
// come last in the table. FIRST_KI_SN lets us determine which skills
// are ki skills or not (every sn >= FIRST_KI_SN is a ki skill)
#define FIRST_KI_SN gsn_kamehameha

/* procedure for all mobile attacks */
void mob_hit (CHAR_DATA * ch, CHAR_DATA * victim, int dt) {
    char *attack_type[3] = {"light", "", "heavy"}; // Different types of attacks
    int number;
    int i, n, sn;

    if (ch->fighting != victim || victim == NULL)
        return;

    // Area attack -- BALLS nasty!
/*
    if (IS_SET (ch->off_flags, OFF_AREA_ATTACK))
    {
        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next;
            if ((vch != victim && vch->fighting == ch))
                one_hit (ch, vch, dt);
        }
    }
*/
    if (ch->wait > 0 || (ch->wait_skill_sn > 0 && ch->wait_skill > 0))
        return;

    if (ch->charge > skill_table[ch->wait_skill_sn].wait) {
		do_function(ch, &do_release, "");
		return;
	}

	if (ch->wait_skill_sn > 0 && ch->charge > 0)
		return;


	if (IS_SET (ch->act, ACT_WIMPY) && ch->ki < 1) {
        if (ch->balance < 7)
            do_function (ch, &do_retreat, "");
        else
            do_function(ch, &do_flee, "");
		return;
	}

	// Animals
	if (IS_SET(ch->act, ACT_ANIMAL)) {
		// Below 25% health, 1 in 2 chance
		if (IS_SET (ch->act, ACT_WIMPY) && number_range(1, 2) == 1 && 100 * ch->hit / UMAX(1,ch->max_hit) < 25) {
            if (ch->balance < 7)
                do_function (ch, &do_retreat, "");
            else
                do_function(ch, &do_flee, "");
        }
        else if (number_range(1,3) == 1 && ch->nCurPl <= 33)
            ch->nCurPl = 100; // Fake a power up (since animals dont really power up)
        else
            do_function(ch, &do_attack, attack_type[number_range(0,2)]);
	}
	// Warrior
	else if (IS_SET(ch->act, ACT_WARRIOR)) {
	    if (mobai_warrior(ch, victim))
	        return;
    }
	// Warriors -- with ki!
	else if (IS_SET(ch->act, ACT_KIWARRIOR)) {
	    if (mobai_kiwarrior(ch, victim))
	        return;
    }
	// Everything else defaults to ACT_CIVILIAN behavior
	else {
		// Below 10% health, 1 in 3 chance
		if (IS_SET (ch->act, ACT_WIMPY) && number_range(1, 3) == 1 && 100 * ch->hit / UMAX(1,ch->max_hit) < 10) {
		    if (ch->balance < 7)
                do_function (ch, &do_retreat, "");
            else
                do_function(ch, &do_flee, "");
			return;
		}
        else if (number_range(1,3) == 1 && ch->nCurPl <= 33) {
            ch->nCurPl = 100; // Fake it (dont want to see civilians powering up)
            return;
        }
        // Do a random skill
		else if (number_range(1,4) == 1) {
			number = 0;
			sn = 0;
			// Find a random skill
			for (i=0; i < FIRST_KI_SN; ++i) {
 				if (skill_table[i].command && (n = number_range(1,100)) > number && get_skill(ch, i) > 0) {
					sn = i;
					number = n;
				}
			}
			if (sn != 0)
				if (skill_driver(ch, "", sn))
					return;
		}
		do_function(ch, &do_attack, attack_type[number_range(0,2)]);
	}
}

/*
 * Inflict damage from a hit/ki attack/anything
 */
bool damage (CHAR_DATA * ch, CHAR_DATA * victim, long long int dam, int dt, int dam_type, bool show)
{
    OBJ_DATA *corpse;
    bool immune;
	int chance, percent;
    OBJ_DATA *obj, *dam_obj;
    int iWear;

    // The second part takes care ROOM_CHAOS rooms where they stay
    // in the same room, with position SLEEPING.
    // Or, if they are killed in the respawn room
    if (victim->position == POS_DEAD || victim->position == POS_UNCONSCIOUS || victim->hit < -11)
        return FALSE;

	if (victim->in_room != ch->in_room)
		return FALSE;

    if (ch->level < IMMORTAL
        && !IS_NPC (ch) && !IS_NPC (victim)
        && !IS_SET(ch->in_room->room_flags, ROOM_CHAOS)
		&& ch->nDifficulty + (ch->nDifficulty / 4) <= victim->nDifficulty
		&& ch->nDifficulty - (ch->nDifficulty / 4) >= victim->nDifficulty) {
        sendch("Your target is out of hostile range.\n\r", ch);
        return FALSE;
    }


    /*
     * Stop up any residual loopholes.
     */
    /*
	if (dam > 2500 && dt >= TYPE_HIT)
    {
        logstr (LOG_BUG, "Damage: %d: more than 2500 points!", dam);
        dam = 2500;
        if (!IS_IMMORTAL (ch))
        {
            OBJ_DATA *obj;
            obj = get_eq_char (ch, WEAR_WIELD);
            sendch ("You really shouldn't cheat.\n\r", ch);
            if (obj != NULL)
                extract_obj (obj);
        }

    }
	*/

	dam = UMIN(dam,MAX_HP);

    if (victim != ch) {
        /*
         * Certain attacks are forbidden.
         * Most other attacks are returned.
         */
        if (is_safe (ch, victim))
            return FALSE;
        check_killer (ch, victim);

        if (victim->position > POS_STUNNED) {
            if (victim->fighting == NULL) {
                set_fighting (victim, ch);
                if (IS_NPC (victim) && HAS_TRIGGER_MOB (victim, TRIG_KILL))
                    p_percent_trigger (victim, NULL, NULL, ch, NULL, NULL, TRIG_KILL);
            }
            if (victim->timer <= 4)
                victim->position = POS_FIGHTING;
        }
        if (victim->position > POS_STUNNED)
            if (ch->fighting == NULL)
                set_fighting (ch, victim);

        /*
         * More charm stuff.
         */
        if (victim->master == ch)
            stop_follower (victim);
    }

    /*
     * Inviso attacks ... not.
     */
  	if (IS_AFFECTED (ch, AFF_INVISIBLE)) {
        //affect_strip (ch, gsn_invis);
        //affect_strip (ch, gsn_mass_invis);
        REMOVE_BIT (ch->affected_by, AFF_INVISIBLE);
        act ("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
    }

    // Just do a check here
    if (IS_AFFECTED (ch, AFF_KAIOKEN)) {
        ImproveSkill (ch, gsn_kaioken, TRUE, 10, ch->nDifficulty);
		ImproveStat (ch, STAT_INT, TRUE, 10, ch->nDifficulty);
	}
    if (IS_AFFECTED (ch, AFF_FLYING)) {
        ImproveSkill (ch, gsn_fly, TRUE, 10, ch->nDifficulty);
		ImproveStat (ch, STAT_INT, TRUE, 10, ch->nDifficulty);
	}

    /*
     * Damage modifiers.
     */
    immune = FALSE;

    switch (check_immune (victim, dam_type)) {
        case (IS_IMMUNE):
            immune = TRUE;
            dam = 0;
            break;
        case (IS_RESISTANT):
            if (dam > 1)
                dam -= dam / 3;
            break;
        case (IS_VULNERABLE):
            if (dam > 1)
                dam += dam / 2;
            break;
    }

    if (dam >= 1) {
		/*
		 * Hurt the victim.
		 * Inform the victim of his new state.
		 */
		victim->hit -= dam;
		if (!IS_NPC (victim) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1)
			victim->hit = 1;

		/*
		if (!IS_NPC(ch) && IS_SET(ch->act, PLR_FINISHMOVE) && victim->position > POS_MORTAL && victim->hit <= -5) {
			// Set to mortally wounded, ready for a finishing move
			act ("{RYou bring $N within an inch of $S life, stopping to deliver the finishing blow.{x", ch, NULL, victim, TO_CHAR);
			act ("{R$n brings $N within an inch of $S life, stopping to deliver the finishing blow.{x", ch, NULL, victim, TO_NOTVICT);
			act ("{R$n brings you within an inch of your life, stopping to deliver the finishing blow.{x", ch, NULL, victim, TO_VICT);
			victim->hit = -5;
		}*/
		update_pos (victim);
	}

	if (show && victim->position == POS_DEAD)
	    FinishMessage (ch, victim, dam, dt, immune);
	else if (show)
		dam_message (ch, victim, dam, dt, immune);

	if (dam < 1)
		return TRUE;

	if (dam > victim->max_hit / 2)
		disrupt (victim);
	else if (dam > victim->max_hit / 5 && number_range(1,3) == 1)
    	disrupt (victim);
	else if (dam > victim->max_hit / 10 && number_range(1,5) == 1)
    	disrupt (victim);

	if (dam > victim->max_hit / 20)
		rage (victim, victim->trans_count + 1);

	// Damage some eq
	chance = 0;
	dam_obj = NULL;
	for (iWear = 0; iWear < MAX_WEAR; iWear++) {
		if ((obj = get_eq_char (victim, iWear)) != NULL) {
			percent = number_percent();
			// Inc/dec probability based on location
			switch (iWear) {
				case WEAR_FINGER_L:
				case WEAR_FINGER_R:
				case WEAR_EAR_L:
				case WEAR_EAR_R:
					percent -= 50;
					break;
				case WEAR_NECK_1:
				case WEAR_NECK_2:
				case WEAR_HANDS:
				case WEAR_LIGHT:
				case WEAR_WRIST_L:
				case WEAR_WRIST_R:
				case WEAR_TAIL:
					percent -= 25;
					break;
				case WEAR_HEAD:
				case WEAR_LEGS:
				case WEAR_FEET:
				case WEAR_WAIST:
				case WEAR_ARMS:
				case WEAR_FLOAT:
				case WEAR_WIELD:
				case WEAR_EYE:
					percent += 0;
					break;
				case WEAR_SHIELD:
				case WEAR_ABOUT:
				case WEAR_HOLD:
					percent += 10;
					break;
				case WEAR_BODY:
					percent += 25;
					break;
				default:
					percent += 0;
			}
			if (percent > chance) {
				chance = percent;
				dam_obj = obj;
			}
		}
	}
	if (dam_obj)
		damage_obj (dam_obj, dam / 100);

    switch (victim->position) {
        case POS_MORTAL:
            act ("$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM);
            sendch ("You are mortally wounded, and will die soon, if not aided.\n\r", victim);
            break;

        case POS_INCAP:
            act ("$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM);
            sendch ("You are incapacitated and will slowly die, if not aided.\n\r", victim);
            break;

        case POS_STUNNED:
            act ("$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM);
            sendch ("You are stunned, but will probably recover.\n\r", victim);
            break;

        case POS_UNCONSCIOUS:
            act("{R$n has been knocked unconscious!{x", victim, NULL, NULL, TO_ROOM);
            sendch ("{RYou have been knocked unconscious!{x\n\r\n\r", victim);
            break;

        case POS_DEAD:
            // Increment evolution counter for bio-androids
            if ((race_lookup("bio-android") == victim->race || race_lookup("android") == victim->race)
                && victim->nDifficulty >= ch->nDifficulty && race_lookup("bio-android") == ch->race) {
                act ("{GDelivering the final blow to $N, $n steps back and absorbs $m!{x", ch, NULL, victim, TO_NOTVICT);
                act ("{GDelivering the final blow to $N, you step back and absorb $m!{x", ch, NULL, victim, TO_CHAR);
				act ("{GDelivering the final blow to you, $n steps back and absorbs you!{x", ch, NULL, victim, TO_VICT);
                inc_bio_evolve (ch);
            }
            else {
			    act ("{R$n is DEAD!!{x", victim, NULL, NULL, TO_ROOM);
                sendch ("{RYou have been KILLED!!{x\n\r\n\r", victim);
			}
            break;

        default:
            if (dam > victim->max_hit / 4)
                sendch ("{RThat really did HURT!{x\n\r", victim);
            if (victim->hit < victim->max_hit / 4)
                sendch ("{RYou sure are BLEEDING!{x\n\r", victim);
            break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if (!IS_AWAKE (victim))
        stop_fighting (victim, FALSE);

    /*
     * Payoff for killing things.
     */
    if (victim->position == POS_UNCONSCIOUS) {
        stop_fighting (victim, TRUE);

        //victim->position = POS_SLEEPING;
        victim->hit = 0;//UMAX (1, victim->hit);
        victim->ki = 0;//UMAX (1, victim->ki);
        victim->charge = 0;
        victim->wait_skill = 0;
        victim->wait_skill_sn = 0;
        victim->wait_skill_vo = NULL;
        if (victim->powerstruggle) {
            victim->powerstruggle->powerstruggle = NULL;
            victim->powerstruggle = NULL;
        }
        wait (victim, 20*PULSE_SECOND);

        return TRUE;
    }
    else if (victim->position == POS_DEAD) {
        group_gain (ch, victim);

        if (!IS_NPC (victim)) {
            logstr (LOG_GAME, "%s killed by %s at %d", victim->name, (IS_NPC (ch) ? ch->short_descr : ch->name), ch->in_room->vnum);

            /*
             * Dying penalty:
             */
        }

        if (IS_NPC(victim) && !IS_NPC(ch) && ch->pcdata->nMissionType == MISSION_SLAY && victim->pIndexData->vnum == ch->pcdata->reward_mob) {
			ch->pcdata->bMissionSuccess = TRUE;
            sendch ("You have disposed of your target! Return immediately to receive your bounty!\n\r", ch);
		}

        sprintf (log_buf, "%s got toasted by %s at %s [room %d]",
                 (IS_NPC (victim) ? victim->short_descr : victim->name),
                 (IS_NPC (ch) ? ch->short_descr : ch->name),
                 victim->in_room->name, victim->in_room->vnum);

        if (IS_NPC (victim))
            wiznet (log_buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0);
        else
            wiznet (log_buf, NULL, NULL, WIZ_DEATHS, 0, 0);

        /*
         * Death trigger
         */
        if (IS_NPC (victim) && HAS_TRIGGER_MOB (victim, TRIG_DEATH)) {
            victim->position = POS_STANDING;
            p_percent_trigger (victim, NULL, NULL, ch, NULL, NULL, TRIG_DEATH);
        }
	    if (victim->in_room && HAS_TRIGGER_ROOM (victim->in_room, TRIG_DEATH))
            p_percent_trigger (NULL, NULL, victim->in_room, victim, NULL, NULL, TRIG_DEATH);

        raw_kill (victim);

        /* dump the flags */
        if (ch != victim && !IS_NPC (ch) && !is_same_clan (ch, victim)) {
            if (IS_SET (victim->act, PLR_HOSTILE))
                REMOVE_BIT (victim->act, PLR_HOSTILE);
            else
                REMOVE_BIT (victim->act, PLR_THIEF);
        }

        /* RT new auto commands */
        if (!IS_NPC (ch)
            && (corpse = get_obj_list (ch, "corpse", ch->in_room->contents)) != NULL
            && corpse->item_type == ITEM_CORPSE_NPC
            && can_see_obj (ch, corpse))
        {
            OBJ_DATA *coins;

            corpse = get_obj_list (ch, "corpse", ch->in_room->contents);

            if (IS_SET (ch->act, PLR_AUTOLOOT) && corpse && corpse->contains)
                                /* exists and not empty */
                do_function (ch, &do_get, "all corpse");

            if (IS_SET (ch->act, PLR_AUTOZENNI) && corpse && corpse->contains &&    /* exists and not empty */
                !IS_SET (ch->act, PLR_AUTOLOOT))
                if ((coins = get_obj_list (ch, "gcash", corpse->contains)) != NULL)
                    do_function (ch, &do_get, "all.gcash corpse");

            if (IS_SET (ch->act, PLR_AUTOSAC))
            {
                if (IS_SET (ch->act, PLR_AUTOLOOT) && corpse && corpse->contains)
                    return TRUE;    /* leave if corpse has treasure */
                else
                    do_function (ch, &do_sacrifice, "corpse");
            }
        }

        return TRUE;
    }

    if (victim == ch)
        return TRUE;

    // Take care of link dead people.
    // Run their character through an AI function (heh?)
    if (!IS_NPC (victim) && victim->desc == NULL) {
        mobai_kiwarrior(victim, ch);
        return TRUE;
    }

    /*
     * Wimp out?
     */
    if (IS_NPC (victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
        if ((IS_SET (victim->act, ACT_WIMPY) && number_bits (2) == 0
             && victim->hit < victim->max_hit / 5)
            || (IS_AFFECTED (victim, AFF_CHARM) && victim->master != NULL
                && victim->master->in_room != victim->in_room)) {
            if (victim->balance < 7)
                do_function (victim, &do_retreat, "");
            else
                do_function(victim, &do_flee, "");
        }

    if (!IS_NPC (victim)
        && victim->hit > 0
        && victim->hit <= victim->wimpy && victim->wait < PULSE_VIOLENCE / 2) {
        if (victim->balance < 10)
            do_function (victim, &do_retreat, "");
        else
            do_function(victim, &do_flee, "");
    }

    tail_chain ();
    return TRUE;
}



// Damage an obj (its durability)
void damage_obj (OBJ_DATA *obj, int dam) {
	if (IS_OBJ_STAT (obj, ITEM_INDESTRUCTIBLE))
		return;

	obj->durability -= dam;

	if (obj->durability < 1) {
        if (obj->carried_by != NULL)
            act ("$p is totally destroyed!", obj->carried_by, obj, NULL, TO_ALL);
        else if (obj->in_room != NULL && obj->in_room->people != NULL)
            act ("$p is totally destroyed!", obj->in_room->people, obj, NULL, TO_ALL);

		if (obj->contains) {
			OBJ_DATA *t_obj, *n_obj;
			if (obj->carried_by != NULL)
				act ("The contents of $p spill out!", obj->carried_by, obj, NULL, TO_ALL);
			else if (obj->in_room != NULL && obj->in_room->people != NULL)
				act ("The contents of $p spill out!", obj->in_room->people, obj, NULL, TO_ALL);
			// dump contents
			for (t_obj = obj->contains; t_obj; t_obj = n_obj) {
				n_obj = t_obj->next_content;
				obj_from_obj (t_obj);
				if (obj->in_room != NULL)
					obj_to_room (t_obj, obj->in_room);
				else if (obj->carried_by != NULL)
					obj_to_room (t_obj, obj->carried_by->in_room);
				else {
					extract_obj (t_obj);
					continue;
				}
				damage_obj (t_obj, dam / 2);
			}
		}
		extract_obj (obj);
	}
}


bool is_safe (CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (victim->in_room == NULL || ch->in_room == NULL)
        return TRUE;

    if (victim->fighting == ch || victim == ch)
        return FALSE;

    if (IS_IMMORTAL (ch) && ch->level > LEVEL_IMMORTAL)
        return FALSE;

    /* killing mobiles */
    if (IS_NPC (victim))
    {

        /* safe room? */
        if (IS_SET (victim->in_room->room_flags, ROOM_SAFE))
        {
            sendch ("Not in this room.\n\r", ch);
            return TRUE;
        }

        if (victim->pIndexData->pShop != NULL)
        {
            sendch ("The shopkeeper wouldn't like that.\n\r", ch);
            return TRUE;
        }

        /* no killing healers, trainers, etc */
        if (IS_SET (victim->act, ACT_IS_HEALER)
            || IS_SET (victim->act, ACT_IS_CHANGER)
			||  IS_SET(victim->act,ACT_MISSION))
        {
            sendch ("I don't think the gods would approve.\n\r", ch);
            return TRUE;
        }

        if (!IS_NPC (ch))
        {
            /* no pets */
            if (IS_SET (victim->act, ACT_PET))
            {
                act ("But $N looks so cute and cuddly...",
                     ch, NULL, victim, TO_CHAR);
                return TRUE;
            }

            /* no charmed creatures unless owner */
            if (IS_AFFECTED (victim, AFF_CHARM) && ch != victim->master)
            {
                sendch ("You don't own that monster.\n\r", ch);
                return TRUE;
            }
        }
    }
    /* killing players */
    else
    {
        /* NPC doing the killing */
        if (IS_NPC (ch))
        {
            /* safe room check */
            if (IS_SET (victim->in_room->room_flags, ROOM_SAFE))
            {
                sendch ("Not in this room.\n\r", ch);
                return TRUE;
            }

            /* charmed mobs and pets cannot attack players while owned */
            if (IS_AFFECTED (ch, AFF_CHARM) && ch->master != NULL
                && ch->master->fighting != victim)
            {
                sendch ("Players are your friends!\n\r", ch);
                return TRUE;
            }
        }
        /* player doing the killing */
        else
        {
            if (IS_SET (victim->act, PLR_HOSTILE)
                || IS_SET (victim->act, PLR_THIEF))
                return FALSE;

            /*
			if (!is_clan (ch))
            {
                sendch ("Join a clan if you want to kill players.\n\r",
                              ch);
                return TRUE;
            }

			if (!is_clan (victim))
            {
                sendch ("They aren't in a clan, leave them alone.\n\r",
                              ch);
                return TRUE;
            }

            if (ch->level > victim->level + 8)
            {
                sendch ("Pick on someone your own size.\n\r", ch);
                return TRUE;
            }
			*/
        }
    }
    return FALSE;
}

bool is_safe_spell (CHAR_DATA * ch, CHAR_DATA * victim, bool area)
{
    if (victim->in_room == NULL || ch->in_room == NULL)
        return TRUE;

    if (victim == ch && area)
        return TRUE;

    if (victim->fighting == ch || victim == ch)
        return FALSE;

    if (IS_IMMORTAL (ch) && ch->level > LEVEL_IMMORTAL && !area)
        return FALSE;

    if (area && is_same_group (ch, victim))
        return TRUE;

    /* killing mobiles */
    if (IS_NPC (victim))
    {
        /* safe room? */
        if (IS_SET (victim->in_room->room_flags, ROOM_SAFE))
            return TRUE;

        if (victim->pIndexData->pShop != NULL)
            return TRUE;

        /* no killing healers, trainers, etc */
        if (IS_SET (victim->act, ACT_IS_HEALER)
            || IS_SET (victim->act, ACT_IS_CHANGER))
            return TRUE;

        if (!IS_NPC (ch))
        {
            /* no pets */
            if (IS_SET (victim->act, ACT_PET))
                return TRUE;

            /* no charmed creatures unless owner */
            if (IS_AFFECTED (victim, AFF_CHARM)
                && (area || ch != victim->master))
                return TRUE;

            /* legal kill? -- cannot hit mob fighting non-group member */
            if (victim->fighting != NULL
                && !is_same_group (ch, victim->fighting))
				return TRUE;
        }
        else
        {
            /* area effect spells do not hit other mobs */
            if (area && !is_same_group (victim, ch->fighting))
                return TRUE;
        }
    }
    /* killing players */
    else
    {
        if (area && IS_IMMORTAL (victim) && victim->level > LEVEL_IMMORTAL)
            return TRUE;

        /* NPC doing the killing */
        if (IS_NPC (ch))
        {
            /* charmed mobs and pets cannot attack players while owned */
            if (IS_AFFECTED (ch, AFF_CHARM) && ch->master != NULL
                && ch->master->fighting != victim)
                return TRUE;

            /* safe room? */
            if (IS_SET (victim->in_room->room_flags, ROOM_SAFE))
                return TRUE;

            /* legal kill? -- mobs only hit players grouped with opponent */
            if (ch->fighting != NULL && !is_same_group (ch->fighting, victim))
                return TRUE;
        }

        /* player doing the killing */
        else
        {
            //if (!is_clan (ch))
            //    return TRUE;

            if (IS_SET (victim->act, PLR_HOSTILE)
                || IS_SET (victim->act, PLR_THIEF))
                return FALSE;

            //if (!is_clan (victim))
            //    return TRUE;
        }

    }
    return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer (CHAR_DATA * ch, CHAR_DATA * victim)
{
    char buf[MAX_STRING_LENGTH];
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    while (IS_AFFECTED (victim, AFF_CHARM) && victim->master != NULL)
        victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if (IS_NPC (victim)
        || IS_SET (victim->act, PLR_HOSTILE)
        || IS_SET (victim->act, PLR_THIEF))
        return;

    /*
     * Charm-o-rama.
     */
    if (IS_SET (ch->affected_by, AFF_CHARM))
    {
        if (ch->master == NULL)
        {
            char buf[MAX_STRING_LENGTH];

            sprintf (buf, "Check_killer: %s bad AFF_CHARM",
                     IS_NPC (ch) ? ch->short_descr : ch->name);
            logstr (LOG_BUG, buf, 0);
            REMOVE_BIT (ch->affected_by, AFF_CHARM);
            return;
        }
/*
    sendch( "*** You are now hostile!! ***\n\r", ch->master );
      SET_BIT(ch->master->act, PLR_HOSTILE);
*/

        stop_follower (ch);
        return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if (IS_NPC (ch)
        || ch == victim || ch->level >= LEVEL_IMMORTAL //|| !is_clan (ch)
        || IS_SET (ch->act, PLR_HOSTILE) || ch->fighting == victim
        || IS_SET(ch->in_room->room_flags, ROOM_CHAOS))
        return;

    sendch ("*** You are now hostile!! ***\n\r", ch);
    SET_BIT (ch->act, PLR_HOSTILE);
    sprintf (buf, "$N is attempting to murder %s", victim->name);
    wiznet (buf, ch, NULL, WIZ_FLAGS, 0, 0);
    save_char_obj (ch);
    return;
}



// Finds a random defend skill, given the ones that are possible.
// To get a defend skill for a character responding to an attack
// that can be blocked by shields or parries:
//      get_defend_skill (ch, DEF_PARRY|DEF_DODGE);
// This function uses the following options, as defined in merc.h:
//      DEF_NONE, DEF_DODGE, DEF_PARRY, DEF_SHIELD
// If no defend skills are found, 0 is returned
int get_defend_skill (CHAR_DATA *ch, int nOptions) {
	bool bCanDodge=FALSE, bCanParry=FALSE, bCanShield=FALSE;
	int n;

	// Find possible skills, looking for weapons, shields, etc
	if (nOptions & DEF_DODGE &&
		get_skill(ch, gsn_dodge) > 0)
		bCanDodge = TRUE;
	if (nOptions & DEF_PARRY &&
		get_skill(ch, gsn_parry) > 0 &&
		get_eq_char (ch, WEAR_WIELD) != NULL)
		bCanParry = TRUE;
	if (nOptions & DEF_SHIELD &&
		get_skill(ch, gsn_shield_block) > 0 &&
		get_eq_char (ch, WEAR_SHIELD) != NULL)
		bCanShield = TRUE;

	// No skills are useable by the character
	if (!bCanDodge && !bCanParry && !bCanShield) {
		// If dodge was one of the options, just allow the character
		// to use it, to give him a chance
		if (nOptions & DEF_DODGE)
			return gsn_dodge;
		else
			return 0;
	}

	// pick a random skill
	while (TRUE) {
		n = number_range(1,3);
		switch (n) {
		case DEF_DODGE:  if (bCanDodge)  { return gsn_dodge; } break;
		case DEF_PARRY:  if (bCanParry)  { return gsn_parry; } break;
		case DEF_SHIELD: if (bCanShield) { return gsn_shield_block; } break;
		}
	}

	// shouldn't get here...
	return 0;
}


/*
 * Set position of a victim.
 */
void update_pos (CHAR_DATA * victim)
{
    if (victim->hit > 0) {
        if (victim->position <= POS_UNCONSCIOUS)
            victim->position = POS_STANDING;
        return;
    }

    if (IS_SET(victim->in_room->room_flags, ROOM_CHAOS) && !IS_NPC(victim)) {
        victim->position = POS_UNCONSCIOUS;
        return;
    }

    if (victim->hit <= -11)
        victim->position = POS_DEAD;
    else if (victim->hit <= -6)
        victim->position = POS_MORTAL;
    else if (victim->hit <= -3)
        victim->position = POS_INCAP;
    else
        victim->position = POS_STUNNED;

    return;
}



/*
 * Start fights.
 */
void set_fighting (CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (ch->fighting != NULL)
    {
        logstr (LOG_BUG, "Set_fighting: already fighting", 0);
        return;
    }

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    return;
}



/*
 * Stop fights.
 */
void stop_fighting (CHAR_DATA * ch, bool fBoth)
{
    CHAR_DATA *fch;

    for (fch = char_list; fch != NULL; fch = fch->next) {
        if (fch == ch || (fBoth &&
               (fch->fighting == ch || fch->powerstruggle == ch || (CHAR_DATA*)fch->wait_skill_vo == ch))) {
            fch->fighting = NULL;
            if (fch->powerstruggle == ch || fch == ch)
                fch->powerstruggle = NULL;
            if ((CHAR_DATA*)fch->wait_skill_vo == ch || fch == ch) {
			    fch->charge = 0;
			    fch->wait_skill_sn = 0;
			    fch->wait_skill_vo = NULL;
			    fch->wait_skill_target = TARGET_NONE;
			}
            fch->position = IS_NPC (fch) ? fch->default_pos : POS_STANDING;
            update_pos (fch);
        }
    }

    return;
}



/*
 * Make a corpse out of a character.
 */
void make_corpse (CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

    if (IS_NPC (ch)) {
        name = ch->short_descr;
        corpse = create_object (get_obj_index (OBJ_VNUM_CORPSE_NPC), 0);
        corpse->timer = number_range (3, 6);
        if (ch->zenni > 0)
        {
            obj_to_obj (create_money (ch->zenni), corpse);
            ch->zenni = 0;
        }
        corpse->cost = 0;
    }
    else {
        name = ch->name;
        corpse = create_object (get_obj_index (OBJ_VNUM_CORPSE_PC), 0);
        corpse->timer = number_range (25, 40);
        REMOVE_BIT (ch->act, PLR_CANLOOT);
        if (!is_clan (ch))
            corpse->owner = str_dup (ch->name);
        else
        {
            corpse->owner = NULL;
            if (ch->zenni > 1)
            {
                obj_to_obj (create_money (ch->zenni / 2),
                            corpse);
                ch->zenni -= ch->zenni / 2;
            }
        }

        corpse->cost = 0;
    }

    corpse->llPl = ch->nDifficulty * 10;

    sprintf (buf, corpse->short_descr, name);
    free_string (corpse->short_descr);
    corpse->short_descr = str_dup (buf);

    sprintf (buf, corpse->description, name);
    free_string (corpse->description);
    corpse->description = str_dup (buf);

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
        bool floating = FALSE;

        obj_next = obj->next_content;
        if (obj->wear_loc == WEAR_FLOAT)
            floating = TRUE;
        obj_from_char (obj);
        if (obj->item_type == ITEM_POTION)
            obj->timer = number_range (500, 1000);
        if (obj->item_type == ITEM_SCROLL)
            obj->timer = number_range (1000, 2500);
        if (IS_SET (obj->extra_flags, ITEM_ROT_DEATH) && !floating) {
            obj->timer = number_range (5, 10);
            REMOVE_BIT (obj->extra_flags, ITEM_ROT_DEATH);
        }
        REMOVE_BIT (obj->extra_flags, ITEM_VIS_DEATH);

        if (IS_SET (obj->extra_flags, ITEM_INVENTORY))
            extract_obj (obj);
        else if (floating)
        {
            if (IS_OBJ_STAT (obj, ITEM_ROT_DEATH)) {
                if (obj->contains != NULL) {
                    OBJ_DATA *in, *in_next;

                    act ("$p evaporates,scattering its contents.", ch, obj, NULL, TO_ROOM);
                    for (in = obj->contains; in != NULL; in = in_next){
                        in_next = in->next_content;
                        obj_from_obj (in);
                        obj_to_room (in, ch->in_room);
                    }
                }
                else
                    act ("$p evaporates.", ch, obj, NULL, TO_ROOM);
                extract_obj (obj);
            }
            else {
                act ("$p falls to the floor.", ch, obj, NULL, TO_ROOM);
                obj_to_room (obj, ch->in_room);
            }
        }
        else
            obj_to_obj (obj, corpse);
    }

    obj_to_room (corpse, ch->in_room);
    return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry (CHAR_DATA * ch)
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    int door;
    int vnum;

    vnum = 0;
    msg = "You hear $n's death cry.";

    switch (number_bits (4))
    {
        case 0:
            msg = "$n hits the ground ... DEAD.";
            break;
        case 1:
            if (ch->material == 0)
            {
                msg = "$n splatters blood on your armor.";
                break;
            }
        case 2:
            if (IS_SET (ch->parts, PART_GUTS))
            {
                msg = "$n spills $s guts all over the floor.";
                vnum = OBJ_VNUM_GUTS;
            }
            break;
        case 3:
            if (IS_SET (ch->parts, PART_HEAD))
            {
                msg = "$n's severed head plops on the ground.";
                vnum = OBJ_VNUM_SEVERED_HEAD;
            }
            break;
        case 4:
            if (IS_SET (ch->parts, PART_HEART))
            {
                msg = "$n's heart is torn from $s chest.";
                vnum = OBJ_VNUM_TORN_HEART;
            }
            break;
        case 5:
            if (IS_SET (ch->parts, PART_ARMS))
            {
                msg = "$n's arm is sliced from $s dead body.";
                vnum = OBJ_VNUM_SLICED_ARM;
            }
            break;
        case 6:
            if (IS_SET (ch->parts, PART_LEGS))
            {
                msg = "$n's leg is sliced from $s dead body.";
                vnum = OBJ_VNUM_SLICED_LEG;
            }
            break;
        case 7:
            if (IS_SET (ch->parts, PART_BRAINS))
            {
                msg =
                    "$n's head is shattered, and $s brains splash all over you.";
                vnum = OBJ_VNUM_BRAINS;
            }
    }

    act (msg, ch, NULL, NULL, TO_ROOM);

    if (vnum != 0)
    {
        char buf[MAX_STRING_LENGTH];
        OBJ_DATA *obj;
        char *name;

        name = IS_NPC (ch) ? ch->short_descr : ch->name;
        obj = create_object (get_obj_index (vnum), 0);
        obj->timer = number_range (4, 7);

        sprintf (buf, obj->short_descr, name);
        free_string (obj->short_descr);
        obj->short_descr = str_dup (buf);

        sprintf (buf, obj->description, name);
        free_string (obj->description);
        obj->description = str_dup (buf);

        if (obj->item_type == ITEM_FOOD)
        {
            if (IS_SET (ch->form, FORM_POISON))
                obj->value[3] = 1;
            else if (!IS_SET (ch->form, FORM_EDIBLE))
                obj->item_type = ITEM_TRASH;
        }

        obj_to_room (obj, ch->in_room);
    }

    if (IS_NPC (ch))
        msg = "You hear something's death cry.";
    else
        msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for (door = 0; door <= 5; door++)
    {
        EXIT_DATA *pexit;

        if ((pexit = was_in_room->exit[door]) != NULL
            && pexit->u1.to_room != NULL && pexit->u1.to_room != was_in_room)
        {
            ch->in_room = pexit->u1.to_room;
            act (msg, ch, NULL, NULL, TO_ROOM);
        }
    }
    ch->in_room = was_in_room;

    return;
}



void raw_kill (CHAR_DATA * victim)
{
	int i;

    stop_fighting (victim, TRUE);
    death_cry (victim);
    make_corpse (victim);

    if (IS_NPC (victim))
    {
        victim->pIndexData->killed++;
        kill_table[URANGE (0, victim->level, MAX_LEVEL - 1)].killed++;
        extract_char (victim, TRUE);
        return;
    }

    extract_char (victim, FALSE);
    while (victim->affected)
        affect_remove (victim, victim->affected);
    for (i = 0; i < 4; i++)
    	victim->armor[i]= 0;
    victim->affected_by = race_table[victim->race].aff;
    victim->position = POS_RESTING;
    victim->hit = UMAX (1, victim->hit);
    victim->ki = UMAX (1, victim->ki);
	victim->charge = 0;
	victim->wait_skill = 0;
	victim->wait_skill_sn = 0;
	victim->wait_skill_vo = NULL;
    if (victim->powerstruggle) {
        victim->powerstruggle->powerstruggle = NULL;
        victim->powerstruggle = NULL;
    }
/*  save_char_obj( victim ); we're stable enough to not need this :) */
    return;
}



void group_gain (CHAR_DATA * ch, CHAR_DATA * victim)
{
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if (victim == ch)
        return;

    /*
    members = 0;
    group_levels = 0;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (is_same_group (gch, ch))
        {
            members++;
            group_levels += IS_NPC (gch) ? gch->level / 2 : gch->level;
        }
    }
    if (members == 0)  {
        logstr (LOG_BUG, "Group_gain: members.", members);
        members = 1;
        group_levels = ch->level;
    }

    lch = (ch->leader != NULL) ? ch->leader : ch;
    */

    // Don't need to worry about changing any alignment
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_NOALIGN))
        return;

    // Check equipment / adjust alignment
    for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
        if (!is_same_group (gch, ch) || IS_NPC (gch))
            continue;

        gch->alignment -= victim->alignment / 25;
        gch->alignment = URANGE(-1000, gch->alignment, 1000);

        for (obj = ch->carrying; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            if (obj->wear_loc == WEAR_NONE)
                continue;

            if ((IS_OBJ_STAT (obj, ITEM_ANTI_EVIL) && IS_EVIL (ch))
                || (IS_OBJ_STAT (obj, ITEM_ANTI_GOOD) && IS_GOOD (ch))
                || (IS_OBJ_STAT (obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL (ch)))
            {
                act ("You are zapped by $p.", ch, obj, NULL, TO_CHAR);
                act ("$n is zapped by $p.", ch, obj, NULL, TO_ROOM);
                obj_from_char (obj);
                obj_to_room (obj, ch->in_room);
            }
        }
    }

    return;
}


struct finishMoveData {
	sh_int *pGsn;
	int nSkillLvl;
	int nTransform;
	char *szToRoom, *szToChar, *szToVict;
};

struct finishMoveData finishMoveTable[] = {
    {&gsn_energy_ball, -1, -1, "$n forms a small ball of ki and throws it at $N's head, landing a direct shot to the face!  $N's body falls in a heap to the ground, completely decapitated!",
		                       "You form a small ball of ki and throw it at $N's head, landing a direct shot to the face!  $N's body falls in a heap to the ground, completely decapitated!",
	                           "$n forms a small ball of ki and throws it at your head, landing a direct shot to your face!  You fall in a heap to the ground, completely decapitated!"},
	{&gsn_energy_ball,  4, -1, "Ki flares up around $n as $e forms a ball of energy above $s head and throws it at $N, watching it explode in a bright light!",
	                           "Ki flares up around you as you form a ball of energy above your head and throw it at $N, watching it explode in a bright light!",
	                           "Ki flares up around $n as $e forms a ball of energy above $s head and throws it at you.  The ball explodes in a bright light!"},
    {&gsn_energy_ball,  6, -1, "$N wavers, about to fall, when $n creates a small, powerful ball of energy and throws it at $S chest, hollowing out where $S heart used to be!",
	                           "$N wavers, about to fall, when you create a small, powerful ball of energy and throws it at $S chest, hollowing out where $S heart used to be!",
							   "You waver, about to fall, when $n creates a small, powerful ball of energy and throws it at your chest, hollowing out where your heart used to be!"},
	{&gsn_energy_ball,  8, -1, "$n charges up a good sized ball of energy and throws it at $N, taking $S entire head off just above the shoulders!",
	                           "You charge up a good sized ball of energy and throws it at $N, taking $S entire head off just above the shoulders!",
							   "$n charges up a good sized ball of energy and throws it at you, taking your entire head off just above the shoulders!"},
	{&gsn_energy_ball, 10, -1, "$N stumbles, looking at $n.  $n grins and begins to power up, cupping $s hands.  $e throws $s energyball, sinking it deep into $N's chest. Light pours out of the hole in $N's chest as $E explodes from the inside out, leaving nothing but red hot ashes falling from the sky when the light clears!",
	                           "$N stumbles, looking at you.  You grin and begin to power up, cupping you hands.  You throw your energyball, sinking it deep into $N's chest. Light pours out of the hole in $N's chest as $E explodes from the inside out, leaving nothing but red hot ashes falling from the sky when the light clears!",
							   "You stumble, looking at $n.  $n grins and begins to power up, cupping $s hands.  $e throws $s energyball, sinking it deep into your chest. Light pours out of the hole in your chest as you explode from the inside out, leaving nothing but red hot ashes falling from the sky when the light clears!"},
	{&gsn_energy_beam, -1, -1, "$n throws a small, condensed beam of energy at $N, which pierces through $S stomach, splaying the last drops of $S blood against the ground!",
	                           "You throw a small, condensed beam of energy at $N, which pierces through $S stomach, splaying the last drops of $S blood against the ground!",
							   "$n throws a small, condensed beam of energy at you, which pierces through your stomach, splaying the last drops of your blood against the ground!"},
    {&gsn_energy_beam, 10, -1, "Beams of energy arc through the air, eminating from $n's hands, cutting off $N's limbs.  A last ray sprays out, lobbing off $S head in a final, grand spectacle!",
	                           "Beams of energy arc through the air, eminating from your hands, cutting off $N's limbs.  A last ray sprays out, lobbing off $S head in a final, grand spectacle!",
							   "Beams of energy arc through the air, eminating from $n's hand, cutting off your limbs.  A last ray sprays out, lobbing off off your head in a final, grand spectacle!"},
	{NULL, -1, -1, NULL, NULL, NULL}
};


void FinishMessage (CHAR_DATA * pCh, CHAR_DATA * pVictim, long long int llDam, int nDt, bool bImmune)
{
    char szBuf1[1024], szBuf2[1024], szBuf3[1024];
    int i, nMove = -1;

	if (nDt < 0 || nDt > MAX_SKILL) {
		dam_message (pCh, pVictim, llDam, nDt, bImmune);
		return;
	}

	for (i = 0; finishMoveTable[i].pGsn; ++i) {
        // See if its the right skill
		if (*finishMoveTable[i].pGsn != nDt)
			continue;
		// Check if character is in the right transformation
		if (finishMoveTable[i].nTransform != -1 && finishMoveTable[i].nTransform != GetTrans(pCh))
			continue;
		// Check if skill is closer to the character's actual skill, but don't want to go over
		if (nMove == -1 ||
			(finishMoveTable[i].nSkillLvl > finishMoveTable[nMove].nSkillLvl &&
             finishMoveTable[i].nSkillLvl <= get_skill(pCh, nDt)))
			nMove = i;
	}
	if (nMove == -1) {
		dam_message (pCh, pVictim, llDam, nDt, bImmune);
		return;
	}

    if (llDam > 0) {
        sprintf (szBuf1, "%s (%Ld)", finishMoveTable[nMove].szToRoom, llDam);
		sprintf (szBuf2, "%s (%Ld)", finishMoveTable[nMove].szToChar, llDam);
		sprintf (szBuf3, "%s (%Ld)", finishMoveTable[nMove].szToVict, llDam);
	}
	else {
        sprintf (szBuf1, finishMoveTable[nMove].szToRoom);
		sprintf (szBuf2, finishMoveTable[nMove].szToChar);
		sprintf (szBuf3, finishMoveTable[nMove].szToVict);
    }

    if (pCh == pVictim) {
        act (szBuf1, pCh, NULL, pCh, TO_ROOM);
        act (szBuf2, pCh, NULL, pCh, TO_CHAR);
    }
    else {
        act (szBuf1, pCh, NULL, pVictim, TO_NOTVICT);
        act (szBuf2, pCh, NULL, pVictim, TO_CHAR);
        act (szBuf3, pCh, NULL, pVictim, TO_VICT);
    }
    return;
}



void dam_message (CHAR_DATA * ch, CHAR_DATA * victim, long long int dam, int dt, bool immune)
{
    char buf1[256], buf2[256], buf3[256], dam_buf[20];
    const char *vs;
    const char *vp;
    const char *attack;
    char punct;
    int dam_percent = ((100 * dam) / UMAX(1,victim->max_hit));

    if (ch == NULL || victim == NULL)
        return;

	// Just to be careful
	buf1[0] =  '\0';
	buf2[0] =  '\0';
	buf3[0] =  '\0';

    if (dam < 0) {
        vs = "are defended by";
        vp = "is defended by";
    }
    else if (dam == 0)
    {
        vs = "miss";
        vp = "misses";
    }
    else if (dam_percent <= 5)
    {
        vs = "scratch";
        vp = "scratches";
    }
    else if (dam_percent <= 10)
    {
        vs = "graze";
        vp = "grazes";
    }
    else if (dam_percent <= 15)
    {
        vs = "hit";
        vp = "hits";
    }
    else if (dam_percent <= 20)
    {
        vs = "injure";
        vp = "injures";
    }
    else if (dam_percent <= 25)
    {
        vs = "wound";
        vp = "wounds";
    }
    else if (dam_percent <= 30)
    {
        vs = "maul";
        vp = "mauls";
    }
    else if (dam_percent <= 35)
    {
        vs = "decimate";
        vp = "decimates";
    }
    else if (dam_percent <= 40)
    {
        vs = "devastate";
        vp = "devastates";
    }
    else if (dam_percent <= 45)
    {
        vs = "maim";
        vp = "maims";
    }
    else if (dam_percent <= 50)
    {
        vs = "MUTILATE";
        vp = "MUTILATES";
    }
    else if (dam_percent <= 55)
    {
        vs = "DISEMBOWEL";
        vp = "DISEMBOWELS";
    }
    else if (dam_percent <= 60)
    {
        vs = "DISMEMBER";
        vp = "DISMEMBERS";
    }
    else if (dam_percent <= 65)
    {
        vs = "MASSACRE";
        vp = "MASSACRES";
    }
    else if (dam_percent <= 70)
    {
        vs = "MANGLE";
        vp = "MANGLES";
    }
    else if (dam_percent <= 75)
    {
        vs = "*** DEMOLISH ***";
        vp = "*** DEMOLISHES ***";
    }
    else if (dam_percent <= 80)
    {
        vs = "*** DEVASTATE ***";
        vp = "*** DEVASTATES ***";
    }
    else if (dam_percent <= 85)
    {
        vs = "=== OBLITERATE ===";
        vp = "=== OBLITERATES ===";
    }
    else if (dam_percent <= 90)
    {
        vs = ">>> ANNIHILATE <<<";
        vp = ">>> ANNIHILATES <<<";
    }
    else if (dam_percent <= 95)
    {
        vs = "<<< ERADICATE >>>";
        vp = "<<< ERADICATES >>>";
    }
    else
    {
        vs = "do UNSPEAKABLE things to";
        vp = "does UNSPEAKABLE things to";
    }

    punct = (dam_percent <= 45) ? '.' : '!';

    /*
	if (dt == TYPE_HIT) {
		if (ch == victim)
		{
			sprintf (buf1, "{3$n's punch %s $melf", vp);
			sprintf (buf2, "{2Your punch %s yourself", vp);
		}
		else
		{
			sprintf (buf1, "{3$n's punch %s $N", vp);
			sprintf (buf2, "{2Your punch %s $N", vp);
			sprintf (buf3, "{4$n's punch %s you", vp);
		}
    }
    else
    {*/
        if (dt == TYPE_HIT)
			attack = "punch";
		if (dt >= 0 && dt < MAX_SKILL)
            attack = skill_table[dt].noun_damage;
        else if (dt >= TYPE_HIT && dt < TYPE_HIT + MAX_DAMAGE_MESSAGE)
            attack = attack_table[dt - TYPE_HIT].noun;
        else
        {
            logstr (LOG_BUG, "Dam_message: bad dt %d.", dt);
            dt = TYPE_HIT;
            attack = attack_table[0].name;
        }

        if (immune)
        {
            if (ch == victim)
            {
                sprintf (buf1, "{3$n is unaffected by $s own %s%c{x", attack, punct);
                sprintf (buf2, "{2Luckily, you are immune to that%c{x", punct);
				punct = '.';
            }
            else
            {
                sprintf (buf1, "{3$N is unaffected by $n's %s%c{x", attack, punct);
                sprintf (buf2, "{2$N is unaffected by your %s%c{x", attack, punct);
                sprintf (buf3, "{4$n's %s is powerless against you%c{x", attack, punct);
				punct = '!';
            }
        }
        else
        {
            if (ch == victim)
            {
                sprintf (buf1, "{3$n's %s %s $m%c{x", attack, vp, punct);
                sprintf (buf2, "{2Your %s %s you%c{x", attack, vp, punct);
            }
            else
            {
				sprintf (buf1, "{3$n's %s %s $N%c{x", attack, vp, punct);
                sprintf (buf2, "{2Your %s %s $N%c{x", attack, vp, punct);
                sprintf (buf3, "{4$n's %s %s you%c{x", attack, vp, punct);
            }


        }
    //}

	if (dam > 0) {
        sprintf(dam_buf, " (%Ld)", dam);
		strcat (buf1, dam_buf);
		strcat (buf2, dam_buf);
		strcat (buf3, dam_buf);
	}

    if (ch == victim)
    {
        act (buf1, ch, NULL, NULL, TO_ROOM);
        act (buf2, ch, NULL, NULL, TO_CHAR);
    }
    else
    {
        act (buf1, ch, NULL, victim, TO_NOTVICT);
        act (buf2, ch, NULL, victim, TO_CHAR);
        act (buf3, ch, NULL, victim, TO_VICT);
    }
    return;
}


/*
 * Disrupts (and destroys) a character's ki attack.
 * Caller must check for sucess
 */
void disrupt (CHAR_DATA *ch) {
    if ((ch->charge > 0 || ch->wait_skill > 0) && ch->wait_skill_sn > 0) {
        act ("You lose concentration and your technique is destroyed!", ch, NULL, NULL, TO_CHAR);
        act ("$n's concentration is disrupted, and $s technique is destroyed!", ch, NULL, NULL, TO_ROOM);

	    ch->charge = 0;
	    ch->wait_skill = 0;
	    ch->wait_skill_sn = 0;
	    ch->wait_skill_vo = NULL;
	    ch->wait_skill_target = 0;
    }
    return;
}


/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm (CHAR_DATA * ch, CHAR_DATA * victim)
{
    OBJ_DATA *obj;

    if ((obj = get_eq_char (victim, WEAR_WIELD)) == NULL)
        return;

    if (IS_OBJ_STAT (obj, ITEM_NOREMOVE))
    {
        act ("{5$S weapon won't budge!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n tries to disarm you, but your weapon won't budge!{x",
             ch, NULL, victim, TO_VICT);
        act ("{5$n tries to disarm $N, but fails.{x", ch, NULL, victim,
             TO_NOTVICT);
        return;
    }

    act ("{5$n DISARMS you and sends your weapon flying!{x",
         ch, NULL, victim, TO_VICT);
    act ("{5You disarm $N!{x", ch, NULL, victim, TO_CHAR);
    act ("{5$n disarms $N!{x", ch, NULL, victim, TO_NOTVICT);

    obj_from_char (obj);
    if (IS_OBJ_STAT (obj, ITEM_NODROP) || IS_OBJ_STAT (obj, ITEM_INVENTORY))
        obj_to_char (obj, victim);
    else
    {
        obj_to_room (obj, victim->in_room);
        if (IS_NPC (victim) && victim->wait == 0 && can_see_obj (victim, obj))
            get_obj (victim, obj, NULL);
    }

    return;
}

void do_berserk (CHAR_DATA * ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill (ch, gsn_berserk)) == 0
        || (IS_NPC (ch) && !IS_SET (ch->off_flags, OFF_BERSERK)))
    {
        sendch ("You turn red in the face, but nothing happens.\n\r",
                      ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_BERSERK) || is_affected (ch, gsn_berserk)
        || is_affected (ch, skill_lookup ("frenzy")))
    {
        sendch ("You get a little madder.\n\r", ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CALM))
    {
        sendch ("You're feeling too mellow to berserk.\n\r", ch);
        return;
    }

    if (ch->ki < 50)
    {
        sendch ("You can't get up enough energy.\n\r", ch);
        return;
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING)
        chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit / ch->max_hit;
    chance += 25 - hp_percent / 2;

    if (number_percent () < chance)
    {
        AFFECT_DATA af;

        wait (ch, PULSE_VIOLENCE);
        ch->ki -= 50;

        /* heal a little damage */
        ch->hit += ch->level * 2;
        ch->hit = UMIN (ch->hit, ch->max_hit);

        sendch ("Your pulse races as you are consumed by rage!\n\r",
                      ch);
        act ("$n gets a wild look in $s eyes.", ch, NULL, NULL, TO_ROOM);


        af.where = TO_AFFECTS;
        af.type = gsn_berserk;
        af.skill_lvl = get_skill(ch, gsn_berserk);
        af.duration = number_fuzzy (get_skill(ch,gsn_berserk));
        af.modifier = UMAX (1, 2 * get_skill(ch,gsn_berserk));
        af.bitvector = AFF_BERSERK;

        af.location = APPLY_HITROLL;
        affect_to_char (ch, &af);

        af.location = APPLY_DAMROLL;
        affect_to_char (ch, &af);

        af.modifier = UMAX (10, 10 * get_skill(ch,gsn_berserk));
        af.location = APPLY_AC;
        affect_to_char (ch, &af);
    }

    else
    {
        wait (ch, 3 * PULSE_VIOLENCE);
        ch->ki -= 25;

        sendch ("Your pulse speeds up, but nothing happens.\n\r", ch);
    }
}


void do_flee (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim;
    int attempt;

    if ((victim = ch->fighting) == NULL)
    {
        if (ch->position == POS_FIGHTING)
            ch->position = POS_STANDING;
        sendch ("You aren't fighting anyone.\n\r", ch);
        return;
    }
    if (ch->balance < 7) {
        sendch ("You are too off balance to run away.\n\r", ch);
        return;
    }

    was_in = ch->in_room;
    for (attempt = 0; attempt < 6; attempt++)
    {
        EXIT_DATA *pexit;
        int door;

        door = number_door ();
        if ((pexit = was_in->exit[door]) == 0
            || pexit->u1.to_room == NULL
            || IS_SET (pexit->exit_info, EX_CLOSED)
            || number_range (0, ch->daze) != 0 || (IS_NPC (ch)
                                                   && IS_SET (pexit->u1.
                                                              to_room->
                                                              room_flags,
                                                              ROOM_NO_MOB)))
            continue;

        move_char (ch, door, FALSE);
        if ((now_in = ch->in_room) == was_in)
            continue;

        ch->in_room = was_in;
        act ("$n has fled!", ch, NULL, NULL, TO_ROOM);
        ch->in_room = now_in;
        sendch ("You flee from combat!\n\r", ch);

        stop_fighting (ch, TRUE);
        return;
    }

    sendch ("PANIC! You couldn't escape!\n\r", ch);
    return;
}



void do_rescue (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *fch;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Rescue whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        sendch ("What about fleeing instead?\n\r", ch);
        return;
    }

    if (!IS_NPC (ch) && IS_NPC (victim))
    {
        sendch ("Doesn't need your help!\n\r", ch);
        return;
    }

    if (ch->fighting == victim)
    {
        sendch ("Too late.\n\r", ch);
        return;
    }

    if ((fch = victim->fighting) == NULL)
    {
        sendch ("That person is not fighting right now.\n\r", ch);
        return;
    }

    if (IS_NPC (fch) && !is_same_group (ch, victim))
    {
        sendch ("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    wait (ch, UMAX(0,skill_table[gsn_rescue].wait - get_skill(ch,gsn_rescue)/2));
    if (number_percent () > get_skill (ch, gsn_rescue))
    {
        sendch ("You fail the rescue.\n\r", ch);
        return;
    }

    act ("{5You rescue $N!{x", ch, NULL, victim, TO_CHAR);
    act ("{5$n rescues you!{x", ch, NULL, victim, TO_VICT);
    act ("{5$n rescues $N!{x", ch, NULL, victim, TO_NOTVICT);

    stop_fighting (fch, FALSE);
    stop_fighting (victim, FALSE);

    check_killer (ch, fch);
    set_fighting (ch, fch);
    set_fighting (fch, ch);
    return;
}


void do_disarm (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;

    hth = 0;

    if ((chance = get_skill (ch, gsn_disarm)) == 0)
    {
        sendch ("You don't know how to disarm opponents.\n\r", ch);
        return;
    }

    if (get_eq_char (ch, WEAR_WIELD) == NULL
        && ((hth = get_skill (ch, gsn_hand_to_hand)) == 0
            || (IS_NPC (ch) && !IS_SET (ch->off_flags, OFF_DISARM))))
    {
        sendch ("You must wield a weapon to disarm.\n\r", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
        sendch ("You aren't fighting anyone.\n\r", ch);
        return;
    }

    if ((obj = get_eq_char (victim, WEAR_WIELD)) == NULL)
    {
        sendch ("Your opponent is not wielding a weapon.\n\r", ch);
        return;
    }

    /* find weapon skills */
    ch_weapon = get_skill(ch, get_weapon_sn(ch));
    vict_weapon = get_skill(victim, get_weapon_sn(victim));
    ch_vict_weapon = get_skill(ch, get_weapon_sn(victim));

    /* modifiers */

    /* skill */
    if (get_eq_char (ch, WEAR_WIELD) == NULL)
        chance = chance * hth / 150;
    else
        chance = chance * ch_weapon / 100;

    chance += (ch_vict_weapon / 2 - vict_weapon) / 2;

    /* dex vs. strength */
    chance += get_curr_stat (ch, STAT_DEX);
    chance -= 2 * get_curr_stat (victim, STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* and now the attack */
    if (number_percent () < chance)
    {
        wait (ch, UMAX(0,skill_table[gsn_disarm].wait - get_skill(ch,gsn_disarm)/2));
        disarm (ch, victim);

    }
    else
    {
        wait (ch, UMAX(0,skill_table[gsn_disarm].wait - get_skill(ch,gsn_disarm)/2));
        act ("{5You fail to disarm $N.{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n tries to disarm you, but fails.{x", ch, NULL, victim,
             TO_VICT);
        act ("{5$n tries to disarm $N, but fails.{x", ch, NULL, victim,
             TO_NOTVICT);

    }
    check_killer (ch, victim);
    return;
}

void do_surrender (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *mob;
    if ((mob = ch->fighting) == NULL)
    {
        sendch ("But you're not fighting!\n\r", ch);
        return;
    }
    act ("You surrender to $N!", ch, NULL, mob, TO_CHAR);
    act ("$n surrenders to you!", ch, NULL, mob, TO_VICT);
    act ("$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT);
    stop_fighting (ch, TRUE);

    if (!IS_NPC (ch) && IS_NPC (mob)
        && (!HAS_TRIGGER_MOB (mob, TRIG_SURR)
            || !p_percent_trigger (mob, NULL, NULL, ch, NULL, NULL, TRIG_SURR)))
    {
        act ("$N seems to ignore your cowardly act!", ch, NULL, mob, TO_CHAR);
        begin_combat (mob, ch);
    }
}

void do_sla (CHAR_DATA * ch, char *argument)
{
    sendch ("If you want to SLAY, spell it out.\n\r", ch);
    return;
}



void do_slay (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Slay whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (ch == victim)
    {
        sendch ("Suicide is a mortal sin.\n\r", ch);
        return;
    }

    if (!IS_NPC (victim) && victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    act ("{1You slay $M in cold blood!{x", ch, NULL, victim, TO_CHAR);
    act ("{1$n slays you in cold blood!{x", ch, NULL, victim, TO_VICT);
    act ("{1$n slays $N in cold blood!{x", ch, NULL, victim, TO_NOTVICT);
    raw_kill (victim);
    return;
}


// ************************
// GENERAL ATTACKS/COMMANDS
// ************************
// Function to make a general attack with your weapon (whatever is in right hand, or bare hands)
void do_attack (CHAR_DATA * ch, char *argument)
{
	CHAR_DATA *victim;
    OBJ_DATA *wield;
    //bool result;
    int sn = -1, vi_sn, skill, nNumAttacks, i;
 	int dam, dam_type, dt;
    float wait_mod = 1, dam_mod = 1; // modify based on type of attack selected
    int ki_mod = 0, learn_mod = 1;
    char target_arg[MAX_INPUT_LENGTH];
	char power_arg[MAX_INPUT_LENGTH];
    int nLowDam, nHighDam;

	argument = one_argument (argument, power_arg);
	one_argument (argument, target_arg);

	if (!strcmp(power_arg,"light")) {
	    wait_mod =.5; // ImproveStat/Skill function cant handle floats
		dam_mod = .5;
		ki_mod = 1;
        learn_mod = 2;
	}
	//else if (!strcmp(power_arg,"medium"))
        // Everything already initialised when variables declared
	else if (!strcmp(power_arg,"heavy")) {
		wait_mod = 2;
		dam_mod = 2;
		ki_mod = -1;
        learn_mod = -2;
	}
	else if (target_arg[0] == '\0')
		strcpy(target_arg, power_arg);

	// Check the legality of the attack:
    if (target_arg[0] == '\0') {
        victim = ch->fighting;
        if (victim == NULL) {
            sendch ("But you aren't in combat!\n\r", ch);
            return;
        }
    }
    else if ((victim = get_char_room (ch, NULL, target_arg)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (!legal_attack(ch, victim))
        return;

	// Tired?
	if (IS_EXHAUSTED(ch)) {
		sendch("You're too exhausted.\n\r", ch);
		return;
	}

	if (ch->wait > 0 || ch->wait_skill > 0)
		return;

    // Can't beat a dead char!
    // Guard against weird room-leavings.
    if (victim->position == POS_DEAD || victim->position == POS_UNCONSCIOUS || ch->in_room != victim->in_room)
        return;

    // Figure out the type of damage message.
    wield = get_eq_char (ch, WEAR_WIELD);
    if (wield == NULL) {
        nLowDam = 1;
        nHighDam = 6;
    }
    else {
        nLowDam = wield->value[1];
        nHighDam = wield->value[2];
    }

    dt = TYPE_HIT;
    if (wield != NULL && wield->item_type == ITEM_WEAPON)
        dt += wield->value[3];
    else
        dt += ch->dam_type;

    if (dt < TYPE_HIT)
        if (wield != NULL)
            dam_type = attack_table[wield->value[3]].damage;
        else
            dam_type = attack_table[ch->dam_type].damage;
    else
        dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
        dam_type = DAM_BASH;

    // Get needed skills
    sn = get_weapon_sn (ch);
    skill = get_skill(ch, sn);

    // Results of the attack:
    ki_loss(ch, skill_table[sn].ki_mod + ki_mod);
	wait (ch, skill_table[sn].wait * wait_mod);
    check_killer (ch, victim);
    ImproveSkill (ch, sn, TRUE, learn_mod, victim->nDifficulty);
    ImproveStat (ch, STAT_STR, TRUE, learn_mod, victim->nDifficulty);
    // Get the victim's defensive skill
    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_PARRY|DEF_SHIELD);

	// Check for additional attacks.
    // 5% (hth), 2% (weapons) chance for each skill level to have a second attack.
    // Third attacks start at skill 30, (60 for weapons).
    // Fourth at 60, (90 for weapons)
    /*nNumAttacks = 1;
    if (number_range(1,100) < (skill - (sn == gsn_hand_to_hand ? 50 : 100)) * (sn == gsn_hand_to_hand ? 3 : 1))
        ++nNumAttacks;
    if (number_range(1,100) < (skill - (sn == gsn_hand_to_hand ? 100 : 200)) * (sn == gsn_hand_to_hand ? 3 : 1))
        ++nNumAttacks;
    if (number_range(1,100) < (skill - (sn == gsn_hand_to_hand ? 150 : 300)) * (sn == gsn_hand_to_hand ? 3 : 1))
        ++nNumAttacks;
        */
    nNumAttacks = URANGE(1, skill / 50, 4);

    for (i = 0; i < nNumAttacks; ++i) {
        // Check for a hit
        if (!check_hit(ch, victim, sn, vi_sn, 1, i == 0 ? TRUE : FALSE)) {
            // Miss.
            damage (ch, victim, 0, dt, dam_type, TRUE);
            if (number_range(1,2) == 1)
                set_balance(ch, -1, TRUE);
            continue;
        }

        // Hit.
        // Calculate damage:
        dam = Damroll(ch, nLowDam, nHighDam, sn, TRUE) - Absorb(victim, gsn_defend, dt, TRUE);
        dam *= dam_mod;
        damage (ch, victim, dam, dt, dam_type, TRUE);

        if (number_range(1,2) == 1)
            set_balance(victim, -1, TRUE);
    }

	return;
}

// Powerstruggle to beat another person charging
void do_powerstruggle (CHAR_DATA *ch, char *argument) {
    CHAR_DATA *victim;
    char target_arg[MAX_INPUT_LENGTH];
    char skill_arg[MAX_INPUT_LENGTH];
    char name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int sn;

    argument = one_argument (argument, target_arg);
    argument = one_argument (argument, skill_arg);

    // Check the legality of the attack:
    if (target_arg[0] == '\0') {
        victim = ch->fighting;
        if (victim == NULL) {
            sendch ("But you aren't in combat!\n\r", ch);
            return;
        }
    }
    else if ((victim = get_char_room (ch, NULL, target_arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        sendch ("Very funny.\n\r", ch);
        return;
    }

    if (is_safe (ch, victim))
        return;

    if (IS_NPC (victim) &&
        victim->fighting != NULL && !is_same_group (ch, victim->fighting)) {
        sendch ("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM) && ch->master == victim) {
        act ("But $N is such a good friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

	if (IS_EXHAUSTED(ch)) {
		sendch("You're too exhausted.\n\r", ch);
		return;
    }

    if (victim->charge < 1) {
        act ("$N is not charging an attack.", ch, NULL, victim, TO_CHAR);
        return;
    }

    sn = victim->wait_skill_sn; // Just to make it easier to type
    if ( !(sn == gsn_kamehameha  || sn == gsn_energy_beam || sn == gsn_finalflash || sn == gsn_specialbeam ||
           sn == gsn_masenko     || sn == gsn_eyebeam     || sn == gsn_mouthbeam  || sn == gsn_fingerbeam  ||
           sn == gsn_spirit_bomb || sn == gsn_power_bomb  || sn == gsn_death_ball) ) {
        sendch("You cannot struggle against that type of attack.\n\r", ch);
        return;
    }

    sn = skill_lookup(skill_arg);
    if (sn == -1 ||
        !(sn == gsn_kamehameha  || sn == gsn_energy_beam || sn == gsn_finalflash || sn == gsn_specialbeam ||
          sn == gsn_masenko     || sn == gsn_eyebeam     || sn == gsn_mouthbeam  || sn == gsn_fingerbeam  ||
          sn == gsn_spirit_bomb || sn == gsn_power_bomb  || sn == gsn_death_ball) )
        sn = victim->wait_skill_sn;

    sprintf (buf, "You are using %s to counter.\n\r", skill_table[sn].name);
    sendch (buf, ch);

    if (get_skill(ch, sn) == 0) {
        sendch ("You do not know the needed skill.\n\r", ch);
        return;
    }

    if (ch->powerstruggle != NULL) {
        sendch ("You are already engaged in a powerstruggle.\n\r", ch);
        return;
    }

    if (victim->powerstruggle != NULL) {
        sendch ("Your target is already engaged in a powerstruggle.\n\r", ch);
        return;
    }

    one_argument(victim->name, name); // Pick off the first name
    if (!skill_driver(ch, name, sn)) {
        sendch ("You failed in the use of your skill!\n\r", ch);
        return;
    }

    ch->powerstruggle = victim;
    victim->powerstruggle = ch;

    act ("{BYou enter into a powerstruggle with $N!{x", ch, NULL, victim, TO_CHAR);
    act ("{B$n enters into a powerstruggle with you!{x", ch, NULL, victim, TO_VICT);
    act ("{B$n enters into a powerstruggle with $N!{x", ch, NULL, victim, TO_NOTVICT);

    wait (ch, 3 * PULSE_SECOND);

    if (9*(ch->charge)/10 > victim->charge) {
        sendch ("{BYou are winning!{x\n\r", ch);
        sendch ("{BYou are losing!{x\n\r", victim);
    }
    else if (9*(victim->charge)/10 > ch->charge) {
        sendch ("{BYou are winning!{x\n\r", victim);
        sendch ("{BYou are losing!{x\n\r", ch);
    }

    return;
}


// Add more ki when charging in a powerstruggle
void do_addki (CHAR_DATA *ch, char *argument) {
    int i, ki;

    // This gets bypassed in interpret (needs to be able to be called while charging/waiting).
    // So, this part was just pulled from it
    if (ch->wait > 0 || (ch->wait_skill_sn > 0 && (ch->wait_skill > 0 || ch->charge > 0)) ) {
    	sendch ("You're busy.  Wait a bit longer.\n\r", ch);
		return;
	}

    if (ch->powerstruggle == NULL) {
        sendch ("You aren't in a powerstruggle!\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
        ki = 1;
    else
        ki =  strtol(argument, NULL, 10);

    if (ki < 1 || ki > 50) {
        sendch ("Use a number greater than 0, less than 51.\n\r", ch);
        return;
    }

    act ("{BYou throw more of your energy into the struggle!{x", ch, NULL, NULL, TO_CHAR);
    act ("{B$n throws more of $s energy into the struggle!{x", ch, NULL, NULL, TO_ROOM);

    for (i=0; i<ki && ch->nCurPl > 0; ++i) {
        ki_loss (ch, skill_table[ch->wait_skill_sn].ki_mod);
        ++ch->charge;
    }

    wait (ch, 3 * PULSE_SECOND);

	/*
	// A little graphical display type thing
	int nTotalCharge;
	int nCh1, nCh2;
	char szBar[46];
	nTotalCharge = ch->powerstruggle->charge + ch->charge;
    nCh1 = 40 * ch->charge / nTotalCharge;
	//nCh2 = 40 * ch->powerstruggle->charge / nTotalCharge;
    strcpy (szBar, "{B========================================\n\r");
    szBar[nCh1] = '>';
	szBar[nCh1+1] = '{';
	szBar[nCh1+2] = 'R';
	sendch (ch, szBar);
    */

    if (9*(ch->charge)/10 > ch->powerstruggle->charge) {
		sendch ("{BYou are winning!{x\n\r", ch);
        sendch ("{BYou are losing!{x\n\r", ch->powerstruggle);
    }
    else if (9*(ch->powerstruggle->charge)/10 > ch->charge) {
        sendch ("{BYou are winning!{x\n\r", ch->powerstruggle);
        sendch ("{BYou are losing!{x\n\r", ch);
    }

    return;
}


void do_defend (CHAR_DATA * ch, char *argument) {
    if (ch->fighting == NULL) {
        sendch ("But you aren't in combat!\n\r", ch);
		return;
    }

	// Tired?
	if (IS_EXHAUSTED(ch)) {
		sendch("You're too exhausted.\n\r", ch);
		return;
	}

    act("{5$n drops into a defensive state.{x",ch,NULL,NULL,TO_ROOM);
	act("{5You drop into a defensive state.{x",ch,NULL,NULL,TO_CHAR);

	wait(ch, 4*PULSE_SECOND);

    set_balance(ch, 2, TRUE);

	return;
}


void do_retreat (CHAR_DATA * ch, char *argument) {
    if (ch->fighting == NULL) {
        sendch ("But you aren't in combat!\n\r", ch);
		return;
    }

    // Tired?
	if (IS_EXHAUSTED(ch)) {
		sendch("You're too exhausted.\n\r", ch);
		return;
	}

	act("{5$n retreats from combat!{x",ch,NULL,NULL,TO_ROOM);
	act("{5You retreat from combat!{x",ch,NULL,NULL,TO_CHAR);

	wait(ch, 7*PULSE_SECOND);

    set_balance(ch, 5, TRUE);

	return;
}

// **************
// COMBAT SKILLS:
// **************

void skill_kick (CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int vi_sn;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_kick, TRUE, 1, victim->nDifficulty);

    if (check_hit(ch,victim,gsn_kick,vi_sn,1, TRUE)) {
		dam = Damroll(ch, 1, 6, gsn_kick, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE);
		// Check for an excellent kick
		if (number_percent() < 10 && get_skill(ch,gsn_kick) >= 125) {
			dam *= 5;
			act("{5WHAM! $n unleashes an incredible strike with $s legs!{X",ch,NULL,NULL,TO_ROOM);
			act("{5WHAM! You unleash an incredible strike with your legs!{x",ch,NULL,NULL,TO_CHAR);
		}
		else if (number_percent() < 10 && get_skill(ch,gsn_kick) >= 50) {
			dam *= 3;
			act("{5$n spins around, completing an outstanding roundhouse!{x",ch,NULL,NULL,TO_ROOM);
			act("{5You spin around, completing an outstanding roundhouse!{x",ch,NULL,NULL,TO_CHAR);
		}
		else if (number_percent() < 10 && get_skill(ch, gsn_kick) >= 10) {
			dam *= 2;
			act("{5$n makes an excellent connection.{x",ch,NULL,NULL,TO_ROOM);
			act("{5You make an excellent connection.{x",ch,NULL,NULL,TO_CHAR);
		}
        damage (ch, victim, dam, gsn_kick, DAM_BASH, TRUE);

		if (number_range(1,2) == 1)
			set_balance(victim, -1, TRUE);
    }
    else {
        damage (ch, victim, 0, gsn_kick, DAM_BASH, TRUE);
		if (number_range(1,2) == 1)
			set_balance(ch, -1, TRUE);
    }
    return;
}

void skill_bash (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
    int modifier, vi_sn, dam;

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_kick, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_bash, TRUE, 1, victim->nDifficulty);

    /* Size */
	modifier = (victim->size+1) / (ch->size+1);
    /* Speed */
    if (IS_SET (ch->off_flags, OFF_FAST) || IS_AFFECTED (ch, AFF_HASTE))
        modifier /= 2;
    if (IS_SET (victim->off_flags, OFF_FAST) || IS_AFFECTED (victim, AFF_HASTE))
        modifier *= 2;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_PARRY|DEF_SHIELD);
	if (check_hit(ch,victim,gsn_kick,vi_sn,modifier, TRUE)) {
        dam = Damroll(ch, 1, 10, gsn_kick, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE);

		act ("{5$n sends you sprawling with a powerful bash!{x", ch, NULL, victim, TO_VICT);
        act ("{5You slam into $N, and send $M flying!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n sends $N sprawling with a powerful bash.{x", ch, NULL, victim, TO_NOTVICT);

		wait (victim, 8 * PULSE_SECOND);
        victim->position = POS_RESTING;
        damage (ch, victim, dam, gsn_bash, DAM_BASH, TRUE);
		set_balance(victim, -2, TRUE);
    }
    else {
        damage (ch, victim, 0, gsn_bash, DAM_BASH, TRUE);
        act ("{5You fall flat on your face!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n falls flat on $s face.{x", ch, NULL, victim, TO_NOTVICT);
        act ("{5You evade $n's bash, causing $m to fall flat on $s face.{x", ch, NULL, victim, TO_VICT);
        ch->position = POS_RESTING;
		set_balance(ch, -2, TRUE);
    }
}

void skill_sweep (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;

	/* the attack */
    if (IS_AFFECTED(victim, AFF_FLYING)) {
        act ("{5$n tries sweep your feet, but can't since you're flying!{x", ch, NULL, victim, TO_VICT);
        act ("{5You try to sweep out the feet of $N, but can't since $E's flying!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n tries sweep $N's feet, but can't since $E's flying!{x", ch, NULL, victim, TO_NOTVICT);
		set_balance(ch, -1, TRUE);
		return;
	}

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_kick, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_sweep, TRUE, 1, victim->nDifficulty);

	if (check_hit(ch,victim,gsn_kick,gsn_dodge,victim->size / UMIN(1, ch->size), TRUE)) { // multiplier is the relative sizes of the combatants
        act ("{5$n sweeps your feet!{x", ch, NULL, victim, TO_VICT);
        act ("{5You sweep out the feet of $N!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n sweeps $N's feet, sending $M to the ground.{x", ch, NULL, victim, TO_NOTVICT);
        wait (victim, 6 * PULSE_SECOND);
        victim->position = POS_RESTING;
        dam = Damroll(ch, 1, 4, gsn_kick, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE);
		damage (ch, victim, dam, gsn_sweep, DAM_BASH, TRUE);
		set_balance(victim, -3, TRUE);
		set_balance(ch, -1, TRUE);
    }
    else {
        damage (ch, victim, 0, gsn_sweep, DAM_BASH, TRUE);
		set_balance(ch, -1, TRUE);
    }
}


void skill_throat_shot (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int vi_sn;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_PARRY|DEF_SHIELD);

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_hand_to_hand, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_throat_shot, TRUE, 1, victim->nDifficulty);

    /* the attack */
    if (check_hit(ch,victim,gsn_hand_to_hand,vi_sn,1, TRUE)) {
        act ("{5$n quickly jabs you in the throat, making you gasp for air!{x", ch, NULL, victim, TO_VICT);
        act ("{5You jab $N in the throat!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n jabs $N in the throat, making him gasp for air.{x", ch, NULL, victim, TO_NOTVICT);
        wait (victim, 3 * PULSE_SECOND);
    }
    else
        damage (ch, victim, 0, gsn_throat_shot, DAM_BASH, TRUE);
}


void skill_knee (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int vi_sn, dam;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_kick, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_knee, TRUE, 1, victim->nDifficulty);

    if (check_hit(ch,victim,gsn_kick,vi_sn,1, TRUE)) {
        act ("{5$n knees you in the stomach!{x", ch, NULL, victim, TO_VICT);
        act ("{5You knee $N in the stomach!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n knees $N in the stomach.{x", ch, NULL, victim, TO_NOTVICT);
		wait (victim, 2*PULSE_SECOND);
        dam = Damroll(ch, 1, 6, gsn_kick, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE);
		damage (ch, victim, dam, gsn_knee, DAM_BASH, TRUE);
    }
    else
        damage (ch, victim, 0, gsn_knee, DAM_BASH, TRUE);
    return;
}

void skill_elbow (CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int vi_sn, dam;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD|DEF_PARRY);

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_hand_to_hand, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_elbow, TRUE, 1, victim->nDifficulty);

    if (check_hit(ch,victim,gsn_hand_to_hand,vi_sn,1, TRUE)) {
        act ("{5$n smashes you with an elbow in the face!{x", ch, NULL, victim, TO_VICT);
        act ("{5You elbow $N in the face!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n elbows $N in the face.{x", ch, NULL, victim, TO_NOTVICT);
        dam = Damroll(ch, 1, 4, gsn_hand_to_hand, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE);
	    damage (ch, victim, dam, gsn_elbow, DAM_BASH, TRUE);
    	if (number_range(1,100) > UMIN(75, get_skill(ch, gsn_elbow) + 9))
            disrupt (victim);
    }
    else
        damage (ch, victim, 0, gsn_elbow, DAM_BASH, TRUE);
    return;
}


void skill_eye_gouge (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	AFFECT_DATA af;
	int vi_sn, dam;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_hand_to_hand, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_eye_gouge, TRUE, 1, victim->nDifficulty);

    if (check_hit(ch,victim,gsn_hand_to_hand,vi_sn,1, TRUE)) {
        if ((dam = Damroll(ch, 1, 2, gsn_hand_to_hand, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE)) > 0) {
            act ("{5$n sends a finger into your eye!{x", ch, NULL, victim, TO_VICT);
            act ("{5You poke $N right in the eye!{x", ch, NULL, victim, TO_CHAR);
            act ("{5$n sends a finger right into $N's eye.{x", ch, NULL, victim, TO_NOTVICT);

            af.where = TO_AFFECTS;
            af.type = gsn_eye_gouge;
            af.skill_lvl = get_skill(ch, gsn_eye_gouge);
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.duration = 1;
            af.bitvector = AFF_BLIND;
            affect_to_char (victim, &af);
        }
		damage (ch, victim, dam, gsn_eye_gouge, DAM_BASH, TRUE);
    }
    else
        damage (ch, victim, 0, gsn_eye_gouge, DAM_BASH, TRUE);
    return;
}

void skill_heart_shot (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int vi_sn, dam;

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	ImproveStat (ch, STAT_STR, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_hand_to_hand, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_heart_shot, TRUE, 1, victim->nDifficulty);

    if (check_hit(ch,victim,gsn_hand_to_hand,vi_sn,1, TRUE)) {
		act ("{5$n lands an open palm onto your heart!{x", ch, NULL, victim, TO_VICT);
        act ("{5You smack $N in the heart with your fist!{x", ch, NULL, victim, TO_CHAR);
        act ("{5$n lands an open palm onto $N's heart.{x", ch, NULL, victim, TO_NOTVICT);
        dam = Damroll(ch, 2, 20, gsn_hand_to_hand, TRUE) - Absorb(victim, gsn_defend, DAM_BASH, TRUE);
		damage (ch, victim, dam, gsn_heart_shot, DAM_BASH, TRUE);
        set_balance(victim, -1, TRUE);
    }
    else {
        damage (ch, victim, 0, gsn_heart_shot, DAM_BASH, TRUE);
		if (number_range(1,2) == 1)
			set_balance(ch, -1, TRUE);
    }
    return;
}


void skill_hyperpunch (CHAR_DATA *ch, void *vo, int target) {
 	CHAR_DATA *vch, *vch_next;
    int dam, dam_type, dt;
    int nDiffCount = 0;
    int nNumAttacks, skill, i;
    int n = 0;

    dt = TYPE_HIT + ch->dam_type;
    dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
        dam_type = DAM_BASH;

    skill = get_skill(ch, gsn_hyperpunch);

   	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
        vch_next = vch->next_in_room;

        if (is_safe_spell (ch, vch, TRUE) || // is_safe_spell is the best one to use
            (IS_NPC(vch) && IS_NPC(ch) &&
             (ch->fighting != vch || vch->fighting != ch)))
            continue;

        /*
        nNumAttacks = 1;
        if (number_range(1,100) < skill)
            ++nNumAttacks;
        if (number_range(1,100) < (skill - 60))
            ++nNumAttacks;
        if (number_range(1,100) < (skill - 120))
            ++nNumAttacks;
            */
        nNumAttacks = URANGE(1, skill / 50, 4);

        for (i = 0; i < nNumAttacks; ++i) {
            if (!check_hit(ch, vch, gsn_hand_to_hand, get_defend_skill(vch, DEF_DODGE|DEF_PARRY|DEF_SHIELD), 1, i == 0 ? TRUE : FALSE)) {
                damage (ch, vch, 0, dt, dam_type, TRUE);
                if (number_range(1,2) == 1)
                    set_balance(ch, -1, TRUE);
                continue;
            }

            dam = Damroll(ch, 1, 6, gsn_hand_to_hand, TRUE) - Absorb(vch, gsn_defend, dt, TRUE);
            damage (ch, vch, dam, dt, dam_type, TRUE);
            if (number_range(1,2) == 1)
                set_balance(vch, -1, TRUE);
        }

        check_killer (ch, vch);
        nDiffCount += vch->nDifficulty;
        ++n;
    }
    if (n > 0) {
        nDiffCount /= n;
        ImproveStat (ch, STAT_STR, TRUE, 1, nDiffCount);
        ImproveSkill (ch, gsn_hand_to_hand, TRUE, 1, nDiffCount);
        ImproveSkill (ch, gsn_hyperpunch, TRUE, 1, nDiffCount);
    }
	return;
}

// ***********
// KI ATTACKS:
// ***********

// Takes the amount a character has charged and the skill,
// and converts that to a modifier for ImproveStat/Skill.
// ie something like -5, -2, 1, 6
int ChargeImproveMod (int nCharge, int nSn) {
    int nMod, nWait = skill_table[nSn].wait;
    if (nCharge < 1) // Skill shouldn't have been charged if nCharge == 0
        return 1;
    if (nCharge > nWait)
        nMod = -1 * nCharge / nWait;
    else
        nMod = nWait / nCharge;
    return nMod;
}

void skill_focus (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int vi_sn;
    int sn = gsn_focus;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	ImproveStat (ch, STAT_WIL, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, 1, victim->nDifficulty);

    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	dam = Damroll(ch, 1, 1, sn, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	if (check_kihit(ch,victim,sn,vi_sn,1, TRUE))
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
	else
		damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;
}

void skill_kamehameha (CHAR_DATA *ch, void *vo, int target) {
	int dam;
    int sn = gsn_kamehameha;
	if (target == TARGET_NONE) {
        CHAR_DATA *vch, *vch_next;
		int nDiffCount = 0;
        int n = 0;

        act ("{6You yell, '{7KAMEHAMEHA!{6'{x", ch, NULL, NULL, TO_CHAR);
        act ("{6$n yells, '{7KAMEHAMEHA!{6'{x", ch, NULL, NULL, TO_ROOM);

    	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
            vch_next = vch->next_in_room;

            if (is_safe_spell(ch,vch,TRUE) ||
				( IS_NPC(vch) && IS_NPC(ch) &&
			      (ch->fighting != vch || vch->fighting != ch) ) )
				continue;

			dam = Damroll(ch, 1, 15, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
			damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);

			check_killer (ch, vch);
			nDiffCount += vch->nDifficulty;
			++n;
		}

		if (n > 0) {
			nDiffCount /= n;
			ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
            ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
            ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        }
	}
	else {
		CHAR_DATA *victim = (CHAR_DATA*)vo;
		int vi_sn;

		if (!victim)
			return;

		if (!get_char_room(ch, NULL, victim->name)) {
			sendch("Your target has left the room.\n\r",ch);
			return;
		}

        act ("{6You yell, '{7KAMEHAMEHA!{6'{x", ch, NULL, NULL, TO_CHAR);
        act ("{6$n yells, '{7KAMEHAMEHA!{6'{x", ch, NULL, NULL, TO_ROOM);

	    ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
        ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

		vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

		dam = Damroll(ch, 1, 15, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

		if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE))
			damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
		else
			damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);
	}
	ch->charge = 0;
}

void skill_scattershot (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam, i;
	int vi_sn;
    int sn = gsn_scattershot;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

    act ("{6You yell, '{7SCATTERSHOT!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7SCATTERSHOT!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, 1, victim->nDifficulty);

    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	for (i=0; i<URANGE(1,get_skill(ch, sn) / 25, 20); ++i) {
		if (check_kihit(ch,victim,gsn_focus,vi_sn,1, i == 0 ? TRUE : FALSE)) {
			dam = Damroll(ch, 1, 5, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);
			damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
			if (number_range(1,2) == 1)
				set_balance(victim, -1, TRUE);
		}
		else
			damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);
	}

	ch->charge = 0;
}

void skill_timestop (CHAR_DATA *ch, void *vo, int target) {
	CHAR_DATA *vch, *vch_next;
	int time;
    int sn = gsn_timestop;

	//time = (get_curr_stat(ch,STAT_INT) + get_curr_stat(ch,STAT_INT)) / 2;
	//time += sqrt(ch->nCurPl) / 100;
	//time += UMAX(1,get_skill(ch, gsn_timestop))*5;
	time = 15 * PULSE_SECOND;

    act ("{6You yell, '{7TIMESTOP!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7TIMESTOP!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), ch->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), ch->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), ch->nDifficulty);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (vch == ch)
			continue;
		/*
			if (is_safe_spell(ch,vch,TRUE) ||
			 (IS_NPC(vch) && IS_NPC(ch) &&
			    (ch->fighting != vch || vch->fighting != ch)))
			continue;
		*/

		wait(vch, time);
		//check_killer (ch, vch);
    }

	ch->charge = 0;
}

void skill_energy_ball (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int vi_sn;
    int sn = gsn_energy_ball;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

    act ("{6You yell, '{7ENERGY BALL!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7ENERGY BALL!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	dam = Damroll(ch, 1, 6, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE))
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
	else
		damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;
}

void skill_energy_beam (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam, i;
	int vi_sn;
    int sn = gsn_energy_beam;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

    act ("{6You yell, '{7ENERGY BEAM!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7ENERGY BEAM!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	dam = Damroll(ch, 1, 5, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	for (i = 0; i < URANGE(1,get_skill(ch, sn) / 50, 20); ++i) {
		if (check_kihit(ch,victim,gsn_focus,vi_sn,1, i == 0 ? TRUE : FALSE))
			damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
		else
			damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);
	}

	ch->charge = 0;
}

void skill_energy_slash (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
    OBJ_DATA *wield;
    int dam, dam_type, dt;
    int wpn_sn, sn = gsn_energy_slash;

    if (!victim)
		return;
	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

    wield = get_eq_char(ch, WEAR_WIELD);
    if (wield == NULL)
        return;
    dt = TYPE_HIT;
    if (wield->item_type == ITEM_WEAPON)
        dt += wield->value[3];
    else
        dt += ch->dam_type;
    if (dt < TYPE_HIT)
        dam_type = attack_table[wield->value[3]].damage;
    else
        dam_type = attack_table[dt - TYPE_HIT].damage;
    if (dam_type == -1)
        dam_type = DAM_BASH;

    // Get needed skills
    wpn_sn = get_weapon_sn (ch);

    act ("{6You yell, '{7ENERGY SLASH!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7ENERGY SLASH!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, gsn_energy_slash), victim->nDifficulty);
    ImproveSkill (ch, wpn_sn, TRUE, ChargeImproveMod(ch->charge, gsn_energy_slash), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, gsn_energy_slash), victim->nDifficulty);

    if (check_hit(ch,victim,wpn_sn,get_defend_skill(victim, DEF_DODGE|DEF_SHIELD|DEF_PARRY),1, TRUE)) {
        dam = Damroll(ch, wield->value[1], wield->value[2], wpn_sn, TRUE)
              + Damroll(ch, 4, 25, gsn_focus, TRUE)
              - Absorb(victim, gsn_defend, dt, TRUE);
		damage (ch, victim, dam, gsn_energy_slash, DAM_ENERGY, TRUE);
        set_balance(victim, -1, TRUE);
    }
    else {
        damage (ch, victim, 0, gsn_energy_slash, DAM_ENERGY, TRUE);
		if (number_range(1,2) == 1)
			set_balance(ch, -1, TRUE);
    }
	ch->charge = 0;
}

void skill_finalflash (CHAR_DATA *ch, void *vo, int target) {
	int dam;
    int sn = gsn_finalflash;
	if (target == TARGET_NONE) {
		CHAR_DATA *vch, *vch_next;
		int n = 0;
		int nDiffCount = 0;

        act ("{6You yell, '{7FINAL FLASH!{6'{x", ch, NULL, NULL, TO_CHAR);
        act ("{6$n yells, '{7FINAL FLASH!{6'{x", ch, NULL, NULL, TO_ROOM);

		for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
			vch_next = vch->next_in_room;

			if (is_safe_spell(ch,vch,TRUE) ||
				 (IS_NPC(vch) && IS_NPC(ch) &&
					(ch->fighting != vch || vch->fighting != ch)))
				continue;

			dam = Damroll(ch, 2, 20, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
			damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);

			check_killer (ch, vch);
			nDiffCount += vch->nDifficulty;
			++n;
		}

		if (n > 0) {
			nDiffCount /= n;
			ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
		    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
            ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
		}
	}
	else {
		CHAR_DATA *victim = (CHAR_DATA*)vo;
		int vi_sn;

		if (!victim)
			return;

		if (!get_char_room(ch, NULL, victim->name)) {
			sendch("Your target has left the room.\n\r",ch);
			return;
		}

        act ("{6You yell, '{7FINAL FLASH!{6'{x", ch, NULL, NULL, TO_CHAR);
        act ("{6$n yells, '{7FINAL FLASH!{6'{x", ch, NULL, NULL, TO_ROOM);

	    ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
		ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

		vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

		dam = Damroll(ch, 2, 20, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

		if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE))
			damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
		else
			damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);

	}
	ch->charge = 0;
}

void skill_specialbeam (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
    int sn = gsn_specialbeam;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

    act ("{6You yell, '{7SPECIAL BEAM CANNON!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7SPECIAL BEAM CANNON!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    dam = Damroll(ch, 8, 10, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);
	damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;

}

void skill_solarflare (CHAR_DATA *ch, void *vo, int target) {
	CHAR_DATA *vch, *vch_next;
	AFFECT_DATA af;
	int n = 0;
	long nDiffCount = 0;
    int sn = gsn_solarflare;
	int dam;

    act ("{6You yell, '{7SOLAR FLARE!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7SOLAR FLARE!{6'{x", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE) ||
			 (IS_NPC(vch) && IS_NPC(ch) &&
				(ch->fighting != vch || vch->fighting != ch)))
			continue;

		if (get_skill (ch, sn) <= 50) {
			char *msg;
			if (get_skill (ch, sn) < 10)
				msg = "briefly ";
			else if (get_skill (ch, sn) < 25)
				msg = "somewhat ";
			else if (get_skill (ch, sn) < 40)
				msg = "";
			else
				msg = "terribly ";
			printf_to_char (vch, "You are %s stunned!\n\r", msg);
			act ("{5$n seems to be $tstunned by the flash of light!{x", vch, msg, NULL, TO_ROOM);
			wait (vch, PULSE_SECOND * get_skill (ch, sn) / 10);
		}
		else {
			if (get_skill(ch, sn) < 100) {
				sendch ("You can't see!\n\r", vch);
				act ("{5$n seems to be blinded by the flash of light!{x", vch, NULL, NULL, TO_ROOM);
			}
			else {
				sendch ("Your eyes are burned by the flash!\n\r", vch);
				act ("{5$n's eyes seemed to be burned by the flash of light!{x", vch, NULL, NULL, TO_ROOM);
				dam = Damroll(ch, 1, 10, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
				damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);
			}
			af.where = TO_AFFECTS;
			af.type = sn;
			af.skill_lvl = get_skill(ch, sn);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = 1;
			af.bitvector = AFF_BLIND;
			affect_to_char (vch, &af);
            wait (vch, PULSE_SECOND * get_skill (ch, sn) / 10);
		}
		check_killer (ch, vch);
		begin_combat (vch, ch);
		nDiffCount += vch->nDifficulty;
		++n;
	}

	if (n > 0) {
		nDiffCount /= n;
		ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
		ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
    }

	ch->charge = 0;
}

void skill_destructo_disk (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
    int sn = gsn_destructo_disk;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

    act ("{6You yell, '{7DESTRUCTO DISK!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7DESTRUCTO DISK!{6'{x", ch, NULL, NULL, TO_ROOM);

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

	dam = Damroll(ch, 2, 10, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	// Two disks at skill level >50
	if (get_skill(ch, sn) > 50)
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
	// Three at 750
    if (get_skill(ch, sn) > 750)
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
	damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;
}

void skill_masenko (CHAR_DATA *ch, void *vo, int target) {
	int dam;
    int sn = gsn_masenko;
	if (target == TARGET_NONE) {
		CHAR_DATA *vch, *vch_next;
		int n = 0;
		int nDiffCount = 0;

        act ("{6You yell, '{7MASENKO!{6'{x", ch, NULL, NULL, TO_CHAR);
        act ("{6$n yells, '{7MASENKO!{6'{x", ch, NULL, NULL, TO_ROOM);

        for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
			vch_next = vch->next_in_room;

			if (is_safe_spell(ch,vch,TRUE) ||
				 (IS_NPC(vch) && IS_NPC(ch) &&
					(ch->fighting != vch || vch->fighting != ch)))
				continue;

			dam = Damroll(ch, 5, 15, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
			damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);

			check_killer (ch, vch);
			nDiffCount += vch->nDifficulty;
			++n;
		}

		if (n > 0) {
			nDiffCount /= n;
		    ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
		    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
            ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        }
	}
	else {
		CHAR_DATA *victim = (CHAR_DATA*)vo;
		int vi_sn;

		if (!victim)
			return;

		if (!get_char_room(ch, NULL, victim->name)) {
			sendch("Your target has left the room.\n\r",ch);
			return;
		}

        act ("{6You yell, '{7MASENKO!{6'{x", ch, NULL, NULL, TO_CHAR);
        act ("{6$n yells, '{7MASENKO!{6'{x", ch, NULL, NULL, TO_ROOM);

	    ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
        ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

        vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

		dam = Damroll(ch, 5, 15, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

		if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE))
			damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
		else
			damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);
	}
	ch->charge = 0;
}


void skill_eyebeam (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int vi_sn;
    int sn = gsn_eyebeam;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    act ("{6You yell, '{7EYE BEAM!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7EYE BEAM!{6'{x", ch, NULL, NULL, TO_ROOM);

	vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	dam = Damroll(ch, 1, 10, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE))
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
	else
		damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;
}

void skill_mouthbeam (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int vi_sn;
    int sn = gsn_mouthbeam;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    act ("{6You yell, '{7MOUTH BEAM!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7MOUTH BEAM!{6'{x", ch, NULL, NULL, TO_ROOM);

    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	dam = Damroll(ch, 1, 8, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE))
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
	else
		damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;
}

void skill_fingerbeam (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int dam;
	int vi_sn;
    int sn = gsn_fingerbeam;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    act ("{6You yell, '{7FINGER BEAM!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7FINGER BEAM!{6'{x", ch, NULL, NULL, TO_ROOM);

    vi_sn = get_defend_skill(victim, DEF_DODGE|DEF_SHIELD);

	dam = Damroll(ch, 1, 18, gsn_focus, FALSE) - Absorb(victim, gsn_defend, DAM_ENERGY, FALSE);

	if (check_kihit(ch,victim,gsn_focus,vi_sn,1, TRUE)) {
		damage (ch, victim, dam, sn, DAM_ENERGY, TRUE);
		if (number_range(1,100) + (ch->charge == 1 ? 50 : 0) <= UMIN(1 + get_skill(ch, sn)/2, 75)) {
			act ("{5Your beam kills $M instantly!{x", ch, NULL, victim, TO_CHAR);
			act ("{5$n's beam kills you instantly!{x", ch, NULL, victim, TO_VICT);
			act ("{5$n's beam kills $N instantly!{x", ch, NULL, victim, TO_NOTVICT);
	        ch->charge = 0;

			raw_kill (victim);
			return;
		}
	}
	else
		damage (ch, victim, 0, sn, DAM_ENERGY, TRUE);

	ch->charge = 0;
}

void skill_spirit_bomb (CHAR_DATA *ch, void *vo, int target) {
	CHAR_DATA *vch, *vch_next;
	int dam;
	int n = 0;
	int nDiffCount = 0;
    int sn = gsn_spirit_bomb;

    act ("{6You yell, '{7SPIRIT BOMB!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7SPIRIT BOMB!{6'{x", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE) ||
			 (IS_NPC(vch) && IS_NPC(ch) &&
				(ch->fighting != vch || vch->fighting != ch)))
			continue;

		dam = Damroll(ch, 3, 20, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
		damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);

		check_killer (ch, vch);
		nDiffCount += vch->nDifficulty;
		++n;
	}

	if (n > 0) {
		nDiffCount /= n;
		ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
	}

	ch->charge = 0;
}


void skill_power_bomb (CHAR_DATA *ch, void *vo, int target) {
	CHAR_DATA *vch, *vch_next;
	int dam;
	int n = 0;
	int nDiffCount = 0;
    int sn = gsn_power_bomb;

    act ("{6You yell, '{7POWER BOMB!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7POWER BOMB!{6'{x", ch, NULL, NULL, TO_ROOM);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE) ||
			 (IS_NPC(vch) && IS_NPC(ch) &&
				(ch->fighting != vch || vch->fighting != ch)))
			continue;

		dam = Damroll(ch, 3, 25, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
		damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);

		check_killer (ch, vch);
		nDiffCount += vch->nDifficulty;
		++n;
	}

	if (n > 0) {
		nDiffCount /= n;
		ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
	}

	ch->charge = 0;
}

void skill_death_ball (CHAR_DATA *ch, void *vo, int target) {
	CHAR_DATA *vch, *vch_next;
	int dam;
	int n = 0;
	int nDiffCount = 0;
    int sn = gsn_death_ball;

    act ("{6You yell, '{7DEATH BALL!{6'{x", ch, NULL, NULL, TO_CHAR);
    act ("{6$n yells, '{7DEATH BALL!{6'{x", ch, NULL, NULL, TO_ROOM);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE) ||
			 (IS_NPC(vch) && IS_NPC(ch) &&
				(ch->fighting != vch || vch->fighting != ch)))
			continue;

		dam = Damroll(ch, 5, 20, gsn_focus, FALSE) - Absorb(vch, gsn_defend, DAM_ENERGY, FALSE);
		damage (ch, vch, dam, sn, DAM_ENERGY, TRUE);

		check_killer (ch, vch);
		nDiffCount += vch->nDifficulty;
		++n;
	}

	if (n > 0) {
		nDiffCount /= n;
		ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
        ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), nDiffCount);
	}

	ch->charge = 0;
}

// *****************
// Non-combat skills
// *****************

void skill_heal (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	int heal;
    int sn = gsn_heal;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	ImproveStat (ch, STAT_WIL, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, ChargeImproveMod(ch->charge, sn), victim->nDifficulty);

    heal = number_range(1,6) + (get_curr_stat(ch, STAT_WIL) + get_skill(ch, gsn_focus)) / 4 + ch->charge;
	victim->hit = UMIN (victim->hit + heal, victim->max_hit);
    update_pos (victim);
    sendch ("You have been healed!\n\r", victim);
    if (ch != victim)
        act ("You have healed $N.", ch, NULL, victim, TO_CHAR);
	ch->charge = 0;
}

void skill_revive (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
    int sn = gsn_revive;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	ImproveStat (ch, STAT_WIL, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, 1, victim->nDifficulty);

	victim->hit = victim->max_hit;
    update_pos (victim);
    sendch ("You have been revived!\n\r", victim);
    if (ch != victim)
        act ("You have revived $N.", ch, NULL, victim, TO_CHAR);
	ch->charge = 0;
}

void skill_regen (CHAR_DATA *ch, void *vo, int target) {
    CHAR_DATA *victim = (CHAR_DATA*)vo;
	AFFECT_DATA af;
    int sn = gsn_regen;

	if (!victim)
		return;

	if (!get_char_room(ch, NULL, victim->name)) {
		sendch("Your target has left the room.\n\r",ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_REGENERATION))
		return;

	ImproveStat (ch, STAT_WIL, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, sn, TRUE, 1, victim->nDifficulty);
    ImproveSkill (ch, gsn_focus, TRUE, 1, victim->nDifficulty);

	af.where = TO_AFFECTS;
	af.type = sn;
	af.skill_lvl = get_skill(ch, gsn_focus);
	af.location = APPLY_STR;
	af.modifier = UMAX(1, sqrt(get_skill(ch, gsn_focus) / 2));
	af.duration = 2 + sqrt(get_skill(ch, gsn_focus) * 3);
	af.bitvector = AFF_REGENERATION;
	affect_to_char (victim, &af);

	sendch ("You feel like your stamina has had an incredible boost!\n\r", victim);
    return;
}

