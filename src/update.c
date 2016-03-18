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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>                // For the 'system' call in update(), backup
#include "merc.h"
#include "recycle.h"
#include "interp.h"

/*
 * Local functions.
 */
long long  hit_gain       args ( ( CHAR_DATA * ch ) );
long long  ki_gain        args ( ( CHAR_DATA * ch ) );
void mobile_update  args ( ( void ) );
void weather_update args ( ( void ) );
void underwater_update args( ( void ) );
void char_update    args ( ( void ) );
void obj_update     args ( ( void ) );
void aggr_update    args ( ( void ) );
void wait_update    args ( ( void ) );

/* used for saving */

int save_number = 0;


/*
 * Regeneration stuff.
 */
long long hit_gain (CHAR_DATA * ch)
{
    int gain;

    if (ch->in_room == NULL)
        return 0;

    gain = get_curr_stat (ch, STAT_STR) * HP_STR / 2 + get_curr_stat (ch, STAT_WIL) * HP_WIL / 2;
	if (get_skill(ch,gsn_fast_healing) != 0) {
		gain += 2*get_skill (ch, gsn_fast_healing);
		if (!IS_NPC(ch) && ch->hit < ch->max_hit)
			ImproveSkill (ch, gsn_fast_healing, TRUE, 10, ch->nDifficulty);
	}

    switch (ch->position) {
        default:
            gain /= 4;
            break;
        case POS_SLEEPING:
            break;
        case POS_RESTING:
            gain /= 2;
            break;
        case POS_FIGHTING:
            gain /= 8;
            break;
    }

    gain = gain * UMAX(0, ch->in_room->heal_rate) / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[3] / 100;

	if (IS_AFFECTED (ch, AFF_REGENERATION))
		gain *= 2;

    if (IS_AFFECTED (ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED (ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED (ch, AFF_SLOW))
        gain /= 2;

    return UMAX(1, gain);
}



long long ki_gain (CHAR_DATA * ch)
{
    int gain;

    if (ch->in_room == NULL)
        return 0;

    gain = get_curr_stat(ch, STAT_STR) * KI_STR / 2 + get_curr_stat(ch,STAT_WIL) * KI_WIL / 2;
	if (get_skill(ch,gsn_fast_healing) != 0) {
		gain += 2*get_skill (ch, gsn_fast_healing);
		if (!IS_NPC(ch) && ch->ki < ch->max_ki)
			ImproveSkill (ch, gsn_fast_healing, TRUE, 10, ch->nDifficulty);
	}

    switch (ch->position) {
        default:
            gain /= 4;
            break;
        case POS_SLEEPING:
            break;
        case POS_RESTING:
            gain /= 2;
            break;
        case POS_FIGHTING:
            gain /= 6;
            break;
    }

    gain = gain * UMAX(0, ch->in_room->ki_rate) / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[4] / 100;

    if (IS_AFFECTED (ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED (ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED (ch, AFF_SLOW))
        gain /= 2;

	// Reduce gain for higher powerlevels
    gain = (100 - ch->nCurPl) * gain / 100;

    return UMAX(1, gain);
}


void gain_condition (CHAR_DATA * ch, int iCond, int value)
{
    int condition;

    if (value == 0 || IS_NPC (ch) || ch->level >= LEVEL_IMMORTAL)
        return;

    condition = ch->pcdata->condition[iCond];
    if (condition == -1)
        return;
    ch->pcdata->condition[iCond] = URANGE (0, condition + value, 48);

    if (ch->pcdata->condition[iCond] == 0)
    {
        switch (iCond)
        {
            case COND_DRUNK:
                if (condition != 0)
                    sendch ("You are sober.\n\r", ch);
                break;
        }
    }

    return;
}



/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update (void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    EXIT_DATA *pexit;
    int door;

    /* Examine all mobs. */
    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        ch_next = ch->next;

        if (!IS_NPC (ch) || ch->in_room == NULL
            || IS_AFFECTED (ch, AFF_CHARM))
	    continue;

	if (ch->fighting == NULL && ch->nCurPl != 25) {
	    do_function (ch, &do_power, "base");
	    continue;
	}

	if (ch->in_room->area->empty && !IS_SET (ch->act, ACT_UPDATE_ALWAYS))
        continue;

        /* Examine call for special procedure */
        if (ch->spec_fun != 0)
        {
            if ((*ch->spec_fun) (ch))
                continue;
        }

        if (ch->pIndexData->pShop != NULL)    /* give him some zenni */
            if (ch->zenni * 10 < ch->pIndexData->wealth)
                ch->zenni += ch->pIndexData->wealth * number_range (1, 20) / 500000;

        /*
         * Check triggers only if mobile still in default position
         */
        if (ch->position == ch->pIndexData->default_pos)
        {
            /* Delay */
            if (HAS_TRIGGER_MOB (ch, TRIG_DELAY) && ch->mprog_delay > 0)
            {
                if (--ch->mprog_delay <= 0)
                {
                    p_percent_trigger (ch, NULL, NULL, NULL, NULL, NULL, TRIG_DELAY);
                    continue;
                }
            }
            if (HAS_TRIGGER_MOB (ch, TRIG_RANDOM))
            {
                if (p_percent_trigger (ch, NULL, NULL, NULL, NULL, NULL, TRIG_RANDOM))
                    continue;
            }
        }

        /* That's all for sleeping / busy monster, and empty zones */
        if (ch->position != POS_STANDING)
            continue;

        /* Scavenge */
        if (IS_SET (ch->act, ACT_SCAVENGER)
            && ch->in_room->contents != NULL && number_bits (6) == 0)
        {
            OBJ_DATA *obj;
            OBJ_DATA *obj_best;
            int max;

            max = 1;
            obj_best = 0;
            for (obj = ch->in_room->contents; obj; obj = obj->next_content)
            {
                if (CAN_WEAR (obj, ITEM_TAKE) && can_loot (ch, obj)
                    && obj->cost > max && obj->cost > 0)
                {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if (obj_best)
            {
                obj_from_room (obj_best);
                obj_to_char (obj_best, ch);
                act ("$n gets $p.", ch, obj_best, NULL, TO_ROOM);
            }
        }

        /* Wander */
        if (!IS_SET (ch->act, ACT_SENTINEL)
            && number_bits (3) == 0
            && (door = number_bits (5)) <= 5
            && (pexit = ch->in_room->exit[door]) != NULL
            && pexit->u1.to_room != NULL
            && !IS_SET (pexit->exit_info, EX_CLOSED)
            && !IS_SET (pexit->u1.to_room->room_flags, ROOM_NO_MOB)
            && (!IS_SET (ch->act, ACT_STAY_AREA)
                || pexit->u1.to_room->area == ch->in_room->area)
            && (!IS_SET (ch->act, ACT_OUTDOORS)
                || !IS_SET (pexit->u1.to_room->room_flags, ROOM_INDOORS))
            && (!IS_SET (ch->act, ACT_INDOORS)
                || IS_SET (pexit->u1.to_room->room_flags, ROOM_INDOORS)))
        {
            move_char (ch, door, FALSE);
        }
    }

    return;
}


void underwater_update (void) {
	CHAR_DATA *ch;
	CHAR_DATA *ch_next;

	for (ch = char_list; ch != NULL; ch = ch_next) {
		ch_next = ch->next;

		if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
			&& ch->in_room->sector_type == SECT_UNDERWATER) { 
			if (ch->hit > ch->max_hit / 15) {
				ch->position = POS_RESTING;
				ch->hit -= ch->max_hit / 15;
				sendch("{RYou're drowning!!!{x\n\r", ch);
			}
			else {
				ch->hit = 1;
				raw_kill(ch);
				sendch("{RYou are DEAD!!{x\n\r", ch );
			}
		}
	}
}


/*
 * Update all chars, including mobs.
*/
void char_update (void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *ch_quit;
    int i;

    ch_quit = NULL;

    /* update save counter */
    save_number++;

    if (save_number > 29)
        save_number = 0;

    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        AFFECT_DATA *paf;
        AFFECT_DATA *paf_next;

        ch_next = ch->next;

        if (ch->timer > 30)
            ch_quit = ch;

        if (ch->fuse_count > 0) {
            if (--ch->fuse_count == 0) {
                unfuse (ch);
                continue;
            }
        }

        if (ch->position >= POS_STUNNED)
        {
            /* check to see if we need to go home */
            if (IS_NPC (ch) && ch->zone != NULL
                && ch->zone != ch->in_room->area && ch->desc == NULL
                && ch->fighting == NULL && !IS_AFFECTED (ch, AFF_CHARM)
                && number_percent () < 5)
            {
                act ("$n wanders on home.", ch, NULL, NULL, TO_ROOM);
                extract_char (ch, TRUE);
                continue;
            }

			ch->hit = URANGE(0, ch->hit + hit_gain(ch), ch->max_hit);
			ch->ki = URANGE(0, ch->ki + ki_gain(ch), ch->max_ki);

			// If ki drops too low, start hitting current pl
			if (ch->ki < 1) {
				ch->nCurPl -= 10;
				sendch("Your ki is too low to maintain your current power level!\n\r", ch);
			}
        }

        if (ch->position == POS_RESTING || ch->position == POS_SLEEPING) {
            if (number_range(1,15) == 1)
                rage (ch, ch->trans_count - 1);
            for (i=0; i<MAX_TRANS; ++i)
                ch->trans_heal_count[i] = UMIN(ch->trans_heal_count[i]+1, 5);
        }

        if (ch->position == POS_STUNNED || ch->position == POS_UNCONSCIOUS)
            update_pos (ch);

        // Reward counter
        if (!IS_NPC (ch) && ch->pcdata->nReward > 0)
            --ch->pcdata->nReward;

        if (!IS_NPC (ch) && ch->pcdata->nTrainCount > 0)
            --ch->pcdata->nTrainCount;
        if (!IS_NPC (ch) && ch->pcdata->nTeachCount > 0)
            --ch->pcdata->nTeachCount;
        if (!IS_NPC (ch) && ch->pcdata->nChaosCount > 0)
            ch->pcdata->nChaosCount -= 8;
            
        if (!IS_NPC (ch) && get_skill(ch, gsn_sense) > 0)
            ImproveSkill (ch, gsn_sense, TRUE, 10, ch->nDifficulty);

        // Teaching
		if (!IS_NPC (ch) && ch->pcdata->nTeachSn > -1) {
            CHAR_DATA *pStudent;
			// Check validity
			if (ch->position < POS_RESTING
				|| ch->position == POS_FIGHTING) {
				ch->pcdata->nTeachSn = -1;
				break;
			}
			for (pStudent = ch->in_room->people; pStudent; pStudent = pStudent->next_in_room) {
                if (!IS_NPC(pStudent) && pStudent->pcdata->pTeacher == ch) {
					ImproveSkill (ch, gsn_teaching, TRUE, UMIN(-3 + ch->pcdata->nTeachCount / 20, 0), ch->nDifficulty);
					ImproveStat (ch, STAT_INT, TRUE, UMIN(-3 + ch->pcdata->nTeachCount / 20, 0), ch->nDifficulty);
					break;
				}
			}
            ch->pcdata->nTeachCount += 2;
            act ("$n teaches the skill of $t.", ch, skill_table[ch->pcdata->nTeachSn].name, NULL, TO_ROOM);
			act ("You teach the skill of $t.", ch, skill_table[ch->pcdata->nTeachSn].name, NULL, TO_CHAR);
		}
		// Learning
		else if (!IS_NPC (ch) && ch->pcdata->pTeacher) {
            CHAR_DATA *pTeacher = ch->pcdata->pTeacher;
			// Check validity
			if (pTeacher->pcdata == NULL) { 
				ch->pcdata->pTeacher = NULL;
				break;
			}
			else if (pTeacher->in_room != ch->in_room
				     || ch->position < POS_RESTING
					 || ch->position == POS_FIGHTING) {
				ch->pcdata->pTeacher = NULL;
				break;
			}
		    if (get_skill(pTeacher, pTeacher->pcdata->nTeachSn) < get_skill(ch, pTeacher->pcdata->nTeachSn)) {
				act ("You are more skilled at $t than $N, and thus stop listening.",
					 ch, skill_table[pTeacher->pcdata->nTeachSn].name, pTeacher, TO_CHAR);
				ch->pcdata->pTeacher = NULL;
				return;
			}
            ch->pcdata->nTeachCount  += 2;
			ImproveSkill (ch, pTeacher->pcdata->nTeachSn, TRUE, UMIN(-3 + ch->pcdata->nTeachCount / 20, 0), pTeacher->nDifficulty);
			ImproveStat (ch, STAT_INT, TRUE, UMIN(-3 + ch->pcdata->nTeachCount / 20, 0), pTeacher->nDifficulty);
		}

        if (!IS_NPC (ch)) {
            int i;
            for (i = 0; i < MAX_SKILL; ++i)
                ch->pcdata->nSkillTick[i] = 0;
            for (i = 0; i < MAX_STATS; ++i)
                ch->pcdata->nStatTick[i] = 0;
            if (ch->level < LEVEL_IMMORTAL) {
                OBJ_DATA *obj;

                if ((obj = get_eq_char (ch, WEAR_LIGHT)) != NULL
                    && obj->item_type == ITEM_LIGHT && obj->value[2] > 0)
                {
                    if (--obj->value[2] == 0 && ch->in_room != NULL)
                    {
                        --ch->in_room->light;
                        act ("$p goes out.", ch, obj, NULL, TO_ROOM);
                        act ("$p flickers and goes out.", ch, obj, NULL, TO_CHAR);
                        extract_obj (obj);
                    }
                    else if (obj->value[2] <= 5 && ch->in_room != NULL)
                        act ("$p flickers.", ch, obj, NULL, TO_CHAR);
                }

                if (IS_IMMORTAL (ch))
                    ch->timer = 0;

                if (++ch->timer >= 12)
                {
                    if (ch->was_in_room == NULL && ch->in_room != NULL)
                    {
                        ch->was_in_room = ch->in_room;
                        if (ch->fighting != NULL)
                            stop_fighting (ch, TRUE);
                        act ("$n disappears into the void.",
                            ch, NULL, NULL, TO_ROOM);
                        sendch ("You disappear into the void.\n\r", ch);
                        save_char_obj (ch);
                        char_from_room (ch);
                        char_to_room (ch, get_room_index (ROOM_VNUM_LIMBO));
                    }
                }

                gain_condition (ch, COND_DRUNK, -1);
            }
        }

        for (paf = ch->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            if (paf->duration > 0)
            {
                paf->duration--;
                //if (number_range (0, 4) == 0 && paf->skill_lvl > 0)
                //    --paf->skill_lvl;    /* spell strength fades with time */
            }
            else if (paf->duration < 0);
            else
            {
                if (paf_next == NULL
                    || paf_next->type != paf->type || paf_next->duration > 0)
                {
                    if (paf->type > 0 && skill_table[paf->type].msg_off)
                    {
                        sendch (skill_table[paf->type].msg_off, ch);
                        sendch ("\n\r", ch);
                    }
                }

                affect_remove (ch, paf);
            }
        }

        /*
         * Careful with the damages here,
         *   MUST NOT refer to ch after damage taken,
         *   as it may be lethal damage (on NPC).
         */
/* removed
        if (is_affected (ch, gsn_plague) && ch != NULL)
        {
            AFFECT_DATA *af, plague;
            CHAR_DATA *vch;
            int dam;

            if (ch->in_room == NULL)
                continue;

            act ("$n writhes in agony as plague sores erupt from $s skin.",
                 ch, NULL, NULL, TO_ROOM);
            sendch ("You writhe in agony from the plague.\n\r", ch);
            for (af = ch->affected; af != NULL; af = af->next)
            {
                if (af->type == gsn_plague)
                    break;
            }

            if (af == NULL)
            {
                REMOVE_BIT (ch->affected_by, AFF_PLAGUE);
                continue;
            }

            if (af->level == 1)
                continue;

            plague.where = TO_AFFECTS;
            plague.type = gsn_plague;
            plague.level = af->level - 1;
            plague.duration = number_range (1, 2 * plague.level);
            plague.location = APPLY_STR;
            plague.modifier = -5;
            plague.bitvector = AFF_PLAGUE;

            for (vch = ch->in_room->people; vch != NULL;
                 vch = vch->next_in_room)
            {
                if (!saves_spell (plague.level - 2, vch, DAM_DISEASE)
                    && !IS_IMMORTAL (vch)
                    && !IS_AFFECTED (vch, AFF_PLAGUE) && number_bits (4) == 0)
                {
                    sendch ("You feel hot and feverish.\n\r", vch);
                    act ("$n shivers and looks very ill.", vch, NULL, NULL,
                         TO_ROOM);
                    affect_join (vch, &plague);
                }
            }

            dam = UMIN (ch->level, af->level / 5 + 1);
            ch->ki -= dam;
            damage (ch, ch, dam, gsn_plague, DAM_DISEASE, FALSE);
        }
        else if (IS_AFFECTED (ch, AFF_POISON) && ch != NULL
                 && !IS_AFFECTED (ch, AFF_SLOW))
        {
            AFFECT_DATA *poison;

            poison = affect_find (ch->affected, gsn_poison);

            if (poison != NULL)
            {
                act ("$n shivers and suffers.", ch, NULL, NULL, TO_ROOM);
                sendch ("You shiver and suffer.\n\r", ch);
                damage (ch, ch, poison->level / 10 + 1, gsn_poison,
                        DAM_POISON, FALSE);
            }
        }
*/
        if (ch->position == POS_INCAP && number_range (0, 1) == 0)
            damage (ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, FALSE);
        else if (ch->position == POS_MORTAL)
            damage (ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, FALSE);
	}


    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for (ch = char_list; ch != NULL; ch = ch_next)
    {
    	/*
    	 * Edwin's fix for possible pet-induced problem
    	 * JR -- 10/15/00
    	 */
    	if (!IS_VALID(ch))
    	{
        	logstr (LOG_BUG, "update_char: Trying to work with an invalidated character.\n",0);
        	break;
     	}

        ch_next = ch->next;

        if (ch->desc != NULL && ch->desc->descriptor % 30 == save_number && !IS_FUSED(ch))
            save_char_obj (ch);

        if (ch == ch_quit)
        {
            do_function (ch, &do_quit, "");
        }
    }

    return;
}




/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update (void)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;

    for (obj = object_list; obj != NULL; obj = obj_next)
    {
        CHAR_DATA *rch;
        char *message;

        obj_next = obj->next;

        /* go through affects and decrement */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            if (paf->duration > 0)
            {
                paf->duration--;
                if (number_range (0, 4) == 0 && paf->skill_lvl > 0)
                    --paf->skill_lvl;    /* spell strength fades with time */
            }
            else if (paf->duration < 0);
            else
            {
                if (paf_next == NULL
                    || paf_next->type != paf->type || paf_next->duration > 0)
                {
                    if (paf->type > 0 && skill_table[paf->type].msg_obj)
                    {
                        if (obj->carried_by != NULL)
                        {
                            rch = obj->carried_by;
                            act (skill_table[paf->type].msg_obj,
                                 rch, obj, NULL, TO_CHAR);
                        }
                        if (obj->in_room != NULL
                            && obj->in_room->people != NULL)
                        {
                            rch = obj->in_room->people;
                            act (skill_table[paf->type].msg_obj,
                                 rch, obj, NULL, TO_ALL);
                        }
                    }
                }

                affect_remove_obj (obj, paf);
            }
        }


        if (obj->timer <= 0 || --obj->timer > 0)
            continue;

		/*
		 * Oprog triggers!
		 */
		if ( obj->in_room || (obj->carried_by && obj->carried_by->in_room))
		{
			if ( HAS_TRIGGER_OBJ( obj, TRIG_DELAY )
			  && obj->oprog_delay > 0 )
			{
				if ( --obj->oprog_delay <= 0 )
				p_percent_trigger( NULL, obj, NULL, NULL, NULL, NULL, TRIG_DELAY );
			}
			else if ( ((obj->in_room && !obj->in_room->area->empty)
	    		|| obj->carried_by ) && HAS_TRIGGER_OBJ( obj, TRIG_RANDOM ) )
				p_percent_trigger( NULL, obj, NULL, NULL, NULL, NULL, TRIG_RANDOM );
		 }
		/* Make sure the object is still there before proceeding */
		if ( !obj )
			continue;

        switch (obj->item_type)
        {
            default:
                message = "$p crumbles into dust.";
                break;
            case ITEM_FOUNTAIN:
                message = "$p dries up.";
                break;
            case ITEM_CORPSE_NPC:
                message = "$p decays into dust.";
                break;
            case ITEM_CORPSE_PC:
                message = "$p decays into dust.";
                break;
            case ITEM_FOOD:
                message = "$p decomposes.";
                break;
            case ITEM_POTION:
                message = "$p has evaporated from disuse.";
                break;
            case ITEM_PORTAL:
                message = "$p fades out of existence.";
                break;
            case ITEM_CONTAINER:
                if (CAN_WEAR (obj, ITEM_WEAR_FLOAT))
                    if (obj->contains)
                        message =
                            "$p flickers and vanishes, spilling its contents on the floor.";
                    else
                        message = "$p flickers and vanishes.";
                else
                    message = "$p crumbles into dust.";
                break;
        }

        if (obj->carried_by != NULL)
        {
            if (IS_NPC (obj->carried_by)
                && obj->carried_by->pIndexData->pShop != NULL)
                obj->carried_by->zenni += obj->cost / 5;
            else
            {
                act (message, obj->carried_by, obj, NULL, TO_CHAR);
                if (obj->wear_loc == WEAR_FLOAT)
                    act (message, obj->carried_by, obj, NULL, TO_ROOM);
            }
        }
        else if (obj->in_room != NULL && (rch = obj->in_room->people) != NULL)
        {
            if (!(obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
                  && !CAN_WEAR (obj->in_obj, ITEM_TAKE)))
            {
                act (message, rch, obj, NULL, TO_ROOM);
                act (message, rch, obj, NULL, TO_CHAR);
            }
        }

        if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT)
            && obj->contains)
        {                        /* save the contents */
            OBJ_DATA *t_obj, *next_obj;

            for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
            {
                next_obj = t_obj->next_content;
                obj_from_obj (t_obj);

                if (obj->in_obj)    /* in another object */
                    obj_to_obj (t_obj, obj->in_obj);

                else if (obj->carried_by)    /* carried */
                    if (obj->wear_loc == WEAR_FLOAT)
                        if (obj->carried_by->in_room == NULL)
                            extract_obj (t_obj);
                        else
                            obj_to_room (t_obj, obj->carried_by->in_room);
                    else
                        obj_to_char (t_obj, obj->carried_by);

                else if (obj->in_room == NULL)    /* destroy it */
                    extract_obj (t_obj);

                else            /* to a room */
                    obj_to_room (t_obj, obj->in_room);
            }
        }

        extract_obj (obj);
    }

    return;
}



