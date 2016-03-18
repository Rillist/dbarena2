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

/****************************************************************************
*    ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*    ROM has been brought to you by the ROM consortium                      *
*        Russ Taylor (rtaylor@hypercube.org)                                *
*        Gabrielle Taylor (gtaylor@hypercube.org)                           *
*        Brian Moore (zump@rom.org)                                         *
*    By using this code, you have agreed to follow the terms of the         *
*    ROM license, in the file Rom24/doc/rom.license                         *
****************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"

// For the memory checking
extern int nAllocString;
extern int nAllocPerm;

bool check_social args ((CHAR_DATA * ch, char *command, char *argument));
bool check_skill  args ((CHAR_DATA * ch, char *command, char *argument));

/*
 * Command logging types.
 */
#define LOG_NORMAL  0
#define LOG_ALWAYS  1
#define LOG_NEVER   2


char    last_command[MAX_STRING_LENGTH];

/*
 * Log-all switch.
 */
bool fLogAll = FALSE;

// Top node of the various BSTs
CmdNode *pCmdTopNode = NULL;
SocialNode *pSocialTopNode = NULL;
SkillNode *pSkillTopNode = NULL;

/*
 * Command table.
 */
const struct cmd_type cmd_table[] = {
    /*
     * Common movement commands.
     */
    {"north", do_north, POS_STANDING, 0, LOG_NEVER, 0, TRUE},
    {"east",  do_east,  POS_STANDING, 0, LOG_NEVER, 0, TRUE},
    {"south", do_south, POS_STANDING, 0, LOG_NEVER, 0, TRUE},
    {"west",  do_west,  POS_STANDING, 0, LOG_NEVER, 0, TRUE},
    {"up",    do_up,    POS_STANDING, 0, LOG_NEVER, 0, TRUE},
    {"down",  do_down,  POS_STANDING, 0, LOG_NEVER, 0, TRUE},

    /*
     * Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    {"at",        do_at,        POS_DEAD,     L4, LOG_NORMAL, 1, FALSE},
    {"auction",   do_auction,   POS_SLEEPING, 0,  LOG_NORMAL, 1, FALSE},
    {"buy",       do_buy,       POS_RESTING,  0,  LOG_NORMAL, 1, TRUE},
    {"channels",  do_channels,  POS_DEAD,     0,  LOG_NORMAL, 1, FALSE},
    {"exits",     do_exits,     POS_RESTING,  0,  LOG_NORMAL, 1, FALSE},
    {"get",       do_get,       POS_RESTING,  0,  LOG_NORMAL, 1, TRUE},
    {"goto",      do_goto,      POS_DEAD,     IM, LOG_NORMAL, 1, TRUE},
    {"group",     do_group,     POS_SLEEPING, 0,  LOG_NORMAL, 1, FALSE},
    {"guild",     do_guild,     POS_DEAD,     L5, LOG_ALWAYS, 1, FALSE},
    {"inventory", do_inventory, POS_DEAD,     0,  LOG_NORMAL, 1, FALSE},
    {"look",      do_look,      POS_RESTING,  0,  LOG_NORMAL, 1, FALSE},
    {"clan",      do_clantalk,  POS_SLEEPING, 0,  LOG_NORMAL, 1, FALSE},
    {"music",     do_music,     POS_SLEEPING, 0,  LOG_NORMAL, 1, FALSE},
    {"order",     do_order,     POS_RESTING,  0,  LOG_NORMAL, 1, TRUE},
    {"rest",      do_rest,      POS_SLEEPING, 0,  LOG_NORMAL, 1, TRUE},
    {"sit",       do_sit,       POS_SLEEPING, 0,  LOG_NORMAL, 1, TRUE},
    {"sockets",   do_sockets,   POS_DEAD,     L6, LOG_NORMAL, 1, FALSE},
    {"stand",     do_stand,     POS_SLEEPING, 0,  LOG_NORMAL, 1, TRUE},
    {"tell",      do_tell,      POS_RESTING,  0,  LOG_NORMAL, 1, FALSE},
    {"unlock",    do_unlock,    POS_RESTING,  0,  LOG_NORMAL, 1, TRUE},
    {"wield",     do_wear,      POS_RESTING,  0,  LOG_NORMAL, 1, TRUE},
    {"wizhelp",   do_wizhelp,   POS_DEAD,     IM, LOG_NORMAL, 1, FALSE},
	{"attack",    do_attack,    POS_FIGHTING, 0,  LOG_NORMAL, 1, TRUE},
	{"go",        do_enter,     POS_STANDING, 0,  LOG_NORMAL, 0, TRUE},
    {"quest",     do_quest,     POS_DEAD,     L4, LOG_NORMAL, 1, FALSE},
	{"medit",     do_medit,     POS_DEAD,     IM, LOG_NORMAL, 1, FALSE}, // Conflict with meditation

    /*
     * Informational commands.
     */
    {"accept",    do_accept,    POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"affects",   do_affects,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"areas",     do_areas,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
//    {"bug",       do_bug,       POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
	{"balance",   do_balance,   POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"board",     do_board,     POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"commands",  do_commands,  POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"compare",   do_compare,   POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"consider",  do_consider,  POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"count",     do_count,     POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"credits",   do_credits,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"clanremove",do_clanremove,POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"equipment", do_equipment, POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"examine",   do_examine,   POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"help",      do_help,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"join",      do_join,      POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"loner",     do_loner,     POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"motd",      do_motd,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"read",      do_read,      POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"report",    do_report,    POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"rules",     do_rules,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"score",     do_score,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"skills",    do_skills,    POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"socials",   do_socials,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"show",      do_show,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"story",     do_story,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"time",      do_time,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"toplist",   do_toplist,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
//    {"typo",      do_typo,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"weather",   do_weather,   POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"who",       do_who,       POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"wizlist",   do_wizlist,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"worth",     do_worth,     POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},

    /*
     * Configuration commands.
     */
    {"alia",        do_alia,        POS_DEAD,     0, LOG_NORMAL, 0, FALSE},
    {"alias",       do_alias,       POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autolist",    do_autolist,    POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autoall",     do_autoall,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autoassist",  do_autoassist,  POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autoexit",    do_autoexit,    POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autozenni",   do_autozenni,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autoloot",    do_autoloot,    POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autosac",     do_autosac,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autosplit",   do_autosplit,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autoattack",  do_autoattack,  POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"autoweather", do_autoweather, POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"brief",       do_brief,       POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
/*  { "channels",    do_channels,    POS_DEAD,     0,  LOG_NORMAL, 1 }, */
    {"colour",	    do_colour,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"color",	    do_colour,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"combine",	    do_combine,  	POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"compact",	    do_compact,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"description",	do_description,	POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"delet",		do_delet,	    POS_DEAD,	  0, LOG_ALWAYS, 0, FALSE},
    {"delete",		do_delete,	    POS_STANDING, 0, LOG_ALWAYS, 1, FALSE},
    {"finishingmove", do_finishingmove, POS_DEAD, 0, LOG_NORMAL, 1, FALSE},
    {"nofollow",	do_nofollow,	POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"noloot",		do_noloot,   	POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"nosummon",	do_nosummon,	POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"password",	do_password,	POS_DEAD,	  0, LOG_NEVER,  1, FALSE},
    {"pose",        do_pose,        POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"prompt",		do_prompt,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"scroll",		do_scroll,	    POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"reveal",      do_reveal,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"telnetga",	do_telnetga,	POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"title",		do_title,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"unalias",		do_unalias,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},
    {"wimpy",		do_wimpy,	    POS_DEAD,	  0, LOG_NORMAL, 1, FALSE},

    /*
     * Communication commands.
     */
    {"afk",      do_afk,      POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"answer",   do_answer,   POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
/*  { "auction",   do_auction,    POS_SLEEPING,     0,  LOG_NORMAL, 1 }, */
    {"deaf",     do_deaf,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"emote",    do_emote,    POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"pmote",    do_pmote,    POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {".",        do_gossip,   POS_SLEEPING, 0, LOG_NORMAL, 0, FALSE},
    {"gossip",   do_gossip,   POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {",",        do_emote,    POS_RESTING,  0, LOG_NORMAL, 0, FALSE},
    {"grats",    do_grats,    POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"gtell",    do_gtell,    POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {";",        do_gtell,    POS_DEAD,     0, LOG_NORMAL, 0, FALSE},
/*  { "music",        do_music,    POS_SLEEPING,     0,  LOG_NORMAL, 1 }, */
    {"note",     do_note,     POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"ooc",      do_ooc,      POS_DEAD,     0, LOG_NORMAL, 1, FALSE},
    {"question", do_question, POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"quote",    do_quote,    POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"quiet",    do_quiet,    POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"reply",    do_reply,    POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"replay",   do_replay,   POS_SLEEPING, 0, LOG_NORMAL, 1, FALSE},
    {"say",      do_say,      POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"'",        do_say,      POS_RESTING,  0, LOG_NORMAL, 0, FALSE},
    {"shout",    do_shout,    POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
    {"yell",     do_yell,     POS_RESTING,  0, LOG_NORMAL, 1, FALSE},

    /*
     * Object manipulation commands.
     */
    {"brandish",  do_brandish,  POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"close",     do_close,     POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"drink",     do_drink,     POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"drop",      do_drop,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"eat",       do_eat,       POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"fill",      do_fill,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"give",      do_give,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"hold",      do_wear,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"list",      do_list,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"lock",      do_lock,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"open",      do_open,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"pick",      do_pick,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"pour",      do_pour,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"put",       do_put,       POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"quaff",     do_quaff,     POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"recite",    do_recite,    POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"remove",    do_remove,    POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"sell",      do_sell,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"take",      do_get,       POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"sacrifice", do_sacrifice, POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"junk",      do_sacrifice, POS_RESTING, 0, LOG_NORMAL, 0, TRUE},
    {"tap",       do_sacrifice, POS_RESTING, 0, LOG_NORMAL, 0, TRUE},
/*  { "unlock",   do_unlock,    POS_RESTING, 0,  LOG_NORMAL, 1 }, */
    {"value",     do_value,     POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"wear",      do_wear,      POS_RESTING, 0, LOG_NORMAL, 1, TRUE},
    {"zap",       do_zap,       POS_RESTING, 0, LOG_NORMAL, 1, TRUE},

    /*
     * Transformations
     */
    {"ssj1",       do_ssj1,     POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"ssj2",       do_ssj2,     POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"ssj3",       do_ssj3,     POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"ssj4",       do_ssj4,     POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"ssj5",       do_ssj5,     POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"mystic",     do_mystic,   POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"super",      do_super,    POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"hyper",      do_hyper,    POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"revert",     do_revert,   POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"upgrade",    do_upgrade,  POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"selffuse",   do_selffuse, POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"transup",    do_transup,  POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"transdown",  do_transdown,POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"fuse",       do_fuse,     POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
    {"unfuse",     do_unfuse,   POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
    {"kaioken",    do_kaioken,  POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"fly",        do_fly,      POS_RESTING,  0, LOG_NORMAL, 1, TRUE},

    /*
     * Combat commands.
     */
    {"defend",        do_defend,        POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
	{"cancel",        do_cancel,        POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE},
	{"release",       do_release,       POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE},
	{"power",         do_power,         POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"berserk",       do_berserk,       POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"disarm",        do_disarm,        POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"flee",          do_flee,          POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"rescue",        do_rescue,        POS_FIGHTING, 0, LOG_NORMAL, 0, TRUE},
	{"retreat",       do_retreat,       POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"surrender",     do_surrender,     POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"stance",        do_stance,        POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"powerstruggle", do_powerstruggle, POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"struggle",      do_powerstruggle, POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"addki",         do_addki,         POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE},

    /*
     * Mob command interpreter (placed here for faster scan...)
     */
    {"mob", do_mob, POS_DEAD, 0, LOG_NEVER, 0, FALSE},

    /*
     * Miscellaneous commands.
     */
    {"customskill", do_customskill, POS_DEAD, 0, LOG_NORMAL, 1, FALSE},
	{"enter",   do_enter,   POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
    {"follow",  do_follow,  POS_RESTING,  0, LOG_NORMAL, 1, FALSE},
/*  {"go",      do_enter,   POS_STANDING, 0, LOG_NORMAL, 0, TRUE}, */
/*  { "group",        do_group,    POS_SLEEPING,     0,  LOG_NORMAL, 1 }, */
    {"hide",    do_hide,    POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"mission",	do_mission, POS_FIGHTING, 0, LOG_NORMAL, 1, TRUE},
    {"qui",     do_qui,     POS_DEAD,     0, LOG_NORMAL, 0, TRUE},
    {"quit",    do_quit,    POS_DEAD,     0, LOG_NORMAL, 1, TRUE},
    {"save",    do_save,    POS_DEAD,     0, LOG_NORMAL, 1, TRUE},
    {"scan",    do_scan,    POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"sleep",   do_sleep,   POS_SLEEPING, 0, LOG_NORMAL, 1, TRUE},
    {"sneak",   do_sneak,   POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
    {"split",   do_split,   POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"steal",   do_steal,   POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
    {"suppress",do_suppress,POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    {"visible", do_visible, POS_SLEEPING, 0, LOG_NORMAL, 1, TRUE},
    {"wake",    do_wake,    POS_SLEEPING, 0, LOG_NORMAL, 1, TRUE},
    {"where",   do_where,   POS_RESTING,  0, LOG_NORMAL, 1, TRUE},
    // Training sub-group
    {"pushup",   do_pushup,   POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
    {"meditate", do_meditate, POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
	{"stretch",  do_stretch,  POS_STANDING, 0, LOG_NORMAL, 1, TRUE},	
	{"study",    do_study,    POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
	{"teach",    do_teach,    POS_STANDING, 0, LOG_NORMAL, 1, TRUE},
	{"listen",   do_listen,   POS_STANDING, 0, LOG_NORMAL, 1, TRUE},

    /*
     * Immortal commands.
	 * Organized by level.
     */
	// Builders
    {"combatstat",  do_combatstat,  POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"combatinfo",  do_combatinfo,  POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"freevnum",    do_fvlist,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
//  { "goto",       do_goto,        POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"holylight",	do_holylight,	POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"immtalk",		do_immtalk,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {":",			do_immtalk,		POS_DEAD, IM, LOG_NORMAL, 0, FALSE},
	{"immtitle",	do_immtitle,	POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"imotd",		do_imotd,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"incognito",	do_incognito,	POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"invis",		do_invis,		POS_DEAD, IM, LOG_NORMAL, 0, FALSE},
	{"poofin",		do_bamfin,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"poofout",		do_bamfout,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"vnum",		do_vnum,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"wizinvis",	do_invis,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"wiznet",		do_wiznet,		POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    // OLC
    {"aedit",       do_aedit,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"alist",       do_alist,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"asave",       do_asave,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"edit",        do_olc,         POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"hedit",       do_hedit,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
//  {"medit",       do_medit,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"mlist",       do_mlist,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"mplist",      do_mplist,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"mpdump",		do_mpdump,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"mpedit",      do_mpedit,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"mpstat",		do_mpstat,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"oedit",       do_oedit,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"opedit",      do_opedit,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"olist",       do_olist,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"oplist",      do_oplist,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"opdump",		do_opdump,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"opstat",		do_opstat,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"resets",      do_resets,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"redit",       do_redit,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},    
	{"rlist",       do_rlist,       POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"rpdump",		do_rpdump,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
    {"rpedit",      do_rpedit,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"rplist",      do_rplist,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},
	{"rpstat",		do_rpstat,      POS_DEAD, IM, LOG_NORMAL, 1, FALSE},

    // Enforcers
//  { "at",         do_at,          POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
	{"clone",		do_clone,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"echo",		do_recho,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"force",		do_force,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"gecho",		do_echo,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"grant",       do_grant,       POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"load",		do_load,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"mwhere",		do_mwhere,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
	{"nochannels",	do_nochannels,	POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"noemote",		do_noemote,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"noshout",		do_noshout,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"notell",		do_notell,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"owhere",		do_owhere,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
	{"purge",		do_purge,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"peace",		do_peace,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
    {"pecho",		do_pecho,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"prefi",		do_prefi,		POS_DEAD, L4, LOG_NORMAL, 0, FALSE},
    {"prefix",		do_prefix,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
//  {"quest",       do_quest,       POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
    {"randobj",     do_randobj,     POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"restore",		do_restore,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"return",		do_return,		POS_DEAD,  0, LOG_NORMAL, 1, FALSE},
    {"reward",      do_reward,      POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"skillprereq", do_skillprereq, POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
	{"smote",		do_smote,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
	{"stat",		do_stat,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},
    {"string",		do_string,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"switch",		do_switch,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
	{"teleport",	do_transfer,	POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"transfer",	do_transfer,	POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    {"unwait",		do_unwait,		POS_DEAD, L4, LOG_NORMAL, 1, FALSE},	
	{"zecho",		do_zecho,		POS_DEAD, L4, LOG_ALWAYS, 1, FALSE},
    
    // Head Builder
	{"advance",		do_advance,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"backup",      do_backup,      POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"deny",		do_deny,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
    {"disconnect",	do_disconnect,	POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},	
	{"flag",		do_flag,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"freeze",		do_freeze,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"godtalk",     do_god,			POS_DEAD, L5, LOG_NORMAL, 1, FALSE},
	{"multilink",   do_multilink,   POS_DEAD, L5, LOG_NORMAL, 1, FALSE},
	{"pardon",		do_pardon,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"skillstat",   do_skillstat,   POS_DEAD, L5, LOG_NORMAL, 1, FALSE},
    {"sla",		    do_sla,			POS_DEAD, L5, LOG_NORMAL, 0, FALSE},
    {"slay",		do_slay,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"snoop",		do_snoop,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
    {"violate",		do_violate,		POS_DEAD, L5, LOG_ALWAYS, 1, FALSE},
	{"wpeace",		do_wpeace,		POS_DEAD, L5, LOG_NORMAL, 1, FALSE},

	// Coder
	{"copyover",	do_copyover,	POS_DEAD, L6, LOG_ALWAYS, 1, FALSE},
    {"dump",		do_dump,		POS_DEAD, L6, LOG_ALWAYS, 0, FALSE},
	{"log",		    do_log,			POS_DEAD, L6, LOG_ALWAYS, 1, FALSE},
    {"memory",		do_memory,		POS_DEAD, L6, LOG_NORMAL, 1, FALSE},
//  {"sockets",     do_sockets,     POS_DEAD, L6, LOG_NORMAL, 1, FALSE},
	{"varlimit",    do_varlimit,    POS_DEAD, L6, LOG_NORMAL, 1, FALSE},
    
	// Head Coder (let them do all the same stuff as the Implementor)
    {"allow",		do_allow,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"ban",		    do_ban,			POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"cleader",     do_cleader,     POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"givecustom",  do_givecustom,  POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"incinerat",   do_incinerat,   POS_DEAD, L7, LOG_NORMAL, 0, FALSE},
    {"incinerate",  do_incinerate,  POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
    {"invade",		do_invade,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"newlock",		do_newlock,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
    {"permban",		do_permban,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
    {"protect",		do_protect,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"qmconfig",	do_qmconfig,	POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
    {"reboo",		do_reboo,		POS_DEAD, L7, LOG_NORMAL, 0, FALSE},
    {"reboot",		do_reboot,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"rename",      do_rename,      POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"set",		    do_set,			POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"shutdow",		do_shutdow,		POS_DEAD, L7, LOG_NORMAL, 0, FALSE},
    {"shutdown",	do_shutdown,	POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
	{"skillchange", do_skillchange,	POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},    
    {"wizlock",		do_wizlock,		POS_DEAD, L7, LOG_ALWAYS, 1, FALSE},
        
	/*
     * End of list.
     */
    {"", 0, POS_DEAD, 0, LOG_NORMAL, 0}
};




/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret (CHAR_DATA * ch, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int trust;
    bool found;
    // Memory checking:
	int string_count = nAllocString;
	int perm_count = nAllocPerm;
	char cmd_copy[MAX_INPUT_LENGTH];
	// BST searching:
	CmdNode *pCur;
	int nCmp;

    /*
     * Strip leading spaces.
     */
    while (isspace (*argument))
        argument++;
    if (argument[0] == '\0')
        return;

    /*
     * No hiding.
     */
    REMOVE_BIT (ch->affected_by, AFF_HIDE);

    /*
     * Implement freeze command.
     */
    if (!IS_NPC (ch) && IS_SET (ch->act, PLR_FREEZE))
    {
        sendch ("You're totally frozen!\n\r", ch);
        return;
    }

	/*
     * Grab the command word.
     * Special parsing so ' can be a command,
     * also no spaces needed after punctuation.
     */
    strcpy (logline, argument);

    /*Lets see who is doing what? -Ferric*/
    strcpy (buf, argument);
    sprintf (last_command,"%s in room[%d]: %s.", ch->name, ch->in_room ? ch->in_room->vnum : -1, buf);
	strcpy (cmd_copy, argument);

    if (!isalpha (argument[0]) && !isdigit (argument[0]))
    {
        command[0] = argument[0];
        command[1] = '\0';
        argument++;
        while (isspace (*argument))
            argument++;
    }
    else
    {
        argument = one_argument (argument, command);
    }

    /*
     * Look for the command in the command tree
     */
    trust = ch->level;
	pCur = pCmdTopNode;
	cmd = 0;
	found = FALSE;
	while (TRUE) {
		if (pCur == NULL)
			break;
        if (command[0] == cmd_table[pCur->nNum].name[0]
			&& !str_prefix (command, cmd_table[pCur->nNum].name)
			&& cmd_table[pCur->nNum].level <= trust) {
			found = TRUE;
			cmd = pCur->nNum;
			break;
		}
		else if (command[0] < cmd_table[pCur->nNum].name[0])
			pCur = pCur->pLeft;
		else if (command[0] > cmd_table[pCur->nNum].name[0])
			pCur = pCur->pRight;
		else if ((nCmp = strcmp (command, cmd_table[pCur->nNum].name)) < 0)
			pCur = pCur->pLeft;
		else if (nCmp > 0)
			pCur = pCur->pRight;
		else
            break;
    }
/*
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
    {
        if (command[0] == cmd_table[cmd].name[0]
            && !str_prefix (command, cmd_table[cmd].name)
            && cmd_table[cmd].level <= trust)
        {
            found = TRUE;
            break;
        }
    }
*/
    /*
     * Log and snoop.
     */
    smash_dollar(logline);

    if (cmd_table[cmd].log == LOG_NEVER)
        strcpy (logline, "");

	/* Replaced original block of code with fix from Edwin
	 * to prevent crashes due to dollar signs in logstrings.
	 * I threw in the above call to smash_dollar() just for
	 * the sake of overkill :) JR -- 10/15/00
	 */
    if ( ( !IS_NPC(ch) && IS_SET(ch->act, PLR_LOG) )
		||   fLogAll
		||   cmd_table[cmd].log == LOG_ALWAYS )
	{
    	char    s[2*MAX_INPUT_LENGTH],*ps;
    	int     i;

    	ps=s; 
    	sprintf( log_buf, "Log %s: %s", ch->name, logline );
    	/* Make sure that was is displayed is what is typed */
    	for (i=0;log_buf[i];i++)
    	{
			*ps++=log_buf[i];
			if (log_buf[i]=='$')
	    		*ps++='$';
			if (log_buf[i]=='{')
	    		*ps++='{';
    	}
    	*ps=0;
    	wiznet(s,ch,NULL,WIZ_SECURE,0,ch->level);
        logstr (LOG_COMMAND, log_buf);
        //log_string( log_buf );
	}

    if (ch->desc != NULL && ch->desc->snoop_by != NULL)
    {
        write_to_buffer (ch->desc->snoop_by, "% ", 2);
        write_to_buffer (ch->desc->snoop_by, logline, 0);
        write_to_buffer (ch->desc->snoop_by, "\n\r", 2);
    }

	if (!found)
    {
	    /*
         * Look for command in skills table, and then the socials table.
         */
        if (!check_skill (ch, command, argument)) {
			if (!check_social (ch, command, argument)) {
				sendch ("Huh?\n\r", ch);
				return;
			}
		}

		if (string_count < nAllocString) {
			sprintf(buf, "Memcheck : Increase in strings :: %s : %s", ch->name, cmd_copy) ;
			wiznet(buf, NULL, NULL, WIZ_MEMCHECK,0,0) ;
		}

		if (perm_count < nAllocPerm) {
			sprintf(buf, "Memcheck : Increase in perms :: %s : %s", ch->name, cmd_copy) ;
			wiznet(buf, NULL, NULL, WIZ_MEMCHECK, 0,0) ;
		}
			
        return;
    }

	// Can't use the command if waiting
	if (cmd_table[cmd].wait && (ch->wait > 0 || (ch->wait_skill_sn > 0 && (ch->wait_skill > 0 || ch->charge > 0))) ) {
		if (IS_SET(ch->act, PLR_AUTOATTACK)) {
			strcpy (ch->cmd_buf, command);
            strcat (ch->cmd_buf, argument);
        }
		else
			sendch ("You're busy.  Wait a bit longer.\n\r", ch);
		return;
	}

	/*
     * Character not in position for command?
     */
    if (ch->position < cmd_table[cmd].position)
    {
        switch (ch->position)
        {
            case POS_DEAD:
                sendch ("Lie still; you are DEAD.\n\r", ch);
                break;

            case POS_UNCONSCIOUS:
                sendch ("You have been knocked unconscious.\n\r", ch);
                break;

            case POS_MORTAL:
            case POS_INCAP:
                sendch ("You are hurt far too bad for that.\n\r", ch);
                break;

            case POS_STUNNED:
                sendch ("You are too stunned to do that.\n\r", ch);
                break;

            case POS_SLEEPING:
                sendch ("In your dreams, or what?\n\r", ch);
                break;

            case POS_RESTING:
                sendch ("Nah... You feel too relaxed...\n\r", ch);
                break;

            case POS_SITTING:
                sendch ("Better stand up first.\n\r", ch);
                break;

            case POS_FIGHTING:
                sendch ("No way!  You are still fighting!\n\r", ch);
                break;

        }
        return;
    }

    /*
     * Dispatch the command.
     */
    (*cmd_table[cmd].do_fun) (ch, argument);

	if (string_count < nAllocString) {
		sprintf(buf, "Memcheck : Increase in strings :: %s : %s", ch->name, cmd_copy) ;
		wiznet(buf, NULL, NULL, WIZ_MEMCHECK,0,0) ;
	}

	if (perm_count < nAllocPerm) {
		sprintf(buf, "Memcheck : Increase in perms :: %s : %s", ch->name, cmd_copy) ;
		wiznet(buf, NULL, NULL, WIZ_MEMCHECK, 0,0) ;
	}

    tail_chain ();
    return;
}

/* function to keep argument safe in all commands -- no static strings */
void do_function (CHAR_DATA * ch, DO_FUN * do_fun, char *argument)
{
    char *command_string;

    /* copy the string */
    command_string = str_dup (argument);

    /* dispatch the command */
    (*do_fun) (ch, command_string);

    /* free the string */
    free_string (command_string);
}

bool check_social (CHAR_DATA * ch, char *command, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    int cmd = 0;
    bool found = FALSE;
	SocialNode *pCur;
	int nCmp;

	pCur = pSocialTopNode;
	while (TRUE) {
		if (pCur == NULL)
			break;
		if (command[0] == social_table[pCur->nNum].name[0]
			&& !str_prefix (command, social_table[pCur->nNum].name)) {
			found = TRUE;
			cmd = pCur->nNum;
			break;
		}
		else if (command[0] < social_table[pCur->nNum].name[0])
			pCur = pCur->pLeft;
		else if (command[0] > social_table[pCur->nNum].name[0])
			pCur = pCur->pRight;
		else if ((nCmp = strcmp (command, social_table[pCur->nNum].name)) < 0)
			pCur = pCur->pLeft;
		else if (nCmp > 0)
			pCur = pCur->pRight;
		else
			break;
	}
/*
    found = FALSE;
    for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
    {
        if (command[0] == social_table[cmd].name[0]
            && !str_prefix (command, social_table[cmd].name))
        {
            found = TRUE;
            break;
        }
    }
*/
    if (!found)
        return FALSE;

    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE))
    {
        sendch ("You are anti-social!\n\r", ch);
        return TRUE;
    }

    switch (ch->position)
    {
        case POS_DEAD:
            sendch ("Lie still; you are DEAD.\n\r", ch);
            return TRUE;

        case POS_UNCONSCIOUS:
            sendch ("You have been knocked unconscious.\n\r", ch);
            break;

        case POS_INCAP:
        case POS_MORTAL:
            sendch ("You are hurt far too bad for that.\n\r", ch);
            return TRUE;

        case POS_STUNNED:
            sendch ("You are too stunned to do that.\n\r", ch);
            return TRUE;

        case POS_SLEEPING:
            /*
             * I just know this is the path to a 12" 'if' statement.  :(
             * But two players asked for it already!  -- Furey
             */
            if (!str_cmp (social_table[cmd].name, "snore"))
                break;
            sendch ("In your dreams, or what?\n\r", ch);
            return TRUE;

    }

    one_argument (argument, arg);
    victim = NULL;
    if (arg[0] == '\0')
    {
        act (social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM);
        act (social_table[cmd].char_no_arg, ch, NULL, victim, TO_CHAR);
    }
    else if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        sendch ("They aren't here.\n\r", ch);
    }
    else if (victim == ch)
    {
        act (social_table[cmd].others_auto, ch, NULL, victim, TO_ROOM);
        act (social_table[cmd].char_auto, ch, NULL, victim, TO_CHAR);
    }
    else
    {
        act (social_table[cmd].others_found, ch, NULL, victim, TO_NOTVICT);
        act (social_table[cmd].char_found, ch, NULL, victim, TO_CHAR);
        act (social_table[cmd].vict_found, ch, NULL, victim, TO_VICT);

        if (!IS_NPC (ch) && IS_NPC (victim)
            && !IS_AFFECTED (victim, AFF_CHARM)
            && IS_AWAKE (victim) && victim->desc == NULL)
        {
            switch (number_bits (4))
            {
                case 0:

                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    act (social_table[cmd].others_found,
                         victim, NULL, ch, TO_NOTVICT);
                    act (social_table[cmd].char_found, victim, NULL, ch,
                         TO_CHAR);
                    act (social_table[cmd].vict_found, victim, NULL, ch,
                         TO_VICT);
                    break;

                case 9:
                case 10:
                case 11:
                case 12:
                    act ("$n slaps $N.", victim, NULL, ch, TO_NOTVICT);
                    act ("You slap $N.", victim, NULL, ch, TO_CHAR);
                    act ("$n slaps you.", victim, NULL, ch, TO_VICT);
                    break;
            }
        }
    }

    return TRUE;
}



bool check_skill (CHAR_DATA * ch, char *command, char *argument)
{
    int sn = 0;
    bool found = FALSE;
	SkillNode *pCur;
	int nCmp;
/*
    found = FALSE;
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].command
			&& (( command[0] == skill_table[sn].name[0]
				  && !str_prefix (command, skill_table[sn].name) )
			 || ( skill_table[sn].syntax[0] != '\0'
			      && command[0] == skill_table[sn].syntax[0]
				  && !str_prefix (command, skill_table[sn].syntax) ))
			)
        {
            found = TRUE;
            break;
        }
    }
*/
	pCur = pSkillTopNode;
	while (TRUE) {
		if (pCur == NULL)
			break;
		if (command[0] == pCur->szName[0]
			&& !str_prefix (command, pCur->szName)) {
			found = TRUE;
			sn = pCur->nSn;
			break;
		}
		else if (command[0] < pCur->szName[0])
			pCur = pCur->pLeft;
		else if (command[0] > pCur->szName[0])
			pCur = pCur->pRight;
		else if ((nCmp = strcmp (command, pCur->szName)) < 0)
			pCur = pCur->pLeft;
		else if (nCmp > 0)
			pCur = pCur->pRight;
		else
			break;
	}


    if (!found)
        return FALSE;

	// Can't use the command if waiting
	if (ch->wait > 0 || (ch->wait_skill_sn > 0 && (ch->wait_skill > 0 || ch->charge > 0))) {
		if (IS_SET(ch->act, PLR_AUTOATTACK)) {
			strcpy (ch->cmd_buf, command);
            strcat (ch->cmd_buf, argument);
        }
		else
			sendch ("You're busy.  Wait a bit longer.\n\r", ch);
		return TRUE;
	}

    if (ch->position < skill_table[sn].minimum_position)
    {
        switch (ch->position)
        {
            case POS_DEAD:
                sendch ("Lie still; you are DEAD.\n\r", ch);
                break;

            case POS_UNCONSCIOUS:
                sendch ("You have been knocked unconscious.\n\r", ch);
                break;

            case POS_MORTAL:
            case POS_INCAP:
                sendch ("You are hurt far too bad for that.\n\r", ch);
                break;

            case POS_STUNNED:
                sendch ("You are too stunned to do that.\n\r", ch);
                break;

            case POS_SLEEPING:
                sendch ("In your dreams, or what?\n\r", ch);
                break;

            case POS_RESTING:
                sendch ("Nah... You feel too relaxed...\n\r", ch);
                break;

            case POS_SITTING:
                sendch ("Better stand up first.\n\r", ch);
                break;

            case POS_FIGHTING:
                sendch ("No way!  You are still fighting!\n\r", ch);
                break;

        }
        return TRUE;
    }

	// start the skill
	skill_driver(ch, argument, sn);

    return TRUE;
}




/*
 * Return true if an argument is completely numeric.
 */
bool is_number (char *arg)
{

    if (*arg == '\0')
        return FALSE;

    if (*arg == '+' || *arg == '-')
        arg++;

    for (; *arg != '\0'; arg++)
    {
        if (!isdigit (*arg))
            return FALSE;
    }

    return TRUE;
}



/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument (char *argument, char *arg)
{
    char *pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++)
    {
        if (*pdot == '.')
        {
            *pdot = '\0';
            number = atoi (argument);
            *pdot = '.';
            strcpy (arg, pdot + 1);
            return number;
        }
    }

    strcpy (arg, argument);
    return 1;
}

/* 
 * Given a string like 14*foo, return 14 and 'foo'
*/
int mult_argument (char *argument, char *arg)
{
    char *pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++)
    {
        if (*pdot == '*')
        {
            *pdot = '\0';
            number = atoi (argument);
            *pdot = '*';
            strcpy (arg, pdot + 1);
            return number;
        }
    }

    strcpy (arg, argument);
    return 1;
}



/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char *one_argument (char *argument, char *arg_first)
{
    char cEnd;

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
        *arg_first = LOWER (*argument);
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (isspace (*argument))
        argument++;

    return argument;
}

/*
 * Contributed by Alander.
 */
void do_commands (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;

    col = 0;
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
    {
        if (cmd_table[cmd].level < LEVEL_HERO
            && cmd_table[cmd].level <= ch->level && cmd_table[cmd].show)
        {
            sprintf (buf, "%-14.14s", cmd_table[cmd].name);
            sendch (buf, ch);
            if (++col % 5 == 0)
                sendch ("\n\r", ch);
        }
    }

    if (col % 5 != 0)
        sendch ("\n\r", ch);
    return;
}

void do_wizhelp (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;

    col = 0;
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
    {
        if (cmd_table[cmd].level >= LEVEL_HERO
            && cmd_table[cmd].level <= ch->level && cmd_table[cmd].show)
        {
            sprintf (buf, "%-14.14s", cmd_table[cmd].name);
            sendch (buf, ch);
            if (++col % 5 == 0)
                sendch ("\n\r", ch);
        }
    }

    if (col % 5 != 0)
        sendch ("\n\r", ch);
    return;
}

void GenerateCmdBST (void) {
    CmdNode *pCur;
    char *szCurCmd, *szCompare;
    int nCmd;
	int nCmp;

    pCmdTopNode = alloc_perm (sizeof(*pCmdTopNode));
	pCmdTopNode->nNum = 0;
	pCmdTopNode->pRight = NULL;
	pCmdTopNode->pLeft = NULL;

	for (nCmd = 1; cmd_table[nCmd].name[0] != '\0'; ++nCmd) {
		pCur = pCmdTopNode;
        szCurCmd = cmd_table[nCmd].name;
		while (TRUE) {
            szCompare = cmd_table[pCur->nNum].name;
            if (szCurCmd[0] < szCompare[0]) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->nNum = nCmd;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
            else if (szCurCmd[0] > szCompare[0]) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->nNum = nCmd;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else if ((nCmp = strcmp (szCurCmd, szCompare)) < 0) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->nNum = nCmd;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
			else if (nCmp > 0) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->nNum = nCmd;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else
                break;
		}
	}
}

void GenerateSocialBST (void) {
    SocialNode *pCur;
    char *szCurSoc, *szCompare;
    int nSoc;
	int nCmp;

    pSocialTopNode = alloc_perm (sizeof(*pSocialTopNode));
	pSocialTopNode->nNum = 0;
	pSocialTopNode->pRight = NULL;
	pSocialTopNode->pLeft = NULL;

	for (nSoc = 1; social_table[nSoc].name[0]; ++nSoc) {
		pCur = pSocialTopNode;
        szCurSoc = social_table[nSoc].name;
		while (TRUE) {
            szCompare = social_table[pCur->nNum].name;
            if (szCurSoc[0] < szCompare[0]) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->nNum = nSoc;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
            else if (szCurSoc[0] > szCompare[0]) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->nNum = nSoc;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else if ((nCmp = strcmp (szCurSoc, szCompare)) < 0) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->nNum = nSoc;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
			else if (nCmp > 0) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->nNum = nSoc;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else
                break;
		}
	}
}

void GenerateSkillBST (void) {
    SkillNode *pCur;
    char *szCurSkill, *szCompare;
    bool bFound = FALSE;
    int nSn, nFirstSn;
	int nCmp;

    // Look for the first applicable skill
    for (nFirstSn = 0; nFirstSn < MAX_SKILL; ++nFirstSn) {
        if (!skill_table[nFirstSn].command) {
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
       return;
    pSkillTopNode = alloc_perm (sizeof(*pSkillTopNode));
	pSkillTopNode->nSn = nFirstSn;
	pSkillTopNode->szName = str_dup (skill_table[nFirstSn].name);
	pSkillTopNode->pRight = NULL;
	pSkillTopNode->pLeft = NULL;

	// Go through looking at the actual skill names
	for (nSn = nFirstSn+1; nSn < MAX_SKILL; ++nSn) {
        if (!skill_table[nSn].command)
            continue;
        pCur = pSkillTopNode;
        szCurSkill = skill_table[nSn].name;
		while (TRUE) {
            szCompare = pCur->szName;
            if (szCurSkill[0] < szCompare[0]) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->szName = str_dup (szCurSkill);
					pCur->pLeft->nSn = nSn;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
            else if (szCurSkill[0] > szCompare[0]) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->szName = str_dup (szCurSkill);
					pCur->pRight->nSn = nSn;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else if ((nCmp = strcmp (szCurSkill, szCompare)) < 0) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->szName = str_dup (szCurSkill);
					pCur->pLeft->nSn = nSn;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
			else if (nCmp > 0) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->szName = str_dup (szCurSkill);
					pCur->pRight->nSn = nSn;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else
                break;
		}
	}
	// And again with the syntax
	for (nSn = 0; nSn < MAX_SKILL; ++nSn) {
		if (!skill_table[nSn].command || skill_table[nSn].syntax[0] == '\0')
			continue;
        pCur = pSkillTopNode;
        szCurSkill = skill_table[nSn].syntax;
		while (TRUE) {
            szCompare = pCur->szName;
            if (szCurSkill[0] <szCompare[0]) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->szName = str_dup (szCurSkill);
					pCur->pLeft->nSn = nSn;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
            else if (szCurSkill[0] > szCompare[0]) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->szName = str_dup (szCurSkill);
					pCur->pRight->nSn = nSn;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else if ((nCmp = strcmp (szCurSkill, szCompare)) < 0) {
				if (pCur->pLeft)
					pCur = pCur->pLeft;
				else {
					pCur->pLeft = alloc_perm (sizeof(*pCur));
					pCur->pLeft->szName = str_dup (szCurSkill);
					pCur->pLeft->nSn = nSn;
					pCur->pLeft->pRight = NULL;
					pCur->pLeft->pLeft = NULL;
					break;
				}
			}
			else if (nCmp > 0) {
				if (pCur->pRight)
					pCur = pCur->pRight;
				else {
					pCur->pRight = alloc_perm (sizeof(*pCur));
					pCur->pRight->szName = str_dup (szCurSkill);
					pCur->pRight->nSn = nSn;
					pCur->pRight->pRight = NULL;
					pCur->pRight->pLeft = NULL;
					break;
				}
			}
			else
                break;
		}
	}
}


