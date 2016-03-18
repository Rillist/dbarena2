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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <math.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

extern int _filbuf args ((FILE *));


/* int rename(const char *oldfname, const char *newfname); viene en stdio.h */

char *print_flags (int flag)
{
    int count, pos = 0;
    static char buf[52];


    for (count = 0; count < 32; count++)
    {
        if (IS_SET (flag, 1 << count))
        {
            if (count < 26)
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }

    if (pos == 0)
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}


/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST    100
static OBJ_DATA *rgObjNest[MAX_NEST];



/*
 * Local functions.
 */
void fwrite_char args ((CHAR_DATA * ch, FILE * fp));
void fwrite_obj args ((CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest));
void fwrite_pet args ((CHAR_DATA * pet, FILE * fp));
void fread_char args ((CHAR_DATA * ch, FILE * fp));
void fread_pet args ((CHAR_DATA * ch, FILE * fp));
void fread_obj args ((CHAR_DATA * ch, FILE * fp));



/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj (CHAR_DATA * ch)
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    if (IS_NPC (ch))
        return;

	/*
	 * Fix by Edwin. JR -- 10/15/00
	 *
	 * Don't save if the character is invalidated.
	 * This might happen during the auto-logoff of players.
	 * (or other places not yet found out)
	 */
	if ( !IS_VALID(ch)) {
    	logstr (LOG_BUG, "save_char_obj: Trying to save an invalidated character.\n",0);
    	return;
	}

    if (IS_FUSED(ch))
        return;

    if (ch->desc != NULL && ch->desc->original != NULL)
        ch = ch->desc->original;

#if defined(unix)
    /* create god log */
    if (IS_IMMORTAL (ch) || ch->level >= LEVEL_IMMORTAL)
    {
        fclose (fpReserve);
        sprintf (strsave, "%s%s", GOD_DIR, capitalize (ch->name));
        if ((fp = fopen (strsave, "w")) == NULL)
        {
            logstr (LOG_BUG, "Save_char_obj: fopen", 0);
            perror (strsave);
        }

        fprintf (fp, "Lev %2d %s%s\n",
                 ch->level, ch->name, ch->pcdata->title);
        fclose (fp);
        fpReserve = fopen (NULL_FILE, "r");
    }
#endif

    fclose (fpReserve);
    sprintf (strsave, "%s%s", PLAYER_DIR, capitalize (ch->name));
    if ((fp = fopen (TEMP_FILE, "w")) == NULL)
    {
        logstr (LOG_BUG, "Save_char_obj: fopen", 0);
        perror (strsave);
    }
    else
    {
        fwrite_char (ch, fp);
        if (ch->carrying != NULL)
            fwrite_obj (ch, ch->carrying, fp, 0);
        /* save the pets */
        if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
            fwrite_pet (ch->pet, fp);
        fprintf (fp, "#END\n");
    }
    fclose (fp);
    rename (TEMP_FILE, strsave);
    fpReserve = fopen (NULL_FILE, "r");

	AddTopList (ch);
    return;
}



/*
 * Write the char.
 */
