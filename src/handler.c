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

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"

/*
 * Local functions.
 */
void affect_modify args ((CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd));


/* friend stuff -- for NPC's mostly */
bool is_friend (CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (is_same_group (ch, victim))
        return TRUE;


    if (!IS_NPC (ch))
        return FALSE;

    if (!IS_NPC (victim))
    {
        if (IS_SET (ch->off_flags, ASSIST_PLAYERS))
            return TRUE;
        else
            return FALSE;
    }

    if (IS_AFFECTED (ch, AFF_CHARM))
        return FALSE;

    if (IS_SET (ch->off_flags, ASSIST_ALL))
        return TRUE;

    if (ch->group && ch->group == victim->group)
        return TRUE;

    if (IS_SET (ch->off_flags, ASSIST_VNUM)
        && ch->pIndexData == victim->pIndexData)
        return TRUE;

    if (IS_SET (ch->off_flags, ASSIST_RACE) && ch->race == victim->race)
        return TRUE;

    if (IS_SET (ch->off_flags, ASSIST_ALIGN)
        && !IS_SET (ch->act, ACT_NOALIGN)
        && !IS_SET (victim->act, ACT_NOALIGN)
        && ((IS_GOOD (ch) && IS_GOOD (victim))
            || (IS_EVIL (ch) && IS_EVIL (victim)) || (IS_NEUTRAL (ch)
                                                      &&
                                                      IS_NEUTRAL (victim))))
        return TRUE;

    return FALSE;
}

/* returns number of people on an object */
int count_users (OBJ_DATA * obj)
{
    CHAR_DATA *fch;
    int count = 0;

    if (obj->in_room == NULL)
        return 0;

    for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
        if (fch->on == obj)
            count++;

    return count;
}

/* returns material number */
int material_lookup (const char *name)
{
    return 0;
}

int weapon_lookup (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER (name[0]) == LOWER (weapon_table[type].name[0])
            && !str_prefix (name, weapon_table[type].name))
            return type;
    }

    return -1;
}

int weapon_type (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER (name[0]) == LOWER (weapon_table[type].name[0])
            && !str_prefix (name, weapon_table[type].name))
            return weapon_table[type].type;
    }

    return WEAPON_EXOTIC;
}

char *item_name (int item_type)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
        if (item_type == item_table[type].type)
            return item_table[type].name;
    return "none";
}

char *weapon_name (int weapon_type)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
    return "exotic";
}

int attack_lookup (const char *name)
{
    int att;

    for (att = 0; attack_table[att].name != NULL; att++)
    {
        if (LOWER (name[0]) == LOWER (attack_table[att].name[0])
            && !str_prefix (name, attack_table[att].name))
            return att;
    }

    return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup (const char *name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
        if (LOWER (name[0]) == LOWER (wiznet_table[flag].name[0])
            && !str_prefix (name, wiznet_table[flag].name))
            return flag;
    }

    return -1;
}


/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int check_immune (CHAR_DATA * ch, int dam_type)
{
    int immune, def;
    int bit;

    immune = -1;
    def = IS_NORMAL;

    if (dam_type == DAM_NONE)
        return immune;

    if (dam_type <= 3)
    {
        if (IS_SET (ch->imm_flags, IMM_WEAPON))
            def = IS_IMMUNE;
        else if (IS_SET (ch->res_flags, RES_WEAPON))
            def = IS_RESISTANT;
        else if (IS_SET (ch->vuln_flags, VULN_WEAPON))
            def = IS_VULNERABLE;
    }
    else
    {                            /* magical attack */

        if (IS_SET (ch->imm_flags, IMM_MAGIC))
            def = IS_IMMUNE;
        else if (IS_SET (ch->res_flags, RES_MAGIC))
            def = IS_RESISTANT;
        else if (IS_SET (ch->vuln_flags, VULN_MAGIC))
            def = IS_VULNERABLE;
    }

    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    switch (dam_type)
    {
        case (DAM_BASH):
            bit = IMM_BASH;
            break;
        case (DAM_PIERCE):
            bit = IMM_PIERCE;
            break;
        case (DAM_SLASH):
            bit = IMM_SLASH;
            break;
        case (DAM_FIRE):
            bit = IMM_FIRE;
            break;
        case (DAM_COLD):
            bit = IMM_COLD;
            break;
        case (DAM_LIGHTNING):
            bit = IMM_LIGHTNING;
            break;
        case (DAM_ACID):
            bit = IMM_ACID;
            break;
        case (DAM_POISON):
            bit = IMM_POISON;
            break;
        case (DAM_NEGATIVE):
            bit = IMM_NEGATIVE;
            break;
        case (DAM_HOLY):
            bit = IMM_HOLY;
            break;
        case (DAM_ENERGY):
            bit = IMM_ENERGY;
            break;
        case (DAM_MENTAL):
            bit = IMM_MENTAL;
            break;
        case (DAM_DISEASE):
            bit = IMM_DISEASE;
            break;
        case (DAM_DROWNING):
            bit = IMM_DROWNING;
            break;
        case (DAM_LIGHT):
            bit = IMM_LIGHT;
            break;
        case (DAM_CHARM):
            bit = IMM_CHARM;
            break;
        case (DAM_SOUND):
            bit = IMM_SOUND;
            break;
        default:
            return def;
    }

    if (IS_SET (ch->imm_flags, bit))
        immune = IS_IMMUNE;
    else if (IS_SET (ch->res_flags, bit) && immune != IS_IMMUNE)
        immune = IS_RESISTANT;
    else if (IS_SET (ch->vuln_flags, bit))
    {
        if (immune == IS_IMMUNE)
            immune = IS_RESISTANT;
        else if (immune == IS_RESISTANT)
            immune = IS_NORMAL;
        else
            immune = IS_VULNERABLE;
    }

    if (immune == -1)
        return def;
    else
        return immune;
}

bool is_clan (CHAR_DATA * ch)
{
    return ch->clan;
}

bool is_same_clan (CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (clan_table[ch->clan].independent)
        return FALSE;
    else
        return (ch->clan == victim->clan);
}

int skill_prereq_total[MAX_SKILL] = {-1};

// This adds up the levels of skills needed to get the skill, and
// the levels to get those skills (so on and so forth, recursively)
int PrereqTotal (int nSn) {
    int i, nTotal = 0;
    if (nSn < 0 || nSn > MAX_SKILL)
        return 0;
    for (i = 0; i < 5; ++i) {
        if (skill_table[nSn].skill_prereq[i] == NULL)
            break;
        nTotal += skill_table[nSn].skill_value[i];
        nTotal += PrereqTotal (skill_lookup(skill_table[nSn].skill_prereq[i]));
    }
    return nTotal;
}

int get_skill (CHAR_DATA * ch, int sn)
{
    int skill;

	// Well, can't use difficulty or powerlevel in deciding skills because
	// those values are based on skills. Instead, use stats

	// Level based skills
    if (sn == -1)
        skill = get_curr_stat (ch, STAT_INT);
    else if (sn < -1 || sn > MAX_SKILL) {
        logstr (LOG_BUG, "Bad sn %d in get_skill", sn);
        skill = 0;
    }
    else if (!IS_NPC (ch) && ch->pcdata)
        skill = ch->pcdata->learned[sn];
	else {
		if (!MeetsPrereq(ch, sn))
            skill = 0;
        else {
            // Calculate the total prerequisites to attain each skill
            if (skill_prereq_total[0] == -1) {
                int i;
                for (i = 0; i < MAX_SKILL; ++i)
                    skill_prereq_total[i] = PrereqTotal(i);
            }
            skill = get_curr_stat (ch, STAT_INT) - skill_prereq_total[sn];

            // Bonuses if the bits for skills are set
            if ((sn == gsn_dodge && IS_SET (ch->off_flags, OFF_DODGE))
                || (sn == gsn_parry && IS_SET (ch->off_flags, OFF_PARRY)))
                skill = 4 * skill / 3;
            else if (sn == gsn_dodge || sn == gsn_parry)
                skill = 5 * skill / 4;
            else if (sn == gsn_shield_block)
                skill = 5 * skill / 4;
            else if (sn == gsn_bash && IS_SET (ch->off_flags, OFF_BASH))
                skill = 5 * skill / 4;
            else if (sn == gsn_disarm && IS_SET (ch->off_flags, OFF_DISARM))
                skill = 5 * skill / 4;
            else if (sn == gsn_berserk && IS_SET (ch->off_flags, OFF_BERSERK))
                skill = 5 * skill / 4;
            else if (sn == gsn_hand_to_hand)
                skill = 3 * skill / 2;
        }
    }
    if (ch->daze > 0)
	    skill = 2 * skill / 3;
    if (!IS_NPC (ch) && ch->pcdata->condition[COND_DRUNK] > 10)
        skill = 9 * skill / 10;

    return UMAX (0, skill);
}

/* for returning weapon information */
int get_weapon_sn (CHAR_DATA * ch)
{
    OBJ_DATA *wield;
    int sn;

    wield = get_eq_char (ch, WEAR_WIELD);
    if (wield == NULL || wield->item_type != ITEM_WEAPON)
        sn = gsn_hand_to_hand;
    else
        switch (wield->value[0])
        {
            default:
                sn = -1;
                break;
            case (WEAPON_SWORD):
                sn = gsn_sword;
                break;
            case (WEAPON_DAGGER):
                sn = gsn_dagger;
                break;
            case (WEAPON_SPEAR):
                sn = gsn_spear;
                break;
            case (WEAPON_MACE):
                sn = gsn_mace;
                break;
            case (WEAPON_AXE):
                sn = gsn_axe;
                break;
            case (WEAPON_FLAIL):
                sn = gsn_flail;
                break;
            case (WEAPON_WHIP):
                sn = gsn_whip;
                break;
            case (WEAPON_POLEARM):
                sn = gsn_polearm;
                break;
            case (WEAPON_EXOTIC):
                sn = gsn_exotic;
                break;
        }
    return sn;
}


