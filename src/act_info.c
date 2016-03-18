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

/*   QuickMUD - The Lazy Man's ROM - $Id: act_info.c,v 1.3 2000/12/01 10:48:33 ring0 Exp $ */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

char *const where_name[] = {
    "<used as light>     ",
    "<worn on finger>    ",
    "<worn on finger>    ",
    "<worn around neck>  ",
    "<worn around neck>  ",
    "<worn on torso>     ",
    "<worn on head>      ",
    "<worn on legs>      ",
    "<worn on feet>      ",
    "<worn on hands>     ",
    "<worn on arms>      ",
    "<worn as shield>    ",
    "<worn about body>   ",
    "<worn about waist>  ",
    "<worn around wrist> ",
    "<worn around wrist> ",
    "<wielded>           ",
    "<held>              ",
    "<floating nearby>   ",
    "<worn on tail>      ",
    "<worn over eye>     ",
    "<worn on ear>       ",
    "<worn on ear>       ",
};


/* for  keeping track of the player count */
int max_on = 0;

/*
 * Local functions.
 */
char *format_obj_to_char args( ( OBJ_DATA * obj, CHAR_DATA * ch, bool fShort ) );
void show_list_to_char   args( ( OBJ_DATA * list, CHAR_DATA * ch, bool fShort, bool fShowNothing ) );
void show_char_to_char_0 args( ( CHAR_DATA * victim, CHAR_DATA * ch ) );
void show_char_to_char_1 args( ( CHAR_DATA * victim, CHAR_DATA * ch ) );
void show_char_to_char   args( ( CHAR_DATA * list, CHAR_DATA * ch ) );
bool check_blind         args( ( CHAR_DATA * ch ) );

char *format_pl (long long int powerlevel) {
	static char format[40];
	char pl[40];
	char *j;
	int m;

	sprintf (pl, "%Ld", powerlevel);
	j = &pl[0];
	m = 0;
	// Ugly formatting to put in commas
	while (*j != '\0') {
		format[m++] = *j;
		++j;
		if ((strlen(pl) - (int)(j - &pl[0])) % 3 == 0)
			format[m++] = ',';
	}
	format[m-1] = '\0';
	return format;
}


char *format_obj_to_char (OBJ_DATA * obj, CHAR_DATA * ch, bool fShort)
{
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
        || (obj->description == NULL || obj->description[0] == '\0'))
        return buf;

    if (IS_OBJ_STAT (obj, ITEM_INVIS))
        strcat (buf, "(Invis) ");
    if (IS_AFFECTED (ch, AFF_DETECT_EVIL) && IS_OBJ_STAT (obj, ITEM_EVIL))
        strcat (buf, "(Red Aura) ");
    if (IS_AFFECTED (ch, AFF_DETECT_GOOD) && IS_OBJ_STAT (obj, ITEM_BLESS))
        strcat (buf, "(Blue Aura) ");
    if (IS_AFFECTED (ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT (obj, ITEM_MAGIC))
        strcat (buf, "(Magical) ");
    if (IS_OBJ_STAT (obj, ITEM_GLOW))
        strcat (buf, "(Glowing) ");
    if (IS_OBJ_STAT (obj, ITEM_HUM))
        strcat (buf, "(Humming) ");
	if (!IS_NPC(ch) && obj->pIndexData->vnum == ch->pcdata->reward_obj) {
		if (ch->pcdata->hunt_time < current_time)
			ResetHunt(ch);
		else
			strcat (buf, "(Stolen Aura)");
	}

    if (fShort) 
    {
        if (obj->short_descr != NULL)
            strcat (buf, obj->short_descr);
    }
    else
    {
        if (obj->description != NULL)
            strcat (buf, obj->description);
    }

    return buf;
}



/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char (OBJ_DATA * list, CHAR_DATA * ch, bool fShort,
                        bool fShowNothing)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;

    if (ch->desc == NULL)
        return;

    /*
     * Alloc space for output lines.
     */
    output = new_buf ();

    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
        count++;
    prgpstrShow = alloc_mem (count * sizeof (char *));
    prgnShow = alloc_mem (count * sizeof (int));
    nShow = 0;

    /*
     * Format the list of objects.
     */
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == WEAR_NONE && can_see_obj (ch, obj))
        {
            pstrShow = format_obj_to_char (obj, ch, fShort);

            fCombine = FALSE;

            if (IS_NPC (ch) || IS_SET (ch->comm, COMM_COMBINE))
            {
                /*
                 * Look for duplicates, case sensitive.
                 * Matches tend to be near end so run loop backwords.
                 */
                for (iShow = nShow - 1; iShow >= 0; iShow--)
                {
                    if (!strcmp (prgpstrShow[iShow], pstrShow))
                    {
                        prgnShow[iShow]++;
                        fCombine = TRUE;
                        break;
                    }
                }
            }

            /*
             * Couldn't combine, or didn't want to.
             */
            if (!fCombine)
            {
                prgpstrShow[nShow] = str_dup (pstrShow);
                prgnShow[nShow] = 1;
                nShow++;
            }
        }
    }

    /*
     * Output the formatted list.
     */
    for (iShow = 0; iShow < nShow; iShow++)
    {
        if (prgpstrShow[iShow][0] == '\0')
        {
            free_string (prgpstrShow[iShow]);
            continue;
        }

        if (IS_NPC (ch) || IS_SET (ch->comm, COMM_COMBINE))
        {
            if (prgnShow[iShow] != 1)
            {
                sprintf (buf, "(%2d) ", prgnShow[iShow]);
                add_buf (output, buf);
            }
            else
            {
                add_buf (output, "     ");
            }
        }
        add_buf (output, prgpstrShow[iShow]);
        add_buf (output, "\n\r");
        free_string (prgpstrShow[iShow]);
    }

    if (fShowNothing && nShow == 0)
    {
        if (IS_NPC (ch) || IS_SET (ch->comm, COMM_COMBINE))
            sendch ("     ", ch);
        sendch ("Nothing.\n\r", ch);
    }
    page_to_char (buf_string (output), ch);

    /*
     * Clean up.
     */
    free_buf (output);
    free_mem (prgpstrShow, count * sizeof (char *));
    free_mem (prgnShow, count * sizeof (int));

    return;
}



