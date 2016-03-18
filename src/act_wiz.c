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

/*   QuickMUD - The Lazy Man's ROM - $Id: act_wiz.c,v 1.3 2000/12/01 10:48:33 ring0 Exp $ */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>                /* For execl in copyover() */
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "olc.h"
#include "rand.h"

// For set weather
DECLARE_DO_FUN(do_wset);

/*
 * Stolen from save.c for reading in QuickMUD config stuff
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )			\
                if ( !str_cmp( word, literal ) )	\
		{					\
			field  = value;			\
			fMatch = TRUE;			\
			break;				\
		}

/*
 * Local functions.
 */
ROOM_INDEX_DATA *find_location args ((CHAR_DATA * ch, char *arg));

void do_wiznet (CHAR_DATA * ch, char *argument)
{
    int flag;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->wiznet, WIZ_ON))
        {
            sendch ("Signing off of Wiznet.\n\r", ch);
            REMOVE_BIT (ch->wiznet, WIZ_ON);
        }
        else
        {
            sendch ("Welcome to Wiznet!\n\r", ch);
            SET_BIT (ch->wiznet, WIZ_ON);
        }
        return;
    }

    if (!str_prefix (argument, "on"))
    {
        sendch ("Welcome to Wiznet!\n\r", ch);
        SET_BIT (ch->wiznet, WIZ_ON);
        return;
    }

    if (!str_prefix (argument, "off"))
    {
        sendch ("Signing off of Wiznet.\n\r", ch);
        REMOVE_BIT (ch->wiznet, WIZ_ON);
        return;
    }

    /* show wiznet status */
    if (!str_prefix (argument, "status"))
    {
        buf[0] = '\0';

        if (!IS_SET (ch->wiznet, WIZ_ON))
            strcat (buf, "off ");

        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
            if (IS_SET (ch->wiznet, wiznet_table[flag].flag))
            {
                strcat (buf, wiznet_table[flag].name);
                strcat (buf, " ");
            }

        strcat (buf, "\n\r");

        sendch ("Wiznet status:\n\r", ch);
        sendch (buf, ch);
        return;
    }

    if (!str_prefix (argument, "show"))
        /* list of all wiznet options */
    {
        buf[0] = '\0';

        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
        {
            if (wiznet_table[flag].level <= ch->level)
            {
                strcat (buf, wiznet_table[flag].name);
                strcat (buf, " ");
            }
        }

        strcat (buf, "\n\r");

        sendch ("Wiznet options available to you are:\n\r", ch);
        sendch (buf, ch);
        return;
    }

    flag = wiznet_lookup (argument);

    if (flag == -1 || ch->level < wiznet_table[flag].level)
    {
        sendch ("No such option.\n\r", ch);
        return;
    }

    if (IS_SET (ch->wiznet, wiznet_table[flag].flag))
    {
        sprintf (buf, "You will no longer see %s on wiznet.\n\r",
                 wiznet_table[flag].name);
        sendch (buf, ch);
        REMOVE_BIT (ch->wiznet, wiznet_table[flag].flag);
        return;
    }
    else
    {
        sprintf (buf, "You will now see %s on wiznet.\n\r",
                 wiznet_table[flag].name);
        sendch (buf, ch);
        SET_BIT (ch->wiznet, wiznet_table[flag].flag);
        return;
    }

}

void wiznet (char *string, CHAR_DATA * ch, OBJ_DATA * obj,
             long flag, long flag_skip, int min_level)
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && IS_IMMORTAL (d->character)
            && IS_SET (d->character->wiznet, WIZ_ON)
            && (!flag || IS_SET (d->character->wiznet, flag))
            && (!flag_skip || !IS_SET (d->character->wiznet, flag_skip))
            && d->character->level >= min_level && d->character != ch)
        {
            if (IS_SET (d->character->wiznet, WIZ_PREFIX))
                sendch ("{Z--> ", d->character);
            else
                sendch ("{Z", d->character);
            act_new (string, d->character, obj, ch, TO_CHAR, POS_DEAD);
            sendch ("{x", d->character);
        }
    }

    return;
}

void do_guild (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int clan;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        sendch ("Syntax: guild <char> <cln name>\n\r", ch);
        return;
    }
    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        sendch ("They aren't playing.\n\r", ch);
        return;
    }

    if (!str_prefix (arg2, "none"))
    {
        sendch ("They are now clanless.\n\r", ch);
        sendch ("You are now a member of no clan!\n\r", victim);
        victim->clan = 0;
        return;
    }

    if ((clan = clan_lookup (arg2)) == 0)
    {
        sendch ("No such clan exists.\n\r", ch);
        return;
    }

    if (clan_table[clan].independent)
    {
        sprintf (buf, "They are now a %s.\n\r", clan_table[clan].name);
        sendch (buf, ch);
        sprintf (buf, "You are now a %s.\n\r", clan_table[clan].name);
        sendch (buf, victim);
    }
    else
    {
        sprintf (buf, "They are now a member of clan %s.\n\r",
                 capitalize (clan_table[clan].name));
        sendch (buf, ch);
        sprintf (buf, "You are now a member of clan %s.\n\r",
                 capitalize (clan_table[clan].name));
    }

    victim->clan = clan;
}

/* RT nochannels command, for those spammers */
void do_nochannels (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Nochannel whom?", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOCHANNELS))
    {
        REMOVE_BIT (victim->comm, COMM_NOCHANNELS);
        sendch ("The gods have restored your channel priviliges.\n\r",
                      victim);
        sendch ("NOCHANNELS removed.\n\r", ch);
        sprintf (buf, "$N restores channels to %s", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOCHANNELS);
        sendch ("The gods have revoked your channel priviliges.\n\r",
                      victim);
        sendch ("NOCHANNELS set.\n\r", ch);
        sprintf (buf, "$N revokes %s's channels.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}


void do_smote (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *vch;
    char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE))
    {
        sendch ("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        sendch ("Emote what?\n\r", ch);
        return;
    }

    if (strstr (argument, ch->name) == NULL)
    {
        sendch ("You must include your name in an smote.\n\r", ch);
        return;
    }

    sendch (argument, ch);
    sendch ("\n\r", ch);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr (argument, vch->name)) == NULL)
        {
            sendch (argument, vch);
            sendch ("\n\r", vch);
            continue;
        }

        strcpy (temp, argument);
        temp[strlen (argument) - strlen (letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen (vch->name))
            {
                strcat (temp, "r");
                continue;
            }

            if (*letter == 's' && matches == strlen (vch->name))
            {
                matches = 0;
                continue;
            }

            if (matches == strlen (vch->name))
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen (vch->name))
                {
                    strcat (temp, "you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat (last, letter, 1);
                continue;
            }

            matches = 0;
            strcat (temp, last);
            strncat (temp, letter, 1);
            last[0] = '\0';
            name = vch->name;
        }

        sendch (temp, vch);
        sendch ("\n\r", vch);
    }

    return;
}

void do_bamfin (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC (ch))
    {
        smash_tilde (argument);

        if (argument[0] == '\0')
        {
            sprintf (buf, "Your poofin is %s\n\r", ch->pcdata->bamfin);
            sendch (buf, ch);
            return;
        }

        if (strstr (argument, ch->name) == NULL)
        {
            sendch ("You must include your name.\n\r", ch);
            return;
        }

        free_string (ch->pcdata->bamfin);
        ch->pcdata->bamfin = str_dup (argument);

        sprintf (buf, "Your poofin is now %s\n\r", ch->pcdata->bamfin);
        sendch (buf, ch);
    }
    return;
}

void do_bamfout (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC (ch))
    {
        smash_tilde (argument);

        if (argument[0] == '\0')
        {
            sprintf (buf, "Your poofout is %s\n\r", ch->pcdata->bamfout);
            sendch (buf, ch);
            return;
        }

        if (strstr (argument, ch->name) == NULL)
        {
            sendch ("You must include your name.\n\r", ch);
            return;
        }

        free_string (ch->pcdata->bamfout);
        ch->pcdata->bamfout = str_dup (argument);

        sprintf (buf, "Your poofout is now %s\n\r", ch->pcdata->bamfout);
        sendch (buf, ch);
    }
    return;
}



void do_deny (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Deny whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    SET_BIT (victim->act, PLR_DENY);
    sendch ("You are denied access!\n\r", victim);
    sprintf (buf, "$N denies access to %s", victim->name);
    wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    sendch ("OK.\n\r", ch);
    save_char_obj (victim);
    stop_fighting (victim, TRUE);
    do_function (victim, &do_quit, "");

    return;
}



void do_disconnect (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Disconnect whom?\n\r", ch);
        return;
    }

    if (is_number (arg))
    {
        int desc;

        desc = atoi (arg);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->descriptor == desc)
            {
                close_socket (d);
                sendch ("Ok.\n\r", ch);
                return;
            }
        }
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL)
    {
        act ("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR);
        return;
    }

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d == victim->desc)
        {
            close_socket (d);
            sendch ("Ok.\n\r", ch);
            return;
        }
    }

    logstr (LOG_BUG, "Do_disconnect: desc not found.", 0);
    sendch ("Descriptor not found!\n\r", ch);
    return;
}



void do_pardon (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        sendch ("Syntax: pardon <character> <hostile|thief>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    if (!str_cmp (arg2, "hostile"))
    {
        if (IS_SET (victim->act, PLR_HOSTILE))
        {
            REMOVE_BIT (victim->act, PLR_HOSTILE);
            sendch ("Hostile flag removed.\n\r", ch);
            sendch ("You are no longer hostile.\n\r", victim);
        }
        return;
    }

    if (!str_cmp (arg2, "thief"))
    {
        if (IS_SET (victim->act, PLR_THIEF))
        {
            REMOVE_BIT (victim->act, PLR_THIEF);
            sendch ("Thief flag removed.\n\r", ch);
            sendch ("You are no longer a THIEF.\n\r", victim);
        }
        return;
    }

    sendch ("Syntax: pardon <character> <hostile|thief>.\n\r", ch);
    return;
}

void do_quest (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0') {
        sendch ("Syntax: quest <character/cancel>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "cancel")) {
        for (d = descriptor_list; d != NULL; d = d->next)
            if (d->connected == CON_PLAYING)
                if (IS_SET (d->character->act, PLR_QUEST))
					REMOVE_BIT (d->character->act, PLR_QUEST);
		sendch ("The quest has been canceled.\n\r", ch);
		return;
	}

	if ((victim = get_char_world (ch, arg)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim)) {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    if (IS_SET (victim->act, PLR_QUEST)) {
        REMOVE_BIT (victim->act, PLR_QUEST);
        act ("$E is no longer on a quest.", ch, NULL, victim, TO_CHAR);
    }
	else {
        SET_BIT (victim->act, PLR_QUEST);
        act ("$E is now on a quest.", ch, NULL, victim, TO_CHAR);
	}
    return;
}

void do_echo (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        sendch ("Global echo what?\n\r", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING)
        {
            if (d->character->level >= ch->level)
                sendch ("global> ", d->character);
            sendch (argument, d->character);
            sendch ("\n\r", d->character);
        }
    }

    return;
}



void do_recho (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        sendch ("Local echo what?\n\r", ch);

        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character->in_room == ch->in_room)
        {
            if (d->character->level >= ch->level)
                sendch ("local> ", d->character);
            sendch (argument, d->character);
            sendch ("\n\r", d->character);
        }
    }

    return;
}

void do_zecho (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        sendch ("Zone echo what?\n\r", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character->in_room != NULL && ch->in_room != NULL
            && d->character->in_room->area == ch->in_room->area)
        {
            if (d->character->level >= ch->level)
                sendch ("zone> ", d->character);
            sendch (argument, d->character);
            sendch ("\n\r", d->character);
        }
    }
}

void do_pecho (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument (argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0')
    {
        sendch ("Personal echo what?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("Target not found.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level && ch->level < CODER)
        sendch ("personal> ", victim);

    sendch (argument, victim);
    sendch ("\n\r", victim);
    sendch ("personal> ", ch);
    sendch (argument, ch);
    sendch ("\n\r", ch);
}


ROOM_INDEX_DATA *find_location (CHAR_DATA * ch, char *arg)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if (is_number (arg))
        return get_room_index (atoi (arg));

    if ((victim = get_char_world (ch, arg)) != NULL)
        return victim->in_room;

    if ((obj = get_obj_world (ch, arg)) != NULL)
        return obj->in_room;

    return NULL;
}



void do_transfer (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0')
    {
        sendch ("Transfer whom (and where)? Or, transfer all/quest.\n\r", ch);
        return;
    }

    if (!str_cmp (arg1, "all"))
    {
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && d->character != ch
                && d->character->in_room != NULL
                && can_see (ch, d->character))
            {
                char buf[MAX_STRING_LENGTH];
                sprintf (buf, "%s %s", d->character->name, arg2);
                do_function (ch, &do_transfer, buf);
            }
        }
        return;
    }

    if (!str_cmp (arg1, "quest"))
    {
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && d->character != ch
                && d->character->in_room != NULL
				&& IS_SET(d->character->act, PLR_QUEST)
                && can_see (ch, d->character))
            {
                char buf[MAX_STRING_LENGTH];
                sprintf (buf, "%s %s", d->character->name, arg2);
                do_function (ch, &do_transfer, buf);
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0')
    {
        location = ch->in_room;
    }
    else
    {
        if ((location = find_location (ch, arg2)) == NULL)
        {
            sendch ("No such location.\n\r", ch);
            return;
        }

        if (!is_room_owner (ch, location) && room_is_private (location)
            && ch->level < CODER)
        {
            sendch ("That room is private right now.\n\r", ch);
            return;
        }
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->in_room == NULL)
    {
        sendch ("They are in limbo.\n\r", ch);
        return;
    }

    if (victim->fighting != NULL)
        stop_fighting (victim, TRUE);
    act ("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);
    char_from_room (victim);
    char_to_room (victim, location);
    act ("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM);
    if (ch != victim)
        act ("$n has transferred you.", ch, NULL, victim, TO_VICT);
    do_function (victim, &do_look, "auto");
    sendch ("Ok.\n\r", ch);
}



void do_at (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    OBJ_DATA *on;
    CHAR_DATA *wch;

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        sendch ("At where what?\n\r", ch);
        return;
    }

    if ((location = find_location (ch, arg)) == NULL)
    {
        sendch ("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, location) && room_is_private (location)
        && ch->level < CODER)
    {
        sendch ("That room is private right now.\n\r", ch);
        return;
    }

    original = ch->in_room;
    on = ch->on;
    char_from_room (ch);
    char_to_room (ch, location);
    interpret (ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (wch == ch)
        {
            char_from_room (ch);
            char_to_room (ch, original);
            ch->on = on;
            break;
        }
    }

    return;
}



void do_goto (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;

    if (argument[0] == '\0')
    {
        sendch ("Goto where?\n\r", ch);
        return;
    }

    if ((location = find_location (ch, argument)) == NULL)
    {
        sendch ("No such location.\n\r", ch);
        return;
    }

    count = 0;
    for (rch = location->people; rch != NULL; rch = rch->next_in_room)
        count++;

    if (!is_room_owner (ch, location) && room_is_private (location)
        && (count > 1 || ch->level < CODER))
    {
        sendch ("That room is private right now.\n\r", ch);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting (ch, TRUE);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->level >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
            else
                act ("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
        }
    }

    char_from_room (ch);
    char_to_room (ch, location);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->level >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
            else
                act ("$n appears in a swirling mist.", ch, NULL, rch,
                     TO_VICT);
        }
    }

    do_function (ch, &do_look, "auto");
    return;
}