void fwrite_char (CHAR_DATA * ch, FILE * fp)
{
    AFFECT_DATA *paf;
    int sn, pos, i;

    fprintf (fp, "#%s\n", IS_NPC (ch) ? "MOB" : "PLAYER");

    fprintf (fp, "Name %s~\n", ch->name);
    fprintf (fp, "Id   %ld\n", ch->id);
    fprintf (fp, "LogO %ld\n", current_time);
    fprintf (fp, "Vers %d\n", 5);
    if (ch->short_descr[0] != '\0')
        fprintf (fp, "ShD  %s~\n", ch->short_descr);
    if (ch->long_descr[0] != '\0')
        fprintf (fp, "LnD  %s~\n", ch->long_descr);
    if (ch->description[0] != '\0')
        fprintf (fp, "Desc %s~\n", ch->description);
    if (ch->immtitle != NULL)
    fprintf( fp, "Immtitle %s~\n",  ch->immtitle);
    if (ch->prompt != NULL || !str_cmp (ch->prompt, "{x[%p/%P]{cPL%c{x[%h]{cHEALTH {x[%k]{cKI{x> [%e] ")
        || !str_cmp (ch->prompt, "[%p/%P]PL%c[%h]HEALTH [%k]KI> [%e] "))
        fprintf (fp, "Prom %s~\n", ch->prompt);
    fprintf (fp, "Race %s~\n", pc_race_table[ch->race].name);
    if (ch->clan)
        fprintf (fp, "Clan %s~\n", clan_table[ch->clan].name);
    fprintf (fp, "Sex  %d\n", ch->sex);
    fprintf (fp, "Levl %d\n", ch->level);
    fprintf (fp, "CurPlvl %d\n", ch->nCurPl);
    if (ch->llSuppressPl > -1)
        fprintf (fp, "SupPlvl %Ld\n", ch->llSuppressPl);
	fprintf (fp, "Sec  %d\n", ch->pcdata->security);    /* OLC */
    fprintf (fp, "Plyd %d\n", ch->played + (int) (current_time - ch->logon));
    fprintf (fp, "Scro %d\n", ch->lines);
    fprintf (fp, "Room %d\n", (ch->in_room == get_room_index (ROOM_VNUM_LIMBO)
                               && ch->was_in_room != NULL)
             ? ch->was_in_room->vnum
             : ch->in_room == NULL ? 400 : ch->in_room->vnum);

    fprintf (fp, "HK  %d %d %d %d\n",
             ch->hit, ch->max_hit, ch->ki, ch->max_ki);
    if (ch->pcdata->nReward > 0)
        fprintf (fp, "Reward %d\n", ch->pcdata->nReward);
    if (ch->trans_count > 0)
        fprintf (fp, "TransCount %d\n", ch->trans_count);
    if (ch->zenni > 0)
        fprintf (fp, "Zenni %ld\n", ch->zenni);
    else
        fprintf (fp, "Zenni %d\n", 0);
    if (ch->act != 0)
        fprintf (fp, "Act  %s\n", print_flags (ch->act));
    if (ch->affected_by != 0)
        fprintf (fp, "AfBy %s\n", print_flags (ch->affected_by));
    fprintf (fp, "Comm %s\n", print_flags (ch->comm));
    if (ch->wiznet)
        fprintf (fp, "Wizn %s\n", print_flags (ch->wiznet));
    if (ch->invis_level)
        fprintf (fp, "Invi %d\n", ch->invis_level);
    if (ch->incog_level)
        fprintf (fp, "Inco %d\n", ch->incog_level);
    fprintf (fp, "Pos  %d\n",
             ch->position == POS_FIGHTING ? POS_STANDING : ch->position);
    if (ch->stance != STANCE_NORMAL)
        fprintf (fp, "Stance %d\n", ch->stance);

    if (ch->saving_throw != 0)
        fprintf (fp, "Save  %d\n", ch->saving_throw);
    fprintf (fp, "Alig  %d\n", ch->alignment);
    if (ch->hitroll != 0)
        fprintf (fp, "Hit   %d\n", ch->hitroll);
    if (ch->damroll != 0)
        fprintf (fp, "Dam   %d\n", ch->damroll);
    if (ch->wimpy != 0)
        fprintf (fp, "Wimp  %d\n", ch->wimpy);
    
	fprintf( fp, "AC %d %d %d %d\n",	
			ch->armor[0],
			ch->armor[1],
			ch->armor[2],
			ch->armor[3]);

	fprintf (fp, "Attr %d %d %d %d %d\n",
             ch->perm_stat[STAT_STR],
             ch->perm_stat[STAT_INT],
             ch->perm_stat[STAT_WIL],
             ch->perm_stat[STAT_DEX],
			 ch->perm_stat[STAT_CHA]);
	
	if (!IS_NPC (ch)) 
	    fprintf (fp, "AttrProg %d %d %d %d %d\n",
                 ch->pcdata->nStatProgress[STAT_STR],
                 ch->pcdata->nStatProgress[STAT_INT],
                 ch->pcdata->nStatProgress[STAT_WIL],
                 ch->pcdata->nStatProgress[STAT_DEX],
				 ch->pcdata->nStatProgress[STAT_CHA]);

    fprintf (fp, "AMod %d %d %d %d %d\n",
             ch->mod_stat[STAT_STR],
             ch->mod_stat[STAT_INT],
             ch->mod_stat[STAT_WIL],
             ch->mod_stat[STAT_DEX],
			 ch->mod_stat[STAT_CHA]);

    if (IS_NPC (ch))
    {
        fprintf (fp, "Vnum %d\n", ch->pIndexData->vnum);
    }
    else
    {
        fprintf (fp, "Pass %s~\n", ch->pcdata->pwd);
        if (ch->pcdata->bamfin[0] != '\0')
            fprintf (fp, "Bin  %s~\n", ch->pcdata->bamfin);
        if (ch->pcdata->bamfout[0] != '\0')
            fprintf (fp, "Bout %s~\n", ch->pcdata->bamfout);
        fprintf (fp, "Titl %s~\n", ch->pcdata->title);
        if (ch->pcdata->pose)
            fprintf (fp, "Pose %s~\n", ch->pcdata->pose);
        fprintf (fp, "TSex %d\n", ch->pcdata->true_sex);
        fprintf (fp, "LLev %d\n", ch->pcdata->last_level);
        fprintf (fp, "HKP %d %d\n", ch->pcdata->perm_hit, ch->pcdata->perm_ki);
        fprintf (fp, "Cnd  %d %d %d %d\n",
                 ch->pcdata->condition[0],
                 ch->pcdata->condition[1],
                 ch->pcdata->condition[2], ch->pcdata->condition[3]);
		if (ch->pcdata->nTrainCount != 0)
            fprintf (fp, "CntTrain %d\n", ch->pcdata->nTrainCount);
		if (ch->pcdata->nTeachCount != 0)
            fprintf (fp, "CntTeach %d\n", ch->pcdata->nTeachCount);
		if (ch->pcdata->nChaosCount != 0)
            fprintf (fp, "CntChaos %d\n", ch->pcdata->nChaosCount);
        if (ch->pcdata->hunt_time != 0)
			fprintf (fp, "HuntTime %d\n", ch->pcdata->hunt_time);
		if (ch->pcdata->rewards != 0)
			fprintf (fp, "Rewards %d\n", ch->pcdata->rewards);
		if (ch->pcdata->recovery != 0)
			fprintf (fp, "Recovery %d\n", ch->pcdata->recovery);
		if (ch->pcdata->reward_obj != 0)
			fprintf (fp, "RewardObj %d\n", ch->pcdata->reward_obj);
		if (ch->pcdata->reward_mob != 0)
			fprintf (fp, "RewardMob %d\n", ch->pcdata->reward_mob);
		if (ch->pcdata->rewarder != 0)
			fprintf (fp, "Rewarder %d\n", ch->pcdata->rewarder);
		if (ch->pcdata->nMissionType != 0)
            fprintf (fp, "MType %d\n", ch->pcdata->nMissionType);

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (ch->pcdata->alias[pos] == NULL
                || ch->pcdata->alias_sub[pos] == NULL)
                break;

            fprintf (fp, "Alias %s %s~\n", ch->pcdata->alias[pos],
                     ch->pcdata->alias_sub[pos]);
        }

		/* Save note board status */
		/* Save number of boards in case that number changes */
		fprintf (fp, "Boards       %d ", MAX_BOARD);
		for (i = 0; i < MAX_BOARD; i++)
			fprintf (fp, "%s %ld ", boards[i].short_name, ch->pcdata->last_note[i]);
		fprintf (fp, "\n");

        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0)
            {
                fprintf (fp, "Sk %d '%s' %d\n",
                         ch->pcdata->learned[sn], skill_table[sn].name, ch->pcdata->nSkillProgress[sn]);
            }
        }
    }

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        fprintf (fp, "Affc '%s' %3d %3d %3d %14Ld %3d %10d\n",
                 skill_table[paf->type].name,
                 paf->where,
                 paf->skill_lvl,
                 paf->duration, paf->modifier, paf->location, paf->bitvector);
    }

    fprintf (fp, "End\n\n");
    return;
}