void show_char_to_char_0 (CHAR_DATA * victim, CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH], message[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if (IS_SET (victim->comm, COMM_AFK))
        strcat (buf, "[AFK] ");
    if (IS_AFFECTED (victim, AFF_INVISIBLE))
        strcat (buf, "(Invis) ");
    if (victim->invis_level >= LEVEL_HERO)
        strcat (buf, "(Wizi) ");
    if (IS_AFFECTED (victim, AFF_HIDE))
        strcat (buf, "(Hide) ");
    if (IS_AFFECTED (victim, AFF_CHARM))
        strcat (buf, "(Charmed) ");
    if (IS_AFFECTED (victim, AFF_PASS_DOOR))
        strcat (buf, "({DTranslucent{x) ");
    if (IS_AFFECTED (victim, AFF_FAERIE_FIRE))
        strcat (buf, "({MPink Aura{x) ");
    if (IS_EVIL (victim) && IS_AFFECTED (ch, AFF_DETECT_EVIL))
        strcat (buf, "({RRed Aura{x) ");
    if (IS_GOOD (victim) && IS_AFFECTED (ch, AFF_DETECT_GOOD))
        strcat (buf, "({YGolden Aura{x) ");
    if (IS_AFFECTED (victim, AFF_SANCTUARY))
        strcat (buf, "({WWhite Aura{x) ");
    if (!IS_NPC (victim) && IS_SET (victim->act, PLR_HOSTILE))
        strcat (buf, "({RHostile{x) ");
    if (!IS_NPC (victim) && IS_SET (victim->act, PLR_THIEF))
        strcat (buf, "({CTHIEF{x) ");
    if (!IS_NPC (victim) && IS_SET (victim->act, PLR_QUEST))
        strcat (buf, "({GQuest{x) ");
    switch (GetTrans (victim)) {
        case TRANS_SSJ1:   strcat (buf, "({YGolden Hair{x) "); break;
        case TRANS_SSJ2:   strcat (buf, "({WArcs of Ki{x) "); break;
        case TRANS_SSJ3:   strcat (buf, "({YLong Golden Hair{x) "); break;
        case TRANS_SSJ4:   strcat (buf, "({RRed Fur{x) "); break;
        case TRANS_SSJ5:   strcat (buf, "({WP{CO{WW{CE{WR{x) "); break;
        case TRANS_HYPERN: strcat (buf, "({WWhite Aura{x) "); break;
        case TRANS_SUPERN: strcat (buf, "({GGreen Aura{x) "); break;
    }
    if (IS_SET (victim->affected_by, AFF_KAIOKEN))
        strcat (buf, "({RRed Haze{x) ");

    if (victim->position == victim->start_pos
        && victim->long_descr[0] != '\0') {
        strcat (buf, victim->long_descr);
		// the "- 2" is to remove the \n\r characters
		if (get_scouter(ch) != NULL)
			sprintf (buf + strlen(buf) - 2, " (%s)\n\r", get_pl_from_scouter(ch, victim));
		sendch (buf, ch);
        return;
    }

    strcat (buf, PERS (victim, ch));

    if (!IS_NPC (victim) && !IS_SET (ch->comm, COMM_BRIEF)
        && victim->position == POS_STANDING && ch->on == NULL) {
        if (victim->pcdata->pose)
            strcat (buf, victim->pcdata->pose);
        else
            strcat (buf, victim->pcdata->title);
    }

    switch (victim->position)
    {
        case POS_DEAD:
            strcat (buf, " is DEAD!!");
            break;
        case POS_UNCONSCIOUS:
            strcat (buf, " is unconscious.");
            break;
        case POS_MORTAL:
            strcat (buf, " is mortally wounded.");
            break;
        case POS_INCAP:
            strcat (buf, " is incapacitated.");
            break;
        case POS_STUNNED:
            strcat (buf, " is lying here stunned.");
            break;
        case POS_SLEEPING:
            if (victim->on != NULL)
            {
                if (IS_SET (victim->on->value[2], SLEEP_AT))
                {
                    sprintf (message, " is sleeping at %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
                else if (IS_SET (victim->on->value[2], SLEEP_ON))
                {
                    sprintf (message, " is sleeping on %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
                else
                {
                    sprintf (message, " is sleeping in %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
            }
            else
                strcat (buf, " is sleeping here.");
            break;
        case POS_RESTING:
            if (victim->on != NULL)
            {
                if (IS_SET (victim->on->value[2], REST_AT))
                {
                    sprintf (message, " is resting at %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
                else if (IS_SET (victim->on->value[2], REST_ON))
                {
                    sprintf (message, " is resting on %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
                else
                {
                    sprintf (message, " is resting in %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
            }
            else
                strcat (buf, " is resting here.");
            break;
        case POS_SITTING:
            if (victim->on != NULL)
            {
                if (IS_SET (victim->on->value[2], SIT_AT))
                {
                    sprintf (message, " is sitting at %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
                else if (IS_SET (victim->on->value[2], SIT_ON))
                {
                    sprintf (message, " is sitting on %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
                else
                {
                    sprintf (message, " is sitting in %s.",
                             victim->on->short_descr);
                    strcat (buf, message);
                }
            }
            else
                strcat (buf, " is sitting here.");
            break;
        case POS_STANDING:
            if (victim->on != NULL)
            {
                if (IS_SET (victim->on->value[2], STAND_AT))
                {
                    sprintf (message, " is standing at %s.", victim->on->short_descr);
                    strcat (buf, message);
                }
                else if (IS_SET (victim->on->value[2], STAND_ON))
                {
                    sprintf (message, " is standing on %s.", victim->on->short_descr);
                    strcat (buf, message);
                }
                else
                {
                    sprintf (message, " is standing in %s.", victim->on->short_descr);
                    strcat (buf, message);
                }
            }
            else if (victim->pcdata->pose)
                strcat (buf, ".");
            else
                strcat (buf, " is here.");
            break;
        case POS_FIGHTING:
            strcat (buf, " is here, fighting ");
            if (victim->fighting == NULL)
                strcat (buf, "thin air??");
            else if (victim->fighting == ch)
                strcat (buf, "YOU!");
            else if (victim->in_room == victim->fighting->in_room)
            {
                strcat (buf, PERS (victim->fighting, ch));
                strcat (buf, ".");
            }
            else
                strcat (buf, "someone who left??");
            break;
    }
	if (get_scouter(ch) != NULL)
		sprintf (buf + strlen(buf), " (%s)", get_pl_from_scouter(ch, victim));
	strcat (buf, "\n\r");
    buf[0] = UPPER (buf[0]);
    sendch (buf, ch);
    return;
}



void show_char_to_char_1 (CHAR_DATA * victim, CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int iWear;
    int percent;
    bool found;

    if (can_see (victim, ch))
    {
        if (ch == victim)
            act ("$n looks at $mself.", ch, NULL, NULL, TO_ROOM);
        else
        {
            act ("$n looks at you.", ch, NULL, victim, TO_VICT);
            act ("$n looks at $N.", ch, NULL, victim, TO_NOTVICT);
        }
    }

    if (victim->description[0] != '\0')
    {
        sendch (victim->description, ch);
    }
    else
    {
        act ("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);
    }

    if (victim->max_hit > 0)
        percent = (100 * (long long int)victim->hit) / (long long int)victim->max_hit;
    else
        percent = -1;

    strcpy (buf, PERS (victim, ch));

    if (percent >= 100)
        strcat (buf, " is in excellent condition.");
    else if (percent >= 90)
        strcat (buf, " has a few scratches.");
    else if (percent >= 75)
        strcat (buf, " has some small wounds and bruises.");
    else if (percent >= 50)
        strcat (buf, " has quite a few wounds.");
    else if (percent >= 30)
        strcat (buf, " has some big nasty wounds and scratches.");
    else if (percent >= 15)
        strcat (buf, " looks pretty hurt.");
    else if (percent >= 0)
        strcat (buf, " is in awful condition.");
    else
        strcat (buf, " is bleeding to death.");

    buf[0] = UPPER (buf[0]);
	if (get_scouter(ch) != NULL)
		sprintf (buf + strlen(buf), " (%s)\n\r", get_pl_from_scouter(ch, victim));
	else
		strcat (buf, "\n\r");
    sendch (buf, ch);

    found = FALSE;
    for (iWear = 0; iWear < MAX_WEAR; iWear++)
    {
        if ((obj = get_eq_char (victim, iWear)) != NULL
            && can_see_obj (ch, obj))
        {
            if (!found)
            {
                sendch ("\n\r", ch);
                act ("$N is using:", ch, NULL, victim, TO_CHAR);
                found = TRUE;
            }
            sendch (where_name[iWear], ch);
            sendch (format_obj_to_char (obj, ch, TRUE), ch);
            sendch ("\n\r", ch);
        }
    }

    if (victim != ch && !IS_NPC (ch)
		&&	// Check relative stats and skills
			number_range( (get_curr_stat(ch,STAT_DEX)+get_skill(ch,gsn_peek)*5)/2,
			            get_curr_stat(ch,STAT_DEX)+get_skill(ch,gsn_peek)*5     ) >
			number_range( (get_curr_stat(victim,STAT_DEX)+get_skill(victim,gsn_perception)*5)/2,
					    get_curr_stat(victim,STAT_DEX)+get_skill(victim,gsn_perception)*5)
		&& get_skill(ch,gsn_pick_lock) != 0)
    {
        sendch ("\n\rYou peek at the inventory:\n\r", ch);
        show_list_to_char (victim->carrying, ch, TRUE, TRUE);
    }

    return;
}



void show_char_to_char (CHAR_DATA * list, CHAR_DATA * ch)
{
    CHAR_DATA *rch;

    for (rch = list; rch != NULL; rch = rch->next_in_room)
    {
        if (rch == ch)
            continue;

        if (ch->level < rch->invis_level)
            continue;

        if (can_see (ch, rch))
        {
            show_char_to_char_0 (rch, ch);
        }
        else if (room_is_dark (ch->in_room)
                 && IS_AFFECTED (rch, AFF_INFRARED))
        {
            sendch ("You see glowing red eyes watching YOU!\n\r", ch);
        }
    }

    return;
}



bool check_blind (CHAR_DATA * ch)
{

    if (!IS_NPC (ch) && IS_SET (ch->act, PLR_HOLYLIGHT))
        return TRUE;

    if (IS_AFFECTED (ch, AFF_BLIND))
    {
        sendch ("You can't see a thing!\n\r", ch);
        return FALSE;
    }

    return TRUE;
}

/* changes your scroll */
void do_scroll (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        if (ch->lines == 0)
            sendch ("You do not page long messages.\n\r", ch);
        else
        {
            sprintf (buf, "You currently display %d lines per page.\n\r",
                     ch->lines + 2);
            sendch (buf, ch);
        }
        return;
    }

    if (!is_number (arg))
    {
        sendch ("You must provide a number.\n\r", ch);
        return;
    }

    lines = atoi (arg);

    if (lines == 0)
    {
        sendch ("Paging disabled.\n\r", ch);
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 100)
    {
        sendch ("You must provide a reasonable number.\n\r", ch);
        return;
    }

    sprintf (buf, "Scroll set to %d lines.\n\r", lines);
    sendch (buf, ch);
    ch->lines = lines - 2;
}

/* RT does socials */
void do_socials (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;

    col = 0;

    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
    {
        sprintf (buf, "%-12s", social_table[iSocial].name);
        sendch (buf, ch);
        if (++col % 6 == 0)
            sendch ("\n\r", ch);
    }

    if (col % 6 != 0)
        sendch ("\n\r", ch);
    return;
}



/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_help, "motd");
}

void do_imotd (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_help, "imotd");
}

void do_rules (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_help, "rules");
}

void do_story (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_help, "story");
}

void do_wizlist (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_help, "wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist (CHAR_DATA * ch, char *argument)
{
    /* lists most player flags */
    if (IS_NPC (ch))
        return;

    sendch ("   action     status\n\r", ch);
    sendch ("---------------------\n\r", ch);

    sendch ("autoassist     ", ch);
    if (IS_SET (ch->act, PLR_AUTOASSIST))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("autoexit       ", ch);
    if (IS_SET (ch->act, PLR_AUTOEXIT))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("autozenni      ", ch);
    if (IS_SET (ch->act, PLR_AUTOZENNI))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("autoloot       ", ch);
    if (IS_SET (ch->act, PLR_AUTOLOOT))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("autosac        ", ch);
    if (IS_SET (ch->act, PLR_AUTOSAC))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("autosplit      ", ch);
    if (IS_SET (ch->act, PLR_AUTOSPLIT))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("autoattack     ", ch);
    if (IS_SET (ch->act, PLR_AUTOATTACK))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);
    
    sendch ("autoweather    ",ch);
    if (IS_SET(ch->act,PLR_AUTOWEATHER))
        sendch ("{GON{x\n\r",ch);
    else
        sendch( "{ROFF{x\n\r",ch);

    sendch ("telnetga       ", ch);
    if (IS_SET (ch->comm, COMM_TELNET_GA))
	    sendch ("{GON{x\n\r", ch);
    else
	    sendch ("{ROFF{x\n\r",ch);

    sendch ("compact mode   ", ch);
    if (IS_SET (ch->comm, COMM_COMPACT))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("prompt         ", ch);
    if (IS_SET (ch->comm, COMM_PROMPT))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    sendch ("combine items  ", ch);
    if (IS_SET (ch->comm, COMM_COMBINE))
        sendch ("{GON{x\n\r", ch);
    else
        sendch ("{ROFF{x\n\r", ch);

    if (!IS_SET (ch->act, PLR_CANLOOT))
        sendch ("Your corpse is safe from thieves.\n\r", ch);
    else
        sendch ("Your corpse may be looted.\n\r", ch);

    if (IS_SET (ch->act, PLR_NOSUMMON))
        sendch ("You cannot be summoned.\n\r", ch);
    else
        sendch ("You can be summoned.\n\r", ch);

    if (IS_SET (ch->act, PLR_NOFOLLOW))
        sendch ("You do not welcome followers.\n\r", ch);
    else
        sendch ("You accept followers.\n\r", ch);
}

void do_autoassist (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_AUTOASSIST))
    {
        sendch ("Autoassist removed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_AUTOASSIST);
    }
    else
    {
        sendch ("You will now assist when needed.\n\r", ch);
        SET_BIT (ch->act, PLR_AUTOASSIST);
    }
}

void do_autoexit (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_AUTOEXIT))
    {
        sendch ("Exits will no longer be displayed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_AUTOEXIT);
    }
    else
    {
        sendch ("Exits will now be displayed.\n\r", ch);
        SET_BIT (ch->act, PLR_AUTOEXIT);
    }
}

void do_autozenni (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_AUTOZENNI))
    {
        sendch ("Autozenni removed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_AUTOZENNI);
    }
    else
    {
        sendch ("Automatic zenni looting set.\n\r", ch);
        SET_BIT (ch->act, PLR_AUTOZENNI);
    }
}

void do_autoloot (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_AUTOLOOT))
    {
        sendch ("Autolooting removed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_AUTOLOOT);
    }
    else
    {
        sendch ("Automatic corpse looting set.\n\r", ch);
        SET_BIT (ch->act, PLR_AUTOLOOT);
    }
}

void do_autosac (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_AUTOSAC))
    {
        sendch ("Autosacrificing removed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_AUTOSAC);
    }
    else
    {
        sendch ("Automatic corpse sacrificing set.\n\r", ch);
        SET_BIT (ch->act, PLR_AUTOSAC);
    }
}

void do_autosplit (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_AUTOSPLIT))
    {
        sendch ("Autosplitting removed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_AUTOSPLIT);
    }
    else
    {
        sendch ("Automatic zenni splitting set.\n\r", ch);
        SET_BIT (ch->act, PLR_AUTOSPLIT);
    }
}

void do_autoattack (CHAR_DATA *ch, char *argument) {
   if (IS_NPC (ch))
	return;
   
   sendch ("Autoattack has been removed due to misuse.\n\r", ch);
   REMOVE_BIT (ch->act, PLR_AUTOATTACK);
   return;

   if (IS_SET (ch->act, PLR_AUTOATTACK)) {
	sendch ("Autoattack removed.\n\r", ch);
	REMOVE_BIT (ch->act, PLR_AUTOATTACK);
   }
   else {
   	sendch ("Automatic attacking set.\n\r", ch);
   	SET_BIT (ch->act, PLR_AUTOATTACK);
	if (ch->fighting)
	    do_function (ch, &do_attack, "");
   }
}

void do_autoweather(CHAR_DATA *ch, char *argument) {
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->act,PLR_AUTOWEATHER)) {
        sendch ("You will no longer see weather descriptions in rooms\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOWEATHER);
    }
    else {
        sendch ("You will now see weather descriptions in rooms\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOWEATHER);
    }
}

void do_autoall (CHAR_DATA *ch, char * argument)
{
    if (IS_NPC(ch))
        return;

    if (!strcmp (argument, "on"))
    {
        SET_BIT(ch->act,PLR_AUTOASSIST);
        SET_BIT(ch->act,PLR_AUTOEXIT);
        SET_BIT(ch->act,PLR_AUTOZENNI);
        SET_BIT(ch->act,PLR_AUTOLOOT);
        SET_BIT(ch->act,PLR_AUTOSAC);
        SET_BIT(ch->act,PLR_AUTOSPLIT);
        SET_BIT(ch->act,PLR_AUTOWEATHER);

        sendch("All autos turned on.\n\r",ch);
    }
    else if (!strcmp (argument, "off"))
    {
        REMOVE_BIT (ch->act, PLR_AUTOASSIST);
        REMOVE_BIT (ch->act, PLR_AUTOEXIT);
        REMOVE_BIT (ch->act, PLR_AUTOZENNI);
        REMOVE_BIT (ch->act, PLR_AUTOLOOT);
        REMOVE_BIT (ch->act, PLR_AUTOSAC);
        REMOVE_BIT (ch->act, PLR_AUTOSPLIT);
        REMOVE_BIT (ch->act, PLR_AUTOWEATHER);

        sendch("All autos turned off.\n\r", ch);
    }
    else
        sendch("Usage: autoall [on|off]\n\r", ch);
}

void do_reveal (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->act, PLR_DONTREVEAL))
    {
        sendch ("Your power will be revealed.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_DONTREVEAL);
    }
    else
    {
        sendch ("Your power will no longer be revealed to the game, such as a position in the Top List.\n\r", ch);
        RemoveNameTopList (ch->name);
        SET_BIT (ch->act, PLR_DONTREVEAL);
    }
}

void do_brief (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_BRIEF))
    {
        sendch ("Full descriptions activated.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_BRIEF);
    }
    else
    {
        sendch ("Short descriptions activated.\n\r", ch);
        SET_BIT (ch->comm, COMM_BRIEF);
    }
}

void do_compact (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_COMPACT))
    {
        sendch ("Compact mode removed.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_COMPACT);
    }
    else
    {
        sendch ("Compact mode set.\n\r", ch);
        SET_BIT (ch->comm, COMM_COMPACT);
    }
}

void do_show (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_SHOW_AFFECTS))
    {
        sendch ("Affects will no longer be shown in score.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_SHOW_AFFECTS);
    }
    else
    {
        sendch ("Affects will now be shown in score.\n\r", ch);
        SET_BIT (ch->comm, COMM_SHOW_AFFECTS);
    }
}

void do_prompt (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_PROMPT))
        {
            sendch ("You will no longer see prompts.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_PROMPT);
        }
        else
        {
            sendch ("You will now see prompts.\n\r", ch);
            SET_BIT (ch->comm, COMM_PROMPT);
        }
        return;
    }

    if (!strcmp (argument, "all"))
        strcpy (buf, "{x[%p/%P]{cPL%c{x[%h]{cHEALTH {x[%k]{cKI{x> [%e] ");
    else
    {
        if (strlen (argument) > 100)
            argument[100] = '\0';
        strcpy (buf, argument);
        smash_tilde (buf);
        if (str_suffix ("%c", buf))
            strcat (buf, " ");

    }

    free_string (ch->prompt);
    ch->prompt = str_dup (buf);
    sprintf (buf, "Prompt set to %s\n\r", ch->prompt);
    sendch (buf, ch);
    return;
}

