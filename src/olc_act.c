/***************************************************************************
 *  File: olc_act.c                                                        *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"

char *prog_type_to_name (int type);

#define ALT_FLAGVALUE_SET( _blargh, _table, _arg )        \
    {                            \
        int blah = flag_value( _table, _arg );        \
        _blargh = (blah == NO_FLAG) ? 0 : blah;        \
    }

#define ALT_FLAGVALUE_TOGGLE( _blargh, _table, _arg )        \
    {                            \
        int blah = flag_value( _table, _arg );        \
        _blargh ^= (blah == NO_FLAG) ? 0 : blah;    \
    }

/* Return TRUE if area changed, FALSE if not. */
#define REDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define OEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define MEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define AEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )



struct olc_help_type {
    char *command;
    const void *structure;
    char *desc;
};



bool show_version (CHAR_DATA * ch, char *argument)
{
    sendch (VERSION, ch);
    sendch ("\n\r", ch);
    sendch (AUTHOR, ch);
    sendch ("\n\r", ch);
    sendch (DATE, ch);
    sendch ("\n\r", ch);
    sendch (CREDITS, ch);
    sendch ("\n\r", ch);

    return FALSE;
}

/*
 * This table contains help commands and a brief description of each.
 * ------------------------------------------------------------------
 */
const struct olc_help_type help_table[] = {
    {"area", area_flags, "Area attributes."},
    {"room", room_flags, "Room attributes."},
    {"sector", sector_flags, "Sector types, terrain."},
    {"exit", exit_flags, "Exit types."},
    {"type", type_flags, "Types of objects."},
    {"extra", extra_flags, "Object attributes."},
    {"wear", wear_flags, "Where to wear object."},
    {"spec", spec_table, "Available special programs."},
    {"sex", sex_flags, "Sexes."},
    {"act", act_flags, "Mobile attributes."},
    {"affect", affect_flags, "Mobile affects."},
    {"wear-loc", wear_loc_flags, "Where mobile wears object."},
    {"spells", skill_table, "Names of current spells."},
    {"container", container_flags, "Container status."},

/* ROM specific bits: */

    {"armor", ac_type, "Ac for different attacks."},
    {"apply", apply_flags, "Apply flags"},
    {"form", form_flags, "Mobile body form."},
    {"part", part_flags, "Mobile body parts."},
    {"imm", imm_flags, "Mobile immunity."},
    {"res", res_flags, "Mobile resistance."},
    {"vuln", vuln_flags, "Mobile vulnerability."},
    {"off", off_flags, "Mobile offensive behaviour."},
    {"size", size_flags, "Mobile size."},
    {"position", position_flags, "Mobile positions."},
    {"wclass", weapon_class, "Weapon class."},
    {"wtype", weapon_type2, "Special weapon type."},
    {"portal", portal_flags, "Portal types."},
    {"furniture", furniture_flags, "Furniture types."},
    {"liquid", liq_table, "Liquid types."},
    {"apptype", apply_types, "Apply types."},
    {"weapon", attack_table, "Weapon types."},
    {"mprog", mprog_flags, "MobProgram flags."},
    {	"oprog",	oprog_flags,	 "ObjProgram flags."		 },
    {	"rprog",	rprog_flags,	 "RoomProgram flags."		 },
    {NULL, NULL, NULL}
};



/*****************************************************************************
 Name:        show_flag_cmds
 Purpose:    Displays settable flags and stats.
 Called by:    show_help(olc_act.c).
 ****************************************************************************/
void show_flag_cmds (CHAR_DATA * ch, const struct flag_type *flag_table)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int flag;
    int col;

    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
        if (flag_table[flag].settable)
        {
            sprintf (buf, "%-19.18s", flag_table[flag].name);
            strcat (buf1, buf);
            if (++col % 4 == 0)
                strcat (buf1, "\n\r");
        }
    }

    if (col % 4 != 0)
        strcat (buf1, "\n\r");

    sendch (buf1, ch);
    return;
}


/*****************************************************************************
 Name:        show_skill_cmds
 Purpose:    Displays all skill functions.
         Does remove those damn immortal commands from the list.
         Could be improved by:
         (1) Adding a check for a particular class.
         (2) Adding a check for a level range.
 Called by:    show_help(olc_act.c).
 ****************************************************************************/
void show_skill_cmds (CHAR_DATA * ch, int tar)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH * 2];
    int sn;
    int col;

    buf1[0] = '\0';
    col = 0;
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (!skill_table[sn].name)
            break;

        if (!str_cmp (skill_table[sn].name, "reserved"))
            continue;

        if (tar == -1 || skill_table[sn].target == tar)
        {
            sprintf (buf, "%-19.18s", skill_table[sn].name);
            strcat (buf1, buf);
            if (++col % 4 == 0)
                strcat (buf1, "\n\r");
        }
    }

    if (col % 4 != 0)
        strcat (buf1, "\n\r");

    sendch (buf1, ch);
    return;
}



/*****************************************************************************
 Name:        show_spec_cmds
 Purpose:    Displays settable special functions.
 Called by:    show_help(olc_act.c).
 ****************************************************************************/
void show_spec_cmds (CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int spec;
    int col;

    buf1[0] = '\0';
    col = 0;
    sendch ("Preceed special functions with 'spec_'\n\r\n\r", ch);
    for (spec = 0; spec_table[spec].function != NULL; spec++)
    {
        sprintf (buf, "%-19.18s", &spec_table[spec].name[5]);
        strcat (buf1, buf);
        if (++col % 4 == 0)
            strcat (buf1, "\n\r");
    }

    if (col % 4 != 0)
        strcat (buf1, "\n\r");

    sendch (buf1, ch);
    return;
}



/*****************************************************************************
 Name:        show_help
 Purpose:    Displays help for many tables used in OLC.
 Called by:    olc interpreters.
 ****************************************************************************/
bool show_help (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char spell[MAX_INPUT_LENGTH];
    int cnt;

    argument = one_argument (argument, arg);
    one_argument (argument, spell);

    /*
     * Display syntax.
     */
    if (arg[0] == '\0')
    {
        sendch ("Syntax:  ? [command]\n\r\n\r", ch);
        sendch ("[command]  [description]\n\r", ch);
        for (cnt = 0; help_table[cnt].command != NULL; cnt++)
        {
            sprintf (buf, "%-10.10s -%s\n\r",
                     capitalize (help_table[cnt].command),
                     help_table[cnt].desc);
            sendch (buf, ch);
        }
        return FALSE;
    }

    /*
     * Find the command, show changeable data.
     * ---------------------------------------
     */
    for (cnt = 0; help_table[cnt].command != NULL; cnt++)
    {
        if (arg[0] == help_table[cnt].command[0]
            && !str_prefix (arg, help_table[cnt].command))
        {
            if (help_table[cnt].structure == spec_table)
            {
                show_spec_cmds (ch);
                return FALSE;
            }
            else if (help_table[cnt].structure == liq_table)
            {
                show_liqlist (ch);
                return FALSE;
            }
            else if (help_table[cnt].structure == attack_table)
            {
                show_damlist (ch);
                return FALSE;
            }
            else if (help_table[cnt].structure == skill_table)
            {

                if (spell[0] == '\0')
                {
                    sendch ("Syntax:  ? spells "
                                  "[ignore/attack/defend/self/object/areaoff/all]\n\r",
                                  ch);
                    return FALSE;
                }

                if (!str_prefix (spell, "all"))
                    show_skill_cmds (ch, -1);
                else if (!str_prefix (spell, "ignore"))
                    show_skill_cmds (ch, TAR_IGNORE);
                else if (!str_prefix (spell, "attack"))
                    show_skill_cmds (ch, TAR_CHAR_OFFENSIVE);
                else if (!str_prefix (spell, "defend"))
                    show_skill_cmds (ch, TAR_CHAR_DEFENSIVE);
                else if (!str_prefix (spell, "self"))
                    show_skill_cmds (ch, TAR_CHAR_SELF);
                else if (!str_prefix (spell, "object"))
                    show_skill_cmds (ch, TAR_OBJ_INV);
                else if (!str_prefix (spell, "areaoff"))
                    show_skill_cmds (ch, TAR_AREA_OFF);
                else
                    sendch ("Syntax:  ? spell "
                                  "[ignore/attack/defend/self/object/areaoff/all]\n\r",
                                  ch);

                return FALSE;
            }
            else
            {
                show_flag_cmds (ch, help_table[cnt].structure);
                return FALSE;
            }
        }
    }

    show_help (ch, "");
    return FALSE;
}

REDIT (redit_mshow)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  mshow <vnum>\n\r", ch);
        return FALSE;
    }

    if (!is_number (argument))
    {
        sendch ("REdit: Must be a number.\n\r", ch);
        return FALSE;
    }

    if (is_number (argument))
    {
        value = atoi (argument);
        if (!(pMob = get_mob_index (value)))
        {
            sendch ("REdit:  That mobile does not exist.\n\r", ch);
            return FALSE;
        }

        ch->desc->pEdit = (void *) pMob;
    }

    medit_show (ch, argument);
    ch->desc->pEdit = (void *) ch->in_room;
    return FALSE;
}



REDIT (redit_oshow)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  oshow <vnum>\n\r", ch);
        return FALSE;
    }

    if (!is_number (argument))
    {
        sendch ("REdit: Must be a number.\n\r", ch);
        return FALSE;
    }

    if (is_number (argument))
    {
        value = atoi (argument);
        if (!(pObj = get_obj_index (value)))
        {
            sendch ("REdit:  That object does not exist.\n\r", ch);
            return FALSE;
        }

        ch->desc->pEdit = (void *) pObj;
    }

    oedit_show (ch, argument);
    ch->desc->pEdit = (void *) ch->in_room;
    return FALSE;
}



/*****************************************************************************
 Name:        check_range( lower vnum, upper vnum )
 Purpose:    Ensures the range spans only one area.
 Called by:    aedit_vnum(olc_act.c).
 ****************************************************************************/
bool check_range (int lower, int upper)
{
    AREA_DATA *pArea;
    int cnt = 0;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        /*
         * lower < area < upper
         */
        if ((lower <= pArea->min_vnum && pArea->min_vnum <= upper)
            || (lower <= pArea->max_vnum && pArea->max_vnum <= upper))
            ++cnt;

        if (cnt > 1)
            return FALSE;
    }
    return TRUE;
}



AREA_DATA *get_vnum_area (int vnum)
{
    AREA_DATA *pArea;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        if (vnum >= pArea->min_vnum && vnum <= pArea->max_vnum)
            return pArea;
    }

    return 0;
}



/*
 * Area Editor Functions.
 */
AEDIT (aedit_show)
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA (ch, pArea);

    sprintf (buf, "Name:     [%5d] %s\n\r", pArea->vnum, pArea->name);
    sendch (buf, ch);

#if 0                            /* ROM OLC */
    sprintf (buf, "Recall:   [%5d] %s\n\r", pArea->recall,
             get_room_index (pArea->recall)
             ? get_room_index (pArea->recall)->name : "none");
    sendch (buf, ch);
#endif /* ROM */

    sprintf (buf, "File:     %s\n\r", pArea->file_name);
    sendch (buf, ch);

    sprintf (buf, "Vnums:    [%d-%d]\n\r", pArea->min_vnum, pArea->max_vnum);
    sendch (buf, ch);

    sprintf (buf, "Age:      [%d]\n\r", pArea->age);
    sendch (buf, ch);

    sprintf (buf, "Players:  [%d]\n\r", pArea->nplayer);
    sendch (buf, ch);

    sprintf (buf, "Security: [%d]\n\r", pArea->security);
    sendch (buf, ch);

    sprintf (buf, "Builders: [%s]\n\r", pArea->builders);
    sendch (buf, ch);

    sprintf (buf, "Credits : [%s]\n\r", pArea->credits);
    sendch (buf, ch);

    sprintf (buf, "Flags:    [%s]\n\r",
             flag_string (area_flags, pArea->area_flags));
    sendch (buf, ch);

    return FALSE;
}



AEDIT (aedit_reset)
{
    AREA_DATA *pArea;

    EDIT_AREA (ch, pArea);

    reset_area (pArea);
    sendch ("Area reset.\n\r", ch);

    return FALSE;
}



AEDIT (aedit_create)
{
    AREA_DATA *pArea;

    pArea = new_area ();
    area_last->next = pArea;
    area_last = pArea;            /* Thanks, Walker. */
    ch->desc->pEdit = (void *) pArea;

    SET_BIT (pArea->area_flags, AREA_ADDED);
    sendch ("Area Created.\n\r", ch);
    return FALSE;
}



AEDIT (aedit_name)
{
    AREA_DATA *pArea;

    EDIT_AREA (ch, pArea);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:   name [$name]\n\r", ch);
        return FALSE;
    }

    free_string (pArea->name);
    pArea->name = str_dup (argument);

    sendch ("Name set.\n\r", ch);
    return TRUE;
}

AEDIT (aedit_credits)
{
    AREA_DATA *pArea;

    EDIT_AREA (ch, pArea);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:   credits [$credits]\n\r", ch);
        return FALSE;
    }

    free_string (pArea->credits);
    pArea->credits = str_dup (argument);

    sendch ("Credits set.\n\r", ch);
    return TRUE;
}


AEDIT (aedit_file)
{
    AREA_DATA *pArea;
    char file[MAX_STRING_LENGTH];
    int i, length;

    EDIT_AREA (ch, pArea);

    one_argument (argument, file);    /* Forces Lowercase */

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  filename [$file]\n\r", ch);
        return FALSE;
    }

    /*
     * Simple Syntax Check.
     */
    length = strlen (argument);
    if (length > 8)
    {
        sendch ("No more than eight characters allowed.\n\r", ch);
        return FALSE;
    }

    /*
     * Allow only letters and numbers.
     */
    for (i = 0; i < length; i++)
    {
        if (!isalnum (file[i]))
        {
            sendch ("Only letters and numbers are valid.\n\r", ch);
            return FALSE;
        }
    }

    free_string (pArea->file_name);
    strcat (file, ".are");
    pArea->file_name = str_dup (file);

    sendch ("Filename set.\n\r", ch);
    return TRUE;
}



AEDIT (aedit_age)
{
    AREA_DATA *pArea;
    char age[MAX_STRING_LENGTH];

    EDIT_AREA (ch, pArea);

    one_argument (argument, age);

    if (!is_number (age) || age[0] == '\0')
    {
        sendch ("Syntax:  age [#xage]\n\r", ch);
        return FALSE;
    }

    pArea->age = atoi (age);

    sendch ("Age set.\n\r", ch);
    return TRUE;
}


