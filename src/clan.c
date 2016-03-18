/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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
*	ROM 2.4 is copyright 1993-1996 Russ Taylor		                 	   *
*	ROM has been brought to you by the ROM consortium		               *
*	    Russ Taylor (rtaylor@efn.org)				                       *
*	    Gabrielle Taylor						                           *
*	    Brian Moore (zump@rom.org)					                       *
*	By using this code, you have agreed to follow the terms of the	       *
*	ROM license, in the file Rom24/doc/rom.license			               *
***************************************************************************/

/***************************************************************************
*       This file is for clan related commands.  Included here is:         *
*                 The JOIN command.                                        *
*                 The immortal CLEADER command.                            *
*                 The clan leader ACCEPT command.                          *
*                 The clan leader LONER command.                           *
*       Clans can be found in the clan table in tables.c                   *
*                             -Blizzard (blizzard_imp@hotmail.com)         *
***************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

void do_cleader (CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if (arg[0] == '\0') {
        sendch( "Syntax: CLEADER <name>\n\r", ch );
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL ) {
        sendch( "They aren't here.\n\r", ch );
        return;
    }

    if (victim->level >= ch->level) {
        sendch( "You failed.\n\r", ch );
        return;
    }

    if (IS_SET(victim->act, PLR_LEADER)) {
        REMOVE_BIT(victim->act, PLR_LEADER);
        sendch( "The gods have revoked your leadership priviliges.\n\r", victim );
        sendch( "Leadership removed.\n\r", ch );
        //sprintf(buf,"$N takes leadership away from %s",victim->name);
    }
    else {
        SET_BIT(victim->act, PLR_LEADER);
        sendch( "The gods have made you a clan leader.\n\r", victim );
        sendch( "Leadership set.\n\r", ch );
        //sprintf(buf,"$N make %s a clan leader.",victim->name);
    }

    return;
}

void do_join (CHAR_DATA *ch, char *argument) {
    DESCRIPTOR_DATA *d;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    int clan;

    if (IS_SET(ch->act, PLR_LEADER)) {
        sendch("You can't join another clan while you are currently a leader!\n\r"
                     "Step down from leadership first.\n\r", ch);
        return;
    }

    one_argument( argument, arg );

    if (arg[0] == '\0') {
        sendch( "What clan do you wish to join? <join none> to cancel.\n\r", ch );
        return;
    }

    if (!str_cmp(arg, "none")) {
        sendch("You no longer wish to join a clan.\n\r", ch);
        ch->petition = 0;
        return;
    }

    if ((clan = clan_lookup(arg)) == 0) {
        sendch("There is no clan by that name.\n\r", ch);
        return;
    }

    if (clan == ch->clan) {
        sendch ("You are already a memeber of that clan!\n\r", ch);
        return;
    }

    sprintf (buf, "Ok. You have applied to %s.\n\r", clan_table[clan].name);
    sendch(buf, ch);
    ch->petition = clan;

    sprintf (buf, "%s has petitioned to join your clan.\n\r", ch->name);
    for (d = descriptor_list; d != NULL; d = d->next)
        if (IS_SET(d->character->act, PLR_LEADER) && clan == d->character->clan)
            sendch (buf, d->character);

    return;
}

void do_accept (CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (!IS_SET(ch->act, PLR_LEADER))
    {
        sendch("You are not a clan leader.\n\r", ch);
        return;
    }

    one_argument( argument, arg );

    if (arg[0] == '\0') {
        DESCRIPTOR_DATA *d;
        char buf[MAX_STRING_LENGTH];
        bool found = FALSE;

        sendch ("Pending applications:\n\r", ch);
        for (d = descriptor_list; d != NULL; d = d->next) {
            if (d->character->petition == ch->clan) {
                sprintf (buf, "  %s\n\r", d->character->name);
                sendch (buf, ch);
                found = TRUE;
            }
        }
        if (!found)
            sendch ("None.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        sendch("They are not playing.\n\r", ch);
        return;
    }

    if (victim->petition != ch->clan) {
        sendch("They do not wish to join your clan.\n\r", ch);
        return;
    }

    victim->clan = ch->clan;
    victim->petition = 0;
    sendch("You have accepted them into your clan.\n\r", ch);
    sendch("Your clan application was successful.\n\r", victim);
    return;
 }

void do_clanremove (CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (!IS_SET(ch->act, PLR_LEADER)) {
        sendch("You are not a clan leader.\n\r", ch);
        return;
    }

    one_argument( argument, arg );

    if (arg[0] == '\0') {
        sendch( "Remove who?\n\r", ch );
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
        sendch("They are not playing.\n\r", ch);
        return;
    }

    if (victim->clan == 0)
    {
        sendch("They're not in a clan in the first place.\n\r", ch);
        return;
    }

    if (victim->clan != ch->clan)
    {
        sendch("They're not in your clan.\n\r", ch);
        return;
    }

    victim->clan = 0;
    victim->petition = 0;
    sendch("You have removed them from your clan.\n\r", ch);
    sendch("You have been removed from your clan.\n\r", victim);
    return;
}



void do_loner( CHAR_DATA *ch, char *argument )
 {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (!IS_SET(ch->act, PLR_LEADER)) {
        sendch("You are not a clan leader.\n\r", ch);
        return;
    }

    one_argument( argument, arg );

    if (arg[0] == '\0') {
        sendch( "Loner who?\n\r", ch );
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        sendch("They are not playing.\n\r", ch);
        return;
    }

    if (victim->clan == 0)
    {
        sendch("They're not in a clan in the first place.\n\r", ch);
        return;
    }

    if (victim->clan != ch->clan)
    {
        sendch("They're not in your clan.\n\r", ch);
        return;
    }

    victim->clan = 1;
    victim->petition = 0;
    sendch ("You have made them a loner.\n\r", ch);
    sendch ("You have been made a loner.\n\r", victim);
    return;
 }