void do_combine (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_COMBINE))
    {
        sendch ("Long inventory selected.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_COMBINE);
    }
    else
    {
        sendch ("Combined inventory selected.\n\r", ch);
        SET_BIT (ch->comm, COMM_COMBINE);
    }
}

void do_noloot (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_CANLOOT))
    {
        sendch ("Your corpse is now safe from thieves.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_CANLOOT);
    }
    else
    {
        sendch ("Your corpse may now be looted.\n\r", ch);
        SET_BIT (ch->act, PLR_CANLOOT);
    }
}

void do_nofollow (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_NOFOLLOW))
    {
        sendch ("You now accept followers.\n\r", ch);
        REMOVE_BIT (ch->act, PLR_NOFOLLOW);
    }
    else
    {
        sendch ("You no longer accept followers.\n\r", ch);
        SET_BIT (ch->act, PLR_NOFOLLOW);
        die_follower (ch);
    }
}

void do_nosummon (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
    {
        if (IS_SET (ch->imm_flags, IMM_SUMMON))
        {
            sendch ("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT (ch->imm_flags, IMM_SUMMON);
        }
        else
        {
            sendch ("You are now immune to summoning.\n\r", ch);
            SET_BIT (ch->imm_flags, IMM_SUMMON);
        }
    }
    else
    {
        if (IS_SET (ch->act, PLR_NOSUMMON))
        {
            sendch ("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT (ch->act, PLR_NOSUMMON);
        }
        else
        {
            sendch ("You are now immune to summoning.\n\r", ch);
            SET_BIT (ch->act, PLR_NOSUMMON);
        }
    }
}

void do_look (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int number, count;

    if (ch->desc == NULL)
        return;

    if (ch->position < POS_SLEEPING)
    {
        sendch ("You can't see anything but stars!\n\r", ch);
        return;
    }

    if (ch->position == POS_SLEEPING)
    {
        sendch ("You can't see anything, you're sleeping!\n\r", ch);
        return;
    }

    if (!check_blind (ch))
        return;

    if (!IS_NPC (ch)
        && !IS_SET (ch->act, PLR_HOLYLIGHT) && room_is_dark (ch->in_room))
    {
        sendch ("It is pitch black ... \n\r", ch);
        show_char_to_char (ch->in_room->people, ch);
        return;
    }

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    number = number_argument (arg1, arg3);
    count = 0;

    if (arg1[0] == '\0' || !str_cmp (arg1, "auto"))
    {
        /* 'look' or 'look auto' */
        sendch ("{c", ch);
        sendch (ch->in_room->name, ch);
        sendch ("{x", ch);

        if ((IS_IMMORTAL (ch)
             && (IS_NPC (ch) || IS_SET (ch->act, PLR_HOLYLIGHT)))
            || IS_BUILDER (ch, ch->in_room->area))
        {
            sprintf (buf, "{r [{RRoom %d{r]{x", ch->in_room->vnum);
            sendch (buf, ch);
        }

        sendch ("\n\r", ch);

        if (IS_SET(ch->act, PLR_AUTOWEATHER) && IsOutdoors(ch))
            show_weather(ch);

        if (arg1[0] == '\0'
            || (!IS_NPC (ch) && !IS_SET (ch->comm, COMM_BRIEF)))
        {
            sendch ("  ", ch);
            sendch ("{W", ch);
            sendch (ch->in_room->description, ch);
            sendch ("{x", ch);
        }

        if (!IS_NPC (ch) && IS_SET (ch->act, PLR_AUTOEXIT))
        {
            sendch ("\n\r", ch);
            do_function (ch, &do_exits, "auto");
        }

        show_list_to_char (ch->in_room->contents, ch, FALSE, FALSE);
        show_char_to_char (ch->in_room->people, ch);
        return;
    }

    if (!str_cmp (arg1, "i") || !str_cmp (arg1, "in")
        || !str_cmp (arg1, "on"))
    {
        /* 'look in' */
        if (arg2[0] == '\0')
        {
            sendch ("Look in what?\n\r", ch);
            return;
        }

        if ((obj = get_obj_here (ch, NULL, arg2)) == NULL)
        {
            sendch ("You do not see that here.\n\r", ch);
            return;
        }

        switch (obj->item_type)
        {
            default:
                sendch ("That is not a container.\n\r", ch);
                break;

            case ITEM_DRINK_CON:
                if (obj->value[1] <= 0)
                {
                    sendch ("It is empty.\n\r", ch);
                    break;
                }

                sprintf (buf, "It's %sfilled with  a %s liquid.\n\r",
                         obj->value[1] < obj->value[0] / 4
                         ? "less than half-" :
                         obj->value[1] < 3 * obj->value[0] / 4
                         ? "about half-" : "more than half-",
                         liq_table[obj->value[2]].liq_color);

                sendch (buf, ch);
                break;

            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                if (IS_SET (obj->value[1], CONT_CLOSED))
                {
                    sendch ("It is closed.\n\r", ch);
                    break;
                }

                act ("$p holds:", ch, obj, NULL, TO_CHAR);
                show_list_to_char (obj->contains, ch, TRUE, TRUE);
                break;
        }
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg1)) != NULL)
    {
        show_char_to_char_1 (victim, ch);
        return;
    }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (can_see_obj (ch, obj))
        {                        /* player can see object */
            pdesc = get_extra_descr (arg3, obj->extra_descr);
            if (pdesc != NULL)
            {
                if (++count == number)
                {
                    sendch (pdesc, ch);
                    return;
                }
                else
                    continue;
            }

            pdesc = get_extra_descr (arg3, obj->pIndexData->extra_descr);
            if (pdesc != NULL)
            {
                if (++count == number)
                {
                    sendch (pdesc, ch);
                    return;
                }
                else
                    continue;
            }

            if (is_name (arg3, obj->name))
                if (++count == number)
                {
                    sendch (obj->description, ch);
                    sendch ("\n\r", ch);
                    return;
                }
        }
    }

    for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
    {
        if (can_see_obj (ch, obj))
        {
            pdesc = get_extra_descr (arg3, obj->extra_descr);
            if (pdesc != NULL)
                if (++count == number)
                {
                    sendch (pdesc, ch);
                    return;
                }

            pdesc = get_extra_descr (arg3, obj->pIndexData->extra_descr);
            if (pdesc != NULL)
                if (++count == number)
                {
                    sendch (pdesc, ch);
                    return;
                }

            if (is_name (arg3, obj->name))
                if (++count == number)
                {
                    sendch (obj->description, ch);
                    sendch ("\n\r", ch);
                    return;
                }
        }
    }

    pdesc = get_extra_descr (arg3, ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
        if (++count == number)
        {
            sendch (pdesc, ch);
            return;
        }
    }

    if (count > 0 && count != number)
    {
        if (count == 1)
            sprintf (buf, "You only see one %s here.\n\r", arg3);
        else
            sprintf (buf, "You only see %d of those here.\n\r", count);

        sendch (buf, ch);
        return;
    }

    if (!str_cmp (arg1, "n") || !str_cmp (arg1, "north"))
        door = 0;
    else if (!str_cmp (arg1, "e") || !str_cmp (arg1, "east"))
        door = 1;
    else if (!str_cmp (arg1, "s") || !str_cmp (arg1, "south"))
        door = 2;
    else if (!str_cmp (arg1, "w") || !str_cmp (arg1, "west"))
        door = 3;
    else if (!str_cmp (arg1, "u") || !str_cmp (arg1, "up"))
        door = 4;
    else if (!str_cmp (arg1, "d") || !str_cmp (arg1, "down"))
        door = 5;
    else
    {
        sendch ("You do not see that here.\n\r", ch);
        return;
    }

    /* 'look direction' */
    if ((pexit = ch->in_room->exit[door]) == NULL)
    {
        sendch ("Nothing special there.\n\r", ch);
        return;
    }

    if (pexit->description != NULL && pexit->description[0] != '\0')
        sendch (pexit->description, ch);
    else
        sendch ("Nothing special there.\n\r", ch);

    if (pexit->keyword != NULL
        && pexit->keyword[0] != '\0' && pexit->keyword[0] != ' ')
    {
        if (IS_SET (pexit->exit_info, EX_CLOSED))
        {
            act ("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
        }
        else if (IS_SET (pexit->exit_info, EX_ISDOOR))
        {
            act ("The $d is open.", ch, NULL, pexit->keyword, TO_CHAR);
        }
    }

    return;
}

