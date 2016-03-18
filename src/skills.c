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
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"

// Locals
void          UpdateSkills    args ( ( CHAR_DATA *pCh ) );
long long int ApplyMisc (CHAR_DATA *pCh, long long int llValue);
long long int ApplyCharge (CHAR_DATA *pCh, long long int llValue, int nSn);

/*
 * Lookup a skill by name.
 */
int skill_lookup (const char *name)
{
    int sn;

    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
        if (LOWER (name[0]) == LOWER (skill_table[sn].name[0])
            && !str_prefix (name, skill_table[sn].name))
            return sn;
    }

    return -1;
}

/* skill_driver:
 * Does a skill. Checks for targets, shows messages, performs the skill.
 * Returns true if the character completes the skill.
 */
bool skill_driver (CHAR_DATA * ch, char *argument, int sn)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    void *vo;
    int target, sn_target;
    CHAR_DATA *victim;
    OBJ_DATA *obj;

	if (sn < 1)
		return FALSE;

    if (ch->position < skill_table[sn].minimum_position)
    {
        sendch ("You can't do that in your current position.\n\r", ch);
        return FALSE;
    }


	if (get_skill(ch, sn) < 1) {
		sendch ("What?\n\r", ch);
		return FALSE;
	}

    argument = one_argument (argument, arg1);
    one_argument (argument, arg2);

    // Switch them if needed
    if (!str_cmp(arg1, "release")) {
        char t[MAX_INPUT_LENGTH];
        sprintf(t, arg2);
        sprintf(arg2, arg1);
        sprintf(arg1, t);
    }
    
    /*
     * Locate targets.
     */
    victim = NULL;
    obj = NULL;
    vo = NULL;
    target = TARGET_NONE;

	sn_target = skill_table[sn].target;
	// Look for skills whose targets switch at certain skill levels
	if (sn_target == TAR_HYBRID100) {
		if (get_skill(ch, sn) > 99 && !str_cmp(arg1, "all"))
			sn_target = TAR_AREA_OFF;
		else
			sn_target = TAR_CHAR_OFFENSIVE;
	}

    switch (sn_target)
    {
        default:
            logstr (LOG_BUG, "skill_driver: bad target for sn %d.", sn);
            return FALSE;

        case TAR_IGNORE:
        case TAR_AREA_OFF:
			vo = NULL;
			target = TARGET_NONE;
            break;

        case TAR_CHAR_OFFENSIVE:
            if (arg1[0] == '\0')
            {
                if ((victim = ch->fighting) == NULL)
                {
                    sendch ("Direct that at whom?\n\r", ch);
                    return FALSE;
                }
            }
            else
            {
                if ((victim = get_char_room (ch, NULL, arg1)) == NULL)
                {
                    sendch ("They aren't here.\n\r", ch);
                    return FALSE;
                }
            }

			if (ch == victim)
			{
				sendch( "You can't do that to yourself.\n\r", ch );
				return FALSE;
			}

            if (IS_NPC (victim) && victim->fighting != NULL && !is_same_group (ch, victim->fighting)) {
                sendch ("Kill stealing is not permitted.\n\r", ch);
                return FALSE;
            }

            if (!IS_NPC (ch))
            {

                if (is_safe (ch, victim) && victim != ch)
                {
                    sendch ("Not on that target.\n\r", ch);
                    return FALSE;
                }
                check_killer (ch, victim);
            }

            if (IS_AFFECTED (ch, AFF_CHARM) && ch->master == victim)
            {
                sendch ("You can't do that on your own follower.\n\r", ch);
                return FALSE;
            }
            vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
            if (arg1[0] == '\0')
            {
                victim = ch;
            }
            else
            {
                if ((victim = get_char_room (ch, NULL, arg1)) == NULL)
                {
                    sendch ("They aren't here.\n\r", ch);
                    return FALSE;
                }
            }
			vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if (arg1[0] != '\0' && !is_name (arg1, ch->name))
            {
                sendch ("You cannot do that on another.\n\r", ch);
                return FALSE;
            }
    		vo = (void *) ch;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if (arg1[0] == '\0')
            {
                sendch ("What object should that be done on?\n\r", ch);
                return FALSE;
            }

            if ((obj = get_obj_carry (ch, arg1, ch)) == NULL)
            {
                sendch ("You are not carrying that.\n\r", ch);
                return FALSE;
            }
			vo = (void *) obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if (arg1[0] == '\0')
            {
                if ((victim = ch->fighting) == NULL)
                {
                    sendch ("Do that on whom or what?\n\r", ch);
                    return FALSE;
                }

                target = TARGET_CHAR;
            }
            else if ((victim = get_char_room (ch, NULL, arg1)) != NULL)
            {
                target = TARGET_CHAR;
            }

            if (target == TARGET_CHAR)
            {                    /* check the sanity of the attack */
                if (is_safe_spell (ch, victim, FALSE) && victim != ch)
                {
                    sendch ("Not on that target.\n\r", ch);
                    return FALSE;
                }

                if (IS_AFFECTED (ch, AFF_CHARM) && ch->master == victim)
                {
                    sendch ("You can't do that on your own follower.\n\r", ch);
                    return FALSE;

                }

                if (!IS_NPC (ch))
                    check_killer (ch, victim);
				vo = (void *) victim;
            }
            else if ((obj = get_obj_here (ch, NULL, arg1)) != NULL)
            {
				vo = (void *) obj;
                target = TARGET_OBJ;
            }
            else
            {
                sendch ("You don't see that here.\n\r", ch);
                return FALSE;
            }
            break;

        case TAR_OBJ_CHAR_DEF:
            if (arg1[0] == '\0')
            {
				vo = (void *) ch;
                target = TARGET_CHAR;
            }
            else if ((victim = get_char_room (ch, NULL, arg1)) != NULL)
            {
				vo = (void *) victim;
                target = TARGET_CHAR;
            }
            else if ((obj = get_obj_carry (ch, arg1, ch)) != NULL)
            {

				vo = (void *) obj;
                target = TARGET_OBJ;
            }
            else
            {
                sendch ("You don't see that here.\n\r", ch);
                return FALSE;
            }
            break;
    }

    if (IS_EXHAUSTED(ch)) {
        sendch ("You're too exhausted.\n\r", ch);
        return FALSE;
    }

    // Hard coded checks for skills
    if (sn == gsn_hyperpunch) {
        if (get_eq_char(ch, WEAR_WIELD) != NULL) {
            sendch("You cannot hyperpunch with a weapon.\n\r", ch);
            return FALSE;
        }
    }
    else if (sn == gsn_energy_slash) {
        if (get_eq_char(ch, WEAR_WIELD) == NULL) {
            sendch("You need a weapon to energy slash with.\n\r", ch);
            return FALSE;
        }
    }

    // Use a ki_loss value based on the skill

    // Ki loss.
    // If its a charge type skill, make the initial loss much greater
    if (skill_table[sn].type == SKILL_CHARGE)
        ki_loss(ch, skill_table[sn].ki_mod*5);
    else
        ki_loss(ch, skill_table[sn].ki_mod);

    // Message if the skill is started
	if (skill_table[sn].msg_immediate1)
		act(skill_table[sn].msg_immediate1,ch,NULL,NULL,TO_CHAR);
	if (skill_table[sn].msg_immediate2)
		act(skill_table[sn].msg_immediate2,ch,NULL,NULL,TO_ROOM);

	// Make the char wait, and get the skill ready to fire (or fire it)
	if (skill_table[sn].type == SKILL_DELAY) {
		ch->wait_skill = skill_table[sn].wait;
		ch->wait_skill_sn = sn;
		ch->wait_skill_vo = vo;
		ch->wait_skill_target = target;
	}
	else if (skill_table[sn].type == SKILL_IMM) {
		wait (ch, skill_table[sn].wait);
		(*skill_table[sn].skill_fun) (ch, vo, target);
	}
    else if (skill_table[sn].type == SKILL_CHARGE) {
        ch->wait_skill = 0;
        ch->charge = 1;
        ch->wait_skill_sn = sn;
        ch->wait_skill_vo = vo;
        ch->wait_skill_target = target;
    }

    if ((sn_target == TAR_CHAR_OFFENSIVE
         || (sn_target == TAR_OBJ_CHAR_OFF
             && target == TARGET_CHAR))
	    && victim != ch
        && victim->master != ch
        && victim->fighting == NULL
        && IS_AWAKE(victim)
        && !IS_AFFECTED (victim, AFF_CALM) ) {
        check_killer (victim, ch);
        begin_combat (victim, ch);
    }
    else if (sn_target == TAR_AREA_OFF) {
		CHAR_DATA *vch;

        for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {
            if (IS_NPC(vch)
                && vch->fighting == NULL
                && !IS_AFFECTED (vch, AFF_CALM)
                && !IS_AFFECTED (vch, AFF_CHARM)
                && IS_AWAKE (vch)
                && !IS_SET (vch->act, ACT_WIMPY)
                && !is_same_group(vch, ch)
                && can_see (vch, ch) ) {
                check_killer (vch, ch);
                begin_combat (vch, ch);
                break;
            }
        }
    }

    // Immediate "charged" attack
    if (skill_table[sn].type == SKILL_CHARGE &&
        (!str_cmp(arg1, "release") || !str_cmp(arg2, "release")) ) {
        // Pulled from do_release
        act("You release!",ch,NULL,NULL,TO_CHAR);
        act("$n releases!",ch,NULL,NULL,TO_ROOM);

        if (skill_table[ch->wait_skill_sn].msg_delay1)
            act(skill_table[ch->wait_skill_sn].msg_delay1,ch,NULL,NULL,TO_CHAR);
        if (skill_table[ch->wait_skill_sn].msg_delay2)
            act(skill_table[ch->wait_skill_sn].msg_delay2,ch,NULL,NULL,TO_ROOM);

        if (skill_table[ch->wait_skill_sn].skill_fun)
            (*skill_table[ch->wait_skill_sn].skill_fun) (ch, ch->wait_skill_vo, ch->wait_skill_target);

        ch->charge = 0;
        ch->wait_skill_sn = 0;
        ch->wait_skill_vo = NULL;
        ch->wait_skill_target = 0;
        wait (ch, 3 * PULSE_SECOND);
    }

    return TRUE;
}