/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update (void)
{
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;

    for (wch = char_list; wch != NULL; wch = wch_next)
    {
		wch_next = wch->next;
        if (IS_NPC (wch)
            || wch->level >= LEVEL_IMMORTAL
            || wch->in_room == NULL || wch->in_room->area->empty)
            continue;

        for (ch = wch->in_room->people; ch != NULL; ch = ch_next)
        {
            int count;

            ch_next = ch->next_in_room;

            if (!IS_NPC (ch)
                || !IS_SET (ch->act, ACT_AGGRESSIVE)
                || IS_SET (ch->in_room->room_flags, ROOM_SAFE)
                || IS_AFFECTED (ch, AFF_CALM)
                || ch->fighting != NULL
                || IS_AFFECTED (ch, AFF_CHARM)
                || !IS_AWAKE (ch)
                || (IS_SET (ch->act, ACT_WIMPY) && IS_AWAKE (wch))
                || !can_see (ch, wch)
                || number_bits (1) == 0)
                continue;

            /*
             * Ok we have a 'wch' player character and a 'ch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count = 0;
            victim = NULL;
            for (vch = wch->in_room->people; vch != NULL; vch = vch_next)
            {
                vch_next = vch->next_in_room;

                if (!IS_NPC (vch)
                    && vch->level < LEVEL_IMMORTAL
                    && ch->level >= vch->level - 5
                    && (!IS_SET (ch->act, ACT_WIMPY) || !IS_AWAKE (vch))
                    && can_see (ch, vch))
                {
                    if (number_range (0, count) == 0)
                        victim = vch;
                    count++;
                }
            }

            if (victim == NULL)
                continue;

            begin_combat (ch, victim);
        }
    }

    return;
}


/*
 * Decrements the wait and skill wait on each character. Sets
 * the skill off if the wait is over
 */
void wait_update (void) {
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;

    for (ch = char_list; ch != NULL; ch = ch_next)
    {
		ch_next = ch->next;

		// Decrement the wait
		if (ch->wait > 0) {
			--ch->wait;
			if (ch->wait == 0) {
				if (!ch->short_wait)
					sendch ("You are no longer waiting.\n\r", ch);
				if (IS_SET(ch->act, PLR_AUTOATTACK) && ch->fighting) {
					if (ch->cmd_buf[0] != '\0') {
						interpret (ch, ch->cmd_buf);
						ch->cmd_buf[0] = '\0';
					}
					else
						do_function (ch, &do_attack, "");
				}
			}
		}

		// Decrement wait for skills
		if (ch->wait_skill > 0) {
           
			if (--ch->wait_skill == 0) {
				if (ch->wait_skill_sn > 0 && skill_table[ch->wait_skill_sn].skill_fun) {
					// Delay over, fire the skill
					if (skill_table[ch->wait_skill_sn].msg_delay1)
						act(skill_table[ch->wait_skill_sn].msg_delay1,ch,NULL,NULL,TO_CHAR);
					if (skill_table[ch->wait_skill_sn].msg_delay2)
						act(skill_table[ch->wait_skill_sn].msg_delay2,ch,NULL,NULL,TO_ROOM);
					(*skill_table[ch->wait_skill_sn].skill_fun) (ch, ch->wait_skill_vo, ch->wait_skill_target);
					ch->wait_skill = 0;
					ch->wait_skill_vo = NULL;
					ch->wait_skill_target = TARGET_NONE;
					ch->wait_skill_sn = 0;
				}
			}

			else if (ch->wait_skill_sn > 0) {
				if (ch->wait_skill_target == TARGET_CHAR
					&& ch->wait_skill_vo 
					&& !get_char_room(ch, NULL, ((CHAR_DATA*)ch->wait_skill_vo)->name)) {
					sendch("Your target has left the room.\n\r",ch);
					ch->wait_skill_sn = 0;
					ch->wait_skill_vo = NULL;
					ch->wait_skill_target = TARGET_NONE;
					ch->wait_skill = 0;
	    		}
			}
		}


		if (ch->charge > 0 && ch->wait_skill_sn > 0) {
			if (ch->wait_skill_target == TARGET_CHAR
				&& ch->wait_skill_vo
				&& !get_char_room(ch, NULL, ((CHAR_DATA*)ch->wait_skill_vo)->name)) {
				sendch("Your target has left the room.\n\r",ch);
				ch->charge = 0;
				ch->wait_skill_sn = 0;
				ch->wait_skill_vo = NULL;
				ch->wait_skill_target = 0;
				ch->wait_skill = 0;
				continue;
			}

			++ch->charge;

			// Loss ki every second
			if (ch->charge % PULSE_SECOND == 0)
				ki_loss(ch, skill_table[ch->wait_skill_sn].ki_mod);
			// Check if player is exhausted
			if (IS_EXHAUSTED(ch)) {
				do_function (ch, &do_release, "");
				continue;
			}
			
			if (ch->charge == skill_table[ch->wait_skill_sn].wait)
				sendch ("You may now release.\n\r", ch);

			// Every fifteen seconds, show charge message
			if (ch->charge % (15*PULSE_SECOND) == 0) {
				act("You continue to charge.",ch,NULL,NULL,TO_CHAR);
				act("$n continues to charge.",ch,NULL,NULL,TO_ROOM);
			}
		}

	}
}

int pulse_backup = 3600;

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler (void)
{
    static int pulse_area = 0;
    static int pulse_mobile = 0;
    static int pulse_violence = 0;
    static int pulse_point = 0;
	static int pulse_underwater = 0;


	if (--pulse_backup <= 0) {
		pulse_backup = 21600; // 21600 seconds = 6 hours.
		wiznet("Backing up...",NULL,NULL,WIZ_SECURE,0,IMMORTAL);
		system ("cd ../../ && tar -zcf dbz-`date -I`-auto.tar.gz dbz && mv dbz*auto.tar.gz backup");
		wiznet("Backup complete",NULL,NULL,WIZ_SECURE,0,IMMORTAL);
	}

	if (--pulse_area <= 0)
    {
        pulse_area = PULSE_AREA;
        /* number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 ); */
        area_update ();
    }

    if (--pulse_underwater <= 0)
    {
        pulse_underwater = PULSE_UNDERWATER;
        underwater_update ();
    }

    if (--pulse_mobile <= 0)
    {
        pulse_mobile = PULSE_MOBILE;
        mobile_update ();
    }

    if (--pulse_violence <= 0)
    {
        pulse_violence = PULSE_VIOLENCE;
        violence_update ();
    }

    if (--pulse_point <= 0)
    {
        wiznet ("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
        pulse_point = PULSE_TICK;
/* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */
        weather_update ();
        char_update ();
        obj_update ();
    }

    aggr_update ();
    wait_update ();
    tail_chain ();
    return;
}