/* RT added back for the hell of it */
void do_read (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_look, argument);
}

void do_examine (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Examine what?\n\r", ch);
        return;
    }

    do_function (ch, &do_look, arg);

    if ((obj = get_obj_here (ch, NULL, arg)) != NULL)
    {
        switch (obj->item_type)
        {
            default:
                break;

            case ITEM_MONEY:
                sprintf (buf,
					 "There are %d zenni in the pile.\n\r", obj->value[0]);
                sendch (buf, ch);
                break;

            case ITEM_DRINK_CON:
            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                sprintf (buf, "in %s", argument);
                do_function (ch, &do_look, buf);
        }
    }

    return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits (CHAR_DATA * ch, char *argument)
{
    extern char *const dir_name[];
    char buf[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;

    fAuto = !str_cmp (argument, "auto");

    if (!check_blind (ch))
        return;

    if (fAuto)
        sprintf (buf, "{g[Exits:{G");
    else if (IS_IMMORTAL (ch))
        sprintf (buf, "Exits from room %d:\n\r", ch->in_room->vnum);
    else
        sprintf (buf, "Exits:\n\r");

    found = FALSE;
    for (door = 0; door <= 5; door++)
    {
        if ((pexit = ch->in_room->exit[door]) != NULL
            && pexit->u1.to_room != NULL
            && can_see_room (ch, pexit->u1.to_room))
        {
            found = TRUE;
            if (fAuto)
            {
                strcat (buf, " ");
				if (IS_SET (pexit->exit_info, EX_CLOSED))
					strcat (buf, "(");
                strcat (buf, dir_name[door]);
				if (IS_SET (pexit->exit_info, EX_CLOSED))
					strcat (buf, ")");
            }
            else
            {
                sprintf (buf + strlen (buf), "%-5s - %s",
                         capitalize (dir_name[door]),
                         room_is_dark (pexit->u1.to_room)
                         ? "Too dark to tell" : pexit->u1.to_room->name);
                if (IS_SET (pexit->exit_info, EX_CLOSED))
					strcat (buf, "(closed)");
                if (IS_IMMORTAL (ch))
                    sprintf (buf + strlen (buf),
                             " (room %d)", pexit->u1.to_room->vnum);
                strcat (buf, "\n\r");
            }
        }
    }

    if (!found)
        strcat (buf, fAuto ? " none" : "None.\n\r");

    if (fAuto)
        strcat (buf, "]{x\n\r");

    sendch (buf, ch);
    return;
}

void do_worth (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    sprintf (buf, "You have %ld zenni.\n\r", ch->zenni);
    sendch (buf, ch);
    return;
}


void do_score( CHAR_DATA *ch, char *argument ) {
    char buf[MAX_STRING_LENGTH];
    // extra bufs for some formatting
   char buf1[MAX_STRING_LENGTH],
        buf2[MAX_STRING_LENGTH];
      //buf3[MAX_STRING_LENGTH];

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf( buf,"{cName :{W %-15s {cSex   :{W %-14s {cRace :{W %-15s\n\r",
             ch->name, ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female", race_table[ch->race].name);
    sendch( buf, ch );
    sprintf( buf,"{cAge  :{W %-15d {cHours :{W %-13d\n\r",
             get_age(ch), (( ch->played + (int) (current_time - ch->logon)) / 3600));
    sendch( buf, ch );

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    if (GetTrans(ch) != TRANS_NONE) {
        sendch("{cYou are transformed into a ", ch);
        sendch("{R", ch);
        sendch(trans_table[GetTrans(ch)], ch);
        sendch("{c\n\r", ch);
    }

    if (IS_FUSED(ch)) {
        sprintf (buf, "{cYou are fused for {W%d{c hours.\n\r", ch->fuse_count);
        sendch (buf, ch);
    }

    sprintf( buf, "{cPlvl :{W %Ld/%Ld (%d%%)\n\r", ch->nCurPl * ch->llPl / 100,ch->llPl, ch->nCurPl);
    sendch( buf, ch );
    if (ch->llSuppressPl > -1)
        printf_to_char (ch, "{cSuppressed Plvl: {W%Ld\n\r", ch->llSuppressPl);
    sprintf( buf, "{cHps  :{W %d/%d\n\r", ch->hit, ch->max_hit );
    sendch( buf, ch );
    sprintf( buf, "{cKi   :{W %d/%d\n\r", ch->ki, ch->max_ki );
    sendch( buf, ch );

    sprintf( buf, "{cHitroll:{W %-13d  {cDamroll:{W %-15d\n\r",
	     GET_HITROLL(ch), GET_DAMROLL(ch) );
    sendch( buf, ch );

    sprintf( buf,"{cStr:{W %d(%d)  {cInt:{W %d(%d)  {cWil:{W %d(%d)  {cDex:{W %d(%d)  {cCha:{W %d(%d)\n\r",
	    ch->perm_stat[STAT_STR], get_curr_stat(ch,STAT_STR),
	    ch->perm_stat[STAT_INT], get_curr_stat(ch,STAT_INT),
	    ch->perm_stat[STAT_WIL], get_curr_stat(ch,STAT_WIL),
	    ch->perm_stat[STAT_DEX], get_curr_stat(ch,STAT_DEX),
	    ch->perm_stat[STAT_CHA], get_curr_stat(ch,STAT_CHA) );
    sendch( buf, ch );

    sprintf( buf,"{cArmor: {Wpierce: %d  bash: %d  slash: %d  ki: %d\n\r",
	     GET_AC(ch,AC_PIERCE),
             GET_AC(ch,AC_BASH),
	     GET_AC(ch,AC_SLASH),
	     GET_AC(ch,AC_EXOTIC));
    sendch(buf,ch);

    sprintf (buf, "{cYou are in a {W%s {cstance.\n\r", stance_table[ch->stance]);
    sendch (buf, ch);

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf( buf, "{cAlignment:{W %d  ", ch->alignment );
    sendch( buf, ch );
    sendch( "{cYou are ", ch );
         if ( ch->alignment >  900 ) sendch( "{Wangelic.{x\n\r", ch );
    else if ( ch->alignment >  700 ) sendch( "{Wsaintly.{x\n\r", ch );
    else if ( ch->alignment >  350 ) sendch( "{wgood.{x\n\r",    ch );
    else if ( ch->alignment >  100 ) sendch( "{wkind.{x\n\r",    ch );
    else if ( ch->alignment > -100 ) sendch( "{cneutral.{x\n\r", ch );
    else if ( ch->alignment > -350 ) sendch( "{wmean.{x\n\r",    ch );
    else if ( ch->alignment > -700 ) sendch( "{bevil.{x\n\r",    ch );
    else if ( ch->alignment > -900 ) sendch( "{rdemonic.{x\n\r", ch );
    else                             sendch( "{rsatanic.{x\n\r", ch );
        
    sprintf( buf,"{cZenni :{W %-15ld\n\r", ch->zenni );
    sendch( buf, ch );

    sprintf( buf1, "{cItems:{W %d/%d", ch->carry_number, can_carry_n(ch) );
	sprintf( buf2, "{cWeight:{W %ld/%d", get_carry_weight(ch) / 10, can_carry_w(ch) / 10 );
	sprintf( buf, "%-27s%-27s\n\r", buf1, buf2 );
    sendch( buf, ch );

    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);

    sprintf( buf,"{cWimpy:{W %d hps\n\r", ch->wimpy );
    sendch( buf, ch );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]   > 10 )
	sendch( "{cYou are drunk.\n\r",   ch );

    /* RT wizinvis and holy light */
    if ( IS_IMMORTAL(ch)) {
        sprintf(buf, "{cHoly Light:{W %-10s ", IS_SET(ch->act,PLR_HOLYLIGHT) ? "on" : "off");
        sendch(buf,ch);

        sprintf(buf, "{cCombat Info:{W %-10s ", IS_SET(ch->act,PLR_COMBATINFO) ? "on" : "off");
        sendch(buf, ch);

        if (ch->invis_level) {
            sprintf( buf, "{c  Invisible:{W level %d",ch->invis_level);
            sendch(buf,ch);
        }

        if (ch->incog_level) {
	        sprintf(buf," {c Incognito:{W level %d",ch->incog_level);
	        sendch(buf,ch);
        }
        sendch("\n\r",ch);
    }

    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS)) {
        sendch("{Y-----------------------------------------------------------------{x\n\r",ch);
        do_affects(ch,"");
    }
    sendch("{Y-----------------------------------------------------------------{x\n\r",ch);
}

