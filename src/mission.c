/* * * * * * * * * * * * * *    mission.c   * * * * * * * * * * * * * * *
*                                                                       *
*    Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,    *
*   Michael Seifert, Hans-Henrik Stæfeldt, Tom Madsen and Katja Nyboe   *
*                                                                       *
*         Merc Diku Mud improvements copyright (C) 1992, 1993 by        *
*            Michael Chastain, Michael Quan, and Mitchell Tse           *
*                                                                       *
*         ROM 2.4 is copyright 1993-1998 Russ Taylor                    *
*         ROM has been brought to you by the ROM consortium:            *
*               Russ Taylor (rtaylor@hypercube.org)                     *
*               Gabrielle Taylor (gtaylor@hypercube.org)                *
*               Brian Moore (zump@rom.org)                              *
*                                                                       *
*    mission.c and associated patches copyright 2001 by Sandi Fallon    *
*                                                                       *
*  In order to use any part of this ROM Merc Diku code you must comply  *
*  the original Diku license in 'license.doc' as well the Merc license  *
*  in 'license.txt' and also the ROM license in 'rom.license', each to  *
*  be found in doc/. Using the reward.c code without conforming to the  *
*  requirements of each of these documents is violation of any and all  *
*  applicable copyright laws. In particular, you may not remove any of  *
*  these copyright notices or claim other's work as your own.           *
*                                                                       *
*    Much time and thought has gone into this software you are using.   *
*            We hope that you share your improvements, too.             *
*                   "What goes around, comes around."                   *
*                                                                       *
* * tabs = 4 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

// Perhaps the search could be based upon the indexes of the mobs.
// By picking a random mob based on index, each mob has an equal
// chance to be selected. The search would also be much faster.
// Once the mob has been selected, it only takes a small search
// through the world to find the required one.
// Index the index of mobs using a hash table, the key being
// the difficulty of the mob. Each index should link to an actual
// loaded mob, and each mob should have a pointer to the next
// of the same mob


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/* imported stuff */
DECLARE_DO_FUN (do_say);
DECLARE_DO_FUN (do_emote);

// Locals
CHAR_DATA *FindMissionAssigner args( ( CHAR_DATA *pCh ) );
CHAR_DATA *FindMissionRewarder args( ( CHAR_DATA *pCh ) );
CHAR_DATA *GetGuard            args( ( CHAR_DATA *pCh ) );
void       GenMission          args( ( CHAR_DATA *ch, CHAR_DATA *pAssign ) );

// Find a character in the current room of the character that can
// assign missions
CHAR_DATA *FindMissionAssigner (CHAR_DATA *pCh) {
	CHAR_DATA *pACh;

	for (pACh = pCh->in_room->people; pACh; pACh = pACh->next_in_room)
		if (IS_NPC(pACh) && IS_SET(pACh->act, ACT_MISSION))
            break;
	if (pACh == NULL)
		return NULL;
	if(!can_see (pACh, pCh)) {
		do_say (pACh, "I can't see anyone.");
		return NULL;
	}
	if((IS_NPC(pCh) || IS_SET(pCh->act, ACT_PET))) {
		do_say (pACh, "I can't send you on a mission!");
		return NULL;
	}
	return pACh;
}

// Find a character in the current room of the character that will
// give the reward for a mission
CHAR_DATA *FindMissionRewarder (CHAR_DATA *pCh) {
	CHAR_DATA *pRCh;

	for (pRCh = pCh->in_room->people; pRCh; pRCh = pRCh->next_in_room)
		if (IS_NPC(pRCh) && pRCh->pIndexData->vnum == pCh->pcdata->rewarder)
            break;
	if (pRCh == NULL)
		return NULL;
	if (!can_see (pRCh, pCh)) {
		do_say (pRCh, "I can't see anyone.");
		return NULL;
	}
	if((IS_NPC (pCh) || IS_SET(pCh->act, ACT_PET))) {
		do_say (pRCh, "I can't reward you!");
		return NULL;
	}
	return pRCh;
}
		
CHAR_DATA *GetGuard (CHAR_DATA *pCh) {
	CHAR_DATA *pGuard;
	for (pGuard = pCh->in_room->people; pGuard; pGuard = pGuard->next_in_room) {
		if (IS_NPC(pGuard) 
			&& !IS_AFFECTED(pGuard, AFF_CHARM)
			&& !IS_SET(pGuard->act, ACT_MISSION))
			break;
	}
	return pGuard;
}