/* write a pet */
void fwrite_pet (CHAR_DATA * pet, FILE * fp)
{
    AFFECT_DATA *paf;

    fprintf (fp, "#PET\n");

    fprintf (fp, "Vnum %d\n", pet->pIndexData->vnum);

    fprintf (fp, "Name %s~\n", pet->name);
    fprintf (fp, "LogO %ld\n", current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
        fprintf (fp, "ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
        fprintf (fp, "LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
        fprintf (fp, "Desc %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
        fprintf (fp, "Race %s~\n", race_table[pet->race].name);
    if (pet->clan)
        fprintf (fp, "Clan %s~\n", clan_table[pet->clan].name);
    fprintf (fp, "Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
        fprintf (fp, "Levl %d\n", pet->level);
    fprintf (fp, "HK  %d %d %d %d\n",
             pet->hit, pet->max_hit, pet->ki, pet->max_ki);
    if (pet->zenni > 0)
        fprintf (fp, "Zenni %ld\n", pet->zenni);
    if (pet->act != pet->pIndexData->act)
        fprintf (fp, "Act  %s\n", print_flags (pet->act));
    if (pet->affected_by != pet->pIndexData->affected_by)
        fprintf (fp, "AfBy %s\n", print_flags (pet->affected_by));
    if (pet->comm != 0)
        fprintf (fp, "Comm %s\n", print_flags (pet->comm));
    fprintf (fp, "Pos  %d\n", pet->position =
             POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->stance != STANCE_NORMAL)
        fprintf (fp, "Stance %d\n", pet->stance);
    if (pet->saving_throw != 0)
        fprintf (fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
        fprintf (fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
        fprintf (fp, "Hit  %d\n", pet->hitroll);
    fprintf (fp, "Dam  %d\n", pet->damroll);
    fprintf( fp, "AC %d %d %d %d\n",
			 pet->armor[0],
			 pet->armor[1],
			 pet->armor[2],
			 pet->armor[3]);
	fprintf (fp, "Attr %d %d %d %d %d\n",
             pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
             pet->perm_stat[STAT_WIL], pet->perm_stat[STAT_DEX],
             pet->perm_stat[STAT_CHA]);
    fprintf (fp, "AMod %d %d %d %d %d\n",
             pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
             pet->mod_stat[STAT_WIL], pet->mod_stat[STAT_DEX],
             pet->mod_stat[STAT_CHA]);

    for (paf = pet->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        fprintf (fp, "Affc '%s' %3d %3d %3d %14Ld %3d %10d\n",
                 paf->type < 0 || paf->type >= MAX_SKILL ? "none" : skill_table[paf->type].name,
                 paf->where, paf->skill_lvl, paf->duration, paf->modifier,
                 paf->location, paf->bitvector);
    }

    fprintf (fp, "End\n");
    return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj (CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest)
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if (obj->next_content != NULL)
        fwrite_obj (ch, obj->next_content, fp, iNest);

    /*
     * Castrate storage characters.
     */
	/*
    if ((sqrt(ch->pl)/300 < sqrt(obj->pl)/300 - 10 && obj->item_type != ITEM_CONTAINER)
        || obj->item_type == ITEM_KEY
        || (obj->item_type == ITEM_MAP && !obj->value[0]))
        return;
	 */

    fprintf (fp, "#O\n");
    if (IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Vnum %d\n", OBJ_VNUM_RANDOM);
    else
        fprintf (fp, "Vnum %d\n", obj->pIndexData->vnum);
    if (obj->enchanted)
        fprintf (fp, "Enchanted\n");
    fprintf (fp, "Nest %d\n", iNest);

    /* these data are only used if they do not match the defaults */
    if (obj->name != obj->pIndexData->name || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Name %s~\n", obj->name);
    if (obj->short_descr != obj->pIndexData->short_descr || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "ShD  %s~\n", obj->short_descr);
    if (obj->description != obj->pIndexData->description || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Desc %s~\n", obj->description);
    if (obj->extra_flags != obj->pIndexData->extra_flags || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "ExtF %d\n", obj->extra_flags);
    if (obj->wear_flags != obj->pIndexData->wear_flags || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "WeaF %d\n", obj->wear_flags);
    if (obj->item_type != obj->pIndexData->item_type || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Ityp %d\n", obj->item_type);
    if (obj->weight != obj->pIndexData->weight || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Wt   %d\n", obj->weight);
    if (obj->condition != obj->pIndexData->condition || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Cond %d\n", obj->condition);
    if (obj->llPl != obj->pIndexData->llPl || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Pl  %Ld\n", obj->llPl);
    if (obj->timer != 0)
        fprintf (fp, "Time %d\n", obj->timer);
    if (obj->value[0] != obj->pIndexData->value[0]
        || obj->value[1] != obj->pIndexData->value[1]
        || obj->value[2] != obj->pIndexData->value[2]
        || obj->value[3] != obj->pIndexData->value[3]
        || obj->value[4] != obj->pIndexData->value[4]
        || IS_OBJ_STAT (obj, ITEM_RANDOM))
        fprintf (fp, "Val  %d %d %d %d %d\n",
                obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);

    /* variable data */
    fprintf (fp, "Wear %d\n", obj->wear_loc);
    fprintf (fp, "Cost %d\n", obj->cost);
    fprintf (fp, "Dur %d\n", obj->durability);


    switch (obj->item_type)
    {
        case ITEM_POTION:
        case ITEM_SCROLL:
        case ITEM_PILL:
            if (obj->value[1] > 0)
            {
                fprintf (fp, "Spell 1 '%s'\n",
                         skill_table[obj->value[1]].name);
            }

            if (obj->value[2] > 0)
            {
                fprintf (fp, "Spell 2 '%s'\n",
                         skill_table[obj->value[2]].name);
            }

            if (obj->value[3] > 0)
            {
                fprintf (fp, "Spell 3 '%s'\n",
                         skill_table[obj->value[3]].name);
            }

            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            if (obj->value[3] > 0)
            {
                fprintf (fp, "Spell 3 '%s'\n",
                         skill_table[obj->value[3]].name);
            }

            break;
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
        fprintf (fp, "Affc '%s' %3d %3d %3d %14Ld %3d %10d\n",
                 paf->type < 0 || paf->type >= MAX_SKILL ? "none" : skill_table[paf->type].name,
                 paf->where,
                 paf->skill_lvl,
                 paf->duration, paf->modifier, paf->location, paf->bitvector);

    for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
        fprintf (fp, "ExDe %s~ %s~\n", ed->keyword, ed->description);

    fprintf (fp, "End\n\n");

    if (obj->contains != NULL)
        fwrite_obj (ch, obj->contains, fp, iNest + 1);

    return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj (DESCRIPTOR_DATA * d, char *name)
{
    char strsave[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;

    ch = new_char ();
    ch->pcdata = new_pcdata ();

    if (d)
        d->character = ch;
    ch->desc = d;
    ch->name = str_dup (name);
    ch->id = get_pc_id ();
    ch->race = race_lookup ("human");
    ch->act = PLR_NOSUMMON;
    ch->comm = COMM_COMBINE | COMM_PROMPT;
    ch->prompt = str_dup ("{x[%p/%P]{cPL%c{x[%h]{cHEALTH {x[%k]{cKI{x> [%e]");
    ch->pcdata->confirm_delete = FALSE;
	ch->pcdata->board = &boards[DEFAULT_BOARD];
    ch->pcdata->pwd = str_dup ("");
    ch->pcdata->bamfin = str_dup ("");
    ch->pcdata->bamfout = str_dup ("");
    ch->pcdata->title = str_dup ("");
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat] = 10;
    ch->pcdata->condition[COND_THIRST] = 0;
    ch->pcdata->condition[COND_FULL] = 0;
    ch->pcdata->condition[COND_HUNGER] = 0;
    ch->pcdata->security = 0;    /* OLC */

    found = FALSE;
    fclose (fpReserve);

#if defined(unix)
    /* decompress if .gz file exists */
    sprintf (strsave, "%s%s%s", PLAYER_DIR, capitalize (name), ".gz");
    if ((fp = fopen (strsave, "r")) != NULL)
    {
        fclose (fp);
        sprintf (buf, "gzip -dfq %s", strsave);
        system (buf);
    }
#endif

    sprintf (strsave, "%s%s", PLAYER_DIR, capitalize (name));
    if ((fp = fopen (strsave, "r")) != NULL)
    {
        int iNest;

        for (iNest = 0; iNest < MAX_NEST; iNest++)
            rgObjNest[iNest] = NULL;

        found = TRUE;
        for (;;)
        {
            char letter;
            char *word;

            letter = fread_letter (fp);
            if (letter == '*')
            {
                fread_to_eol (fp);
                continue;
            }

            if (letter != '#')
            {
                logstr (LOG_BUG, "Load_char_obj: # not found.", 0);
                break;
            }

            word = fread_word (fp);
            if (!str_cmp (word, "PLAYER"))
                fread_char (ch, fp);
            else if (!str_cmp (word, "OBJECT"))
                fread_obj (ch, fp);
            else if (!str_cmp (word, "O"))
                fread_obj (ch, fp);
            else if (!str_cmp (word, "PET"))
                fread_pet (ch, fp);
            else if (!str_cmp (word, "END"))
                break;
            else
            {
                logstr (LOG_BUG, "Load_char_obj: bad section.", 0);
                break;
            }
        }
        fclose (fp);
    }

    fpReserve = fopen (NULL_FILE, "r");


    /* initialize race */
    if (found)
    {
        int i, sn;

        if (ch->race == 0)
            ch->race = race_lookup ("human");

        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = 17;        /*punch */

        for (i = 0; i < 5; i++)
        {
    		if ((sn = skill_lookup(pc_race_table[ch->race].skills[i])) == -1)
                break;
            if (ch->pcdata->learned[sn] == 0)
				ch->pcdata->learned[sn] = 1;
        }
        ch->affected_by = ch->affected_by | race_table[ch->race].aff;
        ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
        ch->form = race_table[ch->race].form;
        ch->parts = race_table[ch->race].parts;
    }
    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                    \
                if ( !str_cmp( word, literal ) )    \
                {                    \
                    field  = value;            \
                    fMatch = TRUE;            \
                    break;                \
                }

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS( literal, field, value )                    \
                if ( !str_cmp( word, literal ) )    \
                {                    \
                    free_string(field);            \
                    field  = value;            \
                    fMatch = TRUE;            \
                    break;                \
                }

void fread_char (CHAR_DATA * ch, FILE * fp)
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    int count = 0;
    int lastlogoff = current_time;
    int percent;
	int holder;

    sprintf (buf, "Loading %s.", ch->name);
    logstr (LOG_GAME, buf);

    for (;;)
    {
        word = feof (fp) ? "End" : fread_word (fp);
        fMatch = FALSE;

        switch (UPPER (word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol (fp);
                break;

            case 'A':
                KEY ("Act", ch->act, fread_flag (fp));
                KEY ("AffectedBy", ch->affected_by, fread_flag (fp));
                KEY ("AfBy", ch->affected_by, fread_flag (fp));
                KEY ("Alignment", ch->alignment, fread_number (fp));
                KEY ("Alig", ch->alignment, fread_number (fp));

			    if (!str_cmp(word,"AC")) {
					int i;

					for	(i = 0; i < 4; i++)
						ch->armor[i] = fread_number(fp);
					fMatch = TRUE;
					break;
				}

                if (!str_cmp (word, "Alia"))
                {
                    if (count >= MAX_ALIAS)
                    {
                        fread_to_eol (fp);
                        fMatch = TRUE;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup (fread_word (fp));
                    ch->pcdata->alias_sub[count] = str_dup (fread_word (fp));
                    count++;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "Alias"))
                {
                    if (count >= MAX_ALIAS)
                    {
                        fread_to_eol (fp);
                        fMatch = TRUE;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup (fread_word (fp));
                    ch->pcdata->alias_sub[count] = fread_string (fp);
                    count++;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "Affc"))
                {
                    AFFECT_DATA *paf;
                    int sn;
                    char *szSkill;

                    paf = new_affect ();

                    szSkill = fread_word (fp);
                    if (!str_cmp(szSkill, "none"))
                        paf->type = -1;
                    else {
                        sn = skill_lookup (szSkill);
                        if (sn < 0)
                            logstr (LOG_BUG, "Fread_obj: unknown skill.", 0);
                        else
                            paf->type = sn;
                    }

                    paf->where = fread_number (fp);
					paf->skill_lvl = fread_number (fp);
                    paf->duration = fread_number (fp);
                    paf->modifier = fread_number (fp);
                    paf->location = fread_number (fp);
                    paf->bitvector = fread_number (fp);
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "AMod"))
                {
                    int stat;
                    for (stat = 0; stat < MAX_STATS; stat++)
                        ch->mod_stat[stat] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "Attr"))
                {
                    int stat;
                    for (stat = 0; stat < MAX_STATS; stat++)
                        ch->perm_stat[stat] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
                
				if (!str_cmp (word, "AttrProg"))
                {
                    int stat;
                    for (stat = 0; stat < MAX_STATS; stat++)
						ch->pcdata->nStatProgress[stat] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'B':
                KEY ("Bamfin", ch->pcdata->bamfin, fread_string (fp));
                KEY ("Bamfout", ch->pcdata->bamfout, fread_string (fp));
                KEY ("Bin", ch->pcdata->bamfin, fread_string (fp));
                KEY ("Bout", ch->pcdata->bamfout, fread_string (fp));

				/* Read in board status */
				if (!str_cmp(word, "Boards" ))
				{
					int i,num = fread_number (fp); /* number of boards saved */
					char *boardname;

					for (; num ; num-- ) /* for each of the board saved */
					{
						boardname = fread_word (fp);
						i = board_lookup (boardname); /* find board number */

						if (i == BOARD_NOTFOUND) /* Does board still exist ? */
						{
							logstr (LOG_BUG, "fread_char: %s had unknown board name: %s. Skipped.", ch->name, boardname);
							fread_number (fp); /* read last_note and skip info */
						}
						else /* Save it */
							ch->pcdata->last_note[i] = fread_number (fp);
					} /* for */

					fMatch = TRUE;
				} /* Boards */
                break;

            case 'C':
                KEY ("Clan", ch->clan, clan_lookup (fread_string (fp)));
                KEY ("Comm", ch->comm, fread_flag (fp));
                KEY ("CntChaos", ch->pcdata->nChaosCount, fread_number (fp));
                KEY ("CntTeach", ch->pcdata->nTeachCount, fread_number (fp));
                KEY ("CntTrain", ch->pcdata->nTrainCount, fread_number (fp));
                KEY ("CurPlvl", ch->nCurPl, fread_number (fp));

                if (!str_cmp (word, "Condition") || !str_cmp (word, "Cond"))
                {
                    ch->pcdata->condition[0] = fread_number (fp);
                    ch->pcdata->condition[1] = fread_number (fp);
                    ch->pcdata->condition[2] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
                if (!str_cmp (word, "Cnd"))
                {
                    ch->pcdata->condition[0] = fread_number (fp);
                    ch->pcdata->condition[1] = fread_number (fp);
                    ch->pcdata->condition[2] = fread_number (fp);
                    ch->pcdata->condition[3] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'D':
                KEY ("Damroll", ch->damroll, fread_number (fp));
                KEY ("Dam", ch->damroll, fread_number (fp));
                KEY ("Description", ch->description, fread_string (fp));
                KEY ("Desc", ch->description, fread_string (fp));
                break;

            case 'E':
                if (!str_cmp (word, "End"))
                {
                    /* adjust hp ki up  -- here for speed's sake */
                    percent =
                        (current_time - lastlogoff) * 25 / (2 * 60 * 60);

                    percent = UMIN (percent, 100);

                    if (percent > 0 && !IS_AFFECTED (ch, AFF_POISON)
                        && !IS_AFFECTED (ch, AFF_PLAGUE))
                    {
                        ch->hit += (ch->max_hit - ch->hit) * percent / 100;
                        ch->ki += (ch->max_ki - ch->ki) * percent / 100;
                    }
                    return;
                }
                break;

            case 'H':
                KEY ("Hitroll", ch->hitroll, fread_number (fp));
                KEY ("Hit", ch->hitroll, fread_number (fp));
                KEY ("HuntTime", ch->pcdata->hunt_time, fread_number(fp));

				if (!str_cmp (word, "HK"))
                {
					ch->hit = fread_number (fp);
                    ch->max_hit = fread_number (fp);
                    ch->ki = fread_number (fp);
                    ch->max_ki = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }

				if (!str_cmp (word, "HKP"))
                {
                    ch->pcdata->perm_hit = fread_number (fp);
                    ch->pcdata->perm_ki = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
		
                break;

            case 'I':
                KEY ("Id", ch->id, fread_number (fp));
                KEY ("InvisLevel", ch->invis_level, fread_number (fp));
                KEY ("Inco", ch->incog_level, fread_number (fp));
                KEY ("Invi", ch->invis_level, fread_number (fp));
                KEY( "Immtitle",   ch->immtitle, fread_string( fp ) );
                break;

            case 'L':
                KEY ("LastLevel", ch->pcdata->last_level, fread_number (fp));
                KEY ("LLev", ch->pcdata->last_level, fread_number (fp));
                KEY ("Level", ch->level, fread_number (fp));
                KEY ("Lev", ch->level, fread_number (fp));
                KEY ("Levl", ch->level, fread_number (fp));
                KEY ("LogO", lastlogoff, fread_number (fp));
                KEY ("LongDescr", ch->long_descr, fread_string (fp));
                KEY ("LnD", ch->long_descr, fread_string (fp));
                break;

            case 'M':
				KEY ("MType", ch->pcdata->nMissionType, fread_number (fp));
				break;

			case 'N':
                KEYS ("Name", ch->name, fread_string (fp));
                break;

            case 'P':
                KEY ("Password", ch->pcdata->pwd, fread_string (fp));
                KEY ("Pass", ch->pcdata->pwd, fread_string (fp));
                KEY ("Played", ch->played, fread_number (fp));
                KEY ("Plyd", ch->played, fread_number (fp));
                KEY ("Position", ch->position, fread_number (fp));
                KEY ("Pos", ch->position, fread_number (fp));
				KEYS ("Prompt", ch->prompt, fread_string (fp));
                KEY ("Prom", ch->prompt, fread_string (fp));
                
                if (!str_cmp (word, "Pose")) {
                    ch->pcdata->pose = fread_string (fp);
                    if (ch->pcdata->pose[0] != '.'
                        && ch->pcdata->pose[0] != ','
                        && ch->pcdata->pose[0] != '!'
                        && ch->pcdata->pose[0] != '?')
                    {
                        sprintf (buf, " %s", ch->pcdata->pose);
                        free_string (ch->pcdata->pose);
                        ch->pcdata->pose = str_dup (buf);
                    }
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'R':
                KEY ("Race", ch->race, race_lookup (fread_string (fp)));
                KEY ("Reward", ch->pcdata->nReward, fread_number (fp));
				KEY ("Recovery", ch->pcdata->recovery, fread_number (fp));
				KEY ("Rewarder", ch->pcdata->rewarder, fread_number (fp));
				KEY ("Rewards",	ch->pcdata->rewards, fread_number (fp));
				KEY ("RewardMob", ch->pcdata->reward_mob, fread_number (fp));
				KEY ("RewardObj", ch->pcdata->reward_obj, fread_number (fp));

                if (!str_cmp (word, "Room"))
                {
                    ch->in_room = get_room_index (fread_number (fp));
                    if (ch->in_room == NULL)
                        ch->in_room = get_room_index (ROOM_VNUM_LIMBO);
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'S':
                KEY ("SavingThrow", ch->saving_throw, fread_number (fp));
                KEY ("Save", ch->saving_throw, fread_number (fp));
                KEY ("Scro", ch->lines, fread_number (fp));
                KEY ("Sex", ch->sex, fread_number (fp));
                KEY ("ShortDescr", ch->short_descr, fread_string (fp));
                KEY ("ShD", ch->short_descr, fread_string (fp));
                KEY ("Sec", ch->pcdata->security, fread_number (fp));    /* OLC */
				KEY ("SkillLvl", holder, fread_number (fp));
                KEY ("Stance", ch->stance, fread_number (fp));
                KEY ("StatLvl", holder, fread_number (fp));
                KEY ("SupPlvl", ch->llSuppressPl, fread_number (fp));

                if (!str_cmp (word, "Sk"))
                {
                    int sn;
                    int value;
                    char *temp;

                    value = fread_number (fp);
                    temp = fread_word (fp);
                    sn = skill_lookup (temp);
                    if (sn < 0)
                    {
                        logstr (LOG_BUG, "Fread_char: unknown skill '%s'", temp);
						fread_number (fp);
                    }
                    else {
                        ch->pcdata->learned[sn] = value;
						ch->pcdata->nSkillProgress[sn] = fread_number (fp);
					}
                    fMatch = TRUE;
                }

                break;

            case 'T':
                KEY ("TSex", ch->pcdata->true_sex, fread_number (fp));
                KEY ("TransCount", ch->trans_count, fread_number (fp));

                if (!str_cmp (word, "Titl"))
                {
                    ch->pcdata->title = fread_string (fp);
                    if (ch->pcdata->title[0] != '.'
                        && ch->pcdata->title[0] != ','
                        && ch->pcdata->title[0] != '!'
                        && ch->pcdata->title[0] != '?')
                    {
                        sprintf (buf, " %s", ch->pcdata->title);
                        free_string (ch->pcdata->title);
                        ch->pcdata->title = str_dup (buf);
                    }
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'V':
                KEY ("Version", ch->version, fread_number (fp));
                KEY ("Vers", ch->version, fread_number (fp));
                if (!str_cmp (word, "Vnum"))
                {
                    ch->pIndexData = get_mob_index (fread_number (fp));
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY ("Wimpy", ch->wimpy, fread_number (fp));
                KEY ("Wimp", ch->wimpy, fread_number (fp));
                KEY ("Wizn", ch->wiznet, fread_flag (fp));
                break;

			case 'Z':
				KEY ("Zenni", ch->zenni, fread_number (fp));
				break;
        }

        if (!fMatch)
        {
            logstr (LOG_BUG, "Fread_char: no match.", 0);
            logstr (LOG_BUG, word, 0);
            fread_to_eol (fp);
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet (CHAR_DATA * ch, FILE * fp)
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;
    int lastlogoff = current_time;
    int percent;

    /* first entry had BETTER be the vnum or we barf */
    word = feof (fp) ? "END" : fread_word (fp);
    if (!str_cmp (word, "Vnum"))
    {
        int vnum;

        vnum = fread_number (fp);
        if (get_mob_index (vnum) == NULL)
        {
            logstr (LOG_BUG, "Fread_pet: bad vnum %d.", vnum);
            pet = create_mobile (get_mob_index (MOB_VNUM_FIDO));
        }
        else
            pet = create_mobile (get_mob_index (vnum));
    }
    else
    {
        logstr (LOG_BUG, "Fread_pet: no vnum in file.", 0);
        pet = create_mobile (get_mob_index (MOB_VNUM_FIDO));
    }

    for (;;)
    {
        word = feof (fp) ? "END" : fread_word (fp);
        fMatch = FALSE;

        switch (UPPER (word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol (fp);
                break;

            case 'A':
                KEY ("Act", pet->act, fread_flag (fp));
                KEY ("AfBy", pet->affected_by, fread_flag (fp));
                KEY ("Alig", pet->alignment, fread_number (fp));

                
			    if (!str_cmp(word,"AC")) {
					int i;

					for	(i = 0; i < 4; i++)
						pet->armor[i] = fread_number(fp);
					fMatch = TRUE;
					break;
				}

                if (!str_cmp (word, "Affc"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect ();

                    sn = skill_lookup (fread_word (fp));
                    if (sn < 0)
                        logstr (LOG_BUG, "Fread_char: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->where = fread_number (fp);
					paf->skill_lvl = fread_number (fp);
                    paf->duration = fread_number (fp);
                    paf->modifier = fread_number (fp);
                    paf->location = fread_number (fp);
                    paf->bitvector = fread_number (fp);
                    paf->next = pet->affected;
                    pet->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "AMod"))
                {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++)
                        pet->mod_stat[stat] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "Attr"))
                {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++)
                        pet->perm_stat[stat] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY ("Clan", pet->clan, clan_lookup (fread_string (fp)));
                KEY ("Comm", pet->comm, fread_flag (fp));
                break;

            case 'D':
                KEY ("Dam", pet->damroll, fread_number (fp));
                KEY ("Desc", pet->description, fread_string (fp));
                break;

            case 'E':
                if (!str_cmp (word, "End"))
                {
                    pet->leader = ch;
                    pet->master = ch;
                    ch->pet = pet;
                    /* adjust hp ki up  -- here for speed's sake */
                    percent =
                        (current_time - lastlogoff) * 25 / (2 * 60 * 60);

                    if (percent > 0 && !IS_AFFECTED (ch, AFF_POISON)
                        && !IS_AFFECTED (ch, AFF_PLAGUE))
                    {
                        percent = UMIN (percent, 100);
                        pet->hit += (pet->max_hit - pet->hit) * percent / 100;
                        pet->ki +=
                            (pet->max_ki - pet->ki) * percent / 100;
                    }
                    return;
                }
                break;

            case 'H':
                KEY ("Hit", pet->hitroll, fread_number (fp));

                if (!str_cmp (word, "HK"))
                {
                    pet->hit = fread_number (fp);
                    pet->max_hit = fread_number (fp);
                    pet->ki = fread_number (fp);
                    pet->max_ki = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'L':
                KEY ("Levl", pet->level, fread_number (fp));
                KEY ("LnD", pet->long_descr, fread_string (fp));
                KEY ("LogO", lastlogoff, fread_number (fp));
                break;

            case 'N':
                KEY ("Name", pet->name, fread_string (fp));
                break;

            case 'P':
		        KEY ("Pos", pet->position, fread_number (fp));
                break;

            case 'R':
                KEY ("Race", pet->race, race_lookup (fread_string (fp)));
                break;

            case 'S':
                KEY ("Save", pet->saving_throw, fread_number (fp));
                KEY ("Sex", pet->sex, fread_number (fp));
                KEY ("ShD", pet->short_descr, fread_string (fp));
                KEY ("Stance", pet->stance, fread_number (fp));
                break;

	        case 'Z':
                KEY ("Zenni", pet->zenni, fread_number (fp));
                break;
        }
        if (!fMatch)
        {
            logstr (LOG_BUG, "Fread_pet: no match.", 0);
            fread_to_eol (fp);
        }
    }
}

extern OBJ_DATA *obj_free;

void fread_obj (CHAR_DATA * ch, FILE * fp)
{
    OBJ_DATA *obj;
    char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool make_new;                /* update object */

    fVnum = FALSE;
    obj = NULL;
    first = TRUE;                /* used to counter fp offset */
    make_new = FALSE;

    word = feof (fp) ? "End" : fread_word (fp);
    if (!str_cmp (word, "Vnum"))
    {
        int vnum;
        first = FALSE;            /* fp will be in right place */

        vnum = fread_number (fp);
        if (get_obj_index (vnum) == NULL)
        {
            logstr (LOG_BUG, "Fread_obj: bad vnum %d.", vnum);
        }
        else
        {
            obj = create_object (get_obj_index (vnum), -1);
        }

    }

    if (obj == NULL)
    {                            /* either not found or old style */
        obj = new_obj ();
        obj->name = str_dup ("");
        obj->short_descr = str_dup ("");
        obj->description = str_dup ("");
    }

    fNest = FALSE;
    fVnum = TRUE;
    iNest = 0;

    for (;;)
    {
        if (first)
            first = FALSE;
        else
            word = feof (fp) ? "End" : fread_word (fp);
        fMatch = FALSE;

        switch (UPPER (word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol (fp);
                break;

            case 'A':
                if (!str_cmp (word, "Affc"))
                {
                    AFFECT_DATA *paf;
                    int sn;
                    char *szSkill;

                    paf = new_affect ();

                    szSkill = fread_word (fp);
                    if (!str_cmp(szSkill, "none"))
                        paf->type = -1;
                    else {
                        sn = skill_lookup (szSkill);
                        if (sn < 0)
                            logstr (LOG_BUG, "Fread_obj: unknown skill.", 0);
                        else
                            paf->type = sn;
                    }

                    paf->where = fread_number (fp);
					paf->skill_lvl = fread_number (fp);
                    paf->duration = fread_number (fp);
                    paf->modifier = fread_number (fp);
                    paf->location = fread_number (fp);
                    paf->bitvector = fread_number (fp);
                    paf->next = obj->affected;
                    obj->affected = paf;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY ("Cond", obj->condition, fread_number (fp));
                KEY ("Cost", obj->cost, fread_number (fp));
                break;

            case 'D':
                KEY ("Description", obj->description, fread_string (fp));
                KEY ("Desc", obj->description, fread_string (fp));
                KEY ("Dur", obj->durability, fread_number (fp));
                break;

            case 'E':

                if (!str_cmp (word, "Enchanted"))
                {
                    obj->enchanted = TRUE;
                    fMatch = TRUE;
                    break;
                }

                KEY ("ExtraFlags", obj->extra_flags, fread_number (fp));
                KEY ("ExtF", obj->extra_flags, fread_number (fp));

                if (!str_cmp (word, "ExtraDescr") || !str_cmp (word, "ExDe"))
                {
                    EXTRA_DESCR_DATA *ed;

                    ed = new_extra_descr ();

                    ed->keyword = fread_string (fp);
                    ed->description = fread_string (fp);
                    ed->next = obj->extra_descr;
                    obj->extra_descr = ed;
                    fMatch = TRUE;
                }

                if (!str_cmp (word, "End"))
                {
                    if (!fNest || (fVnum && obj->pIndexData == NULL))
                    {
                        logstr (LOG_BUG, "Fread_obj: incomplete object.", 0);
                        free_obj (obj);
                        return;
                    }
                    else
                    {
                        if (!fVnum)
                        {
                            free_obj (obj);
                            obj =
                                create_object (get_obj_index (OBJ_VNUM_DUMMY),
                                               0);
                        }

                        if (make_new)
                        {
                            int wear;

                            wear = obj->wear_loc;
                            extract_obj (obj);

                            obj = create_object (obj->pIndexData, 0);
                            obj->wear_loc = wear;
                        }
                        if (iNest == 0 || rgObjNest[iNest] == NULL)
                            obj_to_char (obj, ch);
                        else
                            obj_to_obj (obj, rgObjNest[iNest - 1]);
                        return;
                    }
                }
                break;

            case 'I':
                KEY ("ItemType", obj->item_type, fread_number (fp));
                KEY ("Ityp", obj->item_type, fread_number (fp));
                break;

            case 'N':
                KEY ("Name", obj->name, fread_string (fp));

                if (!str_cmp (word, "Nest"))
                {
                    iNest = fread_number (fp);
                    if (iNest < 0 || iNest >= MAX_NEST)
                    {
                        logstr (LOG_BUG, "Fread_obj: bad nest %d.", iNest);
                    }
                    else
                    {
                        rgObjNest[iNest] = obj;
                        fNest = TRUE;
                    }
                    fMatch = TRUE;
                }
                break;

			case 'P':
				KEY ("Pl", obj->llPl, fread_number (fp));
				break;

            case 'S':
                KEY ("ShortDescr", obj->short_descr, fread_string (fp));
                KEY ("ShD", obj->short_descr, fread_string (fp));

                if (!str_cmp (word, "Spell"))
                {
                    int iValue;
                    int sn;

                    iValue = fread_number (fp);
                    sn = skill_lookup (fread_word (fp));
                    if (iValue < 0 || iValue > 3)
                    {
                        logstr (LOG_BUG, "Fread_obj: bad iValue %d.", iValue);
                    }
                    else if (sn < 0)
                    {
                        logstr (LOG_BUG, "Fread_obj: unknown skill.", 0);
                    }
                    else
                    {
                        obj->value[iValue] = sn;
                    }
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'T':
                KEY ("Timer", obj->timer, fread_number (fp));
                KEY ("Time", obj->timer, fread_number (fp));
                break;

            case 'V':
                if (!str_cmp (word, "Values") || !str_cmp (word, "Vals"))
                {
                    obj->value[0] = fread_number (fp);
                    obj->value[1] = fread_number (fp);
                    obj->value[2] = fread_number (fp);
                    obj->value[3] = fread_number (fp);
                    if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                        obj->value[0] = obj->pIndexData->value[0];
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "Val"))
                {
                    obj->value[0] = fread_number (fp);
                    obj->value[1] = fread_number (fp);
                    obj->value[2] = fread_number (fp);
                    obj->value[3] = fread_number (fp);
                    obj->value[4] = fread_number (fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp (word, "Vnum"))
                {
                    int vnum;

                    vnum = fread_number (fp);
                    if ((obj->pIndexData = get_obj_index (vnum)) == NULL)
                        logstr (LOG_BUG, "Fread_obj: bad vnum %d.", vnum);
                    else
                        fVnum = TRUE;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY ("WearFlags", obj->wear_flags, fread_number (fp));
                KEY ("WeaF", obj->wear_flags, fread_number (fp));
                KEY ("WearLoc", obj->wear_loc, fread_number (fp));
                KEY ("Wear", obj->wear_loc, fread_number (fp));
                KEY ("Weight", obj->weight, fread_number (fp));
                KEY ("Wt", obj->weight, fread_number (fp));
                break;

        }

        if (!fMatch)
        {
            logstr (LOG_BUG, "Fread_obj: no match.", 0);
            fread_to_eol (fp);
        }
    }
}