void do_affects (CHAR_DATA * ch, char *argument)
{
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];

    if (GetTrans(ch) != TRANS_NONE) {
        sendch("{cYou are transformed into a ", ch);
        sendch("{R", ch);
        sendch(trans_table[GetTrans(ch)], ch);
        sendch("{c\n\r", ch);
    }
    if (IS_FUSED(ch)) {
        sprintf (buf, "{cYou are fused for {W%d {chours.\n\r", ch->fuse_count);
        sendch (buf, ch);
    }
    if (ch->affected)
    {
        sendch ("{cYou are affected by the following:\n\r", ch);
        for (paf = ch->affected; paf != NULL; paf = paf->next) {
            if (paf->bitvector == AFF_NONE && paf->location == APPLY_NONE)
				continue;

			if (paf_last != NULL && paf->type == paf_last->type)
                sprintf (buf, "               : ");
            else
                sprintf (buf, "{w%-15s{c: ", skill_table[paf->type].name);
            sendch (buf, ch);

			if (paf->location != APPLY_NONE) {
				sprintf (buf,
						 "modifies {W%s {cby {W%Ld {c",
						 affect_loc_name (paf->location), paf->modifier);
				sendch (buf, ch);
			}
            
            if (paf->bitvector != AFF_NONE)
				printf_to_char (ch, "grants {W%s {c", affect_bit_name (paf->bitvector));

			if (paf->duration == -1)
                sprintf (buf, "{Wpermanently{c");
            else
                sprintf (buf, "{cfor {W%d {chours{x", paf->duration);
            sendch (buf, ch);

            sendch ("\n\r", ch);
            paf_last = paf;
        }
    }
    else
        sendch ("{cYou are not affected by anything.{w\n\r", ch);

    return;
}