// The magic number 720 is the number of seconds in 12 minutes
void ResetHunt (CHAR_DATA *pCh) {
	int nDiff = 0;

	if (IS_NPC(pCh))
		return;
	// If less than 12 minutes have passed
	if ((nDiff = (current_time - pCh->pcdata->hunt_time)) < 720) {
		if (nDiff < 0)
			pCh->pcdata->recovery = current_time + 720;
		else
			pCh->pcdata->recovery = current_time + 720 - nDiff;
	}
	// Otherwise, too late, reset
	else {
		pCh->pcdata->recovery  = 0;
		pCh->pcdata->hunt_time = 0;
	}
	pCh->pcdata->rewarder   = 0;
	pCh->pcdata->reward_mob = 0;
	pCh->pcdata->reward_obj = 0;
	pCh->pcdata->nMissionType = MISSION_NONE;
	pCh->pcdata->bMissionSuccess = FALSE;
}

/* adapted from Furey's mfind() code */
// Finds a mission for ch, pAssign is the character assigning the mission
void GenMission (CHAR_DATA *ch, CHAR_DATA *pAssign) {
	char 			buf[MAX_STRING_LENGTH];
	char 			first[MAX_INPUT_LENGTH];
	char const 		*prep;
	ROOM_INDEX_DATA *room;
	CHAR_DATA		*guard;
	int vnum 		= 0;
	int nType;
    
	nType = number_range (1, 2);
    // Maybe change type depending on alignment?
	if (nType == 1)
		nType = MISSION_OBJ;
	else
		nType = MISSION_SLAY;
	
    if (nType == MISSION_OBJ) {
		CHAR_DATA *target = NULL;
		MOB_INDEX_DATA *pIndex;
		MOB_INDEX_DATA *pList[250];
		OBJ_DATA *pObjList[1500], *pObj;
		int i, nCount = 0, nObjCount = 0;

		// Populate the list
		for (vnum = 0; vnum < top_vnum_mob; ++vnum) {
			if ((pIndex = get_mob_index (vnum)) == NULL)
				continue;
			if (!pIndex->pCreated) // No mobs of this vnum loaded
				continue;
			target = pIndex->pCreated;
			if ((IS_NEUTRAL(target) || (ch->alignment > 0 ? IS_EVIL(target) : IS_GOOD(target)))
				&& ch->nDifficulty + (ch->nDifficulty / 2) >= target->nDifficulty
				&& ch->nDifficulty - (ch->nDifficulty / 2) <= target->nDifficulty
				&& !IS_SET(target->act, ACT_IS_HEALER)
				&& !IS_SET(target->act, ACT_IS_CHANGER)
				&& !IS_SET(target->imm_flags, IMM_SUMMON)
				&& !IS_SET(target->affected_by, AFF_CHARM)
				&& target->carrying) { // If the first isn't carrying anything, assume they all aren't
				if (nCount >= 250)
					pList[number_range(0,249)] = pIndex;
				else
				    pList[nCount++] = pIndex;
			}
		}
		if (nCount == 0) {
			sprintf( buf, "Right now, %s, there are no missions worthy of your talents.", ch->name );
			do_say (pAssign, buf);
			return;
		}
		// Now make a list of objects, based on the actual (not indexes) objects.
		// It really should only add one object of each index...
        for (i = 0; i < nCount; ++i) {
			for (target = pList[i]->pCreated; target; target = target->pNextSame) {
				for (pObj = target->carrying; pObj; pObj = pObj->next_content) {
					if (!IS_SET(pObj->extra_flags, ITEM_INVENTORY)
						&& !IS_SET(pObj->extra_flags, ITEM_ROT_DEATH)
						&& IS_SET(pObj->wear_flags, ITEM_TAKE)
						&& pObj->wear_loc != WEAR_NONE) {
						if (nObjCount >= 1500)
							pObjList[number_range(0,1499)] = pObj;
						else
							pObjList[nObjCount++] = pObj;
						break;
					}
				}
			}
		}
		if (nObjCount == 0) {
			sprintf( buf, "Right now, %s, there are no missions worthy of your talents.", ch->name );
			do_say (pAssign, buf);
			return;
		}
		pObj = pObjList[number_range(0, nObjCount - 1)]; // Random object
		target = pObj->carried_by;

/*
		bool obj_found	= FALSE;
	    CHAR_DATA *victim = NULL;
		OBJ_DATA *obj = NULL;

		while (!obj_found) {
			vnum = number_range (0, 30000);
			if (++nFailCount > 1000000)
				break;
			if ((room = get_room_index(vnum)) != NULL) {
				// Go through the mobs in the room and check for aggies
				//aggie_found = FALSE;
				//for (victim = room->people; victim; victim = victim->next_in_room) {
				//	if (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE) && victim->level > ch->level) {
				//		aggie_found = TRUE;
				//		break;
				//	}
				//}
				//if (!aggie_found) {
				// go through the mobs in the room and check their level and flags
				for (victim = room->people; victim; victim = victim->next_in_room) {
					if (IS_NPC(victim)
						&& (IS_NEUTRAL(victim) || (ch->alignment > 0 ? IS_EVIL(victim) : IS_GOOD(victim)))
						&& ch->nDifficulty + (ch->nDifficulty / 2) >= victim->nDifficulty
						&& ch->nDifficulty - (ch->nDifficulty / 2) <= victim->nDifficulty
						&& !IS_SET(victim->act, ACT_IS_HEALER)
						&& !IS_SET(victim->act, ACT_IS_CHANGER)
						&& !IS_SET(victim->imm_flags, IMM_SUMMON)
						&& !IS_SET(victim->affected_by, AFF_CHARM)) {
						obj_found = FALSE;
						for (obj = victim->carrying; obj != NULL; obj = obj->next_content) {
							// Make sure the object is being worn, since it's
							// too hard to find it otherwise
							if (!IS_SET(obj->extra_flags, ITEM_INVENTORY)
								&& !IS_SET(obj->extra_flags, ITEM_ROT_DEATH)
								&& IS_SET(obj->wear_flags, ITEM_TAKE)
								&& obj->wear_loc != WEAR_NONE) {
								obj_found = TRUE;
								break;
							}
						}
					}
					if (obj_found)
						break;
				}
			}
		}

		if (!obj_found) {
			sprintf( buf, "Right now, %s, there are no missions worthy of your talents.", ch->name );
			do_say (pAssign, buf);
			return;
		}
*/
		if ((room = get_room_index( vnum ) ) == NULL || pObj == NULL || target == NULL) {
			logstr (LOG_BUG, "GenMission: found NULL in MISSION_OBJ");
			return;
		}

		switch (number_range(1,4)) {
			case 1:
				sprintf (buf,
					"%s, we know %s stole %s. Find the thief, get %s, bring it to me, and I will see you are rewarded!",
					ch->name, target->short_descr, pObj->short_descr, pObj->short_descr );
				break;
			case 2:
				sprintf (buf,
					"%s has been reported lost, %s, but we know who took it. Find %s and get %s, bring it to me, and I will direct you to someone with a reward for its return.",
					capitalize(pObj->short_descr), ch->name, target->short_descr, pObj->short_descr );
				break;
			case 3:
				sprintf (buf,
					"%s is gone! %s, rumor among the %ss is that %s stole it. Slay %s and get %s, bring it to me, and I'll see to it you are rewarded!",
					capitalize(pObj->short_descr), ch->name, race_table[target->race].name, target->short_descr, target->short_descr, pObj->short_descr );
				break;
			default:
				sprintf (buf, 
					"%s is gone! %s, just before %s died, our %s informant said %s stole it. Slay %s and get %s, bring it to me, and I'll see to it you are rewarded!",
					capitalize(pObj->short_descr), ch->name, target->sex == 1 ? "he" : "she", race_table[target->race].name, target->short_descr, target->short_descr, pObj->short_descr );
				break;
		}
		do_say (pAssign, buf);

		if ((guard = GetGuard(ch)) == NULL) 
			guard = pAssign;

		do_emote (guard, "points to a map.");

		one_argument (room->name, first);
		if (is_exact_name(first, "on in at by near inside under within approaching standing climbing swimming sailing walking sliding floating close lost along going over near very atop beneath before below between"))
			prep = "";
		else
			prep = "at ";
		sprintf (buf, "I hear that %s was recently seen %s%s in %s.",
			target->short_descr, prep, decap(target->in_room->name), room->area->name);
		do_say (guard, buf);

		ch->pcdata->reward_mob = target->pIndexData->vnum;
		ch->pcdata->reward_obj = pObj->pIndexData->vnum;
	}
	else {
		//bool target_found	= FALSE;
		CHAR_DATA *target = NULL;
		MOB_INDEX_DATA *pIndex;
		int i, num, nCount = 0;
		MOB_INDEX_DATA *pList[250];

		// Populate the list
		for (vnum = 0; vnum < top_vnum_mob; ++vnum) {
			if ((pIndex = get_mob_index (vnum)) == NULL)
				continue;
			if (!pIndex->pCreated) // No mobs of this vnum loaded
				continue;
			target = pIndex->pCreated;
			if ((IS_NEUTRAL(target) || (ch->alignment > 0 ? IS_EVIL(target) : IS_GOOD(target)))
				&& ch->nDifficulty + (ch->nDifficulty / 2) >= target->nDifficulty
				&& ch->nDifficulty - (ch->nDifficulty / 2) <= target->nDifficulty
				&& !IS_SET(target->act, ACT_IS_HEALER)
				&& !IS_SET(target->act, ACT_IS_CHANGER)
				&& !IS_SET(target->imm_flags, IMM_SUMMON)
				&& !IS_SET(target->affected_by, AFF_CHARM)) {
				if (nCount >= 250)
					pList[number_range(0,249)] = pIndex;
				else
				    pList[nCount++] = pIndex;
			}
		}
		if (nCount == 0) {
			sprintf( buf, "Right now, %s, there are no missions worthy of your talents.", ch->name );
			do_say (pAssign, buf);
			return;
		}
		pIndex = pList[number_range(0, nCount - 1)]; // Random index
		// Find a random mob of this index
		target = pIndex->pCreated;
		num = number_range (1, pIndex->count);
		for (i = 0; i < num; ++i) {
            if (target->pNextSame)
				target = target->pNextSame;
			else
				break;
		}

		// Locate an index
/*
		while (!target_found) {
			if (++nFailCount > 1000000)
				break;
            vnum = number_range (0, 30000);
			pIndex = get_mob_index (vnum);
			if (!pIndex)
				continue;
			if (!pIndex->pCreated) // No mobs of this vnum loaded
				continue;
			// Go to a random mob
			num = number_range (1, pIndex->count);
			target = pIndex->pCreated;
			for (i = 0; i < num; ++i) {
                if (target->pNextSame)
					target = target->pNextSame;
				else
					break;
			}			
			if (IS_NPC(target)
				&& (IS_NEUTRAL(target) || (ch->alignment > 0 ? IS_EVIL(target) : IS_GOOD(target)))
				&& ch->nDifficulty + (ch->nDifficulty / 2) >= target->nDifficulty
				&& ch->nDifficulty - (ch->nDifficulty / 2) <= target->nDifficulty
				&& !IS_SET(target->act, ACT_IS_HEALER)
				&& !IS_SET(target->act, ACT_IS_CHANGER)
				&& !IS_SET(target->imm_flags, IMM_SUMMON)
				&& !IS_SET(target->affected_by, AFF_CHARM)) {
				target_found = TRUE;
				break;
			}
		}
*/
		/*
		while (!target_found) {
			vnum = number_range(0, 30000);
			if (++nFailCount > 1000000)
				break;
			if ((room = get_room_index (vnum)) != NULL) {
				for (target = room->people; target != NULL; target = target->next_in_room) {
					if (IS_NPC(target)
						&& (IS_NEUTRAL(target) || (ch->alignment > 0 ? IS_EVIL(target) : IS_GOOD(target)))
						&& ch->nDifficulty + (ch->nDifficulty / 2) >= target->nDifficulty
						&& ch->nDifficulty - (ch->nDifficulty / 2) <= target->nDifficulty
						&& !IS_SET(target->act, ACT_IS_HEALER)
						&& !IS_SET(target->act, ACT_IS_CHANGER)
						&& !IS_SET(target->imm_flags, IMM_SUMMON)
						&& !IS_SET(target->affected_by, AFF_CHARM)) {
						target_found = TRUE;
						break;
					}
				}
			}
		}

		if (!target_found) {
			sprintf( buf, "Right now, %s, there are no missions worthy of your talents.", ch->name );
			do_say (pAssign, buf);
			return;
		}
        */
		
		if ((room = get_room_index(vnum)) == NULL || target == NULL) {
			logstr (LOG_BUG, "GenMission: found NULL in MISSION_SLAY");
			return;
		}

		sprintf (buf, "%s has commited several heinous crimes. You must stop him. %s, see that %s is killed!",
			target->short_descr, ch->name, target->short_descr);
		do_say (pAssign, buf);

		if ((guard = GetGuard(ch)) == NULL) 
			guard = pAssign;

		do_emote (guard, "points to a map.");

		one_argument (room->name, first);
		if (is_exact_name(first, "on in at by near inside under within approaching standing climbing swimming sailing walking sliding floating close lost along going over near very atop beneath before below between"))
			prep = "";
		else
			prep = "at ";
		sprintf (buf, "I hear that %s was recently seen %s%s in %s.",
			target->short_descr, prep, decap(target->in_room->name), room->area->name);
		do_say (guard, buf);

		ch->pcdata->reward_mob = target->pIndexData->vnum;
		ch->pcdata->reward_obj = 0;
	}


	ch->pcdata->nMissionType = nType;
	ch->pcdata->rewarder   = 0;
	ch->pcdata->hunt_time  = (current_time + 1200);
	return;
}