/* used to de-screw characters */
void reset_char (CHAR_DATA * ch)
{
    long long int mod;
	int loc, stat, i;
	OBJ_DATA *obj;
    AFFECT_DATA *af;

    if (IS_NPC (ch))
        return;

    if (ch->pcdata->perm_hit == 0
        || ch->pcdata->perm_ki == 0
        || ch->pcdata->last_level == 0)
    {
        /* do a FULL reset */
        for (loc = 0; loc < MAX_WEAR; loc++)
        {
            obj = get_eq_char (ch, loc);
            if (obj == NULL)
                continue;
            if (!obj->enchanted)
                for (af = obj->pIndexData->affected; af != NULL;
                     af = af->next)
                {
                    mod = af->modifier;
                    switch (af->location)
                    {
                        case APPLY_SEX:
                            ch->sex -= mod;
                            if (ch->sex < 0 || ch->sex > 2)
                                ch->sex =
                                    IS_NPC (ch) ? 0 : ch->pcdata->true_sex;
                            break;
                        case APPLY_KI:
                            ch->max_ki -= mod;
                            break;
                        case APPLY_HIT:
                            ch->max_hit -= mod;
                            break;
                    }
                }

            for (af = obj->affected; af != NULL; af = af->next)
            {
                mod = af->modifier;
                switch (af->location)
                {
                    case APPLY_SEX:
                        ch->sex -= mod;
                        break;
                    case APPLY_KI:
                        ch->max_ki -= mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit -= mod;
                        break;
                }
            }
        }
        /* now reset the permanent stats */
        ch->pcdata->perm_hit = ch->max_hit;
        ch->pcdata->perm_ki = ch->max_ki;
        ch->pcdata->last_level = ch->played / 3600;
        if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
        {
            if (ch->sex > 0 && ch->sex < 3)
                ch->pcdata->true_sex = ch->sex;
            else
                ch->pcdata->true_sex = 0;
        }

    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->mod_stat[stat] = 0;

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
        ch->pcdata->true_sex = 0;
    ch->sex = ch->pcdata->true_sex;
    ch->max_hit = ch->pcdata->perm_hit;
    ch->max_ki = ch->pcdata->perm_ki;

    for (i = 0; i < 4; i++)
    	ch->armor[i] = 0;

    ch->hitroll = 0;
    ch->damroll = 0;
    ch->saving_throw = 0;

    /* now start adding back the effects */
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
        obj = get_eq_char (ch, loc);
        if (obj == NULL)
            continue;

		for (i = 0; i < 4; i++)
			ch->armor[i] += apply_ac(obj, loc, i);

        if (!obj->enchanted)
            for (af = obj->pIndexData->affected; af != NULL; af = af->next)
            {
                mod = af->modifier;
                switch (af->location)
                {
                    case APPLY_STR:
                        ch->mod_stat[STAT_STR] += mod;
						ch->max_hit += mod * 5;
						ch->max_ki += mod * 5;
                        break;
                    case APPLY_DEX:
                        ch->mod_stat[STAT_DEX] += mod;
                        break;
                    case APPLY_INT:
                        ch->mod_stat[STAT_INT] += mod;
                        break;
                    case APPLY_WIL:
                        ch->mod_stat[STAT_WIL] += mod;
						ch->max_ki += mod * 5;
                        break;
                    case APPLY_CHA:
                        ch->mod_stat[STAT_CHA] += mod;
                        break;
					case APPLY_AC:
						for (i = 0; i < 4; i ++)
							ch->armor[i] += mod;
						break;

                    case APPLY_SEX:
                        ch->sex += mod;
                        break;
                    case APPLY_KI:
                        ch->max_ki += mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit += mod;
                        break;
                    case APPLY_HITROLL:
                        ch->hitroll += mod;
                        break;
                    case APPLY_DAMROLL:
                        ch->damroll += mod;
                        break;

                    case APPLY_SAVES:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_ROD:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_PETRI:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_BREATH:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_SPELL:
                        ch->saving_throw += mod;
                        break;
                }
            }

        for (af = obj->affected; af != NULL; af = af->next)
        {
            mod = af->modifier;
            switch (af->location)
            {
                case APPLY_STR:
                    ch->mod_stat[STAT_STR] += mod;
					ch->max_hit += mod * 5;
					ch->max_ki += mod * 5;
                    break;
                case APPLY_DEX:
                    ch->mod_stat[STAT_DEX] += mod;
                    break;
                case APPLY_INT:
                    ch->mod_stat[STAT_INT] += mod;
                    break;
                case APPLY_WIL:
                    ch->mod_stat[STAT_WIL] += mod;
					ch->max_ki += mod * 5;
                    break;
                case APPLY_CHA:
                    ch->mod_stat[STAT_CHA] += mod;
                    break;
				case APPLY_AC:
					for (i = 0; i < 4; i ++)
						ch->armor[i] += mod;
					break;
                case APPLY_SEX:
                    ch->sex += mod;
                    break;
                case APPLY_KI:
                    ch->max_ki += mod;
                    break;
                case APPLY_HIT:
                    ch->max_hit += mod;
                    break;
                case APPLY_HITROLL:
                    ch->hitroll += mod;
                    break;
                case APPLY_DAMROLL:
                    ch->damroll += mod;
                    break;

                case APPLY_SAVES:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_ROD:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_PETRI:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_BREATH:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_SPELL:
                    ch->saving_throw += mod;
                    break;
            }
        }
    }

    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
    {
        mod = af->modifier;
        switch (af->location)
        {
            case APPLY_STR:
                ch->mod_stat[STAT_STR] += mod;
				ch->max_hit += mod * 5;
				ch->max_ki += mod * 5;
                break;
            case APPLY_DEX:
                ch->mod_stat[STAT_DEX] += mod;
                break;
            case APPLY_INT:
                ch->mod_stat[STAT_INT] += mod;
                break;
            case APPLY_WIL:
                ch->mod_stat[STAT_WIL] += mod;
				ch->max_ki += mod * 5;
                break;
            case APPLY_CHA:
                ch->mod_stat[STAT_CHA] += mod;
                break;
			case APPLY_AC:
				for (i = 0; i < 4; i ++)
					ch->armor[i] += mod;
				break;

            case APPLY_SEX:
                ch->sex += mod;
                break;
            case APPLY_KI:
                ch->max_ki += mod;
                break;
            case APPLY_HIT:
                ch->max_hit += mod;
                break;
            case APPLY_HITROLL:
                ch->hitroll += mod;
                break;
            case APPLY_DAMROLL:
                ch->damroll += mod;
                break;

            case APPLY_SAVES:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_ROD:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_PETRI:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_BREATH:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_SPELL:
                ch->saving_throw += mod;
                break;
        }
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
        ch->sex = ch->pcdata->true_sex;

	ResetDiff(ch);
}


/*
 * Retrieve a character's age.
 */
int get_age (CHAR_DATA * ch)
{
    return 17 + (ch->played + (int) (current_time - ch->logon)) / 72000;
}

/* command for retrieving stats */
int get_curr_stat (CHAR_DATA * ch, int stat)
{
    /*return 1000;
    */
    int max;

    if (IS_NPC (ch) || ch->level > LEVEL_IMMORTAL)
        max = 1000;

    else
    {
        max = pc_race_table[ch->race].max_stats[stat];

        //if (class_table[ch->class].attr_prime == stat)
        //    max += 2;

        //if (ch->race == race_lookup ("human"))
        //    max += 1;

        max = UMIN (max, 1000);
    }

    return URANGE (1, ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n (CHAR_DATA * ch)
{
    if (!IS_NPC (ch) && ch->level >= LEVEL_IMMORTAL)
        return 1000;

    if (IS_NPC (ch) && IS_SET (ch->act, ACT_PET))
        return 0;

    return MAX_WEAR + get_curr_stat (ch, STAT_STR) + 5;
}



/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w (CHAR_DATA * ch)
{
    if (!IS_NPC (ch) && ch->level >= LEVEL_IMMORTAL)
        return 10000000;

    if (IS_NPC (ch) && IS_SET (ch->act, ACT_PET))
        return 0;

    return 500 + get_curr_stat (ch, STAT_STR) * 100;
}



/*
 * See if a string is one of the names of an object.
 */

bool is_name (char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
        return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
        return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for (;;)
    {                            /* start parsing string */
        str = one_argument (str, part);

        if (part[0] == '\0')
            return TRUE;

        /* check to see if this is part of namelist */
        list = namelist;
        for (;;)
        {                        /* start parsing namelist */
            list = one_argument (list, name);
            if (name[0] == '\0')    /* this name was not found */
                return FALSE;

            if (!str_prefix (string, name))
                return TRUE;    /* full pattern match */

            if (!str_prefix (part, name))
                break;
        }
    }
}

bool is_exact_name (char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL)
        return FALSE;

    for (;;)
    {
        namelist = one_argument (namelist, name);
        if (name[0] == '\0')
            return FALSE;
        if (!str_cmp (str, name))
            return TRUE;
    }
}

/* enchanted stuff for eq */
void affect_enchant (OBJ_DATA * obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *paf, *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            af_new = new_affect ();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX (0, paf->type);
			af_new->skill_lvl = paf->skill_lvl;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }
}


/*
 * Apply or remove an affect to a character.
 */
void affect_modify (CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd)
{
    long long int mod;
    int i;

    mod = paf->modifier;

    if (fAdd)
    {
        switch (paf->where)
        {
            case TO_AFFECTS:
                SET_BIT (ch->affected_by, paf->bitvector);
                break;
            case TO_IMMUNE:
                SET_BIT (ch->imm_flags, paf->bitvector);
                break;
            case TO_RESIST:
                SET_BIT (ch->res_flags, paf->bitvector);
                break;
            case TO_VULN:
                SET_BIT (ch->vuln_flags, paf->bitvector);
                break;
        }
    }
    else
    {
        switch (paf->where)
        {
            case TO_AFFECTS:
                REMOVE_BIT (ch->affected_by, paf->bitvector);
                break;
            case TO_IMMUNE:
                REMOVE_BIT (ch->imm_flags, paf->bitvector);
                break;
            case TO_RESIST:
                REMOVE_BIT (ch->res_flags, paf->bitvector);
                break;
            case TO_VULN:
                REMOVE_BIT (ch->vuln_flags, paf->bitvector);
                break;
        }
        mod = 0 - mod;
    }

    switch (paf->location)
    {
        default:
            logstr (LOG_BUG, "Affect_modify: unknown location %d.", paf->location);
            return;

        case APPLY_NONE:
            break;
        case APPLY_STR:
			ch->mod_stat[STAT_STR] += mod;
			ch->max_hit += mod * HP_STR;
			ch->max_ki += mod * KI_STR;
            break;
        case APPLY_DEX:
			ch->mod_stat[STAT_DEX] += mod;
            break;
        case APPLY_INT:
            ch->mod_stat[STAT_INT] += mod;
            break;
        case APPLY_WIL:
			ch->mod_stat[STAT_WIL] += mod;
			ch->max_hit += mod * HP_WIL;
            ch->max_ki += mod * KI_WIL;
            break;
        case APPLY_CHA:
			ch->mod_stat[STAT_CHA] += mod;
            break;
		case APPLY_AC:
			for (i = 0; i < 4; i ++)
				ch->armor[i] += mod;
			break;
        case APPLY_SEX:
            ch->sex += mod;
            break;
        case APPLY_LEVEL:
            break;
        case APPLY_AGE:
            break;
        case APPLY_HEIGHT:
            break;
        case APPLY_WEIGHT:
            break;
        case APPLY_KI:
            ch->max_ki += mod;
            break;
        case APPLY_HIT:
            ch->max_hit += mod;
            break;
        case APPLY_ZENNI:
            break;
        case APPLY_EXP:
            break;
        case APPLY_HITROLL:
            ch->hitroll += mod;
            break;
        case APPLY_DAMROLL:
            ch->damroll += mod;
            break;
        case APPLY_SAVES:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_ROD:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_PETRI:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_BREATH:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_SPELL:
            ch->saving_throw += mod;
            break;
        case APPLY_SPELL_AFFECT:
            break;
    }

    ResetDiff(ch);
    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
/*
	if (!IS_NPC (ch) && (wield = get_eq_char (ch, WEAR_WIELD)) != NULL
        && get_obj_weight (wield) >
        (str_app[get_curr_stat (ch, STAT_STR)].wield * 10))
    {
        static int depth;

        if (depth == 0)
        {
            depth++;
            act ("You drop $p.", ch, wield, NULL, TO_CHAR);
            act ("$n drops $p.", ch, wield, NULL, TO_ROOM);
            obj_from_char (wield);
            obj_to_room (wield, ch->in_room);
            depth--;
        }
    }
*/
    return;
}


/* find an effect in an affect list */
AFFECT_DATA *affect_find (AFFECT_DATA * paf, int sn)
{
    AFFECT_DATA *paf_find;
    for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next)
        if (paf_find->type == sn)
            return paf_find;
    return NULL;
}