#if 0                            /* ROM OLC */
AEDIT (aedit_recall)
{
    AREA_DATA *pArea;
    char room[MAX_STRING_LENGTH];
    int value;

    EDIT_AREA (ch, pArea);

    one_argument (argument, room);

    if (!is_number (argument) || argument[0] == '\0')
    {
        sendch ("Syntax:  recall [#xrvnum]\n\r", ch);
        return FALSE;
    }

    value = atoi (room);

    if (!get_room_index (value))
    {
        sendch ("AEdit:  Room vnum does not exist.\n\r", ch);
        return FALSE;
    }

    pArea->recall = value;

    sendch ("Recall set.\n\r", ch);
    return TRUE;
}
#endif /* ROM OLC */


AEDIT (aedit_security)
{
    AREA_DATA *pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int value;

    EDIT_AREA (ch, pArea);

    one_argument (argument, sec);

    if (!is_number (sec) || sec[0] == '\0')
    {
        sendch ("Syntax:  security [#xlevel]\n\r", ch);
        return FALSE;
    }

    value = atoi (sec);

    if (value > ch->pcdata->security || value < 0)
    {
        if (ch->pcdata->security != 0)
        {
            sprintf (buf, "Security is 0-%d.\n\r", ch->pcdata->security);
            sendch (buf, ch);
        }
        else
            sendch ("Security is 0 only.\n\r", ch);
        return FALSE;
    }

    pArea->security = value;

    sendch ("Security set.\n\r", ch);
    return TRUE;
}



AEDIT (aedit_builder)
{
    AREA_DATA *pArea;
    char name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA (ch, pArea);

    one_argument (argument, name);

    if (name[0] == '\0')
    {
        sendch ("Syntax:  builder [$name]  -toggles builder\n\r", ch);
        sendch ("Syntax:  builder All      -allows everyone\n\r", ch);
        return FALSE;
    }

    name[0] = UPPER (name[0]);

    if (strstr (pArea->builders, name) != '\0')
    {
        pArea->builders = string_replace (pArea->builders, name, "\0");
        pArea->builders = string_unpad (pArea->builders);

        if (pArea->builders[0] == '\0')
        {
            free_string (pArea->builders);
            pArea->builders = str_dup ("None");
        }
        sendch ("Builder removed.\n\r", ch);
        return TRUE;
    }
    else
    {
        buf[0] = '\0';
        if (strstr (pArea->builders, "None") != '\0')
        {
            pArea->builders = string_replace (pArea->builders, "None", "\0");
            pArea->builders = string_unpad (pArea->builders);
        }

        if (pArea->builders[0] != '\0')
        {
            strcat (buf, pArea->builders);
            strcat (buf, " ");
        }
        strcat (buf, name);
        free_string (pArea->builders);
        pArea->builders = string_proper (str_dup (buf));

        sendch ("Builder added.\n\r", ch);
        sendch (pArea->builders, ch);
        return TRUE;
    }

    return FALSE;
}



AEDIT (aedit_vnum)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA (ch, pArea);

    argument = one_argument (argument, lower);
    one_argument (argument, upper);

    if (!is_number (lower) || lower[0] == '\0'
        || !is_number (upper) || upper[0] == '\0')
    {
        sendch ("Syntax:  vnum [#xlower] [#xupper]\n\r", ch);
        return FALSE;
    }

    if ((ilower = atoi (lower)) > (iupper = atoi (upper)))
    {
        sendch ("AEdit:  Upper must be larger then lower.\n\r", ch);
        return FALSE;
    }

    if (!check_range (atoi (lower), atoi (upper)))
    {
        sendch ("AEdit:  Range must include only this area.\n\r", ch);
        return FALSE;
    }

    if (get_vnum_area (ilower) && get_vnum_area (ilower) != pArea)
    {
        sendch ("AEdit:  Lower vnum already assigned.\n\r", ch);
        return FALSE;
    }

    pArea->min_vnum = ilower;
    sendch ("Lower vnum set.\n\r", ch);

    if (get_vnum_area (iupper) && get_vnum_area (iupper) != pArea)
    {
        sendch ("AEdit:  Upper vnum already assigned.\n\r", ch);
        return TRUE;            /* The lower value has been set. */
    }

    pArea->max_vnum = iupper;
    sendch ("Upper vnum set.\n\r", ch);

    return TRUE;
}



AEDIT (aedit_lvnum)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA (ch, pArea);

    one_argument (argument, lower);

    if (!is_number (lower) || lower[0] == '\0')
    {
        sendch ("Syntax:  min_vnum [#xlower]\n\r", ch);
        return FALSE;
    }

    if ((ilower = atoi (lower)) > (iupper = pArea->max_vnum))
    {
        sendch ("AEdit:  Value must be less than the max_vnum.\n\r",
                      ch);
        return FALSE;
    }

    if (!check_range (ilower, iupper))
    {
        sendch ("AEdit:  Range must include only this area.\n\r", ch);
        return FALSE;
    }

    if (get_vnum_area (ilower) && get_vnum_area (ilower) != pArea)
    {
        sendch ("AEdit:  Lower vnum already assigned.\n\r", ch);
        return FALSE;
    }

    pArea->min_vnum = ilower;
    sendch ("Lower vnum set.\n\r", ch);
    return TRUE;
}