void do_skills (CHAR_DATA * ch, char *argument) {
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH],
         buf2[MAX_STRING_LENGTH],
		 buf3[MAX_STRING_LENGTH];
	char buf_temp[MAX_STRING_LENGTH];
    bool found=FALSE, bColumn=TRUE, bOtherFound=FALSE;
    int i;

    if (IS_NPC (ch))
        return;

    sprintf(buf, "Statistics:\n\r  ");
    for (i = 0; i < MAX_STATS; i++) {
        if (ch->perm_stat[i] > 0) {
			sprintf (buf_temp, "%-20s %3d.%-2.2d          ", stat_table[i], ch->perm_stat[i],
				     100*ch->pcdata->nStatProgress[i]/(ch->perm_stat[i]*3 + 1800));
			strcat (buf, buf_temp);
			if ((bColumn=!bColumn))
				strcat (buf, "\n\r  ");
        }
    }
	if (!bColumn)
		strcat (buf, "\n\r");

    sprintf(buf2, "\n\rSkills:\n\r  ");
    bColumn = TRUE;
    for (i = 0; i < MAX_SKILL; i++) {
        if (skill_table[i].name == NULL)
            break;
        if (ch->pcdata->learned[i] > 0 && skill_table[i].bCanImprove) {
            found = TRUE;
			sprintf (buf_temp, "%-20s %3d.%-2.2d         ", skill_table[i].name, ch->pcdata->learned[i],
				     100*ch->pcdata->nSkillProgress[i]/(3240 + ch->pcdata->learned[i]*36));
			strcat (buf2, buf_temp);
			if ((bColumn=!bColumn))
				strcat (buf2, "\n\r  ");
        }
    }
    if (!found)
        sprintf (buf2, "\n\rSkills:\n\r  No skills found.\n\r");
    else if (!bColumn)
	    strcat (buf2, "\n\r");

    // Other skills that dont show any skill level
	sprintf(buf3, "\n\rOther Skills:\n\r  ");
    bColumn = TRUE;
    for (i = 0; i < MAX_SKILL; i++) {
        if (skill_table[i].name == NULL)
            break;
        if (ch->pcdata->learned[i] > 0 && !skill_table[i].bCanImprove) {
            bOtherFound = TRUE;
			sprintf (buf_temp, "%-20s                 ", skill_table[i].name);
			strcat (buf3, buf_temp);
			if ((bColumn=!bColumn))
				strcat (buf3, "\n\r  ");
        }
    }
    if (!bColumn)    
	    strcat (buf3, "\n\r");

    buffer = new_buf ();
	add_buf (buffer, buf);
	if (bOtherFound && !found)
		add_buf (buffer, buf3);
	else {
	    add_buf (buffer, buf2);
		if (bOtherFound)
           add_buf (buffer, buf3);
	}
	page_to_char (buf_string (buffer), ch);
    free_buf (buffer);
}