char *const day_name[] = {
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

char *const month_name[] = {
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
    "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
    "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time (CHAR_DATA * ch, char *argument)
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char *suf;
    int day;

    day = time_info.day + 1;

    if (day > 4 && day < 20)
        suf = "th";
    else if (day % 10 == 1)
        suf = "st";
    else if (day % 10 == 2)
        suf = "nd";
    else if (day % 10 == 3)
        suf = "rd";
    else
        suf = "th";

    sprintf (buf,
             "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r",
             (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
             time_info.hour >= 12 ? "pm" : "am",
             day_name[day % 7], day, suf, month_name[time_info.month]);
    sendch (buf, ch);
    sprintf (buf, "ROM started up at %s\n\rThe system time is %s.\n\r",
             str_boot_time, (char *) ctime (&current_time));

    sendch (buf, ch);
    return;
}



void do_help (CHAR_DATA * ch, char *argument)
{
    HELP_DATA *pHelp;
    BUFFER *output;
    bool found = FALSE;
    char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH];
    int level;

    output = new_buf ();

    if (argument[0] == '\0')
        argument = "summary";

    /* this parts handle "help a b" so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0')
    {
        argument = one_argument (argument, argone);
        if (argall[0] != '\0')
            strcat (argall, " ");
        strcat (argall, argone);
    }

    for (pHelp = help_first; pHelp != NULL; pHelp = pHelp->next)
    {
        level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;

        if (level > ch->level)
            continue;

        if (is_name (argall, pHelp->keyword))
        {
            /* add seperator if found */
            if (found)
                add_buf (output,
                         "\n\r{Y============================================================{x\n\r\n\r");
            if (pHelp->level >= 0 && str_cmp (argall, "imotd"))
            {
                add_buf (output, "{c");
                add_buf (output, pHelp->keyword);
                add_buf (output, "{x\n\r");
            }

            /*
             * Strip leading '.' to allow initial blanks.
             */
            if (pHelp->text[0] == '.')
                add_buf (output, pHelp->text + 1);
            else
                add_buf (output, pHelp->text);
            found = TRUE;
            /* small hack :) */
            if (ch->desc != NULL && ch->desc->connected != CON_PLAYING
                && ch->desc->connected != CON_GEN_GROUPS)
                break;
        }
    }

    if (!found)
	{
        sendch ("No help on that word.\n\r", ch);
		/*
		 * Let's log unmet help requests so studious IMP's can improve their help files ;-)
		 * But to avoid idiots, we will check the length of the help request, and trim to
		 * a reasonable length (set it by redefining MAX_CMD_LEN in merc.h).  -- JR
		 */
		if (strlen(argall) > MAX_CMD_LEN)
		{
			argall[MAX_CMD_LEN - 1] = '\0';
			logfile ("Excessive command length: %s requested %s.", ch, argall);
			sendch ("That was rude!\n\r", ch);
		}
		/* OHELPS_FILE is the "orphaned helps" files. Defined in merc.h -- JR */
		else
		{
			append_file (ch, OHELPS_FILE, argall);
		}
	}
    else
        page_to_char (buf_string (output), ch);
    free_buf (output);
}

void do_who( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int half, sechalf;
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int iRace;
    int iClan;
    int nMatch;
    int i;
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[MAX_CLAN];
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
    bool imm_count = 0;
    bool mortal_count = 0;

    /*
     * Set default arguments.
     */
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = FALSE;
    for (iClan = 0; iClan < MAX_CLAN; iClan++)
		rgfClan[iClan] = FALSE;

    /*
     * Parse arguments.
     */
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];

        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;

        /*
         * Look for classes to turn on.
         */
        if (!str_prefix(arg,"immortals"))
            fImmortalOnly = TRUE;
        else {
            iRace = race_lookup(arg);

            if (iRace == 0 || iRace >= MAX_PC_RACE)	{
				if (!str_prefix(arg,"clan"))
					fClan = TRUE;
				else {
					iClan = clan_lookup(arg);
					if (iClan) {
						fClanRestrict = TRUE;
			   			rgfClan[iClan] = TRUE;
					}
					else {
						sendch("That's not a valid race, class, or clan.\n\r",ch);
						return;
					}
				}
			}
			else {
				fRaceRestrict = TRUE;
				rgfRace[iRace] = TRUE;
			}
        }
    }

    // Get a primary count of the players
    for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch,wch))
			continue;

		if ( (fImmortalOnly  && wch->level < LEVEL_IMMORTAL)
		  || (fRaceRestrict && !rgfRace[wch->race])
		  || (fClan && !is_clan(wch))
		  || (fClanRestrict && !rgfClan[wch->clan]))
			continue;

        if (wch->level < LEVEL_IMMORTAL)
            ++mortal_count;
        else
            ++imm_count;
    }


    /*
     * Now show matching chars.
     */
    nMatch = 0;
	buf[0] = '\0';
	output = new_buf();
	add_buf(output, "\n\r");

    // First the imms
    if (imm_count > 0) {
        add_buf (output, "        {WIMMORTALS{x\n\r");
        add_buf (output, "        {B========={x\n\r");
        for (i = MAX_LEVEL; i >= LEVEL_IMMORTAL; --i) {
            for (d = descriptor_list; d != NULL; d = d->next) {
                CHAR_DATA *wch;

                if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
                    continue;

                wch = (d->original != NULL) ? d->original : d->character;

                if (wch->level != i)
                    continue;

                if (!can_see(ch,wch))
                    continue;

                if ( (fRaceRestrict && !rgfRace[wch->race])
                    || (fClan && !is_clan(wch))
                    || (fClanRestrict && !rgfClan[wch->clan])
                    || wch->level < LEVEL_IMMORTAL)
                    continue;

                nMatch++;

                if (wch->level > HERO && wch->immtitle != NULL) {
                    if (colorstrlen(wch->immtitle) == 12)
                        sprintf(buf, "  {B[%s{B]{x", wch->immtitle);
                    else if (colorstrlen(wch->immtitle) == 11)
                        sprintf(buf, "  {B[%s {B]{x", wch->immtitle);
                    else {
                        half = ((12 - colorstrlen(wch->immtitle)) / 2);
                        sechalf = (12 - (half + colorstrlen(wch->immtitle)));
                        sprintf(buf, "  {B[%*c%s%*c{B]{x", half, ' ', wch->immtitle, sechalf,' ');
                    }
                }
                else {
                    if (wch->level < MAX_LEVEL)
                        sprintf(buf, "  {B[{c %11.11s{B]{x",
                            wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "        ");
                    else if (wch->immtitle == NULL)
                        sprintf(buf, "  {B[{c %11.11s{B]{x",
                            wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ");
                }
                add_buf(output,buf);
                /*
                * Format it up.
                */
                sprintf( buf, " %s%s%s%s%s%s%s%s%s%s",
                    wch->incog_level >= LEVEL_HERO ? "(Incog) " : "",
                    wch->invis_level >= LEVEL_HERO ? "{w({bW{Biz{bi{w){x " : "",
                    clan_table[wch->clan].who_name,
                    IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
                    IS_SET(wch->act, PLR_HOSTILE) ? "({rHostile{x) " : "",
                    IS_SET(wch->act, PLR_THIEF)  ? "(THIEF) "  : "",
                    IS_SET(wch->act, PLR_QUEST) ? "({GQuest{x) " : "",
                    ch->level >= IMMORTAL ? (wch->pcdata->nReward > 0 ? "({RReward{x) " : "") : "",
                    wch->name,
                    IS_NPC(wch) ? "" : wch->pcdata->title );
                if (get_scouter(ch) != NULL || get_skill(ch, gsn_sense) > 0)
                    sprintf (buf + strlen(buf), " (%s)", get_pl_from_scouter(ch, wch));
                strcat (buf, "\n\r");
                add_buf(output,buf);
            }
        }
    }

    // Now the players
    if (mortal_count > 0 && !fImmortalOnly) {
		int nLowerDiff, nDiff;
		CHAR_DATA *pMaxCh, *wch, *pch;

        add_buf (output, "\n\r         {WMORTALS{x\n\r");
        add_buf (output, "         {B======={x\n\r");
		nLowerDiff = -1;
        while (TRUE) {
			pMaxCh = NULL;
			for (d = descriptor_list; d != NULL; d = d->next) {
				if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
					continue;

				wch = (d->original != NULL) ? d->original : d->character;

				if (!can_see(ch,wch))
					continue;

				if ( (fRaceRestrict && !rgfRace[wch->race])
					 || (fClan && !is_clan(wch))
					 || (fClanRestrict && !rgfClan[wch->clan])
					 || wch->level >= LEVEL_IMMORTAL)
					continue;

                    // Find the proper difficulty while factoring in suppression
                    if (wch->llSuppressPl > -1 && get_skill(wch, gsn_suppress) > get_skill(ch, gsn_sense))
                        nDiff = sqrt(wch->llSuppressPl); // pl = difficulty^2
                    else
                        nDiff = wch->nDifficulty;

				if ((nDiff < nLowerDiff || nLowerDiff == -1)
                    && (pMaxCh == NULL || nDiff > pMaxCh->nDifficulty))
					pMaxCh = wch;
			}
			if (pMaxCh == NULL)
				break;

            if (pMaxCh->llSuppressPl > -1 && get_skill(pMaxCh, gsn_suppress) > get_skill(ch, gsn_sense))
                nLowerDiff = sqrt(pMaxCh->llSuppressPl); // pl = difficulty^2
            else
                nLowerDiff = pMaxCh->nDifficulty;

		    // Print each character with this difficulty
            for (d = descriptor_list; d != NULL; d = d->next) {
				if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
					continue;

				pch = (d->original != NULL) ? d->original : d->character;

				if (!can_see(ch,pch))
					continue;

				if ( (fRaceRestrict && !rgfRace[pch->race])
					 || (fClan && !is_clan(pch))
					 || (fClanRestrict && !rgfClan[pch->clan])
					 || pch->level >= LEVEL_IMMORTAL)
					continue;

                if (pch->llSuppressPl > -1 && get_skill(pch, gsn_suppress) > get_skill(ch, gsn_sense))
                    nDiff = sqrt(pch->llSuppressPl); // pl = difficulty^2
                else
                    nDiff = pch->nDifficulty;

                if (nDiff == nLowerDiff) {
                    nMatch++;

                    sprintf(buf, "  {B[{c %11.11s{B]{x", pch->race < MAX_PC_RACE ? pc_race_table[pch->race].who_name : "        ");
                    add_buf(output,buf);
                    /*
                    * Format it up.
                    */
                    sprintf( buf, " %s%s%s%s%s%s%s%s",
                            clan_table[pch->clan].who_name,
                            IS_SET(pch->comm, COMM_AFK) ? "[AFK] " : "",
                            IS_SET(pch->act, PLR_HOSTILE) ? "({rHostile{x) " : "",
                            IS_SET(pch->act, PLR_THIEF)  ? "(THIEF) "  : "",
                            IS_SET(pch->act, PLR_QUEST) ? "({GQuest{x) " : "",
                            ch->level >= IMMORTAL ? (pch->pcdata->nReward > 0 ? "({RReward{x) " : "") : "",
                            pch->name,
                            IS_NPC(pch) ? "" : pch->pcdata->title );
                    if (get_scouter(ch) != NULL || get_skill(ch, gsn_sense) > 0)
                        sprintf (buf + strlen(buf), " (%s)", get_pl_from_scouter(ch, pch));
                    strcat (buf, "\n\r");
                    add_buf(output,buf);
                }
            }
        }
    }
	sprintf( buf2, "\n\r     Players found: %d\n\r", nMatch );
	add_buf(output,buf2);
	page_to_char( buf_string(output), ch );
	free_buf(output);
	return;
}


