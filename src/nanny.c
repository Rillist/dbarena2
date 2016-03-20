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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
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

/****************************************************************************
 *   This file is just the stock nanny() function ripped from comm.c. It    *
 *   seems to be a popular task for new mud coders, so what the heck?       *
 ***************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>                /* OLC -- for close read write etc */
#include <stdarg.h>                /* printf_to_char */

#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"

#if    defined(unix)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "telnet.h"
extern const char echo_off_str[];
extern const char echo_on_str[];
extern const char go_ahead_str[];
#endif

/*
 * OS-dependent local functions.
 */
#if defined(unix)
void game_loop_unix args ((int control));
int init_socket args ((int port));
void init_descriptor args ((int control));
bool read_from_descriptor args ((DESCRIPTOR_DATA * d));
bool write_to_descriptor args ((int desc, char *txt, int length));
#endif

/*
 *  * Other local functions (OS-independent).
 *   */
bool check_parse_name args ((char *name));
bool check_reconnect args ((DESCRIPTOR_DATA * d, char *name, bool fConn));
bool check_playing args ((DESCRIPTOR_DATA * d, char *name));
void enter_game args ((DESCRIPTOR_DATA * d));

/*
 * Global variables.
 */
extern DESCRIPTOR_DATA *descriptor_list;    /* All open descriptors     */
extern DESCRIPTOR_DATA *d_next;        /* Next descriptor in loop  */
extern FILE *fpReserve;                /* Reserved file handle     */
extern bool god;                        /* All new chars are gods!  */
extern bool merc_down;                    /* Shutdown         */
extern bool wizlock;                    /* Game is wizlocked        */
extern bool newlock;                    /* Game is newlocked        */
extern char str_boot_time[MAX_INPUT_LENGTH];
extern time_t current_time;            /* time of this pulse */
extern int mud_telnetga, mud_ansicolor;

char *szSexData =
    "Select your sex:\n\r"
    "    {b[{B1.male  {b]\n\r"
    "    [{B2.female{b]{x\n\r"
    "Sex is a personal choice. Neither gender has any advantages.\n\r"
    "{B[{xGender{B]{x ";
char *szAlignData =
    "Select an alignment:\n\r"
    "    {b[{B1.good   {b]\n\r"
    "    [{B2.neutral{b]\n\r"
    "    [{B3.evil   {b]{x\n\r"
    "This is your general outlook on life; your moral values. Alignment is \n\r"
    " not permanent and changes dynamically.\n\r"
    "{B[{xAlignment{B]{x ";