// If the multiplier is negative, its easier.
// For example, nMultiplier =  5 : 5 times harder to learn
//              nMultiplier = -4 : 4 times easier to learn
void ImproveSkill (CHAR_DATA *pCh, int nSn, bool bSuccess, float nMultiplier, int nViDifficulty) {
    char szBuf[MAX_STRING_LENGTH];
	int nGain, nNext;

    if (IS_NPC (pCh))
        return;

	if (nSn < 0
		|| nSn >= MAX_SKILL
        || pCh->pcdata->learned[nSn] <= 0
		|| !skill_table[nSn].bCanImprove
        || !skill_table[nSn].bCanLearn)
        return;
        
    if (nViDifficulty < pCh->nDifficulty / 4)
        return;

    // could all the counts (teach, train, chaos) be added in here.
    // pass a flag in ie IMPROVE_PENALTY_TEACH and automatically
    // increment the necessary counter?
    if (IS_SET(pCh->in_room->room_flags, ROOM_CHAOS) && pCh->fighting) {
        nMultiplier += pCh->pcdata->nChaosCount / 70;
        ++pCh->pcdata->nChaosCount;
    }

    // This does not increase linearally, since its based on int. As
    // int increases, skills have to become harder to learn (so, it
    // actually seems linear)
    // The magic number is 360, which is the number of 5 second actions
    // to make up half an hour.  We're aiming for a skill to go up every
    // half an hour.  So, nNext / nGain = 360. Also, gains are based on
    // intelligence, so nNext must increase to offest increasing
    // intelligence.  Since stats are equal to skills in level, if
    // intelligence is equal to the skill, nNext / nGain will equal 360.
    nNext = 3240 + pCh->pcdata->learned[nSn]*36;

    nGain = 9 + get_curr_stat (pCh, STAT_INT) / 10;
	if (nMultiplier > 0)
		nGain = UMAX(1, nGain / nMultiplier); // Don't want it to totally destroy any gains
	else
		nGain *= -1 * nMultiplier;

    // Change the gain based on relative difficulties
	//nGain = URANGE(nGain / 3, nViDifficulty * nGain / pCh->nDifficulty, nGain * 2);
    // Change the gain based on current power use
	nGain = UMAX(nGain / 10, pCh->nCurPl * nGain / 100);
	// Make all skills increase at the same rate, regardless of speed
    nGain = (nGain * skill_table[nSn].wait) / (5 * PULSE_SECOND);
    // Limit the gain
    nGain = UMIN(nGain, nNext / 50);
    // Get a reward?
    if (pCh->pcdata->nReward > 0)
        nGain *= 2;

    if (pCh->pcdata->nSkillTick[nSn] + nGain > (1800 * PULSE_SECOND * nNext) / PULSE_TICK) {
        nGain = ((1800 * PULSE_SECOND * nNext) / PULSE_TICK) - pCh->pcdata->nSkillTick[nSn];
        pCh->pcdata->nSkillTick[nSn] = (1800 * PULSE_SECOND * nNext) / PULSE_TICK;
    }

	pCh->pcdata->nSkillProgress[nSn] += nGain;

	if (pCh->pcdata->nSkillProgress[nSn] < nNext)
		return;

	if (bSuccess)
        sprintf (szBuf, "{CHoning your skills, your %s improves!{x\n\r", skill_table[nSn].name);
    else
        sprintf (szBuf, "{CThrough trial and error, your %s improves!{x\n\r", skill_table[nSn].name);
    sendch (szBuf, pCh);
    pCh->pcdata->learned[nSn]++;
	pCh->pcdata->nSkillProgress[nSn] = 0;
	ResetDiff(pCh);
    UpdateSkills (pCh);
    return;
}

