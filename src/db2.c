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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>

#include "merc.h"
#include "db.h"
#include "tables.h"
#include "lookup.h"

extern int flag_lookup
args ((const char *name, const struct flag_type * flag_table));

/* values for db2.c */
struct social_type social_table[MAX_SOCIALS];
int social_count;

/* snarf a socials file */
void load_socials (FILE * fp)
{
    for (;;)
    {
        struct social_type social;
        char *temp;
        /* clear social */
        social.char_no_arg = NULL;
        social.others_no_arg = NULL;
        social.char_found = NULL;
        social.others_found = NULL;
        social.vict_found = NULL;
        social.char_not_found = NULL;
        social.char_auto = NULL;
        social.others_auto = NULL;

        temp = fread_word (fp);
        if (!strcmp (temp, "#0"))
            return;                /* done */
#if defined(social_debug)
        else
            fprintf (stderr, "%s\n\r", temp);
#endif

        strcpy (social.name, temp);
        fread_to_eol (fp);

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.char_no_arg = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_no_arg = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.others_no_arg = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_no_arg = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.char_found = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_found = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.others_found = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_found = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.vict_found = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.vict_found = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.char_not_found = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_not_found = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.char_auto = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_auto = temp;

        temp = fread_string_eol (fp);
        if (!strcmp (temp, "$"))
            social.others_auto = NULL;
        else if (!strcmp (temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
    }
    return;
}






/*
 * Snarf a mob section.  new style
 */
void load_mobiles (FILE * fp)
{
    MOB_INDEX_DATA *pMobIndex;
	int i, j;
    char *szWord[10];

    if (!area_last)
    {                            /* OLC */
        logstr (LOG_BUG, "Load_mobiles: no #AREA seen yet.", 0);
        exit (1);
    }

    for (;;)
    {
        sh_int vnum;
        char letter;
        int iHash;

        letter = fread_letter (fp);
        if (letter != '#')
        {
            logstr (LOG_BUG, "Load_mobiles: # not found.", 0);
            exit (1);
        }

        vnum = fread_number (fp);
        if (vnum == 0)
            break;

        fBootDb = FALSE;
        if (get_mob_index (vnum) != NULL)
        {
            logstr (LOG_BUG, "Load_mobiles: vnum %d duplicated.", vnum);
            exit (1);
        }
        fBootDb = TRUE;

        pMobIndex = alloc_perm (sizeof (*pMobIndex));
        pMobIndex->vnum = vnum;
        pMobIndex->area = area_last;    /* OLC */
        newmobs++;
        pMobIndex->player_name = fread_string (fp);
        pMobIndex->short_descr = fread_string (fp);
        pMobIndex->long_descr = fread_string (fp);
        pMobIndex->description = fread_string (fp);
        pMobIndex->race = race_lookup (fread_string (fp));

        pMobIndex->long_descr[0] = UPPER (pMobIndex->long_descr[0]);
        pMobIndex->description[0] = UPPER (pMobIndex->description[0]);

        pMobIndex->act = fread_flag (fp) | ACT_IS_NPC | race_table[pMobIndex->race].act;
        pMobIndex->affected_by = fread_flag (fp) | race_table[pMobIndex->race].aff;
        pMobIndex->pShop = NULL;
        pMobIndex->alignment = fread_number (fp);
        pMobIndex->group = fread_number (fp);

        pMobIndex->level = 1;

        for (j = 0; j < 10; ++j)
            szWord[j] = str_dup(fread_word (fp));
        /* If version 1:
           0pl    1hitroll
           2stat 3stat 4stat 5stat 6stat
           7hit_regen   8ki_regen
           9dam_type

           otherwise,
           0hitroll
           1stat 2stat 3stat 4stat 5stat
           6hit_regen   7ki_regen
           8dam_type
           9ac_pierce
         */
        // version one:
        if (!is_number (szWord[9])) {
            pMobIndex->hitroll = atoi(szWord[1]);
            for (i = 0; i < MAX_STATS; i++)
			    pMobIndex->stat[i] = atoi(szWord[i+2]);
            pMobIndex->hit_bonus = atoi(szWord[7]);
            pMobIndex->ki_bonus = atoi(szWord[8]);
            pMobIndex->dam_type = attack_lookup (szWord[9]);

            pMobIndex->ac[AC_PIERCE] = fread_number (fp);
        }
        // other version
        else {
            pMobIndex->hitroll = atoi(szWord[0]);
            for (i = 0; i < MAX_STATS; i++)
			    pMobIndex->stat[i] = atoi(szWord[i+1]);
            pMobIndex->hit_bonus = atoi(szWord[6]);
            pMobIndex->ki_bonus = atoi(szWord[7]);
            pMobIndex->dam_type = attack_lookup (szWord[8]);
            pMobIndex->ac[AC_PIERCE] = atoi(szWord[9]);
        }

        for (j = 0; j < 10; ++j)
            free_string (szWord[j]);
        /*
        pMobIndex->hitroll = fread_number (fp);

		// stats
		for (i = 0; i < MAX_STATS; i++)
			pMobIndex->stat[i] = fread_number(fp);
		pMobIndex->hit_bonus = fread_number(fp);
		pMobIndex->ki_bonus = fread_number(fp);

        pMobIndex->dam_type = attack_lookup (fread_word (fp));
        */
        /* read armor class */
        //pMobIndex->ac[AC_PIERCE] = fread_number (fp);
        pMobIndex->ac[AC_BASH] = fread_number (fp);
        pMobIndex->ac[AC_SLASH] = fread_number (fp);
        pMobIndex->ac[AC_EXOTIC] = fread_number (fp);

        /* read flags and add in data from the race table */
        pMobIndex->off_flags = fread_flag (fp) | race_table[pMobIndex->race].off;
        pMobIndex->imm_flags = fread_flag (fp) | race_table[pMobIndex->race].imm;
        pMobIndex->res_flags = fread_flag (fp) | race_table[pMobIndex->race].res;
        pMobIndex->vuln_flags = fread_flag (fp) | race_table[pMobIndex->race].vuln;

        /* vital statistics */
        pMobIndex->start_pos = position_lookup (fread_word (fp));
        pMobIndex->default_pos = position_lookup (fread_word (fp));
        pMobIndex->sex = sex_lookup (fread_word (fp));

        pMobIndex->wealth = fread_number (fp);

        pMobIndex->form = fread_flag (fp) | race_table[pMobIndex->race].form;
        pMobIndex->parts = fread_flag (fp)
            | race_table[pMobIndex->race].parts;
        /* size */
        CHECK_POS (pMobIndex->size, size_lookup (fread_word (fp)), "size");
/*    pMobIndex->size            = size_lookup(fread_word(fp)); */
        pMobIndex->material = str_dup (fread_word (fp));

        for (;;)
        {
            letter = fread_letter (fp);

            if (letter == 'F')
            {
                char *word;
                long vector;

                word = fread_word (fp);
                vector = fread_flag (fp);

                if (!str_prefix (word, "act"))
                    REMOVE_BIT (pMobIndex->act, vector);
                else if (!str_prefix (word, "aff"))
                    REMOVE_BIT (pMobIndex->affected_by, vector);
                else if (!str_prefix (word, "off"))
                    REMOVE_BIT (pMobIndex->off_flags, vector);
                else if (!str_prefix (word, "imm"))
                    REMOVE_BIT (pMobIndex->imm_flags, vector);
                else if (!str_prefix (word, "res"))
                    REMOVE_BIT (pMobIndex->res_flags, vector);
                else if (!str_prefix (word, "vul"))
                    REMOVE_BIT (pMobIndex->vuln_flags, vector);
                else if (!str_prefix (word, "for"))
                    REMOVE_BIT (pMobIndex->form, vector);
                else if (!str_prefix (word, "par"))
                    REMOVE_BIT (pMobIndex->parts, vector);
                else
                {
                    logstr (LOG_BUG, "Flag remove: flag not found.", 0);
                    exit (1);
                }
            }
            else if (letter == 'M')
            {
                PROG_LIST *pMprog;
                char *word;
                int trigger = 0;

                pMprog = alloc_perm (sizeof (*pMprog));
                word = fread_word (fp);
                if ((trigger = flag_lookup (word, mprog_flags)) == NO_FLAG)
                {
                    logstr (LOG_BUG, "MOBprogs: invalid trigger.", 0);
                    exit (1);
                }
                SET_BIT (pMobIndex->mprog_flags, trigger);
                pMprog->trig_type = trigger;
                pMprog->vnum = fread_number (fp);
                pMprog->trig_phrase = fread_string (fp);
                pMprog->next = pMobIndex->mprogs;
                pMobIndex->mprogs = pMprog;
            }
            else
            {
                ungetc (letter, fp);
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pMobIndex->next = mob_index_hash[iHash];
        mob_index_hash[iHash] = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;    /* OLC */
        assign_area_vnum (vnum);    /* OLC */
        kill_table[URANGE (0, pMobIndex->level, MAX_LEVEL - 1)].number++;
    }

    return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects (FILE * fp)
{
    OBJ_INDEX_DATA *pObjIndex;

    if (!area_last)
    {                            /* OLC */
        logstr (LOG_BUG, "Load_objects: no #AREA seen yet.", 0);
        exit (1);
    }

    for (;;)
    {
        sh_int vnum;
        char letter;
        int iHash;

        letter = fread_letter (fp);
        if (letter != '#')
        {
            logstr (LOG_BUG, "Load_objects: # not found.", 0);
            exit (1);
        }

        vnum = fread_number (fp);
        if (vnum == 0)
            break;

        fBootDb = FALSE;
        if (get_obj_index (vnum) != NULL)
        {
            logstr (LOG_BUG, "Load_objects: vnum %d duplicated.", vnum);
            exit (1);
        }
        fBootDb = TRUE;

        pObjIndex = alloc_perm (sizeof (*pObjIndex));
        pObjIndex->vnum = vnum;
        pObjIndex->area = area_last;    /* OLC */
        pObjIndex->reset_num = 0;
        newobjs++;
        pObjIndex->name = fread_string (fp);
        pObjIndex->short_descr = fread_string (fp);
        pObjIndex->description = fread_string (fp);
        pObjIndex->material = fread_string (fp);
		pObjIndex->durability = fread_number (fp);

        CHECK_POS (pObjIndex->item_type, item_lookup (fread_word (fp)),
                   "item_type");
        pObjIndex->extra_flags = fread_flag (fp);
        pObjIndex->wear_flags = fread_flag (fp);
        switch (pObjIndex->item_type)
        {
            case ITEM_WEAPON:
                pObjIndex->value[0] = weapon_type (fread_word (fp));
                pObjIndex->value[1] = fread_number (fp);
                pObjIndex->value[2] = fread_number (fp);
                pObjIndex->value[3] = attack_lookup (fread_word (fp));
                pObjIndex->value[4] = fread_flag (fp);
                break;
            case ITEM_CONTAINER:
                pObjIndex->value[0] = fread_number (fp);
                pObjIndex->value[1] = fread_flag (fp);
                pObjIndex->value[2] = fread_number (fp);
                pObjIndex->value[3] = fread_number (fp);
                pObjIndex->value[4] = fread_number (fp);
                break;
            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
                pObjIndex->value[0] = fread_number (fp);
                pObjIndex->value[1] = fread_number (fp);
                CHECK_POS (pObjIndex->value[2], liq_lookup (fread_word (fp)),
                           "liq_lookup");
                pObjIndex->value[3] = fread_number (fp);
                pObjIndex->value[4] = fread_number (fp);
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                pObjIndex->value[0] = fread_number (fp);
                pObjIndex->value[1] = fread_number (fp);
                pObjIndex->value[2] = fread_number (fp);
                pObjIndex->value[3] = skill_lookup(fread_word (fp));
                pObjIndex->value[4] = fread_number (fp);
                break;
            case ITEM_POTION:
            case ITEM_PILL:
            case ITEM_SCROLL:
                pObjIndex->value[0] = fread_number (fp);
                pObjIndex->value[1] = skill_lookup (fread_word (fp));
                pObjIndex->value[2] = skill_lookup (fread_word (fp));
                pObjIndex->value[3] = skill_lookup (fread_word (fp));
                pObjIndex->value[4] = skill_lookup (fread_word (fp));
                break;
            default:
                pObjIndex->value[0] = fread_flag (fp);
                pObjIndex->value[1] = fread_flag (fp);
                pObjIndex->value[2] = fread_flag (fp);
                pObjIndex->value[3] = fread_flag (fp);
                pObjIndex->value[4] = fread_flag (fp);
                break;
        }
        pObjIndex->llPl = fread_number (fp);
        pObjIndex->weight = fread_number (fp);
        pObjIndex->cost = fread_number (fp);

        /* condition */
        letter = fread_letter (fp);
        switch (letter)
        {
            case ('P'):
                pObjIndex->condition = 100;
                break;
            case ('G'):
                pObjIndex->condition = 90;
                break;
            case ('A'):
                pObjIndex->condition = 75;
                break;
            case ('W'):
                pObjIndex->condition = 50;
                break;
            case ('D'):
                pObjIndex->condition = 25;
                break;
            case ('B'):
                pObjIndex->condition = 10;
                break;
            case ('R'):
                pObjIndex->condition = 0;
                break;
            default:
                pObjIndex->condition = 100;
                break;
        }

        for (;;)
        {
            char letter;

            letter = fread_letter (fp);

            if (letter == 'A')
            {
                AFFECT_DATA *paf;

                paf = alloc_perm (sizeof (*paf));
                paf->where = TO_OBJECT;
                paf->type = -1;
				paf->skill_lvl = PL_TO_SKILL(pObjIndex->llPl);
                paf->duration = -1;
                paf->location = fread_number (fp);
                paf->modifier = fread_number (fp);
                paf->bitvector = 0;
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == 'F')
            {
                AFFECT_DATA *paf;

                paf = alloc_perm (sizeof (*paf));
                letter = fread_letter (fp);
                switch (letter)
                {
                    case 'A':
                        paf->where = TO_AFFECTS;
                        break;
                    case 'I':
                        paf->where = TO_IMMUNE;
                        break;
                    case 'R':
                        paf->where = TO_RESIST;
                        break;
                    case 'V':
                        paf->where = TO_VULN;
                        break;
                    default:
                        logstr (LOG_BUG, "Load_objects: Bad where on flag set.", 0);
                        exit (1);
                }
                paf->type = -1;
				paf->skill_lvl = PL_TO_SKILL(pObjIndex->llPl);
                paf->duration = -1;
                paf->location = fread_number (fp);
                paf->modifier = fread_number (fp);
                paf->bitvector = fread_flag (fp);
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == 'E')
            {
                EXTRA_DESCR_DATA *ed;

                ed = alloc_perm (sizeof (*ed));
                ed->keyword = fread_string (fp);
                ed->description = fread_string (fp);
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
                top_ed++;
            }

			else if ( letter == 'O' )
			{
				PROG_LIST *pOprog;
				char *word;
				int trigger = 0;

				pOprog			= alloc_perm(sizeof(*pOprog));
				word			= fread_word( fp );
				if ( !(trigger = flag_lookup( word, oprog_flags )) )
				{
					logstr (LOG_BUG,  "OBJprogs: invalid trigger.",0);
					exit(1);
				}
				SET_BIT( pObjIndex->oprog_flags, trigger );
				pOprog->trig_type	= trigger;
				pOprog->vnum	 	= fread_number( fp );
				pOprog->trig_phrase	= fread_string( fp );
				pOprog->next		= pObjIndex->oprogs;
				pObjIndex->oprogs	= pOprog;
			}

            else
            {
                ungetc (letter, fp);
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;    /* OLC */
        assign_area_vnum (vnum);    /* OLC */
    }

    return;
}

