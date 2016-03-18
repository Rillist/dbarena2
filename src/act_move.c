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
 **************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor (rtaylor@hypercube.org)                               *
 *       Gabrielle Taylor (gtaylor@hypercube.org)                          *
 *       Brian Moore (zump@rom.org)                                        *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 **************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

/*   QuickMUD - The Lazy Man's ROM - $Id: act_move.c,v 1.2 2000/12/01 10:48:33 ring0 Exp $ */

#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "merc.h"
#include "interp.h"

char *const dir_name[] = {
    "north", "east", "south", "west", "up", "down"
};

const sh_int rev_dir[] = {
    2, 3, 0, 1, 5, 4
};

const sh_int movement_loss[SECT_MAX] = {
    1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6
};



/*
 * Local functions.
 */
int find_door args ((CHAR_DATA * ch, char *arg));
bool has_key args ((CHAR_DATA * ch, int key));
void release args((CHAR_DATA *ch));


void move_char (CHAR_DATA * ch, int door, bool follow)
{
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;

    if (door < 0 || door > 5)
    {
        logstr (LOG_BUG, "Do_move: bad door %d.", door);
        return;
    }

    /*
     * Exit trigger, if activated, bail out. Only PCs are triggered.
     */
    if ( !IS_NPC(ch) 
      && (p_exit_trigger( ch, door, PRG_MPROG ) 
      ||  p_exit_trigger( ch, door, PRG_OPROG )
      ||  p_exit_trigger( ch, door, PRG_RPROG )) )
	return;

    in_room = ch->in_room;
    if ((pexit = in_room->exit[door]) == NULL
        || (to_room = pexit->u1.to_room) == NULL
        || !can_see_room (ch, pexit->u1.to_room))
    {
        sendch ("Alas, you cannot go that way.\n\r", ch);
        return;
    }

    if (IS_SET (pexit->exit_info, EX_CLOSED)
        && (!IS_AFFECTED (ch, AFF_PASS_DOOR)
            || IS_SET (pexit->exit_info, EX_NOPASS))
        && !IS_TRUSTED (ch, IMMORTAL))
    {
        act ("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM)
        && ch->master != NULL && in_room == ch->master->in_room)
    {
        sendch ("What?  And leave your beloved master?\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, to_room) && room_is_private (to_room))
    {
        sendch ("That room is private right now.\n\r", ch);
        return;
    }

    if (in_room->sector_type == SECT_UNDERWATER &&
		!IS_AFFECTED(ch,AFF_SWIM) && !IS_IMMORTAL(ch)) {
        sendch ("You can't swim.\n\r", ch);
        return;
    }

    if (!IS_NPC (ch))
    {
        int ki;

        if (in_room->sector_type == SECT_AIR
            || to_room->sector_type == SECT_AIR)
        {
            if (!IS_AFFECTED (ch, AFF_FLYING) && !IS_IMMORTAL (ch))
            {
                sendch ("You can't fly.\n\r", ch);
                return;
            }
        }

        if ((in_room->sector_type == SECT_WATER_NOSWIM
             || to_room->sector_type == SECT_WATER_NOSWIM)
            && !IS_AFFECTED (ch, AFF_FLYING))
        {
            OBJ_DATA *obj;
            bool found;

            /*
             * Look for a boat.
             */
            found = FALSE;

            if (IS_IMMORTAL (ch))
                found = TRUE;

            for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
            {
                if (obj->item_type == ITEM_BOAT)
                {
                    found = TRUE;
                    break;
                }
            }
            if (!found)
            {
                sendch ("You need a boat to go there.\n\r", ch);
                return;
            }
        }

        ki = movement_loss[UMIN (SECT_MAX - 1, in_room->sector_type)]
            + movement_loss[UMIN (SECT_MAX - 1, to_room->sector_type)];

        ki /= 2;                /* i.e. the average */


        /* conditional effects */
        if (IS_AFFECTED (ch, AFF_FLYING) || IS_AFFECTED (ch, AFF_HASTE))
            ki /= 2;

        if (IS_AFFECTED (ch, AFF_SLOW))
            ki *= 2;

        if (ch->ki < ki)
        {
            sendch ("You are too exhausted.\n\r", ch);
            return;
        }

        wait (ch, 1);
        ch->ki -= ki;
    }

    if (!IS_AFFECTED (ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO)
        act ("$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM);

    char_from_room (ch);
    char_to_room (ch, to_room);
    if (!IS_AFFECTED (ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO)
        act ("$n has arrived.", ch, NULL, NULL, TO_ROOM);

    do_function (ch, &do_look, "auto");

    if (in_room == to_room)        /* no circular follows */
        return;

    for (fch = in_room->people; fch != NULL; fch = fch_next)
    {
        fch_next = fch->next_in_room;

        if (fch->master == ch && IS_AFFECTED (fch, AFF_CHARM)
            && fch->position < POS_STANDING)
            do_function (fch, &do_stand, "");

        if (fch->master == ch && fch->position == POS_STANDING
            && can_see_room (fch, to_room))
        {

            if (IS_SET (ch->in_room->room_flags, ROOM_LAW)
                && (IS_NPC (fch) && IS_SET (fch->act, ACT_AGGRESSIVE)))
            {
                act ("You can't bring $N into the city.",
                     ch, NULL, fch, TO_CHAR);
                act ("You aren't allowed in the city.",
                     fch, NULL, NULL, TO_CHAR);
                continue;
            }

            act ("You follow $N.", fch, NULL, ch, TO_CHAR);
            move_char (fch, door, TRUE);
        }
    }

    /* 
     * If someone is following the char, these triggers get activated
     * for the followers before the char, but it's safer this way...
     */
    if (IS_NPC (ch) && HAS_TRIGGER_MOB (ch, TRIG_ENTRY))
        p_percent_trigger (ch, NULL, NULL, NULL, NULL, NULL, TRIG_ENTRY);
    if ( !IS_NPC( ch ) ) {
    	p_greet_trigger( ch, PRG_MPROG );
		p_greet_trigger( ch, PRG_OPROG );
		p_greet_trigger( ch, PRG_RPROG );
    }

    return;
}



void do_north (CHAR_DATA * ch, char *argument)
{
    move_char (ch, DIR_NORTH, FALSE);
    return;
}



void do_east (CHAR_DATA * ch, char *argument)
{
    move_char (ch, DIR_EAST, FALSE);
    return;
}



void do_south (CHAR_DATA * ch, char *argument)
{
    move_char (ch, DIR_SOUTH, FALSE);
    return;
}



void do_west (CHAR_DATA * ch, char *argument)
{
    move_char (ch, DIR_WEST, FALSE);
    return;
}



void do_up (CHAR_DATA * ch, char *argument)
{
    move_char (ch, DIR_UP, FALSE);
    return;
}



void do_down (CHAR_DATA * ch, char *argument)
{
    move_char (ch, DIR_DOWN, FALSE);
    return;
}



int find_door (CHAR_DATA * ch, char *arg)
{
    EXIT_DATA *pexit;
    int door;

    if (!str_cmp (arg, "n") || !str_cmp (arg, "north"))
        door = 0;
    else if (!str_cmp (arg, "e") || !str_cmp (arg, "east"))
        door = 1;
    else if (!str_cmp (arg, "s") || !str_cmp (arg, "south"))
        door = 2;
    else if (!str_cmp (arg, "w") || !str_cmp (arg, "west"))
        door = 3;
    else if (!str_cmp (arg, "u") || !str_cmp (arg, "up"))
        door = 4;
    else if (!str_cmp (arg, "d") || !str_cmp (arg, "down"))
        door = 5;
    else
    {
        for (door = 0; door <= 5; door++)
        {
            if ((pexit = ch->in_room->exit[door]) != NULL
                && IS_SET (pexit->exit_info, EX_ISDOOR)
                && pexit->keyword != NULL && is_name (arg, pexit->keyword))
                return door;
        }
        act ("I see no $T here.", ch, NULL, arg, TO_CHAR);
        return -1;
    }

    if ((pexit = ch->in_room->exit[door]) == NULL)
    {
        act ("I see no door $T here.", ch, NULL, arg, TO_CHAR);
        return -1;
    }

    if (!IS_SET (pexit->exit_info, EX_ISDOOR))
    {
        sendch ("You can't do that.\n\r", ch);
        return -1;
    }

    return door;
}



void do_open (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Open what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here (ch, NULL, arg)) != NULL)
    {
        /* open portal */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET (obj->value[1], EX_ISDOOR))
            {
                sendch ("You can't do that.\n\r", ch);
                return;
            }

            if (!IS_SET (obj->value[1], EX_CLOSED))
            {
                sendch ("It's already open.\n\r", ch);
                return;
            }

            if (IS_SET (obj->value[1], EX_LOCKED))
            {
                sendch ("It's locked.\n\r", ch);
                return;
            }

            REMOVE_BIT (obj->value[1], EX_CLOSED);
            act ("You open $p.", ch, obj, NULL, TO_CHAR);
            act ("$n opens $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'open object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            sendch ("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_CLOSED))
        {
            sendch ("It's already open.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_CLOSEABLE))
        {
            sendch ("You can't do that.\n\r", ch);
            return;
        }
        if (IS_SET (obj->value[1], CONT_LOCKED))
        {
            sendch ("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT (obj->value[1], CONT_CLOSED);
        act ("You open $p.", ch, obj, NULL, TO_CHAR);
        act ("$n opens $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door (ch, arg)) >= 0)
    {
        /* 'open door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET (pexit->exit_info, EX_CLOSED))
        {
            sendch ("It's already open.\n\r", ch);
            return;
        }
        if (IS_SET (pexit->exit_info, EX_LOCKED))
        {
            sendch ("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT (pexit->exit_info, EX_CLOSED);
        act ("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM);
        sendch ("Ok.\n\r", ch);

        /* open the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            CHAR_DATA *rch;

            REMOVE_BIT (pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
                act ("The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR);
        }
    }

    return;
}



void do_close (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Close what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here (ch, NULL, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {

            if (!IS_SET (obj->value[1], EX_ISDOOR)
                || IS_SET (obj->value[1], EX_NOCLOSE))
            {
                sendch ("You can't do that.\n\r", ch);
                return;
            }

            if (IS_SET (obj->value[1], EX_CLOSED))
            {
                sendch ("It's already closed.\n\r", ch);
                return;
            }

            SET_BIT (obj->value[1], EX_CLOSED);
            act ("You close $p.", ch, obj, NULL, TO_CHAR);
            act ("$n closes $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'close object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            sendch ("That's not a container.\n\r", ch);
            return;
        }
        if (IS_SET (obj->value[1], CONT_CLOSED))
        {
            sendch ("It's already closed.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_CLOSEABLE))
        {
            sendch ("You can't do that.\n\r", ch);
            return;
        }

        SET_BIT (obj->value[1], CONT_CLOSED);
        act ("You close $p.", ch, obj, NULL, TO_CHAR);
        act ("$n closes $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door (ch, arg)) >= 0)
    {
        /* 'close door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (IS_SET (pexit->exit_info, EX_CLOSED))
        {
            sendch ("It's already closed.\n\r", ch);
            return;
        }

        SET_BIT (pexit->exit_info, EX_CLOSED);
        act ("$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM);
        sendch ("Ok.\n\r", ch);

        /* close the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
            && pexit_rev->u1.to_room == ch->in_room)
        {
            CHAR_DATA *rch;

            SET_BIT (pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
                act ("The $d closes.", rch, NULL, pexit_rev->keyword,
                     TO_CHAR);
        }
    }

    return;
}



bool has_key (CHAR_DATA * ch, int key)
{
    OBJ_DATA *obj;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == key)
            return TRUE;
    }

    return FALSE;
}



void do_lock (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Lock what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here (ch, NULL, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET (obj->value[1], EX_ISDOOR)
                || IS_SET (obj->value[1], EX_NOCLOSE))
            {
                sendch ("You can't do that.\n\r", ch);
                return;
            }
            if (!IS_SET (obj->value[1], EX_CLOSED))
            {
                sendch ("It's not closed.\n\r", ch);
                return;
            }

            if (obj->value[4] < 0 || IS_SET (obj->value[1], EX_NOLOCK))
            {
                sendch ("It can't be locked.\n\r", ch);
                return;
            }

            if (!has_key (ch, obj->value[4]))
            {
                sendch ("You lack the key.\n\r", ch);
                return;
            }

            if (IS_SET (obj->value[1], EX_LOCKED))
            {
                sendch ("It's already locked.\n\r", ch);
                return;
            }

            SET_BIT (obj->value[1], EX_LOCKED);
            act ("You lock $p.", ch, obj, NULL, TO_CHAR);
            act ("$n locks $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'lock object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            sendch ("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_CLOSED))
        {
            sendch ("It's not closed.\n\r", ch);
            return;
        }
        if (obj->value[2] < 0)
        {
            sendch ("It can't be locked.\n\r", ch);
            return;
        }
        if (!has_key (ch, obj->value[2]))
        {
            sendch ("You lack the key.\n\r", ch);
            return;
        }
        if (IS_SET (obj->value[1], CONT_LOCKED))
        {
            sendch ("It's already locked.\n\r", ch);
            return;
        }

        SET_BIT (obj->value[1], CONT_LOCKED);
        act ("You lock $p.", ch, obj, NULL, TO_CHAR);
        act ("$n locks $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door (ch, arg)) >= 0)
    {
        /* 'lock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET (pexit->exit_info, EX_CLOSED))
        {
            sendch ("It's not closed.\n\r", ch);
            return;
        }
        if (pexit->key < 0)
        {
            sendch ("It can't be locked.\n\r", ch);
            return;
        }
        if (!has_key (ch, pexit->key))
        {
            sendch ("You lack the key.\n\r", ch);
            return;
        }
        if (IS_SET (pexit->exit_info, EX_LOCKED))
        {
            sendch ("It's already locked.\n\r", ch);
            return;
        }

        SET_BIT (pexit->exit_info, EX_LOCKED);
        sendch ("*Click*\n\r", ch);
        act ("$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

        /* lock the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
            && pexit_rev->u1.to_room == ch->in_room)
        {
            SET_BIT (pexit_rev->exit_info, EX_LOCKED);
        }
    }

    return;
}



void do_unlock (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Unlock what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here (ch, NULL, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET (obj->value[1], EX_ISDOOR))
            {
                sendch ("You can't do that.\n\r", ch);
                return;
            }

            if (!IS_SET (obj->value[1], EX_CLOSED))
            {
                sendch ("It's not closed.\n\r", ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                sendch ("It can't be unlocked.\n\r", ch);
                return;
            }

            if (!has_key (ch, obj->value[4]))
            {
                sendch ("You lack the key.\n\r", ch);
                return;
            }

            if (!IS_SET (obj->value[1], EX_LOCKED))
            {
                sendch ("It's already unlocked.\n\r", ch);
                return;
            }

            REMOVE_BIT (obj->value[1], EX_LOCKED);
            act ("You unlock $p.", ch, obj, NULL, TO_CHAR);
            act ("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'unlock object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            sendch ("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_CLOSED))
        {
            sendch ("It's not closed.\n\r", ch);
            return;
        }
        if (obj->value[2] < 0)
        {
            sendch ("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!has_key (ch, obj->value[2]))
        {
            sendch ("You lack the key.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_LOCKED))
        {
            sendch ("It's already unlocked.\n\r", ch);
            return;
        }

        REMOVE_BIT (obj->value[1], CONT_LOCKED);
        act ("You unlock $p.", ch, obj, NULL, TO_CHAR);
        act ("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door (ch, arg)) >= 0)
    {
        /* 'unlock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET (pexit->exit_info, EX_CLOSED))
        {
            sendch ("It's not closed.\n\r", ch);
            return;
        }
        if (pexit->key < 0)
        {
            sendch ("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!has_key (ch, pexit->key))
        {
            sendch ("You lack the key.\n\r", ch);
            return;
        }
        if (!IS_SET (pexit->exit_info, EX_LOCKED))
        {
            sendch ("It's already unlocked.\n\r", ch);
            return;
        }

        REMOVE_BIT (pexit->exit_info, EX_LOCKED);
        sendch ("*Click*\n\r", ch);
        act ("$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

        /* unlock the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            REMOVE_BIT (pexit_rev->exit_info, EX_LOCKED);
        }
    }

    return;
}



void do_pick (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Pick what?\n\r", ch);
        return;
    }

    wait (ch, skill_table[gsn_pick_lock].wait - get_skill(ch,gsn_pick_lock)/2);

    /* look for guards */
    for (gch = ch->in_room->people; gch; gch = gch->next_in_room)
    {
        if (IS_NPC (gch) && IS_AWAKE (gch) &&
			// Check relative stats and skills
			number_range( (get_curr_stat(ch,STAT_DEX) +get_skill(ch,gsn_pick_lock)*5)/2,   get_curr_stat(ch,STAT_DEX) +get_skill(ch,gsn_pick_lock)*5) >
			number_range( (get_curr_stat(gch,STAT_DEX)+get_skill(gch,gsn_perception)*5)/2, get_curr_stat(gch,STAT_DEX)+get_skill(gch,gsn_perception)*5))
        {
            act ("$N is standing too close to the lock.",
                 ch, NULL, gch, TO_CHAR);
            return;
        }
    }

    if ((!IS_NPC (ch) && number_percent() > get_skill (ch, gsn_pick_lock) * 50)
		|| get_skill(ch,gsn_pick_lock) == 0)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if ((obj = get_obj_here (ch, NULL, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET (obj->value[1], EX_ISDOOR))
            {
                sendch ("You can't do that.\n\r", ch);
                return;
            }

            if (!IS_SET (obj->value[1], EX_CLOSED))
            {
                sendch ("It's not closed.\n\r", ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                sendch ("It can't be unlocked.\n\r", ch);
                return;
            }

            if (IS_SET (obj->value[1], EX_PICKPROOF))
            {
                sendch ("You failed.\n\r", ch);
                return;
            }

            REMOVE_BIT (obj->value[1], EX_LOCKED);
            act ("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
            act ("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
            return;
        }


        /* 'pick object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            sendch ("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_CLOSED))
        {
            sendch ("It's not closed.\n\r", ch);
            return;
        }
        if (obj->value[2] < 0)
        {
            sendch ("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!IS_SET (obj->value[1], CONT_LOCKED))
        {
            sendch ("It's already unlocked.\n\r", ch);
            return;
        }
        if (IS_SET (obj->value[1], CONT_PICKPROOF))
        {
            sendch ("You failed.\n\r", ch);
            return;
        }

        REMOVE_BIT (obj->value[1], CONT_LOCKED);
        act ("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
        act ("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door (ch, arg)) >= 0)
    {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET (pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL (ch))
        {
            sendch ("It's not closed.\n\r", ch);
            return;
        }
        if (pexit->key < 0 && !IS_IMMORTAL (ch))
        {
            sendch ("It can't be picked.\n\r", ch);
            return;
        }
        if (!IS_SET (pexit->exit_info, EX_LOCKED))
        {
            sendch ("It's already unlocked.\n\r", ch);
            return;
        }
        if (IS_SET (pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL (ch))
        {
            sendch ("You failed.\n\r", ch);
            return;
        }

        REMOVE_BIT (pexit->exit_info, EX_LOCKED);
        sendch ("*Click*\n\r", ch);
        act ("$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

        /* pick the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            REMOVE_BIT (pexit_rev->exit_info, EX_LOCKED);
        }
    }

    return;
}




void do_stand (CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (argument[0] != '\0')
    {
        if (ch->position == POS_FIGHTING)
        {
            sendch ("Maybe you should finish fighting first?\n\r", ch);
            return;
        }
        obj = get_obj_list (ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
            sendch ("You don't see that here.\n\r", ch);
            return;
        }
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET (obj->value[2], STAND_AT)
                && !IS_SET (obj->value[2], STAND_ON)
                && !IS_SET (obj->value[2], STAND_IN)))
        {
            sendch ("You can't seem to find a place to stand.\n\r", ch);
            return;
        }
        if (ch->on != obj && count_users (obj) >= obj->value[0])
        {
            act_new ("There's no room to stand on $p.",
                     ch, obj, NULL, TO_CHAR, POS_DEAD);
            return;
        }
        ch->on = obj;
		if ( HAS_TRIGGER_OBJ( obj, TRIG_SIT ) )
			p_percent_trigger( NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT );
    }

    switch (ch->position)
    {
        case POS_SLEEPING:
            if (IS_AFFECTED (ch, AFF_SLEEP))
            {
                sendch ("You can't wake up!\n\r", ch);
                return;
            }

            if (obj == NULL)
            {
                sendch ("You wake and stand up.\n\r", ch);
                act ("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM);
                ch->on = NULL;
            }
            else if (IS_SET (obj->value[2], STAND_AT))
            {
                act_new ("You wake and stand at $p.", ch, obj, NULL, TO_CHAR,
                         POS_DEAD);
                act ("$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], STAND_ON))
            {
                act_new ("You wake and stand on $p.", ch, obj, NULL, TO_CHAR,
                         POS_DEAD);
                act ("$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act_new ("You wake and stand in $p.", ch, obj, NULL, TO_CHAR,
                         POS_DEAD);
                act ("$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_STANDING;
            do_function (ch, &do_look, "auto");
            break;

        case POS_RESTING:
        case POS_SITTING:
            if (obj == NULL)
            {
                sendch ("You stand up.\n\r", ch);
                act ("$n stands up.", ch, NULL, NULL, TO_ROOM);
                ch->on = NULL;
            }
            else if (IS_SET (obj->value[2], STAND_AT))
            {
                act ("You stand at $p.", ch, obj, NULL, TO_CHAR);
                act ("$n stands at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], STAND_ON))
            {
                act ("You stand on $p.", ch, obj, NULL, TO_CHAR);
                act ("$n stands on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act ("You stand in $p.", ch, obj, NULL, TO_CHAR);
                act ("$n stands on $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_STANDING;
            break;

        case POS_STANDING:
            sendch ("You are already standing.\n\r", ch);
            break;

        case POS_FIGHTING:
            sendch ("You are already fighting!\n\r", ch);
            break;
    }

    return;
}



void do_rest (CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
        sendch ("You are already fighting!\n\r", ch);
        return;
    }

    /* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0')
    {
        obj = get_obj_list (ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
            sendch ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
        obj = ch->on;

    if (obj != NULL)
    {
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET (obj->value[2], REST_ON)
                && !IS_SET (obj->value[2], REST_IN)
                && !IS_SET (obj->value[2], REST_AT)))
        {
            sendch ("You can't rest on that.\n\r", ch);
            return;
        }

        if (obj != NULL && ch->on != obj
            && count_users (obj) >= obj->value[0])
        {
            act_new ("There's no more room on $p.", ch, obj, NULL, TO_CHAR,
                     POS_DEAD);
            return;
        }

        ch->on = obj;
		if ( HAS_TRIGGER_OBJ( obj, TRIG_SIT ) )
			p_percent_trigger( NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT );
    }

    switch (ch->position)
    {
        case POS_SLEEPING:
            if (IS_AFFECTED (ch, AFF_SLEEP))
            {
                sendch ("You can't wake up!\n\r", ch);
                return;
            }

            if (obj == NULL)
            {
                sendch ("You wake up and start resting.\n\r", ch);
                act ("$n wakes up and starts resting.", ch, NULL, NULL,
                     TO_ROOM);
            }
            else if (IS_SET (obj->value[2], REST_AT))
            {
                act_new ("You wake up and rest at $p.",
                         ch, obj, NULL, TO_CHAR, POS_SLEEPING);
                act ("$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], REST_ON))
            {
                act_new ("You wake up and rest on $p.",
                         ch, obj, NULL, TO_CHAR, POS_SLEEPING);
                act ("$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act_new ("You wake up and rest in $p.",
                         ch, obj, NULL, TO_CHAR, POS_SLEEPING);
                act ("$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_RESTING;
            break;

        case POS_RESTING:
            sendch ("You are already resting.\n\r", ch);
            break;

        case POS_STANDING:
            if (obj == NULL)
            {
                sendch ("You rest.\n\r", ch);
                act ("$n sits down and rests.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], REST_AT))
            {
                act ("You sit down at $p and rest.", ch, obj, NULL, TO_CHAR);
                act ("$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], REST_ON))
            {
                act ("You sit on $p and rest.", ch, obj, NULL, TO_CHAR);
                act ("$n sits on $p and rests.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act ("You rest in $p.", ch, obj, NULL, TO_CHAR);
                act ("$n rests in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_RESTING;
            break;

        case POS_SITTING:
            if (obj == NULL)
            {
                sendch ("You rest.\n\r", ch);
                act ("$n rests.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], REST_AT))
            {
                act ("You rest at $p.", ch, obj, NULL, TO_CHAR);
                act ("$n rests at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], REST_ON))
            {
                act ("You rest on $p.", ch, obj, NULL, TO_CHAR);
                act ("$n rests on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act ("You rest in $p.", ch, obj, NULL, TO_CHAR);
                act ("$n rests in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_RESTING;
            break;
    }


    return;
}


void do_sit (CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
        sendch ("Maybe you should finish this fight first?\n\r", ch);
        return;
    }

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0')
    {
        obj = get_obj_list (ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
            sendch ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
        obj = ch->on;

    if (obj != NULL)
    {
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET (obj->value[2], SIT_ON)
                && !IS_SET (obj->value[2], SIT_IN)
                && !IS_SET (obj->value[2], SIT_AT)))
        {
            sendch ("You can't sit on that.\n\r", ch);
            return;
        }

        if (obj != NULL && ch->on != obj
            && count_users (obj) >= obj->value[0])
        {
            act_new ("There's no more room on $p.", ch, obj, NULL, TO_CHAR,
                     POS_DEAD);
            return;
        }

        ch->on = obj;
		if ( HAS_TRIGGER_OBJ( obj, TRIG_SIT ) )
			p_percent_trigger( NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT );
    }
    switch (ch->position)
    {
        case POS_SLEEPING:
            if (IS_AFFECTED (ch, AFF_SLEEP))
            {
                sendch ("You can't wake up!\n\r", ch);
                return;
            }

            if (obj == NULL)
            {
                sendch ("You wake and sit up.\n\r", ch);
                act ("$n wakes and sits up.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], SIT_AT))
            {
                act_new ("You wake and sit at $p.", ch, obj, NULL, TO_CHAR,
                         POS_DEAD);
                act ("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], SIT_ON))
            {
                act_new ("You wake and sit on $p.", ch, obj, NULL, TO_CHAR,
                         POS_DEAD);
                act ("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act_new ("You wake and sit in $p.", ch, obj, NULL, TO_CHAR,
                         POS_DEAD);
                act ("$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM);
            }

            ch->position = POS_SITTING;
            break;
        case POS_RESTING:
            if (obj == NULL)
                sendch ("You stop resting.\n\r", ch);
            else if (IS_SET (obj->value[2], SIT_AT))
            {
                act ("You sit at $p.", ch, obj, NULL, TO_CHAR);
                act ("$n sits at $p.", ch, obj, NULL, TO_ROOM);
            }

            else if (IS_SET (obj->value[2], SIT_ON))
            {
                act ("You sit on $p.", ch, obj, NULL, TO_CHAR);
                act ("$n sits on $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_SITTING;
            break;
        case POS_SITTING:
            sendch ("You are already sitting down.\n\r", ch);
            break;
        case POS_STANDING:
            if (obj == NULL)
            {
                sendch ("You sit down.\n\r", ch);
                act ("$n sits down on the ground.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], SIT_AT))
            {
                act ("You sit down at $p.", ch, obj, NULL, TO_CHAR);
                act ("$n sits down at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET (obj->value[2], SIT_ON))
            {
                act ("You sit on $p.", ch, obj, NULL, TO_CHAR);
                act ("$n sits on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act ("You sit down in $p.", ch, obj, NULL, TO_CHAR);
                act ("$n sits down in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_SITTING;
            break;
    }
    return;
}


void do_sleep (CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    switch (ch->position)
    {
        case POS_SLEEPING:
            sendch ("You are already sleeping.\n\r", ch);
            break;

        case POS_RESTING:
        case POS_SITTING:
        case POS_STANDING:
            if (argument[0] == '\0' && ch->on == NULL)
            {
                sendch ("You go to sleep.\n\r", ch);
                act ("$n goes to sleep.", ch, NULL, NULL, TO_ROOM);
                ch->position = POS_SLEEPING;
            }
            else
            {                    /* find an object and sleep on it */

                if (argument[0] == '\0')
                    obj = ch->on;
                else
                    obj = get_obj_list (ch, argument, ch->in_room->contents);

                if (obj == NULL)
                {
                    sendch ("You don't see that here.\n\r", ch);
                    return;
                }
                if (obj->item_type != ITEM_FURNITURE
                    || (!IS_SET (obj->value[2], SLEEP_ON)
                        && !IS_SET (obj->value[2], SLEEP_IN)
                        && !IS_SET (obj->value[2], SLEEP_AT)))
                {
                    sendch ("You can't sleep on that!\n\r", ch);
                    return;
                }

                if (ch->on != obj && count_users (obj) >= obj->value[0])
                {
                    act_new ("There is no room on $p for you.",
                             ch, obj, NULL, TO_CHAR, POS_DEAD);
                    return;
                }

                ch->on = obj;
				if ( HAS_TRIGGER_OBJ( obj, TRIG_SIT ) )
					p_percent_trigger( NULL, obj, NULL, ch, NULL, NULL, TRIG_SIT );

                if (IS_SET (obj->value[2], SLEEP_AT))
                {
                    act ("You go to sleep at $p.", ch, obj, NULL, TO_CHAR);
                    act ("$n goes to sleep at $p.", ch, obj, NULL, TO_ROOM);
                }
                else if (IS_SET (obj->value[2], SLEEP_ON))
                {
                    act ("You go to sleep on $p.", ch, obj, NULL, TO_CHAR);
                    act ("$n goes to sleep on $p.", ch, obj, NULL, TO_ROOM);
                }
                else
                {
                    act ("You go to sleep in $p.", ch, obj, NULL, TO_CHAR);
                    act ("$n goes to sleep in $p.", ch, obj, NULL, TO_ROOM);
                }
                ch->position = POS_SLEEPING;
            }
            break;

        case POS_FIGHTING:
            sendch ("You are already fighting!\n\r", ch);
            break;
    }

    return;
}



void do_wake (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        do_function (ch, &do_stand, "");
        return;
    }

    if (!IS_AWAKE (ch))
    {
        sendch ("You are asleep yourself!\n\r", ch);
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AWAKE (victim))
    {
        act ("$N is already awake.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED (victim, AFF_SLEEP))
    {
        act ("You can't wake $M!", ch, NULL, victim, TO_CHAR);
        return;
    }

    act_new ("$n wakes you.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    do_function (ch, &do_stand, "");
    return;
}



void do_sneak (CHAR_DATA * ch, char *argument)
{
    AFFECT_DATA af;
    
	if (get_skill(ch,gsn_sneak) == 0) {
		sendch ("You don't know how to!\n\r", ch);
		return;
	}

    sendch ("You attempt to move silently.\n\r", ch);
    affect_strip (ch, gsn_sneak);

    if (IS_AFFECTED (ch, AFF_SNEAK))
        return;

	if (get_skill (ch, gsn_sneak)*10 + get_curr_stat(ch,STAT_DEX) - get_carry_weight(ch)/4 < 25) {
		sendch ("You're carrying too much!\n\r", ch);
		return;
	}

    if (number_percent () < get_skill (ch, gsn_sneak)*10 - get_carry_weight(ch)/4 + get_curr_stat(ch, STAT_DEX))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_sneak;
        af.skill_lvl = get_skill(ch,gsn_sneak);
        af.duration = get_skill(ch,gsn_sneak);
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_char (ch, &af);
    }

    return;
}



void do_hide (CHAR_DATA * ch, char *argument)
{
    if (get_skill(ch,gsn_hide) == 0) {
		sendch ("You don't know how to!\n\r", ch);
		return;
	}


	sendch ("You attempt to hide.\n\r", ch);

	if (IS_AFFECTED (ch, AFF_HIDE))
        REMOVE_BIT (ch->affected_by, AFF_HIDE);

	if (get_skill (ch, gsn_sneak)*10 + get_curr_stat(ch,STAT_DEX) - get_carry_weight(ch)/4 < 25) {
		sendch ("You're carrying too much!\n\r", ch);
		return;
	}

	if (number_percent () < get_skill (ch, gsn_sneak)*10 - get_carry_weight(ch)/4 + get_curr_stat(ch, STAT_DEX))
        SET_BIT (ch->affected_by, AFF_HIDE);

    return;
}



/*
 * Contributed by Alander.
 */
void do_visible (CHAR_DATA * ch, char *argument)
{
    affect_strip (ch, gsn_sneak);
    REMOVE_BIT (ch->affected_by, AFF_HIDE);
    REMOVE_BIT (ch->affected_by, AFF_INVISIBLE);
    REMOVE_BIT (ch->affected_by, AFF_SNEAK);
    sendch ("Ok.\n\r", ch);
    return;
}



void do_recall (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;

    if (IS_NPC (ch) && !IS_SET (ch->act, ACT_PET))
    {
        sendch ("Only players can recall.\n\r", ch);
        return;
    }

    act ("$n prays for transportation!", ch, 0, 0, TO_ROOM);

    if ((location = get_room_index (ROOM_VNUM_TEMPLE)) == NULL)
    {
        sendch ("You are completely lost.\n\r", ch);
        return;
    }

    if (ch->in_room == location)
        return;

    if (IS_SET (ch->in_room->room_flags, ROOM_NO_RECALL)
        || IS_AFFECTED (ch, AFF_CURSE))
    {
        sendch ("Mota has forsaken you.\n\r", ch);
        return;
    }

    if ((victim = ch->fighting) != NULL)
    {
        /* you can always recall!
		int skill;

        skill = get_skill (ch, gsn_recall);

        if (number_percent () < 80 * skill / 100)
        {
            wait (ch, 4);
            sprintf (buf, "You failed!.\n\r");
            sendch (buf, ch);
            return;
        }
	    */
        sprintf (buf, "You recall from combat!\n\r");
        sendch (buf, ch);
        stop_fighting (ch, TRUE);

    }

    ch->ki /= 2;
    act ("$n disappears.", ch, NULL, NULL, TO_ROOM);
    char_from_room (ch);
    char_to_room (ch, location);
    act ("$n appears in the room.", ch, NULL, NULL, TO_ROOM);
    do_function (ch, &do_look, "auto");

    if (ch->pet != NULL)
        do_function (ch->pet, &do_recall, "");

    return;
}



void do_power (CHAR_DATA * ch, char *argument)
{
    int nCurPl;

    if (!str_cmp(argument, "max"))
        nCurPl = 100;
    else if (!str_cmp(argument, "base") || !str_cmp(argument, "normal") || !str_cmp(argument, "rest"))
        nCurPl = 25;
    else
        nCurPl = atoi (argument);

    if (nCurPl < 0 || nCurPl > 100) {
        sendch("Current power level must be between 0 and 100.\n\r",ch);
        return;
    }

    if (ch->ki < 1 && nCurPl > 0) {
        sendch("You don't have enough ki to substain a power level!\n\r", ch);
        return;
    }

    wait (ch, 12);

    if (nCurPl > ch->nCurPl) {
        switch (GetTrans(ch)) {
            case TRANS_SSJ1:
            case TRANS_SSJ2:
            case TRANS_SSJ3:
            act ("{YThe burning golden aura around $n grows and swells!{x", ch, NULL, NULL, TO_ROOM);
            act ("{YThe burning golden aura around you grows and swells!{x", ch, NULL, NULL, TO_CHAR);
            break;
            case TRANS_SSJ4:
            act ("{RThe bright red aura around $n flares up!{x", ch, NULL, NULL, TO_ROOM);
            act ("{RThe bright red aura around you flares up!{x", ch, NULL, NULL, TO_CHAR);
            break;
            case TRANS_SSJ5:
            act ("{C$n's image flickers and $e growls, suddenly erupting with immense power!{x", ch, NULL, NULL, TO_ROOM);
            act ("{CYour image flickers and you growl, suddenly erupting with immense power!{x", ch, NULL, NULL, TO_CHAR);
            break;

            case TRANS_MYSTIC:
            act ("{MThe earth around $n begins to violently shake and upheave!{x", ch, NULL, NULL, TO_ROOM);
            act ("{MThe earth around you begins to violently shake and upheave!{x", ch, NULL, NULL, TO_CHAR);
            break;

            case TRANS_HYPERN:
            act ("{WThe flaming white glow around $n brightens!{x", ch, NULL, NULL, TO_ROOM);
            act ("{WThe flaming white glow around you brightens!{x", ch, NULL, NULL, TO_CHAR);
            break;

            case TRANS_SUPERN:
            act ("{GThe intense green aura surrounding $n flares to blinding proportions!{x", ch, NULL, NULL, TO_ROOM);
            act ("{GThe intense green aura surrounding you flares to blinding proportions!{x", ch, NULL, NULL, TO_CHAR);
            break;

            default:
	    act("{Y$n errupts in a blaze of energy!{x",ch,NULL,NULL,TO_ROOM);
	    act("{YYou errupt in a blaze of energy!{x",ch,NULL,NULL,TO_CHAR);
	    break;
        }
    }
    else if (nCurPl < ch->nCurPl) {
        switch (GetTrans(ch)) {
            case TRANS_SSJ1:
            case TRANS_SSJ2:
            case TRANS_SSJ3:
            act ("{yThe burning golden aura around $n dies down.{x", ch, NULL, NULL, TO_ROOM);
            act ("{yThe burning golden aura around you dies down.{x", ch, NULL, NULL, TO_CHAR);
            break;
            case TRANS_SSJ4:
            act ("{rThe bright red aura around $n dies down.{x", ch, NULL, NULL, TO_ROOM);
            act ("{rThe bright red aura around you dies down.{x", ch, NULL, NULL, TO_CHAR);
            break;
            break;
            case TRANS_SSJ5:
            act ("{C$n exhales and stretches out, causing the power around $m to gradually settle down.{x", ch, NULL, NULL, TO_ROOM);
            act ("{CYou exhale and stretch out, causing the power around you to gradually settle down.{x", ch, NULL, NULL, TO_CHAR);
            break;

            case TRANS_MYSTIC:
            act ("{mThe earth around $n gradually begins to calm itself.{x", ch, NULL, NULL, TO_ROOM);
            act ("{mThe earth around you gradually begins to calm itself.{x", ch, NULL, NULL, TO_CHAR);
            break;

            case TRANS_HYPERN:
            act ("{wThe white glow around $n darkens.{x", ch, NULL, NULL, TO_ROOM);
            act ("{wThe white glow around you darkens.{x", ch, NULL, NULL, TO_CHAR);
            break;

            case TRANS_SUPERN:
            act ("{gThe green aura surrounding $n loses its intensity.{x", ch, NULL, NULL, TO_ROOM);
            act ("{gThe green aura surrounding you loses its intensity.{x", ch, NULL, NULL, TO_CHAR);
            break;

            default:
            act("{yThe energy around $n fades down.{x",ch,NULL,NULL,TO_ROOM);
            act("{yThe energy around you fades down.{x",ch,NULL,NULL,TO_CHAR);
            break;
        }
    }
    ch->nCurPl = nCurPl;

    return;
}


void do_cancel (CHAR_DATA *ch, char *argument) {
	if (ch->wait_skill > 0 && ch->wait_skill_sn > 0) {
		ch->wait_skill = 0;
		ch->wait_skill_sn = 0;
		ch->wait_skill_vo = NULL;
		ch->wait_skill_target = 0;
		sendch("You stop peforming your skill.\n\r", ch);
		return;
	}
	if (ch->charge > 0 && ch->wait_skill_sn > 0) {
		act("You stop charging.",ch,NULL,NULL,TO_CHAR);
		act("$n stops charging.",ch,NULL,NULL,TO_ROOM);
        if (ch->powerstruggle) {
		    act("{BYou concede the powerstruggle!{x",ch,NULL,NULL,TO_CHAR);
		    act("{B$n concedes the powerstruggle!{x",ch,NULL,NULL,TO_ROOM);
            release (ch->powerstruggle);
            ch->powerstruggle->powerstruggle = NULL;
            ch->powerstruggle = NULL;
        }
		ch->charge = 0;
		ch->wait_skill_sn = 0;
		ch->wait_skill_vo = NULL;
		ch->wait_skill_target = 0;
		return;
	}
}


void do_release (CHAR_DATA *ch, char *argument) {
	if (ch->wait_skill_sn > 0) {
		CHAR_DATA *victim = (CHAR_DATA*)ch->wait_skill_vo;
		if (victim && !get_char_room(ch, NULL, victim->name)) {
			sendch("Your target has left the room.\n\r",ch);
			ch->charge = 0;
			ch->wait_skill_sn = 0;
			ch->wait_skill_vo = NULL;
			ch->wait_skill_target = TARGET_NONE;
			return;
		}

		if (ch->charge >= skill_table[ch->wait_skill_sn].wait || IS_EXHAUSTED(ch)) { // If exhausted, pl = 0
            // Resolve the powerstruggle
            if (ch->powerstruggle) {
                CHAR_DATA *victim = ch->powerstruggle;
                CHAR_DATA *winner, *loser;
                int ch_roll, vi_roll;
                ch_roll  = get_curr_stat(ch, STAT_INT) * 4;
                ch_roll += (ch->charge / PULSE_SECOND) * 8;
                ch_roll += UMAX(1,get_skill(ch, ch->wait_skill_sn)) / 2;
                vi_roll  = get_curr_stat(victim, STAT_INT) * 4;
                vi_roll += (victim->charge / PULSE_SECOND) * 8;
                vi_roll += UMAX(1,get_skill(victim, victim->wait_skill_sn)) / 2;
                if (number_range(ch_roll/2, ch_roll) > number_range(vi_roll/2, vi_roll)) {
                    winner = ch;
                    loser = victim;
                }
                else {
                    winner = victim;
                    loser = ch;
                }
                loser->charge = 0;
			    loser->wait_skill_sn = 0;
			    loser->wait_skill_vo = NULL;
			    loser->wait_skill_target = TARGET_NONE;
                act ("{BYou win the powerstruggle!{x", winner, NULL, loser, TO_CHAR);
                act ("{B$n defeats you in the powerstruggle!{x", winner, NULL, loser, TO_VICT);
                act ("{B$n defeats $N in a powerstruggle!{x", winner, NULL, loser, TO_NOTVICT);
                release(winner);
                victim->powerstruggle = NULL;
                ch->powerstruggle = NULL;
            }
            else
                release (ch);
        }
		else
			sendch("Charge a bit longer before releasing.\n\r", ch);
	}
}

void release (CHAR_DATA *ch) {
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
} 
   
   
void StripTrans (CHAR_DATA *pCh) {
    affect_strip (pCh, gsn_ssj1);
    affect_strip (pCh, gsn_ssj2);
    affect_strip (pCh, gsn_ssj3);
    affect_strip (pCh, gsn_ssj4);
    affect_strip (pCh, gsn_ssj5);
    affect_strip (pCh, gsn_mystic);
    affect_strip (pCh, gsn_superh);
    affect_strip (pCh, gsn_hypern);
    affect_strip (pCh, gsn_supern);
    affect_strip (pCh, gsn_selffuse);
    affect_strip (pCh, gsn_form1);
    affect_strip (pCh, gsn_form2);
    affect_strip (pCh, gsn_form3);
    affect_strip (pCh, gsn_form4);
    affect_strip (pCh, gsn_form5);
}

void do_ssj1 (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (get_skill(ch, gsn_ssj1) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_SSJ1) {
    	sendch ("You are already Super Saiya-jin Level 1.\n\r", ch);
    	return;
    }
    if (GetTrans(ch) == TRANS_NONE)
        bInc = TRUE;
    else
        bInc = FALSE;

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_ssj1;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 10;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);

    act ("{Y$n tenses as $s hair turns golden, and a burning golden aura surrounds $m.{x", ch, NULL, NULL, TO_ROOM);
    act ("{YYou tense as your hair turns golden, and a burning golden aura surrounds you.{x", ch, NULL, NULL, TO_CHAR);

	ResetDiff(ch);
    reset_after_trans (ch, bInc);
    return;
}


void do_ssj2 (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (get_skill(ch, gsn_ssj2) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_SSJ2) {
    	sendch ("You are already Super Saiya-jin Level 2.\n\r", ch);
    	return;
    }
    if (GetTrans(ch) == TRANS_NONE || GetTrans(ch) == TRANS_SSJ1)
        bInc = TRUE;
    else
        bInc = FALSE;

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_ssj2;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 20;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);

    act ("{YBolts of ki arc around $n, as $s hair grows and turns golden.{x", ch, NULL, NULL, TO_ROOM);
    act ("{YBolts of ki arc around you, as your hair grows and turns golden.{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
	reset_after_trans (ch, bInc);
    return;
}

void do_ssj3 (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (get_skill(ch, gsn_ssj3) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_SSJ3) {
    	sendch ("You are already Super Saiya-jin Level 3.\n\r", ch);
    	return;
    }
    if (GetTrans(ch) != TRANS_SSJ2 && GetTrans(ch) != TRANS_SSJ4) {
        sendch ("You cannot transform directly to Super Saiya-jin Level 3.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_NONE || GetTrans(ch) == TRANS_SSJ1 || GetTrans(ch) == TRANS_SSJ2)
        bInc = TRUE;
    else
        bInc = FALSE;

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_ssj3;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 30;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);

    act ("{YEyebrows disappearing, $n's hair grows longer and flows down $s back. An immense golden aura erupts around $m.{x", ch, NULL, NULL, TO_ROOM);
    act ("{YEyebrows disappearing, your hair grows longer and flows down your back. An immense golden aura erupts around you.{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
	reset_after_trans (ch, bInc);
    return;
}

void do_ssj4 (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (get_skill(ch, gsn_ssj4) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_SSJ4) {
    	sendch ("You are already Super Saiya-jin Level 4.\n\r", ch);
    	return;
    }
    if (GetTrans(ch) == TRANS_NONE || GetTrans(ch) == TRANS_SSJ1 || GetTrans(ch) == TRANS_SSJ2 || GetTrans(ch) == TRANS_SSJ3)
        bInc = TRUE;
    else
        bInc = FALSE;

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_ssj4;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 40;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);

    act ("{RHair begins to sprout everywhere on $n, except on $s chest. The hair begins to thicken and turn black, as a massive red glow envelops $m.{x", ch, NULL, NULL, TO_ROOM);
    act ("{RHair begins to sprout everywhere on you, except on your chest. The hair begins to thicken and turn black, as a massive red glow envelops you.{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
    reset_after_trans (ch, bInc);
    return;
}

void do_ssj5 (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;

    if (get_skill(ch, gsn_ssj5) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_SSJ5) {
    	sendch ("You are already Super Saiya-jin Level 5.\n\r", ch);
    	return;
    }

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_ssj5;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 100;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);

    act ("{CA beam of light drops down from the heavens. It swirls, wailing, spinning around $n.  A massive wave of energy erupts, pouring ki into $m body.  A scream.  Suddenly it all fades, and $n emerges, walking calmly, trails of ki streaking with every moment and energy crackling between $s fingertips.{x", ch, NULL, NULL, TO_ROOM);
    act ("{CA beam of light drops down from the heavens. It swirls, wailing, spinning around you.  A massive wave of energy erupts, pouring ki into your body.  You scream.  Suddenly it all fades, and you emerges, walking calmly, trails of ki streaking with every moment and energy crackling between your fingertips.{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
    reset_after_trans (ch, FALSE);
    return;
}

void do_mystic (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;

    if (get_skill (ch, gsn_mystic) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_MYSTIC) {
    	sendch ("You are already Mystic.\n\r", ch);
    	return;
    }

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_mystic;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 30;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);
    af.modifier = 10;
    af.location = APPLY_WIL;
    affect_to_char (ch, &af);
    af.location = APPLY_INT;
    affect_to_char (ch, &af);
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_SANCTUARY;
    affect_to_char (ch, &af);

    act ("{M$n glances around and suddenly grimaces as frightening power begins to build within $m.{x", ch, NULL, NULL, TO_ROOM);
    act ("{MYou glance around and suddenly grimace as frightening power begins to build within you.{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
    reset_after_trans (ch, TRUE);
    return;
}


void do_super (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }

    if (get_skill (ch, gsn_superh) > 0) {
        if (GetTrans(ch) == TRANS_SUPERH) {
            sendch ("You are already Super Human.\n\r", ch);
            return;
        }

        if (GetTrans(ch) == TRANS_NONE)
            bInc = TRUE;
        else
            bInc = FALSE;

        StripTrans (ch);

        af.skill_lvl = 1;
		af.where = TO_AFFECTS;
		af.type = gsn_superh;
		af.duration = -1;
		af.bitvector = AFF_NONE;
		af.modifier = 15;
		af.location = APPLY_STR;
		affect_to_char (ch, &af);
		af.location = APPLY_DEX;
		affect_to_char (ch, &af);

        act ("{M$n's muscles erupt and swell with power!{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYour muscles erupt and swell with power!{x", ch, NULL, NULL, TO_CHAR);

    }
    else if (get_skill (ch, gsn_supern) > 0) {
        if (GetTrans(ch) == TRANS_SUPERN) {
            sendch ("You are already Super Namek.\n\r", ch);
            return;
        }

        if (GetTrans(ch) == TRANS_NONE || GetTrans(ch) == TRANS_HYPERN)
            bInc = TRUE;
        else
            bInc = FALSE;

        StripTrans (ch);

        af.skill_lvl = 1;
		af.where = TO_AFFECTS;
		af.type = gsn_supern;
		af.duration = -1;
		af.bitvector = AFF_NONE;
		af.location = APPLY_STR;
        af.modifier = 25;
		affect_to_char (ch, &af);
		af.location = APPLY_WIL;
        af.modifier = 35;
		affect_to_char (ch, &af);
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_REGENERATION;
		affect_to_char (ch, &af);

        act ("{GAn intense green aura develops and consumes $n!{x", ch, NULL, NULL, TO_ROOM);
        act ("{GAn intense green aura develops and consumes you!{x", ch, NULL, NULL, TO_CHAR);

    }
    else {
        sendch ("What?\n\r", ch);
        return;
    }

    ResetDiff(ch);
    reset_after_trans (ch, bInc);
    return;
}

void do_hyper (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (get_skill(ch, gsn_hypern) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_HYPERN) {
    	sendch ("You are already Hyper Namek.\n\r", ch);
    	return;
    }
    if (GetTrans(ch) == TRANS_NONE)
        bInc = TRUE;
    else
        bInc = FALSE;

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_hypern;
    af.skill_lvl = 1;
    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.modifier = 15;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.modifier = 20;
    af.location = APPLY_WIL;
    affect_to_char (ch, &af);
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_REGENERATION;
    affect_to_char (ch, &af);

    act ("{WA white aura starts to burn around $n!{x", ch, NULL, NULL, TO_ROOM);
    act ("{WA white aura starts to burn around you!{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
	reset_after_trans (ch, bInc);
    return;
}

void do_revert (CHAR_DATA *ch, char *argument) {
    switch (GetTrans(ch)) {
        case TRANS_SSJ1:
        act ("{Y$n's hair drops and loses its golden hue as $e reverts back to $s normal state.{x", ch, NULL, NULL, TO_ROOM);
        act ("{YYour hair drops and loses its golden hue as you revert back to your normal state.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_SSJ2:
        act ("{YHair shortening, the bolts of ki around $n slow to a stop as $e loses $s rage.{x", ch, NULL, NULL, TO_ROOM);
        act ("{YHair shortening, the bolts of ki around you slow to a stop as you lose your rage.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_SSJ3:
        act ("{Y$n's eyebrows and hair return to normal, and $e slowly loses $s golden aura.{x", ch, NULL, NULL, TO_ROOM);
        act ("{YYour eyebrows and hair return to normal, and you slowly lose your golden aura.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_SSJ4:
        act ("{RBoth $s long black mane and red fur suddenly disappear as $n drops back into a more subdued state.{x", ch, NULL, NULL, TO_ROOM);
        act ("{RBoth your long black mane and red fur suddenly disappear as you drop back into a more subdued state.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_SSJ5:
        act ("{CBeams of light radiate out of from $m.  With a sudden limpness of body, a great amount of energy exits from $n.  The crackling stops and the ki fades.  Fallen from the pinnacle of an avatar, $n sighs and straightens, looking ahead.  The discourse remains.{x", ch, NULL, NULL, TO_ROOM);
        act ("{CBeams of light radiate out of you.  With a sudden limpness of body, a great amount of energy exits from you.  The crackling stops and the ki fades.  Fallen from the pinnacle of an avatar, you sigh and straighten, looking ahead.  Despite the release, your discourse remains.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_MYSTIC:
        act ("{MThe frightening power within $n exits, replacing the contorted expression on $s face.{x", ch, NULL, NULL, TO_ROOM);
        act ("{MThe frightening power within you exit, replacing the contorted expression on your face.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_SUPERH:
        act ("{Mn's muscles contract and shrink back to their normal size.{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYour muscles contract and shrink back to their normal size.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_HYPERN:
        act ("{WThe white aura around $n fades away.{x", ch, NULL, NULL, TO_ROOM);
        act ("{WThe white aura around you fades away.{x", ch, NULL, NULL, TO_CHAR);
        break;
        case TRANS_SUPERN:
        act ("{GThe intense green aura around $n melts away to nothingness.{x", ch, NULL, NULL, TO_ROOM);
        act ("{GThe intense green aura around you melts away to nothingness.{x", ch, NULL, NULL, TO_CHAR);
        break;
        default:
        sendch ("What?\n\r", ch);
        return;
    }

    StripTrans (ch);

    ResetDiff(ch);
    reset_after_trans(ch, FALSE);
}


void do_upgrade (CHAR_DATA *ch, char *argument) {
    CHAR_DATA *mob;
    char buf[MAX_STRING_LENGTH];
    int cost, upgrade;
    int nDifficulty;
    int stat_bonus = 0, skill_bonus = 0;
    int i;

    if (IS_NPC (ch))
        return;

    if (race_lookup("android") != ch->race) {
        sendch ("You're not an android.\n\r", ch);
        return;
    }

    /*
     * Check for upgrader.
     */
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
        if (IS_NPC (mob) && IS_SET (mob->act, ACT_UPGRADE))
            break;

    if (mob == NULL) {
        sendch ("You can't do that here.\n\r", ch);
        return;
    }

    switch (GetTrans(ch)) {
        case TRANS_NONE:
            cost = 50000;
            stat_bonus = 10;
            skill_bonus = 10;
            nDifficulty = 1000;
            upgrade = TRANS_UPGRADE1;
            break;
        case TRANS_UPGRADE1:
            cost = 1000000;
            stat_bonus = 20;
            skill_bonus = 20;
            nDifficulty = 2500;
            upgrade = TRANS_UPGRADE2;
            break;
        case TRANS_UPGRADE2:
            cost = 500000000;
            stat_bonus = 40;
            skill_bonus = 40;
            nDifficulty = 7500;
            upgrade = TRANS_UPGRADE3;
            break;
        default:
            sendch ("You cannot be upgraded again!\n\r", ch);
            return;
    }

    if (!str_cmp(argument, "arm") || !str_cmp(argument, "leg")
        || !str_cmp(argument, "neuro") || !str_cmp(argument, "torso")
        || !str_cmp(argument, "skills") ) {
        if (ch->nDifficulty < nDifficulty) {
            sendch ("Your powerlevel is not high enough.\n\r", ch);
            return;
        }

        if (ch->zenni < cost) {
             sprintf (buf, "You need %d zenni.\n\r", cost);
             sendch (buf, ch);
             return;
        }

        ch->zenni -= cost;

        if (!str_cmp(argument, "arm"))
            ch->perm_stat[STAT_STR] += stat_bonus;
        if (!str_cmp(argument, "leg"))
            ch->perm_stat[STAT_DEX] += stat_bonus;
        if (!str_cmp(argument, "neuro")) {
            ch->perm_stat[STAT_WIL] += stat_bonus/2;
            ch->perm_stat[STAT_INT] += stat_bonus/2;
        }

        if (!str_cmp(argument, "skills"))
            for (i=0; i < MAX_SKILL; ++i)
                if (ch->pcdata->learned[i] > 0 && skill_table[i].bCanImprove)
                    ch->pcdata->learned[i] = UMIN(ch->pcdata->learned[i]+skill_bonus, 10);

        switch (GetTrans(ch)) {
            case TRANS_NONE:
                ch->pcdata->learned[gsn_upgrade1] = 1;
                break;
            case TRANS_UPGRADE1:
                ch->pcdata->learned[gsn_upgrade2] = 1;
                break;
            case TRANS_UPGRADE2:
                ch->pcdata->learned[gsn_upgrade3] = 1;
                break;
        }
        sendch ("You have been upgraded to the next level!\n\r", ch);
        ResetDiff(ch);
        return;
    }

    if (ch->nDifficulty < nDifficulty)
        sendch("Sorry, your powerlevel is not great enough for an upgrade.\n\r", ch);
    else {
        sendch("Your powerlevel is high enough.\n\r", ch);
        sendch("The following upgrades are availible:\n\r", ch);
        sendch("  *arm implants (strength)\n\r", ch);
        sendch("  *leg implants (dexterity)\n\r", ch);
        sendch("  *neuro implants (wisdom and intelligence)\n\r", ch);
        sendch("  *skills upgrade (increase all known skills)\n\r", ch);
        sprintf(buf, "An upgrade will require %d zenni.\n\r", cost);
        sendch(buf, ch);
        sendch ("Type <upgrade arm/leg/neuro/torso/skills> if you're sure you wish to upgrade.\n\r", ch);
    }
    return;
}

void do_selffuse (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
    bool bInc;

    if (get_skill(ch, gsn_selffuse) < 1) {
        sendch ("What?\n\r", ch);
	    return;
    }
    if (GetTrans(ch) == TRANS_KAIOKEN) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }
    if (GetTrans(ch) == TRANS_SELFFUSE) {
    	sendch ("You are already Self-fused.\n\r", ch);
    	return;
    }
    if (GetTrans(ch) == TRANS_NONE)
        bInc = TRUE;
    else
        bInc = FALSE;

    StripTrans (ch);

    af.where = TO_AFFECTS;
    af.type = gsn_selffuse;
    af.skill_lvl = 4;
    af.duration = 10;
    af.bitvector = AFF_NONE;
    af.modifier = get_curr_stat (ch, STAT_STR);
    af.location = APPLY_STR;
    affect_to_char (ch, &af);
    af.modifier = get_curr_stat (ch, STAT_DEX);
    af.location = APPLY_DEX;
    affect_to_char (ch, &af);
    af.modifier = get_curr_stat (ch, STAT_INT);
    af.location = APPLY_INT;
    affect_to_char (ch, &af);
    af.modifier = get_curr_stat (ch, STAT_WIL);
    af.location = APPLY_WIL;
    affect_to_char (ch, &af);

    act ("{m$n splits $mself into two, and then fuses together, greatly increasing $s power!{x", ch, NULL, NULL, TO_ROOM);
    act ("{mYou split yourself into two, and then fuse together, greatly increasing your power!{x", ch, NULL, NULL, TO_CHAR);

    ResetDiff(ch);
	reset_after_trans (ch, TRUE);
    return;
}

void do_transup (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;

    if (race_lookup("icer") != ch->race) {
        sendch ("What?\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_KAIOKEN)) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }

    if (GetTrans(ch) == TRANS_NONE && get_skill (ch, gsn_form2) > 0) {
        StripTrans (ch);
        af.type = gsn_form2;
        af.modifier = 30;
        act ("{M$n begins to swell and grow massive, $s horns turning into long spikes!{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYou begin to swell and grow massive, your horns turning into long spikes!{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER2 && get_skill (ch, gsn_form3) > 0) {
        StripTrans (ch);
        af.type = gsn_form3;
        af.modifier = 60;
        act ("{M$n's size shrinks slightly, but claws grow out of $s arms and $s forhead extends, becoming very long!{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYour size shrinks slightly, but claws grow out of your arms and your forhead extends, becoming very long!{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER3 && get_skill (ch, gsn_form4) > 0) {
        StripTrans (ch);
        af.type = gsn_form4;
        af.modifier = 90;
        act ("{MA blinding light envelops $n, and $e becomes dramatically smaller! The spikes on $s arms fade away, and $s head returns to its normal rounded shape!{x", ch, NULL, NULL, TO_ROOM);
        act ("{MA blinding light envelops you, and you become dramatically smaller! The spikes on your arms fade away, and your head returns to its normal rounded shape!{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER4 && get_skill (ch, gsn_form5) > 0) {
        StripTrans (ch);
        af.type = gsn_form5;
        af.modifier = 120;
        act ("{M$n's muscles erupt with power as spikes grow out of $s head and arms, and plates form around $s wrists! $n swells up greatly with power!{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYour muscles erupt with power as spikes grow out of your head and arms, and plates form around your wrists! You swell up greatly with power!{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER5) {
        sendch ("You cannot transform into anything greater.\n\r", ch);
        return;
    }
    else {
        sendch ("Your powers are not yet great enough.\n\r", ch);
        return;
    }

    af.skill_lvl = 1;
    af.where = TO_AFFECTS;

    af.duration = -1;
    af.bitvector = AFF_NONE;
    af.location = APPLY_STR;
    affect_to_char (ch, &af);

    ResetDiff(ch);
    reset_after_trans(ch, TRUE);
    return;
}

void do_transdown (CHAR_DATA *ch, char *argument) {
    AFFECT_DATA af;
	bool bApply;

    if (race_lookup("icer") != ch->race) {
        sendch ("What?\n\r", ch);
	    return;
    }

    if (IS_AFFECTED(ch, AFF_KAIOKEN)) {
        sendch ("You cannot transform while using kaioken.\n\r", ch);
        return;
    }

    if (GetTrans(ch) == TRANS_NONE) {
        sendch ("You cannot transform into anything of less power!\n\r", ch);
        return;
    }
    else if (GetTrans(ch) == TRANS_ICER2) {
        bApply = FALSE;
		act ("{MSpikes growing quite small, $n's body follows, as $e shrinks and loses power.{x", ch, NULL, NULL, TO_ROOM);
        act ("{MSpikes growing quite small, your body follows, as you shrink and lose power.{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER3) {
        bApply = TRUE;
        af.type = gsn_form2;
        af.modifier = 30;
		act ("{MThe spikes on $n shrink as $s head and body become smaller.{x", ch, NULL, NULL, TO_ROOM);
        act ("{MThe spikes on you shrink as your head and body become smaller.{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER4) {
        bApply = TRUE;
        af.type = gsn_form3;
        af.modifier = 60;
        act ("{M$n grows much larger, as spikes grow on $s arms and $s head elongates.{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYou grow much larger, as spikes grow on your arms and your head elongates.{x", ch, NULL, NULL, TO_CHAR);
    }
    else if (GetTrans(ch) == TRANS_ICER5) {
        bApply = TRUE;
        af.type = gsn_form4;
        af.modifier = 90;
        act ("{M$n's spikes disappear and $e becomes much smaller, while $s muscles shrink.{x", ch, NULL, NULL, TO_ROOM);
        act ("{MYour spikes disappear and you become much smaller, while your muscles shrink.{x", ch, NULL, NULL, TO_CHAR);
    }
    else {
        sendch ("!error!\n\r", ch);
        return;
    }

    StripTrans (ch);

    if (bApply) {
        af.skill_lvl = 1;
		af.where = TO_AFFECTS;
		af.duration = -1;
		af.bitvector = AFF_NONE;
		af.location = APPLY_STR;
		affect_to_char (ch, &af);
	}
    ResetDiff(ch);
	reset_after_trans (ch, FALSE);
    return;
}


void do_stance (CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        sendch ("Use stance <stance> to set your fighting stance.\n\r", ch);
        sendch ("You can use any of these stances:\n\r", ch);
        sendch ("   normal offensive defensive", ch);
        if (get_skill(ch, gsn_kamikaze) > 0)
            sendch (" kamikaze", ch);
        sendch ("\n\r\n\r", ch);
        sprintf (buf, "Your current stance is {W%s{x.\n\r", stance_table[ch->stance]);
        sendch (buf, ch);
        return;
    }

    if (!str_prefix(argument, "normal")) {
        if (IS_EXHAUSTED(ch)) {
		    sendch("You're too exhausted.\n\r", ch);
		    return;
	    }
        ch->stance = STANCE_NORMAL;
        sendch ("Stance set back to normal.\n\r", ch);
        act ("$n stands in a normal position.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (!str_prefix(argument, "offensive")) {
        if (IS_EXHAUSTED(ch)) {
		    sendch("You're too exhausted.\n\r", ch);
		    return;
	    }
        ch->stance = STANCE_OFFEN;
        sendch ("Stance set to offensive.\n\r", ch);
        act ("$n leans forward eagerly in an offensive stance.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (!str_prefix(argument, "defensive")) {
        if (IS_EXHAUSTED(ch)) {
		    sendch("You're too exhausted.\n\r", ch);
		    return;
	    }
        ch->stance = STANCE_DEFEN;
        sendch ("Stance set to defensive.\n\r", ch);
        act ("Stepping back, $n defensively begins protect $mself.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (!str_prefix(argument, "kamikaze")) {
        if (IS_EXHAUSTED(ch)) {
		    sendch("You're too exhausted.\n\r", ch);
		    return;
	    }
        if (get_skill(ch, gsn_kamikaze) > 0) {
            ch->stance = STANCE_KAMIK;
            sendch ("Stance set to kamikaze!\n\r", ch);
            act ("With no fear of danger, $n throws defense aside and $mself into a kamikaze stance!", ch, NULL, NULL, TO_ROOM);
            return;
        }
        else {
            sendch ("You do not know how to use that stance.\n\r", ch);
            return;
        }
    }

    // Generate usage
    do_function (ch, &do_stance, "");
}


void do_kaioken (CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];

    if (IS_TRANSFORMED(ch) && GetTrans(ch) != TRANS_KAIOKEN) {
        sendch ("You cannot use kaioken while transformed.\n\r", ch);
        return;
    }

    if (get_skill(ch, gsn_kaioken) < 1) {
        sendch ("You don't know kaioken.\n\r", ch);
        return;
    }

    if (IS_SET(ch->affected_by, AFF_KAIOKEN)) {
        affect_strip (ch, gsn_kaioken);
        REMOVE_BIT (ch->affected_by, AFF_KAIOKEN);
        act ("{RThe red, fiery haze around $n fades away.{x", ch, NULL, NULL, TO_ROOM);
        act ("{RThe red, fiery haze around you fades away.{x", ch, NULL, NULL, TO_CHAR);

    }
    else {
        AFFECT_DATA af;

        sprintf (buf, "{6You yell, '{7KAIOKEN %d!{6'{x", get_skill(ch, gsn_kaioken));
        act (buf, ch, NULL, NULL, TO_CHAR);
        sprintf (buf, "{6$n yells, '{7KAIOKEN %d!{6'{x", get_skill(ch, gsn_kaioken));
        act (buf, ch, NULL, NULL, TO_ROOM);

        act ("{RA red, fiery haze ignites around $n!{x", ch, NULL, NULL, TO_ROOM);
        act ("{RA red, fiery haze ignites around you!{x", ch, NULL, NULL, TO_CHAR);

        af.where = TO_AFFECTS;
        af.type = gsn_kaioken;
        af.skill_lvl = get_skill(ch, gsn_kaioken);
        af.duration = -1;
        af.bitvector = AFF_KAIOKEN;

        af.modifier = 10 + get_skill(ch, gsn_kaioken) / 10;
        af.location = APPLY_DEX;
        affect_to_char (ch, &af);

        af.location = APPLY_STR;
        affect_to_char (ch, &af);

        ResetDiff(ch);
    }

    return;
}


void do_fly (CHAR_DATA *ch, char *argument) {
    if (get_skill(ch, gsn_fly) < 1) {
        sendch ("You don't know how to fly.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_FLYING)) {
        affect_strip (ch, gsn_fly);
        REMOVE_BIT (ch->affected_by, AFF_FLYING);
        act ("{5$n gradually floats back down to the earth.{x", ch, NULL, NULL, TO_ROOM);
        act ("{5You gradually float back down to the earth.{x", ch, NULL, NULL, TO_CHAR);
    }
    else {
        AFFECT_DATA af;

        act ("{5$n begins to fly!{x", ch, NULL, NULL, TO_ROOM);
        act ("{5You begin to fly!{x", ch, NULL, NULL, TO_CHAR);

        af.where = TO_AFFECTS;
        af.type = gsn_fly;
        af.skill_lvl = get_skill(ch, gsn_fly);
        af.duration = -1;
        af.location = 0;
        af.modifier = 0;
        af.bitvector = AFF_FLYING;
        affect_to_char (ch, &af);
    }

    return;
}

void do_finishingmove (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_FINISHMOVE))
    {
        REMOVE_BIT (ch->act, PLR_FINISHMOVE);
        sendch ("Finishing move off.\n\r", ch);
    }
    else
    {
        SET_BIT (ch->act, PLR_FINISHMOVE);
        sendch ("Finishing move on.\n\r", ch);
    }

    return;
}

void do_customskill (CHAR_DATA *ch, char *argument) {
	char buf[MAX_INPUT_LENGTH],
		 arg1[MAX_INPUT_LENGTH],
		 arg2[MAX_INPUT_LENGTH];

	if (IS_NPC (ch))
		return;
	
	if (ch->pcdata->nCsPoints < 1) {
        sendch ("You cannot create, or edit, your custom skill.\n\r", ch);
		return;
	}

	argument = one_argument (argument, arg1);
	strcpy (arg2, argument);
	
	if (arg1[0] != '\0' && !str_cmp (arg1, "done")) {
		if (ch->pcdata->bCsConfirm) {
            sendch ("Ok. You have finished editing your custom skill.\n\r", ch);
			ch->pcdata->nCsPoints = 0;
			return;
		}
		sendch ("Are you sure you want to finish you skill?\n\r"
			    "Enter <customskill done> if you are sure, or <customskill> to cancel.\n\r", ch);
		ch->pcdata->bCsConfirm = TRUE;
		return;
	}
    else if (ch->pcdata->bCsConfirm) {
		sendch ("Confirmation to finish the custom skill is canceled.\n\r", ch);
		ch->pcdata->bCsConfirm = FALSE;
		return;
	}
	else if (arg1[0] != '\0' && !str_cmp (arg1, "name")) {
        if (arg2[0] == '\0') {
			sendch ("No name was passed.\n\r"
				    "Use <customskill name [name]>.\n\r", ch);
			return;
		}
		if (ch->pcdata->szCsName)
			free_string(ch->pcdata->szCsName);
		ch->pcdata->szCsName = str_dup(arg2);
		sendch ("Skill name was set.\n\r", ch);
		return;
	}
	else if (arg1[0] != '\0' && !str_cmp (arg1, "flag")) {
        // Probably could use a table of data for the flags here
		char arg[MAX_INPUT_LENGTH];

        one_argument (arg2, arg);

		if (arg[0] != '\0' && !str_cmp (arg, "quick")) {
            if (IS_SET (ch->pcdata->lCsFlags, CS_QUICK)) {
                ch->pcdata->nCsPoints += 10;
				sendch ("Quick flag removed.\n\r", ch);
				return;
			}
			if (ch->pcdata->nCsPoints < 10) {
                sendch ("You need 10 points for the quick flag.\n\r", ch);
				return;
			}
            SET_BIT (ch->pcdata->lCsFlags, CS_QUICK);
			ch->pcdata->nCsPoints -= 10;
			sendch ("Quick flag added.\n\r", ch);
			return;
		}
		if (arg[0] != '\0' && !str_cmp (arg, "simple")) {
            if (IS_SET (ch->pcdata->lCsFlags, CS_SIMPLE)) {
                ch->pcdata->nCsPoints += 10;
				sendch ("Simple flag removed.\n\r", ch);
				return;
			}
			if (ch->pcdata->nCsPoints < 10) {
                sendch ("You need 10 points for the simple flag.\n\r", ch);
				return;
			}
            SET_BIT (ch->pcdata->lCsFlags, CS_SIMPLE);
			ch->pcdata->nCsPoints -= 10;
			sendch ("Simple flag added.\n\r", ch);
			return;
		}
		printf_to_char (ch, 
				"No flag was passed. Use <customskill name [flags]>.\n\r"
				"Possble flags are (followed by required points):\n\r"
				"  quick(10)  -quick to use, short delay\n\r"
				"  simple(10) -easier to improve\n\r"
				"You have %d points remaining.\n\r", ch->pcdata->nCsPoints);
		return;
	}

	if (ch->pcdata->szCsName)
		printf_to_char (ch, "Skill Name [%s]\n\r", ch->pcdata->szCsName);
	else
		sendch ("Skill Name [none]\n\r", ch);

    buf[0] = '\0';
    if (ch->pcdata->lCsFlags & CS_QUICK)
        strcat (buf, " quick");
    if (ch->pcdata->lCsFlags & CS_SIMPLE)
        strcat (buf, " simple");
    printf_to_char (ch, "Flags [%s]\n\r", (buf[0] != '\0') ? buf + 1 : "none");

	printf_to_char (ch, "Points left [%d]\n\r", ch->pcdata->nCsPoints);
	
	sendch ("\n\rFields:\n\r"
		    "  name flag done\n\r", ch);
	sendch ("Use <customskill [field]>.\n\r", ch);

	return;
}


void do_pushup (CHAR_DATA *ch, char *argument) {
	if (IS_NPC (ch))
		return;
    if (IS_EXHAUSTED(ch)) {
        sendch ("You are too exhausted to do another!\n\r", ch);
        return;
    }
    sendch ("You do a set of pushups.\n\r", ch);
    act ("$n does a set of pushups.", ch, NULL, NULL, TO_ROOM);
    ki_loss (ch, 10);
    wait (ch, 15 * PULSE_SECOND);
    ImproveStat (ch, STAT_STR, TRUE,
                 ch->pcdata->nTrainCount >= 150 ? 0 : UMAX(1, ch->pcdata->nTrainCount / 25),
                 ch->nDifficulty);
    ++ch->pcdata->nTrainCount;
}

void do_meditate (CHAR_DATA *ch, char *argument) {
    if (IS_NPC (ch))
		return;
    if (IS_EXHAUSTED(ch)) {
        sendch ("You are too exhausted to continue!\n\r", ch);
        return;
    }
    sendch ("You sigh, focusing your will power...\n\r", ch);
    act ("$n sighs, focusing $s will power...", ch, NULL, NULL, TO_ROOM);
    ki_loss (ch, 10);
    wait (ch, 15 * PULSE_SECOND);
    ImproveStat (ch, STAT_WIL, TRUE,
                 ch->pcdata->nTrainCount >= 150 ? 0 : UMAX(1, ch->pcdata->nTrainCount / 25),
                 ch->nDifficulty);
    ++ch->pcdata->nTrainCount;
}

void do_study (CHAR_DATA *ch, char *argument) {
	if (IS_NPC (ch))
		return;
    if (IS_EXHAUSTED(ch)) {
        sendch ("You are too exhausted to continue!\n\r", ch);
        return;
    }
    sendch ("You study some complex material about cellular automata.\n\r", ch);
    act ("$n studies some complex material about cellular automata.", ch, NULL, NULL, TO_ROOM);
    ki_loss (ch, 10);
    wait (ch, 15 * PULSE_SECOND);
    ImproveStat (ch, STAT_INT, TRUE,
                 ch->pcdata->nTrainCount >= 150 ? 0 : UMAX(1, ch->pcdata->nTrainCount / 25),
                 ch->nDifficulty);
    ++ch->pcdata->nTrainCount;
}

void do_stretch (CHAR_DATA *ch, char *argument) {
	if (IS_NPC (ch))
		return;
    if (IS_EXHAUSTED(ch)) {
        sendch ("You are too exhausted to do another!\n\r", ch);
        return;
    }
    sendch ("You do a series of stretches.\n\r", ch);
    act ("$n does a series of stretches.", ch, NULL, NULL, TO_ROOM);
    ki_loss (ch, 10);
    wait (ch, 15 * PULSE_SECOND);
    ImproveStat (ch, STAT_DEX, TRUE,
                 ch->pcdata->nTrainCount >= 150 ? 0 : UMAX(1, ch->pcdata->nTrainCount / 25),
                 ch->nDifficulty);
    ++ch->pcdata->nTrainCount;
}

void do_teach (CHAR_DATA *ch, char *argument) {
    int nSn;
	char szArg[MAX_INPUT_LENGTH];
	char szBuf[MAX_STRING_LENGTH];
    
	if (IS_NPC (ch))
		return;
	if (ch->pcdata->pTeacher) {
        sendch ("You cannot teach because you are listening to someone else.\n\r", ch);
		return;
	}
	argument = one_argument (argument, szArg);
	if (szArg[0] == '\0') {
        if (ch->pcdata->nTeachSn < 0) {
		    sendch ("You are not teaching anything.\n\r", ch);
		    sendch ("Use <teach [skill]> to begin to teach something.\n\r", ch);
		}
		else
			printf_to_char (ch, "You are teaching %s.\n\r", skill_table[ch->pcdata->nTeachSn].name);
		return;
	}
	else if (!str_cmp (szArg, "stop")) {
        CHAR_DATA *pCur;
		sendch ("You stop teaching.\n\r", ch);
		act ("$n stops teaching.", ch, NULL, NULL, TO_ROOM);
		ch->pcdata->nTeachSn = -1;
		for (pCur = ch->in_room->people; pCur; pCur = pCur->next_in_room)
			if (!IS_NPC (pCur) && pCur->pcdata->pTeacher == ch)
				pCur->pcdata->pTeacher = NULL;
		return;
	}
	else if (get_skill (ch, gsn_teaching) < 1) {
        sendch ("You don't know how to teach.\n\r", ch);
		return;
	}
	else if ((nSn = skill_lookup(szArg)) < 0) {
        sendch ("That isn't a skill.\n\r", ch);
		return;
	}
	else if (get_skill (ch, nSn) < 1) {
        sendch ("You don't know that skill.\n\r", ch);
		return;
	}
	else if (!skill_table[nSn].bCanLearn) {
        sendch ("You can't teach that skill.\n\r", ch);
		return;
	}
	ch->pcdata->nTeachSn = nSn;
	printf_to_char (ch, "You begin to teach %s.\n\r", skill_table[nSn].name);
	sprintf (szBuf, "$n begins to teach %s.", skill_table[nSn].name);
	act (szBuf, ch, NULL, NULL, TO_ROOM);
}

void do_listen (CHAR_DATA *ch, char *argument) {
    CHAR_DATA *pVict;
	char szArg[MAX_INPUT_LENGTH];

	if (IS_NPC (ch))
		return;

	argument = one_argument (argument, szArg);
	if (szArg[0] == '\0') {
        bool bFound = FALSE;
		CHAR_DATA *pCur;
		
		sendch ("The following people are teaching:\n\r", ch);
        for (pCur = ch->in_room->people; pCur; pCur = pCur->next_in_room) {
            if (!IS_NPC (pCur) && pCur != ch && pCur->pcdata->nTeachSn > -1) {
				printf_to_char (ch, "  %s is teaching %s\n\r", pCur->name, skill_table[pCur->pcdata->nTeachSn].name);
				bFound = TRUE;
			}
		}
		if (!bFound)
			sendch ("  No one is teaching anything\n\r", ch);
		if (ch->pcdata->pTeacher)
			printf_to_char (ch, "You are listening to %s.\n\r", ch->pcdata->pTeacher->name);
		if (ch->pcdata->nTeachSn > -1)
			printf_to_char (ch, "You are teaching %s.\n\r", skill_table[ch->pcdata->nTeachSn].name);			
		return;
	}
	else if (!str_cmp (szArg, "stop")) {
		if (ch->pcdata->pTeacher) {
			act ("You stop listening to $N.", ch, NULL, ch->pcdata->pTeacher, TO_CHAR);	
			act ("$n stops listening to you.", ch, NULL, ch->pcdata->pTeacher, TO_VICT);
			act ("$n stops listening to $N.", ch, NULL, ch->pcdata->pTeacher, TO_NOTVICT);
			ch->pcdata->pTeacher = NULL;
		}        
		else
			sendch ("You aren't listening to anyone.\n\r", ch);
		return;
	}
	else if (ch->pcdata->nTeachSn > -1) {
        printf_to_char (ch, "You cannot listen because you are teaching %s.\n\r", skill_table[ch->pcdata->nTeachSn].name);
		return;
	}
    else if ((pVict = get_char_world (ch, szArg)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }
    else if (IS_NPC (pVict)) {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }
	else if (pVict->pcdata->nTeachSn < 0) {
        act ("$E isn't teaching anything.", ch, NULL, pVict, TO_CHAR);
		return;
	}
	else if (get_skill(pVict, pVict->pcdata->nTeachSn) < get_skill(ch, pVict->pcdata->nTeachSn)) {
        act ("You are more skilled at $t than $N.", ch, skill_table[pVict->pcdata->nTeachSn].name, pVict, TO_CHAR);
		return;
	}
    else if (get_skill(ch, pVict->pcdata->nTeachSn) < 1) {
        sendch ("You cannot listen if you don't have any prior knowledge in the skill.\n\r", ch);
        return;
    }
	else if (ch->pcdata->pTeacher) {
		act ("You stop listening to $N.", ch, NULL, ch->pcdata->pTeacher, TO_CHAR);	
		act ("$n stops listening to you.", ch, NULL, ch->pcdata->pTeacher, TO_VICT);
		act ("$n stops listening to $N.", ch, NULL, ch->pcdata->pTeacher, TO_NOTVICT);
	}
	ch->pcdata->pTeacher = pVict;
    act ("You begin to listen to $N.", ch, NULL, pVict, TO_CHAR);	
	act ("$n begins to listen to you.", ch, NULL, pVict, TO_VICT);
	act ("$n begins to listen to $N.", ch, NULL, pVict, TO_NOTVICT);
}

void do_suppress (CHAR_DATA * ch, char *argument) {
    long long int nPl;

    if (get_skill(ch, gsn_suppress) < 1) {
        sendch ("What?\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        sendch ("You are no longer suppressing your powerlevel.\n\r", ch);
        ch->llSuppressPl = -1;
        return;
    }
    nPl = atoll (argument);

    if (nPl < 0 || nPl > ch->llPl) {
        printf_to_char (ch,"Suppressed powerlevel must be between 0 and %Ld.\n\r",ch->llPl);
        return;
    }

    printf_to_char (ch, "You supress your powerlevel to %Ld.\n\r", nPl);
    wait (ch, 6*PULSE_SECOND);
    ch->llSuppressPl = nPl;

    return;
}