AEDIT (aedit_uvnum)
{
    AREA_DATA *pArea;
    char upper[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA (ch, pArea);

    one_argument (argument, upper);

    if (!is_number (upper) || upper[0] == '\0')
    {
        sendch ("Syntax:  max_vnum [#xupper]\n\r", ch);
        return FALSE;
    }

    if ((ilower = pArea->min_vnum) > (iupper = atoi (upper)))
    {
        sendch ("AEdit:  Upper must be larger then lower.\n\r", ch);
        return FALSE;
    }

    if (!check_range (ilower, iupper))
    {
        sendch ("AEdit:  Range must include only this area.\n\r", ch);
        return FALSE;
    }

    if (get_vnum_area (iupper) && get_vnum_area (iupper) != pArea)
    {
        sendch ("AEdit:  Upper vnum already assigned.\n\r", ch);
        return FALSE;
    }

    pArea->max_vnum = iupper;
    sendch ("Upper vnum set.\n\r", ch);

    return TRUE;
}



/*
 * Room Editor Functions.
 */
REDIT (redit_show)
{
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    char buf1[2 * MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *rch;
	PROG_LIST		*list;
    int door;
    bool fcnt;

    EDIT_ROOM (ch, pRoom);

    buf1[0] = '\0';

    sprintf (buf, "Description:\n\r%s", pRoom->description);
    strcat (buf1, buf);

    sprintf (buf, "Name:       [%s]\n\rArea:       [%5d] %s\n\r",
             pRoom->name, pRoom->area->vnum, pRoom->area->name);
    strcat (buf1, buf);

    sprintf (buf, "Vnum:       [%5d]\n\rSector:     [%s]\n\r",
             pRoom->vnum, flag_string (sector_flags, pRoom->sector_type));
    strcat (buf1, buf);

    sprintf (buf, "Room flags: [%s]\n\r",
             flag_string (room_flags, pRoom->room_flags));
    strcat (buf1, buf);

    if (pRoom->heal_rate != 100 || pRoom->ki_rate != 100)
    {
        sprintf (buf, "Health rec: [%d]\n\rKi rec  : [%d]\n\r",
                 pRoom->heal_rate, pRoom->ki_rate);
        strcat (buf1, buf);
    }

    if (pRoom->clan > 0)
    {
        sprintf (buf, "Clan      : [%d] %s\n\r",
                 pRoom->clan, clan_table[pRoom->clan].name);
        strcat (buf1, buf);
    }

    if (!IS_NULLSTR (pRoom->owner))
    {
        sprintf (buf, "Owner     : [%s]\n\r", pRoom->owner);
        strcat (buf1, buf);
    }

    if (pRoom->extra_descr)
    {
        EXTRA_DESCR_DATA *ed;

        strcat (buf1, "Desc Kwds:  [");
        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            strcat (buf1, ed->keyword);
            if (ed->next)
                strcat (buf1, " ");
        }
        strcat (buf1, "]\n\r");
    }

    strcat (buf1, "Characters: [");
    fcnt = FALSE;
    for (rch = pRoom->people; rch; rch = rch->next_in_room)
    {
        one_argument (rch->name, buf);
        strcat (buf1, buf);
        strcat (buf1, " ");
        fcnt = TRUE;
    }

    if (fcnt)
    {
        int end;

        end = strlen (buf1) - 1;
        buf1[end] = ']';
        strcat (buf1, "\n\r");
    }
    else
        strcat (buf1, "none]\n\r");

    strcat (buf1, "Objects:    [");
    fcnt = FALSE;
    for (obj = pRoom->contents; obj; obj = obj->next_content)
    {
        one_argument (obj->name, buf);
        strcat (buf1, buf);
        strcat (buf1, " ");
        fcnt = TRUE;
    }

    if (fcnt)
    {
        int end;

        end = strlen (buf1) - 1;
        buf1[end] = ']';
        strcat (buf1, "\n\r");
    }
    else
        strcat (buf1, "none]\n\r");

    for (door = 0; door < MAX_DIR; door++)
    {
        EXIT_DATA *pexit;

        if ((pexit = pRoom->exit[door]))
        {
            char word[MAX_INPUT_LENGTH];
            char reset_state[MAX_STRING_LENGTH];
            char *state;
            int i, length;

            sprintf (buf, "-%-5s to [%5d] Key: [%5d] ",
                     capitalize (dir_name[door]),
                     pexit->u1.to_room ? pexit->u1.to_room->vnum : 0,    /* ROM OLC */
                     pexit->key);
            strcat (buf1, buf);

            /*
             * Format up the exit info.
             * Capitalize all flags that are not part of the reset info.
             */
            strcpy (reset_state, flag_string (exit_flags, pexit->rs_flags));
            state = flag_string (exit_flags, pexit->exit_info);
            strcat (buf1, " Exit flags: [");
            for (;;)
            {
                state = one_argument (state, word);

                if (word[0] == '\0')
                {
                    int end;

                    end = strlen (buf1) - 1;
                    buf1[end] = ']';
                    strcat (buf1, "\n\r");
                    break;
                }

                if (str_infix (word, reset_state))
                {
                    length = strlen (word);
                    for (i = 0; i < length; i++)
                        word[i] = UPPER (word[i]);
                }
                strcat (buf1, word);
                strcat (buf1, " ");
            }

            if (pexit->keyword && pexit->keyword[0] != '\0')
            {
                sprintf (buf, "Kwds: [%s]\n\r", pexit->keyword);
                strcat (buf1, buf);
            }
            if (pexit->description && pexit->description[0] != '\0')
            {
                sprintf (buf, "%s", pexit->description);
                strcat (buf1, buf);
            }
        }
    }

    sendch (buf1, ch);

    if ( pRoom->rprogs )
    {
		int cnt;

		sprintf(buf, "\n\rROOMPrograms for [%5d]:\n\r", pRoom->vnum);
		sendch( buf, ch );

		for (cnt=0, list=pRoom->rprogs; list; list=list->next)
		{
			if (cnt ==0)
			{
				sendch ( " Number Vnum Trigger Phrase\n\r", ch );
				sendch ( " ------ ---- ------- ------\n\r", ch );
			}

			sprintf(buf, "[%5d] %4d %7s %s\n\r", cnt,
				list->vnum,prog_type_to_name(list->trig_type),
				list->trig_phrase);
			sendch( buf, ch );
			cnt++;
		}
    }

    return FALSE;
}




/* Local function. */
bool change_exit (CHAR_DATA * ch, char *argument, int door)
{
    ROOM_INDEX_DATA *pRoom;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int value;

    EDIT_ROOM (ch, pRoom);

    /*
     * Set the exit flags, needs full argument.
     * ----------------------------------------
     */
    if ((value = flag_value (exit_flags, argument)) != NO_FLAG)
    {
        ROOM_INDEX_DATA *pToRoom;
        sh_int rev;                /* ROM OLC */

        if (!pRoom->exit[door])
        {
            sendch ("Exit doesn't exist.\n\r", ch);
            return FALSE;
        }

        /*
         * This room.
         */
        TOGGLE_BIT (pRoom->exit[door]->rs_flags, value);
        /* Don't toggle exit_info because it can be changed by players. */
        pRoom->exit[door]->exit_info = pRoom->exit[door]->rs_flags;

        /*
         * Connected room.
         */
        pToRoom = pRoom->exit[door]->u1.to_room;    /* ROM OLC */
        rev = rev_dir[door];

        if (pToRoom->exit[rev] != NULL)
        {
            pToRoom->exit[rev]->rs_flags = pRoom->exit[door]->rs_flags;
            pToRoom->exit[rev]->exit_info = pRoom->exit[door]->exit_info;
        }

        sendch ("Exit flag toggled.\n\r", ch);
        return TRUE;
    }

    /*
     * Now parse the arguments.
     */
    argument = one_argument (argument, command);
    one_argument (argument, arg);

    if (command[0] == '\0' && argument[0] == '\0')
    {                            /* Move command. */
        move_char (ch, door, TRUE);    /* ROM OLC */
        return FALSE;
    }

    if (command[0] == '?')
    {
        do_help (ch, "EXIT");
        return FALSE;
    }

    if (!str_cmp (command, "delete"))
    {
        ROOM_INDEX_DATA *pToRoom;
        sh_int rev;                /* ROM OLC */

        if (!pRoom->exit[door])
        {
            sendch ("REdit:  Cannot delete a null exit.\n\r", ch);
            return FALSE;
        }

        /*
         * Remove ToRoom Exit.
         */
        rev = rev_dir[door];
        pToRoom = pRoom->exit[door]->u1.to_room;    /* ROM OLC */

        if (pToRoom->exit[rev])
        {
            free_exit (pToRoom->exit[rev]);
            pToRoom->exit[rev] = NULL;
        }

        /*
         * Remove this exit.
         */
        free_exit (pRoom->exit[door]);
        pRoom->exit[door] = NULL;

        sendch ("Exit unlinked.\n\r", ch);
        return TRUE;
    }

    if (!str_cmp (command, "link"))
    {
        EXIT_DATA *pExit;
        ROOM_INDEX_DATA *toRoom;

        if (arg[0] == '\0' || !is_number (arg))
        {
            sendch ("Syntax:  [direction] link [vnum]\n\r", ch);
            return FALSE;
        }

        value = atoi (arg);

        if (!(toRoom = get_room_index (value)))
        {
            sendch ("REdit:  Cannot link to non-existant room.\n\r",
                          ch);
            return FALSE;
        }

        if (!IS_BUILDER (ch, toRoom->area))
        {
            sendch ("REdit:  Cannot link to that area.\n\r", ch);
            return FALSE;
        }

        if (toRoom->exit[rev_dir[door]])
        {
            sendch ("REdit:  Remote side's exit already exists.\n\r",
                          ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
            pRoom->exit[door] = new_exit ();

        pRoom->exit[door]->u1.to_room = toRoom;
        pRoom->exit[door]->orig_door = door;

        door = rev_dir[door];
        pExit = new_exit ();
        pExit->u1.to_room = pRoom;
        pExit->orig_door = door;
        toRoom->exit[door] = pExit;

        sendch ("Two-way link established.\n\r", ch);
        return TRUE;
    }

    if (!str_cmp (command, "dig"))
    {
        char buf[MAX_STRING_LENGTH];

        if (arg[0] == '\0' || !is_number (arg))
        {
            sendch ("Syntax: [direction] dig <vnum>\n\r", ch);
            return FALSE;
        }

        redit_create (ch, arg);
        sprintf (buf, "link %s", arg);
        change_exit (ch, buf, door);
        return TRUE;
    }

    if (!str_cmp (command, "room"))
    {
        ROOM_INDEX_DATA *toRoom;

        if (arg[0] == '\0' || !is_number (arg))
        {
            sendch ("Syntax:  [direction] room [vnum]\n\r", ch);
            return FALSE;
        }

        value = atoi (arg);

        if (!(toRoom = get_room_index (value)))
        {
            sendch ("REdit:  Cannot link to non-existant room.\n\r",
                          ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
            pRoom->exit[door] = new_exit ();

        pRoom->exit[door]->u1.to_room = toRoom;    /* ROM OLC */
        pRoom->exit[door]->orig_door = door;

        sendch ("One-way link established.\n\r", ch);
        return TRUE;
    }

    if (!str_cmp (command, "key"))
    {
        OBJ_INDEX_DATA *key;

        if (arg[0] == '\0' || !is_number (arg))
        {
            sendch ("Syntax:  [direction] key [vnum]\n\r", ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
        {
            sendch ("Exit doesn't exist.\n\r", ch);
            return FALSE;
        }

        value = atoi (arg);

        if (!(key = get_obj_index (value)))
        {
            sendch ("REdit:  Key doesn't exist.\n\r", ch);
            return FALSE;
        }

        if (key->item_type != ITEM_KEY)
        {
            sendch ("REdit:  Object is not a key.\n\r", ch);
            return FALSE;
        }

        pRoom->exit[door]->key = value;

        sendch ("Exit key set.\n\r", ch);
        return TRUE;
    }

    if (!str_cmp (command, "name"))
    {
        if (arg[0] == '\0')
        {
            sendch ("Syntax:  [direction] name [string]\n\r", ch);
            sendch ("         [direction] name none\n\r", ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
        {
            sendch ("Exit doesn't exist.\n\r", ch);
            return FALSE;
        }

        free_string (pRoom->exit[door]->keyword);

        if (str_cmp (arg, "none"))
            pRoom->exit[door]->keyword = str_dup (arg);
        else
            pRoom->exit[door]->keyword = str_dup ("");

        sendch ("Exit name set.\n\r", ch);
        return TRUE;
    }

    if (!str_prefix (command, "description"))
    {
        if (arg[0] == '\0')
        {
            if (!pRoom->exit[door])
            {
                sendch ("Exit doesn't exist.\n\r", ch);
                return FALSE;
            }

            string_append (ch, &pRoom->exit[door]->description);
            return TRUE;
        }

        sendch ("Syntax:  [direction] desc\n\r", ch);
        return FALSE;
    }

    return FALSE;
}



REDIT (redit_north)
{
    if (change_exit (ch, argument, DIR_NORTH))
        return TRUE;

    return FALSE;
}



REDIT (redit_south)
{
    if (change_exit (ch, argument, DIR_SOUTH))
        return TRUE;

    return FALSE;
}



REDIT (redit_east)
{
    if (change_exit (ch, argument, DIR_EAST))
        return TRUE;

    return FALSE;
}



REDIT (redit_west)
{
    if (change_exit (ch, argument, DIR_WEST))
        return TRUE;

    return FALSE;
}



REDIT (redit_up)
{
    if (change_exit (ch, argument, DIR_UP))
        return TRUE;

    return FALSE;
}



REDIT (redit_down)
{
    if (change_exit (ch, argument, DIR_DOWN))
        return TRUE;

    return FALSE;
}



REDIT (redit_ed)
{
    ROOM_INDEX_DATA *pRoom;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_ROOM (ch, pRoom);

    argument = one_argument (argument, command);
    one_argument (argument, keyword);

    if (command[0] == '\0' || keyword[0] == '\0')
    {
        sendch ("Syntax:  ed add [keyword]\n\r", ch);
        sendch ("         ed edit [keyword]\n\r", ch);
        sendch ("         ed delete [keyword]\n\r", ch);
        sendch ("         ed format [keyword]\n\r", ch);
        return FALSE;
    }

    if (!str_cmp (command, "add"))
    {
        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed add [keyword]\n\r", ch);
            return FALSE;
        }

        ed = new_extra_descr ();
        ed->keyword = str_dup (keyword);
        ed->description = str_dup ("");
        ed->next = pRoom->extra_descr;
        pRoom->extra_descr = ed;

        string_append (ch, &ed->description);

        return TRUE;
    }


    if (!str_cmp (command, "edit"))
    {
        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed edit [keyword]\n\r", ch);
            return FALSE;
        }

        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            if (is_name (keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            sendch ("REdit:  Extra description keyword not found.\n\r",
                          ch);
            return FALSE;
        }

        string_append (ch, &ed->description);

        return TRUE;
    }


    if (!str_cmp (command, "delete"))
    {
        EXTRA_DESCR_DATA *ped = NULL;

        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed delete [keyword]\n\r", ch);
            return FALSE;
        }

        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            if (is_name (keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed)
        {
            sendch ("REdit:  Extra description keyword not found.\n\r",
                          ch);
            return FALSE;
        }

        if (!ped)
            pRoom->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr (ed);

        sendch ("Extra description deleted.\n\r", ch);
        return TRUE;
    }


    if (!str_cmp (command, "format"))
    {
        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed format [keyword]\n\r", ch);
            return FALSE;
        }

        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            if (is_name (keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            sendch ("REdit:  Extra description keyword not found.\n\r",
                          ch);
            return FALSE;
        }

        ed->description = format_string (ed->description);

        sendch ("Extra description formatted.\n\r", ch);
        return TRUE;
    }

    redit_ed (ch, "");
    return FALSE;
}



REDIT (redit_create)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    int value;
    int iHash;

    EDIT_ROOM (ch, pRoom);

    value = atoi (argument);

    if (argument[0] == '\0' || value <= 0)
    {
        sendch ("Syntax:  create [vnum > 0]\n\r", ch);
        return FALSE;
    }

    pArea = get_vnum_area (value);
    if (!pArea)
    {
        sendch ("REdit:  That vnum is not assigned an area.\n\r", ch);
        return FALSE;
    }

    if (!IS_BUILDER (ch, pArea))
    {
        sendch ("REdit:  Vnum in an area you cannot build in.\n\r", ch);
        return FALSE;
    }

    if (get_room_index (value))
    {
        sendch ("REdit:  Room vnum already exists.\n\r", ch);
        return FALSE;
    }

    pRoom = new_room_index ();
    pRoom->area = pArea;
    pRoom->vnum = value;

    if (value > top_vnum_room)
        top_vnum_room = value;

    iHash = value % MAX_KEY_HASH;
    pRoom->next = room_index_hash[iHash];
    room_index_hash[iHash] = pRoom;
    ch->desc->pEdit = (void *) pRoom;

    sendch ("Room created.\n\r", ch);
    return TRUE;
}



REDIT (redit_name)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  name [name]\n\r", ch);
        return FALSE;
    }

    free_string (pRoom->name);
    pRoom->name = str_dup (argument);

    sendch ("Name set.\n\r", ch);
    return TRUE;
}



REDIT (redit_desc)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    if (argument[0] == '\0')
    {
        string_append (ch, &pRoom->description);
        return TRUE;
    }

    sendch ("Syntax:  desc\n\r", ch);
    return FALSE;
}

REDIT (redit_heal)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    if (is_number (argument))
    {
        pRoom->heal_rate = atoi (argument);
        sendch ("Heal rate set.\n\r", ch);
        return TRUE;
    }

    sendch ("Syntax: heal <#xnumber>\n\r", ch);
    return FALSE;
}

REDIT (redit_ki)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    if (is_number (argument))
    {
        pRoom->ki_rate = atoi (argument);
        sendch ("Ki rate set.\n\r", ch);
        return TRUE;
    }

    sendch ("Syntax: ki <#xnumber>\n\r", ch);
    return FALSE;
}

REDIT (redit_clan)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    pRoom->clan = clan_lookup (argument);

    sendch ("Clan set.\n\r", ch);
    return TRUE;
}

REDIT (redit_format)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    pRoom->description = format_string (pRoom->description);

    sendch ("String formatted.\n\r", ch);
    return TRUE;
}



REDIT (redit_mreset)
{
    ROOM_INDEX_DATA *pRoom;
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *newmob;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    RESET_DATA *pReset;
    char output[MAX_STRING_LENGTH];

    EDIT_ROOM (ch, pRoom);

    argument = one_argument (argument, arg);
    argument = one_argument (argument, arg2);

    if (arg[0] == '\0' || !is_number (arg))
    {
        sendch ("Syntax:  mreset <vnum> <max #x> <max #x in area>\n\r", ch);
        return FALSE;
    }

    if (!(pMobIndex = get_mob_index (atoi (arg))))
    {
        sendch ("REdit: No mobile has that vnum.\n\r", ch);
        return FALSE;
    }

    if (pMobIndex->area != pRoom->area)
    {
        sendch ("REdit: No such mobile in this area.\n\r", ch);
        return FALSE;
    }

    /*
     * Create the mobile reset.
     */
    pReset = new_reset_data ();
    pReset->command = 'M';
    pReset->arg1 = pMobIndex->vnum;
    pReset->arg2 = is_number (arg2) ? atoi (arg2) : MAX_MOB;
    pReset->arg3 = pRoom->vnum;
    pReset->arg4 = is_number (argument) ? atoi (argument) : 1;
    add_reset (pRoom, pReset, 0 /* Last slot */ );

    /*
     * Create the mobile.
     */
    newmob = create_mobile (pMobIndex);
    char_to_room (newmob, pRoom);

    sprintf (output, "%s (%d) has been loaded and added to resets.\n\r"
             "There will be a maximum of %d loaded to this room.\n\r",
             capitalize (pMobIndex->short_descr),
             pMobIndex->vnum, pReset->arg2);
    sendch (output, ch);
    act ("$n has created $N!", ch, NULL, newmob, TO_ROOM);
    return TRUE;
}



struct wear_type {
    int wear_loc;
    int wear_bit;
};



const struct wear_type wear_table[] = {
    {WEAR_NONE, ITEM_TAKE},
    {WEAR_LIGHT, ITEM_LIGHT},
    {WEAR_FINGER_L, ITEM_WEAR_FINGER},
    {WEAR_FINGER_R, ITEM_WEAR_FINGER},
    {WEAR_NECK_1, ITEM_WEAR_NECK},
    {WEAR_NECK_2, ITEM_WEAR_NECK},
    {WEAR_BODY, ITEM_WEAR_BODY},
    {WEAR_HEAD, ITEM_WEAR_HEAD},
    {WEAR_LEGS, ITEM_WEAR_LEGS},
    {WEAR_FEET, ITEM_WEAR_FEET},
    {WEAR_HANDS, ITEM_WEAR_HANDS},
    {WEAR_ARMS, ITEM_WEAR_ARMS},
    {WEAR_SHIELD, ITEM_WEAR_SHIELD},
    {WEAR_ABOUT, ITEM_WEAR_ABOUT},
    {WEAR_WAIST, ITEM_WEAR_WAIST},
    {WEAR_WRIST_L, ITEM_WEAR_WRIST},
    {WEAR_WRIST_R, ITEM_WEAR_WRIST},
    {WEAR_WIELD, ITEM_WIELD},
    {WEAR_HOLD, ITEM_HOLD},
    {WEAR_TAIL, ITEM_WEAR_TAIL},
    {WEAR_EYE, ITEM_WEAR_EYE},
    {WEAR_EAR_R, ITEM_WEAR_EAR},
    {WEAR_EAR_L, ITEM_WEAR_EAR},
    {NO_FLAG, NO_FLAG}
};



/*****************************************************************************
 Name:        wear_loc
 Purpose:    Returns the location of the bit that matches the count.
         1 = first match, 2 = second match etc.
 Called by:    oedit_reset(olc_act.c).
 ****************************************************************************/
int wear_loc (int bits, int count)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++)
    {
        if (IS_SET (bits, wear_table[flag].wear_bit) && --count < 1)
            return wear_table[flag].wear_loc;
    }

    return NO_FLAG;
}



/*****************************************************************************
 Name:        wear_bit
 Purpose:    Converts a wear_loc into a bit.
 Called by:    redit_oreset(olc_act.c).
 ****************************************************************************/
int wear_bit (int loc)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++)
    {
        if (loc == wear_table[flag].wear_loc)
            return wear_table[flag].wear_bit;
    }

    return 0;
}