// If the multiplier is negative, its easier.
// For example, nMultiplier =  5 : 5 times harder to learn
//              nMultiplier = -4 : 4 times easier to learn
void ImproveStat (CHAR_DATA *pCh, int nStat, bool bSuccess, float nMultiplier, int nViDifficulty) {
    char szBuf[MAX_STRING_LENGTH];
	int nGain, nNext;

    if (IS_NPC (pCh))
        return;

	if (nStat < 0
		|| nStat >= MAX_STATS)
        return;
    
    if (nViDifficulty < pCh->nDifficulty / 4)
        return;

    if (IS_SET(pCh->in_room->room_flags, ROOM_CHAOS) && pCh->fighting) {
        nMultiplier += pCh->pcdata->nChaosCount / 70;
        ++pCh->pcdata->nChaosCount;
    }

	// This increases almost linearally, since int doesn't effect it.
    // Skills, on the other hand, increase based on int, so they have to
    // become harder to offset increasing int
    nNext = pCh->perm_stat[nStat]*3 + 1800;

    nGain = pc_race_table[pCh->race].stat_gain[nStat];
	if (nMultiplier > 0)
		nGain = UMAX(1, nGain / nMultiplier); // Don't want it to totally destroy any gains
	else
		nGain *= -1 * nMultiplier;


    // Change the gain based on relative difficulties
	//nGain = URANGE(nGain / 3, nViDifficulty * nGain / pCh->nDifficulty, nGain * 2);
    // Change the gain based on current power use
	nGain = UMAX(nGain / 10, pCh->nCurPl * nGain / 100);
	// Limit the gain
	nGain = UMIN(nGain, nNext / 50);
    // Get a reward?
    if (pCh->pcdata->nReward > 0)
        nGain *= 2;

	pCh->pcdata->nStatProgress[nStat] += nGain;

	if (pCh->pcdata->nStatProgress[nStat] < nNext)
		return;

    if (bSuccess)
        sprintf (szBuf, "{CWith hard work, your %s improves!{x\n\r", stat_table[nStat]);
    else
        sprintf (szBuf, "{CThrough trial and error, your %s improves!{x\n\r", stat_table[nStat]);
    sendch (szBuf, pCh);
    pCh->perm_stat[nStat]++;
	if (nStat == STAT_STR) {
        pCh->max_hit += HP_STR;
		pCh->pcdata->perm_hit += HP_STR;
        pCh->max_ki += KI_STR;
		pCh->pcdata->perm_ki += KI_STR;
	}
    else if (nStat == STAT_WIL) {
        pCh->max_hit += HP_WIL;
		pCh->pcdata->perm_hit += HP_WIL;
        pCh->max_ki += KI_WIL;
		pCh->pcdata->perm_ki += KI_WIL;
    }
	pCh->pcdata->nStatProgress[nStat] = 0;
	ResetDiff(pCh);
    UpdateSkills (pCh);
    return;
}

void UpdateSkills (CHAR_DATA *pCh) {
    char szBuf[MAX_STRING_LENGTH];
    int nSn;

    if (IS_NPC(pCh))
        return;
    for (nSn = 0; nSn < MAX_SKILL; ++nSn) {
        if (pCh->pcdata->learned[nSn] < 1 && MeetsPrereq(pCh, nSn)) {
            pCh->pcdata->learned[nSn] = 1;
            sprintf(szBuf, "{CWith your new insights, you now know %s!{x\n\r", skill_table[nSn].name);
            sendch (szBuf, pCh);
        }
    }
}