void do_mission (CHAR_DATA *ch, char *argument) {
	char 				buf[MAX_STRING_LENGTH];
	char 				arg[MAX_STRING_LENGTH];
	char const 			*prep;
	CHAR_DATA 			*captain;
	CHAR_DATA 			*rewarder = NULL;
	OBJ_DATA			*obj = NULL;
	OBJ_DATA			*obj_next;
	ROOM_INDEX_DATA 	*room;
	int					reward = 0;
	int 				vnum = 0;
	bool rewarder_found	= FALSE;
    
	if (IS_NPC(ch))
		return;

	one_argument (argument, arg);
    if (!str_cmp(arg, "reset") && IS_IMMORTAL(ch))  {
		ResetHunt (ch);
		ch->pcdata->recovery  = 0;
		ch->pcdata->hunt_time = 0;
		sendch ("All mission data reset.\n\r", ch);
		return;
	}

	/* Reset them if they've run out of time */
	if (ch->pcdata->hunt_time < current_time)
		ResetHunt(ch);

	/* See if they're available for work */
	if (ch->pcdata->recovery > current_time) {
		sprintf (buf, "You have %ld minute%s left to recover from the last hunt.\n\r",
		    ((ch->pcdata->recovery - current_time) / 60) + 1,
		    (((ch->pcdata->recovery - current_time) / 60)) + 1 == 1 ? "" : "s");
		sendch (buf, ch);
		return;
	}

	/* If they have been assigned a hunt, look for the object in
	 * the char's inventory */
	if (ch->pcdata->nMissionType == MISSION_OBJ && ch->pcdata->reward_obj > 0) {
		ch->pcdata->bMissionSuccess = FALSE;
        for (obj = ch->carrying; obj != NULL; obj= obj_next) {
			obj_next = obj->next_content;
			if (obj->pIndexData->vnum == ch->pcdata->reward_obj) {
				ch->pcdata->bMissionSuccess = TRUE;
				break;
			}
		}
	}

	/* If they have the object, check for the rewarder in the room */
	if (ch->pcdata->nMissionType == MISSION_OBJ && ch->pcdata->bMissionSuccess 
		&& (rewarder = FindMissionRewarder(ch)) != NULL) {
   		// Need to create a temporary mob to access the difficulty rating
		CHAR_DATA *reward_mob = create_mobile(get_mob_index (ch->pcdata->reward_mob));
		if (reward_mob == NULL)
			reward_mob = ch;
		reward = 75 + ((ch->pcdata->hunt_time - current_time) / 20);
		reward = reward_mob->nDifficulty * reward / ch->nDifficulty;
		switch (number_range (1, 3)) {
			case 1:
			    sprintf (buf, "Wonderful, %s! You have found %s that was stolen from me by %s! In appreciation, I give you a bounty of %d points.",
				   ch->name, obj->pIndexData->short_descr, reward_mob->short_descr, reward);
				break;
			case 2:
			    sprintf (buf, "%s, you have found %s! This is marvelous! Here, I'll give you %d points for a bounty.",
				    ch->name, obj->pIndexData->short_descr, reward);
				break;
			default:
			    sprintf (buf, "%s! %s, you have %s! To show my gratitude, you shall have a bounty of %d points.",
				   capitalize(obj->pIndexData->short_descr), ch->name, obj->pIndexData->short_descr, reward);
				break;
		}
		do_say (rewarder, buf);
		act ("$n takes $p from $N and gives $M the bounty.", rewarder, obj, ch, TO_NOTVICT);
		act ("$n takes $p from you and gives you the bounty.", rewarder, obj, ch, TO_VICT);
		extract_obj (obj);
		ResetHunt (ch);
		ch->pcdata->rewards += reward;
		extract_char (reward_mob, TRUE);
		return;
	}

	/* see if the character is in the room with the ACT_MISSION mob */
	if ((captain = FindMissionAssigner(ch)) != NULL) {
		if (ch->pcdata->nMissionType == MISSION_NONE) {
			/* GenMission() will almost always find a mob, so if you want a chance of
			 * failure, THIS is the place to put it - before we load the CPU. */
			GenMission (ch, captain);
			return;
		}

		/* they have found the object, so we find someone to send them to */
		if (ch->pcdata->nMissionType == MISSION_OBJ && ch->pcdata->bMissionSuccess) {
			/* first make sure it's their first time asking */
			if (ch->pcdata->rewarder > 0)	{
				sprintf( buf, "%s, you have %s that belongs to %s. Now, get going! You have %ld minute%s left.",
					ch->name,
					get_obj_index(ch->pcdata->reward_obj)->short_descr,
					get_mob_index(ch->pcdata->rewarder)->short_descr,
					(ch->pcdata->hunt_time - current_time) / 60 + 1,
					(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s" );
				do_say( captain, buf );
				return;
			}

			else {
				rewarder_found	= FALSE;
				while (!rewarder_found) {
				    vnum = number_range(0, top_vnum_room);
					if ((room = get_room_index (vnum)) != NULL) {
						for (rewarder = room->people; rewarder != NULL; rewarder = rewarder->next_in_room) {
							if (IS_NPC(rewarder)
								&& ch->nDifficulty + (ch->nDifficulty / 2) >= rewarder->nDifficulty
								&& ch->nDifficulty - (ch->nDifficulty / 2) <= rewarder->nDifficulty
							    && !IS_SET(rewarder->act, ACT_AGGRESSIVE)
								&& !IS_SET(rewarder->act, ACT_ANIMAL)
							    && !IS_SET(rewarder->affected_by, AFF_CHARM)) {
								ch->pcdata->rewarder = rewarder->pIndexData->vnum;
								ch->pcdata->hunt_time = (current_time + 1200);
								rewarder_found = TRUE;
								break;
							}
						}
					}
				}
			}

			if ((room = get_room_index (vnum)) == NULL || rewarder == NULL)	{
				logstr (LOG_BUG, "do_mission: found NULL");
				return;
			}

			one_argument (room->name, arg);
			if( is_exact_name(arg, "on in at by near inside under within approaching standing climbing flying sailing swimming falling close shipwrecked along going over near very beneath before way" ) )
			    prep = "";
			else 
				prep = "at ";

			sprintf( buf, "Well done, %s, you have found %s! Now, take it to %s %s%s in %s and ask for a reward.",
				ch->name, get_obj_index(ch->pcdata->reward_obj)->short_descr,
				get_mob_index(ch->pcdata->rewarder)->short_descr, prep,
				decap(room->name), room->area->name);
			do_say( captain, buf );

			return;
		}
		else if (ch->pcdata->nMissionType == MISSION_SLAY && ch->pcdata->bMissionSuccess) {
   			// Need to create a temporary mob to access the difficulty rating
			CHAR_DATA *reward_mob = create_mobile(get_mob_index (ch->pcdata->reward_mob));
			if (reward_mob == NULL)
				reward_mob = ch;
			reward = 50 + ((ch->pcdata->hunt_time - current_time) / 20);
			reward = reward_mob->nDifficulty * reward / ch->nDifficulty;
			sprintf (buf, "Thank you %s! You have slain %s! In appreciation, I give you a bounty of %d points.",
			   ch->name, reward_mob->short_descr, reward);
			do_say (captain, buf);
			act ("$n shakes $N's hand and gives $M the bounty.", captain, NULL, ch, TO_NOTVICT);
			act ("$n shakes your hand and gives you the bounty.", captain, NULL, ch, TO_VICT);
			ResetHunt (ch);
			ch->pcdata->rewards += reward;
			extract_char (reward_mob, TRUE);
			return;
		}

		/* OK, no obj, let's give them a progress report */
		else if (ch->pcdata->nMissionType == MISSION_OBJ) {
			sprintf( buf, "%s, you are hunting for %s that was stolen by %s. You have %ld minute%s left.",
				ch->name,
				get_obj_index(ch->pcdata->reward_obj)->short_descr,
				get_mob_index(ch->pcdata->reward_mob)->short_descr,
				(ch->pcdata->hunt_time - current_time) / 60 + 1,
				(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s");
			do_say( captain, buf );
			return;
		}
		else if (ch->pcdata->nMissionType == MISSION_SLAY) {
			sprintf( buf, "You are looking for %s. You have %ld minute%s left.",
				get_mob_index(ch->pcdata->reward_mob)->short_descr,
				(ch->pcdata->hunt_time - current_time) / 60 + 1,
				(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s");
			do_say( captain, buf );
			return;
		}
	}

	/* not with the ACT_MISSION or REWARDER mob, so...  */

	if (ch->pcdata->nMissionType == MISSION_NONE) {
		sprintf( buf, "You are not bounty hunting at the moment.\n\rFind someone who will give you a mission.\n\r" );
		sendch( buf, ch );
		return;
	}

	if (ch->pcdata->nMissionType == MISSION_OBJ && ch->pcdata->bMissionSuccess) {
		/* if they have a rewarder, send a reminder */
		if (ch->pcdata->rewarder != 0) {
			sprintf( buf, "You have %ld minute%s left to return %s to %s!\n\r",
				(ch->pcdata->hunt_time - current_time) / 60 + 1,
				(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s",
				get_obj_index(ch->pcdata->reward_obj)->short_descr,
				get_mob_index(ch->pcdata->rewarder)->short_descr );
			sendch( buf, ch );
			return;
		}
		/* if they've just found the object, send them back to the captain */
		else {
			sprintf( buf, "You have found %s with %ld minute%s left to go!\n\rReturn to the Captain for further intructions.\n\r",
				get_obj_index(ch->pcdata->reward_obj)->short_descr,
				(ch->pcdata->hunt_time - current_time) / 60 + 1,
				(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s");
			sendch( buf, ch );
			return;
		}
	}
    else if (ch->pcdata->nMissionType == MISSION_SLAY && ch->pcdata->bMissionSuccess) {
		sprintf( buf, "You have slain %s with %ld minute%s left to go!\n\rReturn to the Captain for further intructions.\n\r",
			get_mob_index(ch->pcdata->reward_mob)->short_descr,
			(ch->pcdata->hunt_time - current_time) / 60 + 1,
			(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s");
		sendch( buf, ch );
		return;
	}

	/* if they're still hunting, give them a progress report */
	if (ch->pcdata->nMissionType == MISSION_OBJ) {
		sprintf( buf, "You are hunting for %s that was stolen by %s.\n\rYou have %ld minute%s left.\n\r",
			get_obj_index(ch->pcdata->reward_obj)->short_descr,
			get_mob_index(ch->pcdata->reward_mob)->short_descr,
			(ch->pcdata->hunt_time - current_time) / 60 + 1,
			(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s");
		sendch( buf, ch );
		return;
	}
	else if (ch->pcdata->nMissionType == MISSION_SLAY) {
		sprintf( buf, "You are looking for %s.\n\rYou have %ld minute%s left.\n\r",
			get_mob_index(ch->pcdata->reward_mob)->short_descr,
			(ch->pcdata->hunt_time - current_time) / 60 + 1,
			(((ch->pcdata->hunt_time - current_time ) / 60)) + 1 == 1 ? "" : "s");
		sendch( buf, ch );
		return;
	}
	/* since we've tested all the possibilities, you should never see this message */
	else {
		logstr (LOG_BUG, "do_mission: A player passed ALL the tests");
		return;
	}

}