REDIT (redit_oreset)
{
    ROOM_INDEX_DATA *pRoom;
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *newobj;
    OBJ_DATA *to_obj;
    CHAR_DATA *to_mob;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int olevel = 0;

    RESET_DATA *pReset;
    char output[MAX_STRING_LENGTH];

    EDIT_ROOM (ch, pRoom);

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || !is_number (arg1))
    {
        sendch ("Syntax:  oreset <vnum> <args>\n\r", ch);
        sendch ("        -no_args               = into room\n\r", ch);
        sendch ("        -<obj_name>            = into obj\n\r", ch);
        sendch ("        -<mob_name> <wear_loc> = into mob\n\r", ch);
        return FALSE;
    }

    if (!(pObjIndex = get_obj_index (atoi (arg1))))
    {
        sendch ("REdit: No object has that vnum.\n\r", ch);
        return FALSE;
    }

    if (pObjIndex->area != pRoom->area)
    {
        sendch ("REdit: No such object in this area.\n\r", ch);
        return FALSE;
    }

    /*
     * Load into room.
     */
    if (arg2[0] == '\0')
    {
        pReset = new_reset_data ();
        pReset->command = 'O';
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = pRoom->vnum;
        pReset->arg4 = 0;
        add_reset (pRoom, pReset, 0 /* Last slot */ );

        newobj = create_object (pObjIndex, number_fuzzy (olevel));
        obj_to_room (newobj, pRoom);

        sprintf (output, "%s (%d) has been loaded and added to resets.\n\r",
                 capitalize (pObjIndex->short_descr), pObjIndex->vnum);
        sendch (output, ch);
    }
    else
        /*
         * Load into object's inventory.
         */
        if (argument[0] == '\0'
            && ((to_obj = get_obj_list (ch, arg2, pRoom->contents)) != NULL))
    {
        pReset = new_reset_data ();
        pReset->command = 'P';
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = to_obj->pIndexData->vnum;
        pReset->arg4 = 1;
        add_reset (pRoom, pReset, 0 /* Last slot */ );

        newobj = create_object (pObjIndex, number_fuzzy (olevel));
        newobj->cost = 0;
        obj_to_obj (newobj, to_obj);

        sprintf (output, "%s (%d) has been loaded into "
                 "%s (%d) and added to resets.\n\r",
                 capitalize (newobj->short_descr),
                 newobj->pIndexData->vnum,
                 to_obj->short_descr, to_obj->pIndexData->vnum);
        sendch (output, ch);
    }
    else
        /*
         * Load into mobile's inventory.
         */
    if ((to_mob = get_char_room (ch, NULL, arg2)) != NULL)
    {
        int wear_loc;

        /*
         * Make sure the location on mobile is valid.
         */
        if ((wear_loc = flag_value (wear_loc_flags, argument)) == NO_FLAG)
        {
            sendch ("REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch);
            return FALSE;
        }

        /*
         * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
         */
        if (!IS_SET (pObjIndex->wear_flags, wear_bit (wear_loc)))
        {
            sprintf (output,
                     "%s (%d) has wear flags: [%s]\n\r",
                     capitalize (pObjIndex->short_descr),
                     pObjIndex->vnum,
                     flag_string (wear_flags, pObjIndex->wear_flags));
            sendch (output, ch);
            return FALSE;
        }

        /*
         * Can't load into same position.
         */
        if (get_eq_char (to_mob, wear_loc))
        {
            sendch ("REdit:  Object already equipped.\n\r", ch);
            return FALSE;
        }

        pReset = new_reset_data ();
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = wear_loc;
        if (pReset->arg2 == WEAR_NONE)
            pReset->command = 'G';
        else
            pReset->command = 'E';
        pReset->arg3 = wear_loc;

        add_reset (pRoom, pReset, 0 /* Last slot */ );

        olevel = URANGE (0, to_mob->level - 2, LEVEL_HERO);
        newobj = create_object (pObjIndex, number_fuzzy (olevel));

        if (to_mob->pIndexData->pShop)
        {                        /* Shop-keeper? */
            switch (pObjIndex->item_type)
            {
                default:
                    olevel = 0;
                    break;
                case ITEM_PILL:
                    olevel = number_range (0, 10);
                    break;
                case ITEM_POTION:
                    olevel = number_range (0, 10);
                    break;
                case ITEM_SCROLL:
                    olevel = number_range (5, 15);
                    break;
                case ITEM_WAND:
                    olevel = number_range (10, 20);
                    break;
                case ITEM_STAFF:
                    olevel = number_range (15, 25);
                    break;
                case ITEM_ARMOR:
                    olevel = number_range (5, 15);
                    break;
                case ITEM_WEAPON:
                    if (pReset->command == 'G')
                        olevel = number_range (5, 15);
                    else
                        olevel = number_fuzzy (olevel);
                    break;
            }

            newobj = create_object (pObjIndex, olevel);
            if (pReset->arg2 == WEAR_NONE)
                SET_BIT (newobj->extra_flags, ITEM_INVENTORY);
        }
        else
            newobj = create_object (pObjIndex, number_fuzzy (olevel));

        obj_to_char (newobj, to_mob);
        if (pReset->command == 'E')
            equip_char (to_mob, newobj, pReset->arg3);

        sprintf (output, "%s (%d) has been loaded "
                 "%s of %s (%d) and added to resets.\n\r",
                 capitalize (pObjIndex->short_descr),
                 pObjIndex->vnum,
                 flag_string (wear_loc_strings, pReset->arg3),
                 to_mob->short_descr, to_mob->pIndexData->vnum);
        sendch (output, ch);
    }
    else
    {                            /* Display Syntax */

        sendch ("REdit:  That mobile isn't here.\n\r", ch);
        return FALSE;
    }

    act ("$n has created $p!", ch, newobj, NULL, TO_ROOM);
    return TRUE;
}



/*
 * Object Editor Functions.
 */
void show_obj_values (CHAR_DATA * ch, OBJ_INDEX_DATA * obj)
{
    char buf[MAX_STRING_LENGTH];

    switch (obj->item_type)
    {
        default:                /* No values. */
            break;

        case ITEM_LIGHT:
            if (obj->value[2] == -1 || obj->value[2] == 999)    /* ROM OLC */
                sprintf (buf, "[v2] Light:  Infinite[-1]\n\r");
            else
                sprintf (buf, "[v2] Light:  [%d]\n\r", obj->value[2]);
            sendch (buf, ch);
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            sprintf (buf,
                     "[v0] Level:          [%d]\n\r"
                     "[v1] Charges Total:  [%d]\n\r"
                     "[v2] Charges Left:   [%d]\n\r"
                     "[v3] Spell:          %s\n\r",
                     obj->value[0],
                     obj->value[1],
                     obj->value[2],
                     obj->value[3] != -1 ? skill_table[obj->value[3]].name
                     : "none");
            sendch (buf, ch);
            break;

        case ITEM_PORTAL:
            sprintf (buf,
                     "[v0] Charges:        [%d]\n\r"
                     "[v1] Exit Flags:     %s\n\r"
                     "[v2] Portal Flags:   %s\n\r"
                     "[v3] Goes to (vnum): [%d]\n\r",
                     obj->value[0],
                     flag_string (exit_flags, obj->value[1]),
                     flag_string (portal_flags, obj->value[2]),
                     obj->value[3]);
            sendch (buf, ch);
            break;

        case ITEM_FURNITURE:
            sprintf (buf,
                     "[v0] Max people:      [%d]\n\r"
                     "[v1] Max weight:      [%d]\n\r"
                     "[v2] Furniture Flags: %s\n\r"
                     "[v3] Heal bonus:      [%d]\n\r"
                     "[v4] Ki bonus:      [%d]\n\r",
                     obj->value[0],
                     obj->value[1],
                     flag_string (furniture_flags, obj->value[2]),
                     obj->value[3], obj->value[4]);
            sendch (buf, ch);
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf (buf,
                     "[v0] Level:  [%d]\n\r"
                     "[v1] Spell:  %s\n\r"
                     "[v2] Spell:  %s\n\r"
                     "[v3] Spell:  %s\n\r"
                     "[v4] Spell:  %s\n\r",
                     obj->value[0],
                     obj->value[1] != -1 ? skill_table[obj->value[1]].name
                     : "none",
                     obj->value[2] != -1 ? skill_table[obj->value[2]].name
                     : "none",
                     obj->value[3] != -1 ? skill_table[obj->value[3]].name
                     : "none",
                     obj->value[4] != -1 ? skill_table[obj->value[4]].name
                     : "none");
            sendch (buf, ch);
            break;

/* ARMOR for ROM */

        case ITEM_ARMOR:
            sprintf (buf,
                     "[v0] Ac pierce       [%d]\n\r"
                     "[v1] Ac bash         [%d]\n\r"
                     "[v2] Ac slash        [%d]\n\r"
                     "[v3] Ac exotic       [%d]\n\r",
                     obj->value[0], obj->value[1], obj->value[2],
                     obj->value[3]);
            sendch (buf, ch);
            break;

/* WEAPON changed in ROM: */
/* I had to split the output here, I have no idea why, but it helped -- Hugin */
/* It somehow fixed a bug in showing scroll/pill/potions too ?! */
        case ITEM_WEAPON:
            sprintf (buf, "[v0] Weapon class:   %s\n\r",
                     flag_string (weapon_class, obj->value[0]));
            sendch (buf, ch);
            sprintf (buf, "[v1] Number of dice: [%d]\n\r", obj->value[1]);
            sendch (buf, ch);
            sprintf (buf, "[v2] Type of dice:   [%d]\n\r", obj->value[2]);
            sendch (buf, ch);
            sprintf (buf, "[v3] Type:           %s\n\r",
                     attack_table[obj->value[3]].name);
            sendch (buf, ch);
            sprintf (buf, "[v4] Special type:   %s\n\r",
                     flag_string (weapon_type2, obj->value[4]));
            sendch (buf, ch);
            break;

        case ITEM_CONTAINER:
            sprintf (buf,
                     "[v0] Weight:     [%d kg]\n\r"
                     "[v1] Flags:      [%s]\n\r"
                     "[v2] Key:     %s [%d]\n\r"
                     "[v3] Capacity    [%d]\n\r"
                     "[v4] Weight Mult [%d]\n\r",
                     obj->value[0],
                     flag_string (container_flags, obj->value[1]),
                     get_obj_index (obj->value[2])
                     ? get_obj_index (obj->value[2])->short_descr
                     : "none", obj->value[2], obj->value[3], obj->value[4]);
            sendch (buf, ch);
            break;

        case ITEM_DRINK_CON:
            sprintf (buf,
                     "[v0] Liquid Total: [%d]\n\r"
                     "[v1] Liquid Left:  [%d]\n\r"
                     "[v2] Liquid:       %s\n\r"
                     "[v3] Poisoned:     %s\n\r",
                     obj->value[0],
                     obj->value[1],
                     liq_table[obj->value[2]].liq_name,
                     obj->value[3] != 0 ? "Yes" : "No");
            sendch (buf, ch);
            break;

        case ITEM_FOUNTAIN:
            sprintf (buf,
                     "[v0] Liquid Total: [%d]\n\r"
                     "[v1] Liquid Left:  [%d]\n\r"
                     "[v2] Liquid:        %s\n\r",
                     obj->value[0],
                     obj->value[1], liq_table[obj->value[2]].liq_name);
            sendch (buf, ch);
            break;

        case ITEM_FOOD:
            sprintf (buf,
                     "[v0] Food hours: [%d]\n\r"
                     "[v1] Full hours: [%d]\n\r"
                     "[v3] Poisoned:   %s\n\r"
					 "[v4] Regen:      [%d]\n\r",
                     obj->value[0],
                     obj->value[1], obj->value[3] != 0 ? "Yes" : "No",
					 obj->value[4]);
            sendch (buf, ch);
            break;

        case ITEM_MONEY:
            sprintf (buf, "[v0] Zenni:   [%d]\n\r", obj->value[0]);
            sendch (buf, ch);
            break;
    }

    return;
}



