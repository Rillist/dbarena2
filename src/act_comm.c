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

/*   QuickMUD - The Lazy Man's ROM - $Id: act_comm.c,v 1.2 2000/12/01 10:48:33 ring0 Exp $ */
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include <ctype.h> /* for isalpha() and isspace() -- JR */

// Local functions
void channel args ((CHAR_DATA *ch, char *argument, int chan));

// For all the channels
#define CHAN_AUCTION   0
#define CHAN_GOSSIP    1
#define CHAN_OOC       2
#define CHAN_GRATS     3
#define CHAN_QUOTE     4
#define CHAN_QUESTION  5
#define CHAN_ANSWER    6
#define CHAN_MUSIC     7
#define CHAN_CLAN      8
#define CHAN_IMM       9
#define CHAN_GOD      10

int chan_nobit[11] = {COMM_NOAUCTION, COMM_NOGOSSIP,   COMM_NOOOC,      COMM_NOGRATS,
                      COMM_NOQUOTE,   COMM_NOQUESTION, COMM_NOQUESTION, COMM_NOMUSIC,
                      COMM_NOCLAN,    COMM_NOWIZ,      COMM_NOGOD};
char *chan_name[11] = {"Auction", "Gossip", "OOC", "Grats", "Quote", "Q/A", "Q/A", "Music", "Clan", "Immortal", "GOD"};
char *chan_personaltext[11] = {"{aYou auction '{A%s{a'{x\n\r", "{dYou gossip '{9%s{d'{x\n\r",
                               "{bYou ooc '{B%s{b'{x\n\r",     "{tYou grats '%s'{x\n\r",
                               "{hYou quote '{H%s{h'{x\n\r",   "{qYou question '{Q%s{q'{x\n\r",
                               "{qYou answer '{Q%s{q'{x\n\r",  "{eYou MUSIC: '{E%s{e'{x\n\r",
                               "{tYou clan '{T%s{t'{x\n\r",        "{i[{IYou{i]: %s{x\n\r",
                               "{W<<{BYou{W>>: {c%s{x\n\r"};
char *chan_acttext[11] = {"{a$n auctions '{A$t{a'{x", "{d$n gossips '{9$t{d'{x",
                          "{b$n oocs '{B$t{b'{x",     "{t$n grats '$t'{x",
                          "{h$n quotes '{H$t{h'{x",   "{q$n questions '{Q$t{q'{x",
                          "{q$n answers '{Q$t{q'{x",  "{e$n MUSIC: '{E$t{e'{x",
                          "{t$n clans '{T$t{t'{x",    "{i[{I$n{i]: $t{x",
                          "{W<<{B$n{W>>: {c$t{x"};

/* RT code to delete yourself */

void do_delet (CHAR_DATA * ch, char *argument)
{
    sendch ("You must type the full command to delete yourself.\n\r",
                  ch);
}