void do_count (CHAR_DATA * ch, char *argument)
{
    int count;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    count = 0;

    for (d = descriptor_list; d != NULL; d = d->next)
        if (d->connected == CON_PLAYING && can_see (ch, d->character))
            count++;

    max_on = UMAX (count, max_on);

    if (max_on == count)
        sprintf (buf,
                 "There are %d characters on, the most so far today.\n\r",
                 count);
    else
        sprintf (buf,
                 "There are %d characters on, the most on today was %d.\n\r",
                 count, max_on);

    sendch (buf, ch);
}

void do_inventory (CHAR_DATA * ch, char *argument)
{
    sendch ("You are carrying:\n\r", ch);
    show_list_to_char (ch->carrying, ch, TRUE, TRUE);
    return;
}



void do_equipment (CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj;
    int iWear;
    bool found;

    sendch ("You are using:\n\r", ch);
    found = FALSE;
    for (iWear = 0; iWear < MAX_WEAR; iWear++)
    {
        if ((obj = get_eq_char (ch, iWear)) == NULL)
            continue;

        sendch (where_name[iWear], ch);
        if (can_see_obj (ch, obj))
        {
            sendch (format_obj_to_char (obj, ch, TRUE), ch);
            sendch ("\n\r", ch);
        }
        else
        {
            sendch ("something.\n\r", ch);
        }
        found = TRUE;
    }

    if (!found)
        sendch ("Nothing.\n\r", ch);

    return;
}



void do_compare (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    if (arg1[0] == '\0')
    {
        sendch ("Compare what to what?\n\r", ch);
        return;
    }

    if ((obj1 = get_obj_carry (ch, arg1, ch)) == NULL)
    {
        sendch ("You do not have that item.\n\r", ch);
        return;
    }

    if (arg2[0] == '\0')
    {
        for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
        {
            if (obj2->wear_loc != WEAR_NONE && can_see_obj (ch, obj2)
                && obj1->item_type == obj2->item_type
                && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
                break;
        }

        if (obj2 == NULL)
        {
            sendch ("You aren't wearing anything comparable.\n\r", ch);
            return;
        }
    }

    else if ((obj2 = get_obj_carry (ch, arg2, ch)) == NULL)
    {
        sendch ("You do not have that item.\n\r", ch);
        return;
    }

    msg = NULL;
    value1 = 0;
    value2 = 0;

    if (obj1 == obj2)
    {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if (obj1->item_type != obj2->item_type)
    {
        msg = "You can't compare $p and $P.";
    }
    else
    {
        switch (obj1->item_type)
        {
            default:
                msg = "You can't compare $p and $P.";
                break;

            case ITEM_ARMOR:
                value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
                value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
                break;

            case ITEM_WEAPON:
                value1 = (1 + obj1->value[2]) * obj1->value[1];
                value2 = (1 + obj2->value[2]) * obj2->value[1];
                break;
        }
    }

    if (msg == NULL)
    {
        if (value1 == value2)
            msg = "$p and $P look about the same.";
        else if (value1 > value2)
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act (msg, ch, obj1, obj2, TO_CHAR);
    return;
}



void do_credits (CHAR_DATA * ch, char *argument)
{
    do_function (ch, &do_help, "diku");
    return;
}



void do_where (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Players near you:\n\r", ch);
        found = FALSE;
        for (d = descriptor_list; d; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && (victim = d->character) != NULL && !IS_NPC (victim)
                && victim->in_room != NULL
                && !IS_SET (victim->in_room->room_flags, ROOM_NOWHERE)
                && (is_room_owner (ch, victim->in_room)
                    || !room_is_private (victim->in_room))
                && victim->in_room->area == ch->in_room->area
                && can_see (ch, victim))
            {
                found = TRUE;
                sprintf (buf, "%-28s %s\n\r",
                         victim->name, victim->in_room->name);
                sendch (buf, ch);
            }
        }
        if (!found)
            sendch ("None\n\r", ch);
    }
    else
    {
        found = FALSE;
        for (victim = char_list; victim != NULL; victim = victim->next)
        {
            if (victim->in_room != NULL
                && victim->in_room->area == ch->in_room->area
                && !IS_AFFECTED (victim, AFF_HIDE)
                && !IS_AFFECTED (victim, AFF_SNEAK)
                && can_see (ch, victim) && is_name (arg, victim->name))
            {
                found = TRUE;
                sprintf (buf, "%-28s %s\n\r",
                         PERS (victim, ch), victim->in_room->name);
                sendch (buf, ch);
                break;
            }
        }
        if (!found)
            act ("You didn't find any $T.", ch, NULL, arg, TO_CHAR);
    }

    return;
}




void do_consider (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int percent;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        sendch ("Consider killing whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They're not here.\n\r", ch);
        return;
    }

    if (is_safe (ch, victim))
    {
        sendch ("Don't even think about it.\n\r", ch);
        return;
    }

    percent =  100 * ch->nDifficulty / UMAX(1, victim->nDifficulty);

    if (percent < 20)
        msg = "Death will thank you for your gift.";
    else if (percent < 50)
        msg = "$N laughs at you mercilessly.";
    else if (percent < 80)
        msg = "$N says 'Do you feel lucky, punk?'.";
    else if (percent < 110)
        msg = "The perfect match!";
    else if (percent < 135)
        msg = "$N looks like an easy kill.";
    else if (percent < 175)
        msg = "$N is no match for you.";
    else
        msg = "You can kill $N naked and weaponless.";

    act (msg, ch, NULL, victim, TO_CHAR);
    if (IS_IMMORTAL (ch)) {
        printf_to_char (ch, "  Your difficulty: %d\n\r", ch->nDifficulty);
        printf_to_char (ch, "Target difficulty: %d\n\r", victim->nDifficulty);
        printf_to_char (ch, "          Percent: %d\n\r", percent);
    }
    return;
}