bool set_obj_values (CHAR_DATA * ch, OBJ_INDEX_DATA * pObj, int value_num,
                     char *argument)
{
    switch (pObj->item_type)
    {
        default:
            break;

        case ITEM_LIGHT:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_LIGHT");
                    return FALSE;
                case 2:
                    sendch ("HOURS OF LIGHT SET.\n\r\n\r", ch);
                    pObj->value[2] = atoi (argument);
                    break;
            }
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_STAFF_WAND");
                    return FALSE;
                case 0:
                    sendch ("SPELL LEVEL SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch ("TOTAL NUMBER OF CHARGES SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 2:
                    sendch ("CURRENT NUMBER OF CHARGES SET.\n\r\n\r",
                                  ch);
                    pObj->value[2] = atoi (argument);
                    break;
                case 3:
                    sendch ("SPELL TYPE SET.\n\r", ch);
                    pObj->value[3] = skill_lookup (argument);
                    break;
            }
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_SCROLL_POTION_PILL");
                    return FALSE;
                case 0:
                    sendch ("SPELL LEVEL SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch ("SPELL TYPE 1 SET.\n\r\n\r", ch);
                    pObj->value[1] = skill_lookup (argument);
                    break;
                case 2:
                    sendch ("SPELL TYPE 2 SET.\n\r\n\r", ch);
                    pObj->value[2] = skill_lookup (argument);
                    break;
                case 3:
                    sendch ("SPELL TYPE 3 SET.\n\r\n\r", ch);
                    pObj->value[3] = skill_lookup (argument);
                    break;
                case 4:
                    sendch ("SPELL TYPE 4 SET.\n\r\n\r", ch);
                    pObj->value[4] = skill_lookup (argument);
                    break;
            }
            break;

/* ARMOR for ROM: */

        case ITEM_ARMOR:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_ARMOR");
                    return FALSE;
                case 0:
                    sendch ("AC PIERCE SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch ("AC BASH SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 2:
                    sendch ("AC SLASH SET.\n\r\n\r", ch);
                    pObj->value[2] = atoi (argument);
                    break;
                case 3:
                    sendch ("AC EXOTIC SET.\n\r\n\r", ch);
                    pObj->value[3] = atoi (argument);
                    break;
            }
            break;

/* WEAPONS changed in ROM */

        case ITEM_WEAPON:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_WEAPON");
                    return FALSE;
                case 0:
                    sendch ("WEAPON CLASS SET.\n\r\n\r", ch);
                    ALT_FLAGVALUE_SET (pObj->value[0], weapon_class,
                                       argument);
                    break;
                case 1:
                    sendch ("NUMBER OF DICE SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 2:
                    sendch ("TYPE OF DICE SET.\n\r\n\r", ch);
                    pObj->value[2] = atoi (argument);
                    break;
                case 3:
                    sendch ("WEAPON TYPE SET.\n\r\n\r", ch);
                    pObj->value[3] = attack_lookup (argument);
                    break;
                case 4:
                    sendch ("SPECIAL WEAPON TYPE TOGGLED.\n\r\n\r", ch);
                    ALT_FLAGVALUE_TOGGLE (pObj->value[4], weapon_type2,
                                          argument);
                    break;
            }
            break;

        case ITEM_PORTAL:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_PORTAL");
                    return FALSE;

                case 0:
                    sendch ("CHARGES SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch ("EXIT FLAGS SET.\n\r\n\r", ch);
                    ALT_FLAGVALUE_SET (pObj->value[1], exit_flags, argument);
                    break;
                case 2:
                    sendch ("PORTAL FLAGS SET.\n\r\n\r", ch);
                    ALT_FLAGVALUE_SET (pObj->value[2], portal_flags,
                                       argument);
                    break;
                case 3:
                    sendch ("EXIT VNUM SET.\n\r\n\r", ch);
                    pObj->value[3] = atoi (argument);
                    break;
            }
            break;

        case ITEM_FURNITURE:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_FURNITURE");
                    return FALSE;

                case 0:
                    sendch ("NUMBER OF PEOPLE SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch ("MAX WEIGHT SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 2:
                    sendch ("FURNITURE FLAGS TOGGLED.\n\r\n\r", ch);
                    ALT_FLAGVALUE_TOGGLE (pObj->value[2], furniture_flags,
                                          argument);
                    break;
                case 3:
                    sendch ("HEAL BONUS SET.\n\r\n\r", ch);
                    pObj->value[3] = atoi (argument);
                    break;
                case 4:
                    sendch ("KI BONUS SET.\n\r\n\r", ch);
                    pObj->value[4] = atoi (argument);
                    break;
            }
            break;

        case ITEM_CONTAINER:
            switch (value_num)
            {
                    int value;

                default:
                    do_help (ch, "ITEM_CONTAINER");
                    return FALSE;
                case 0:
                    sendch ("WEIGHT CAPACITY SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    if ((value = flag_value (container_flags, argument)) !=
                        NO_FLAG)
                        TOGGLE_BIT (pObj->value[1], value);
                    else
                    {
                        do_help (ch, "ITEM_CONTAINER");
                        return FALSE;
                    }
                    sendch ("CONTAINER TYPE SET.\n\r\n\r", ch);
                    break;
                case 2:
                    if (atoi (argument) != 0)
                    {
                        if (!get_obj_index (atoi (argument)))
                        {
                            sendch ("THERE IS NO SUCH ITEM.\n\r\n\r",
                                          ch);
                            return FALSE;
                        }

                        if (get_obj_index (atoi (argument))->item_type !=
                            ITEM_KEY)
                        {
                            sendch ("THAT ITEM IS NOT A KEY.\n\r\n\r",
                                          ch);
                            return FALSE;
                        }
                    }
                    sendch ("CONTAINER KEY SET.\n\r\n\r", ch);
                    pObj->value[2] = atoi (argument);
                    break;
                case 3:
                    sendch ("CONTAINER MAX WEIGHT SET.\n\r", ch);
                    pObj->value[3] = atoi (argument);
                    break;
                case 4:
                    sendch ("WEIGHT MULTIPLIER SET.\n\r\n\r", ch);
                    pObj->value[4] = atoi (argument);
                    break;
            }
            break;

        case ITEM_DRINK_CON:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_DRINK");
/* OLC            do_help( ch, "liquids" );    */
                    return FALSE;
                case 0:
                    sendch
                        ("MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch
                        ("CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 2:
                    sendch ("LIQUID TYPE SET.\n\r\n\r", ch);
                    pObj->value[2] = (liq_lookup (argument) != -1 ?
                                      liq_lookup (argument) : 0);
                    break;
                case 3:
                    sendch ("POISON VALUE TOGGLED.\n\r\n\r", ch);
                    pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
                    break;
            }
            break;

        case ITEM_FOUNTAIN:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_FOUNTAIN");
/* OLC            do_help( ch, "liquids" );    */
                    return FALSE;
                case 0:
                    sendch
                        ("MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch
                        ("CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 2:
                    sendch ("LIQUID TYPE SET.\n\r\n\r", ch);
                    pObj->value[2] = (liq_lookup (argument) != -1 ?
                                      liq_lookup (argument) : 0);
                    break;
            }
            break;

        case ITEM_FOOD:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_FOOD");
                    return FALSE;
                case 0:
                    sendch ("HOURS OF FOOD SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
                case 1:
                    sendch ("HOURS OF FULL SET.\n\r\n\r", ch);
                    pObj->value[1] = atoi (argument);
                    break;
                case 3:
                    sendch ("POISON VALUE TOGGLED.\n\r\n\r", ch);
                    pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
                    break;
                case 4:
                    sendch ("REGEN VALUE SET.\n\r\n\r", ch);
                    pObj->value[4] = atoi (argument);
                    break;
            }
            break;

        case ITEM_MONEY:
            switch (value_num)
            {
                default:
                    do_help (ch, "ITEM_MONEY");
                    return FALSE;
                case 0:
                    sendch ("ZENNI AMOUNT SET.\n\r\n\r", ch);
                    pObj->value[0] = atoi (argument);
                    break;
            }
            break;
    }

    show_obj_values (ch, pObj);

    return TRUE;
}



OEDIT (oedit_show)
{
    OBJ_INDEX_DATA *pObj;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
	PROG_LIST			*list;
    int cnt;

    EDIT_OBJ (ch, pObj);

    sprintf (buf, "Name:        [%s]\n\rArea:        [%5d] %s\n\r",
             pObj->name,
             !pObj->area ? -1 : pObj->area->vnum,
             !pObj->area ? "No Area" : pObj->area->name);
    sendch (buf, ch);


    sprintf (buf, "Vnum:        [%5d]\n\rType:        [%s]\n\r",
             pObj->vnum, flag_string (type_flags, pObj->item_type));
    sendch (buf, ch);

    sprintf (buf, "PL:          [%14Ld]\n\r", pObj->llPl);
    sendch (buf, ch);

    sprintf (buf, "Wear flags:  [%s]\n\r",
             flag_string (wear_flags, pObj->wear_flags));
    sendch (buf, ch);

    sprintf (buf, "Extra flags: [%s]\n\r",
             flag_string (extra_flags, pObj->extra_flags));
    sendch (buf, ch);

    sprintf (buf, "Material:    [%s]\n\r",    /* ROM */
             pObj->material);
    sendch (buf, ch);

	sprintf (buf, "Durability:  [%5d]\n\r",
		     pObj->durability);
	sendch (buf, ch);

    sprintf (buf, "Condition:   [%5d]\n\r",    /* ROM */
             pObj->condition);
    sendch (buf, ch);

    sprintf (buf, "Weight:      [%5d]\n\rCost:        [%5d]\n\r",
             pObj->weight, pObj->cost);
    sendch (buf, ch);

    if (pObj->extra_descr)
    {
        EXTRA_DESCR_DATA *ed;

        sendch ("Ex desc kwd: ", ch);

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            sendch ("[", ch);
            sendch (ed->keyword, ch);
            sendch ("]", ch);
        }

        sendch ("\n\r", ch);
    }

    sprintf (buf, "Short desc:  %s\n\rLong desc:\n\r     %s\n\r",
             pObj->short_descr, pObj->description);
    sendch (buf, ch);

    for (cnt = 0, paf = pObj->affected; paf; paf = paf->next)
    {
        if (cnt == 0)
        {
            sendch ("Number Modifier       Affects\n\r", ch);
            sendch ("------ -------------- -------\n\r", ch);
        }
        sprintf (buf, "[%6d] %-14Ld %s\n\r", cnt,
                 paf->modifier, flag_string (apply_flags, paf->location));
        sendch (buf, ch);
        cnt++;
    }

    show_obj_values (ch, pObj);

    if ( pObj->oprogs )
    {
		int cnt;

		sprintf(buf, "\n\rOBJPrograms for [%5d]:\n\r", pObj->vnum);
		sendch( buf, ch );

		for (cnt=0, list=pObj->oprogs; list; list=list->next)
		{
			if (cnt ==0)
			{
				sendch ( " Number Vnum Trigger Phrase\n\r", ch );
				sendch ( " ------ ---- ------- ------\n\r", ch );
			}

			sprintf(buf, "[%5d] %4d %7s %s\n\r", cnt,
				list->vnum,prog_type_to_name(list->trig_type),
				list->trig_phrase);
			sendch( buf, ch );
			cnt++;
		}
    }

    return FALSE;
}


/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
OEDIT (oedit_addaffect)
{
    int value;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];

    EDIT_OBJ (ch, pObj);

    argument = one_argument (argument, loc);
    one_argument (argument, mod);

    if (loc[0] == '\0' || mod[0] == '\0' || !is_number (mod))
    {
        sendch ("Syntax:  addaffect [location] [#xmod]\n\r", ch);
        return FALSE;
    }

    if ((value = flag_value (apply_flags, loc)) == NO_FLAG)
    {                            /* Hugin */
        sendch ("Valid affects are:\n\r", ch);
        show_help (ch, "apply");
        return FALSE;
    }

    pAf = new_affect ();
    pAf->location = value;
    pAf->modifier = atoi (mod);
    pAf->where = TO_OBJECT;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = 0;
	pAf->skill_lvl = PL_TO_SKILL(pObj->llPl);
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    sendch ("Affect added.\n\r", ch);
    return TRUE;
}

OEDIT (oedit_addapply)
{
    int value, bv, typ;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    char type[MAX_STRING_LENGTH];
    char bvector[MAX_STRING_LENGTH];

    EDIT_OBJ (ch, pObj);

    argument = one_argument (argument, type);
    argument = one_argument (argument, loc);
    argument = one_argument (argument, mod);
    one_argument (argument, bvector);

    if (type[0] == '\0' || (typ = flag_value (apply_types, type)) == NO_FLAG)
    {
        sendch ("Invalid apply type. Valid apply types are:\n\r", ch);
        show_help (ch, "apptype");
        return FALSE;
    }

    if (loc[0] == '\0' || (value = flag_value (apply_flags, loc)) == NO_FLAG)
    {
        sendch ("Valid applys are:\n\r", ch);
        show_help (ch, "apply");
        return FALSE;
    }

    if (bvector[0] == '\0'
        || (bv = flag_value (bitvector_type[typ].table, bvector)) == NO_FLAG)
    {
        sendch ("Invalid bitvector type.\n\r", ch);
        sendch ("Valid bitvector types are:\n\r", ch);
        show_help (ch, bitvector_type[typ].help);
        return FALSE;
    }

    if (mod[0] == '\0' || !is_number (mod))
    {
        sendch
            ("Syntax:  addapply [type] [location] [#xmod] [bitvector]\n\r",
             ch);
        return FALSE;
    }

    pAf = new_affect ();
    pAf->location = value;
    pAf->modifier = atoi (mod);
    pAf->where = apply_types[typ].bit;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = bv;
    pAf->skill_lvl = PL_TO_SKILL(pObj->llPl);
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    sendch ("Apply added.\n\r", ch);
    return TRUE;
}

/*
 * My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
 * for really teaching me how to manipulate pointers.
 */
OEDIT (oedit_delaffect)
{
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    AFFECT_DATA *pAf_next;
    char affect[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_OBJ (ch, pObj);

    one_argument (argument, affect);

    if (!is_number (affect) || affect[0] == '\0')
    {
        sendch ("Syntax:  delaffect [#xaffect]\n\r", ch);
        return FALSE;
    }

    value = atoi (affect);

    if (value < 0)
    {
        sendch ("Only non-negative affect-numbers allowed.\n\r", ch);
        return FALSE;
    }

    if (!(pAf = pObj->affected))
    {
        sendch ("OEdit:  Non-existant affect.\n\r", ch);
        return FALSE;
    }

    if (value == 0)
    {                            /* First case: Remove first affect */
        pAf = pObj->affected;
        pObj->affected = pAf->next;
        free_affect (pAf);
    }
    else
    {                            /* Affect to remove is not the first */

        while ((pAf_next = pAf->next) && (++cnt < value))
            pAf = pAf_next;

        if (pAf_next)
        {                        /* See if it's the next affect */
            pAf->next = pAf_next->next;
            free_affect (pAf_next);
        }
        else
        {                        /* Doesn't exist */
            sendch ("No such affect.\n\r", ch);
            return FALSE;
        }
    }

    sendch ("Affect removed.\n\r", ch);
    return TRUE;
}



OEDIT (oedit_name)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  name [string]\n\r", ch);
        return FALSE;
    }

    free_string (pObj->name);
    pObj->name = str_dup (argument);

    sendch ("Name set.\n\r", ch);
    return TRUE;
}



OEDIT (oedit_short)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  short [string]\n\r", ch);
        return FALSE;
    }

    free_string (pObj->short_descr);
    pObj->short_descr = str_dup (argument);
    pObj->short_descr[0] = LOWER (pObj->short_descr[0]);

    sendch ("Short description set.\n\r", ch);
    return TRUE;
}



OEDIT (oedit_long)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  long [string]\n\r", ch);
        return FALSE;
    }

    free_string (pObj->description);
    pObj->description = str_dup (argument);
    pObj->description[0] = UPPER (pObj->description[0]);

    sendch ("Long description set.\n\r", ch);
    return TRUE;
}



bool set_value (CHAR_DATA * ch, OBJ_INDEX_DATA * pObj, char *argument,
                int value)
{
    if (argument[0] == '\0')
    {
        set_obj_values (ch, pObj, -1, "");    /* '\0' changed to "" -- Hugin */
        return FALSE;
    }

    if (set_obj_values (ch, pObj, value, argument))
        return TRUE;

    return FALSE;
}



/*****************************************************************************
 Name:        oedit_values
 Purpose:    Finds the object and sets its value.
 Called by:    The four valueX functions below. (now five -- Hugin )
 ****************************************************************************/
bool oedit_values (CHAR_DATA * ch, char *argument, int value)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (set_value (ch, pObj, argument, value))
        return TRUE;

    return FALSE;
}


OEDIT (oedit_value0)
{
    if (oedit_values (ch, argument, 0))
        return TRUE;

    return FALSE;
}



OEDIT (oedit_value1)
{
    if (oedit_values (ch, argument, 1))
        return TRUE;

    return FALSE;
}



OEDIT (oedit_value2)
{
    if (oedit_values (ch, argument, 2))
        return TRUE;

    return FALSE;
}



OEDIT (oedit_value3)
{
    if (oedit_values (ch, argument, 3))
        return TRUE;

    return FALSE;
}



OEDIT (oedit_value4)
{
    if (oedit_values (ch, argument, 4))
        return TRUE;

    return FALSE;
}