// Find an effect in an affect list.
// Based on bit vector
AFFECT_DATA *affect_find_bit (AFFECT_DATA * paf, int bit)
{
    AFFECT_DATA *paf_find;
    for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next)
        if (paf_find->bitvector & bit)
            return paf_find;
    return NULL;
}


/* fix object affects when removing one */
void affect_check (CHAR_DATA * ch, int where, int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
        return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
        if (paf->where == where && paf->bitvector == vector)
        {
            switch (where)
            {
                case TO_AFFECTS:
                    SET_BIT (ch->affected_by, vector);
                    break;
                case TO_IMMUNE:
                    SET_BIT (ch->imm_flags, vector);
                    break;
                case TO_RESIST:
                    SET_BIT (ch->res_flags, vector);
                    break;
                case TO_VULN:
                    SET_BIT (ch->vuln_flags, vector);
                    break;
            }
            return;
        }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == -1)
            continue;

        for (paf = obj->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT (ch->affected_by, vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT (ch->imm_flags, vector);
                        break;
                    case TO_RESIST:
                        SET_BIT (ch->res_flags, vector);
                        break;
                    case TO_VULN:
                        SET_BIT (ch->vuln_flags, vector);

                }
                return;
            }

        if (obj->enchanted)
            continue;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT (ch->affected_by, vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT (ch->imm_flags, vector);
                        break;
                    case TO_RESIST:
                        SET_BIT (ch->res_flags, vector);
                        break;
                    case TO_VULN:
                        SET_BIT (ch->vuln_flags, vector);
                        break;
                }
                return;
            }
    }
}

/*
 * Give an affect to a char.
 */
void affect_to_char (CHAR_DATA * ch, AFFECT_DATA * paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect ();

    *paf_new = *paf;

    VALIDATE (paf);                /* in case we missed it when we set up paf */
    paf_new->next = ch->affected;
    ch->affected = paf_new;

    affect_modify (ch, paf_new, TRUE);
    return;
}

/* give an affect to an object */
void affect_to_obj (OBJ_DATA * obj, AFFECT_DATA * paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect ();

    *paf_new = *paf;

    VALIDATE (paf);                /* in case we missed it when we set up paf */
    paf_new->next = obj->affected;
    obj->affected = paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        switch (paf->where)
        {
            case TO_OBJECT:
                SET_BIT (obj->extra_flags, paf->bitvector);
                break;
            case TO_WEAPON:
                if (obj->item_type == ITEM_WEAPON)
                    SET_BIT (obj->value[4], paf->bitvector);
                break;
        }


    return;
}



/*
 * Remove an affect from a char.
 */
void affect_remove (CHAR_DATA * ch, AFFECT_DATA * paf)
{
    int where;
    int vector;

    if (ch->affected == NULL)
    {
        logstr (LOG_BUG, "Affect_remove: no affect.", 0);
        return;
    }

    affect_modify (ch, paf, FALSE);
    where = paf->where;
    vector = paf->bitvector;

    if (paf == ch->affected)
    {
        ch->affected = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for (prev = ch->affected; prev != NULL; prev = prev->next)
        {
            if (prev->next == paf)
            {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL)
        {
            logstr (LOG_BUG, "Affect_remove: cannot find paf.", 0);
            return;
        }
    }

    free_affect (paf);

    affect_check (ch, where, vector);
    return;
}

void affect_remove_obj (OBJ_DATA * obj, AFFECT_DATA * paf)
{
    int where, vector;
    if (obj->affected == NULL)
    {
        logstr (LOG_BUG, "Affect_remove_object: no affect.", 0);
        return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_modify (obj->carried_by, paf, FALSE);

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
        switch (paf->where)
        {
            case TO_OBJECT:
                REMOVE_BIT (obj->extra_flags, paf->bitvector);
                break;
            case TO_WEAPON:
                if (obj->item_type == ITEM_WEAPON)
                    REMOVE_BIT (obj->value[4], paf->bitvector);
                break;
        }

    if (paf == obj->affected)
    {
        obj->affected = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for (prev = obj->affected; prev != NULL; prev = prev->next)
        {
            if (prev->next == paf)
            {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL)
        {
            logstr (LOG_BUG, "Affect_remove_object: cannot find paf.", 0);
            return;
        }
    }

    free_affect (paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_check (obj->carried_by, where, vector);
    return;
}



/*
 * Strip all affects of a given sn.
 */
void affect_strip (CHAR_DATA * ch, int sn)
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;
        if (paf->type == sn)
            affect_remove (ch, paf);
    }

    return;
}



/*
 * Return true if a char is affected by a spell.
 */
bool is_affected (CHAR_DATA * ch, int sn)
{
    AFFECT_DATA *paf;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type == sn)
            return TRUE;
    }

    return FALSE;
}



/*
 * Add or enhance an affect.
 */
void affect_join (CHAR_DATA * ch, AFFECT_DATA * paf)
{
    AFFECT_DATA *paf_old;
    bool found;

    found = FALSE;
    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next)
    {
        if (paf_old->type == paf->type)
        {
			paf->skill_lvl = (paf->skill_lvl + paf_old->skill_lvl) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove (ch, paf_old);
            break;
        }
    }

    affect_to_char (ch, paf);
    return;
}



/*
 * Move a char out of a room.
 */
void char_from_room (CHAR_DATA * ch)
{
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
    {
        logstr (LOG_BUG, "Char_from_room: NULL.", 0);
        return;
    }

    if (!IS_NPC (ch))
        --ch->in_room->area->nplayer;

    if ((obj = get_eq_char (ch, WEAR_LIGHT)) != NULL
        && obj->item_type == ITEM_LIGHT
        && obj->value[2] != 0 && ch->in_room->light > 0)
        --ch->in_room->light;

    // Stop any teaching/learning.
	// Turn this into a function?
	{
		CHAR_DATA *pCur;
		for (pCur = ch->in_room->people; pCur; pCur = pCur->next_in_room) {
			if (!IS_NPC (pCur) && pCur->pcdata->pTeacher == ch) {
				sendch ("Your teacher has left.\n\r", pCur);
				pCur->pcdata->pTeacher = NULL;
			}
		}
		if (!IS_NPC (ch))
			ch->pcdata->nTeachSn = -1;

	}

    if (ch == ch->in_room->people)
    {
        ch->in_room->people = ch->next_in_room;
    }
    else
    {
        CHAR_DATA *prev;

        for (prev = ch->in_room->people; prev; prev = prev->next_in_room)
        {
            if (prev->next_in_room == ch)
            {
                prev->next_in_room = ch->next_in_room;
                break;
            }
        }

        if (prev == NULL)
            logstr (LOG_BUG, "Char_from_room: ch not found.", 0);
    }

    ch->in_room = NULL;
    ch->next_in_room = NULL;
    ch->on = NULL;                /* sanity check! */
    return;
}



/*
 * Move a char into a room.
 */
void char_to_room (CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex)
{
    OBJ_DATA *obj;

    if (pRoomIndex == NULL)
    {
        ROOM_INDEX_DATA *room;

        logstr (LOG_BUG, "Char_to_room: NULL.", 0);

        if ((room = get_room_index (ROOM_VNUM_TEMPLE)) != NULL)
            char_to_room (ch, room);

        return;
    }

    ch->in_room = pRoomIndex;
    ch->next_in_room = pRoomIndex->people;
    pRoomIndex->people = ch;

    if (!IS_NPC (ch))
    {
        if (ch->in_room->area->empty)
        {
            ch->in_room->area->empty = FALSE;
            ch->in_room->area->age = 0;
        }
        ++ch->in_room->area->nplayer;
    }

    if ((obj = get_eq_char (ch, WEAR_LIGHT)) != NULL
        && obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
        ++ch->in_room->light;

/* removed
    if (IS_AFFECTED (ch, AFF_PLAGUE))
    {
        AFFECT_DATA *af, plague;
        CHAR_DATA *vch;

        for (af = ch->affected; af != NULL; af = af->next)
        {
            if (af->type == gsn_plague)
                break;
        }

        if (af == NULL)
        {
            REMOVE_BIT (ch->affected_by, AFF_PLAGUE);
            return;
        }

        if (af->level == 1)
            return;

        plague.where = TO_AFFECTS;
        plague.type = gsn_plague;
        plague.level = af->level - 1;
        plague.duration = number_range (1, 2 * plague.level);
        plague.location = APPLY_STR;
        plague.modifier = -5;
        plague.bitvector = AFF_PLAGUE;

        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell (plague.level - 2, vch, DAM_DISEASE)
                && !IS_IMMORTAL (vch) &&
                !IS_AFFECTED (vch, AFF_PLAGUE) && number_bits (6) == 0)
            {
                sendch ("You feel hot and feverish.\n\r", vch);
                act ("$n shivers and looks very ill.", vch, NULL, NULL,
                     TO_ROOM);
                affect_join (vch, &plague);
            }
        }
    }
*/

    return;
}



/*
 * Give an obj to a char.
 */
void obj_to_char (OBJ_DATA * obj, CHAR_DATA * ch)
{
    obj->next_content = ch->carrying;
    ch->carrying = obj;
    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += get_obj_number (obj);
    ch->carry_weight += get_obj_weight (obj);
}



/*
 * Take an obj from its character.
 */
void obj_from_char (OBJ_DATA * obj)
{
    CHAR_DATA *ch;

    if ((ch = obj->carried_by) == NULL)
    {
        logstr (LOG_BUG, "Obj_from_char: null ch.", 0);
        return;
    }

    if (obj->wear_loc != WEAR_NONE)
        unequip_char (ch, obj);

    if (ch->carrying == obj)
    {
        ch->carrying = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = ch->carrying; prev != NULL; prev = prev->next_content)
        {
            if (prev->next_content == obj)
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if (prev == NULL)
            logstr (LOG_BUG, "Obj_from_char: obj not in list.", 0);
    }

    obj->carried_by = NULL;
    obj->next_content = NULL;
    ch->carry_number -= get_obj_number (obj);
    ch->carry_weight -= get_obj_weight (obj);
    return;
}



/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac (OBJ_DATA * obj, int iWear, int type)
{
    if (obj->item_type != ITEM_ARMOR)
        return 0;

    switch (iWear) {
        case WEAR_BODY:
            return 3 * obj->value[type];

        case WEAR_HEAD:
        case WEAR_LEGS:
        case WEAR_ABOUT:
            return 2 * obj->value[type];

        case WEAR_FEET:
        case WEAR_HANDS:
        case WEAR_ARMS:
        case WEAR_SHIELD:
        case WEAR_NECK_1:
        case WEAR_NECK_2:
        case WEAR_WAIST:
        case WEAR_WRIST_L:
        case WEAR_WRIST_R:
        case WEAR_HOLD:
        case WEAR_EYE:
        case WEAR_FINGER_L:
        case WEAR_FINGER_R:
        case WEAR_EAR_L:
        case WEAR_EAR_R:
        case WEAR_TAIL:
            return obj->value[type];
    }

    return 0;
}



/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char (CHAR_DATA * ch, int iWear)
{
    OBJ_DATA *obj;

    if (ch == NULL)
        return NULL;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == iWear)
            return obj;
    }

    return NULL;
}