void do_delete (CHAR_DATA * ch, char *argument)
{
    char strsave[MAX_INPUT_LENGTH];

    if (IS_NPC (ch))
        return;

    if (IS_FUSED(ch)) {
        sendch ("You cannot delete while fused.\n\r", ch);
        return;
    }

    if (ch->pcdata->confirm_delete)
    {
        if (argument[0] != '\0')
        {
            sendch ("Delete status removed.\n\r", ch);
            ch->pcdata->confirm_delete = FALSE;
            return;
        }
        else
        {
            sprintf (strsave, "%s%s", PLAYER_DIR, capitalize (ch->name));
            wiznet ("$N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
            stop_fighting (ch, TRUE);
            do_function (ch, &do_quit, "");
            unlink (strsave);
            return;
        }
    }

    if (argument[0] != '\0')
    {
        sendch ("Just type delete. No argument.\n\r", ch);
        return;
    }

    sendch ("Type delete again to confirm this command.\n\r", ch);
    sendch ("WARNING: this command is irreversible.\n\r", ch);
    sendch
        ("Typing delete with an argument will undo delete status.\n\r", ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet ("$N is contemplating deletion.", ch, NULL, 0, 0, ch->level);
}


/* RT code to display channel status */

void do_channels (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    sendch ("   channel     status\n\r", ch);
    sendch ("---------------------\n\r", ch);

    sendch ("{dgossip{x         ", ch);
    if (!IS_SET (ch->comm, COMM_NOGOSSIP))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{aauction{x        ", ch);
    if (!IS_SET (ch->comm, COMM_NOAUCTION))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{emusic{x          ", ch);
    if (!IS_SET (ch->comm, COMM_NOMUSIC))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{qQ{x/{qA{x            ", ch);
    if (!IS_SET (ch->comm, COMM_NOQUESTION))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{hQuote{x          ", ch);
    if (!IS_SET (ch->comm, COMM_NOQUOTE))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{tgrats{x          ", ch);
    if (!IS_SET (ch->comm, COMM_NOGRATS))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    if (IS_IMMORTAL (ch))
    {
        sendch ("{iimm channel{x    ", ch);
        if (!IS_SET (ch->comm, COMM_NOWIZ))
            sendch ("ON\n\r", ch);
        else
            sendch ("OFF\n\r", ch);
    }

    if (ch->level > HEADBUILDER)
    {
        sendch ("{BGOD channel{x    ",ch);
        if (!IS_SET (ch->comm, COMM_NOGOD))
            sendch ("ON\n\r", ch);
        else
            sendch ("OFF\n\r", ch);
    }

    sendch ("{tshouts{x         ", ch);
    if (!IS_SET (ch->comm, COMM_SHOUTSOFF))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{ktells{x          ", ch);
    if (!IS_SET (ch->comm, COMM_DEAF))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    sendch ("{tquiet mode{x     ", ch);
    if (IS_SET (ch->comm, COMM_QUIET))
        sendch ("ON\n\r", ch);
    else
        sendch ("OFF\n\r", ch);

    if (IS_SET (ch->comm, COMM_AFK))
        sendch ("You are AFK.\n\r", ch);

    if (IS_SET (ch->comm, COMM_SNOOP_PROOF))
        sendch ("You are immune to snooping.\n\r", ch);

    if (ch->lines != PAGELEN)
    {
        if (ch->lines)
        {
            sprintf (buf, "You display %d lines of scroll.\n\r",
                     ch->lines + 2);
            sendch (buf, ch);
        }
        else
            sendch ("Scroll buffering is off.\n\r", ch);
    }

    if (ch->prompt != NULL)
    {
        sprintf (buf, "Your current prompt is: %s\n\r", ch->prompt);
        sendch (buf, ch);
    }

    if (IS_SET (ch->comm, COMM_NOSHOUT))
        sendch ("You cannot shout.\n\r", ch);

    if (IS_SET (ch->comm, COMM_NOTELL))
        sendch ("You cannot use tell.\n\r", ch);

    if (IS_SET (ch->comm, COMM_NOCHANNELS))
        sendch ("You cannot use channels.\n\r", ch);

    if (IS_SET (ch->comm, COMM_NOEMOTE))
        sendch ("You cannot show emotions.\n\r", ch);

}

/* RT deaf blocks out all shouts */

void do_deaf (CHAR_DATA * ch, char *argument)
{

    if (IS_SET (ch->comm, COMM_DEAF))
    {
        sendch ("You can now hear tells again.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_DEAF);
    }
    else
    {
        sendch ("From now on, you won't hear tells.\n\r", ch);
        SET_BIT (ch->comm, COMM_DEAF);
    }
}

/* RT quiet blocks out all communication */

void do_quiet (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_QUIET))
    {
        sendch ("Quiet mode removed.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_QUIET);
    }
    else
    {
        sendch ("From now on, you will only hear says and emotes.\n\r",
                      ch);
        SET_BIT (ch->comm, COMM_QUIET);
    }
}

/* afk command */

void do_afk (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_AFK))
    {
        sendch ("AFK mode removed. Type 'replay' to see tells.\n\r",
                      ch);
        REMOVE_BIT (ch->comm, COMM_AFK);
    }
    else
    {
        sendch ("You are now in AFK mode.\n\r", ch);
        SET_BIT (ch->comm, COMM_AFK);
    }
}

void do_replay (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
    {
        sendch ("You can't replay.\n\r", ch);
        return;
    }

    if (buf_string (ch->pcdata->buffer)[0] == '\0')
    {
        sendch ("You have no tells to replay.\n\r", ch);
        return;
    }

    page_to_char (buf_string (ch->pcdata->buffer), ch);
    clear_buf (ch->pcdata->buffer);
}



void channel (CHAR_DATA *ch, char *argument, int chan) {
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    if (argument[0] == '\0') {
        if (IS_SET (ch->comm, chan_nobit[chan])) {
            sprintf (buf, "{a%s channel is now ON.{x\n\r", chan_name[chan]);
            sendch (buf, ch);
            REMOVE_BIT (ch->comm, chan_nobit[chan]);
        }
        else {
            sprintf (buf, "{a%s channel is now OFF.{x\n\r", chan_name[chan]);
            sendch (buf, ch);
            SET_BIT (ch->comm, chan_nobit[chan]);
        }
    }
    else {
        if (IS_SET (ch->comm, COMM_QUIET)) {
            sendch ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS)) {
            sendch ("The gods have revoked your channel priviliges.\n\r", ch);
            return;
        }

        REMOVE_BIT (ch->comm, chan_nobit[chan]);

        sprintf (buf, chan_personaltext[chan], argument);
        sendch (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next) {
            victim = d->original ? d->original : d->character;

            if (d->connected != CON_PLAYING || victim == ch)
                continue;
            else if (IS_SET (victim->comm, chan_nobit[chan]))
                continue;
            else if (chan == CHAN_CLAN && ch->clan != victim->clan)
                continue;
            else if (chan == CHAN_IMM && !IS_IMMORTAL(victim))
                continue;
            else if (chan == CHAN_GOD && victim->level < HEADBUILDER)
                continue;
            else if (chan != CHAN_GOD && chan != CHAN_IMM && IS_SET (victim->comm, COMM_QUIET))
                continue;

            act_new (chan_acttext[chan], ch, argument, victim, TO_VICT, POS_DEAD);
        }
    }
    return;
}

void do_auction (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_AUCTION);
}

void do_gossip (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_GOSSIP);
}

void do_ooc (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_OOC);
}

void do_grats (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_GRATS);
}

void do_quote (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_QUOTE);
}

void do_question (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_QUESTION);
}

void do_answer (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_ANSWER);
}

void do_music (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_MUSIC);
}

void do_clantalk (CHAR_DATA * ch, char *argument) {
    if (!is_clan (ch) || clan_table[ch->clan].independent) {
        sendch ("You aren't in a clan.\n\r", ch);
        return;
    }
    channel (ch, argument, CHAN_CLAN);
}

void do_immtalk (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_IMM);
}

void do_god (CHAR_DATA * ch, char *argument) {
    channel (ch, argument, CHAN_GOD);
}

void do_gtell (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *gch;
	char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        sendch ("Tell your group what?\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_NOTELL))
    {
        sendch ("Your message didn't get through!\n\r", ch);
        return;
    }

    sprintf (buf, "You tell the group '%s'\n\r", argument);
	sendch (buf, ch);
	for (gch = char_list; gch != NULL; gch = gch->next)
    {
        if (is_same_group (gch, ch))
            act_new ("$n tells the group '$t'",
                     ch, argument, gch, TO_VICT, POS_SLEEPING);
    }

    return;
}


void do_say (CHAR_DATA * ch, char *argument)
{
    if (argument[0] == '\0')
    {
        sendch ("Say what?\n\r", ch);
        return;
    }

    act ("{6$n says '{7$T{6'{x", ch, NULL, argument, TO_ROOM);
    act ("{6You say '{7$T{6'{x", ch, NULL, argument, TO_CHAR);

    if (!IS_NPC (ch))
    {
        CHAR_DATA *mob, *mob_next;
		OBJ_DATA *obj, *obj_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next)
        {
            mob_next = mob->next_in_room;
            if (IS_NPC (mob) && HAS_TRIGGER_MOB (mob, TRIG_SPEECH)
                && mob->position == mob->pIndexData->default_pos)
                p_act_trigger (argument, mob, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);

			for ( obj = mob->carrying; obj; obj = obj_next ) {
				obj_next = obj->next_content;
				if ( HAS_TRIGGER_OBJ( obj, TRIG_SPEECH ) )
					p_act_trigger( argument, NULL, obj, NULL, ch, NULL, NULL, TRIG_SPEECH );
			}
        }
    }

	if ( ch->in_room && HAS_TRIGGER_ROOM( ch->in_room, TRIG_SPEECH ) )
	    p_act_trigger( argument, NULL, NULL, ch->in_room, ch, NULL, NULL, TRIG_SPEECH );

    return;
}



void do_shout (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_SHOUTSOFF))
        {
            sendch ("You can hear shouts again.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_SHOUTSOFF);
        }
        else
        {
            sendch ("You will no longer hear shouts.\n\r", ch);
            SET_BIT (ch->comm, COMM_SHOUTSOFF);
        }
        return;
    }

    if (IS_SET (ch->comm, COMM_NOSHOUT))
    {
        sendch ("You can't shout.\n\r", ch);
        return;
    }

    REMOVE_BIT (ch->comm, COMM_SHOUTSOFF);

    wait (ch, 12);

    act ("You shout '$T'", ch, NULL, argument, TO_CHAR);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            !IS_SET (victim->comm, COMM_SHOUTSOFF) &&
            !IS_SET (victim->comm, COMM_QUIET))
        {
            act ("$n shouts '$t'", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}



void do_tell (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (IS_SET (ch->comm, COMM_NOTELL) || IS_SET (ch->comm, COMM_DEAF))
    {
        sendch ("Your message didn't get through.\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_QUIET))
    {
        sendch ("You must turn off quiet mode first.\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_DEAF))
    {
        sendch ("You must turn off deaf mode first.\n\r", ch);
        return;
    }

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        sendch ("Tell whom what?\n\r", ch);
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ((victim = get_char_world (ch, arg)) == NULL
        || (IS_NPC (victim) && victim->in_room != ch->in_room))
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL && !IS_NPC (victim))
    {
        act ("$N seems to have misplaced $S link...try again later.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "{k%s tells you '{K%s{k'{x\n\r", PERS (ch, victim), argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }

    if (!(IS_IMMORTAL (ch) && ch->level > LEVEL_IMMORTAL)
        && !IS_AWAKE (victim))
    {
        act ("$E can't hear you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (
        (IS_SET (victim->comm, COMM_QUIET)
         || IS_SET (victim->comm, COMM_DEAF)) && !IS_IMMORTAL (ch))
    {
        act ("$E is not receiving tells.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (IS_SET (victim->comm, COMM_AFK))
    {
        if (IS_NPC (victim))
        {
            act ("$E is AFK, and not receiving tells.", ch, NULL, victim,
                 TO_CHAR);
            return;
        }

        act ("$E is AFK, but your tell will go through when $E returns.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "{k%s tells you '{K%s{k'{x\n\r", PERS (ch, victim),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }

    if (!IS_NPC(victim)) {
        if (victim->desc->connected >= CON_NOTE_TO && victim->desc->connected <= CON_NOTE_FINISH) {
		    act ("$E is writing a note, but your tell will go through when $E returns.", ch, NULL, victim, TO_CHAR);
		    sprintf (buf, "{k%s tells you '{K%s{k'{x\n\r", PERS (ch, victim), argument);
		    buf[0] = UPPER (buf[0]);
		    add_buf (victim->pcdata->buffer, buf);
		    return;
	    }
    }

    act ("{kYou tell $N '{K$t{k'{x", ch, argument, victim, TO_CHAR);
    act_new ("{k$n tells you '{K$t{k'{x", ch, argument, victim, TO_VICT, POS_DEAD);
    victim->reply = ch;

    if (!IS_NPC (ch) && IS_NPC (victim) && HAS_TRIGGER_MOB (victim, TRIG_SPEECH))
        p_act_trigger (argument, victim, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);

    return;
}



void do_reply (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET (ch->comm, COMM_NOTELL))
    {
        sendch ("Your message didn't get through.\n\r", ch);
        return;
    }

    if ((victim = ch->reply) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL && !IS_NPC (victim))
    {
        act ("$N seems to have misplaced $S link...try again later.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "{k%s tells you '{K%s{k'{x\n\r", PERS (ch, victim),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }

    if (!IS_IMMORTAL (ch) && !IS_AWAKE (victim))
    {
        act ("$E can't hear you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (
        (IS_SET (victim->comm, COMM_QUIET)
         || IS_SET (victim->comm, COMM_DEAF)) && !IS_IMMORTAL (ch)
        && !IS_IMMORTAL (victim))
    {
        act_new ("$E is not receiving tells.", ch, 0, victim, TO_CHAR,
                 POS_DEAD);
        return;
    }

    if (!IS_IMMORTAL (victim) && !IS_AWAKE (ch))
    {
        sendch ("In your dreams, or what?\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_AFK))
    {
        if (IS_NPC (victim))
        {
            act_new ("$E is AFK, and not receiving tells.",
                     ch, NULL, victim, TO_CHAR, POS_DEAD);
            return;
        }

        act_new ("$E is AFK, but your tell will go through when $E returns.",
                 ch, NULL, victim, TO_CHAR, POS_DEAD);
        sprintf (buf, "{k%s tells you '{K%s{k'{x\n\r", PERS (ch, victim),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }

    act_new ("{kYou tell $N '{K$t{k'{x", ch, argument, victim, TO_CHAR,
             POS_DEAD);
    act_new ("{k$n tells you '{K$t{k'{x", ch, argument, victim, TO_VICT,
             POS_DEAD);
    victim->reply = ch;

    return;
}



void do_yell (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (IS_SET (ch->comm, COMM_NOSHOUT))
    {
        sendch ("You can't yell.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        sendch ("Yell what?\n\r", ch);
        return;
    }


    act ("You yell '$t'", ch, argument, NULL, TO_CHAR);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character != ch
            && d->character->in_room != NULL
            && d->character->in_room->area == ch->in_room->area
            && !IS_SET (d->character->comm, COMM_QUIET))
        {
            act ("$n yells '$t'", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}


void do_emote (CHAR_DATA * ch, char *argument)
{
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

    /* little hack to fix the ',{' bug posted to rom list
     * around 4/16/01 -- JR
     */
    if (!(isalpha(argument[0])) || (isspace(argument[0])))
    {
	sendch ("Moron!\n\r", ch);
	return;
    }

    MOBtrigger = FALSE;
    act ("$n $T", ch, NULL, argument, TO_ROOM);
    act ("$n $T", ch, NULL, argument, TO_CHAR);
    MOBtrigger = TRUE;
    return;
}

void do_pmote (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *vch;
    char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE)) {
        sendch ("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        sendch ("Emote what?\n\r", ch);
        return;
    }

    /* little hack to fix the ',{' bug posted to rom list
     * around 4/16/01 -- JR
     */
    if (!(isalpha(argument[0])) || (isspace(argument[0]))) {
	    sendch ("Moron!\n\r", ch);
	    return;
    }

    act ("$n $t", ch, argument, NULL, TO_CHAR);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr (argument, vch->name)) == NULL) {
            MOBtrigger = FALSE;
            act ("$N $t", vch, argument, ch, TO_CHAR);
            MOBtrigger = TRUE;
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

        MOBtrigger = FALSE;
        act ("$N $t", vch, temp, ch, TO_CHAR);
        MOBtrigger = TRUE;
    }

    return;
}


void do_bug (CHAR_DATA * ch, char *argument)
{
    append_file (ch, BUG_FILE, argument);
    sendch ("Bug logged.\n\r", ch);
    return;
}

void do_typo (CHAR_DATA * ch, char *argument)
{
    append_file (ch, TYPO_FILE, argument);
    sendch ("Typo logged.\n\r", ch);
    return;
}

void do_qui (CHAR_DATA * ch, char *argument)
{
    sendch ("If you want to QUIT, you have to spell it out.\n\r", ch);
    return;
}



void do_quit (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d, *d_next;
    int id;

    if (IS_NPC (ch))
        return;

    if (IS_FUSED(ch)) {
        sendch ("You cannot quit while fused.\n\r", ch);
        return;
    }

    if (ch->position == POS_FIGHTING)
    {
        sendch ("No way! You are fighting.\n\r", ch);
        return;
    }

    if (ch->position < POS_STUNNED)
    {
        sendch ("You're not DEAD yet.\n\r", ch);
        return;
    }
    sendch ("The {YArena{x awaits your return...\n\r", ch);
    act ("$n has left the game.", ch, NULL, NULL, TO_ROOM);
    sprintf (log_buf, "%s has quit.", ch->name);
    log_string (log_buf);
    logstr (LOG_CONNECT, "%s has quit", ch->name);
    wiznet ("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0,
            ch->level);

    /*
     * After extract_char the ch is no longer valid!
     */
    save_char_obj (ch);

	/* Free note that might be there somehow */
	if (ch->pcdata->in_progress)
		free_note (ch->pcdata->in_progress);

    id = ch->id;
    d = ch->desc;
    extract_char (ch, TRUE);
    if (d != NULL)
        close_socket (d);

    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        CHAR_DATA *tch;

        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id)
        {
            extract_char (tch, TRUE);
            close_socket (d);
        }
    }

    return;
}



void do_save (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_FUSED(ch)) {
        sendch ("You cannot save while fused.\n\r", ch);
        return;
    }

    save_char_obj (ch);
    sendch ("        ***Saving Character***\n\r", ch);
    wait (ch, 2 * PULSE_SECOND);
    return;
}



void do_follow (CHAR_DATA * ch, char *argument)
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Follow whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM) && ch->master != NULL)
    {
        act ("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        if (ch->master == NULL)
        {
            sendch ("You already follow yourself.\n\r", ch);
            return;
        }
        stop_follower (ch);
        return;
    }

    if (!IS_NPC (victim) && IS_SET (victim->act, PLR_NOFOLLOW)
        && !IS_IMMORTAL (ch))
    {
        act ("$N doesn't seem to want any followers.\n\r", ch, NULL, victim,
             TO_CHAR);
        return;
    }

    REMOVE_BIT (ch->act, PLR_NOFOLLOW);

    if (ch->master != NULL)
        stop_follower (ch);

    add_follower (ch, victim);
    return;
}


void add_follower (CHAR_DATA * ch, CHAR_DATA * master)
{
    if (ch->master != NULL)
    {
        logstr (LOG_BUG, "Add_follower: non-null master.", 0);
        return;
    }

    ch->master = master;
    ch->leader = NULL;

    if (can_see (master, ch))
        act ("$n now follows you.", ch, NULL, master, TO_VICT);

    act ("You now follow $N.", ch, NULL, master, TO_CHAR);

    return;
}



void stop_follower (CHAR_DATA * ch)
{
    if (ch->master == NULL)
    {
        logstr (LOG_BUG, "Stop_follower: null master.", 0);
        return;
    }

    if (can_see (ch->master, ch) && ch->in_room != NULL)
    {
        act ("$n stops following you.", ch, NULL, ch->master, TO_VICT);
        act ("You stop following $N.", ch, NULL, ch->master, TO_CHAR);
    }
    if (ch->master->pet == ch)
        ch->master->pet = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets (CHAR_DATA * ch)
{
    CHAR_DATA *pet;

    if ((pet = ch->pet) != NULL)
    {
        stop_follower (pet);
        if (pet->in_room != NULL)
            act ("$N slowly fades away.", ch, NULL, pet, TO_NOTVICT);
        extract_char (pet, TRUE);
    }
    ch->pet = NULL;

    return;
}



void die_follower (CHAR_DATA * ch)
{
    CHAR_DATA *fch;

    if (ch->master != NULL)
    {
        if (ch->master->pet == ch)
            ch->master->pet = NULL;
        stop_follower (ch);
    }

    ch->leader = NULL;

    for (fch = char_list; fch != NULL; fch = fch->next)
    {
        if (fch->master == ch)
            stop_follower (fch);
        if (fch->leader == ch)
            fch->leader = fch;
    }

    return;
}



void do_order (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    argument = one_argument (argument, arg);
    one_argument (argument, arg2);

    if (!str_cmp (arg2, "delete") || !str_cmp (arg2, "mob"))
    {
        sendch ("That will NOT be done.\n\r", ch);
        return;
    }

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        sendch ("Order whom to do what?\n\r", ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM))
    {
        sendch ("You feel like taking, not giving, orders.\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        fAll = TRUE;
        victim = NULL;
    }
    else
    {
        fAll = FALSE;
        if ((victim = get_char_room (ch, NULL, arg)) == NULL)
        {
            sendch ("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch)
        {
            sendch ("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!IS_AFFECTED (victim, AFF_CHARM) || victim->master != ch
            || (IS_IMMORTAL (victim) && victim->level >= ch->level))
        {
            sendch ("Do it yourself!\n\r", ch);
            return;
        }
    }

    found = FALSE;
    for (och = ch->in_room->people; och != NULL; och = och_next)
    {
        och_next = och->next_in_room;

        if (IS_AFFECTED (och, AFF_CHARM)
            && och->master == ch && (fAll || och == victim))
        {
            found = TRUE;
            sprintf (buf, "$n orders you to '%s'.", argument);
            act (buf, ch, NULL, och, TO_VICT);
            interpret (och, argument);
        }
    }

    if (found)
    {
        wait (ch, PULSE_VIOLENCE);
        sendch ("Ok.\n\r", ch);
    }
    else
        sendch ("You have no followers here.\n\r", ch);
    return;
}



void do_group (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        CHAR_DATA *gch;
        CHAR_DATA *leader;

        leader = (ch->leader != NULL) ? ch->leader : ch;
        sprintf (buf, "%s's group:\n\r", PERS (leader, ch));
        sendch (buf, ch);

        for (gch = char_list; gch != NULL; gch = gch->next)
        {
            if (is_same_group (gch, ch))
            {
                sprintf (buf,
                         "[%5s] %-16s %7d/%-7d hp %7d/%-7d ki %4Ldpl\n\r",

						 IS_NPC (gch) ? "Mob" : (gch->race < MAX_PC_RACE ? pc_race_table[gch->race].who_name : "     "),
                         capitalize (PERS (gch, ch)), gch->hit, gch->max_hit,
                         gch->ki, gch->max_ki, gch->nCurPl * gch->llPl / 100);
                sendch (buf, ch);
            }
        }
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
        return;
    }

    if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch))
    {
        sendch ("But you are following someone else!\n\r", ch);
        return;
    }

    if (victim->master != ch && ch != victim)
    {
        act_new ("$N isn't following you.", ch, NULL, victim, TO_CHAR,
                 POS_SLEEPING);
        return;
    }

    if (IS_AFFECTED (victim, AFF_CHARM))
    {
        sendch ("You can't remove charmed mobs from your group.\n\r",
                      ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM))
    {
        act_new ("You like your master too much to leave $m!",
                 ch, NULL, victim, TO_VICT, POS_SLEEPING);
        return;
    }

    if (is_same_group (victim, ch) && ch != victim)
    {
        victim->leader = NULL;
        act_new ("$n removes $N from $s group.",
                 ch, NULL, victim, TO_NOTVICT, POS_RESTING);
        act_new ("$n removes you from $s group.",
                 ch, NULL, victim, TO_VICT, POS_SLEEPING);
        act_new ("You remove $N from your group.",
                 ch, NULL, victim, TO_CHAR, POS_SLEEPING);
        return;
    }

    victim->leader = ch;
    act_new ("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT,
             POS_RESTING);
    act_new ("You join $n's group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    act_new ("$N joins your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount = 0, share, extra;

    argument = one_argument (argument, arg1);
    one_argument (argument, arg2);

    if (arg1[0] == '\0')
    {
        sendch ("Split how much?\n\r", ch);
        return;
    }

    amount = atoi (arg1);

    if (amount < 0)
    {
        sendch ("Your group wouldn't like that.\n\r", ch);
        return;
    }

    if (amount == 0)
    {
        sendch ("You hand out zero coins, but no one notices.\n\r", ch);
        return;
    }

    if (ch->zenni < amount)
    {
        sendch ("You don't have that much to split.\n\r", ch);
        return;
    }

    members = 0;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
        if (is_same_group (gch, ch) && !IS_AFFECTED (gch, AFF_CHARM))
            members++;

    if (members < 2)
    {
        sendch ("Just keep it all.\n\r", ch);
        return;
    }

    share = amount / members;
    extra = amount % members;

    if (share == 0)
    {
        sendch ("Don't even bother, cheapskate.\n\r", ch);
        return;
    }

    ch->zenni -= amount;
    ch->zenni += share + extra;

    if (share > 0)
    {
        sprintf (buf, "You split %d zenni. Your share is %d zenni.\n\r", amount, share + extra);
        sendch (buf, ch);
    }

    sprintf (buf,
             "$n splits %d zenni, giving you %d zenni.\n\r",
             amount, share);

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (gch != ch && is_same_group (gch, ch)
            && !IS_AFFECTED (gch, AFF_CHARM))
        {
            act (buf, ch, NULL, gch, TO_VICT);
            gch->zenni += share;
        }
    }

    return;
}


/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group (CHAR_DATA * ach, CHAR_DATA * bch)
{
    if (ach == NULL || bch == NULL)
        return FALSE;

    if (ach->leader != NULL)
        ach = ach->leader;
    if (bch->leader != NULL)
        bch = bch->leader;
    return ach == bch;
}

void do_colour (CHAR_DATA * ch, char *argument) {
    if (IS_NPC (ch)) {
        sendch_bw ("Colour is not on!\n\r", ch);
        return;
    }

    if (IS_SET (ch->act, PLR_COLOUR)) {
        REMOVE_BIT (ch->act, PLR_COLOUR);
		sendch ("Colour is now off\n\r", ch);
    }
    else {
        SET_BIT (ch->act, PLR_COLOUR);
        sendch ("Colour is now on!\n\r", ch);
    }
    return;
}