OEDIT (oedit_weight)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  weight [number]\n\r", ch);
        return FALSE;
    }

    pObj->weight = atoi (argument);

    sendch ("Weight set.\n\r", ch);
    return TRUE;
}

OEDIT (oedit_cost)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  cost [number]\n\r", ch);
        return FALSE;
    }

    pObj->cost = atoi (argument);

    sendch ("Cost set.\n\r", ch);
    return TRUE;
}



OEDIT (oedit_create)
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    int value;
    int iHash;

    value = atoi (argument);
    if (argument[0] == '\0' || value == 0)
    {
        sendch ("Syntax:  oedit create [vnum]\n\r", ch);
        return FALSE;
    }

    pArea = get_vnum_area (value);
    if (!pArea)
    {
        sendch ("OEdit:  That vnum is not assigned an area.\n\r", ch);
        return FALSE;
    }

    if (!IS_BUILDER (ch, pArea))
    {
        sendch ("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return FALSE;
    }

    if (get_obj_index (value))
    {
        sendch ("OEdit:  Object vnum already exists.\n\r", ch);
        return FALSE;
    }

    pObj = new_obj_index ();
    pObj->vnum = value;
    pObj->area = pArea;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    iHash = value % MAX_KEY_HASH;
    pObj->next = obj_index_hash[iHash];
    obj_index_hash[iHash] = pObj;
    ch->desc->pEdit = (void *) pObj;

    sendch ("Object Created.\n\r", ch);
    return TRUE;
}



OEDIT (oedit_ed)
{
    OBJ_INDEX_DATA *pObj;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_OBJ (ch, pObj);

    argument = one_argument (argument, command);
    one_argument (argument, keyword);

    if (command[0] == '\0')
    {
        sendch ("Syntax:  ed add [keyword]\n\r", ch);
        sendch ("         ed delete [keyword]\n\r", ch);
        sendch ("         ed edit [keyword]\n\r", ch);
        sendch ("         ed format [keyword]\n\r", ch);
        return FALSE;
    }

    if (!str_cmp (command, "add"))
    {
        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed add [keyword]\n\r", ch);
            return FALSE;
        }

        ed = new_extra_descr ();
        ed->keyword = str_dup (keyword);
        ed->next = pObj->extra_descr;
        pObj->extra_descr = ed;

        string_append (ch, &ed->description);

        return TRUE;
    }

    if (!str_cmp (command, "edit"))
    {
        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed edit [keyword]\n\r", ch);
            return FALSE;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            if (is_name (keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            sendch ("OEdit:  Extra description keyword not found.\n\r",
                          ch);
            return FALSE;
        }

        string_append (ch, &ed->description);

        return TRUE;
    }

    if (!str_cmp (command, "delete"))
    {
        EXTRA_DESCR_DATA *ped = NULL;

        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed delete [keyword]\n\r", ch);
            return FALSE;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            if (is_name (keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed)
        {
            sendch ("OEdit:  Extra description keyword not found.\n\r",
                          ch);
            return FALSE;
        }

        if (!ped)
            pObj->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr (ed);

        sendch ("Extra description deleted.\n\r", ch);
        return TRUE;
    }


    if (!str_cmp (command, "format"))
    {
        EXTRA_DESCR_DATA *ped = NULL;

        if (keyword[0] == '\0')
        {
            sendch ("Syntax:  ed format [keyword]\n\r", ch);
            return FALSE;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            if (is_name (keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed)
        {
            sendch ("OEdit:  Extra description keyword not found.\n\r",
                          ch);
            return FALSE;
        }

        ed->description = format_string (ed->description);

        sendch ("Extra description formatted.\n\r", ch);
        return TRUE;
    }

    oedit_ed (ch, "");
    return FALSE;
}





/* ROM object functions : */

OEDIT (oedit_extra)
{                                /* Moved out of oedit() due to naming conflicts -- Hugin */
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_OBJ (ch, pObj);

        if ((value = flag_value (extra_flags, argument)) != NO_FLAG)
        {
            TOGGLE_BIT (pObj->extra_flags, value);

            sendch ("Extra flag toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax:  extra [flag]\n\r"
                  "Type '? extra' for a list of flags.\n\r", ch);
    return FALSE;
}


OEDIT (oedit_wear)
{                                /* Moved out of oedit() due to naming conflicts -- Hugin */
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_OBJ (ch, pObj);

        if ((value = flag_value (wear_flags, argument)) != NO_FLAG)
        {
            TOGGLE_BIT (pObj->wear_flags, value);

            sendch ("Wear flag toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax:  wear [flag]\n\r"
                  "Type '? wear' for a list of flags.\n\r", ch);
    return FALSE;
}


OEDIT (oedit_type)
{                                /* Moved out of oedit() due to naming conflicts -- Hugin */
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_OBJ (ch, pObj);

        if ((value = flag_value (type_flags, argument)) != NO_FLAG)
        {
            pObj->item_type = value;

            sendch ("Type set.\n\r", ch);

            /*
             * Clear the values.
             */
            pObj->value[0] = 0;
            pObj->value[1] = 0;
            pObj->value[2] = 0;
            pObj->value[3] = 0;
            pObj->value[4] = 0;    /* ROM */

            return TRUE;
        }
    }

    sendch ("Syntax:  type [flag]\n\r"
                  "Type '? type' for a list of flags.\n\r", ch);
    return FALSE;
}

OEDIT (oedit_material)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  material [string]\n\r", ch);
        return FALSE;
    }

    free_string (pObj->material);
    pObj->material = str_dup (argument);

    sendch ("Material set.\n\r", ch);
    return TRUE;
}

OEDIT (oedit_durability)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  durability [number]\n\r", ch);
        return FALSE;
    }

    pObj->durability = atoi (argument);

    sendch ("Durability set.\n\r", ch);
    return TRUE;
}

OEDIT (oedit_pl)
{
    OBJ_INDEX_DATA *pObj;
	char buf[MAX_STRING_LENGTH];

    EDIT_OBJ (ch, pObj);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  pl [number]\n\r", ch);
        return FALSE;
    }

	if (strtoll(argument, NULL, 10) < 1 || strtoll(argument, NULL, 10) > MAX_PL)
    {
        sprintf(buf, "Power level must be between 0 to %Ld.", MAX_PL);
		sendch(buf, ch);
        return FALSE;
    }

    pObj->llPl = strtoll(argument, NULL, 10);

    sendch ("Power level set.\n\r", ch);
    return TRUE;
}



OEDIT (oedit_condition)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0'
        && (value = atoi (argument)) >= 0 && (value <= 100))
    {
        EDIT_OBJ (ch, pObj);

        pObj->condition = value;
        sendch ("Condition set.\n\r", ch);

        return TRUE;
    }

    sendch ("Syntax:  condition [number]\n\r"
                  "Where number can range from 0 (ruined) to 100 (perfect).\n\r",
                  ch);
    return FALSE;
}





/*
 * Mobile Editor Functions.
 */
MEDIT (medit_show)
{
    MOB_INDEX_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    PROG_LIST *list;

    EDIT_MOB (ch, pMob);

    sprintf (buf, "Name:        [%s]\n\rArea:        [%5d] %s\n\r",
             pMob->player_name,
             !pMob->area ? -1 : pMob->area->vnum,
             !pMob->area ? "No Area" : pMob->area->name);
    sendch (buf, ch);

    sprintf (buf, "Act:         [%s]\n\r",
             flag_string (act_flags, pMob->act));
    sendch (buf, ch);

    sprintf (buf, "Vnum:        [%5d] Sex:   [%s]   Race: [%s]\n\r",
             pMob->vnum,
             pMob->sex == SEX_MALE ? "male   " :
             pMob->sex == SEX_FEMALE ? "female " :
             pMob->sex == 3 ? "random " : "neutral",
             race_table[pMob->race].name);
    sendch (buf, ch);

    sprintf (buf,
             "Level:       [%2d]    Align: [%4d]      Hitroll: [%2d] Dam Type:    [%s]\n\r",
             pMob->level, pMob->alignment,
             pMob->hitroll, attack_table[pMob->dam_type].name);
    sendch (buf, ch);

	sprintf (buf,"Stats:       [Str:%d  Int:%d  Wil:%d  Dex:%d  Cha:%d]\n\r",
		 pMob->stat[STAT_STR], pMob->stat[STAT_INT], pMob->stat[STAT_WIL], pMob->stat[STAT_DEX], pMob->stat[STAT_CHA]);
	sendch (buf, ch);

	sprintf (buf,"Hitbonus:    [%3d%%]       Kibonus: [%3d%%]\n\r",
		pMob->hit_bonus, pMob->ki_bonus);
	sendch (buf, ch);

    if (pMob->group)
    {
        sprintf (buf, "Group:       [%5d]\n\r", pMob->group);
        sendch (buf, ch);
    }

/* ROM values end */

    sprintf (buf, "Affected by: [%s]\n\r",
             flag_string (affect_flags, pMob->affected_by));
    sendch (buf, ch);

/* ROM values: */

    sprintf (buf,
             "Armor:       [pierce: %d  bash: %d  slash: %d  magic: %d]\n\r",
             pMob->ac[AC_PIERCE], pMob->ac[AC_BASH], pMob->ac[AC_SLASH],
             pMob->ac[AC_EXOTIC]);
    sendch (buf, ch);

    sprintf (buf, "Form:        [%s]\n\r",
             flag_string (form_flags, pMob->form));
    sendch (buf, ch);

    sprintf (buf, "Parts:       [%s]\n\r",
             flag_string (part_flags, pMob->parts));
    sendch (buf, ch);

    sprintf (buf, "Imm:         [%s]\n\r",
             flag_string (imm_flags, pMob->imm_flags));
    sendch (buf, ch);

    sprintf (buf, "Res:         [%s]\n\r",
             flag_string (res_flags, pMob->res_flags));
    sendch (buf, ch);

    sprintf (buf, "Vuln:        [%s]\n\r",
             flag_string (vuln_flags, pMob->vuln_flags));
    sendch (buf, ch);

    sprintf (buf, "Off:         [%s]\n\r",
             flag_string (off_flags, pMob->off_flags));
    sendch (buf, ch);

    sprintf (buf, "Size:        [%s]\n\r",
             flag_string (size_flags, pMob->size));
    sendch (buf, ch);

    sprintf (buf, "Material:    [%s]\n\r", pMob->material);
    sendch (buf, ch);

    sprintf (buf, "Start pos.   [%s]\n\r",
             flag_string (position_flags, pMob->start_pos));
    sendch (buf, ch);

    sprintf (buf, "Default pos  [%s]\n\r",
             flag_string (position_flags, pMob->default_pos));
    sendch (buf, ch);

    sprintf (buf, "Wealth:      [%5ld]\n\r", pMob->wealth);
    sendch (buf, ch);

/* ROM values end */

    if (pMob->spec_fun)
    {
        sprintf (buf, "Spec fun:    [%s]\n\r", spec_name (pMob->spec_fun));
        sendch (buf, ch);
    }

    sprintf (buf, "Short descr: %s\n\rLong descr:\n\r%s",
             pMob->short_descr, pMob->long_descr);
    sendch (buf, ch);

    sprintf (buf, "Description:\n\r%s", pMob->description);
    sendch (buf, ch);

    if (pMob->pShop)
    {
        SHOP_DATA *pShop;
        int iTrade;

        pShop = pMob->pShop;

        sprintf (buf,
                 "Shop data for [%5d]:\n\r"
                 "  Markup for purchaser: %d%%\n\r"
                 "  Markdown for seller:  %d%%\n\r",
                 pShop->keeper, pShop->profit_buy, pShop->profit_sell);
        sendch (buf, ch);
        sprintf (buf, "  Hours: %d to %d.\n\r",
                 pShop->open_hour, pShop->close_hour);
        sendch (buf, ch);

        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
        {
            if (pShop->buy_type[iTrade] != 0)
            {
                if (iTrade == 0)
                {
                    sendch ("  Number Trades Type\n\r", ch);
                    sendch ("  ------ -----------\n\r", ch);
                }
                sprintf (buf, "  [%4d] %s\n\r", iTrade,
                         flag_string (type_flags, pShop->buy_type[iTrade]));
                sendch (buf, ch);
            }
        }
    }

    if (pMob->mprogs)
    {
        int cnt;

        sprintf (buf, "\n\rMOBPrograms for [%5d]:\n\r", pMob->vnum);
        sendch (buf, ch);

        for (cnt = 0, list = pMob->mprogs; list; list = list->next)
        {
            if (cnt == 0)
            {
                sendch (" Number Vnum Trigger Phrase\n\r", ch);
                sendch (" ------ ---- ------- ------\n\r", ch);
            }

            sprintf (buf, "[%5d] %4d %7s %s\n\r", cnt,
                     list->vnum, prog_type_to_name (list->trig_type),
                     list->trig_phrase);
            sendch (buf, ch);
            cnt++;
        }
    }

    return FALSE;
}



MEDIT (medit_create)
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int value;
    int iHash;

    value = atoi (argument);
    if (argument[0] == '\0' || value == 0)
    {
        sendch ("Syntax:  medit create [vnum]\n\r", ch);
        return FALSE;
    }

    pArea = get_vnum_area (value);

    if (!pArea)
    {
        sendch ("MEdit:  That vnum is not assigned an area.\n\r", ch);
        return FALSE;
    }

    if (!IS_BUILDER (ch, pArea))
    {
        sendch ("MEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return FALSE;
    }

    if (get_mob_index (value))
    {
        sendch ("MEdit:  Mobile vnum already exists.\n\r", ch);
        return FALSE;
    }

    pMob = new_mob_index ();
    pMob->vnum = value;
    pMob->area = pArea;

    if (value > top_vnum_mob)
        top_vnum_mob = value;

    pMob->act = ACT_IS_NPC;
    iHash = value % MAX_KEY_HASH;
    pMob->next = mob_index_hash[iHash];
    mob_index_hash[iHash] = pMob;
    ch->desc->pEdit = (void *) pMob;

    sendch ("Mobile Created.\n\r", ch);
    return TRUE;
}