/*
 * Equip a char with an obj.
 */
void equip_char (CHAR_DATA * ch, OBJ_DATA * obj, int iWear)
{
    int i;
    AFFECT_DATA *paf;

    if (get_eq_char (ch, iWear) != NULL)
    {
        logstr (LOG_BUG, "Equip_char: already equipped (%d).", iWear);
        return;
    }

    if ((IS_OBJ_STAT (obj, ITEM_ANTI_EVIL) && IS_EVIL (ch))
        || (IS_OBJ_STAT (obj, ITEM_ANTI_GOOD) && IS_GOOD (ch))
        || (IS_OBJ_STAT (obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL (ch)))
    {
        /*
         * Thanks to Morgenes for the bug fix here!
         */
        act ("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
        act ("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
        obj_from_char (obj);
        obj_to_room (obj, ch->in_room);
        return;
    }

    if (IS_OBJ_STAT (obj, ITEM_NEED_PL) && ch->llPl < obj->llPl) {
	sendch ("Your powerlevel is not high enough to use this item!\n\r",ch);
	return;
    }

    for (i = 0; i < 4; i++)
    	ch->armor[i] += apply_ac(obj, iWear, i);

    obj->wear_loc = iWear;

    if (!obj->enchanted)
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->location != APPLY_SPELL_AFFECT)
                affect_modify (ch, paf, TRUE);
    for (paf = obj->affected; paf != NULL; paf = paf->next)
        if (paf->location == APPLY_SPELL_AFFECT)
            affect_to_char (ch, paf);
        else
            affect_modify (ch, paf, TRUE);

    if (obj->item_type == ITEM_LIGHT
        && obj->value[2] != 0 && ch->in_room != NULL) ++ch->in_room->light;

    return;
}



/*
 * Unequip a char with an obj.
 */
void unequip_char (CHAR_DATA * ch, OBJ_DATA * obj)
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int i;

    if (obj->wear_loc == WEAR_NONE)
    {
        logstr (LOG_BUG, "Unequip_char: already unequipped.", 0);
        return;
    }


    for (i = 0; i < 4; i++)
    	ch->armor[i] -= apply_ac( obj, obj->wear_loc,i );
    obj->wear_loc = -1;

    if (!obj->enchanted)
    {
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_SPELL_AFFECT)
            {
                for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next)
                {
                    lpaf_next = lpaf->next;
                    if ((lpaf->type == paf->type) &&
						(lpaf->skill_lvl == paf->skill_lvl) &&
                        (lpaf->location == APPLY_SPELL_AFFECT))
                    {
                        affect_remove (ch, lpaf);
                        lpaf_next = NULL;
                    }
                }
            }
            else
            {
                affect_modify (ch, paf, FALSE);
                affect_check (ch, paf->where, paf->bitvector);
            }
        }
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
        if (paf->location == APPLY_SPELL_AFFECT)
        {
            logstr (LOG_BUG, "Norm-Apply: %d", 0);
            for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next)
            {
                lpaf_next = lpaf->next;
                if ((lpaf->type == paf->type) &&
                    (lpaf->skill_lvl == paf->skill_lvl) &&
                    (lpaf->location == APPLY_SPELL_AFFECT))
                {
                    logstr (LOG_BUG, "location = %d", lpaf->location);
                    logstr (LOG_BUG, "type = %d", lpaf->type);
                    affect_remove (ch, lpaf);
                    lpaf_next = NULL;
                }
            }
        }
        else
        {
            affect_modify (ch, paf, FALSE);
            affect_check (ch, paf->where, paf->bitvector);
        }

    if (obj->item_type == ITEM_LIGHT
        && obj->value[2] != 0
        && ch->in_room != NULL
        && ch->in_room->light > 0) --ch->in_room->light;

    return;
}



/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list (OBJ_INDEX_DATA * pObjIndex, OBJ_DATA * list)
{
    OBJ_DATA *obj;
    int nMatch;

    nMatch = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData == pObjIndex)
            nMatch++;
    }

    return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room (OBJ_DATA * obj)
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;

    if ((in_room = obj->in_room) == NULL)
    {
        logstr (LOG_BUG, "obj_from_room: NULL.", 0);
        return;
    }

    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
        if (ch->on == obj)
            ch->on = NULL;

    if (obj == in_room->contents)
    {
        in_room->contents = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = in_room->contents; prev; prev = prev->next_content)
        {
            if (prev->next_content == obj)
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if (prev == NULL)
        {
            logstr (LOG_BUG, "Obj_from_room: obj not found.", 0);
            return;
        }
    }

    obj->in_room = NULL;
    obj->next_content = NULL;
    return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room (OBJ_DATA * obj, ROOM_INDEX_DATA * pRoomIndex)
{
    static int crash_protect = 0;

    obj->next_content = pRoomIndex->contents;
    pRoomIndex->contents = obj;
    obj->in_room = pRoomIndex;
    obj->carried_by = NULL;
    obj->in_obj = NULL;
    if ((pRoomIndex->sector_type == SECT_AIR) && (pRoomIndex->exit[DIR_DOWN] != NULL)) {
	    crash_protect++;
	    if (crash_protect == 10)
            return;
	    act( "$t falls from the sky above.\n\r", pRoomIndex->exit[DIR_DOWN]->u1.to_room->people, obj->short_descr, NULL, TO_ROOM);
	    act( "$t falls from the sky above.\n\r", pRoomIndex->exit[DIR_DOWN]->u1.to_room->people, obj->short_descr, NULL, TO_CHAR);
	    act( "$t falls through the air below you.\n\r", pRoomIndex->people, obj->short_descr, NULL, TO_ROOM);
	    act( "$t falls through the air below you.\n\r", pRoomIndex->people, obj->short_descr, NULL, TO_CHAR);
	    obj_from_room( obj );
	    obj_to_room( obj, pRoomIndex->exit[DIR_DOWN]->u1.to_room);
    }
    crash_protect = 0;

    return;
}



/*
 * Move an object into an object.
 */
void obj_to_obj (OBJ_DATA * obj, OBJ_DATA * obj_to)
{
    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
    obj->in_obj = obj_to;
    obj->in_room = NULL;
    obj->carried_by = NULL;
    if (obj_to->pIndexData->vnum == OBJ_VNUM_PIT)
        obj->cost = 0;

    for (; obj_to != NULL; obj_to = obj_to->in_obj)
    {
        if (obj_to->carried_by != NULL)
        {
            obj_to->carried_by->carry_number += get_obj_number (obj);
            obj_to->carried_by->carry_weight += get_obj_weight (obj)
                * WEIGHT_MULT (obj_to) / 100;
        }
    }

    return;
}



/*
 * Move an object out of an object.
 */
void obj_from_obj (OBJ_DATA * obj)
{
    OBJ_DATA *obj_from;

    if ((obj_from = obj->in_obj) == NULL)
    {
        logstr (LOG_BUG, "Obj_from_obj: null obj_from.", 0);
        return;
    }

    if (obj == obj_from->contains)
    {
        obj_from->contains = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = obj_from->contains; prev; prev = prev->next_content)
        {
            if (prev->next_content == obj)
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if (prev == NULL)
        {
            logstr (LOG_BUG, "Obj_from_obj: obj not found.", 0);
            return;
        }
    }

    obj->next_content = NULL;
    obj->in_obj = NULL;

    for (; obj_from != NULL; obj_from = obj_from->in_obj)
    {
        if (obj_from->carried_by != NULL)
        {
            obj_from->carried_by->carry_number -= get_obj_number (obj);
            obj_from->carried_by->carry_weight -= get_obj_weight (obj)
                * WEIGHT_MULT (obj_from) / 100;
        }
    }

    return;
}



/*
 * Extract an obj from the world.
 */
void extract_obj (OBJ_DATA * obj)
{
    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;

    if (obj->in_room != NULL)
        obj_from_room (obj);
    else if (obj->carried_by != NULL)
        obj_from_char (obj);
    else if (obj->in_obj != NULL)
        obj_from_obj (obj);

    for (obj_content = obj->contains; obj_content; obj_content = obj_next)
    {
        obj_next = obj_content->next_content;
        extract_obj (obj_content);
    }

    if (object_list == obj)
    {
        object_list = obj->next;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = object_list; prev != NULL; prev = prev->next)
        {
            if (prev->next == obj)
            {
                prev->next = obj->next;
                break;
            }
        }

        if (prev == NULL)
        {
            logstr (LOG_BUG, "Extract_obj: obj %d not found.", obj->pIndexData->vnum);
            return;
        }
    }

    --obj->pIndexData->count;
    free_obj (obj);
    return;
}



/*
 * Extract a char from the world.
 */
void extract_char (CHAR_DATA * ch, bool fPull)
{
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    /* doesn't seem to be necessary
       if ( ch->in_room == NULL )
       {
       logstr (LOG_BUG,  "Extract_char: NULL.", 0 );
       return;
       }
     */

    nuke_pets (ch);
    ch->pet = NULL;                /* just in case */

    if (fPull)
        die_follower (ch);

    stop_fighting (ch, TRUE);

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        extract_obj (obj);
    }

    if (ch->in_room != NULL)
        char_from_room (ch);

    /* Death room is set in the clan table now */
    if (!fPull) {
        char_to_room (ch, get_room_index (clan_table[ch->clan].hall));
        return;
    }

    if (IS_NPC (ch)) {
        --ch->pIndexData->count;
		if (ch->pIndexData->pCreated == ch) {
            if (ch->pNextSame)
				ch->pIndexData->pCreated = ch->pNextSame;
			else
				ch->pIndexData->pCreated = ch->pPrevSame;
		}
		if (ch->pNextSame)
		    ch->pNextSame->pPrevSame = ch->pPrevSame;
		if (ch->pPrevSame)
			ch->pPrevSame->pNextSame = ch->pNextSame;
		ch->pNextSame = NULL;
		ch->pPrevSame = NULL;
	}

    if (ch->desc != NULL && ch->desc->original != NULL)
    {
        do_function (ch, &do_return, "");
        ch->desc = NULL;
    }

    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (wch->reply == ch)
            wch->reply = NULL;
        if (ch->mprog_target == wch)
            wch->mprog_target = NULL;
		if (!IS_NPC(wch) && wch->pcdata->pTeacher == ch)
			wch->pcdata->pTeacher = NULL;
    }

    if (ch == char_list)
    {
        char_list = ch->next;
    }
    else
    {
        CHAR_DATA *prev;

        for (prev = char_list; prev != NULL; prev = prev->next)
        {
            if (prev->next == ch)
            {
                prev->next = ch->next;
                break;
            }
        }

        if (prev == NULL)
        {
            logstr (LOG_BUG, "Extract_char: char not found.", 0);
            return;
        }
    }

    if (ch->desc != NULL)
        ch->desc->character = NULL;
    free_char (ch);
    return;
}