void ResetDiff (CHAR_DATA *pCh) {
    int i, nCount = 0;
	pCh->nDifficulty = 0;
    pCh->nTrueDiff = 0;

    for (i = 0; i < MAX_SKILL; ++i) {
		// Limit the number of skills NPCs get, to protect against
		// artificially high difficult ratings
		if (IS_NPC(pCh) && ++nCount > get_curr_stat (pCh, STAT_INT) + 1)
			break;
		pCh->nDifficulty += get_skill (pCh, i);
	}
	pCh->nDifficulty += get_curr_stat (pCh, STAT_STR);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_DEX);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_CHA);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_INT);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_WIL);
    pCh->llPl = pCh->nDifficulty * pCh->nDifficulty;

    for (i = 0; i < MAX_SKILL; ++i) {
		if (IS_NPC(pCh) && ++nCount > pCh->perm_stat[STAT_INT] + 1)
			break;
		pCh->nTrueDiff += get_skill (pCh, i);
	}
	pCh->nTrueDiff += pCh->perm_stat[STAT_STR];
	pCh->nTrueDiff += pCh->perm_stat[STAT_DEX];
	pCh->nTrueDiff += pCh->perm_stat[STAT_CHA];
	pCh->nTrueDiff += pCh->perm_stat[STAT_INT];
	pCh->nTrueDiff += pCh->perm_stat[STAT_WIL];
    pCh->llTruePl = pCh->nTrueDiff * pCh->nTrueDiff;

    UpdateSkills (pCh);
}



/*
// Total of all skills and statistics
void ResetDifficulty (CHAR_DATA *pCh) {
    int i, nCount = 0;
	pCh->nDifficulty = 0;

    for (i = 0; i < MAX_SKILL; ++i) {
		// Limit the number of skills NPCs get, to protect against
		// artificially high difficult ratings
		if (IS_NPC(pCh) && ++nCount > get_curr_stat (pCh, STAT_INT) + 1)
			break;
		pCh->nDifficulty += get_skill (pCh, i);
	}

	pCh->nDifficulty += get_curr_stat (pCh, STAT_STR);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_DEX);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_CHA);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_INT);
	pCh->nDifficulty += get_curr_stat (pCh, STAT_WIL);
}

// Total based on skills and statistics.
// Skills are worth 50 pl each, plus the summation of each number from 1 to 20 * skill_level (1 + 2 + 3 + 4 + ... + 20 * lvl).
// Stats are worth the same as skills
// The formulas used are based on (n/2)(n+1)
void ResetPl (CHAR_DATA *pCh) {
    int i, nCount = 0;

	// Maximum powerlevel, including affects
	pCh->llPl = 0;
	for (i = 0; i < MAX_SKILL; ++i) {
		// Limit the number of skills NPCs get, to protect against
		// artificially high powerlevels
		if (IS_NPC(pCh) && ++nCount > get_curr_stat (pCh, STAT_INT) + 1)
			break;
		pCh->llPl += 50 + (10 * get_skill (pCh, i)) * (20 * get_skill (pCh, i) + 1);
	}
	pCh->llPl += 50 + (10 * get_curr_stat (pCh, STAT_STR)) * (20 * get_curr_stat (pCh, STAT_STR) + 1);
	pCh->llPl += 50 + (10 * get_curr_stat (pCh, STAT_DEX)) * (20 * get_curr_stat (pCh, STAT_DEX) + 1);
	pCh->llPl += 50 + (10 * get_curr_stat (pCh, STAT_CHA)) * (20 * get_curr_stat (pCh, STAT_CHA) + 1);
	pCh->llPl += 50 + (10 * get_curr_stat (pCh, STAT_INT)) * (20 * get_curr_stat (pCh, STAT_INT) + 1);
	pCh->llPl += 50 + (10 * get_curr_stat (pCh, STAT_WIL)) * (20 * get_curr_stat (pCh, STAT_WIL) + 1);

	// Find permanent, unadultered powerlevel
    pCh->llTruePl = 0;
	for (i = 0; i < MAX_SKILL; ++i) {
		if (IS_NPC(pCh) && ++nCount > pCh->perm_stat[STAT_INT] + 1)
			break;
		pCh->llTruePl += 50 + (10 * get_skill (pCh, i)) * (20 * get_skill (pCh, i) + 1);
	}
	pCh->llTruePl += 50 + (10 * pCh->perm_stat[STAT_STR]) * (20 * pCh->perm_stat[STAT_STR] + 1);
	pCh->llTruePl += 50 + (10 * pCh->perm_stat[STAT_DEX]) * (20 * pCh->perm_stat[STAT_DEX] + 1);
	pCh->llTruePl += 50 + (10 * pCh->perm_stat[STAT_CHA]) * (20 * pCh->perm_stat[STAT_CHA] + 1);
	pCh->llTruePl += 50 + (10 * pCh->perm_stat[STAT_INT]) * (20 * pCh->perm_stat[STAT_INT] + 1);
	pCh->llTruePl += 50 + (10 * pCh->perm_stat[STAT_WIL]) * (20 * pCh->perm_stat[STAT_WIL] + 1);
}
*/