MEDIT (medit_autoset) {
    MOB_INDEX_DATA *pMob;
    long long int llPl;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int diff; // difficulty: -1 = easy, 0 = normal, 1 = hard
    int i;

    EDIT_MOB (ch, pMob);
    
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    
    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number (arg2)) {
        sendch ("Syntax:  autoset [easy/normal/hard] [powerlevel]\n\r", ch);
        return FALSE;
    }

    if (!str_cmp(arg1, "easy"))
        diff = -1;
    else if (!str_cmp(arg1, "normal"))
        diff = 0;
    else if (!str_cmp(arg1, "hard"))
        diff = 1;        
    else {
        sendch ("Syntax:  autoset [easy/normal/hard] [powerlevel]\n\r", ch);
        return FALSE;
    }       
    
    llPl = strtoll(arg2, NULL, 10);
    
    // Wealth 
    if (llPl <= 1000)
        pMob->wealth = 5 + 5 * diff;
    else if (llPl <= 5000)
        pMob->wealth = 20 + 10 * diff;
    else if (llPl <= 10000)
        pMob->wealth = 40 + 10 * diff;        
    else if (llPl <= 50000)
        pMob->wealth = 55 + 5 * diff;
    else if (llPl <= 100000)
        pMob->wealth = 65 + 5 * diff;
    else if (llPl <= 500000)
        pMob->wealth = 75 + 5 * diff;        
    else if (llPl <= 1000000)
        pMob->wealth = 85 + 5 * diff;
    else
        pMob->wealth = 100 + 5 * diff;

    // Stats
    for (i = 0; i < MAX_STATS; ++i) {
        pMob->stat[i] = PL_TO_STAT(llPl);
        pMob->stat[i] += (pMob->stat[i] / 5) * diff;        
        pMob->stat[i] = URANGE(0, pMob->stat[i], 1000);
    }
        
    // AC
    for (i = 0; i < 4; ++i)
        pMob->ac[i] = sqrt(llPl) / (4 + -1 * diff);

    // Damage Type
    pMob->dam_type = attack_lookup ("punch");
    
    // Hitroll
    pMob->hitroll = sqrt(llPl) / (4 + -1 * diff);
                            
    sprintf (buf, "Autoset: %Ld powerlevel, %s. Check for accuracy\n\r",
		llPl, diff == -1 ? "easy" : (diff == 0 ? "normal" : "hard"));
    sendch (buf, ch);
    return TRUE;    
}

MEDIT (medit_spec)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  spec [special function]\n\r", ch);
        return FALSE;
    }


    if (!str_cmp (argument, "none"))
    {
        pMob->spec_fun = NULL;

        sendch ("Spec removed.\n\r", ch);
        return TRUE;
    }

    if (spec_lookup (argument))
    {
        pMob->spec_fun = spec_lookup (argument);
        sendch ("Spec set.\n\r", ch);
        return TRUE;
    }

    sendch ("MEdit: No such special function.\n\r", ch);
    return FALSE;
}

MEDIT (medit_damtype)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  damtype [damage message]\n\r", ch);
        sendch
            ("For a list of damage types, type '? weapon'.\n\r",
             ch);
        return FALSE;
    }

    pMob->dam_type = attack_lookup (argument);
    sendch ("Damage type set.\n\r", ch);
    return TRUE;
}


MEDIT (medit_align)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  alignment [number]\n\r", ch);
        return FALSE;
    }

    pMob->alignment = atoi (argument);

    sendch ("Alignment set.\n\r", ch);
    return TRUE;
}



MEDIT (medit_level)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  level [number]\n\r", ch);
        return FALSE;
    }

    pMob->level = atoi (argument);

    sendch ("Level set.\n\r", ch);
    return TRUE;
}


MEDIT (medit_desc)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        string_append (ch, &pMob->description);
        return TRUE;
    }

    sendch ("Syntax:  desc    - line edit\n\r", ch);
    return FALSE;
}




MEDIT (medit_long)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  long [string]\n\r", ch);
        return FALSE;
    }

    free_string (pMob->long_descr);
	strcat(argument, "\n\r");
    pMob->long_descr = str_dup (argument);
    pMob->long_descr[0] = UPPER (pMob->long_descr[0]);

    sendch ("Long description set.\n\r", ch);
    return TRUE;
}



MEDIT (medit_short)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  short [string]\n\r", ch);
        return FALSE;
    }

    free_string (pMob->short_descr);
    pMob->short_descr = str_dup (argument);

    sendch ("Short description set.\n\r", ch);
    return TRUE;
}



MEDIT (medit_name)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  name [string]\n\r", ch);
        return FALSE;
    }

    free_string (pMob->player_name);
    pMob->player_name = str_dup (argument);

    sendch ("Name set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_shop)
{
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument (argument, command);
    argument = one_argument (argument, arg1);

    EDIT_MOB (ch, pMob);

    if (command[0] == '\0')
    {
        sendch ("Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch);
        sendch ("         shop profit [#xbuying%] [#xselling%]\n\r",
                      ch);
        sendch ("         shop type [#x0-4] [item type]\n\r", ch);
        sendch ("         shop assign\n\r", ch);
        sendch ("         shop remove\n\r", ch);
        return FALSE;
    }


    if (!str_cmp (command, "hours"))
    {
        if (arg1[0] == '\0' || !is_number (arg1)
            || argument[0] == '\0' || !is_number (argument))
        {
            sendch ("Syntax:  shop hours [#xopening] [#xclosing]\n\r",
                          ch);
            return FALSE;
        }

        if (!pMob->pShop)
        {
            sendch
                ("MEdit:  You must create the shop first (shop assign).\n\r",
                 ch);
            return FALSE;
        }

        pMob->pShop->open_hour = atoi (arg1);
        pMob->pShop->close_hour = atoi (argument);

        sendch ("Shop hours set.\n\r", ch);
        return TRUE;
    }


    if (!str_cmp (command, "profit"))
    {
        if (arg1[0] == '\0' || !is_number (arg1)
            || argument[0] == '\0' || !is_number (argument))
        {
            sendch ("Syntax:  shop profit [#xbuying%] [#xselling%]\n\r",
                          ch);
            return FALSE;
        }

        if (!pMob->pShop)
        {
            sendch
                ("MEdit:  You must create the shop first (shop assign).\n\r",
                 ch);
            return FALSE;
        }

        pMob->pShop->profit_buy = atoi (arg1);
        pMob->pShop->profit_sell = atoi (argument);

        sendch ("Shop profit set.\n\r", ch);
        return TRUE;
    }


    if (!str_cmp (command, "type"))
    {
        char buf[MAX_INPUT_LENGTH];
        int value;

        if (arg1[0] == '\0' || !is_number (arg1) || argument[0] == '\0')
        {
            sendch ("Syntax:  shop type [#x0-4] [item type]\n\r", ch);
            return FALSE;
        }

        if (atoi (arg1) >= MAX_TRADE)
        {
            sprintf (buf, "MEdit:  May sell %d items max.\n\r", MAX_TRADE);
            sendch (buf, ch);
            return FALSE;
        }

        if (!pMob->pShop)
        {
            sendch
                ("MEdit:  You must create the shop first (shop assign).\n\r",
                 ch);
            return FALSE;
        }

        if ((value = flag_value (type_flags, argument)) == NO_FLAG)
        {
            sendch ("MEdit:  That type of item is not known.\n\r", ch);
            return FALSE;
        }

        pMob->pShop->buy_type[atoi (arg1)] = value;

        sendch ("Shop type set.\n\r", ch);
        return TRUE;
    }

    /* shop assign && shop delete by Phoenix */

    if (!str_prefix (command, "assign"))
    {
        if (pMob->pShop)
        {
            sendch ("Mob already has a shop assigned to it.\n\r", ch);
            return FALSE;
        }

        pMob->pShop = new_shop ();
        if (!shop_first)
            shop_first = pMob->pShop;
        if (shop_last)
            shop_last->next = pMob->pShop;
        shop_last = pMob->pShop;

        pMob->pShop->keeper = pMob->vnum;

        sendch ("New shop assigned to mobile.\n\r", ch);
        return TRUE;
    }

    if (!str_prefix (command, "remove"))
    {
        SHOP_DATA *pShop;

        pShop = pMob->pShop;
        pMob->pShop = NULL;

        if (pShop == shop_first)
        {
            if (!pShop->next)
            {
                shop_first = NULL;
                shop_last = NULL;
            }
            else
                shop_first = pShop->next;
        }
        else
        {
            SHOP_DATA *ipShop;

            for (ipShop = shop_first; ipShop; ipShop = ipShop->next)
            {
                if (ipShop->next == pShop)
                {
                    if (!pShop->next)
                    {
                        shop_last = ipShop;
                        shop_last->next = NULL;
                    }
                    else
                        ipShop->next = pShop->next;
                }
            }
        }

        free_shop (pShop);

        sendch ("Mobile is no longer a shopkeeper.\n\r", ch);
        return TRUE;
    }

    medit_shop (ch, "");
    return FALSE;
}


/* ROM medit functions: */


MEDIT (medit_sex)
{                                /* Moved out of medit() due to naming conflicts -- Hugin */
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (sex_flags, argument)) != NO_FLAG)
        {
            pMob->sex = value;

            sendch ("Sex set.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: sex [sex]\n\r"
                  "Type '? sex' for a list of flags.\n\r", ch);
    return FALSE;
}


MEDIT (medit_act)
{                                /* Moved out of medit() due to naming conflicts -- Hugin */
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (act_flags, argument)) != NO_FLAG)
        {
            pMob->act ^= value;
            SET_BIT (pMob->act, ACT_IS_NPC);

            sendch ("Act flag toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: act [flag]\n\r"
                  "Type '? act' for a list of flags.\n\r", ch);
    return FALSE;
}


MEDIT (medit_affect)
{                                /* Moved out of medit() due to naming conflicts -- Hugin */
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (affect_flags, argument)) != NO_FLAG)
        {
            pMob->affected_by ^= value;

            sendch ("Affect flag toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: affect [flag]\n\r"
                  "Type '? affect' for a list of flags.\n\r", ch);
    return FALSE;
}



MEDIT (medit_ac)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int pierce, bash, slash, exotic;

    do
    {                            /* So that I can use break and send the syntax in one place */
        if (argument[0] == '\0')
            break;

        EDIT_MOB (ch, pMob);
        argument = one_argument (argument, arg);

        if (!is_number (arg))
            break;
        pierce = atoi (arg);
        argument = one_argument (argument, arg);

        if (arg[0] != '\0')
        {
            if (!is_number (arg))
                break;
            bash = atoi (arg);
            argument = one_argument (argument, arg);
        }
        else
            bash = pMob->ac[AC_BASH];

        if (arg[0] != '\0')
        {
            if (!is_number (arg))
                break;
            slash = atoi (arg);
            argument = one_argument (argument, arg);
        }
        else
            slash = pMob->ac[AC_SLASH];

        if (arg[0] != '\0')
        {
            if (!is_number (arg))
                break;
            exotic = atoi (arg);
        }
        else
            exotic = pMob->ac[AC_EXOTIC];

        pMob->ac[AC_PIERCE] = pierce;
        pMob->ac[AC_BASH] = bash;
        pMob->ac[AC_SLASH] = slash;
        pMob->ac[AC_EXOTIC] = exotic;

        sendch ("Ac set.\n\r", ch);
        return TRUE;
    }
    while (FALSE);                /* Just do it once.. */

    sendch
        ("Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\n\r"
         "help MOB_AC  gives a list of reasonable ac-values.\n\r", ch);
    return FALSE;
}

MEDIT (medit_form)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (form_flags, argument)) != NO_FLAG)
        {
            pMob->form ^= value;
            sendch ("Form toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: form [flags]\n\r"
                  "Type '? form' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT (medit_part)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (part_flags, argument)) != NO_FLAG)
        {
            pMob->parts ^= value;
            sendch ("Parts toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: part [flags]\n\r"
                  "Type '? part' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT (medit_imm)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (imm_flags, argument)) != NO_FLAG)
        {
            pMob->imm_flags ^= value;
            sendch ("Immunity toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: imm [flags]\n\r"
                  "Type '? imm' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT (medit_res)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (res_flags, argument)) != NO_FLAG)
        {
            pMob->res_flags ^= value;
            sendch ("Resistance toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: res [flags]\n\r"
                  "Type '? res' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT (medit_vuln)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (vuln_flags, argument)) != NO_FLAG)
        {
            pMob->vuln_flags ^= value;
            sendch ("Vulnerability toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: vuln [flags]\n\r"
                  "Type '? vuln' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT (medit_material)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  material [string]\n\r", ch);
        return FALSE;
    }

    free_string (pMob->material);
    pMob->material = str_dup (argument);

    sendch ("Material set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_off)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (off_flags, argument)) != NO_FLAG)
        {
            pMob->off_flags ^= value;
            sendch ("Offensive behaviour toggled.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: off [flags]\n\r"
                  "Type '? off' for a list of flags.\n\r", ch);
    return FALSE;
}

MEDIT (medit_size)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB (ch, pMob);

        if ((value = flag_value (size_flags, argument)) != NO_FLAG)
        {
            pMob->size = value;
            sendch ("Size set.\n\r", ch);
            return TRUE;
        }
    }

    sendch ("Syntax: size [size]\n\r"
                  "Type '? size' for a list of sizes.\n\r", ch);
    return FALSE;
}


MEDIT (medit_race)
{
    MOB_INDEX_DATA *pMob;
    int race;

    if (argument[0] != '\0' && (race = race_lookup (argument)) != 0)
    {
        EDIT_MOB (ch, pMob);

        pMob->race = race;
        pMob->act = race_table[race].act;
        pMob->affected_by = race_table[race].aff;
        pMob->off_flags = race_table[race].off;
        pMob->imm_flags = race_table[race].imm;
        pMob->res_flags = race_table[race].res;
        pMob->vuln_flags = race_table[race].vuln;
        pMob->form = race_table[race].form;
        pMob->parts = race_table[race].parts;

        sendch ("Race set.\n\r", ch);
        return TRUE;
    }

    if (argument[0] == '?')
    {
        char buf[MAX_STRING_LENGTH];

        sendch ("Available races are:", ch);

        for (race = 0; race_table[race].name != NULL; race++)
        {
            if ((race % 3) == 0)
                sendch ("\n\r", ch);
            sprintf (buf, " %-15s", race_table[race].name);
            sendch (buf, ch);
        }

        sendch ("\n\r", ch);
        return FALSE;
    }

    sendch ("Syntax:  race [race]\n\r"
                  "Type 'race ?' for a list of races.\n\r", ch);
    return FALSE;
}


MEDIT (medit_position)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument (argument, arg);

    switch (arg[0])
    {
        default:
            break;

        case 'S':
        case 's':
            if (str_prefix (arg, "start"))
                break;

            if ((value = flag_value (position_flags, argument)) == NO_FLAG)
                break;

            EDIT_MOB (ch, pMob);

            pMob->start_pos = value;
            sendch ("Start position set.\n\r", ch);
            return TRUE;

        case 'D':
        case 'd':
            if (str_prefix (arg, "default"))
                break;

            if ((value = flag_value (position_flags, argument)) == NO_FLAG)
                break;

            EDIT_MOB (ch, pMob);

            pMob->default_pos = value;
            sendch ("Default position set.\n\r", ch);
            return TRUE;
    }

    sendch ("Syntax:  position [start/default] [position]\n\r"
                  "Type '? position' for a list of positions.\n\r", ch);
    return FALSE;
}


MEDIT (medit_zenni)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  wealth [number]\n\r", ch);
        return FALSE;
    }

    pMob->wealth = atoi (argument);

    sendch ("Wealth set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_hitroll)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  hitroll [number]\n\r", ch);
        return FALSE;
    }

    pMob->hitroll = atoi (argument);

    sendch ("Hitroll set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_str)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  str [number]\n\r", ch);
        return FALSE;
    }

    pMob->stat[STAT_STR] = atoi (argument);

    sendch ("Strength set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_int)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  int [number]\n\r", ch);
        return FALSE;
    }

    pMob->stat[STAT_INT] = atoi (argument);

    sendch ("Intelligence set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_wil)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  wis [number]\n\r", ch);
        return FALSE;
    }

    pMob->stat[STAT_WIL] = atoi (argument);

    sendch ("Will power set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_dex)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  dex [number]\n\r", ch);
        return FALSE;
    }

    pMob->stat[STAT_DEX] = atoi (argument);

    sendch ("Dexterity set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_cha)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  con [number]\n\r", ch);
        return FALSE;
    }

    pMob->stat[STAT_CHA] = atoi (argument);

    sendch ("Charisma set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_hitbonus)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  hitbonus [number] (in %)\n\r", ch);
        return FALSE;
    }

    pMob->hit_bonus = atoi (argument);

    sendch ("Hitbonus set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_kibonus)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0' || !is_number (argument))
    {
        sendch ("Syntax:  kibonus [number] (in %)\n\r", ch);
        return FALSE;
    }

    pMob->ki_bonus = atoi (argument);

    sendch ("Kibonus set.\n\r", ch);
    return TRUE;
}