/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room( CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *rch;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    if ( !str_cmp( arg, "self" ) )
        return ch;

    if ( ch && room )
    {
	    logstr (LOG_BUG,  "get_char_room received multiple types (ch/room)", 0 );
	    return NULL;
    }

    if ( ch )
	    rch = ch->in_room->people;
    else
	    rch = room->people;

    for ( ; rch != NULL; rch = rch->next_in_room )
    {
	if ( (ch && !can_see( ch, rch )) || !is_name( arg, rch->name ) )
	    continue;
	if ( ++count == number )
	    return rch;
    }

    return NULL;
}




/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;

    if ( ch && ( wch = get_char_room( ch, NULL, argument ) ) != NULL )
	return wch;

    number = number_argument( argument, arg );
    count  = 0;
    for ( wch = char_list; wch != NULL ; wch = wch->next )
    {
	if ( wch->in_room == NULL || ( ch && !can_see( ch, wch ) )
	||   !is_name( arg, wch->name ) )
	    continue;
	if ( ++count == number )
	    return wch;
    }

    return NULL;
}



/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type (OBJ_INDEX_DATA * pObjIndex)
{
    OBJ_DATA *obj;

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (obj->pIndexData == pObjIndex)
            return obj;
    }

    return NULL;
}


/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list (CHAR_DATA * ch, char *argument, OBJ_DATA * list)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument (argument, arg);
    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
        if (can_see_obj (ch, obj) && is_name (arg, obj->name))
        {
            if (++count == number)
                return obj;
        }
    }

    return NULL;
}



/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, char *argument, CHAR_DATA *viewer )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == WEAR_NONE
        &&   ( viewer ? can_see_obj( viewer, obj ) : TRUE )
        &&   is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                            return obj;
        }
    }

    return NULL;
}



/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, char *argument, bool character )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc != WEAR_NONE
                &&  ( character ? can_see_obj( ch, obj ) : TRUE)
        &&   is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
        }
    }

    return NULL;
}



/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here( CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument )
{
    OBJ_DATA *obj;
    int number, count;
    char arg[MAX_INPUT_LENGTH];

    if ( ch && room )
    {
	logstr (LOG_BUG,  "get_obj_here received a ch and a room",0);
	return NULL;
    }

    number = number_argument( argument, arg );
    count = 0;

    if ( ch )
    {
	obj = get_obj_list( ch, argument, ch->in_room->contents );
	if ( obj != NULL )
	    return obj;

	if ( ( obj = get_obj_carry( ch, argument, ch ) ) != NULL )
	    return obj;

	if ( ( obj = get_obj_wear( ch, argument, TRUE ) ) != NULL )
	    return obj;
    }
    else
    {
	for ( obj = room->contents; obj; obj = obj->next_content )
	{
	    if ( !is_name( arg, obj->name ) )
		continue;
	    if ( ++count == number )
		return obj;
	}
    }

    return NULL;
}



/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    if ( ch && ( obj = get_obj_here( ch, NULL, argument ) ) != NULL )
	return obj;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
	if ( obj->in_room == NULL || ( ch && !can_see_obj( ch, obj ) )
	|| !is_name( arg, obj->name ) )
	    continue;
	if ( ++count == number )
	    return obj;
    }

    return NULL;
}

/* deduct cost from a character */

void deduct_cost (CHAR_DATA * ch, int cost)
{
    int zenni = 0;

    zenni = UMIN (ch->zenni, cost);
    ch->zenni -= zenni;

    if (ch->zenni < 0)
    {
        logstr (LOG_BUG, "deduct costs: ch zenni %d < 0", ch->zenni);
        ch->zenni = 0;
    }
}

/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money (int zenni)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if (zenni <= 0)
    {
        logstr (LOG_BUG, "Create_money: zero or negative money.", zenni);
        zenni = UMAX (1, zenni);
    }

    if (zenni == 1)
    {
        obj = create_object (get_obj_index (OBJ_VNUM_ZENNI_ONE), 0);
    }
    else
    {
        obj = create_object (get_obj_index (OBJ_VNUM_ZENNI_SOME), 0);
        sprintf (buf, obj->short_descr, zenni);
        free_string (obj->short_descr);
        obj->short_descr = str_dup (buf);
        obj->value[0] = zenni;
        obj->cost = zenni;
        obj->weight = zenni / 5;
    }

    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number (OBJ_DATA * obj)
{
    int number;

    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
        || obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
        number = 0;
    else
        number = 1;

    for (obj = obj->contains; obj != NULL; obj = obj->next_content)
        number += get_obj_number (obj);

    return number;
}


/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight (OBJ_DATA * obj)
{
    int weight;
    OBJ_DATA *tobj;

    weight = obj->weight;
    for (tobj = obj->contains; tobj != NULL; tobj = tobj->next_content)
        weight += get_obj_weight (tobj) * WEIGHT_MULT (obj) / 100;

    return weight;
}