// ch_gsn is the gsn of the skill the character is using to attack with
// vi_gsn is the gsn he is defending with
// multiplier is used against the victim's chance (therefore, high multiplier makes it harder
//     to hit)
bool check_hit (CHAR_DATA *ch, CHAR_DATA *victim, int ch_gsn, int vi_gsn, float multiplier, bool bLearn) {
    CHAR_DATA *vch;
    char buf[MAX_STRING_LENGTH];
    int ch_roll, vi_roll;
    
    if (bLearn) {
        ImproveSkill (ch, ch_gsn, TRUE, 1, victim->nDifficulty);
        ImproveStat (ch, STAT_DEX, TRUE, 2, victim->nDifficulty); // 2 since it's called twice as often (to hit and dodge)

        ImproveSkill (victim, vi_gsn, TRUE, 1, ch->nDifficulty);
        ImproveStat (victim, STAT_DEX, TRUE, 2, ch->nDifficulty);
    }

    ch_roll = Hitroll (ch, ch_gsn, TRUE);
    vi_roll = Armour (victim, vi_gsn, TRUE);

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {
        if (!IS_NPC(vch) && IS_IMMORTAL(vch) && IS_SET(vch->act, PLR_COMBATINFO)) {
            sprintf(buf, "{WCI:hit  [%s] %d -vs- %d [%s]{x\n\r", NAME(ch), ch_roll, vi_roll, NAME(victim));
		    sendch(buf, vch);
        }
    }

    ki_loss (victim, 6);

	if (ch_roll > vi_roll)
		return TRUE;

    if (vi_gsn == gsn_dodge) {
        act ("{2You dodge!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n dodges!{x", victim, NULL, NULL, TO_ROOM);
    }
    else if (vi_gsn == gsn_shield_block) {
        act ("{2You block with a shield!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n blocks with a shield!{x", victim, NULL, NULL, TO_ROOM);
    }
    else if (vi_gsn == gsn_parry) {
        act ("{2You parry!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n parries!{x", victim, NULL, NULL, TO_ROOM);
    }
    else {
        act ("{2You defend!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n defends!{x", victim, NULL, NULL, TO_ROOM);
    }

    return FALSE;
}

bool check_kihit (CHAR_DATA *ch, CHAR_DATA *victim, int ch_gsn, int vi_gsn, float multiplier, bool bLearn) {
   CHAR_DATA *vch;
    char buf[MAX_STRING_LENGTH];
    int ch_roll, vi_roll;

    if (bLearn) {
        ImproveSkill (ch, ch_gsn, TRUE, 1, victim->nDifficulty);
        ImproveStat (ch, STAT_INT, TRUE, 2, victim->nDifficulty); // 2 since its used twice as often (to hit and dodge)

        ImproveSkill (victim, vi_gsn, TRUE, 1, ch->nDifficulty);
        ImproveStat (victim, STAT_INT, TRUE, 2, ch->nDifficulty);
    }

    ch_roll = Hitroll (ch, ch_gsn, FALSE);
    vi_roll = Armour (victim, vi_gsn, FALSE);

    for (vch = ch->in_room->people; vch; vch = vch->next) {
        if (!IS_NPC(vch) && IS_IMMORTAL(vch) && IS_SET(vch->act, PLR_COMBATINFO)) {
            sprintf(buf, "{WCI:ki  [%s] %d -vs- %d [%s]{x\n\r", NAME(ch), ch_roll, vi_roll, NAME(victim));
		    sendch(buf, vch);
        }
    }

    ki_loss (victim, 6);

	if (ch_roll > vi_roll)
        return TRUE;

    if (vi_gsn == gsn_dodge) {
        act ("{2You dodge!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n dodges!{x", victim, NULL, NULL, TO_ROOM);
    }
    else if (vi_gsn == gsn_shield_block) {
        act ("{2You block with a shield!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n blocks with a shield!{x", victim, NULL, NULL, TO_ROOM);
    }
    else if (vi_gsn == gsn_parry) {
        act ("{2You parry!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n parries!{x", victim, NULL, NULL, TO_ROOM);
    }
    else {
        act ("{2You defend!{x", victim, NULL, NULL, TO_CHAR);
        act ("{3$n defends!{x", victim, NULL, NULL, TO_ROOM);
    }

    return FALSE;
}

int Hitroll (CHAR_DATA *pCh, int nSn, bool bPhysical) {
    int nRoll, nRand;
    nRand  = number_range(1,50);
    // Critical miss
    if (nRand <= 3)
        return 0;
    nRoll  = nRand;
    nRoll += get_curr_stat(pCh, bPhysical ? STAT_DEX : STAT_INT);
    nRoll += get_skill(pCh, nSn);
    nRoll += GET_HITROLL(pCh);
    nRoll += (pCh->balance - 5) * 2;
    nRoll -= (100 - pCh->nCurPl) / 10;
    if (nSn == gsn_heart_shot)
        nRoll -= 2;
    else if (nSn == gsn_eye_gouge)
        nRoll -= 5;
    if (IS_AFFECTED (pCh, AFF_BLIND))
        nRoll -= 5;
    if (IS_AFFECTED (pCh, AFF_FLYING))
        nRoll += 5;
    if (pCh->stance == STANCE_OFFEN)
        nRoll += 4;
    else if (pCh->stance == STANCE_DEFEN)
        nRoll -= 4;
	if (!IS_AWAKE (pCh))
        nRoll -= 5;
    else if (pCh->position < POS_FIGHTING)
        nRoll -= 2;
    if (IS_EXHAUSTED(pCh))
        nRoll -= 10;
    if (pCh->charge > 0)
        nRoll = (pCh->charge * nRoll) / skill_table[nSn].wait;
    if (pCh->charge == 1) // Immediately released skill -- deduction
        nRoll /= 4;
    // Critical:
    if (nRand == 50)
        nRoll *= 3;
    return nRoll;
}