void show_liqlist (CHAR_DATA * ch)
{
    int liq;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf ();

    for (liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
        if ((liq % 21) == 0)
            add_buf (buffer,
                     "Name                 Color          Drunk Full Thirst Hunger Ssize Regen\n\r");

        sprintf (buf, "%-20s %-14s %5d %4d %6d %4d %5d %4d\n\r",
                 liq_table[liq].liq_name, liq_table[liq].liq_color,
                 liq_table[liq].liq_affect[0], liq_table[liq].liq_affect[1],
                 liq_table[liq].liq_affect[2], liq_table[liq].liq_affect[3],
                 liq_table[liq].liq_affect[4], liq_table[liq].liq_affect[5]);
        add_buf (buffer, buf);
    }

    page_to_char (buf_string (buffer), ch);
    free_buf (buffer);

    return;
}

void show_damlist (CHAR_DATA * ch)
{
    int att;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf ();

    for (att = 0; attack_table[att].name != NULL; att++)
    {
        if ((att % 21) == 0)
            add_buf (buffer, "Name                 Noun\n\r");

        sprintf (buf, "%-20s %-20s\n\r",
                 attack_table[att].name, attack_table[att].noun);
        add_buf (buffer, buf);
    }

    page_to_char (buf_string (buffer), ch);
    free_buf (buffer);

    return;
}

MEDIT (medit_group)
{
    MOB_INDEX_DATA *pMob;
    MOB_INDEX_DATA *pMTemp;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int temp;
    BUFFER *buffer;
    bool found = FALSE;

    EDIT_MOB (ch, pMob);

    if (argument[0] == '\0')
    {
        sendch ("Syntax: group [number]\n\r", ch);
        sendch ("        group show [number]\n\r", ch);
        return FALSE;
    }

    if (is_number (argument))
    {
        pMob->group = atoi (argument);
        sendch ("Group set.\n\r", ch);
        return TRUE;
    }

    argument = one_argument (argument, arg);

    if (!strcmp (arg, "show") && is_number (argument))
    {
        if (atoi (argument) == 0)
        {
            sendch ("Are you crazy?\n\r", ch);
            return FALSE;
        }

        buffer = new_buf ();

        for (temp = 0; temp < 65536; temp++)
        {
            pMTemp = get_mob_index (temp);
            if (pMTemp && (pMTemp->group == atoi (argument)))
            {
                found = TRUE;
                sprintf (buf, "[%5d] %s\n\r", pMTemp->vnum,
                         pMTemp->player_name);
                add_buf (buffer, buf);
            }
        }

        if (found)
            page_to_char (buf_string (buffer), ch);
        else
            sendch ("No mobs in that group.\n\r", ch);

        free_buf (buffer);
        return FALSE;
    }

    return FALSE;
}

REDIT (redit_owner)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM (ch, pRoom);

    if (argument[0] == '\0')
    {
        sendch ("Syntax:  owner [owner]\n\r", ch);
        sendch ("         owner none\n\r", ch);
        return FALSE;
    }

    free_string (pRoom->owner);
    if (!str_cmp (argument, "none"))
        pRoom->owner = str_dup ("");
    else
        pRoom->owner = str_dup (argument);

    sendch ("Owner set.\n\r", ch);
    return TRUE;
}

MEDIT (medit_addmprog)
{
    int value;
    MOB_INDEX_DATA *pMob;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char phrase[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];

    EDIT_MOB (ch, pMob);
    argument = one_argument (argument, num);
    argument = one_argument (argument, trigger);
    argument = one_argument (argument, phrase);

    if (!is_number (num) || trigger[0] == '\0' || phrase[0] == '\0')
    {
        sendch ("Syntax:   addmprog [vnum] [trigger] [phrase]\n\r", ch);
        return FALSE;
    }

    if ((value = flag_value (mprog_flags, trigger)) == NO_FLAG)
    {
        sendch ("Valid flags are:\n\r", ch);
        show_help (ch, "mprog");
        return FALSE;
    }

    if ((code = get_prog_index (atoi (num), PRG_MPROG)) == NULL)
    {
        sendch ("No such MOBProgram.\n\r", ch);
        return FALSE;
    }

    list = new_mprog ();
    list->vnum = atoi (num);
    list->trig_type = value;
    list->trig_phrase = str_dup (phrase);
    list->code = code->code;
    SET_BIT (pMob->mprog_flags, value);
    list->next = pMob->mprogs;
    pMob->mprogs = list;

    sendch ("Mprog Added.\n\r", ch);
    return TRUE;
}

MEDIT (medit_delmprog)
{
    MOB_INDEX_DATA *pMob;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char mprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_MOB (ch, pMob);

    one_argument (argument, mprog);
    if (!is_number (mprog) || mprog[0] == '\0')
    {
        sendch ("Syntax:  delmprog [#mprog]\n\r", ch);
        return FALSE;
    }

    value = atoi (mprog);

    if (value < 0)
    {
        sendch ("Only non-negative mprog-numbers allowed.\n\r", ch);
        return FALSE;
    }

    if (!(list = pMob->mprogs))
    {
        sendch ("MEdit:  Non existant mprog.\n\r", ch);
        return FALSE;
    }

    if (value == 0)
    {
        REMOVE_BIT (pMob->mprog_flags, pMob->mprogs->trig_type);
        list = pMob->mprogs;
        pMob->mprogs = list->next;
        free_mprog (list);
    }
    else
    {
        while ((list_next = list->next) && (++cnt < value))
            list = list_next;

        if (list_next)
        {
            REMOVE_BIT (pMob->mprog_flags, list_next->trig_type);
            list->next = list_next->next;
            free_mprog (list_next);
        }
        else
        {
            sendch ("No such mprog.\n\r", ch);
            return FALSE;
        }
    }

    sendch ("Mprog removed.\n\r", ch);
    return TRUE;
}

REDIT (redit_room)
{
    ROOM_INDEX_DATA *room;
    int value;

    EDIT_ROOM (ch, room);

    if ((value = flag_value (room_flags, argument)) == NO_FLAG)
    {
        sendch ("Sintaxis: room [flags]\n\r", ch);
        return FALSE;
    }

    TOGGLE_BIT (room->room_flags, value);
    sendch ("Room flags toggled.\n\r", ch);
    return TRUE;
}

REDIT (redit_sector)
{
    ROOM_INDEX_DATA *room;
    int value;

    EDIT_ROOM (ch, room);

    if ((value = flag_value (sector_flags, argument)) == NO_FLAG)
    {
        sendch ("Sintaxis: sector [tipo]\n\r", ch);
        return FALSE;
    }

    room->sector_type = value;
    sendch ("Sector type set.\n\r", ch);

    return TRUE;
}

OEDIT ( oedit_addoprog )
{
  int value;
  OBJ_INDEX_DATA *pObj;
  PROG_LIST *list;
  PROG_CODE *code;
  char trigger[MAX_STRING_LENGTH];
  char phrase[MAX_STRING_LENGTH];
  char num[MAX_STRING_LENGTH];

  EDIT_OBJ(ch, pObj);
  argument=one_argument(argument, num);
  argument=one_argument(argument, trigger);
  argument=one_argument(argument, phrase);

  if (!is_number(num) || trigger[0] =='\0' || phrase[0] =='\0' )
  {
        sendch("Syntax:   addoprog [vnum] [trigger] [phrase]\n\r",ch);
        return FALSE;
  }

  if ( (value = flag_value (oprog_flags, trigger) ) == NO_FLAG)
  {
        sendch("Valid flags are:\n\r",ch);
        show_help( ch, "oprog");
        return FALSE;
  }

  if ( ( code =get_prog_index (atoi(num), PRG_OPROG ) ) == NULL)
  {
        sendch("No such OBJProgram.\n\r",ch);
        return FALSE;
  }

  list                  = new_oprog();
  list->vnum            = atoi(num);
  list->trig_type       = value;
  list->trig_phrase     = str_dup(phrase);
  list->code            = code->code;
  SET_BIT(pObj->oprog_flags,value);
  list->next            = pObj->oprogs;
  pObj->oprogs          = list;

  sendch( "Oprog Added.\n\r",ch);
  return TRUE;
}

OEDIT ( oedit_deloprog )
{
    OBJ_INDEX_DATA *pObj;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char oprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument( argument, oprog );
    if (!is_number( oprog ) || oprog[0] == '\0' )
    {
       sendch("Syntax:  deloprog [#oprog]\n\r",ch);
       return FALSE;
    }

    value = atoi ( oprog );

    if ( value < 0 )
    {
        sendch("Only non-negative oprog-numbers allowed.\n\r",ch);
        return FALSE;
    }

    if ( !(list= pObj->oprogs) )
    {
        sendch("OEdit:  Non existant oprog.\n\r",ch);
        return FALSE;
    }

    if ( value == 0 )
    {
	REMOVE_BIT(pObj->oprog_flags, pObj->oprogs->trig_type);
        list = pObj->oprogs;
        pObj->oprogs = list->next;
        free_oprog( list );
    }
    else
    {
        while ( (list_next = list->next) && (++cnt < value ) )
                list = list_next;

        if ( list_next )
        {
		REMOVE_BIT(pObj->oprog_flags, list_next->trig_type);
                list->next = list_next->next;
                free_oprog(list_next);
        }
        else
        {
                sendch("No such oprog.\n\r",ch);
                return FALSE;
        }
    }

    sendch("Oprog removed.\n\r", ch);
    return TRUE;
}

REDIT ( redit_addrprog )
{
  int value;
  ROOM_INDEX_DATA *pRoom;
  PROG_LIST *list;
  PROG_CODE *code;
  char trigger[MAX_STRING_LENGTH];
  char phrase[MAX_STRING_LENGTH];
  char num[MAX_STRING_LENGTH];

  EDIT_ROOM(ch, pRoom);
  argument=one_argument(argument, num);
  argument=one_argument(argument, trigger);
  argument=one_argument(argument, phrase);

  if (!is_number(num) || trigger[0] =='\0' || phrase[0] =='\0' )
  {
        sendch("Syntax:   addrprog [vnum] [trigger] [phrase]\n\r",ch);
        return FALSE;
  }

  if ( (value = flag_value (rprog_flags, trigger) ) == NO_FLAG)
  {
        sendch("Valid flags are:\n\r",ch);
        show_help( ch, "rprog");
        return FALSE;
  }

  if ( ( code =get_prog_index (atoi(num), PRG_RPROG ) ) == NULL)
  {
        sendch("No such ROOMProgram.\n\r",ch);
        return FALSE;
  }

  list                  = new_rprog();
  list->vnum            = atoi(num);
  list->trig_type       = value;
  list->trig_phrase     = str_dup(phrase);
  list->code            = code->code;
  SET_BIT(pRoom->rprog_flags,value);
  list->next            = pRoom->rprogs;
  pRoom->rprogs         = list;

  sendch( "Rprog Added.\n\r",ch);
  return TRUE;
}

REDIT ( redit_delrprog )
{
    ROOM_INDEX_DATA *pRoom;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char rprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_ROOM(ch, pRoom);

    one_argument( argument, rprog );
    if (!is_number( rprog ) || rprog[0] == '\0' )
    {
       sendch("Syntax:  delrprog [#rprog]\n\r",ch);
       return FALSE;
    }

    value = atoi ( rprog );

    if ( value < 0 )
    {
        sendch("Only non-negative rprog-numbers allowed.\n\r",ch);
        return FALSE;
    }

    if ( !(list= pRoom->rprogs) )
    {
        sendch("REdit:  Non existant rprog.\n\r",ch);
        return FALSE;
    }

    if ( value == 0 )
    {
	REMOVE_BIT(pRoom->rprog_flags, pRoom->rprogs->trig_type);
        list = pRoom->rprogs;
        pRoom->rprogs = list->next;
        free_rprog( list );
    }
    else
    {
        while ( (list_next = list->next) && (++cnt < value ) )
                list = list_next;

        if ( list_next )
        {
			REMOVE_BIT(pRoom->rprog_flags, list_next->trig_type);
                list->next = list_next->next;
                free_rprog(list_next);
        }
        else
        {
                sendch("No such rprog.\n\r",ch);
                return FALSE;
        }
    }

    sendch("Rprog removed.\n\r", ch);
    return TRUE;
}