void do_violate (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;

    if (argument[0] == '\0')
    {
        sendch ("Goto where?\n\r", ch);
        return;
    }

    if ((location = find_location (ch, argument)) == NULL)
    {
        sendch ("No such location.\n\r", ch);
        return;
    }

    if (!room_is_private (location))
    {
        sendch ("That room isn't private, use goto.\n\r", ch);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting (ch, TRUE);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->level >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
            else
                act ("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
        }
    }

    char_from_room (ch);
    char_to_room (ch, location);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->level >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
            else
                act ("$n appears in a swirling mist.", ch, NULL, rch,
                     TO_VICT);
        }
    }

    do_function (ch, &do_look, "auto");
    return;
}

/* RT to replace the 3 stat commands */

void do_stat (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *location;
    CHAR_DATA *victim;

    string = one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  stat <name>\n\r", ch);
        sendch ("  stat obj <name>\n\r", ch);
        sendch ("  stat mob <name>\n\r", ch);
        sendch ("  stat room <number>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "room"))
    {
        do_function (ch, &do_rstat, string);
        return;
    }

    if (!str_cmp (arg, "obj"))
    {
        do_function (ch, &do_ostat, string);
        return;
    }

    if (!str_cmp (arg, "char") || !str_cmp (arg, "mob"))
    {
        do_function (ch, &do_mstat, string);
        return;
    }

    /* do it the old way */

    obj = get_obj_world (ch, argument);
    if (obj != NULL)
    {
        do_function (ch, &do_ostat, argument);
        return;
    }

    victim = get_char_world (ch, argument);
    if (victim != NULL)
    {
        do_function (ch, &do_mstat, argument);
        return;
    }

    location = find_location (ch, argument);
    if (location != NULL)
    {
        do_function (ch, &do_rstat, argument);
        return;
    }

    sendch ("Nothing by that name found anywhere.\n\r", ch);
	return;
}

void do_rstat (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door;

    one_argument (argument, arg);

    location = (arg[0] == '\0') ? ch->in_room : find_location (ch, arg);

    if (location == NULL)
    {
        sendch ("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, location) && ch->in_room != location
        && room_is_private (location) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        sendch ("That room is private right now.\n\r", ch);
        return;
    }

    sprintf (buf, "Name: '%s'\n\rArea: '%s'\n\r", location->name, location->area->name);
    sendch (buf, ch);

    sprintf (buf,
             "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Ki: %d\n\r",
             location->vnum,
             location->sector_type,
             location->light, location->heal_rate, location->ki_rate);
    sendch (buf, ch);

    sprintf (buf,
             "Room flags: %d.\n\rDescription:\n\r%s",
             location->room_flags, location->description);
    sendch (buf, ch);

    if (location->extra_descr != NULL)
    {
        EXTRA_DESCR_DATA *ed;

        sendch ("Extra description keywords: '", ch);
        for (ed = location->extra_descr; ed; ed = ed->next)
        {
            sendch (ed->keyword, ch);
            if (ed->next != NULL)
                sendch (" ", ch);
        }
        sendch ("'.\n\r", ch);
    }

    sendch ("Characters:", ch);
    for (rch = location->people; rch; rch = rch->next_in_room)
    {
        if (can_see (ch, rch))
        {
            sendch (" ", ch);
            one_argument (rch->name, buf);
            sendch (buf, ch);
        }
    }

    sendch (".\n\rObjects:   ", ch);
    for (obj = location->contents; obj; obj = obj->next_content)
    {
        sendch (" ", ch);
        one_argument (obj->name, buf);
        sendch (buf, ch);
    }
    sendch (".\n\r", ch);

    for (door = 0; door <= 5; door++)
    {
        EXIT_DATA *pexit;

        if ((pexit = location->exit[door]) != NULL)
        {
            sprintf (buf,
                     "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\n\rKeyword: '%s'.  Description: %s",
                     door,
                     (pexit->u1.to_room ==
                      NULL ? -1 : pexit->u1.to_room->vnum), pexit->key,
                     pexit->exit_info, pexit->keyword,
                     pexit->description[0] !=
                     '\0' ? pexit->description : "(none).\n\r");
            sendch (buf, ch);
        }
    }

    return;
}



void do_ostat (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Stat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_world (ch, argument)) == NULL)
    {
        sendch ("Nothing like that in hell, earth, or heaven.\n\r", ch);
        return;
    }

    sprintf (buf, "Name(s): %s\n\r", obj->name);
    sendch (buf, ch);

    sprintf (buf, "Vnum: %d  Type: %s  Resets: %d\n\r",
             obj->pIndexData->vnum,
             item_name (obj->item_type), obj->pIndexData->reset_num);
    sendch (buf, ch);

    sprintf (buf, "Short description: %s\n\rLong description: %s\n\r",
             obj->short_descr, obj->description);
    sendch (buf, ch);

    sprintf (buf, "Wear bits: %s\n\rExtra bits: %s\n\r",
             wear_bit_name (obj->wear_flags),
             extra_bit_name (obj->extra_flags));
    sendch (buf, ch);

    sprintf (buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\n\r",
             1, get_obj_number (obj),
             obj->weight, get_obj_weight (obj), get_true_weight (obj));
    sendch (buf, ch);

    sprintf (buf, "Power Level: %Ld  Cost: %d  Condition: %d  Timer: %d\n\r",
             obj->llPl, obj->cost, obj->condition, obj->timer);
    sendch (buf, ch);

    sprintf (buf,
             "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
             obj->in_room == NULL ? 0 : obj->in_room->vnum,
             obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr,
             obj->carried_by == NULL ? "(none)" :
             can_see (ch, obj->carried_by) ? obj->carried_by->name
             : "someone", obj->wear_loc);
    sendch (buf, ch);

    sprintf (buf, "Values: %d %d %d %d %d\n\r",
             obj->value[0], obj->value[1], obj->value[2], obj->value[3],
             obj->value[4]);
    sendch (buf, ch);

    /* now give out vital statistics as per identify */

    switch (obj->item_type)
    {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf (buf, "Level %d spells of:", obj->value[0]);
            sendch (buf, ch);

            if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL)
            {
                sendch (" '", ch);
                sendch (skill_table[obj->value[1]].name, ch);
                sendch ("'", ch);
            }

            if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL)
            {
                sendch (" '", ch);
                sendch (skill_table[obj->value[2]].name, ch);
                sendch ("'", ch);
            }

            if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            {
                sendch (" '", ch);
                sendch (skill_table[obj->value[3]].name, ch);
                sendch ("'", ch);
            }

            if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
            {
                sendch (" '", ch);
                sendch (skill_table[obj->value[4]].name, ch);
                sendch ("'", ch);
            }

            sendch (".\n\r", ch);
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            sprintf (buf, "Has %d(%d) charges of level %d",
                     obj->value[1], obj->value[2], obj->value[0]);
            sendch (buf, ch);

            if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            {
                sendch (" '", ch);
                sendch (skill_table[obj->value[3]].name, ch);
                sendch ("'", ch);
            }

            sendch (".\n\r", ch);
            break;

        case ITEM_DRINK_CON:
            sprintf (buf, "It holds %s-colored %s.\n\r",
                     liq_table[obj->value[2]].liq_color,
                     liq_table[obj->value[2]].liq_name);
            sendch (buf, ch);
            break;


        case ITEM_WEAPON:
            sendch ("Weapon type is ", ch);
            switch (obj->value[0])
            {
                case (WEAPON_EXOTIC):
                    sendch ("exotic\n\r", ch);
                    break;
                case (WEAPON_SWORD):
                    sendch ("sword\n\r", ch);
                    break;
                case (WEAPON_DAGGER):
                    sendch ("dagger\n\r", ch);
                    break;
                case (WEAPON_SPEAR):
                    sendch ("spear/staff\n\r", ch);
                    break;
                case (WEAPON_MACE):
                    sendch ("mace/club\n\r", ch);
                    break;
                case (WEAPON_AXE):
                    sendch ("axe\n\r", ch);
                    break;
                case (WEAPON_FLAIL):
                    sendch ("flail\n\r", ch);
                    break;
                case (WEAPON_WHIP):
                    sendch ("whip\n\r", ch);
                    break;
                case (WEAPON_POLEARM):
                    sendch ("polearm\n\r", ch);
                    break;
                default:
                    sendch ("unknown\n\r", ch);
                    break;
            }
            sprintf (buf, "Damage is %d-%d (average %d)\n\r",
	            obj->value[1], obj->value[2],
                (obj->value[1] + obj->value[2]) / 2);
            sendch (buf, ch);

            sprintf (buf, "Damage noun is %s.\n\r",
                     (obj->value[3] > 0
                      && obj->value[3] <
                      MAX_DAMAGE_MESSAGE) ? attack_table[obj->value[3]].noun :
                     "undefined");
            sendch (buf, ch);

            if (obj->value[4])
            {                    /* weapon flags */
                sprintf (buf, "Weapons flags: %s\n\r",
                         weapon_bit_name (obj->value[4]));
                sendch (buf, ch);
            }
            break;

        case ITEM_ARMOR:
            sprintf (buf,
                     "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
                     obj->value[0], obj->value[1], obj->value[2],
                     obj->value[3]);
            sendch (buf, ch);
            break;

        case ITEM_CONTAINER:
            sprintf (buf, "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
                     obj->value[0], obj->value[3],
                     cont_bit_name (obj->value[1]));
            sendch (buf, ch);
            if (obj->value[4] != 100)
            {
                sprintf (buf, "Weight multiplier: %d%%\n\r", obj->value[4]);
                sendch (buf, ch);
            }
            break;
    }


    if (obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL)
    {
        EXTRA_DESCR_DATA *ed;

        sendch ("Extra description keywords: '", ch);

        for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
        {
            sendch (ed->keyword, ch);
            if (ed->next != NULL)
                sendch (" ", ch);
        }

        for (ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next)
        {
            sendch (ed->keyword, ch);
            if (ed->next != NULL)
                sendch (" ", ch);
        }

        sendch ("'\n\r", ch);
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        sprintf (buf, "Affects %s by %Ld, skill level %d",
                 affect_loc_name (paf->location), paf->modifier, paf->skill_lvl);
        sendch (buf, ch);
        if (paf->duration > -1)
            sprintf (buf, ", %d hours.\n\r", paf->duration);
        else
            sprintf (buf, ".\n\r");
        sendch (buf, ch);
        if (paf->bitvector)
        {
            switch (paf->where)
            {
                case TO_AFFECTS:
                    sprintf (buf, "Adds %s affect.\n",
                             affect_bit_name (paf->bitvector));
                    break;
                case TO_WEAPON:
                    sprintf (buf, "Adds %s weapon flags.\n",
                             weapon_bit_name (paf->bitvector));
                    break;
                case TO_OBJECT:
                    sprintf (buf, "Adds %s object flag.\n",
                             extra_bit_name (paf->bitvector));
                    break;
                case TO_IMMUNE:
                    sprintf (buf, "Adds immunity to %s.\n",
                             imm_bit_name (paf->bitvector));
                    break;
                case TO_RESIST:
                    sprintf (buf, "Adds resistance to %s.\n\r",
                             imm_bit_name (paf->bitvector));
                    break;
                case TO_VULN:
                    sprintf (buf, "Adds vulnerability to %s.\n\r",
                             imm_bit_name (paf->bitvector));
                    break;
                default:
                    sprintf (buf, "Unknown bit %d: %d\n\r",
                             paf->where, paf->bitvector);
                    break;
            }
            sendch (buf, ch);
        }
    }

    if (!obj->enchanted)
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            sprintf (buf, "Affects %s by %Ld, skill level %d.\n\r",
                     affect_loc_name (paf->location), paf->modifier, paf->skill_lvl);
            sendch (buf, ch);
            if (paf->bitvector)
            {
                switch (paf->where)
                {
                    case TO_AFFECTS:
                        sprintf (buf, "Adds %s affect.\n",
                                 affect_bit_name (paf->bitvector));
                        break;
                    case TO_OBJECT:
                        sprintf (buf, "Adds %s object flag.\n",
                                 extra_bit_name (paf->bitvector));
                        break;
                    case TO_IMMUNE:
                        sprintf (buf, "Adds immunity to %s.\n",
                                 imm_bit_name (paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf (buf, "Adds resistance to %s.\n\r",
                                 imm_bit_name (paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf (buf, "Adds vulnerability to %s.\n\r",
                                 imm_bit_name (paf->bitvector));
                        break;
                    default:
                        sprintf (buf, "Unknown bit %d: %d\n\r",
                                 paf->where, paf->bitvector);
                        break;
                }
                sendch (buf, ch);
            }
        }

    return;
}



void do_skillstat (CHAR_DATA * ch, char *argument) {
    BUFFER *buffer;
    int sn;
    bool found=FALSE, bColumn1=TRUE;
    char buf[MAX_STRING_LENGTH];
	char buf_temp[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
    
	one_argument (argument, arg);

    if (arg[0] == '\0') {
        sendch ("SkillStat whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, argument)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

	sprintf(buf, "  ");

    for (sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == NULL) 
            break;

        if (get_skill(victim, sn) > 0) {
            found = TRUE;
			sprintf (buf_temp, "%-20s %3d          ", skill_table[sn].name, get_skill(victim, sn));
			strcat (buf, buf_temp);
			if ((bColumn1=!bColumn1))
				strcat (buf, "\n\r  ");
        }
    }

    if (!found) {
        sendch ("No skills found.\n\r", ch);
        return;
    }

    buffer = new_buf ();
    add_buf (buffer, "Skills:\n\r");
	add_buf (buffer, buf);
    add_buf (buffer, "\n\r");
    page_to_char (buf_string (buffer), ch);
    free_buf (buffer);
}

void do_mstat( CHAR_DATA *ch, char *argument ) {
    char buf[MAX_STRING_LENGTH];
    // extra bufs for some formatting
    char buf1[MAX_STRING_LENGTH],
        buf2[MAX_STRING_LENGTH];
      //buf3[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0') {
        sendch ("Stat whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, argument)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf( buf,"{cName :{W %-15s {cSex   :{W %-14s {cRace :{W %-15s\n\r",
             victim->name, victim->sex == 0 ? "sexless" : victim->sex == 1 ? "male" : "female", race_table[victim->race].name);
    sendch( buf, ch );
    sprintf( buf,"{cAge  :{W %-15d {cHours :{W %-13d\n\r",
             get_age(victim), (( victim->played + (int) (current_time - victim->logon)) / 3600));
    sendch( buf, ch );

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    if (GetTrans(victim) != TRANS_NONE) {
        sendch("{cTransformed into a ", ch);
        sendch("{R", ch);
        sendch(trans_table[GetTrans(victim)], ch);
        sendch("{c\n\r", ch);
    }
    if (IS_FUSED(victim)) {
        sprintf (buf, "{cFused for {W%d{c hours.\n\r", victim->fuse_count);
        sendch (buf, ch);
    }

    sprintf( buf, "{cPlvl :{W %Ld/%Ld (%d%%)\n\r", victim->nCurPl * victim->llPl / 100, victim->llPl, victim->nCurPl);
    sendch( buf, ch );
    if (victim->llSuppressPl > -1)
        printf_to_char (victim, "{cSuppressed Plvl: {W%Ld\n\r", victim->llSuppressPl);
	sprintf (buf, "{cDifficulty: {W%d\n\r", victim->nDifficulty);
	sendch (buf, ch);
    sprintf( buf, "{cHps  :{W %d/%d\n\r", victim->hit, victim->max_hit );
    sendch( buf, ch );
    sprintf( buf, "{cKi   :{W %d/%d\n\r", victim->ki, victim->max_ki );
    sendch( buf, ch );

    sprintf( buf, "{cHitroll:{W %-13d  {cDamroll:{W %-15d\n\r",
	     GET_HITROLL(victim), GET_DAMROLL(victim) );
    sendch( buf, ch );

    sprintf( buf,"{cStr:{W %d(%d)  {cInt:{W %d(%d)  {cWil:{W %d(%d)  {cDex:{W %d(%d)  {cCha:{W %d(%d)\n\r",
	    victim->perm_stat[STAT_STR], get_curr_stat(victim,STAT_STR),
	    victim->perm_stat[STAT_INT], get_curr_stat(victim,STAT_INT),
	    victim->perm_stat[STAT_WIL], get_curr_stat(victim,STAT_WIL),
	    victim->perm_stat[STAT_DEX], get_curr_stat(victim,STAT_DEX),
	    victim->perm_stat[STAT_CHA], get_curr_stat(victim,STAT_CHA) );
    sendch( buf, ch );

    sprintf( buf,"{cArmor: {Wpierce: %d  bash: %d  slash: %d  ki: %d\n\r",
	     GET_AC(victim,AC_PIERCE),
             GET_AC(victim,AC_BASH),
	     GET_AC(victim,AC_SLASH),
	     GET_AC(victim,AC_EXOTIC));
    sendch(buf,ch);

    sprintf (buf, "{cIn a {W%s {cstance.\n\r", stance_table[victim->stance]);
    sendch (buf, ch);

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf( buf, "{cAlignment:{W %d  ", victim->alignment );
    sendch( buf, ch );
    sendch( "{cwhich is ", ch );
         if ( victim->alignment >  900 ) sendch( "{Wangelic.{x\n\r", ch );
    else if ( victim->alignment >  700 ) sendch( "{Wsaintly.{x\n\r", ch );
    else if ( victim->alignment >  350 ) sendch( "{wgood.{x\n\r",    ch );
    else if ( victim->alignment >  100 ) sendch( "{wkind.{x\n\r",    ch );
    else if ( victim->alignment > -100 ) sendch( "{cneutral.{x\n\r", ch );
    else if ( victim->alignment > -350 ) sendch( "{wmean.{x\n\r",    ch );
    else if ( victim->alignment > -700 ) sendch( "{bevil.{x\n\r",    ch );
    else if ( victim->alignment > -900 ) sendch( "{rdemonic.{x\n\r", ch );
    else                                 sendch( "{rsatanic.{x\n\r", ch );

    sprintf( buf,"{cZenni :{W %-15ld\n\r", victim->zenni );
    sendch( buf, ch );

    sprintf( buf1, "{cItems:{W %d/%d", victim->carry_number, can_carry_n(victim) );
    sprintf( buf2, "{cWeight:{W %ld/%d", get_carry_weight(victim) / 10, can_carry_w(victim) / 10 );
    sprintf( buf, "%-27s%-27s\n\r", buf1, buf2 );
    sendch( buf, ch );

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf( buf,"{cWimpy:{W %d hps\n\r", victim->wimpy );
    sendch( buf, ch );

    /* RT wizinvis and holy light */
    if ( IS_IMMORTAL(victim)) {
        sprintf(buf, "{cHoly Light:{W %-10s ", IS_SET(victim->act,PLR_HOLYLIGHT) ? "on" : "off");
        sendch(buf,ch);

        sprintf(buf, "{cCombat Info:{W %-10s ", IS_SET(victim->act,PLR_COMBATINFO) ? "on" : "off");
        sendch(buf,ch);

        if (victim->invis_level) {
            sprintf( buf, "{c  Invisible:{W level %d",victim->invis_level);
            sendch(buf,ch);
        }

        if (victim->incog_level) {
	        sprintf(buf," {c Incognito:{W level %d",victim->incog_level);
	        sendch(buf,ch);
        }
        sendch("\n\r",ch);
    }

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf (buf, "{cShort description: {W%s\n\r{cLong  description: {W%s",
             victim->short_descr,
             victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r");
    sendch (buf, ch);

    sprintf (buf, "{cLevel: {W%-5d {cWait:      {W%-8d {cRoom:     {W%d\n\r",
             victim->level, victim->wait, victim->in_room == NULL ? 0 : victim->in_room->vnum);
    sendch (buf, ch);


    sprintf (buf, "{cTransCount:     {W%d\n\r", victim->trans_count);
    sendch (buf, ch);

    sprintf (buf, "{cSaves: {W%-5d {cSize:      {W%-8s {cPosition: {W%s\n\r",
             victim->saving_throw, size_table[victim->size].name,
             position_table[victim->position].name);
    sendch (buf, ch);


    sprintf (buf, "{cFighting: {W%s\n\r",
             victim->fighting ? victim->fighting->name : "(none)");
    sendch (buf, ch);

    sprintf (buf, "{cMaster: {W%s  {cLeader: {W%s  {cPet: {W%s\n\r",
             victim->master ? victim->master->name : "(none)",
             victim->leader ? victim->leader->name : "(none)",
             victim->pet ? victim->pet->name : "(none)");
    sendch (buf, ch);

    if (victim->imm_flags) {
        sprintf (buf, "{cImmune: {W%s\n\r", imm_bit_name (victim->imm_flags));
        sendch (buf, ch);
    }

    if (victim->res_flags) {
        sprintf (buf, "{cResist: {W%s\n\r", imm_bit_name (victim->res_flags));
        sendch (buf, ch);
    }

    if (victim->vuln_flags) {
        sprintf (buf, "{cVulnerable: {W%s\n\r", imm_bit_name (victim->vuln_flags));
        sendch (buf, ch);
    }

    sprintf (buf, "{cAct: {W%s\n\r", act_bit_name (victim->act));
    sendch (buf, ch);

    if (victim->comm) {
        sprintf (buf, "{cComm: {W%s\n\r", comm_bit_name (victim->comm));
        sendch (buf, ch);
    }

    sprintf (buf, "{cForm: {W%s\n\r{cParts: {W%s\n\r",
             form_bit_name (victim->form), part_bit_name (victim->parts));
    sendch (buf, ch);

    if (victim->affected_by) {
        sprintf (buf, "{cAffected by {W%s\n\r",
                 affect_bit_name (victim->affected_by));
        sendch (buf, ch);
    }

    for (paf = victim->affected; paf != NULL; paf = paf->next) {
        sprintf (buf,
                 "{cSpell: '{W%s{c' modifies {W%s {cby {W%Ld {cfor {W%d {chours with bits {W%s{c, skill level {W%d{c.\n\r",
                 skill_table[(int) paf->type].name,
                 affect_loc_name (paf->location),
                 paf->modifier, paf->duration, affect_bit_name (paf->bitvector), paf->skill_lvl);
        sendch (buf, ch);
    }

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    if (!IS_NPC (victim)) {
        sprintf (buf, "{cAge: {W%d  {cPlayed: {W%d  {cLast Level: {W%d  {cTimer: {W%d\n\r",
                 get_age (victim), (int) (victim->played + current_time - victim->logon) / 3600,
                 victim->pcdata->last_level, victim->timer);
        sendch (buf, ch);

        sprintf (buf, "{cDrunk: {W%d\n\r", victim->pcdata->condition[COND_DRUNK]);
        sendch (buf, ch);

        sprintf (buf, "{cSecurity: {W%d\n\r", victim->pcdata->security);
        sendch (buf, ch);

        sendch("{Y-----------------------------------------------------------------{x\n\r",ch);
    }

    if (IS_NPC(victim)) {
        sprintf (buf, "{cVnum: {W%-5d    {cGroup: {W%-5d   {cCount: {W%-5d    {cKilled: {W%-5d\n\r",
                 victim->pIndexData->vnum, victim->group,
                 victim->pIndexData->count, victim->pIndexData->killed);
        sendch(buf, ch);

        if (victim->off_flags) {
            sprintf (buf, "{cOffense: {W%s\n\r", off_bit_name (victim->off_flags));
            sendch (buf, ch);
        }

        if (victim->spec_fun != 0) {
            sprintf (buf, "{cMobile has special procedure {W%s{c.\n\r",
                    spec_name (victim->spec_fun));
            sendch (buf, ch);
        }

        sendch("{Y-----------------------------------------------------------------{x\n\r",ch);
    }
    return;
}

void do_combatstat (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
	CHAR_DATA *vch, *vch_next;

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		sendch ("{Y=============================\n\r", ch);

		sprintf(buf, "{cName:{W%s\n\r",
			IS_NPC (vch) ? vch->short_descr : vch->name);
		sendch (buf, ch);

		sprintf(buf, "{cPL:{W%Ld of %Ld (%d%%)  {cHP:{W%d/%d  {cKI:{W%d/%d\n\r",
			vch->nCurPl * vch->llPl / 100, vch->llPl, vch->nCurPl, vch->hit, vch->max_hit, vch->ki, vch->max_ki);
		sendch (buf, ch);

		sprintf(buf, "{cBalance:{W%d/5  {cWait:{W%ds  {cSkillWait:{W%ds  {cCharge:{W%ds  {cSkill:{W%s\n\r",
			vch->balance, vch->wait / PULSE_SECOND, vch->wait_skill / PULSE_SECOND, vch->charge / PULSE_SECOND,
			vch->wait_skill_sn > 0 ? skill_table[vch->wait_skill_sn].name : "none");
		sendch (buf, ch);
    }
	sendch ("{Y=============================\n\r", ch);
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  vnum obj <name>\n\r", ch);
        sendch ("  vnum mob <name>\n\r", ch);
        sendch ("  vnum skill <skill or spell>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "obj"))
    {
        do_function (ch, &do_ofind, string);
        return;
    }

    if (!str_cmp (arg, "mob") || !str_cmp (arg, "char"))
    {
        do_function (ch, &do_mfind, string);
        return;
    }

    if (!str_cmp (arg, "skill") || !str_cmp (arg, "spell"))
    {
        do_function (ch, &do_slookup, string);
        return;
    }
    /* do both */
    do_function (ch, &do_mfind, argument);
    do_function (ch, &do_ofind, argument);
}


void do_mfind (CHAR_DATA * ch, char *argument)
{
    extern int top_mob_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Find whom?\n\r", ch);
        return;
    }

    fAll = FALSE;                /* !str_cmp( arg, "all" ); */
    found = FALSE;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for (vnum = 0; nMatch < top_mob_index; vnum++)
    {
        if ((pMobIndex = get_mob_index (vnum)) != NULL)
        {
            nMatch++;
            if (fAll || is_name (argument, pMobIndex->player_name))
            {
                found = TRUE;
                sprintf (buf, "[%5d] %s\n\r",
                         pMobIndex->vnum, pMobIndex->short_descr);
                sendch (buf, ch);
            }
        }
    }

    if (!found)
        sendch ("No mobiles by that name.\n\r", ch);

    return;
}



void do_ofind (CHAR_DATA * ch, char *argument)
{
    extern int top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Find what?\n\r", ch);
        return;
    }

    fAll = FALSE;                /* !str_cmp( arg, "all" ); */
    found = FALSE;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
    {
        if ((pObjIndex = get_obj_index (vnum)) != NULL)
        {
            nMatch++;
            if (fAll || is_name (argument, pObjIndex->name))
            {
                found = TRUE;
                sprintf (buf, "[%5d] %s\n\r",
                         pObjIndex->vnum, pObjIndex->short_descr);
                sendch (buf, ch);
            }
        }
    }

    if (!found)
        sendch ("No objects by that name.\n\r", ch);

    return;
}


void do_owhere (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 200;

    buffer = new_buf ();

    if (argument[0] == '\0')
    {
        sendch ("Find what?\n\r", ch);
        return;
    }

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (!can_see_obj (ch, obj) || !is_name (argument, obj->name))
            continue;

        found = TRUE;
        number++;

        for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

        if (in_obj->carried_by != NULL
			&& can_see (ch, in_obj->carried_by)
            && in_obj->carried_by->in_room != NULL)
            sprintf (buf, "%3d) %s is carried by %s [Room %d]\n\r", 
			         number, obj->short_descr, PERS (in_obj->carried_by, ch), in_obj->carried_by->in_room->vnum);
        else if (in_obj->in_room != NULL
                 && can_see_room (ch, in_obj->in_room)) 
		    sprintf (buf, "%3d) %s is in %s [Room %d]\n\r",
                     number, obj->short_descr, in_obj->in_room->name, in_obj->in_room->vnum);
        //else
        //    sprintf (buf, "%3d) %s is somewhere\n\r", number, obj->short_descr);

        buf[0] = UPPER (buf[0]);
        add_buf (buffer, buf);

        if (number >= max_found)
            break;
    }

    if (!found)
        sendch ("Nothing like that in heaven or earth.\n\r", ch);
    else
        page_to_char (buf_string (buffer), ch);

    free_buf (buffer);
}


void do_mwhere (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    if (argument[0] == '\0')
    {
        DESCRIPTOR_DATA *d;

        /* show characters logged */

        buffer = new_buf ();
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->character != NULL && d->connected == CON_PLAYING
                && d->character->in_room != NULL && can_see (ch, d->character)
                && can_see_room (ch, d->character->in_room))
            {
                victim = d->character;
                count++;
                if (d->original != NULL)
                    sprintf (buf,
                             "%3d) %s (in the body of %s) is in %s [%d]\n\r",
                             count, d->original->name, victim->short_descr,
                             victim->in_room->name, victim->in_room->vnum);
                else
                    sprintf (buf, "%3d) %s is in %s [%d]\n\r", count,
                             victim->name, victim->in_room->name,
                             victim->in_room->vnum);
                add_buf (buffer, buf);
            }
        }

        page_to_char (buf_string (buffer), ch);
        free_buf (buffer);
        return;
    }

    found = FALSE;
    buffer = new_buf ();
    for (victim = char_list; victim != NULL; victim = victim->next)
    {
        if (victim->in_room != NULL && is_name (argument, victim->name) && can_see (ch, victim))
        {
            found = TRUE;
            count++;
            sprintf (buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
                     IS_NPC (victim) ? victim->pIndexData->vnum : 0,
                     IS_NPC (victim) ? victim->short_descr : victim->name,
                     victim->in_room->vnum, victim->in_room->name);
            add_buf (buffer, buf);
        }
    }

    if (!found)
        act ("You didn't find any $T.", ch, NULL, argument, TO_CHAR);
    else
        page_to_char (buf_string (buffer), ch);

    free_buf (buffer);

    return;
}



void do_reboo (CHAR_DATA * ch, char *argument)
{
    sendch ("If you want to REBOOT, spell it out.\n\r", ch);
    return;
}



void do_reboot (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d, *d_next;
    CHAR_DATA *vch;

    if (ch->invis_level < LEVEL_HERO)
    {
        sprintf (buf, "Reboot by %s.", ch->name);
        do_function (ch, &do_echo, buf);
    }

    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d->next)
        if (d->character && IS_FUSED(d->character))
            unfuse(d->character);
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj (vch);
        close_socket (d);
    }

    return;
}

void do_shutdow (CHAR_DATA * ch, char *argument)
{
    sendch ("If you want to SHUTDOWN, spell it out.\n\r", ch);
    return;
}

void do_shutdown (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d, *d_next;
    CHAR_DATA *vch;

    if (ch->invis_level < LEVEL_HERO)
        sprintf (buf, "Shutdown by %s.", ch->name);
    append_file (ch, SHUTDOWN_FILE, buf);
    strcat (buf, "\n\r");
    if (ch->invis_level < LEVEL_HERO)
        do_function (ch, &do_echo, buf);
    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d->next)
        if (IS_FUSED(d->character))
            unfuse(d->character);
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj (vch);
        close_socket (d);
    }
    return;
}

void do_protect (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
        sendch ("Protect whom from snooping?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, argument)) == NULL)
    {
        sendch ("You can't find them.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_SNOOP_PROOF))
    {
        act_new ("$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR,
                 POS_DEAD);
        sendch ("Your snoop-proofing was just removed.\n\r", victim);
        REMOVE_BIT (victim->comm, COMM_SNOOP_PROOF);
    }
    else
    {
        act_new ("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR,
                 POS_DEAD);
        sendch ("You are now immune to snooping.\n\r", victim);
        SET_BIT (victim->comm, COMM_SNOOP_PROOF);
    }
}



void do_snoop (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Snoop whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL)
    {
        sendch ("No descriptor to snoop.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        sendch ("Cancelling all snoops.\n\r", ch);
        wiznet ("$N stops being such a snoop.",
                ch, NULL, WIZ_SNOOPS, WIZ_SECURE, ch->level);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->snoop_by == ch->desc)
                d->snoop_by = NULL;
        }
        return;
    }

    if (victim->desc->snoop_by != NULL)
    {
        sendch ("Busy already.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, victim->in_room) && ch->in_room != victim->in_room
        && room_is_private (victim->in_room) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        sendch ("That character is in a private room.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level
        || IS_SET (victim->comm, COMM_SNOOP_PROOF))
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if (ch->desc != NULL)
    {
        for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by)
        {
            if (d->character == victim || d->original == victim)
            {
                sendch ("No snoop loops.\n\r", ch);
                return;
            }
        }
    }

    victim->desc->snoop_by = ch->desc;
    sprintf (buf, "$N starts snooping on %s",
             (IS_NPC (ch) ? victim->short_descr : victim->name));
    wiznet (buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, ch->level);
    sendch ("Ok.\n\r", ch);
    return;
}



void do_switch (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Switch into whom?\n\r", ch);
        return;
    }

    if (ch->desc == NULL)
        return;

    if (ch->desc->original != NULL)
    {
        sendch ("You are already switched.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        sendch ("Ok.\n\r", ch);
        return;
    }

    if (!IS_NPC (victim))
    {
        sendch ("You can only switch into mobiles.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, victim->in_room) && ch->in_room != victim->in_room
        && room_is_private (victim->in_room) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        sendch ("That character is in a private room.\n\r", ch);
        return;
    }

    if (victim->desc != NULL)
    {
        sendch ("Character in use.\n\r", ch);
        return;
    }

    sprintf (buf, "$N switches into %s", victim->short_descr);
    wiznet (buf, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, ch->level);

    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup (ch->prompt);
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    sendch ("Ok.\n\r", victim);
    return;
}



void do_return (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (ch->desc == NULL)
        return;

    if (ch->desc->original == NULL)
    {
        sendch ("You aren't switched.\n\r", ch);
        return;
    }

    sendch("You return to your original body. Type replay to see any missed tells.\n\r", ch);
    if (ch->prompt != NULL)
    {
        free_string (ch->prompt);
        ch->prompt = NULL;
    }

    sprintf (buf, "$N returns from %s.", ch->short_descr);
    wiznet (buf, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, ch->level);
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (IS_TRUSTED (ch, IMMORTAL))
        return TRUE;
    else
        return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone (CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * clone)
{
    OBJ_DATA *c_obj, *t_obj;


    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
        if (obj_check (ch, c_obj))
        {
            t_obj = create_object (c_obj->pIndexData, 0);
            clone_object (c_obj, t_obj);
            obj_to_obj (t_obj, clone);
            recursive_clone (ch, c_obj, t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA *obj;

    rest = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Clone what?\n\r", ch);
        return;
    }

    if (!str_prefix (arg, "object"))
    {
        mob = NULL;
        obj = get_obj_here (ch, NULL, rest);
        if (obj == NULL)
        {
            sendch ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else if (!str_prefix (arg, "mobile") || !str_prefix (arg, "character"))
    {
        obj = NULL;
        mob = get_char_room (ch, NULL, rest);
        if (mob == NULL)
        {
            sendch ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
    {                            /* find both */

        mob = get_char_room (ch, NULL, argument);
        obj = get_obj_here (ch, NULL, argument);
        if (mob == NULL && obj == NULL)
        {
            sendch ("You don't see that here.\n\r", ch);
            return;
        }
    }

    /* clone an object */
    if (obj != NULL)
    {
        OBJ_DATA *clone;

        if (!obj_check (ch, obj))
        {
            sendch
                ("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_object (obj->pIndexData, 0);
        clone_object (obj, clone);
        if (obj->carried_by != NULL)
            obj_to_char (clone, ch);
        else
            obj_to_room (clone, ch->in_room);
        recursive_clone (ch, obj, clone);

        act ("$n has created $p.", ch, clone, NULL, TO_ROOM);
        act ("You clone $p.", ch, clone, NULL, TO_CHAR);
        wiznet ("$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE,
                ch->level);
        return;
    }
    else if (mob != NULL)
    {
        CHAR_DATA *clone;
        OBJ_DATA *new_obj;
        char buf[MAX_STRING_LENGTH];

        if (!IS_NPC (mob))
        {
            sendch ("You can only clone mobiles.\n\r", ch);
            return;
        }

        clone = create_mobile (mob->pIndexData);
        clone_mobile (mob, clone);

        for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
        {
            if (obj_check (ch, obj))
            {
                new_obj = create_object (obj->pIndexData, 0);
                clone_object (obj, new_obj);
                recursive_clone (ch, obj, new_obj);
                obj_to_char (new_obj, clone);
                new_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room (clone, ch->in_room);
        act ("$n has created $N.", ch, NULL, clone, TO_ROOM);
        act ("You clone $N.", ch, NULL, clone, TO_CHAR);
        sprintf (buf, "$N clones %s.", clone->short_descr);
        wiznet (buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, ch->level);
        return;
    }
}

/* RT to replace the two load commands */

void do_load (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  load mob <vnum>\n\r", ch);
        sendch ("  load obj <vnum> <plevel>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "mob") || !str_cmp (arg, "char"))
    {
        do_function (ch, &do_mload, argument);
        return;
    }

    if (!str_cmp (arg, "obj"))
    {
        do_function (ch, &do_oload, argument);
        return;
    }
    /* echo syntax */
    do_function (ch, &do_load, "");
}


void do_mload (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument (argument, arg);

    if (arg[0] == '\0' || !is_number (arg))
    {
        sendch ("Syntax: load mob <vnum>.\n\r", ch);
        return;
    }

    if ((pMobIndex = get_mob_index (atoi (arg))) == NULL)
    {
        sendch ("No mob has that vnum.\n\r", ch);
        return;
    }

	victim = create_mobile (pMobIndex);
    char_to_room (victim, ch->in_room);
    act ("$n has created $N!", ch, NULL, victim, TO_ROOM);
    sprintf (buf, "$N loads %s.", victim->short_descr);
    wiznet (buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, ch->level);
    sendch ("Ok.\n\r", ch);
    return;
}



void do_oload (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    long long int llPl;

    argument = one_argument (argument, arg1);
    one_argument (argument, arg2);

    if (arg1[0] == '\0' || !is_number (arg1))
    {
        sendch ("Syntax: load obj <vnum> <plevel>.\n\r", ch);
        return;
    }

    llPl = ch->llPl;        /* default */

    if (arg2[0] != '\0')
    {                            /* load with a level */
        if (!is_number (arg2))
        {
            sendch ("Syntax: oload <vnum> <plevel>.\n\r", ch);
            return;
        }
        llPl = strtoll(arg2, NULL, 10);
        if (llPl < 0 || llPl > ch->llPl)
        {
            sendch ("Power level must be be between 0 and your power level.\n\r", ch);
            return;
        }
    }

    if ((pObjIndex = get_obj_index (atoi (arg1))) == NULL)
    {
        sendch ("No object has that vnum.\n\r", ch);
        return;
    }

    obj = create_object (pObjIndex, llPl);
    if (CAN_WEAR (obj, ITEM_TAKE))
        obj_to_char (obj, ch);
    else
        obj_to_room (obj, ch->in_room);
    act ("$n has created $p!", ch, obj, NULL, TO_ROOM);
    wiznet ("$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, ch->level);
    sendch ("Ok.\n\r", ch);
    return;
}


void do_randobj (CHAR_DATA * ch, char *argument)
{
    char szArg[MAX_INPUT_LENGTH];
    OBJ_DATA *pObj;
    int nLevel;

    argument = one_argument (argument, szArg);

    if (szArg[0] == '\0' || !is_number (szArg))     {
        sendch ("Syntax: randobj <level>\n\r", ch);
        return;
    }

    nLevel = atoi(szArg);
    if (nLevel < 0 || nLevel > 100) {
        sendch ("Level must be between 0 and 100.", ch);
        return;
    }

    pObj = CreateRandWeapon (nLevel);
    if (CAN_WEAR (pObj, ITEM_TAKE))
        obj_to_char (pObj, ch);
    else
        obj_to_room (pObj, ch->in_room);
    act ("$n has created $p!", ch, pObj, NULL, TO_ROOM);
    wiznet ("$N loads $p.", ch, pObj, WIZ_LOAD, WIZ_SECURE, ch->level);
    printf_to_char (ch, "Ok. You have created %s.\n\r", pObj->short_descr);
    return;
}

void do_purge (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    DESCRIPTOR_DATA *d;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA *obj_next;

        for (victim = ch->in_room->people; victim != NULL; victim = vnext)
        {
            vnext = victim->next_in_room;
            if (IS_NPC (victim) && !IS_SET (victim->act, ACT_NOPURGE)
                && victim != ch /* safety precaution */ )
                extract_char (victim, TRUE);
        }

        for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            if (!IS_OBJ_STAT (obj, ITEM_NOPURGE))
                extract_obj (obj);
        }

        act ("$n purges the room!", ch, NULL, NULL, TO_ROOM);
        sendch ("Ok.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (!IS_NPC (victim))
    {

        if (ch == victim)
        {
            sendch ("Ho ho ho.\n\r", ch);
            return;
        }

        if (ch->level <= victim->level)
        {
            sendch ("Maybe that wasn't a good idea...\n\r", ch);
            sprintf (buf, "%s tried to purge you!\n\r", ch->name);
            sendch (buf, victim);
            return;
        }

        act ("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT);

        if (victim->level > 1)
            save_char_obj (victim);
        d = victim->desc;
        extract_char (victim, TRUE);
        if (d != NULL)
            close_socket (d);

        return;
    }

    act ("$n purges $N.", ch, NULL, victim, TO_NOTVICT);
    extract_char (victim, TRUE);
    return;
}



void do_advance (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        sendch ("Syntax: advance <char> <level>\n\r", ch);
        sendch ("Possible levels are: player, hero, builder, enforcer, headbuilder, coder, headcoder, implementor.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL) {
        sendch ("That player is not here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim)) {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "player"))
        level = PLAYER;
    else if (!str_cmp(arg2, "hero"))
        level = HERO;
    else if (!str_cmp(arg2, "builder"))
        level = BUILDER;
    else if (!str_cmp(arg2, "enforcer"))
        level = ENFORCER;
    else if (!str_cmp(arg2, "headbuilder"))
        level = HEADBUILDER;
    else if (!str_cmp(arg2, "coder"))
        level = CODER;
    else if (!str_cmp(arg2, "headcoder"))
        level = HEADCODER;
    else if (!str_cmp(arg2, "implementor"))
        level = IMPLEMENTOR;
    else {
        sendch ("Possible levels are: player, hero, builder, enforcer, headbuilder, coder, headcoder, implementor.\n\r", ch);
        return;
    }
    if (level > ch->level) {
        sendch ("Limited to your level.\n\r", ch);
        return;
    }
    victim->level = level;
    sendch (buf, victim);
    sprintf (buf, "%d", level);
    act ("$N is now level $t.", ch, buf, victim, TO_CHAR);
    sprintf (buf, "You are now level %d.\n\r", victim->level);
    sendch (buf, victim);
    save_char_obj (victim);
    return;
}



void do_restore (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;

    one_argument (argument, arg);
    if (arg[0] == '\0' || !str_cmp (arg, "room"))
    {
        /* cure room */

        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            vch->hit = vch->max_hit;
            vch->ki = vch->max_ki;
            update_pos (vch);
            act ("$n has restored you.", ch, NULL, vch, TO_VICT);
        }

        sprintf (buf, "$N restored room %d.", ch->in_room->vnum);
        wiznet (buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, ch->level);

        sendch ("Room restored.\n\r", ch);
        return;

    }

    if (ch->level >= HEADBUILDER && !str_cmp (arg, "all"))
    {
        /* cure all */

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            victim = d->character;

            if (victim == NULL || IS_NPC (victim))
                continue;

            victim->hit = victim->max_hit;
            victim->ki = victim->max_ki;
            update_pos (victim);
            if (victim->in_room != NULL)
                act ("$n has restored you.", ch, NULL, victim, TO_VICT);
        }
        sendch ("All active players restored.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    victim->hit = victim->max_hit;
    victim->ki = victim->max_ki;
    update_pos (victim);
    act ("$n has restored you.", ch, NULL, victim, TO_VICT);
    sprintf (buf, "$N restored %s",
             IS_NPC (victim) ? victim->short_descr : victim->name);
    wiznet (buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, ch->level);
    sendch ("Ok.\n\r", ch);
    return;
}


void do_unwait (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;

    one_argument (argument, arg);
    if (arg[0] == '\0' || !str_cmp (arg, "room")) {
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
            vch->wait = 0;
			vch->wait_skill = 0;
            act ("$n has removed all delay from you.", ch, NULL, vch, TO_VICT);
        }

        sprintf (buf, "$N unwaited room %d.", ch->in_room->vnum);
        wiznet (buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, ch->level);

        sendch ("Room unwaited.\n\r", ch);
        return;

    }

    if ((victim = get_char_world (ch, arg)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    victim->wait = 0;
	victim->wait_skill = 0;
    act ("$n has removed all delay from you.", ch, NULL, victim, TO_VICT);
    sprintf (buf, "$N unwaited %s",
             IS_NPC (victim) ? victim->short_descr : victim->name);
    wiznet (buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, ch->level);
    sendch ("Ok.\n\r", ch);
    return;
}


void do_freeze (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Freeze whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->act, PLR_FREEZE))
    {
        REMOVE_BIT (victim->act, PLR_FREEZE);
        sendch ("You can play again.\n\r", victim);
        sendch ("FREEZE removed.\n\r", ch);
        sprintf (buf, "$N thaws %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->act, PLR_FREEZE);
        sendch ("You can't do ANYthing!\n\r", victim);
        sendch ("FREEZE set.\n\r", ch);
        sprintf (buf, "$N puts %s in the deep freeze.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    save_char_obj (victim);

    return;
}



void do_log (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Log whom?\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        if (fLogAll)
        {
            fLogAll = FALSE;
            sendch ("Log ALL off.\n\r", ch);
        }
        else
        {
            fLogAll = TRUE;
            sendch ("Log ALL on.\n\r", ch);
        }
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (IS_SET (victim->act, PLR_LOG))
    {
        REMOVE_BIT (victim->act, PLR_LOG);
        sendch ("LOG removed.\n\r", ch);
    }
    else
    {
        SET_BIT (victim->act, PLR_LOG);
        sendch ("LOG set.\n\r", ch);
    }

    return;
}



void do_noemote (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Noemote whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }


    if (victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOEMOTE))
    {
        REMOVE_BIT (victim->comm, COMM_NOEMOTE);
        sendch ("You can emote again.\n\r", victim);
        sendch ("NOEMOTE removed.\n\r", ch);
        sprintf (buf, "$N restores emotes to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOEMOTE);
        sendch ("You can't emote!\n\r", victim);
        sendch ("NOEMOTE set.\n\r", ch);
        sprintf (buf, "$N revokes %s's emotes.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}



void do_noshout (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Noshout whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOSHOUT))
    {
        REMOVE_BIT (victim->comm, COMM_NOSHOUT);
        sendch ("You can shout again.\n\r", victim);
        sendch ("NOSHOUT removed.\n\r", ch);
        sprintf (buf, "$N restores shouts to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOSHOUT);
        sendch ("You can't shout!\n\r", victim);
        sendch ("NOSHOUT set.\n\r", ch);
        sprintf (buf, "$N revokes %s's shouts.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}



void do_notell (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Notell whom?", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->level >= ch->level)
    {
        sendch ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOTELL))
    {
        REMOVE_BIT (victim->comm, COMM_NOTELL);
        sendch ("You can tell again.\n\r", victim);
        sendch ("NOTELL removed.\n\r", ch);
        sprintf (buf, "$N restores tells to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOTELL);
        sendch ("You can't tell!\n\r", victim);
        sendch ("NOTELL set.\n\r", ch);
        sprintf (buf, "$N revokes %s's tells.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}



void do_peace (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *rch;

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->fighting != NULL)
            stop_fighting (rch, TRUE);
        if (IS_NPC (rch) && IS_SET (rch->act, ACT_AGGRESSIVE))
            REMOVE_BIT (rch->act, ACT_AGGRESSIVE);
    }

    sendch ("Ok.\n\r", ch);
    return;
}

void do_wpeace (CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;

	for (rch = char_list; rch; rch = rch->next)
		if (ch->desc != NULL && ch->desc->connected == CON_PLAYING && rch->fighting)
			stop_fighting (rch, TRUE);
	sendch ("Ok.\n\r", ch);
	return;
}


void do_wizlock (CHAR_DATA * ch, char *argument)
{
    extern bool wizlock;
    wizlock = !wizlock;

    if (wizlock)
    {
        wiznet ("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
        sendch ("Game wizlocked.\n\r", ch);
    }
    else
    {
        wiznet ("$N removes wizlock.", ch, NULL, 0, 0, 0);
        sendch ("Game un-wizlocked.\n\r", ch);
    }

    return;
}

/* RT anti-newbie code */

void do_newlock (CHAR_DATA * ch, char *argument)
{
    extern bool newlock;
    newlock = !newlock;

    if (newlock)
    {
        wiznet ("$N locks out new characters.", ch, NULL, 0, 0, 0);
        sendch ("New characters have been locked out.\n\r", ch);
    }
    else
    {
        wiznet ("$N allows new characters back in.", ch, NULL, 0, 0, 0);
        sendch ("Newlock removed.\n\r", ch);
    }

    return;
}


void do_slookup (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Lookup which skill or spell?\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name == NULL)
                break;
            sprintf (buf, "Sn: %3d  Skill/spell: '%s'\n\r",
                     sn, skill_table[sn].name);
            sendch (buf, ch);
        }
    }
    else
    {
        if ((sn = skill_lookup (arg)) < 0)
        {
            sendch ("No such skill or spell.\n\r", ch);
            return;
        }

        sprintf (buf, "Sn: %3d  Skill/spell: '%s'\n\r",
                 sn, skill_table[sn].name);
        sendch (buf, ch);
    }

    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  set mob       <name> <field> <value>\n\r", ch);
        sendch ("  set character <name> <field> <value>\n\r", ch);
        sendch ("  set obj       <name> <field> <value>\n\r", ch);
        sendch ("  set room      <room> <field> <value>\n\r", ch);
        sendch ("  set skill     <name> <spell or skill> <value>\n\r", ch);
        sendch ("  set weather   <value>\n\r", ch);
        return;
    }

    if (!str_prefix (arg, "mobile") || !str_prefix (arg, "character"))
    {
        do_function (ch, &do_mset, argument);
        return;
    }

    if (!str_prefix (arg, "skill") || !str_prefix (arg, "spell"))
    {
        do_function (ch, &do_sset, argument);
        return;
    }

    if (!str_prefix (arg, "object"))
    {
        do_function (ch, &do_oset, argument);
        return;
    }

    if (!str_prefix (arg, "room"))
    {
        do_function (ch, &do_rset, argument);
        return;
    }
    
    if (!str_prefix(arg,"weather")) {
        do_function (ch, &do_wset, argument);
        return;
    }

    /* echo syntax */
    do_function (ch, &do_set, "");
}

void do_skillchange (CHAR_DATA *ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
    int value=0;
    int sn = 0;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    argument = one_argument (argument, arg3);

    if ((arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') &&
		str_cmp(arg1, "show")) {
        sendch ("Syntax:\n\r", ch);
        sendch ("  skillchange <field> <skill> <value>\n\r", ch);
        sendch ("  skillchange show\n\r", ch);
		sendch ("Fields include:\n\r", ch);
		sendch ("  kimod: 1-50, 50 drains all ki for avg person, at max pl\n\r", ch);
		sendch ("  wait: in pulses, 4 pulses per second\n\r",ch);
        return;
    }

    // If it is not "show"
    if (str_cmp(arg1, "show")) {
		if ((sn = skill_lookup (arg2)) < 0) {
			sendch ("No such skill.\n\r", ch);
		    return;
		}

		if (!is_number (arg3)) {
			sendch ("Value must be numeric.\n\r", ch);
			return;
		}

		value = atoi (arg3);
		if (value < 0 || value > 100)
		{
			sendch ("Value range is 0 to 100.\n\r", ch);
			return;
		}
	}

	if (!str_cmp(arg1, "kimod")) {
		skill_table[sn].ki_mod = value;
		sprintf (buf, "Kimod of %s changed to %d\n\r", skill_table[sn].name, value);
		sendch (buf, ch);
	}
	else if (!str_cmp(arg1, "wait")) {
		skill_table[sn].wait = value;
		sprintf (buf, "Wait of %s changed to %d\n\r", skill_table[sn].name, value);
		sendch (buf, ch);
	}
	else if (!str_cmp(arg1, "show")) {
		int i;
		sendch ("Skill Name                  Kimod  Wait\n\r", ch);
		sendch ("==========================  =====  ====\n\r", ch);
		for (i = 0; i < MAX_SKILL; ++i) {
			if (skill_table[i].name != NULL) {
				sprintf (buf, "%-25.25s   %-3d    %-3d\n\r",skill_table[i].name,skill_table[i].ki_mod, skill_table[i].wait);
				sendch (buf, ch);
			}
		}
	}
	else {
        sendch ("Unknown field.\n\r", ch);
		sendch ("Fields include:\n\r", ch);
		sendch ("  kimod: 1-50, 50 drains all ki for avg person, at max pl\n\r", ch);
		sendch ("  wait: in pulses, 4 pulses per second\n\r",ch);
        return;
    }

	return;
}


void do_sset (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    argument = one_argument (argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  set skill <name> <spell or skill> <value>\n\r", ch);
        sendch ("  set skill <name> all <value>\n\r", ch);
        sendch ("   (use the name of the skill, not the number)\n\r",
                      ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        sendch ("Not on NPC's.\n\r", ch);
        return;
    }

    fAll = !str_cmp (arg2, "all");
    sn = 0;
    if (!fAll && (sn = skill_lookup (arg2)) < 0)
    {
        sendch ("No such skill or spell.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number (arg3))
    {
        sendch ("Value must be numeric.\n\r", ch);
        return;
    }

    value = atoi (arg3);
    if (value < 0 || value > 1000)
    {
        sendch ("Value range is 0 to 1000.\n\r", ch);
        return;
    }

    if (fAll) {
        for (sn = 0; sn < MAX_SKILL; sn++)
            if (skill_table[sn].name != NULL)
                victim->pcdata->learned[sn] = value;
    }
    else
        victim->pcdata->learned[sn] = value;
	
    ResetDiff(victim);
    return;
}

void do_mset (CHAR_DATA * ch, char *argument)
{
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    char buf[100];
    CHAR_DATA *victim;
    long long int value;

    smash_tilde (argument);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  set char <name> <field> <value>\n\r", ch);
        sendch ("  Field being one of:\n\r", ch);

        sendch ("    str        int          wis      dex    cha\n\r", ch);
        sendch ("    race       group        zenni    hp     ki\n\r", ch);
        sendch ("    align      train        skillpt  drunk\n\r", ch);
        sendch ("    security   hours        curpl    rage\n\r", ch);
        sendch ("    transcount level        sex\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
	value = strtoll(arg3, NULL, 0);

    /*
     * Set something.
     */
    if (!str_cmp (arg2, "rage")) {
        rage (victim, value);
    	return;
    }

    if (!str_cmp (arg2, "transcount")) {
        if (race_lookup("bio-android") != victim->race) {
            sendch ("Only on bio-androids.\n\r",ch);
            return;
        }
        victim->trans_count = value;
        return;
    }

    if (!str_cmp (arg2, "security"))
    {                            /* OLC */
        if (IS_NPC (ch))
        {
            sendch ("NPC's can't set this value.\n\r", ch);
            return;
        }

        if (IS_NPC (victim))
        {
            sendch ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value > ch->pcdata->security || value < 0)
        {
            if (ch->pcdata->security != 0)
            {
                sprintf (buf, "Valid security is 0-%d.\n\r",
                         ch->pcdata->security);
                sendch (buf, ch);
            }
            else
            {
                sendch ("Valid security is 0 only.\n\r", ch);
            }
            return;
        }
        victim->pcdata->security = value;
        return;
    }

    if (!str_cmp (arg2, "str")) {
        int c_str;
        if (value < 1 || value > 1000) {
            sprintf (buf, "Strength range is 1 to 1000\n\r.");
            sendch (buf, ch);
            return;
        }
		c_str = value - victim->perm_stat[STAT_STR];
        victim->perm_stat[STAT_STR] = value;
		victim->max_hit = UMAX(1, victim->max_hit + c_str * HP_STR);
		victim->max_ki = UMAX(1, victim->max_ki + c_str * KI_STR);
		if (!IS_NPC(victim) && victim->pcdata) {
			victim->pcdata->perm_hit = UMAX(1, victim->pcdata->perm_hit + c_str * HP_STR);
			victim->pcdata->perm_ki = UMAX(1, victim->pcdata->perm_ki + c_str * KI_STR);
		}
        ResetDiff(victim);
        return;
    }

	if (!str_cmp (arg2, "int")) {
        if (value < 1 || value > 1000) {
            sprintf (buf, "Intelligence range is 1 to 1000.\n\r");
            sendch (buf, ch);
            return;
        }
        victim->perm_stat[STAT_INT] = value;
        ResetDiff(victim);
        return;
    }

    if (!str_cmp (arg2, "wil")) {
        int c_wil;
		if (value < 1 || value > 1000) {
            sprintf (buf, "Will range is 1 to 1000.\n\r");
            sendch (buf, ch);
            return;
        }
		c_wil = value - victim->perm_stat[STAT_WIL];
        victim->perm_stat[STAT_WIL] = value;
		victim->max_hit = UMAX(1, victim->max_hit + c_wil * HP_WIL);
        victim->max_ki = UMAX(1, victim->max_ki + c_wil * KI_WIL);
		if (!IS_NPC(victim) && victim->pcdata) {
			victim->pcdata->perm_hit = UMAX(1, victim->pcdata->perm_hit + c_wil * HP_WIL);
            victim->pcdata->perm_ki = UMAX(1, victim->pcdata->perm_ki + c_wil * KI_WIL);
        }
        ResetDiff(victim);
        return;
    }

    if (!str_cmp (arg2, "dex")) {
        if (value < 1 || value > 1000) {
            sprintf (buf, "Dexterity range is 1 to 1000.\n\r");
            sendch (buf, ch);
            return;
        }
        victim->perm_stat[STAT_DEX] = value;
        ResetDiff(victim);
		return;
    }

    if (!str_cmp (arg2, "cha")) {
		if (value < 1 || value > 1000) {
            sprintf (buf, "Charisma range is 1 to 1000.\n\r");
            sendch (buf, ch);
            return;
        }
        victim->perm_stat[STAT_CHA] = value;
        ResetDiff(victim);
        return;
    }

    if (!str_prefix (arg2, "sex"))
    {
        if (value < 0 || value > 2)
        {
            sendch ("Sex range is 0 to 2.\n\r", ch);
            return;
        }
        victim->sex = value;
        if (!IS_NPC (victim))
            victim->pcdata->true_sex = value;
        return;
    }

	if (!str_prefix (arg2, "curpl"))
    {
		if ( value < 0 || value > 100 )
		{
		   sendch( "You can only set current power level from 0 to 100.",ch);
		   return;
		}

		victim->nCurPl = value;
		return;
    }

    if (!str_prefix (arg2, "level"))
    {
        if (!IS_NPC (victim))
        {
            sendch ("Not on PC's.\n\r", ch);
            return;
        }

        if (value < 0 || value > MAX_LEVEL)
        {
            sprintf (buf, "Level range is 0 to %d.\n\r", MAX_LEVEL);
            sendch (buf, ch);
            return;
        }
        victim->level = value;
        return;
    }

    if (!str_prefix (arg2, "zenni"))
    {
        victim->zenni = value;
        return;
    }

    if (!str_prefix (arg2, "hp"))
    {
        if (value < 1 || value > MAX_HP)
        {
            sprintf(buf,"Hp range is 1 to %d hit points.\n\r", MAX_HP);
			sendch (buf, ch);
            return;
        }
        victim->max_hit = value;
        if (!IS_NPC (victim))
            victim->pcdata->perm_hit = value;
        return;
    }

    if (!str_prefix (arg2, "ki"))
    {
        if (value < 1 || value > MAX_KI)
        {
            sprintf(buf,"Ki range is 1 to %d ki.\n\r", MAX_KI);
			sendch (buf, ch);
            return;
        }
        victim->max_ki = value;
        if (!IS_NPC (victim))
            victim->pcdata->perm_ki = value;
        return;
    }

    if (!str_prefix (arg2, "align"))
    {
        if (value < -1000 || value > 1000)
        {
            sendch ("Alignment range is -1000 to 1000.\n\r", ch);
            return;
        }
        victim->alignment = value;
        return;
    }

      if (!str_prefix (arg2, "drunk"))
    {
        if (IS_NPC (victim))
        {
            sendch ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            sendch ("Drunk range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if (!str_prefix (arg2, "race"))
    {
        int race;

        race = race_lookup (arg3);

        if (race == 0)
        {
            sendch ("That is not a valid race.\n\r", ch);
            return;
        }

        if (!IS_NPC (victim) && !race_table[race].pc_race)
        {
            sendch ("That is not a valid player race.\n\r", ch);
            return;
        }

        victim->race = race;
        return;
    }

    if (!str_prefix (arg2, "group"))
    {
        if (!IS_NPC (victim))
        {
            sendch ("Only on NPCs.\n\r", ch);
            return;
        }
        victim->group = value;
        return;
    }

	if (!str_prefix (arg2, "hours"))
	{
		if (IS_NPC (victim))
		{
			sendch ("Not on NPC's.\n\r", ch);
			return;
		}

		if (!is_number (arg3))
		{
			sendch ("Value must be numeric.\n\r", ch);
			return;
		}

		value = atoi (arg3);

		if (value < 0 || value > 999)
		{
			sendch ("Value must be between 0 and 999.\n\r", ch);
			return;
		}

		victim->played = ( value * 3600 );
		printf_to_char(ch, "%s's hours set to %d.", victim->name, value);

		return;
	}

    /*
     * Generate usage message.
     */
    do_function (ch, &do_mset, "");
    return;
}

void do_string (CHAR_DATA * ch, char *argument)
{
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    smash_tilde (argument);
    argument = one_argument (argument, type);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0'
        || arg3[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  string char <name> <field> <string>\n\r", ch);
        sendch ("    fields: name short long desc title spec\n\r", ch);
        sendch ("  string obj  <name> <field> <string>\n\r", ch);
        sendch ("    fields: name short long extended\n\r", ch);
        return;
    }

    if (!str_prefix (type, "character") || !str_prefix (type, "mobile"))
    {
        if ((victim = get_char_world (ch, arg1)) == NULL)
        {
            sendch ("They aren't here.\n\r", ch);
            return;
        }

        /* clear zone for mobs */
        victim->zone = NULL;

        /* string something */

        if (!str_prefix (arg2, "name"))
        {
            if (!IS_NPC (victim))
            {
                sendch ("Not on PC's.\n\r", ch);
                return;
            }
            free_string (victim->name);
            victim->name = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "description"))
        {
            free_string (victim->description);
            victim->description = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "short"))
        {
            free_string (victim->short_descr);
            victim->short_descr = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "long"))
        {
            free_string (victim->long_descr);
			strcat(arg3,"\n\r");
            victim->long_descr = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "title"))
        {
            if (IS_NPC (victim))
            {
                sendch ("Not on NPC's.\n\r", ch);
                return;
            }

            set_title (victim, arg3);
            return;
        }

        if (!str_prefix (arg2, "spec"))
        {
            if (!IS_NPC (victim))
            {
                sendch ("Not on PC's.\n\r", ch);
                return;
            }

            if ((victim->spec_fun = spec_lookup (arg3)) == 0)
            {
                sendch ("No such spec fun.\n\r", ch);
                return;
            }

            return;
        }
    }

    if (!str_prefix (type, "object"))
    {
        /* string an obj */

        if ((obj = get_obj_world (ch, arg1)) == NULL)
        {
            sendch ("Nothing like that in heaven or earth.\n\r", ch);
            return;
        }

        if (!str_prefix (arg2, "name"))
        {
            free_string (obj->name);
            obj->name = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "short"))
        {
            free_string (obj->short_descr);
            obj->short_descr = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "long"))
        {
            free_string (obj->description);
            obj->description = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "ed") || !str_prefix (arg2, "extended"))
        {
            EXTRA_DESCR_DATA *ed;

            argument = one_argument (argument, arg3);
            if (argument == NULL)
            {
                sendch
                    ("Syntax: oset <object> ed <keyword> <string>\n\r", ch);
                return;
            }

            strcat (argument, "\n\r");

            ed = new_extra_descr ();

            ed->keyword = str_dup (arg3);
            ed->description = str_dup (argument);
            ed->next = obj->extra_descr;
            obj->extra_descr = ed;
            return;
        }
    }


    /* echo bad use message */
    do_function (ch, &do_string, "");
}



void do_oset (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    long long int value;

    smash_tilde (argument);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  set obj <object> <field> <value>\n\r", ch);
        sendch ("  Field being one of:\n\r", ch);
        sendch ("    value0 value1 value2 value3 value4 (v1-v4)\n\r",
                      ch);
        sendch ("    extra wear pl weight cost timer\n\r", ch);
        return;
    }

    if ((obj = get_obj_world (ch, arg1)) == NULL)
    {
        sendch ("Nothing like that in heaven or earth.\n\r", ch);
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = strtoll(arg3, NULL, 10);

    /*
     * Set something.
     */
    if (!str_cmp (arg2, "value0") || !str_cmp (arg2, "v0"))
    {
        obj->value[0] = UMIN (50, value);
        return;
    }

    if (!str_cmp (arg2, "value1") || !str_cmp (arg2, "v1"))
    {
        obj->value[1] = value;
        return;
    }

    if (!str_cmp (arg2, "value2") || !str_cmp (arg2, "v2"))
    {
        obj->value[2] = value;
        return;
    }

    if (!str_cmp (arg2, "value3") || !str_cmp (arg2, "v3"))
    {
        obj->value[3] = value;
        return;
    }

    if (!str_cmp (arg2, "value4") || !str_cmp (arg2, "v4"))
    {
        obj->value[4] = value;
        return;
    }

    if (!str_prefix (arg2, "extra"))
    {
        obj->extra_flags = value;
        return;
    }

    if (!str_prefix (arg2, "wear"))
    {
        obj->wear_flags = value;
        return;
    }

    if (!str_prefix (arg2, "pl"))
    {
        if (value < 0 || value > MAX_PL) {
			sprintf(buf, "Powerlevel must be 0 to %Ld.", MAX_PL);
			sendch(buf, ch);
			return;
		}
		obj->llPl = value;
        return;
    }

    if (!str_prefix (arg2, "weight"))
    {
        obj->weight = value;
        return;
    }

    if (!str_prefix (arg2, "cost"))
    {
        obj->cost = value;
        return;
    }

    if (!str_prefix (arg2, "timer"))
    {
        obj->timer = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function (ch, &do_oset, "");
    return;
}



void do_rset (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde (argument);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        sendch ("Syntax:\n\r", ch);
        sendch ("  set room <location> <field> <value>\n\r", ch);
        sendch ("  Field being one of:\n\r", ch);
        sendch ("    flags sector\n\r", ch);
        return;
    }

    if ((location = find_location (ch, arg1)) == NULL)
    {
        sendch ("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, location) && ch->in_room != location
        && room_is_private (location) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        sendch ("That room is private right now.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number (arg3))
    {
        sendch ("Value must be numeric.\n\r", ch);
        return;
    }
    value = atoi (arg3);

    /*
     * Set something.
     */
    if (!str_prefix (arg2, "flags"))
    {
        location->room_flags = value;
        return;
    }

    if (!str_prefix (arg2, "sector"))
    {
        location->sector_type = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function (ch, &do_rset, "");
    return;
}



/* Written by Stimpy, ported to rom2.4 by Silverhand 3/12
 *
 *	Added the other COMM_ stuff that wasn't defined before 4/16 -Silverhand
 */
void do_sockets( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA       *vch;
	DESCRIPTOR_DATA *d;
	char            buf  [ MAX_STRING_LENGTH ];
	char            buf2 [ MAX_STRING_LENGTH ];
	int             count;
	char *          st;
	char            idle[10];

	count       = 0;
	buf[0]      = '\0';
	buf2[0]     = '\0';

	strcat( buf2, "\n\r{c[Num Connected_State Idl] PlayerName  Host\n\r" );
	strcat( buf2, "{y--------------------------------------------------------------------------{x\n\r");
	for ( d = descriptor_list; d; d = d->next ) {
		if (d->character && can_see(ch, d->character)) {
			switch (d->connected) {
				case CON_PLAYING:              st = "    PLAYING    ";  break;
				case CON_GET_NAME:             st = "   Get Name    ";  break;
				case CON_GET_OLD_PASSWORD:     st = "Get Old Passwd ";  break;
				case CON_CONFIRM_NEW_NAME:     st = " Confirm Name  ";  break;
				case CON_GET_NEW_PASSWORD:     st = "Get New Passwd ";  break;
				case CON_CONFIRM_NEW_PASSWORD: st = "Confirm Passwd ";  break;
				case CON_GET_NEW_RACE:         st = "  Get New Race ";  break;
				case CON_GET_NEW_SEX:          st = "  Get New Sex  ";  break;
				case CON_GET_ALIGNMENT:   	   st = " Get New Align ";	break;
				case CON_DEFAULT_CHOICE:	   st = " Choosing Cust ";	break;
				case CON_GEN_GROUPS:		   st = " Customization ";	break;
				case CON_SET_STATS:		       st = " Setting Stats ";	break;
				case CON_READ_IMOTD:		   st = " Reading IMOTD "; 	break;
				case CON_READ_MOTD:            st = "  Reading MOTD ";  break;
				case CON_BREAK_CONNECT:		   st = "   LINKDEAD    ";	break;

				case CON_ANSI:                 st = " Deciding ANSI ";  break;
				case CON_GET_TELNETGA:         st = "Telnet Gateway ";  break;
				case CON_COPYOVER_RECOVER:     st = "Copyover Recver";  break;
				case CON_NOTE_TO:              st = "   Note: To    ";  break;
				case CON_NOTE_SUBJECT:         st = " Note: Subject ";  break;
				case CON_NOTE_EXPIRE:          st = " Note: Expire  ";  break;
				case CON_NOTE_TEXT:            st = "  Note: Text   ";  break;
				case CON_NOTE_FINISH:          st = " Note: Finish  ";  break;

                case CON_FUSE_SLAVE:           st = " Fused: Slave  ";  break;

				default:                       st = "   !UNKNOWN!   ";  break;
			}
			count++;

			vch = d->original ? d->original : d->character;
			if ( vch->timer > 0 )
				sprintf( idle, "%-2d", vch->timer );
			else
				sprintf( idle, "  " );

			sprintf( buf, "[%3d %s  %2s] %-12s %-32.32s\n\r",
				d->descriptor, st, idle,
				(d->original) ? d->original->name : (d->character) ? d->character->name : "(None!)",
				d->host );

			strcat( buf2, buf );
		}
	}

	sprintf( buf, "\n\r%d user%s\n\r", count, count == 1 ? "" : "s" );
	strcat( buf2, buf );
	sendch( buf2, ch );
	return;
}

/*
void do_sockets (CHAR_DATA * ch, char *argument)
{
    char buf[2 * MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int count;

    count = 0;
    buf[0] = '\0';

    one_argument (argument, arg);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->character != NULL && can_see (ch, d->character)
            && (arg[0] == '\0' || is_name (arg, d->character->name)
                || (d->original && is_name (arg, d->original->name))))
        {
            count++;
            sprintf (buf + strlen (buf), "[%3d %2d] %s@%s\n\r",
                     d->descriptor,
                     d->connected,
                     d->original ? d->original->name :
                     d->character ? d->character->name : "(none)", d->host);
        }
    }
    if (count == 0)
    {
        sendch ("No one by that name is connected.\n\r", ch);
        return;
    }

    sprintf (buf2, "%d user%s\n\r", count, count == 1 ? "" : "s");
    strcat (buf, buf2);
    page_to_char (buf, ch);
    return;
}
*/



/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        sendch ("Force whom to do what?\n\r", ch);
        return;
    }

    one_argument (argument, arg2);

    if (!str_cmp (arg2, "delete") || !str_prefix (arg2, "mob"))
    {
        sendch ("That will NOT be done.\n\r", ch);
        return;
    }

    sprintf (buf, "$n forces you to '%s'.", argument);

	/* Replaced original block with code by Edwin to keep from
	 * corrupting pfiles in certain pet-infested situations.
	 * JR -- 10/15/00
	 */
	if ( !str_cmp( arg, "all" ) )
	{
    	DESCRIPTOR_DATA *desc,*desc_next;

    	if (ch->level < HEADBUILDER)
    	{
			sendch("Not at your level!\n\r",ch);
			return;
    	}

    	for ( desc = descriptor_list; desc != NULL; desc = desc_next )
    	{
			desc_next = desc->next;

			if (desc->connected==CON_PLAYING &&
	    		desc->character->level < ch->level )
	    	{
	    		act( buf, ch, NULL, desc->character, TO_VICT );
	    		interpret( desc->character, argument );
			}
    	}
	}
    else if (!str_cmp (arg, "players"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (ch->level < CODER)
        {
            sendch ("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next;

            if (!IS_NPC (vch) && vch->level < ch->level
                && vch->level < LEVEL_HERO)
            {
                act (buf, ch, NULL, vch, TO_VICT);
                interpret (vch, argument);
            }
        }
    }
    else if (!str_cmp (arg, "gods"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (ch->level < CODER)
        {
            sendch ("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next;

            if (!IS_NPC (vch) && vch->level < ch->level
                && vch->level >= LEVEL_HERO)
            {
                act (buf, ch, NULL, vch, TO_VICT);
                interpret (vch, argument);
            }
        }
    }
    else
    {
        CHAR_DATA *victim;

        if ((victim = get_char_world (ch, arg)) == NULL)
        {
            sendch ("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch)
        {
            sendch ("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!is_room_owner (ch, victim->in_room)
            && ch->in_room != victim->in_room
            && room_is_private (victim->in_room)
            && !IS_TRUSTED (ch, IMPLEMENTOR))
        {
            sendch ("That character is in a private room.\n\r", ch);
            return;
        }

        if (victim->level >= ch->level)
        {
            sendch ("Do it yourself!\n\r", ch);
            return;
        }

        if (!IS_NPC (victim) && ch->level < HEADBUILDER)
        {
            sendch ("Not at your level!\n\r", ch);
            return;
        }

        act (buf, ch, NULL, victim, TO_VICT);
        interpret (victim, argument);
    }

    sendch ("Ok.\n\r", ch);
    return;
}



/*
 * New routines by Dionysos.
 */
void do_invis (CHAR_DATA * ch, char *argument)
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument (argument, arg);

    if (arg[0] == '\0')
        /* take the default path */

        if (ch->invis_level)
        {
            ch->invis_level = 0;
            act ("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM);
            sendch ("You slowly fade back into existence.\n\r", ch);
        }
        else
        {
            ch->invis_level = ch->level;
            act ("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
            sendch ("You slowly vanish into thin air.\n\r", ch);
        }
    else
        /* do the level thing */
    {
        level = atoi (arg);
        if (level < 2 || level > ch->level)
        {
            sendch ("Invis level must be between 2 and your level.\n\r",
                          ch);
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->invis_level = level;
            act ("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
            sendch ("You slowly vanish into thin air.\n\r", ch);
        }
    }

    return;
}


void do_incognito (CHAR_DATA * ch, char *argument)
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument (argument, arg);

    if (arg[0] == '\0')
        /* take the default path */

        if (ch->incog_level)
        {
            ch->incog_level = 0;
            act ("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM);
            sendch ("You are no longer cloaked.\n\r", ch);
        }
        else
        {
            ch->incog_level = ch->level;
            act ("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
            sendch ("You cloak your presence.\n\r", ch);
        }
    else
        /* do the level thing */
    {
        level = atoi (arg);
        if (level < 2 || level > ch->level)
        {
            sendch ("Incog level must be between 2 and your level.\n\r",
                          ch);
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->incog_level = level;
            act ("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
            sendch ("You cloak your presence.\n\r", ch);
        }
    }

    return;
}



void do_holylight (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_HOLYLIGHT))
    {
        REMOVE_BIT (ch->act, PLR_HOLYLIGHT);
        sendch ("Holy light mode off.\n\r", ch);
    }
    else
    {
        SET_BIT (ch->act, PLR_HOLYLIGHT);
        sendch ("Holy light mode on.\n\r", ch);
    }

    return;
}



void do_combatinfo (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_COMBATINFO)) {
        REMOVE_BIT (ch->act, PLR_COMBATINFO);
        sendch ("Combat info off.\n\r", ch);
    }
    else {
        SET_BIT (ch->act, PLR_COMBATINFO);
        sendch ("Combat info on.\n\r", ch);
    }

    return;
}


/* prefix command: it will put the string typed on each line typed */

void do_prefi (CHAR_DATA * ch, char *argument)
{
    sendch ("You cannot abbreviate the prefix command.\r\n", ch);
    return;
}

void do_prefix (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
        if (ch->prefix[0] == '\0')
        {
            sendch ("You have no prefix to clear.\r\n", ch);
            return;
        }

        sendch ("Prefix removed.\r\n", ch);
        free_string (ch->prefix);
        ch->prefix = str_dup ("");
        return;
    }

    if (ch->prefix[0] != '\0')
    {
        sprintf (buf, "Prefix changed to %s.\r\n", argument);
        free_string (ch->prefix);
    }
    else
    {
        sprintf (buf, "Prefix set to %s.\r\n", argument);
    }

    ch->prefix = str_dup (argument);
}

#define CH(descriptor)  ((descriptor)->original ? (descriptor)->original : (descriptor)->character)

/* This file holds the copyover data */
#define COPYOVER_FILE "copyover.data"

/* This is the executable file */
#define EXE_FILE      "../src/dbarena"


/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  http://pip.dknet.dk/~pip1773
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
void do_copyover (CHAR_DATA * ch, char *argument)
{
    FILE *fp;
    DESCRIPTOR_DATA *d, *d_next;
    char buf[100], buf2[100];
    extern int port, control;    /* db.c */

    fp = fopen (COPYOVER_FILE, "w");

    if (!fp) {
        sendch ("Copyover file not writeable, aborted.\n\r", ch);
        logfile ("Could not write to copyover file: %s", COPYOVER_FILE);
        logstr (LOG_ERR, "Could not write to copyover file: %s", COPYOVER_FILE);
        perror ("do_copyover:fopen");
        return;
    }

    /* Consider changing all saved areas here, if you use OLC */

    /* do_asave (NULL, ""); - autosave changed areas */


    sprintf (buf, "\n\r *** COPYOVER by %s - please remain seated!\n\r", ch->name);

    // Need to unfuse everyone first
    for (d = descriptor_list; d != NULL; d = d->next)
        if (d->character && IS_FUSED(d->character))
            unfuse(d->character);

    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d; d = d_next)
    {
        CHAR_DATA *och = CH (d);
        d_next = d->next;        /* We delete from the list , so need to save this */

        if (!d->character || d->connected > CON_PLAYING) {
            /* drop those logging on */
            write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
            close_socket (d);
        }
        else {
            fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);
            save_char_obj (och);
            write_to_descriptor (d->descriptor, buf, 0);
        }
    }

    fprintf (fp, "-1\n");
    fclose (fp);

    /* Close reserve and other always-open files and release other resources */

    fclose (fpReserve);

    /* exec - descriptors are inherited */

    sprintf (buf, "%d", port);
    sprintf (buf2, "%d", control);
    execl (EXE_FILE, "rom", buf, "copyover", buf2, (char *) NULL);

    /* Failed - sucessful exec will not return */

    perror ("do_copyover: execl");
    sendch ("Copyover FAILED!\n\r", ch);

    /* Here you might want to reopen fpReserve */
    fpReserve = fopen (NULL_FILE, "r");
}

/* Recover from a copyover - load players */
void copyover_recover ()
{
    DESCRIPTOR_DATA *d;
    FILE *fp;
    char name[100];
    char host[MSL];
    int desc;
    bool bFound;

    logfile ("Copyover recovery initiated");
    logstr (LOG_GAME, "Copyover recovery initiated");

    fp = fopen (COPYOVER_FILE, "r");

    if (!fp) {
        /* there are some descriptors open which will hang forever then ? */
        perror ("copyover_recover:fopen");
        logfile ("Copyover file not found. Exitting.\n\r");
        logstr (LOG_CRIT, "Copyover file not found. Exitting");
        exit (1);
    }

    unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading  */

    for (;;) {
        fscanf (fp, "%d %s %s\n", &desc, name, host);
        if (desc == -1)
            break;

        /* Write something, and check if it goes error-free */
        if (!write_to_descriptor (desc, "\n\rRestoring from copyover...\n\r", 0)) {
            close (desc);        /* nope */
            continue;
        }

        d = new_descriptor ();
        d->descriptor = desc;

        d->host = str_dup (host);
        d->next = descriptor_list;
        descriptor_list = d;
        d->connected = CON_COPYOVER_RECOVER;    /* -15, so close_socket frees the char */

        /* Now, find the pfile */
        bFound = load_char_obj (d, name);

        if (!bFound) {
            /* Player file not found?! */
            write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
            close_socket (d);
        }
        else {
            write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r", 0);

            reset_char (d->character);

            /* Just In Case */
            if (!d->character->in_room)
                d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

            /* Insert in the char_list */
            d->character->next = char_list;
            char_list = d->character;

            char_to_room (d->character, d->character->in_room);
            do_look (d->character, "auto");
            act ("$n materializes!", d->character, NULL, NULL, TO_ROOM);
            d->connected = CON_PLAYING;

            if (d->character->pet != NULL) {
                char_to_room (d->character->pet, d->character->in_room);
                act ("$n materializes!.", d->character->pet, NULL, NULL, TO_ROOM);
            }
        }
    }
    fclose (fp);


}

/* This _should_ encompass all the QuickMUD config commands */
/* -- JR 11/24/00                                           */

void do_qmconfig (CHAR_DATA * ch, char * argument)
{
	extern int mud_ansiprompt;
	extern int mud_ansicolor;
	extern int mud_telnetga;
	extern char *mud_ipaddress;
	char arg1[MSL];
	char arg2[MSL];

	if (IS_NPC(ch))
		return;

	if (argument[0] == '\0')
	{
		printf_to_char(ch, "Valid qmconfig options are:\n\r");
		printf_to_char(ch, "    show       (shows current status of toggles)\n\r");
		printf_to_char(ch, "    ansiprompt [on|off]\n\r");
		printf_to_char(ch, "    ansicolor  [on|off]\n\r");
		printf_to_char(ch, "    telnetga   [on|off]\n\r");
		printf_to_char(ch, "    read\n\r");
		return;
	}

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );

	if (!str_prefix(arg1, "read")) {
		qmconfig_read();
		return;
	}

	if (!str_prefix(arg1, "show"))
	{
		printf_to_char(ch, "ANSI prompt: %s", mud_ansiprompt ? "{GON{x\n\r" : "{ROFF{x\n\r");
		printf_to_char(ch, "ANSI color : %s", mud_ansicolor ? "{GON{x\n\r" : "{ROFF{x\n\r");
		printf_to_char(ch, "IP Address : %s\n\r", mud_ipaddress);
		printf_to_char(ch, "Telnet GA  : %s", mud_telnetga ? "{GON{x\n\r" : "{ROFF{x\n\r");
		return;
	}

	if (!str_prefix(arg1, "ansiprompt"))
	{
		if (!str_prefix(arg2, "on"))
		{
			mud_ansiprompt=TRUE;
			printf_to_char(ch, "New logins will now get an ANSI color prompt.\n\r");
			return;
		}

		else if(!str_prefix(arg2, "off"))
		{
			mud_ansiprompt=FALSE;
			printf_to_char(ch, "New logins will not get an ANSI color prompt.\n\r");
			return;
		}

		printf_to_char(ch, "Valid arguments are \"on\" and \"off\".\n\r");
		return;
	}

	if (!str_prefix(arg1, "ansicolor"))
	{
		if (!str_prefix(arg2, "on"))
		{
			mud_ansicolor=TRUE;
			printf_to_char(ch, "New players will have color enabled.\n\r");
			return;
		}

		else if (!str_prefix(arg2, "off"))
		{
			mud_ansicolor=FALSE;
			printf_to_char(ch, "New players will not have color enabled.\n\r");
			return;
		}

		printf_to_char(ch, "Valid arguments are \"on\" and \"off\".\n\r");
		return;
	}

	if (!str_prefix(arg1, "telnetga"))
	{
		if (!str_prefix(arg2, "on"))
		{
			mud_telnetga=TRUE;
			printf_to_char(ch, "Telnet GA will be enabled for new players.\n\r");
			return;
		}

		else if (!str_prefix(arg2, "off"))
		{
			mud_telnetga=FALSE;
			printf_to_char(ch, "Telnet GA will be disabled for new players.\n\r");
			return;
		}

		printf_to_char(ch, "Valid arguments are \"on\" and \"off\".\n\r");
		return;
	}

	printf_to_char(ch, "I have no clue what you are trying to do...\n\r");
	return;
}

void qmconfig_read (void) {
	FILE *fp;
	bool fMatch;
	char *word;
	extern int mud_ansiprompt, mud_ansicolor, mud_telnetga;

    logstr (LOG_GAME, "Loading configuration settings from qmconfig.rc...");

	fp = fopen("../area/qmconfig.rc","r");
	if (!fp) {
        logstr (LOG_GAME|LOG_ERR, "qmconfig.rc not found. Using compiled-in defaults");
		return;
	}

	for(;;) {
		word = feof (fp) ? "END" : fread_word(fp);

		fMatch = FALSE;

		switch (UPPER(word[0])) {
			case '*':
				fMatch = TRUE;
				fread_to_eol (fp);
				break;

			case 'A':
				KEY ("Ansicolor", mud_ansicolor, fread_number(fp));
				KEY ("Ansiprompt", mud_ansiprompt, fread_number(fp));
				break;
			case 'E':
				if (!str_cmp(word, "END")) {
                    logstr (LOG_GAME,"Settings read");
                    fclose (fp);
                    return;
                }
				break;
			case 'T':
				KEY ("Telnetga", mud_telnetga, fread_number(fp));
				break;
		}
		if (!fMatch) {
            logstr (LOG_GAME|LOG_ERR,"qmconfig_read: no match for %s!", word);
			fread_to_eol(fp);
		}
	}

}


int colorstrlen(char *argument)
{
  char *str;
  int strlength;

  if (argument == NULL || argument[0] == '\0')
    return 0;

  strlength = 0;
  str = argument;

  while (*str != '\0')
  {
    if ( *str != '{' )
    {
      str++;
      strlength++;
      continue;
    }

    if (*(++str) == '{')
      strlength++;

    str++;
  }
  return strlength;
}

void do_immtitle(CHAR_DATA *ch, char *argument)
{
  if (argument[0] == '\0')
  {
    ch->immtitle = NULL;
    sendch("Immtitle cleared.\n\r", ch);
    return;
  }

  if (colorstrlen(argument) > 12)
  {
    sendch("Immtitle must be 12 (or under) characters long.\n\r", ch); return;
  }

  ch->immtitle = str_dup(argument);
  sendch("Immtitle set.\n\r", ch);
}


void do_invade( CHAR_DATA *ch , char *argument )
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	AREA_DATA *tarea;
	int count, created = 0;
	MOB_INDEX_DATA *pMobIndex;
	ROOM_INDEX_DATA *location;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	argument = one_argument( argument, arg3 );

	if ( arg1[0] == '\0' || arg2[0] == '\0' )
	{
		sendch("invade [area] [# of invaders] [vnum]\n\r", ch );
		return;
	}

	for ( tarea = area_first; tarea; tarea = tarea->next )
	{
		if ( !str_cmp( tarea->file_name, arg1)) break;
	}

	if (!tarea)
	{
		sendch("Area not found.\n\r", ch );
		return;
	}

	if ((count = atoi(arg2)) > 300)
	{
		sendch("Whoa...Less than 300 please.\n\r", ch );
		return;
	}

	if ( ( pMobIndex = get_mob_index( atoi(arg3) ) ) == NULL )
	{
		sendch("No mobile has that vnum.\n\r", ch );
		return;
	}
	while (created < count)
	{
		if ((location = get_room_index(number_range(tarea->min_vnum, tarea->max_vnum))) == NULL)
			continue;

		if (IS_SET( location->room_flags, ROOM_SAFE))
			continue;

		created++;
		victim = create_mobile( pMobIndex );
		if(!IS_SET(pMobIndex->act, ACT_AGGRESSIVE))
			SET_BIT(pMobIndex->act, ACT_AGGRESSIVE);
		//if(!IS_SET(pMobIndex->act, ACT_INVADER))
		//	SET_BIT(pMobIndex->act, ACT_INVADER);
		char_to_room( victim, location );
		// Uhh, this spams everyone to death
		//act("$N appears as part of an invasion force!", ch, NULL, victim, TO_ROOM );
	}

	sendch("The invasion was successful!\n\r", ch );
	return;
}


void do_skillprereq (CHAR_DATA *ch, char *argument) {
    bool found;
    char buf[MAX_STRING_LENGTH];
    int i,j;

    sendch ("{cSkill Name                      Difficulty  Stats{x\n\r", ch);
	sendch ("{Y==========================  ==============  ================{x\n\r", ch);
	for (i = 0; i < MAX_SKILL; ++i) {
	    if (skill_table[i].name != NULL) {
			sprintf (buf, "{c%-25.25s{x   %14d ",skill_table[i].name,skill_table[i].nDiff);
            if (skill_table[i].stat_prereq[STAT_STR] > 0)
                sprintf (buf + strlen(buf), " str=%d", skill_table[i].stat_prereq[STAT_STR]);
            if (skill_table[i].stat_prereq[STAT_WIL] > 0)
                sprintf (buf + strlen(buf), " wil=%d", skill_table[i].stat_prereq[STAT_WIL]);
            if (skill_table[i].stat_prereq[STAT_INT] > 0)
                sprintf (buf + strlen(buf), " int=%d", skill_table[i].stat_prereq[STAT_INT]);
            if (skill_table[i].stat_prereq[STAT_CHA] > 0)
                sprintf (buf + strlen(buf), " cha=%d", skill_table[i].stat_prereq[STAT_CHA]);
            if (skill_table[i].stat_prereq[STAT_DEX] > 0)
                sprintf (buf + strlen(buf), " dex=%d", skill_table[i].stat_prereq[STAT_DEX]);
            strcat (buf, "\n\r");
            sendch (buf, ch);

            sprintf (buf, "    Races:");
		    found = FALSE;
		    for (j=0; j<MAX_PC_RACE; ++j) {
                if (skill_table[i].race_prereq[j] == '\0')
                    break;

                found = TRUE;
                strcat (buf, " ");
                strcat (buf, skill_table[i].race_prereq[j]);
		    }
            if (found) {
                strcat (buf, "\n\r");
                sendch (buf, ch);
            }

            sprintf (buf, "    Skills:");
		    found = FALSE;
		    for (j=0; j<5; ++j) {
                if (skill_table[i].skill_prereq[j] == '\0')
                    break;

                found = TRUE;
                sprintf (buf + strlen(buf), " %s=%d", skill_table[i].skill_prereq[j], skill_table[i].skill_value[j]);
		    }
            if (found) {
                strcat (buf, "\n\r");
                sendch (buf, ch);
            }
		}
	}

	return;
}


/* show a list of all unused AreaVNUMS */
/* By The Mage */
void do_fvlist (CHAR_DATA *ch, char *argument) {
    int i,j;
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument(argument,arg);

    if (arg[0] == '\0') {
        sendch("Syntax:\n\r",ch);
        sendch("  freevnum obj\n\r",ch);
        sendch("  freevnum mob\n\r",ch);
        sendch("  freevnum room\n\r",ch);
        return;
    }

    j=0;
    if (!str_cmp(arg,"obj")) {
        printf_to_char(ch,"{WFree {C%s{W vnum listing for area {C%s{x\n\r",arg, ch->in_room->area->name);
        printf_to_char(ch,"{Y=============================================================================={C\n\r");
        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) {
	        if (get_obj_index(i) == NULL) {
	            printf_to_char(ch,"%8d, ",i);
	            if (j == 6) {
	                sendch("\n\r",ch);
	                j=0;
	            }
                else
	                j++;
	       }
        }
        sendch("{x\n\r",ch);
        return;
    }

    if (!str_cmp(arg,"mob")) {
        printf_to_char(ch,"{WFree {C%s {Wvnum listing for area {C%s{x\n\r",arg, ch->in_room->area->name);
        printf_to_char(ch,"{Y=============================================================================={C\n\r");
        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) {
	        if (get_mob_index(i) == NULL) {
	            printf_to_char(ch,"%8d, ",i);
	            if (j == 6) {
	                sendch("\n\r",ch);
	                j=0;
  	            }
	            else
                    j++;
	        }
        }
        sendch("{x\n\r",ch);
        return;
    }

    if (!str_cmp(arg,"room")) {
        printf_to_char(ch,"{WFree {C%s {Wvnum listing for area {C%s{x\n\r",arg, ch->in_room->area->name);
        printf_to_char(ch,"{Y=============================================================================={C\n\r");
        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) {
	        if (get_room_index(i) == NULL) {
	            printf_to_char(ch,"%8d, ",i);
	            if (j == 6) {
	                sendch("\n\r",ch);
	                j=0;
	            }
	            else
                    j++;
	        }
        }
        sendch("{x\n\r",ch);
        return;
    }
    sendch("Syntax:\n\r"
           "  freevnum obj\n\r"
           "  freevnum mob\n\r"
           "  freevnum room\n\r",ch);

    return;
}

void do_incinerat (CHAR_DATA *ch, char *argument) {
    sendch ("If you want to incinerate, spell it out!\n\r", ch);
    return;
}


// Nuke written by Virus for ROM, changed to incinerate.
void do_incinerate (CHAR_DATA *ch, char *argument) {
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    char arg [MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        sendch("Syntax: incinerate <player name>\n\r",ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
        sendch("They must be playing.\n\r", ch);
        return;
    }

    if (IS_NPC(victim)) {
        sendch("Not on NPCs.\n\r", ch);
        return;
    }

    if (ch->level <= victim->level) {
        sendch("{RYou do not have the authority to do that.{x\n\r",ch);
        act ("{R$N attempted to incinerate you.{x", victim, NULL, ch, TO_CHAR);
        return;
    }

    if (IS_IMMORTAL(victim)) {
        if (ch->level == IMPLEMENTOR) {
#if defined(unix)
            sprintf (buf, "%s%s", GOD_DIR, capitalize(victim->name));
            unlink (buf);
#endif
        }
        else {
            sendch("{ROnly an implementor may incinerate an immortal.\n\r",ch);
            act ("{R$N just attempted to incinerate you.{x", victim, NULL, ch, TO_CHAR);
            return;
        }
    }

    act ("{ROkay. $N enters the flames.{x", ch, NULL, victim, TO_CHAR);
    sendch("{RYour character has been deleted.{x\n\r", victim);
    sprintf(buf, "{R%s has been incinerated!{x\n\r", victim->name);
    for (d = descriptor_list; d; d = d->next)
        if (d->connected == CON_PLAYING)
            sendch (buf, d->character);
    logstr (LOG_SECURITY, "[*****] INCINERATE: %s was deleted by %s", victim->name, ch->name);

    d = victim->desc;
    extract_char (victim, TRUE);
    if (d)
        close_socket (d);
    sprintf (buf, "%s%s", PLAYER_DIR, capitalize(victim->name));
    unlink (buf);
    return;
}


/*
 * Coded by: Thale (Andrew Maslin)
 * Syntax: Rename <victim> <new_name>
 * Limitations: This header must be kept with this function.  In addition,
 * this file is subject to the ROM license.  The code in this file is
 * copywritten by Andrew Maslin, 1998.  If you have a "credits" help in your
 * mud, please add the name Thale to that as credit for this function.
 */
void do_rename(CHAR_DATA *ch, char *argument) {
    CHAR_DATA *victim;
    FILE *fp;
    char strsave[MAX_INPUT_LENGTH];
    char *name;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    char playerfile[MAX_INPUT_LENGTH];

    argument = one_argument(argument,arg1);
    argument = one_argument(argument,arg2);

    if (arg1[0] == '\0') {
        sendch("Rename who?\n\r",ch);
        return;
    }
    if (arg2[0] == '\0') {
        sendch("What should their new name be?\n\r",ch);
        return;
    }

    arg2[0] = UPPER(arg2[0]);

    if ((victim = get_char_world(ch,arg1)) == NULL) {
        sendch("They aren't connected.\n\r",ch);
        return;
    }

    if (IS_NPC(victim)) {
        sendch("Use string for NPC's.\n\r",ch);
        return;
    }

    if (!check_parse_name(arg2)) {
        sprintf(buf,"The name {c%s{x is {Rnot allowed{x.\n\r",arg2);
        sendch(buf,ch);
        return;
    }

    sprintf(playerfile, "%s%s", PLAYER_DIR, capitalize(arg2));
    if ((fp = fopen(playerfile, "r")) != NULL) {
        sprintf(buf,"There is already someone named %s.\n\r",capitalize(arg2));
        sendch(buf,ch);
        return;
    }

    if ((victim->level >= ch->level)
         && (ch->level != IMPLEMENTOR)
         && (ch != victim)) {
        sendch("I don't think that's a good idea.\n\r",ch);
        return;
    }

    // Why?
    if (victim->position == POS_FIGHTING) {
        sendch("They are fighting right now.\n\r",ch);
        return;
    }

    name = str_dup(victim->name);
    sprintf (strsave, "%s%s", PLAYER_DIR, capitalize(victim->name));
    arg2[0] = UPPER(arg2[0]);
    free_string(victim->name);
    victim->name = str_dup(arg2);
    save_char_obj(victim);
    unlink(strsave);
#if defined(unix)
    if (IS_IMMORTAL(victim)) {
        sprintf(strsave,"%s%s", GOD_DIR, capitalize(name));
        unlink(strsave);
    }
#endif

    if (victim != ch) {
        sprintf(buf,"{YNOTICE: {xYou have been renamed to {c%s{x.\n\r",arg2);
        sendch(buf,victim);
    }
    sendch("Done.\n\r",ch);

    return;
}


void do_multilink (CHAR_DATA *ch, char *argument) {
    DESCRIPTOR_DATA *d;
    char *host[500];
    CHAR_DATA *host_ch[500];
    char buf[MAX_STRING_LENGTH];
    bool found;
    bool multiplayer = FALSE;
    int h;

    sendch ("{cMulti-linkers:\n\r"
                  "{Y==============\n\r", ch);

    host[0] = NULL;
    for (d = descriptor_list; d; d = d->next) {
        if (!d->character) // Make sure its got a player
            continue;
        found = FALSE;
        for (h = 0; host[h] && h < 500; ++h) {
            if (!str_cmp(host[h], d->host)) {
                sprintf (buf, "   {W%s%s {cand {W%s%s{x\n\r",
                         IS_IMMORTAL(d->character) ? "[{BIMM{W]" : "", d->character->name,
                         host_ch[h] ? (IS_IMMORTAL(host_ch[h]) ? "[{BIMM{W]" : "") : "",
                         host_ch[h] ? host_ch[h]->name : "(not playing)");
                sendch (buf, ch);
                found = TRUE;
                multiplayer = TRUE;
            }
        }
        if (!found) {
            host[h] = d->host;
            host_ch[h] = d->character;
            host[h+1] = NULL;
        }
    }

    if (!multiplayer)
        sendch ("   {WNone.{c\n\r", ch);

    return;
}


void do_auto_shutdown() {
/*This allows for a shutdown without somebody in-game actually calling it.
		-Ferric*/
    FILE *fp;
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;
    merc_down = TRUE;

    /* This is to write to the file. */
    fclose(fpReserve);
    if((fp = fopen(LAST_COMMAND,"a")) == NULL)
        logstr (LOG_BUG, "Error in do_auto_save opening last_command.txt",0);

    fprintf(fp,"Last Command: %s\n", last_command);

    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );

    for (d = descriptor_list; d != NULL; d = d->next)
        if (d->character && IS_FUSED(d->character))
            unfuse(d->character);

    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj (vch);
        close_socket (d);
    }
    return;
}

void do_rlist (CHAR_DATA *ch, char *argument) {
    ROOM_INDEX_DATA *pRoomIndex;
    AREA_DATA *pArea;
	BUFFER *buf1;
	char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
	bool found;
    int vnum;
    int col = 0;

    one_argument (argument, arg);

    pArea = ch->in_room->area;
    buf1 = new_buf ();
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pRoomIndex = get_room_index (vnum))) {
            found = TRUE;
            sprintf (buf, "[%5d] %-16.16s ", vnum, capitalize (pRoomIndex->name));
            add_buf (buf1, buf);
            if (++col % 3 == 0)
                add_buf (buf1, "\n\r");
        }
    }

    if (!found) {
        sendch ("Room(s) not found in this area.\n\r", ch);
        return;
    }
    else if (col % 3 != 0)
        add_buf (buf1, "\n\r");

    page_to_char (buf_string (buf1), ch);
    free_buf (buf1);
    return;
}

void do_mlist (CHAR_DATA *ch, char *argument) {
    MOB_INDEX_DATA *pMobIndex;
    AREA_DATA *pArea;
    BUFFER *buf1;
	char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    bool fAll, found;
    int vnum;
    int col = 0;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        sendch ("Syntax:  mlist <all/name>\n\r", ch);
        return;
    }

    buf1 = new_buf ();
    pArea = ch->in_room->area;
    fAll = !str_cmp (arg, "all");
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pMobIndex = get_mob_index (vnum)) != NULL) {
            if (fAll || is_name (arg, pMobIndex->player_name)) {
                found = TRUE;
                sprintf (buf, "[%5d] %-16.16s ", pMobIndex->vnum, capitalize (pMobIndex->short_descr));
                add_buf (buf1, buf);
                if (++col % 3 == 0)
                    add_buf (buf1, "\n\r");
            }
        }
    }

    if (!found) {
        sendch ("Mobile(s) not found in this area.\n\r", ch);
        return;
    }
    else if (col % 3 != 0)
        add_buf (buf1, "\n\r");

    page_to_char (buf_string (buf1), ch);
    free_buf (buf1);
    return;
}

void do_olist (CHAR_DATA *ch, char *argument) {
    OBJ_INDEX_DATA *pObjIndex;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buf1;
    char arg[MAX_INPUT_LENGTH];
    bool fAll, found;
    int vnum;
    int col = 0;

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        sendch ("Syntax:  olist <all/name/item_type>\n\r", ch);
        return;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf ();
    fAll = !str_cmp (arg, "all");
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pObjIndex = get_obj_index (vnum))) {
            if (fAll 
				|| is_name (arg, pObjIndex->name) 
				|| flag_value (type_flags, arg) == pObjIndex->item_type) {
                found = TRUE;
                sprintf (buf, "[%5d] %-16.16s ", pObjIndex->vnum, capitalize (pObjIndex->short_descr));
                add_buf (buf1, buf);
                if (++col % 3 == 0)
                    add_buf (buf1, "\n\r");
            }
        }
    }

    if (!found) {
        sendch ("Object(s) not found in this area.\n\r", ch);
        return;
    }
    else if (col % 3 != 0)
        add_buf (buf1, "\n\r");

    page_to_char (buf_string (buf1), ch);
    free_buf (buf1);
    return;
}


// Get the first line of szString and stick it in szLine.
// Only gets the first 20 characters. Or you could just
// use %20.20s ...
void GetFirstLine (char *szString, char *szLine, int nMax) {
	char *s, *l;
	int nCount = 0;
    l = szLine;
	s = szString;
	while (TRUE) {
		if (++nCount > nMax) {
			*l = '\0';
			strcat (szLine, " ...");
			break;
		}
		else if (*s == '\n' || *s == '\0') {
            *l = '\0';
			break;
		}
		else {
            *l = *s;
			++l;
			++s;
		}
	}
	return;
}

void do_rplist (CHAR_DATA *ch, char *argument) {
    PROG_CODE *pProg;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
	char szLine[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
    BUFFER *buf1;
    bool bAll, found;
    int vnum;
	int col = 0;

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        sendch ("Syntax:  rplist <all/name>\n\r", ch);
        return;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf ();
	bAll = !str_cmp (arg, "all");
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pProg = get_prog_index (vnum, PRG_RPROG))) {
            if (bAll 
				|| is_name (arg, pProg->code)) {
				found = TRUE;
				GetFirstLine (pProg->code, szLine, 25);
				sprintf (buf, "[%5d] %-29.29s ", vnum, szLine);
				add_buf (buf1, buf);
				if (++col % 2 == 0)
					add_buf (buf1, "\n\r");
			}
        }
    }

    if (!found) {
        sendch ("Room program(s) not found in this area.\n\r", ch);
        return;
    }
    else if (col % 2 != 0)
        add_buf (buf1, "\n\r");

    page_to_char (buf_string (buf1), ch);
    free_buf (buf1);
    return;
}

void do_mplist (CHAR_DATA *ch, char *argument) {
    PROG_CODE *pProg;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
	char szLine[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
    BUFFER *buf1;
    bool bAll, found;
    int vnum;
	int col = 0;

	one_argument (argument, arg);
    if (arg[0] == '\0') {
        sendch ("Syntax:  mplist <all/name>\n\r", ch);
        return;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf ();
	bAll = !str_cmp (arg, "all");
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pProg = get_prog_index (vnum, PRG_MPROG))) {
            if (bAll 
				|| is_name (arg, pProg->code)) {
				found = TRUE;
				GetFirstLine (pProg->code, szLine, 25);
				sprintf (buf, "[%5d] %-29.29s ", vnum, szLine);
				add_buf (buf1, buf);
				if (++col % 2 == 0)
					add_buf (buf1, "\n\r");
			}
        }
    }

    if (!found) {
        sendch ("Mob program(s) not found in this area.\n\r", ch);
        return;
    }
    else if (col % 2 != 0)
        add_buf (buf1, "\n\r");

    page_to_char (buf_string (buf1), ch);
    free_buf (buf1);
    return;
}

void do_oplist (CHAR_DATA *ch, char *argument) {
    PROG_CODE *pProg;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
	char szLine[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
    BUFFER *buf1;
    bool bAll, found;
    int vnum;
	int col = 0;

	one_argument (argument, arg);
    if (arg[0] == '\0') {
        sendch ("Syntax:  oplist <all/name>\n\r", ch);
        return;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf ();
	bAll = !str_cmp (arg, "all");
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pProg = get_prog_index (vnum, PRG_OPROG))) {
            if (bAll 
				|| is_name (arg, pProg->code)) {
				found = TRUE;
				GetFirstLine(pProg->code, szLine, 25);
				sprintf (buf, "[%5d] %-29.29s ", vnum, szLine);
				add_buf (buf1, buf);
				if (++col % 2 == 0)
					add_buf (buf1, "\n\r");
			}
        }
    }

    if (!found) {
        sendch ("Object program(s) not found in this area.\n\r", ch);
        return;
    }
    else if (col % 2 != 0)
        add_buf (buf1, "\n\r");

    page_to_char (buf_string (buf1), ch);
    free_buf (buf1);
    return;
}

void do_givecustom (CHAR_DATA *ch, char *argument) {
    CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument (argument, arg);

	if ((victim = get_char_world (ch, arg)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

	if (IS_NPC (victim)) {
        sendch ("Not on NPCs.\n\r", ch);
		return;
	}

    act ("$E may now create a custom skill.", ch, NULL, victim, TO_CHAR);
	sendch ("You may now create a custom skill.\n\r", victim);

	victim->pcdata->lCsFlags = 0;
	victim->pcdata->nCsPoints = 50;
	victim->pcdata->bCsConfirm = FALSE;
	free_string (victim->pcdata->szCsName);
	victim->pcdata->szCsName = NULL;
}

void do_reward (CHAR_DATA *ch, char *argument) {
     CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument (argument, arg);

	if ((victim = get_char_world (ch, arg)) == NULL) {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

	if (IS_NPC (victim)) {
        sendch ("Not on NPCs.\n\r", ch);
		return;
	}

    act ("$E has been rewarded.", ch, NULL, victim, TO_CHAR);
    sendch ("You have been rewarded.\n\r", victim);

    victim->pcdata->nReward = 60;
}


// Adapted from do_mpgainskill
void do_grant (CHAR_DATA *ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int sn;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (arg1[0] == '\0' || arg2[0] == '\0') {
		printf_to_char (ch, "Syntax: grant [skill] [character]\n\r");
		return;
    }

    sn = skill_lookup (arg1);
    if (sn < 0) {
		printf_to_char (ch, "That isn't a skill.\n\r");
		return;
    }
    
    if ((victim = get_char_world(ch, arg2)) == NULL) {
		printf_to_char (ch, "They aren't here.");
	    return;
	}
    
    if (victim->pcdata->learned[sn] > 0) {
        act ("$E already knows the skill.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (!MeetsPrereq(victim, sn)) {
        act ("$E doesn't meet the prerequisites.", ch, NULL, victim, TO_CHAR);
        return;
    }

    victim->pcdata->learned[sn] = 1;
    act ("$N grants you $t.", victim, skill_table[sn].name, ch, TO_CHAR);
    return;
}


void do_backup (CHAR_DATA *ch, char *argument) {
	extern int pulse_backup;
	pulse_backup = 0;

	sendch ("Game backup prepared.\n\r", ch);
}