void SendRaceData (DESCRIPTOR_DATA *d) {
    int i, j;
    char buf[MAX_STRING_LENGTH];

    send_to_desc ("Please select a race:\n\r", d);
    send_to_desc ("       Races                   Relative Stats\n\r", d);
    for (i = 0; i < MAX_PC_RACE; ++i) {
        sprintf (buf, "    {b[{B%d.%-13s{b]{x",
            i+1, pc_race_table[i].name);
        send_to_desc (buf, d);
        for (j = 0; j < MAX_STATS; ++j) {
            sprintf (buf, " %c%2.2s:%-2d", UPPER(stat_table[j][0]), stat_table[j]+1, pc_race_table[i].stat_gain[j]);
            send_to_desc (buf, d);
        }
        send_to_desc ("\n\r", d);
        sprintf (buf, "      {x%s\n\r", pc_race_table[i].szDescription);
        send_to_desc (buf, d);
    }
    send_to_desc (
        "Each race is as powerful as the next -- none are dominating.\n\r"
        "Type 'help <race>' for more information about that race.\n\r"
        "{B[{xRace{B]{x ", d);

}
void SendStatData (DESCRIPTOR_DATA *d) {
    CHAR_DATA *ch = d->character;
    int i;
    char buf[MAX_STRING_LENGTH];

    send_to_desc ("Customize Stats:\n\r", d);
    //if (ch->pcdata->nGenStatPoints > 0) {
        for (i = 0; i < MAX_STATS; ++i) {
            if (i == MAX_STATS-1) {
                continue;//hiding charisma
            }
            sprintf (buf, "    {b[{B%d.%-13s{b]{x  %d\n\r",
                i+1, stat_table[i], ch->perm_stat[i]);
            send_to_desc (buf, d);
        }
    //}
    sprintf (buf, "    {b[{B%d.reset        {b]{x\n\r", MAX_STATS+1);
    send_to_desc (buf, d);
    if (ch->pcdata->nGenStatPoints > 0) {
        sprintf (buf, "    {x%d points remaining\n\r", ch->pcdata->nGenStatPoints);
        send_to_desc (buf, d);
    }
    else {
        sprintf (buf, "    {b[{B%d.done         {b]{x\n\r", MAX_STATS+2);
        send_to_desc (buf, d);
    }
    send_to_desc ("Distribute points to statistics in order to customize your character.\n\r"
                  "{B[{xStat{B]{x ", d);
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny (DESCRIPTOR_DATA * d, char *argument)
{
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int race, i, sn, nValue;
    bool fOld;

    /* Delete leading spaces UNLESS character is writing a note */
	if (d->connected != CON_NOTE_TEXT)
	{
		while ( isspace(*argument) )
			argument++;
	}
    nValue = atoi(argument);
    ch = d->character;

    switch (d->connected)
    {

        default:
            logstr (LOG_BUG, "Nanny: bad d->connected %d.", d->connected);
            close_socket (d);
            return;

        case CON_ANSI:
            if (argument[0] == '\0' || UPPER (argument[0]) == 'Y')
            {
                d->ansi = TRUE;
                send_to_desc ("{RAnsi enabled!{x\n\r", d);
                d->connected = CON_GET_NAME;
                {
                    extern char *help_greeting;
                    if (help_greeting == NULL) {
                        logstr (LOG_CRIT, "Nanny: no help_greeting found (NULL). Setting default");
                        help_greeting = str_dup ("{RDefault greeting{x\n\r");
                    }
                    if (help_greeting[0] == '.')
                        send_to_desc (help_greeting + 1, d);
                    else
                        send_to_desc (help_greeting, d);
                }
                break;
            }

            if (UPPER (argument[0]) == 'N')
            {
                d->ansi = FALSE;
                send_to_desc ("Ansi disabled!\n\r", d);
                d->connected = CON_GET_NAME;
                {
                    extern char *help_greeting;
                    if (help_greeting == NULL) {
                        logstr (LOG_CRIT, "Nanny: no help_greeting found (NULL). Setting default");
                        help_greeting = str_dup ("{RDefault greeting{x\n\r");
                    }
                    if (help_greeting[0] == '.')
                        send_to_desc (help_greeting + 1, d);
                    else
                        send_to_desc (help_greeting, d);
                }
                break;
            }
            else
            {
                send_to_desc ("Do you want ANSI? Is this in {Cc{Go{Bl{Ro{Yu{Mr{x? (Y/N) ", d);
                return;
            }


        case CON_GET_NAME:
            if (argument[0] == '\0')
            {
                close_socket (d);
                return;
            }

            argument[0] = UPPER (argument[0]);
            if (!check_parse_name (argument))
            {
                send_to_desc ("Illegal name, try another.\n\r{B[{xName{B]{x ", d);
                return;
            }

            fOld = load_char_obj (d, argument);
            ch = d->character;

            if (IS_SET (ch->act, PLR_DENY))
            {
                logstr (LOG_SECURITY, "Denying access to %s@%s.", argument, d->host);
                send_to_desc ("You are denied access.\n\r", d);
                close_socket (d);
                return;
            }

            if (check_ban (d->host, BAN_PERMIT)
                && !IS_SET (ch->act, PLR_PERMIT))
            {
                send_to_desc ("Your site has been banned from this mud.\n\r", d);
                close_socket (d);
                return;
            }

            if (check_reconnect (d, argument, FALSE))
                fOld = TRUE;
            else {
                if (wizlock && !IS_IMMORTAL (ch)) {
                    send_to_desc ("The game is wizlocked.\n\r", d);
                    close_socket (d);
                    return;
                }
            }

            if (fOld)
            {
                /* Old player */
                send_to_desc ("{B[{xPassword{B]{x ", d);
                write_to_buffer (d, echo_off_str, 0);
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            }
            else
            {
                /* New player */
                if (newlock)
                {
                    send_to_desc ("The game is newlocked.\n\r", d);
                    close_socket (d);
                    return;
                }

                if (check_ban (d->host, BAN_NEWBIES))
                {
                    send_to_desc ("New players are not allowed from your site.\n\r", 0);
                    close_socket (d);
                    return;
                }

                sprintf (buf, "Make sure you have a proper name.  Names from the show,\n\r"
                              "for instance Goku, Gohan or Vegeta, are not allowed.  This\n\r"
                              "includes obvious variations such as Gotu, or Vegeto.\n\r"
                              "Immortals reserve final say; we can change your name at\n\r"
                              "moments notice!\n\r"
                              "\n\r"
                              "{B[{xIs %s correct? (Y/N){B]{x ", argument);
                send_to_desc (buf, d);
                d->connected = CON_CONFIRM_NEW_NAME;
                return;
            }
            break;

        case CON_GET_OLD_PASSWORD:
#if defined(unix)
            write_to_buffer (d, "\n\r", 2);
#endif

            if (strcmp (crypt (argument, ch->pcdata->pwd), ch->pcdata->pwd))
            {
                send_to_desc ("Wrong password!\n\r", d);
                close_socket (d);
                return;
            }

            write_to_buffer (d, echo_on_str, 0);

            if (check_playing (d, ch->name))
                return;

            if (check_reconnect (d, ch->name, TRUE))
                return;

            logstr (LOG_CONNECT, "%s@%s has connected.", ch->name, d->host);
            wiznet (log_buf, NULL, NULL, WIZ_SITES, 0, ch->level);

            if (ch->desc->ansi)
                SET_BIT (ch->act, PLR_COLOUR);
            else
                REMOVE_BIT (ch->act, PLR_COLOUR);

            /*
            if (IS_IMMORTAL (ch))
            {
                //do_function (ch, &do_help, "imotd");
                d->connected = CON_READ_MOTD;
            }
            else
            {
                //do_function (ch, &do_help, "motd");
                d->connected = CON_READ_MOTD;
            }
            */
            enter_game (d);
            break;

/* RT code for breaking link */

        case CON_BREAK_CONNECT:
            switch (*argument)
            {
                case 'y':
                case 'Y':
                    for (d_old = descriptor_list; d_old != NULL;
                         d_old = d_next)
                    {
                        d_next = d_old->next;
                        if (d_old == d || d_old->character == NULL)
                            continue;

                        if (str_cmp (ch->name, d_old->original ?
                                     d_old->original->name : d_old->
                                     character->name))
                            continue;

                        close_socket (d_old);
                    }
                    if (check_reconnect (d, ch->name, TRUE))
                        return;
                    send_to_desc ("Reconnect attempt failed.\n\r{B[{xName{B]{x ", d);
                    if (d->character != NULL)
                    {
                        free_char (d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                case 'n':
                case 'N':
                    send_to_desc ("{B[{xName{B]{x ", d);
                    if (d->character != NULL)
                    {
                        free_char (d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    send_to_desc ("(Y)es or (N)o? ", d);
                    break;
            }
            break;

        case CON_CONFIRM_NEW_NAME:
            switch (*argument)
            {
                case 'y':
                case 'Y':
                    sprintf (buf,
                             "New character!\n\r\n\r{B[{xWhat is your new password?{B]{x %s",
                             echo_off_str);
                    send_to_desc (buf, d);
                    d->connected = CON_GET_NEW_PASSWORD;
                    if (ch->desc->ansi)
                        SET_BIT (ch->act, PLR_COLOUR);
                    break;

                case 'n':
                case 'N':
                    send_to_desc ("{B[{xName{B]{x ", d);
                    free_char (d->character);
                    d->character = NULL;
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    send_to_desc ("(Y)es or (N)o? ", d);
                    break;
            }
            break;

        case CON_GET_NEW_PASSWORD:
            if (strlen (argument) < 5)
            {
                send_to_desc
                    ("\n\rPassword must be at least five characters long.\n\r{B[{xPassword{B]{x ", d);
                return;
            }

            pwdnew = crypt (argument, ch->name);
            for (p = pwdnew; *p != '\0'; p++)
            {
                if (*p == '~')
                {
                    send_to_desc ("\n\rNew password not acceptable, try again.\n\r{B[{xPassword{B]{x ", d);
                    return;
                }
            }

            free_string (ch->pcdata->pwd);
            ch->pcdata->pwd = str_dup (pwdnew);
            send_to_desc ("\n\r{B[{xRetype password{B]{x ", d);
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
            if (strcmp (crypt (argument, ch->pcdata->pwd), ch->pcdata->pwd))
            {
                send_to_desc ("\n\rPasswords don't match.\n\r{B[{xRetype password{B]{x ", d);
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }

            write_to_buffer (d, echo_on_str, 0);
            SendRaceData (d);
            d->connected = CON_GET_NEW_RACE;

            logstr (LOG_GAME, "%s@%s new player.", ch->name, d->host);
            wiznet ("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
            wiznet (log_buf, NULL, NULL, WIZ_SITES, 0, ch->level);
            break;

        case CON_GET_NEW_RACE:
            one_argument (argument, arg);

            if (!strcmp (arg, "help")) {
                argument = one_argument (argument, arg);
                if (argument[0] == '\0')
                    send_to_desc ("Type 'help <race>' for more information about that race.\n\r", d);
                else
                    do_function (ch, &do_help, argument);
                send_to_desc ("{B[{xRace{B]{x ", d);
                break;
            }

            race = nValue - 1;

            if (!race_table[race].pc_race  || !race_table[race].can_select) {
                send_to_desc ("Not a valid race!\n\r", d);
                SendRaceData (d);
                break;
            }

            ch->race = race;
            /* initialize stats */
            for (i = 0; i < MAX_STATS; i++)
                ch->perm_stat[i] = pc_race_table[race].stats[i];
            ch->pcdata->nGenStatPoints = 10;
            ch->affected_by = ch->affected_by | race_table[race].aff;
            ch->imm_flags = ch->imm_flags | race_table[race].imm;
            ch->res_flags = ch->res_flags | race_table[race].res;
            ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
            ch->form = race_table[race].form;
            ch->parts = race_table[race].parts;

            /* add skills */
			for (i = 0; i < 5; i++)
			{
    			if ((sn = skill_lookup(pc_race_table[ch->race].skills[i])) == -1)
					break;
				if (ch->pcdata->learned[sn] == 0)
					ch->pcdata->learned[sn] = 1;
			}
            // etc
            ch->size = pc_race_table[race].size;

			// Skip if the race is always neutral
			if (race == race_lookup("namek")) {
				ch->sex = SEX_NEUTRAL;
				ch->pcdata->true_sex = SEX_NEUTRAL;
                send_to_desc (szAlignData, d);
				d->connected = CON_GET_ALIGNMENT;
			}
			else {
                send_to_desc (szSexData, d);
				d->connected = CON_GET_NEW_SEX;
			}
            break;


        case CON_GET_NEW_SEX:
            switch (argument[0])
            {
                case '1':
                    ch->sex = SEX_MALE;
                    ch->pcdata->true_sex = SEX_MALE;
                    break;
                case '2':
                    ch->sex = SEX_FEMALE;
                    ch->pcdata->true_sex = SEX_FEMALE;
                    break;
                default:
                    send_to_desc ("That's not a sex.\n\r", d);
                    send_to_desc (szSexData, d);
                    return;
            }
            send_to_desc (szAlignData, d);
            d->connected = CON_GET_ALIGNMENT;
            break;

        case CON_GET_ALIGNMENT:
            switch (argument[0]) {
                case '1':
                    ch->alignment = 750;
                    break;
                case '2':
                    ch->alignment = 0;
                    break;
                case '3':
                    ch->alignment = -750;
                    break;
                default:
                    send_to_desc ("That's not a valid alignment.\n\r", d);
                    send_to_desc (szAlignData, d);
                    return;
            }

            write_to_buffer (d, "\n\r", 0);
            d->connected = CON_SET_STATS;
            SendStatData (d);
            break;

        case CON_SET_STATS:
            if (nValue == MAX_STATS + 2 && ch->pcdata->nGenStatPoints < 1) {
                enter_game (d);
                break;
            }
            else if (nValue == MAX_STATS+1) {
                for (i = 0; i < MAX_STATS; i++)
                    ch->perm_stat[i] = pc_race_table[ch->race].stats[i];
                ch->pcdata->nGenStatPoints = 10;
                send_to_desc ("Statistics reset.\n\r", d);
                SendStatData (d);
                return;
            }
            else if (ch->pcdata->nGenStatPoints > 0 && nValue > 0 && nValue <= MAX_STATS) {
                if (nValue == MAX_STATS) {//prevent charisma
                    send_to_desc ("{RThat's not an option!{x\n\r", d);
                } else {
                    ++ch->perm_stat[nValue-1];
                    --ch->pcdata->nGenStatPoints;
                }
            } else if (ch->pcdata->nGenStatPoints == 0){
                send_to_desc ("{YYou're out of points!{R Choose either reset, or done.{x\n\r", d);
            }
            else
                send_to_desc ("{YThat's not an option!{x\n\r", d);
            SendStatData (d);
			break;

        case CON_READ_IMOTD:
            write_to_buffer (d, "\n\r", 2);
            do_function (ch, &do_help, "motd");
            d->connected = CON_READ_MOTD;
            break;

		/* states for new note system, (c)1995-96 erwin@pip.dknet.dk */
		/* ch MUST be PC here; have nwrite check for PC status! */

		case CON_NOTE_TO:
			handle_con_note_to (d, argument);
			break;

		case CON_NOTE_SUBJECT:
			handle_con_note_subject (d, argument);
			break;

		case CON_NOTE_EXPIRE:
			handle_con_note_expire (d, argument);
			break;

		case CON_NOTE_TEXT:
			handle_con_note_text (d, argument);
			break;

		case CON_NOTE_FINISH:
			handle_con_note_finish (d, argument);
			break;

        case CON_FUSE_SLAVE:
            break;

        case CON_READ_MOTD:
            enter_game (d);
            break;
    }

    return;
}

void enter_game (DESCRIPTOR_DATA * d) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch;

    ch = d->character;
    if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0') {
        write_to_buffer (d, "Warning! Null password!\n\r", 0);
        write_to_buffer (d, "Please report old password with bug.\n\r", 0);
        write_to_buffer (d, "Type 'password null <new password>' to fix.\n\r", 0);
    }
    write_to_buffer (d, "\n\rWelcome ... to the Arena!\n\r", 0);
    ch->next = char_list;
    char_list = ch;
    d->connected = CON_PLAYING;

    if (ch->level == 0)
    {
        if (mud_ansicolor)
            SET_BIT (ch->act, PLR_COLOUR);
        if (mud_telnetga)
            SET_BIT (ch->comm, COMM_TELNET_GA);

        ch->act |= PLR_AUTOLOOT|PLR_AUTOEXIT|PLR_AUTOZENNI|PLR_AUTOSAC|PLR_AUTOSPLIT;
        ch->level = 1;
        ch->max_hit = get_curr_stat(ch, STAT_STR) * HP_STR + get_curr_stat(ch, STAT_WIL) * HP_WIL;
        ch->max_ki = 50 + get_curr_stat(ch, STAT_STR) * KI_STR + get_curr_stat(ch, STAT_WIL) * KI_WIL;
        ch->hit = ch->max_hit;
        ch->ki = ch->max_ki;
        ch->pcdata->perm_hit = ch->max_hit;
        ch->pcdata->perm_ki = ch->max_ki;
        ch->zenni = 250;
        sprintf (buf, "the %s", title_table[ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
        set_title (ch, buf);

        // New eq:
        //obj_to_char (create_object (get_obj_index (OBJ_VNUM_MAP), 0), ch);

        char_to_room (ch, get_room_index (ROOM_VNUM_SCHOOL));
        sendch ("\n\r", ch);
        do_function (ch, &do_help, "newbie info");
        sendch ("\n\r", ch);
    }
    else if (ch->in_room != NULL)
        char_to_room (ch, ch->in_room);
    else if (IS_IMMORTAL (ch))
        char_to_room (ch, get_room_index (ROOM_VNUM_CHAT));
    else
        char_to_room (ch, get_room_index (ROOM_VNUM_TEMPLE));

    ResetDiff (ch);
    reset_char (ch);

	act ("$n has entered the Arena.", ch, NULL, NULL, TO_ROOM);
    do_function (ch, &do_look, "auto");

    wiznet ("$N has left real life behind.", ch, NULL, WIZ_LOGINS, WIZ_SITES, ch->level);

    if (ch->pet != NULL) {
        char_to_room (ch->pet, ch->in_room);
        act ("$n has entered the Arena.", ch->pet, NULL, NULL, TO_ROOM);
    }

    sendch("\n", ch);
    do_function (ch, &do_board, "");
}