int Damroll (CHAR_DATA *pCh, int nLow, int nHigh, int nSn, bool bPhysical) {
    int nDam;
    nDam  = number_range(nLow, nHigh);
    nDam += get_curr_stat(pCh, bPhysical ? STAT_STR : STAT_WIL) + get_skill(pCh, nSn);
    nDam += GET_DAMROLL(pCh);
    nDam += pCh->balance - 5;
    nDam -= (100 - pCh->nCurPl) / 10;
    /*
    if (sn == gsn_throat_shot ||
        sn == gsn_sweep ||
        sn == gsn_eye_gouge)          nDam -= 15;
    else if (sn == gsn_knee)          nDam -= 3;
    else if (sn == gsn_elbow)         nDam -= 7;
    else if (sn == gsn_heart_shot)    nDam += 10;
    else if (sn == gsn_power_bomb)    nDam += 15;
    else if (sn == gsn_spirit_bomb ||
             sn == gsn_death_ball)    nDam += 10;
    else if (sn == gsn_finalflash ||
             sn == gsn_kamehameha)    nDam += 5;
    else if (sn == gsn_scattershot ||
             sn == gsn_solarflare)    nDam -= 5;
             */
    if (pCh->stance == STANCE_OFFEN)
        nDam += 4;
    else if (pCh->stance == STANCE_DEFEN)
        nDam -= 4;
	if (!IS_AWAKE (pCh))
        nDam -= 5;
    else if (pCh->position < POS_FIGHTING)
        nDam -= 2;
    if (IS_EXHAUSTED(pCh))
        nDam -= 10;
    if (pCh->charge > 0)
        nDam = (pCh->charge * nDam) / skill_table[nSn].wait;
    if (pCh->charge == 1) // Immediately released skill -- deduction
        nDam /= 4;
    // Critical:
    if (number_range(1,50) == 1)
        nDam *= 2;
    nDam = UMAX(1, nDam);
    return nDam;
}

int Armour (CHAR_DATA *pCh, int nSn, bool bPhysical) {
    int nArmour = 25;
    nArmour += get_curr_stat(pCh, bPhysical ? STAT_DEX : STAT_INT);
    nArmour += get_skill(pCh, nSn);
    //nArmour += GET_ARMROLL(pCh);
    nArmour += (pCh->balance - 5) * 2;
    nArmour -= (100 - pCh->nCurPl) / 10;
    if (IS_AFFECTED (pCh, AFF_BLIND))
        nArmour -= 5;
    if (IS_AFFECTED (pCh, AFF_FLYING))
        nArmour += 5;
    if (pCh->stance == STANCE_OFFEN)
        nArmour -= 4;
    else if (pCh->stance == STANCE_DEFEN)
        nArmour += 4;
	if (!IS_AWAKE (pCh))
        nArmour -= 5;
    else if (pCh->position < POS_FIGHTING)
        nArmour -= 2;
    if (IS_EXHAUSTED(pCh))
        nArmour -= 10;
    return nArmour;
}

int Absorb (CHAR_DATA *pCh, int nSn, int nDamType, bool bPhysical) {
    int nAbsorb = 0;
    switch (nDamType) {
        case DAM_NONE:                                         break;
        case DAM_PIERCE: nAbsorb += pCh->armor[AC_PIERCE] / 5; break;
        case DAM_BASH:   nAbsorb += pCh->armor[AC_BASH]   / 5; break;
        case DAM_SLASH:  nAbsorb += pCh->armor[AC_SLASH]  / 5; break;
        default:         nAbsorb += pCh->armor[AC_EXOTIC] / 5; break;
    }
    if (pCh->stance == STANCE_OFFEN)
        nAbsorb -= 4;
    else if (pCh->stance == STANCE_DEFEN)
        nAbsorb += 4;
    if (IS_AFFECTED (pCh, AFF_SANCTUARY))
        nAbsorb += 15;
    if ((IS_AFFECTED (pCh, AFF_PROTECT_EVIL) && IS_EVIL (pCh))
        || (IS_AFFECTED (pCh, AFF_PROTECT_GOOD) && IS_GOOD (pCh)))
        nAbsorb += 10;
	if (!IS_AWAKE (pCh))
        nAbsorb -= 5;
    else if (pCh->position < POS_FIGHTING)
        nAbsorb -= 2;
    return nAbsorb;
}