int get_true_weight (OBJ_DATA * obj)
{
    int weight;

    weight = obj->weight;
    for (obj = obj->contains; obj != NULL; obj = obj->next_content)
        weight += get_obj_weight (obj);

    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark (ROOM_INDEX_DATA * pRoomIndex)
{
    if (pRoomIndex->light > 0)
        return FALSE;

    if (IS_SET (pRoomIndex->room_flags, ROOM_DARK))
        return TRUE;

    if (pRoomIndex->sector_type == SECT_INSIDE
        || pRoomIndex->sector_type == SECT_CITY) return FALSE;

    if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
        return TRUE;

    return FALSE;
}


bool is_room_owner (CHAR_DATA * ch, ROOM_INDEX_DATA * room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
        return FALSE;

    return is_name (ch->name, room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private (ROOM_INDEX_DATA * pRoomIndex)
{
    CHAR_DATA *rch;
    int count;


    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
        return TRUE;

    count = 0;
    for (rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room)
        count++;

    if (IS_SET (pRoomIndex->room_flags, ROOM_PRIVATE) && count >= 2)
        return TRUE;

    if (IS_SET (pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1)
        return TRUE;

    if (IS_SET (pRoomIndex->room_flags, ROOM_IMP_ONLY))
        return TRUE;

    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room (CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex)
{
    if (IS_SET (pRoomIndex->room_flags, ROOM_IMP_ONLY)
        && ch->level < MAX_LEVEL)
        return FALSE;

    if (IS_SET (pRoomIndex->room_flags, ROOM_GODS_ONLY) && !IS_IMMORTAL (ch))
        return FALSE;

    if (IS_SET (pRoomIndex->room_flags, ROOM_HEROES_ONLY)
        && !IS_IMMORTAL (ch))
        return FALSE;

    /*
	if (IS_SET (pRoomIndex->room_flags, ROOM_NEWBIES_ONLY)
        && ch->level > 5 && !IS_IMMORTAL (ch))
        return FALSE;
	*/

    if (!IS_IMMORTAL (ch) && pRoomIndex->clan && ch->clan != pRoomIndex->clan)
        return FALSE;

    return TRUE;
}



/*
 * True if char can see victim.
 */
bool can_see (CHAR_DATA * ch, CHAR_DATA * victim)
{
/* RT changed so that WIZ_INVIS has levels */
    if (ch == victim)
        return TRUE;

    if (ch->level < victim->invis_level)
        return FALSE;

    if (ch->level < victim->incog_level
        && ch->in_room != victim->in_room)
		return FALSE;

    if ((!IS_NPC (ch) && IS_SET (ch->act, PLR_HOLYLIGHT))
        || (IS_NPC (ch) && IS_IMMORTAL (ch)))
        return TRUE;

    if (IS_AFFECTED (ch, AFF_BLIND)) {
        /*
		AFFECT_DATA *paf;
        // Look for a specific affect
        if ((paf = affect_find_bit(ch->affected, AFF_BLIND))) {
            if (number_range(1, 20) > 5 + paf->skill_lvl)
                return TRUE;
            else
                return FALSE;
        }
		*/
        // Otherwise, 50/50 chance
        if (number_range(1,2) == 1)
            return FALSE;
    }

    if (room_is_dark (ch->in_room) && !IS_AFFECTED (ch, AFF_INFRARED))
        return FALSE;

    if (IS_AFFECTED (victim, AFF_INVISIBLE)
        && !IS_AFFECTED (ch, AFF_DETECT_INVIS))
        return FALSE;

    /* sneaking */
    if (IS_AFFECTED (victim, AFF_SNEAK)
        && !IS_AFFECTED (ch, AFF_DETECT_HIDDEN) && victim->fighting == NULL)
    {
        int chance;
        chance = get_skill (victim, gsn_sneak);
        chance += get_curr_stat (victim, STAT_DEX) * 3 / 2;
        chance -= get_curr_stat (ch, STAT_INT) * 2;
        chance -= ch->level - victim->level * 3 / 2;

        if (number_percent () < chance)
            return FALSE;
    }

    if (IS_AFFECTED (victim, AFF_HIDE)
        && !IS_AFFECTED (ch, AFF_DETECT_HIDDEN) && victim->fighting == NULL)
        return FALSE;

    return TRUE;
}



/*
 * True if char can see obj.
 */
bool can_see_obj (CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (!IS_NPC (ch) && IS_SET (ch->act, PLR_HOLYLIGHT))
        return TRUE;

    if (IS_SET (obj->extra_flags, ITEM_VIS_DEATH))
        return FALSE;

    if (IS_AFFECTED (ch, AFF_BLIND) && obj->item_type != ITEM_POTION)
        return FALSE;

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
        return TRUE;

    if (IS_SET (obj->extra_flags, ITEM_INVIS)
        && !IS_AFFECTED (ch, AFF_DETECT_INVIS))
        return FALSE;

    if (IS_OBJ_STAT (obj, ITEM_GLOW))
        return TRUE;

    if (room_is_dark (ch->in_room) && !IS_AFFECTED (ch, AFF_DARK_VISION))
        return FALSE;

    return TRUE;
}



/*
 * True if char can drop obj.
 */
bool can_drop_obj (CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (!IS_SET (obj->extra_flags, ITEM_NODROP))
        return TRUE;

    if (!IS_NPC (ch) && ch->level >= LEVEL_IMMORTAL)
        return TRUE;

    return FALSE;
}


/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name (int location)
{
    switch (location)
    {
        case APPLY_NONE:
            return "none";
        case APPLY_STR:
            return "strength";
        case APPLY_DEX:
            return "dexterity";
        case APPLY_INT:
            return "intelligence";
        case APPLY_WIL:
            return "will power";
        case APPLY_CHA:
            return "charisma";
        case APPLY_SEX:
            return "sex";
        case APPLY_LEVEL:
            return "level";
        case APPLY_AGE:
            return "age";
        case APPLY_KI:
            return "ki";
        case APPLY_HIT:
            return "hp";
        case APPLY_ZENNI:
            return "zenni";
        case APPLY_EXP:
            return "experience";
        case APPLY_AC:
            return "armor class";
        case APPLY_HITROLL:
            return "hit roll";
        case APPLY_DAMROLL:
            return "damage roll";
        case APPLY_SAVES:
            return "saves";
        case APPLY_SAVING_ROD:
            return "save vs rod";
        case APPLY_SAVING_PETRI:
            return "save vs petrification";
        case APPLY_SAVING_BREATH:
            return "save vs breath";
        case APPLY_SAVING_SPELL:
            return "save vs spell";
        case APPLY_SPELL_AFFECT:
            return "none";
    }

    logstr (LOG_BUG, "Affect_location_name: unknown location %d.", location);
    return "(unknown)";
}



/*
 * Return ascii name of an affect bit vector.
 */
char *affect_bit_name (int vector)
{
    static char buf[512];

    buf[0] = '\0';
    if (vector & AFF_BLIND)
        strcat (buf, " blind");
    if (vector & AFF_INVISIBLE)
        strcat (buf, " invisible");
    if (vector & AFF_DETECT_EVIL)
        strcat (buf, " detect_evil");
    if (vector & AFF_DETECT_GOOD)
        strcat (buf, " detect_good");
    if (vector & AFF_DETECT_INVIS)
        strcat (buf, " detect_invis");
    if (vector & AFF_DETECT_MAGIC)
        strcat (buf, " detect_magic");
    if (vector & AFF_DETECT_HIDDEN)
        strcat (buf, " detect_hidden");
    if (vector & AFF_SANCTUARY)
        strcat (buf, " sanctuary");
    if (vector & AFF_FAERIE_FIRE)
        strcat (buf, " faerie_fire");
    if (vector & AFF_INFRARED)
        strcat (buf, " infrared");
    if (vector & AFF_CURSE)
        strcat (buf, " curse");
    if (vector & AFF_POISON)
        strcat (buf, " poison");
    if (vector & AFF_PROTECT_EVIL)
        strcat (buf, " prot_evil");
    if (vector & AFF_PROTECT_GOOD)
        strcat (buf, " prot_good");
    if (vector & AFF_SLEEP)
        strcat (buf, " sleep");
    if (vector & AFF_SNEAK)
        strcat (buf, " sneak");
    if (vector & AFF_HIDE)
        strcat (buf, " hide");
    if (vector & AFF_CHARM)
        strcat (buf, " charm");
    if (vector & AFF_FLYING)

        strcat (buf, " flying");
    if (vector & AFF_PASS_DOOR)
        strcat (buf, " pass_door");
    if (vector & AFF_BERSERK)
        strcat (buf, " berserk");
    if (vector & AFF_CALM)
        strcat (buf, " calm");
    if (vector & AFF_HASTE)
        strcat (buf, " haste");
    if (vector & AFF_SLOW)
        strcat (buf, " slow");
    if (vector & AFF_PLAGUE)
        strcat (buf, " plague");
    if (vector & AFF_DARK_VISION)
        strcat (buf, " dark_vision");
    if (vector & AFF_KAIOKEN)
        strcat (buf, " kaioken");
    return (buf[0] != '\0') ? buf + 1 : "none";
}



/*
 * Return ascii name of extra flags vector.
 */
char *extra_bit_name (int extra_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (extra_flags & ITEM_GLOW)
        strcat (buf, " glow");
    if (extra_flags & ITEM_HUM)
        strcat (buf, " hum");
    if (extra_flags & ITEM_DARK)
        strcat (buf, " dark");
    if (extra_flags & ITEM_LOCK)
        strcat (buf, " lock");
    if (extra_flags & ITEM_EVIL)
        strcat (buf, " evil");
    if (extra_flags & ITEM_INVIS)
        strcat (buf, " invis");
    if (extra_flags & ITEM_MAGIC)
        strcat (buf, " magic");
    if (extra_flags & ITEM_NODROP)
        strcat (buf, " nodrop");
    if (extra_flags & ITEM_BLESS)
        strcat (buf, " bless");
    if (extra_flags & ITEM_ANTI_GOOD)
        strcat (buf, " anti-good");
    if (extra_flags & ITEM_ANTI_EVIL)
        strcat (buf, " anti-evil");
    if (extra_flags & ITEM_ANTI_NEUTRAL)
        strcat (buf, " anti-neutral");
    if (extra_flags & ITEM_NOREMOVE)
        strcat (buf, " noremove");
    if (extra_flags & ITEM_INVENTORY)
        strcat (buf, " inventory");
    if (extra_flags & ITEM_NOPURGE)
        strcat (buf, " nopurge");
    if (extra_flags & ITEM_VIS_DEATH)
        strcat (buf, " vis_death");
	if (extra_flags & ITEM_NEED_PL)
		strcat (buf, " need_pl");
    if (extra_flags & ITEM_ROT_DEATH)
        strcat (buf, " rot_death");
    if (extra_flags & ITEM_NOLOCATE)
        strcat (buf, " no_locate");
    if (extra_flags & ITEM_SELL_EXTRACT)
        strcat (buf, " sell_extract");
	if (extra_flags & ITEM_INDESTRUCTIBLE)
		strcat (buf, " indestructible");
    if (extra_flags & ITEM_BURN_PROOF)
        strcat (buf, " burn_proof");
    if (extra_flags & ITEM_NOUNCURSE)
        strcat (buf, " no_uncurse");
	if (extra_flags & ITEM_SCOUTER)
		strcat (buf, " scouter");
	if (extra_flags & ITEM_RANDOM)
		strcat (buf, " random");
    return (buf[0] != '\0') ? buf + 1 : "none";
}

/* return ascii name of an act vector */
char *act_bit_name (int act_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET (act_flags, ACT_IS_NPC))
    {
        strcat (buf, " npc");
        if (act_flags & ACT_SENTINEL)
            strcat (buf, " sentinel");
        if (act_flags & ACT_SCAVENGER)
            strcat (buf, " scavenger");
        if (act_flags & ACT_AGGRESSIVE)
            strcat (buf, " aggressive");
        if (act_flags & ACT_MISSION)
			strcat (buf, " mission");
        if (act_flags & ACT_STAY_AREA)
            strcat (buf, " stay_area");
        if (act_flags & ACT_WIMPY)
            strcat (buf, " wimpy");
        if (act_flags & ACT_PET)
            strcat (buf, " pet");
        if (act_flags & ACT_UPGRADE)
            strcat (buf, " upgrade");
        if (act_flags & ACT_UNDEAD)
            strcat (buf, " undead");
        if (act_flags & ACT_NOALIGN)
            strcat (buf, " no_align");
        if (act_flags & ACT_NOPURGE)
            strcat (buf, " no_purge");
        if (act_flags & ACT_IS_HEALER)
            strcat (buf, " healer");
        if (act_flags & ACT_IS_CHANGER)
            strcat (buf, " changer");
        if (act_flags & ACT_UPDATE_ALWAYS)
            strcat (buf, " update_always");
        if (act_flags & ACT_ANIMAL)
            strcat (buf, " animal");
        if (act_flags & ACT_CIVILIAN)
            strcat (buf, " civilian");
        if (act_flags & ACT_WARRIOR)
            strcat (buf, " warrior");
        if (act_flags & ACT_KIWARRIOR)
            strcat (buf, " ki_warrior");
    }
    else
    {
        strcat (buf, " player");
        if (act_flags & PLR_AUTOASSIST)
            strcat (buf, " autoassist");
        if (act_flags & PLR_AUTOEXIT)
            strcat (buf, " autoexit");
        if (act_flags & PLR_AUTOLOOT)
            strcat (buf, " autoloot");
        if (act_flags & PLR_AUTOSAC)
            strcat (buf, " autosac");
        if (act_flags & PLR_AUTOZENNI)
            strcat (buf, " autozenni");
        if (act_flags & PLR_AUTOSPLIT)
            strcat (buf, " autosplit");
        if (act_flags & PLR_COMBATINFO)
            strcat (buf, " combat_info");
        if (act_flags & PLR_HOLYLIGHT)
            strcat (buf, " holy_light");
 	    if (act_flags & PLR_QUEST)
			strcat (buf, " quest");
		if (act_flags & PLR_AUTOATTACK);
	        strcat (buf, " auto_attack");
	    if (act_flags & PLR_CANLOOT)
            strcat (buf, " loot_corpse");
        if (act_flags & PLR_NOSUMMON)
            strcat (buf, " no_summon");
        if (act_flags & PLR_NOFOLLOW)
            strcat (buf, " no_follow");
        if (act_flags & PLR_FREEZE)
            strcat (buf, " frozen");
        if (act_flags & PLR_THIEF)
            strcat (buf, " thief");
        if (act_flags & PLR_HOSTILE)
            strcat (buf, " hostile");
    }
    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *comm_bit_name (int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET)
        strcat (buf, " quiet");
    if (comm_flags & COMM_DEAF)
        strcat (buf, " deaf");
    if (comm_flags & COMM_NOWIZ)
        strcat (buf, " no_wiz");
    if (comm_flags & COMM_NOGOD)
        strcat (buf, " no_god");
    if (comm_flags & COMM_NOAUCTION)
        strcat (buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP)
        strcat (buf, " no_gossip");
    if (comm_flags & COMM_NOQUESTION)
        strcat (buf, " no_question");
    if (comm_flags & COMM_NOMUSIC)
        strcat (buf, " no_music");
    if (comm_flags & COMM_NOQUOTE)
        strcat (buf, " no_quote");
    if (comm_flags & COMM_COMPACT)
        strcat (buf, " compact");
    if (comm_flags & COMM_BRIEF)
        strcat (buf, " brief");
    if (comm_flags & COMM_PROMPT)
        strcat (buf, " prompt");
    if (comm_flags & COMM_COMBINE)
        strcat (buf, " combine");
    if (comm_flags & COMM_NOEMOTE)
        strcat (buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT)
        strcat (buf, " no_shout");
    if (comm_flags & COMM_NOTELL)
        strcat (buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS)
        strcat (buf, " no_channels");


    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *imm_bit_name (int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON)
        strcat (buf, " summon");
    if (imm_flags & IMM_CHARM)
        strcat (buf, " charm");
    if (imm_flags & IMM_MAGIC)
        strcat (buf, " magic");
    if (imm_flags & IMM_WEAPON)
        strcat (buf, " weapon");
    if (imm_flags & IMM_BASH)
        strcat (buf, " blunt");
    if (imm_flags & IMM_PIERCE)
        strcat (buf, " piercing");
    if (imm_flags & IMM_SLASH)
        strcat (buf, " slashing");
    if (imm_flags & IMM_FIRE)
        strcat (buf, " fire");
    if (imm_flags & IMM_COLD)
        strcat (buf, " cold");
    if (imm_flags & IMM_LIGHTNING)
        strcat (buf, " lightning");
    if (imm_flags & IMM_ACID)
        strcat (buf, " acid");
    if (imm_flags & IMM_POISON)
        strcat (buf, " poison");
    if (imm_flags & IMM_NEGATIVE)
        strcat (buf, " negative");
    if (imm_flags & IMM_HOLY)
        strcat (buf, " holy");
    if (imm_flags & IMM_ENERGY)
        strcat (buf, " energy");
    if (imm_flags & IMM_MENTAL)
        strcat (buf, " mental");
    if (imm_flags & IMM_DISEASE)
        strcat (buf, " disease");
    if (imm_flags & IMM_DROWNING)
        strcat (buf, " drowning");
    if (imm_flags & IMM_LIGHT)
        strcat (buf, " light");
    if (imm_flags & VULN_IRON)
        strcat (buf, " iron");
    if (imm_flags & VULN_WOOD)
        strcat (buf, " wood");
    if (imm_flags & VULN_SILVER)
        strcat (buf, " silver");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *wear_bit_name (int wear_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (wear_flags & ITEM_TAKE)
        strcat (buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER)
        strcat (buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK)
        strcat (buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY)
        strcat (buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD)
        strcat (buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS)
        strcat (buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET)
        strcat (buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS)
        strcat (buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS)
        strcat (buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD)
        strcat (buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT)
        strcat (buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST)
        strcat (buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST)
        strcat (buf, " wrist");
    if (wear_flags & ITEM_WIELD)
        strcat (buf, " wield");
    if (wear_flags & ITEM_HOLD)
        strcat (buf, " hold");
    if (wear_flags & ITEM_NO_SAC)
        strcat (buf, " nosac");
    if (wear_flags & ITEM_WEAR_FLOAT)
        strcat (buf, " float");
    if (wear_flags & ITEM_WEAR_TAIL)
        strcat (buf, " tail");
    if (wear_flags & ITEM_WEAR_EYE)
        strcat (buf, " eye");
    if (wear_flags & ITEM_WEAR_EAR)
        strcat (buf, " ear");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *form_bit_name (int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON)
        strcat (buf, " poison");
    else if (form_flags & FORM_EDIBLE)
        strcat (buf, " edible");
    if (form_flags & FORM_MAGICAL)
        strcat (buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY)
        strcat (buf, " instant_rot");
    if (form_flags & FORM_OTHER)
        strcat (buf, " other");
    if (form_flags & FORM_ANIMAL)
        strcat (buf, " animal");
    if (form_flags & FORM_SENTIENT)
        strcat (buf, " sentient");
    if (form_flags & FORM_UNDEAD)
        strcat (buf, " undead");
    if (form_flags & FORM_CONSTRUCT)
        strcat (buf, " construct");
    if (form_flags & FORM_MIST)
        strcat (buf, " mist");
    if (form_flags & FORM_INTANGIBLE)
        strcat (buf, " intangible");
    if (form_flags & FORM_BIPED)
        strcat (buf, " biped");
    if (form_flags & FORM_CENTAUR)
        strcat (buf, " centaur");
    if (form_flags & FORM_INSECT)
        strcat (buf, " insect");
    if (form_flags & FORM_SPIDER)
        strcat (buf, " spider");
    if (form_flags & FORM_CRUSTACEAN)
        strcat (buf, " crustacean");
    if (form_flags & FORM_WORM)
        strcat (buf, " worm");
    if (form_flags & FORM_BLOB)
        strcat (buf, " blob");
    if (form_flags & FORM_MAMMAL)
        strcat (buf, " mammal");
    if (form_flags & FORM_BIRD)
        strcat (buf, " bird");
    if (form_flags & FORM_REPTILE)
        strcat (buf, " reptile");
    if (form_flags & FORM_SNAKE)
        strcat (buf, " snake");
    if (form_flags & FORM_DRAGON)
        strcat (buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN)
        strcat (buf, " amphibian");
    if (form_flags & FORM_FISH)
        strcat (buf, " fish");
    if (form_flags & FORM_COLD_BLOOD)
        strcat (buf, " cold_blooded");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *part_bit_name (int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD)
        strcat (buf, " head");
    if (part_flags & PART_ARMS)
        strcat (buf, " arms");
    if (part_flags & PART_LEGS)
        strcat (buf, " legs");
    if (part_flags & PART_HEART)
        strcat (buf, " heart");
    if (part_flags & PART_BRAINS)
        strcat (buf, " brains");
    if (part_flags & PART_GUTS)
        strcat (buf, " guts");
    if (part_flags & PART_HANDS)
        strcat (buf, " hands");
    if (part_flags & PART_FEET)
        strcat (buf, " feet");
    if (part_flags & PART_FINGERS)
        strcat (buf, " fingers");
    if (part_flags & PART_EAR)
        strcat (buf, " ears");
    if (part_flags & PART_EYE)
        strcat (buf, " eyes");
    if (part_flags & PART_LONG_TONGUE)
        strcat (buf, " long_tongue");
    if (part_flags & PART_EYESTALKS)
        strcat (buf, " eyestalks");
    if (part_flags & PART_TENTACLES)
        strcat (buf, " tentacles");
    if (part_flags & PART_FINS)
        strcat (buf, " fins");
    if (part_flags & PART_WINGS)
        strcat (buf, " wings");
    if (part_flags & PART_TAIL)
        strcat (buf, " tail");
    if (part_flags & PART_CLAWS)
        strcat (buf, " claws");
    if (part_flags & PART_FANGS)
        strcat (buf, " fangs");
    if (part_flags & PART_HORNS)
        strcat (buf, " horns");
    if (part_flags & PART_SCALES)
        strcat (buf, " scales");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *weapon_bit_name (int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING)
        strcat (buf, " flaming");
    if (weapon_flags & WEAPON_FROST)
        strcat (buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC)
        strcat (buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP)
        strcat (buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL)
        strcat (buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS)
        strcat (buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING)
        strcat (buf, " shocking");
    if (weapon_flags & WEAPON_POISON)
        strcat (buf, " poison");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char *cont_bit_name (int cont_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (cont_flags & CONT_CLOSEABLE)
        strcat (buf, " closable");
    if (cont_flags & CONT_PICKPROOF)
        strcat (buf, " pickproof");
    if (cont_flags & CONT_CLOSED)
        strcat (buf, " closed");
    if (cont_flags & CONT_LOCKED)
        strcat (buf, " locked");

    return (buf[0] != '\0') ? buf + 1 : "none";
}


char *off_bit_name (int off_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (off_flags & OFF_AREA_ATTACK)
        strcat (buf, " area attack");
    if (off_flags & OFF_BASH)
        strcat (buf, " bash");
    if (off_flags & OFF_BERSERK)
        strcat (buf, " berserk");
    if (off_flags & OFF_DISARM)
        strcat (buf, " disarm");
    if (off_flags & OFF_DODGE)
        strcat (buf, " dodge");
    if (off_flags & OFF_FADE)
        strcat (buf, " fade");
    if (off_flags & OFF_FAST)
        strcat (buf, " fast");
    if (off_flags & OFF_KICK)
        strcat (buf, " kick");
    if (off_flags & OFF_PARRY)
        strcat (buf, " parry");
    if (off_flags & OFF_RESCUE)
        strcat (buf, " rescue");
    if (off_flags & OFF_TAIL)
        strcat (buf, " tail");
    if (off_flags & OFF_CRUSH)
        strcat (buf, " crush");
    if (off_flags & ASSIST_ALL)
        strcat (buf, " assist_all");
    if (off_flags & ASSIST_ALIGN)
        strcat (buf, " assist_align");
    if (off_flags & ASSIST_RACE)
        strcat (buf, " assist_race");
    if (off_flags & ASSIST_PLAYERS)
        strcat (buf, " assist_players");
    if (off_flags & ASSIST_GUARD)
        strcat (buf, " assist_guard");
    if (off_flags & ASSIST_VNUM)
        strcat (buf, " assist_vnum");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * See if a string is one of the names of an object.
 */

bool is_full_name( const char *str, char *namelist )
{
	char name[MIL];

	for ( ; ; )
	{
		namelist = one_argument( namelist, name );
		if ( name[0] == '\0' )
			return FALSE;
		if ( !str_cmp( str, name ) )
			return TRUE;
	}
}

// Returns a pointer to a scouter, if the character has one
OBJ_DATA *get_scouter (CHAR_DATA *ch) {
    OBJ_DATA *obj;
	int iWear;
	for (iWear = 0; iWear < MAX_WEAR; iWear++) {
        if ((obj = get_eq_char (ch, iWear)) != NULL)
			if (IS_OBJ_STAT (obj, ITEM_SCOUTER))
				return obj;
	}
	return NULL;
}

// Returns a formatted string of what the character sees for someones pl
char *get_pl_from_scouter (CHAR_DATA *ch, CHAR_DATA* victim) {
	OBJ_DATA *obj;
	static char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char rand_let[10] = {'6', '1', 'F', '&', '/', '4', '8', '0', 'L', '0'};
	int n = number_range (3,10), i;
    long long int llPl;

    if (victim->llSuppressPl > -1 && get_skill(victim, gsn_suppress) > get_skill(ch, gsn_sense))
        llPl = victim->llSuppressPl;
    else
        llPl = victim->nCurPl * victim->llPl / 100;

	buf[0] = '\0';
	if ((obj = get_scouter(ch)) != NULL || get_skill(ch, gsn_sense) > 0) {
		if ((obj != NULL && 2*obj->llPl > llPl)
            || get_skill(ch, gsn_sense) * 25 > sqrt(llPl)) // Since pl = difficulty^2
            sprintf (buf, "{R%s{x", format_pl(llPl));
		else {
        	for (i = 0; i < n; ++i)
				buf2[i] = rand_let[number_range(0,9)];
			buf2[n] = '\0';
			sprintf (buf, "{R%s{x", buf2);
		}
	}
	else
		strcpy (buf, "{Rnone{x");
	return buf;
}


void wait (CHAR_DATA *ch, int pulse) {
    if (IS_AFFECTED (ch, AFF_KAIOKEN))
        pulse = URANGE(1, pulse * (800 - get_skill(ch, gsn_kaioken)) / 900, pulse - 1);
	if (IS_AFFECTED (ch, AFF_HASTE))
		pulse = URANGE(1, pulse / 2, pulse - 1);
    //ch->wait += pulse;
	ch->wait = UMAX(ch->wait, pulse);
    if (ch->wait < PULSE_PER_SECOND)
	    ch->short_wait = TRUE;
    else
	    ch->short_wait = FALSE;
	return;
}


//#define WAIT_STATE(ch, npulse)    ((ch)->wait = UMAX((ch)->wait, (npulse)))
/*#define WAIT_STATE(ch, npulse)    ((ch)->wait += (npulse); \
                                   if ((ch)->wait < PULSE_PER_SECOND) \
                                       (ch)->short_wait = TRUE; \
                                   else \
                                       (ch)->short_wait = FALSE)
*/


// Sets the characters rage.  Only works for saiya-jins or half-breeds. Takes
// care of any transformations that may occur.
void rage (CHAR_DATA *ch, int rage) {
    int r;

    if (IS_NPC(ch))
        return;

    if (race_lookup("saiya-jin") != ch->race &&
        race_lookup("half-breed") != ch->race)
        return;

    rage = URANGE(0, rage, 1000);
    if (rage == ch->trans_count)
        return;

    // Rage is increased?
	if (rage > ch->trans_count) {
        // Check if rage is high enough
        if (get_skill(ch, gsn_ssj3) > 0) {
            if (ch->nDifficulty < 25000 || race_lookup("half-breed") == ch->race)
                return;
            r = 100;
        }
        else if (get_skill(ch, gsn_ssj2) > 0) {
            if (ch->nDifficulty < 7500 || race_lookup("half-breed") == ch->race)
                return;
            r =  75;
        }
        else if (get_skill(ch, gsn_ssj1) > 0) {
            if (ch->nDifficulty < 2500)
                return;
            r =  50;
        }
        else {
            if (ch->nDifficulty < 1000)
                return;
            r =  25;
        }

        // See what happens
        if (rage >= r) {
            // Transform!
            ch->trans_count = 0;
            if (get_skill(ch, gsn_ssj3) > 0) {
                ch->pcdata->learned[gsn_ssj4] = 1;
                do_function (ch, &do_ssj4, "");
            }
            else if (get_skill(ch, gsn_ssj2) > 0) {
                ch->pcdata->learned[gsn_ssj3] = 1;
                do_function (ch, &do_ssj3, "");
            }
            else if (get_skill(ch, gsn_ssj1) > 0) {
                ch->pcdata->learned[gsn_ssj2] = 1;
                do_function (ch, &do_ssj2, "");
            }
            else {
                ch->pcdata->learned[gsn_ssj1] = 1;
                do_function (ch, &do_ssj1, "");
            }
            return;
        }
        else if (rage == r - 1) {
            if (get_skill(ch, gsn_ssj3) > 0) {
                sendch ("{RA thick growth of red hair sprouts on you, but then recedes!{x\n\r", ch);
                act ("{RA thick growth of red hair sprouts on $n, but then recedes!{x", ch, NULL, NULL, TO_ROOM);
            }
            else if (get_skill(ch, gsn_ssj2) > 0) {
                sendch ("{YYour eyebrows disappear, but quickly grow back...{x\n\r", ch);
                act ("{Y$n's eyebrows disappear, but quickly grow back...{x", ch, NULL, NULL, TO_ROOM);
            }
            else if (get_skill(ch, gsn_ssj1) > 0) {
                sendch ("{YFor an instant, your hair grows and stiffens up!{x\n\r", ch);
                act ("{YFor an instant, $n's hair grows and stiffens up!{x", ch, NULL, NULL, TO_ROOM);
            }
            else {
                sendch ("{YYour hair briefly flashes yellow!{x\n\r", ch);
                act ("{Y$n's hair briefly flashes yellow!{x", ch, NULL, NULL, TO_ROOM);
            }
        }
        else if (rage % 2 == 0)
            sendch ("{rYou feel more angry!{x\n\r", ch);
    }
    else
        if (rage % 2 == 0)
            sendch ("{yYou feel more mellow.{x\n\r", ch);
    ch->trans_count = rage;
    return;
}

// Increments the trans_count counter for bio-androids.
// Checks if the ch is a bio-android, and if it hits 100,
// he/she/it evolves
void inc_bio_evolve (CHAR_DATA *ch) {
    int i, bonus;

    if (IS_NPC(ch))
        return;
    if (race_lookup("bio-android") != ch->race || GetTrans(ch) == TRANS_BIO5)
        return;

    ++ch->trans_count;
    if (ch->trans_count < 100)
        return;
    ch->trans_count = 0;

    if (GetTrans(ch) == TRANS_NONE) {
        bonus = 4;
        act ("{GYou have evolved into an imperfect bio-android!{x", ch, NULL, NULL, TO_CHAR);
        act ("{G$n has evolved into an imperfect bio-android!{x", ch, NULL, NULL, TO_ROOM);
    }
    else if (GetTrans(ch) == TRANS_BIO2) {
        bonus = 8;
		act ("{GYou have evolved into a semi-perfect bio-android!{x", ch, NULL, NULL, TO_CHAR);
        act ("{G$n has evolved into a semi-perfect bio-android!{x", ch, NULL, NULL, TO_ROOM);
    }
    else if (GetTrans(ch) == TRANS_BIO3) {
        bonus = 16;
        act ("{GYou have evolved into a perfect bio-android!{x", ch, NULL, NULL, TO_CHAR);
        act ("{G$n has evolved into a perfect bio-android!{x", ch, NULL, NULL, TO_ROOM);
    }
    else if (GetTrans(ch) == TRANS_BIO4) {
        bonus = 32;
        act ("{GYou have evolved into an ultra-perfect bio-android!{x", ch, NULL, NULL, TO_CHAR);
        act ("{G$n has evolved into an ultra-perfect bio-android!{x", ch, NULL, NULL, TO_ROOM);
    }
    else {
        sendch ("!error!\n\r", ch);
        return;
    }
    if (get_skill(ch, gsn_perfect) > 0)
        ch->pcdata->learned[gsn_ultraperfect] = 1;
    else if (get_skill(ch, gsn_semiperfect) > 0)
        ch->pcdata->learned[gsn_perfect] = 1;
    else if (get_skill(ch, gsn_imperfect) > 0)
        ch->pcdata->learned[gsn_semiperfect] = 1;
    else
        ch->pcdata->learned[gsn_imperfect] = 1;
	for (i = 0; i < MAX_STATS; ++i)
		ch->perm_stat[i] += bonus;
	for (i = 0; i < MAX_SKILL; ++i) {
		if (!skill_table[i].bCanImprove || ch->pcdata->learned[i] < 1)
			continue;
		ch->pcdata->learned[i] += bonus;
		ch->pcdata->nSkillProgress[i] = 0;
	}
	ResetDiff(ch);
    return;
}

// Resets the characters stats after transforming.  Will lower them to new
// maximum if they go down a step, or heal them if they go up.
// "increased" means their pwerlevel has increased.
void reset_after_trans (CHAR_DATA *ch, bool increased) {
    // Heal them?
    if (increased && ch->trans_heal_count[GetTrans(ch)] >= 5) {
        ch->hit = ch->max_hit;
        ch->ki = ch->max_ki;
        ch->trans_heal_count[GetTrans(ch)] = 0;
    }
    // Else, make sure their stats aren't over the max
    else if (!increased) {
        ch->hit = UMIN(ch->hit, ch->max_hit);
        ch->ki = UMIN(ch->ki, ch->max_ki);
    }
}

// Add some amount to the balance of a character
void set_balance (CHAR_DATA *ch, int bal, bool show) {
    if (IS_AFFECTED(ch, AFF_FLYING))
        if (number_range(1, UMAX(0,15 - get_skill(ch, gsn_fly)/2)) == 1)
            ++bal;

    ch->balance = URANGE(0, ch->balance + bal, 10);
    // These are active, fighting messages (if the characters sitting down, don't want to see them!)
    if (show && ch->position == POS_FIGHTING) {
        if (ch->balance == 10) {
            act ("You are now in a dominating position!", ch, NULL, NULL, TO_CHAR);
            act ("$n is now in a dominating position!", ch, NULL, NULL, TO_ROOM);
        }
        else if (ch->balance == 8) {
            act ("You have manuevered yourself into a tactical position!", ch, NULL, NULL, TO_CHAR);
            act ("$n has manuevered $mself into a tactical position!", ch, NULL, NULL, TO_ROOM);
        }
        else if (ch->balance == 5) {
            act ("You manage to retain your foothold, neither gaining an advantage or a disadvantage.", ch, NULL, NULL, TO_CHAR);
            act ("$n manages to retain $s foothold, neither gaining an advantage or a disadvantage.", ch, NULL, NULL, TO_ROOM);
        }
        else if (ch->balance == 2) {
            act ("Tripping over yourself, you begin to have trouble keeping your balance.", ch, NULL, NULL, TO_CHAR);
            act ("Tripping over $mself, $n begins to have trouble keeping $s balance.", ch, NULL, NULL, TO_ROOM);
        }
        else if (ch->balance == 0) {
            act ("You are pushed aside onto the ground, unable to maintain your balance!", ch, NULL, NULL, TO_CHAR);
            act ("Unable to maintain balance, $n is pushed aside onto the ground!", ch, NULL, NULL, TO_ROOM);
        }
    }
    else if (show)
        if (ch->balance == 5)
            sendch ("Your balance has returned to normal.\n\r", ch);
    return;
}


int GetTrans (CHAR_DATA *pCh) {
    if (IS_AFFECTED (pCh, AFF_KAIOKEN))
        return TRANS_KAIOKEN;

    else if (is_affected (pCh, gsn_selffuse))
        return TRANS_SELFFUSE;
    else if (get_skill(pCh, gsn_upgrade3) > 0)
        return TRANS_UPGRADE3;
    else if (get_skill(pCh, gsn_upgrade2) > 0)
        return TRANS_UPGRADE2;
    else if (get_skill(pCh, gsn_upgrade1) > 0)
        return TRANS_UPGRADE1;

    else if (get_skill(pCh, gsn_ultraperfect) > 0)
        return TRANS_BIO5;
    else if (get_skill(pCh, gsn_perfect) > 0)
        return TRANS_BIO4;
    else if (get_skill(pCh, gsn_semiperfect) > 0)
        return TRANS_BIO3;
    else if (get_skill(pCh, gsn_imperfect) > 0)
        return TRANS_BIO2;

    else if (is_affected(pCh, gsn_form5) > 0)
        return TRANS_ICER5;
    else if (is_affected(pCh, gsn_form4) > 0)
        return TRANS_ICER4;
    else if (is_affected(pCh, gsn_form3) > 0)
        return TRANS_ICER3;
    else if (is_affected(pCh, gsn_form2) > 0)
        return TRANS_ICER2;

    else if (is_affected (pCh, gsn_ssj1))
        return TRANS_SSJ1;
    else if (is_affected (pCh, gsn_ssj2))
        return TRANS_SSJ2;
    else if (is_affected (pCh, gsn_ssj3))
        return TRANS_SSJ3;
    else if (is_affected (pCh, gsn_ssj4))
        return TRANS_SSJ4;
    else if (is_affected (pCh, gsn_ssj5))
        return TRANS_SSJ5;

    else if (is_affected (pCh, gsn_mystic))
        return TRANS_MYSTIC;

    else if (is_affected (pCh, gsn_hypern))
        return TRANS_HYPERN;
    else if (is_affected (pCh, gsn_supern))
        return TRANS_SUPERN;

    else if (is_affected (pCh, gsn_superh))
        return TRANS_SUPERH;
    else
        return TRANS_NONE;
}


bool MeetsPrereq (CHAR_DATA *pCh, int nSn) {
    int i, nPSn;
    bool bFound;
    if (!skill_table[nSn].bCanLearn)
        return FALSE;
    if (skill_table[nSn].race_prereq[0] != NULL) {
		bFound = FALSE;
		for (i=0; i<MAX_PC_RACE; ++i) {
		    if (skill_table[nSn].race_prereq[i] == NULL)
				break;
			if (race_lookup(skill_table[nSn].race_prereq[i]) == pCh->race) {
				bFound = TRUE;
				break;
			}
		}
		if (!bFound)
			return FALSE;
	}
    if (pCh->nTrueDiff < skill_table[nSn].nDiff)
        return FALSE;
    if (!IS_NPC(pCh)) {
        for (i=0; i<5; ++i) {
            if (skill_table[nSn].skill_prereq[i] == NULL)
                break;
            nPSn = skill_lookup(skill_table[nSn].skill_prereq[i]);
            if (skill_table[nSn].skill_value[i] > pCh->pcdata->learned[nPSn])
                return FALSE;
        }
    }
    for (i=0; i<MAX_STATS; ++i)
        if (pCh->perm_stat[i] < skill_table[nSn].stat_prereq[i])
			return FALSE;
    return TRUE;
}