void set_title (CHAR_DATA * ch, char *title)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC (ch))
    {
        logstr (LOG_BUG, "Set_title: NPC.", 0);
        return;
    }

    if (title[0] != '.' && title[0] != ',' && title[0] != '!'
        && title[0] != '?')
    {
        buf[0] = ' ';
        strcpy (buf + 1, title);
    }
    else
    {
        strcpy (buf, title);
    }

    free_string (ch->pcdata->title);
    ch->pcdata->title = str_dup (buf);
    return;
}



void do_title (CHAR_DATA * ch, char *argument)
{
    int i;

    if (IS_NPC (ch))
        return;

	/* Changed this around a bit to do some sanitization first   *
	 * before checking length of the title. Need to come up with *
	 * a centralized user input sanitization scheme. FIXME!      *
	 * JR -- 10/15/00                                            */

    if (strlen (argument) > 45)
        argument[45] = '\0';

	i = strlen(argument);
    if (argument[i-2] != '{' && argument[i-1] == '{')
		argument[i-1] = '\0';

	if (argument[i-2] != '{' || argument[i-1] != 'x')
		strcat (argument, "{x");

	if (argument[0] == '\0')
    {
        sendch ("Change your title to what?\n\r", ch);
        return;
    }

    smash_tilde (argument);
    set_title (ch, argument);
    sendch ("Ok.\n\r", ch);
}

void do_pose (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int i;

    if (IS_NPC (ch))
        return;

	/* Changed this around a bit to do some sanitization first   *
	 * before checking length of the title. Need to come up with *
	 * a centralized user input sanitization scheme. FIXME!      *
	 * JR -- 10/15/00                                            */

    if (strlen (argument) > 45)
        argument[45] = '\0';

	i = strlen(argument);
    if (argument[i-2] != '{' && argument[i-1] == '{')
		argument[i-1] = '\0';

	if (argument[i-2] != '{' || argument[i-1] != 'x')
		strcat (argument, "{x");

	if (argument[0] == '\0')
    {
        sendch ("Change your pose to what?\n\r", ch);
        return;
    }

    smash_tilde (argument);

    if (argument[0] != '.'
        && argument[0] != ','
        && argument[0] != '!'
        && argument[0] != '?') {
        buf[0] = ' ';
        strcpy (buf + 1, argument);
    }
    else
        strcpy (buf, argument);
    free_string (ch->pcdata->pose);
    ch->pcdata->pose = str_dup (buf);
    sendch ("Ok.\n\r", ch);
}

void do_description (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (argument[0] != '\0')
    {
        buf[0] = '\0';
        smash_tilde (argument);

        if (argument[0] == '-')
        {
            int len;
            bool found = FALSE;

            if (ch->description == NULL || ch->description[0] == '\0')
            {
                sendch ("No lines left to remove.\n\r", ch);
                return;
            }

            strcpy (buf, ch->description);

            for (len = strlen (buf); len > 0; len--)
            {
                if (buf[len] == '\r')
                {
                    if (!found)
                    {            /* back it up */
                        if (len > 0)
                            len--;
                        found = TRUE;
                    }
                    else
                    {            /* found the second one */

                        buf[len + 1] = '\0';
                        free_string (ch->description);
                        ch->description = str_dup (buf);
                        sendch ("Your description is:\n\r", ch);
                        sendch (ch->description ? ch->description :
                                      "(None).\n\r", ch);
                        return;
                    }
                }
            }
            buf[0] = '\0';
            free_string (ch->description);
            ch->description = str_dup (buf);
            sendch ("Description cleared.\n\r", ch);
            return;
        }
        if (argument[0] == '+')
        {
            if (ch->description != NULL)
                strcat (buf, ch->description);
            argument++;
            while (isspace (*argument))
                argument++;
        }

        if (strlen (buf) >= 1024)
        {
            sendch ("Description too long.\n\r", ch);
            return;
        }

        strcat (buf, argument);
        strcat (buf, "\n\r");
        free_string (ch->description);
        ch->description = str_dup (buf);
    }

    sendch ("Your description is:\n\r", ch);
    sendch (ch->description ? ch->description : "(None).\n\r", ch);
    return;
}



void do_report (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    sprintf (buf,
             "You say 'I have %d/%d hp %d/%d ki %Ld pl.'\n\r",
             ch->hit, ch->max_hit,
             ch->ki, ch->max_ki, ch->nCurPl * ch->llPl / 100);

    sendch (buf, ch);

    sprintf (buf, "$n says 'I have %d/%d hp %d/%d ki %Ld pl.'",
             ch->hit, ch->max_hit,
             ch->ki, ch->max_ki, ch->nCurPl * ch->llPl / 100);

    act (buf, ch, NULL, NULL, TO_ROOM);

    return;
}


/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument (argument, arg);

    if (arg[0] == '\0')
        wimpy = ch->max_hit / 5;
    else
        wimpy = strtol (arg, NULL, 0);

    if (wimpy < 0)
    {
        sendch ("Your courage exceeds your wisdom.\n\r", ch);
        return;
    }

    if (wimpy > ch->max_hit / 2)
    {
        sendch ("Such cowardice ill becomes you.\n\r", ch);
        return;
    }

    ch->wimpy = wimpy;
    sprintf (buf, "Wimpy set to %d hit points.\n\r", wimpy);
    sendch (buf, ch);
    return;
}



void do_password (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if (IS_NPC (ch))
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while (isspace (*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0')
    {
        if (*argument == cEnd)
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while (isspace (*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0')
    {
        if (*argument == cEnd)
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        sendch ("Syntax: password <old> <new>.\n\r", ch);
        return;
    }

    if (strcmp (crypt (arg1, ch->pcdata->pwd), ch->pcdata->pwd))
    {
        wait (ch, 10*PULSE_SECOND);
        sendch ("Wrong password.  Wait 10 seconds.\n\r", ch);
        return;
    }

    if (strlen (arg2) < 5)
    {
        sendch ("New password must be at least five characters long.\n\r", ch);
        return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt (arg2, ch->name);
    for (p = pwdnew; *p != '\0'; p++)
    {
        if (*p == '~')
        {
            sendch ("New password not acceptable, try again.\n\r", ch);
            return;
        }
    }

    free_string (ch->pcdata->pwd);
    ch->pcdata->pwd = str_dup (pwdnew);
    save_char_obj (ch);
    sendch ("Ok.\n\r", ch);
    return;
}

void do_telnetga (CHAR_DATA * ch, char *argument)
{
	if (IS_NPC (ch))
		return;

	if (IS_SET (ch->comm, COMM_TELNET_GA))
	{
		sendch ("Telnet GA removed.\n\r", ch);
		REMOVE_BIT (ch->comm, COMM_TELNET_GA);
	}
	else
	{
		sendch ("Telnet GA enabled.\n\r", ch);
		SET_BIT (ch->comm, COMM_TELNET_GA);
	}
}


void do_balance (CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "Your balance is %d/5.\n\r", ch->balance);
	sendch(buf, ch);
}


void do_toplist (CHAR_DATA *ch, char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument (argument, arg1);
	argument = one_argument (argument, arg2);

    if (arg1[0] != '\0' && ch->level >= HEADBUILDER) {
        if (!str_prefix(arg1, "purge")) {
            PurgeTopList ();
            SaveTopList ();
            sendch ("The top list was purged.\n\r", ch);
            return;
        }
        else if (!str_prefix(arg1, "remove")) {
            if (arg2[0] == '\0') {
                sendch ("No argument was passed.\n\r", ch);
                return;
            }
            RemoveNameTopList (arg2);
            SaveTopList ();
            sprintf (buf, "%s was removed from the top list.\n\r", arg2);
            sendch (buf, ch);
            return;
        }
        else
            sendch ("Unknown parameter!\n\r", ch);
            // Just go through and display list
    }

    DisplayTopList (ch);
	if (ch->level >= HEADBUILDER) {
        sendch ("\n\rUse <toplist purge> to clean the list.\n\r", ch);
		sendch ("Use <toplist remove [name]> to remove a specific person.\n\r", ch);
	}
}