/*
long long int ApplyMisc (CHAR_DATA *pCh, long long int llValue) {
    llValue = llValue * pCh->balance / 5;
	if (!IS_AWAKE (pCh))
        llValue /= 4;
    else if (pCh->position < POS_FIGHTING)
        llValue /= 2;
    if (IS_EXHAUSTED(pCh))
        llValue /= 50;
    llValue = pCh->nCurPl * llValue / 100;

    return llValue;
}

long long int ApplyCharge (CHAR_DATA *pCh, long long int llValue, int nSn) {
    if (pCh->charge > 0)
        llValue = (pCh->charge * llValue) / skill_table[nSn].wait;
    if (pCh->charge == 1) // Immediately released skill -- deduction
        llValue /= 4;
    return llValue;
}

// Returns a calculated value for chance to hit in melee based on powerlevel,
// stats, skill and other conditions, for some skill
long long int get_attackhit (CHAR_DATA *ch, int sn) {
    long long int value;

    value  = get_curr_stat(ch, STAT_DEX);
    value += get_skill(ch, sn) / 2;
    value += GET_HITROLL(ch);

    if (sn == gsn_heart_shot)
        value /= 2;
    else if (sn == gsn_eye_gouge)
        value /= 10;

    if (IS_AFFECTED (ch, AFF_BLIND))
        value -= value / 5;

    if (IS_AFFECTED (ch, AFF_FLYING))
        value += value / 4;

    if (ch->stance == STANCE_OFFEN)
        value += value / 4;
    else if (ch->stance == STANCE_DEFEN)
        value -= value / 4;
    else if (ch->stance == STANCE_KAMIK)
        value += 1 + get_skill(ch, gsn_kamikaze);

    value = ApplyMisc (ch, value);

    return value;
}


long long int get_attackdam (CHAR_DATA *ch, int sn) {
    long long int value;

    value  = get_curr_stat(ch, STAT_STR);
    value += get_skill(ch, sn) / 2;
    value += GET_DAMROLL(ch);

    if (sn == gsn_throat_shot ||
        sn == gsn_sweep ||
        sn == gsn_eye_gouge)
        value = 1;
    else if (sn == gsn_knee)
        value /= 2;
    else if (sn == gsn_elbow)
        value /= 5;
    else if (sn == gsn_heart_shot)
        value *= 2;

    if (ch->stance == STANCE_OFFEN)
        value += value / 4;
    else if (ch->stance == STANCE_DEFEN)
        value -= value / 4;
    else if (ch->stance == STANCE_KAMIK)
        value += 1 + get_skill(ch, gsn_kamikaze);

    value = ApplyMisc (ch, value);
    value = UMAX(1, value);

    return value;
}

long long int get_attackabsorb (CHAR_DATA *ch, int sn, int dam_type) {
    long long int value = 0;

    switch (dam_type) {
        case DAM_NONE:                                  break;
        case DAM_PIERCE: value += ch->armor[AC_PIERCE] / 5; break;
        case DAM_BASH:   value += ch->armor[AC_BASH] / 5;   break;
        case DAM_SLASH:  value += ch->armor[AC_SLASH] / 5;  break;
        default:         value += ch->armor[AC_EXOTIC] / 5; break;
    }

    if (ch->stance == STANCE_OFFEN)
        value -= value / 4;
    else if (ch->stance == STANCE_DEFEN)
        value += value / 4;
    else if (ch->stance == STANCE_KAMIK)
        value = 0;

    if (IS_AFFECTED (ch, AFF_SANCTUARY))
        value *= 4;

    if ((IS_AFFECTED (ch, AFF_PROTECT_EVIL) && IS_EVIL (ch))
        || (IS_AFFECTED (ch, AFF_PROTECT_GOOD) && IS_GOOD (ch)))
        value += value / 4;

    value = ApplyMisc (ch, value);

    return value;
}

long long int get_attackdodge (CHAR_DATA *ch, int sn) {
    long long int value;

    value  = get_curr_stat(ch, STAT_DEX);
    value += get_skill(ch, sn) / 2;

    if (IS_AFFECTED (ch, AFF_BLIND))
        value -= value / 5;

    if (IS_AFFECTED (ch, AFF_FLYING))
        value += value / 4;
    
	if (IS_AFFECTED (ch, AFF_SANCTUARY))
        value *= 4;

    if (ch->stance == STANCE_OFFEN)
        value -= value / 4;
    else if (ch->stance == STANCE_DEFEN)
        value += value / 4;
    else if (ch->stance == STANCE_KAMIK)
        value -= 3 * value / 4;

    value = ApplyMisc (ch, value);

    return value;
}

long long int get_kihit (CHAR_DATA *ch, int sn) {
    long long int value;

    value  = get_curr_stat(ch, STAT_INT);
    value += get_skill(ch, sn) / 2;
    value += GET_HITROLL(ch);

    if (IS_AFFECTED (ch, AFF_BLIND))
        value -= value / 5;

    if (IS_AFFECTED (ch, AFF_FLYING))
        value += value / 4;

    if (sn == gsn_scattershot)
        value /= 2;

    value = ApplyCharge (ch, value, sn);
	value = ApplyMisc (ch, value);

    return value;
}

long long int get_kidam (CHAR_DATA *ch, int sn) {
    long long int value;

    value  = get_curr_stat(ch,STAT_WIL);
    value += get_skill(ch, sn) / 2;

    if (sn == gsn_power_bomb)
        value *= 4;
    else if (sn == gsn_spirit_bomb || sn == gsn_death_ball)
        value *= 3;
    else if (sn == gsn_finalflash || sn == gsn_kamehameha)
        value *= 2;
    else if (sn == gsn_scattershot || sn == gsn_solarflare)
        value /= 5;

    value = ApplyCharge (ch, value, sn);
    value = ApplyMisc (ch, value);
	value = UMAX(1, value);

    return value;
}

long long int get_kiabsorb (CHAR_DATA *ch, int sn, int dam_type) {
    long long int value = 0;

    // Armor!
    switch (dam_type) {
        case DAM_NONE:                                      break;
        case DAM_PIERCE: value += ch->armor[AC_PIERCE] / 5; break;
        case DAM_BASH:   value += ch->armor[AC_BASH] / 5;   break;
        case DAM_SLASH:  value += ch->armor[AC_SLASH] / 5;  break;
        default:         value += ch->armor[AC_EXOTIC] / 5; break;
    }
    
	if (IS_AFFECTED (ch, AFF_SANCTUARY))
        value *= 4;

    value = ApplyMisc (ch, value);

    return value;
}
*/

