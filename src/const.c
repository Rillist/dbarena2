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
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "interp.h"


/* item type list */
const struct item_type item_table[] = {
    {ITEM_LIGHT, "light"},
    {ITEM_SCROLL, "scroll"},
    {ITEM_WAND, "wand"},
    {ITEM_STAFF, "staff"},
    {ITEM_WEAPON, "weapon"},
    {ITEM_TREASURE, "treasure"},
    {ITEM_ARMOR, "armor"},
    {ITEM_POTION, "potion"},
    {ITEM_CLOTHING, "clothing"},
    {ITEM_FURNITURE, "furniture"},
    {ITEM_TRASH, "trash"},
    {ITEM_CONTAINER, "container"},
    {ITEM_DRINK_CON, "drink"},
    {ITEM_KEY, "key"},
    {ITEM_FOOD, "food"},
    {ITEM_MONEY, "money"},
    {ITEM_BOAT, "boat"},
    {ITEM_CORPSE_NPC, "npc_corpse"},
    {ITEM_CORPSE_PC, "pc_corpse"},
    {ITEM_FOUNTAIN, "fountain"},
    {ITEM_PILL, "pill"},
    {ITEM_PROTECT, "protect"},
    {ITEM_MAP, "map"},
    {ITEM_PORTAL, "portal"},
    {ITEM_WARP_STONE, "warp_stone"},
    {ITEM_ROOM_KEY, "room_key"},
    {ITEM_GEM, "gem"},
    {ITEM_JEWELRY, "jewelry"},
    {0, NULL}
};


/* weapon selection table */
const struct weapon_type weapon_table[] = {
    {"sword", OBJ_VNUM_SCHOOL_SWORD, WEAPON_SWORD, &gsn_sword},
    {"mace", OBJ_VNUM_SCHOOL_MACE, WEAPON_MACE, &gsn_mace},
    {"dagger", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER, &gsn_dagger},
    {"axe", OBJ_VNUM_SCHOOL_AXE, WEAPON_AXE, &gsn_axe},
    {"staff", OBJ_VNUM_SCHOOL_STAFF, WEAPON_SPEAR, &gsn_spear},
    {"flail", OBJ_VNUM_SCHOOL_FLAIL, WEAPON_FLAIL, &gsn_flail},
    {"whip", OBJ_VNUM_SCHOOL_WHIP, WEAPON_WHIP, &gsn_whip},
    {"polearm", OBJ_VNUM_SCHOOL_POLEARM, WEAPON_POLEARM, &gsn_polearm},
    {NULL, 0, 0, NULL}
};



/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] = {
    {"on", WIZ_ON, IM},
    {"prefix", WIZ_PREFIX, IM},
    {"ticks", WIZ_TICKS, IM},
    {"logins", WIZ_LOGINS, IM},
    {"sites", WIZ_SITES, CODER},
    {"links", WIZ_LINKS, IM},
    {"newbies", WIZ_NEWBIE, IM},
    {"spam", WIZ_SPAM, IM},
    {"deaths", WIZ_DEATHS, IM},
    {"resets", WIZ_RESETS, HEADBUILDER},
    {"memcheck", WIZ_MEMCHECK, CODER},
	{"mobdeaths", WIZ_MOBDEATHS, IM},
    {"flags", WIZ_FLAGS, IM},
    {"penalties", WIZ_PENALTIES, IM},
    {"saccing", WIZ_SACCING, IM},
    {"levels", WIZ_LEVELS, IM},
    {"load", WIZ_LOAD, HEADBUILDER},
    {"restore", WIZ_RESTORE, HEADBUILDER},
    {"snoops", WIZ_SNOOPS, HEADCODER},
    {"switches", WIZ_SWITCHES, HEADCODER},
    {"secure", WIZ_SECURE, HEADCODER},
    {NULL, 0, 0}
};

/* attack table  -- not very organized :( */
const struct attack_type attack_table[MAX_DAMAGE_MESSAGE] = {
    {"none",      "hit",           -1},          /*  0 */
    {"slice",     "slice",         DAM_SLASH},
    {"stab",      "stab",          DAM_PIERCE},
    {"slash",     "slash",         DAM_SLASH},
    {"whip",      "whip",          DAM_SLASH},
    {"claw",      "claw",          DAM_SLASH},   /*  5 */
    {"blast",     "blast",         DAM_BASH},
    {"pound",     "pound",         DAM_BASH},
    {"crush",     "crush",         DAM_BASH},
    {"grep",      "grep",          DAM_SLASH},
    {"bite",      "bite",          DAM_PIERCE},  /* 10 */
    {"pierce",    "pierce",        DAM_PIERCE},
    {"suction",   "suction",       DAM_BASH},
    {"beating",   "beating",       DAM_BASH},
    {"digestion", "digestion",     DAM_ACID},
    {"charge",    "charge",        DAM_BASH},    /* 15 */
    {"slap",      "slap",          DAM_BASH},
    {"punch",     "punch",         DAM_BASH},
    {"wrath",     "wrath",         DAM_ENERGY},
    {"magic",     "magic",         DAM_ENERGY},
    {"divine",    "divine power",  DAM_HOLY},    /* 20 */
    {"cleave",    "cleave",        DAM_SLASH},
    {"scratch",   "scratch",       DAM_PIERCE},
    {"peck",      "peck",          DAM_PIERCE},
    {"peckb",     "peck",          DAM_BASH},
    {"chop",      "chop",          DAM_SLASH},   /* 25 */
    {"sting",     "sting",         DAM_PIERCE},
    {"smash",     "smash",         DAM_BASH},
    {"shbite",    "shocking bite", DAM_LIGHTNING},
    {"flbite",    "flaming bite",  DAM_FIRE},
    {"frbite",    "freezing bite", DAM_COLD},    /* 30 */
    {"acbite",    "acidic bite",   DAM_ACID},
    {"chomp",     "chomp",         DAM_PIERCE},
    {"drain",     "life drain",    DAM_NEGATIVE},
    {"thrust",    "thrust",        DAM_PIERCE},
    {"slime",     "slime",         DAM_ACID},
    {"shock",     "shock",         DAM_LIGHTNING},
    {"thwack",    "thwack",        DAM_BASH},
    {"flame",     "flame",         DAM_FIRE},
    {"chill",     "chill",         DAM_COLD},
    {NULL,        NULL,            0}
};

/* race table */
const struct race_type race_table[] = {
/*
    {
    name,        pc_race?,
    act bits,    aff_by bits,    off bits,
    imm,        res,        vuln,
    form,        parts 
    },
*/
//    {"unique", FALSE, 0, 0, 0, 0, 0, 0, 0, 0, FALSE},

    {
     "human", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},

    {
     "saiya-jin", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},
     
    {
     "namek", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},
    
    {
     "half-breed", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},

    {
     "icer", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},
     
    {
     "android", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},

    {
     "bio-android", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},

    {
     "alien", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, TRUE},

    /*
    {
     "majin", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, FALSE},
     */
               
    {
     "elf", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_CHARM, VULN_IRON,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, FALSE},

    {
     "dwarf", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_POISON | RES_DISEASE, VULN_DROWNING,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, FALSE},

    {
     "giant", FALSE,
     0, 0, 0,
     0, RES_FIRE | RES_COLD, VULN_MENTAL | VULN_LIGHTNING,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, FALSE},

    {
     "bat", FALSE,
     0, AFF_FLYING | AFF_DARK_VISION, OFF_DODGE | OFF_FAST,
     0, 0, VULN_LIGHT,
     A | G | V, A | C | D | E | F | H | J | K | P, FALSE},

    {
     "bear", FALSE,
     0, 0, OFF_CRUSH | OFF_DISARM | OFF_BERSERK,
     0, RES_BASH | RES_COLD, 0,
     A | G | V, A | B | C | D | E | F | H | J | K | U | V, FALSE},

    {
     "cat", FALSE,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | Q | U | V, FALSE},

    {
     "centipede", FALSE,
     0, AFF_DARK_VISION, 0,
     0, RES_PIERCE | RES_COLD, VULN_BASH,
     A | B | G | O, A | C | K, FALSE},

    {
     "dog", FALSE,
     0, 0, OFF_FAST,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | U | V, FALSE},

    {
     "doll", FALSE,
     0, 0, 0,
     IMM_COLD | IMM_POISON | IMM_HOLY | IMM_NEGATIVE | IMM_MENTAL |
     IMM_DISEASE | IMM_DROWNING, RES_BASH | RES_LIGHT,
     VULN_SLASH | VULN_FIRE | VULN_ACID | VULN_LIGHTNING | VULN_ENERGY,
     E | J | M | cc, A | B | C | G | H | K, FALSE},

    {"dragon", FALSE,
     0, AFF_INFRARED | AFF_FLYING, 0,
     0, RES_FIRE | RES_BASH | RES_CHARM,
     VULN_PIERCE | VULN_COLD,
     A | H | Z, A | C | D | E | F | G | H | I | J | K | P | Q | U | V | X, FALSE},

    {
     "fido", FALSE,
     0, 0, OFF_DODGE | ASSIST_RACE,
     0, 0, VULN_MAGIC,
     A | B | G | V, A | C | D | E | F | H | J | K | Q | V, FALSE},

    {
     "fox", FALSE,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | Q | V, FALSE},

    {
     "goblin", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE, VULN_MAGIC,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, FALSE},

    {
     "hobgoblin", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE | RES_POISON, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Y, FALSE},

    {
     "kobold", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_POISON, VULN_MAGIC,
     A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Q, FALSE},

    {
     "lizard", FALSE,
     0, 0, 0,
     0, RES_POISON, VULN_COLD,
     A | G | X | cc, A | C | D | E | F | H | K | Q | V, FALSE},

    {
     "modron", FALSE,
     0, AFF_INFRARED, ASSIST_RACE | ASSIST_ALIGN,
     IMM_CHARM | IMM_DISEASE | IMM_MENTAL | IMM_HOLY | IMM_NEGATIVE,
     RES_FIRE | RES_COLD | RES_ACID, 0,
     H, A | B | C | G | H | J | K, FALSE},

    {
     "orc", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE, VULN_LIGHT,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K, FALSE},

    {
     "pig", FALSE,
     0, 0, 0,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K, FALSE},

    {
     "rabbit", FALSE,
     0, 0, OFF_DODGE | OFF_FAST,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K, FALSE},

    {
     "school monster", FALSE,
     ACT_NOALIGN, 0, 0,
     IMM_CHARM | IMM_SUMMON, 0, VULN_MAGIC,
     A | M | V, A | B | C | D | E | F | H | J | K | Q | U, FALSE},

    {
     "snake", FALSE,
     0, 0, 0,
     0, RES_POISON, VULN_COLD,
     A | G | X | Y | cc, A | D | E | F | K | L | Q | V | X, FALSE},

    {
     "song bird", FALSE,
     0, AFF_FLYING, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | W, A | C | D | E | F | H | K | P, FALSE},

    {
     "troll", FALSE,
     0, AFF_REGENERATION | AFF_INFRARED | AFF_DETECT_HIDDEN,
     OFF_BERSERK,
     0, RES_CHARM | RES_BASH, VULN_FIRE | VULN_ACID,
     A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | U | V, FALSE},

    {
     "water fowl", FALSE,
     0, AFF_SWIM | AFF_FLYING, 0,
     0, RES_DROWNING, 0,
     A | G | W, A | C | D | E | F | H | K | P, FALSE},

    {
     "wolf", FALSE,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | J | K | Q | V, FALSE},

    {
     "wyvern", FALSE,
     0, AFF_FLYING | AFF_DETECT_INVIS | AFF_DETECT_HIDDEN,
     OFF_BASH | OFF_FAST | OFF_DODGE,
     IMM_POISON, 0, VULN_LIGHT,
     A | B | G | Z, A | C | D | E | F | H | J | K | Q | V | X, FALSE},

    {
     "unique", FALSE,
     0, 0, 0,
     0, 0, 0,
     0, 0, FALSE},


    {
     NULL, 0, 0, 0, 0, 0, 0, FALSE}
};

const struct pc_race_type pc_race_table[] = {
/*
    {"null race", "",
     {""}, {1, 1, 1, 1, 1}, {18, 18, 18, 18, 18}, {10,10,10,10,10}, 0},


    {
    "race name",     who name,
    { bonus skills },
    { base stats },        { max stats },  { stat learn bonus, add to 80 },      size
    },
*/
    {
     "human", "Human",
     {""},
     {1, 1, 1, 1, 1}, {180, 180, 180, 180, 180}, {5,5,5,5,5}, SIZE_MEDIUM,
     "The inhabitants of Earth. Decent at anything."
    },

    {
     "saiya-jin", "Saiya-jin",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {8,2,2,8,5}, SIZE_MEDIUM,
     "Born fighters, they are strong, but not very intelligent."
    },

    {
     "namek", "Namek",
     {"fast healing", ""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {6,4,6,4,5}, SIZE_MEDIUM,
     "The green-skinned Nameks possess a strong constitution, and the ability to heal quickly. They are sexless."
    },

    {
     "half-breed", "Half-breed",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {7,3,3,7,5}, SIZE_MEDIUM,
     "The offspring of a human and saiya-jin is a half-breed. They are not as strong as saiya-jins,  being more average like humans."
    },

    {
     "icer", "Icer",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {7,5,5,3,5}, SIZE_MEDIUM,
     "A terrible, evil race. Strong, but also not dexterous."
    },

    {
     "android", "Android",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {4,6,6,4,5}, SIZE_MEDIUM,
     "Robots, possibly built from a person. Though lacking intelligence, they have high strength and constitution."
    },

    {
     "bio-android", "Bio-Android",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {3,7,7,3,5}, SIZE_MEDIUM,
     "Similar to robots, but built from the cells of many races. Still lacking intelligence, they are stronger than androids."
    },

    {
     "alien", "Alien",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {5,5,5,5,5}, SIZE_MEDIUM,
     "Aliens or mutants are the generic species scattered across the universe. They can do anything at equal competance."
    },

    /*
    {
     "majin", "Majin",
     {""},
     {1, 1, 1, 1, 1}, {160, 200, 180, 210, 150}, {10,10,10,10,10}, SIZE_MEDIUM,
     "filler"
    }
    */
};


char *const stat_table[MAX_STATS] = {
    "strength",
    "intelligence",
    "will power",
    "dexterity",
    "charisma"
};

/*
 * Titles.
 */
char *const title_table[MAX_LEVEL + 1][2] = {
     {"Man", "Woman"},
     {"Player", "Player"},
     {"Hero", "Hero"},
     {"Builder", "Builder"},
	 {"Enforcer", "Enforcer"},
     {"Head Builder", "Head Builder"},
     {"Coder", "Coder"},
	 {"Head Coder", "Head Coder"},
     {"Implementor", "Implementor"}
};

// Names of the transformations
char *const trans_table[MAX_TRANS] = {
     "not transformed",
     "Super Saiya-Jin Level 1",
     "Super Saiya-Jin Level 2",
     "Super Saiya-Jin Level 3",
     "Super Saiya-Jin Level 4",
     "Super Saiya-Jin Level 5",
     "Mystic",
     "Super Human",
     "Hyper Namek",
     "Super Namek",
     "Android Level 1",
     "Android Level 2",
     "Android Level 3",
     "Self-Fused Android",
     "Icer Form 2",
     "Icer Form 3",
     "Icer Form 4",
     "Icer Form 5",
     "Imperfect Bio-Android",
     "Semi-Perfect Bio-Android",
     "Perfect Bio-Android",
     "Ultra-Perfect Bio-Android",
     "Kaioken"
};

// Stances
char *const stance_table[] = {
    "normal",
    "offensive",
    "defensive",
    "kamikaze",
    NULL
};


/*
 * Liquid properties.
 * Used in world.obj.
 */
/* Liquid Affects:
#define COND_DRUNK   0
#define COND_FULL    1
#define COND_THIRST  2
#define COND_HUNGER  3
*/
const struct liq_type liq_table[] = {
/*                                         (drunk)                  (amount      */
/*                                                               drank per use)  */
/*    name			     colour             proof,full,thirst,hunger,ssize, regen*/
    {"water",			"clear",			{  0,	1,	 10,    0,    16,     0}},
    {"beer",			"amber",			{ 12,	1,	  8,    1,    12,     0}},
    {"red wine",		"burgundy",			{ 30,	1,	  8,    1,     5,     0}},
    {"ale",	           	"brown",			{ 15,	1, 	  8,    1,    12,     0}},
    {"dark ale",		"dark",				{ 16,	1, 	  8,    1,    12,     0}},

    {"whisky",			"golden",			{120,	1,	  5,    0,     2,     0}},
    {"lemonade",		"pink",				{  0,	1,	  9,    2,    12,     0}},
    {"firebreather",	"boiling",			{190,	0,	  4,    0,     2,     0}},
    {"local specialty",	"clear",			{151,	1,	  3,    0,     2,     0}},
    {"slime mold juice","green",			{  0,	2,	 -8,    1,     2,     0}},

    {"milk",			"white",			{  0,	2,	  9,    3,    12,     0}},
    {"tea",	            "tan",				{  0,	1,	  8,    0,     6,     0}},
    {"coffee",			"black",			{  0,	1,	  8,    0,     6,     0}},
    {"blood",			"red",				{  0,	2,	 -1,    2,     6,     0}},
    {"salt water",		"clear",			{  0,	1,	 -2,    0,     1,     0}},

    {"coke",			"brown",			{  0,	2,	  9,    2,    12,     0}},
    {"root beer",		"brown",			{  0,	2,	  9,    2,    12,     0}},
    {"elvish wine",		"green",			{ 35,	2,	  8,    1,     5,     0}},
    {"white wine",		"golden",			{ 28,	1,    8,    1,     5,     0}},
    {"champagne",		"golden",			{ 32,	1,	  8,    1,     5,     0}},

    {"mead",			"honey-colored",	{ 34,	2,	  8,    2,    12,     0}},
    {"rose wine",		"pink",				{ 26,	1,	  8,    1,     5,     0}},
    {"benedictine wine","burgundy",			{ 40,	1,	  8,    1,     5,     0}},
    {"vodka",			"clear",			{130,	1,	  5,    0,     2,     0}},
    {"cranberry juice", "red",				{  0,	1,	  9,    2,    12,     0}},

    {"orange juice",	"orange",			{  0,	2,	  9,    3,    12,     0}},
    {"absinthe",		"green",			{200,	1,	  4,    0,     2,     0}},
    {"brandy",			"golden",			{ 80,	1,	  5,    0,     4,     0}},
    {"aquavit",			"clear",			{140,	1,	  5,    0,     2,     0}},
    {"schnapps",		"clear",			{ 90,	1,	  5,    0,     2,     0}},

    {"icewine",			"purple",			{ 50,	2,	  6,    1,     5,     0}},
    {"amontillado",		"burgundy",			{ 35,	2,	  8,    1,     5,     0}},
    {"sherry",			"red",				{ 38,	2,	  7,    1,     5,     0}},
    {"framboise",		"red",				{ 50,	1,	  7,    1,     5,     0}},
    {"rum",			    "amber",			{151,	1,	  4,    0,     2,     0}},

    {"cordial",			"clear",			{100,	1,	  5,    0,     2,     0}},
    {NULL,			    NULL,				{  0,	0,	  0,    0,     0,     0}}
};



/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
\ */
#define SLOT(n)    n

struct skill_type skill_table[MAX_SKILL] = {

/* combat and weapons skills */

/*struct:
 *	{	"name", &gsn_, skill_fun, command?,
 *		can learn?, can improve?, {"skillpre"}, {skill values}, {str,int,wis,dex,con}, pl_pre, {"race"},
 *		TAR_, POS_, ki, type (SKILL_IMM, SKILL_DELAY, SKILL_CHARGE), wait,
 *		"damage", "off", "obj off", "immediate 1", "imm 2", "delay 1", "delay 2" },
 */
/*
	{	"", &gsn_, NULL, FALSE,
		TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 15, 0, 0,
		"", "!error!", "", "", "",   },
 */
	{	"hand to hand", "", &gsn_hand_to_hand, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"axe", "", &gsn_axe, NULL, FALSE,
		TRUE, TRUE, {"sword"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 10, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""   },

	{	"dagger", "", &gsn_dagger, NULL, FALSE,
		TRUE, TRUE, {"hand to hand"}, {30}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 10, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""   },

	{	"sword", "", &gsn_sword, NULL, FALSE,
		TRUE, TRUE, {"dagger"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""   },

	{	"flail", "", &gsn_flail, NULL, FALSE,
		TRUE, TRUE, {"mace"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"mace", "", &gsn_mace, NULL, FALSE,
		TRUE, TRUE, {"dagger"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"polearm", "", &gsn_polearm, NULL, FALSE,
		TRUE, TRUE, {"spear"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"spear", "", &gsn_spear, NULL, FALSE,
		TRUE, TRUE, {"dagger"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"whip", "", &gsn_whip, NULL, FALSE,
		TRUE, TRUE, {"flail"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"exotic", "", &gsn_exotic, NULL, FALSE,
		TRUE, TRUE, {"whip"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""   },

	{	"shield block", "", &gsn_shield_block, NULL, FALSE,
		TRUE, TRUE, {"parry"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 3, SKILL_NONE, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"dodge", "", &gsn_dodge, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 3, SKILL_NONE, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"parry", "", &gsn_parry, NULL, FALSE,
		TRUE, TRUE, {"dagger"}, {5}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 3, SKILL_NONE, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"fast healing", "", &gsn_fast_healing, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {35,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_SLEEPING, 1, SKILL_NONE, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

// Not updated:

	{	"meditation", "", &gsn_meditation, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_SLEEPING, 1, SKILL_NONE, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"berserk", "", &gsn_berserk, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 5, SKILL_IMM, 5*PULSE_SECOND,
		"", "You feel your pulse slow down.", "", "", "", "", ""  },

	{	"disarm", "", &gsn_disarm, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 5, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"rescue", "", &gsn_rescue, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },
/* non-combat skills */

	{	"haggle", "", &gsn_haggle, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_RESTING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"hide", "", &gsn_hide, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_RESTING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"lore", "", &gsn_lore, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"peek", "", &gsn_peek, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_FIGHTING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"pick lock", "", &gsn_pick_lock, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"sneak", "", &gsn_sneak, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "You no longer feel stealthy.", "", "", "", "", ""  },

	{	"steal", "", &gsn_steal, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"scrolls", "", &gsn_scrolls, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"staves", "", &gsn_staves, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },


	{	"wands", "", &gsn_wands, NULL, FALSE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },


	{	"recall", "", &gsn_recall, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

//other new ones:
	{	"perception", "", &gsn_perception, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"teaching", "", &gsn_teaching, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

	{	"defend", "", &gsn_defend, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"kamikaze", "", &gsn_kamikaze, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 100000, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"fusion", "", &gsn_fusion, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,50,50,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"kaioken", "", &gsn_kaioken, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {0,50,50,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

    {	"fly", "", &gsn_fly, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {0,30,0,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", "" },

    {	"sense", "", &gsn_sense, NULL, FALSE,
		TRUE, TRUE, {}, {0}, {0,0,30,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", "" },

    {	"suppress", "", &gsn_suppress, NULL, FALSE,
		TRUE, TRUE, {"sense"}, {25}, {0,0,30,0,0}, 0, {},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 5*PULSE_SECOND,
		"", "!error!", "", "", "", "", "" },

// Transformations:
    {	"super saiyan 1", "", &gsn_ssj1, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"saiya-jin", "half-breed"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"super saiyan 2", "", &gsn_ssj2, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"saiya-jin", "half-breed"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"super saiyan 3", "", &gsn_ssj3, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"saiya-jin"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"super saiyan 4", "", &gsn_ssj4, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"saiya-jin"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"super saiyan 5", "", &gsn_ssj5, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"saiya-jin"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"mystic", "", &gsn_mystic, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 10000, {"half-breed", "human"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"super human", "", &gsn_superh, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 2000, {"human"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"first form", "", &gsn_form1, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 1000, {"icer"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"second form", "", &gsn_form2, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 2500, {"icer"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"third form", "", &gsn_form3, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 5000, {"icer"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"fourth form", "", &gsn_form4, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 7500, {"icer"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"fifth form", "", &gsn_form5, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 10000, {"icer"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"hyper namek", "", &gsn_hypern, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 3000, {"namek"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"super namek", "", &gsn_supern, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 7500, {"namek"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"imperfect", "", &gsn_imperfect, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"bio-android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"semi-perfect", "", &gsn_semiperfect, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"bio-android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"perfect", "", &gsn_perfect, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"bio-android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"ultra perfect", "", &gsn_ultraperfect, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"bio-android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"upgrade 1", "", &gsn_upgrade1, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },
        
    {	"upgrade 2", "", &gsn_upgrade2, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },
        
    {	"upgrade 3", "", &gsn_upgrade3, NULL, FALSE,
		FALSE, FALSE, {}, {0}, {0,0,0,0,0}, 0, {"android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

    {	"self-fuse", "", &gsn_selffuse, NULL, FALSE,
		TRUE, FALSE, {}, {0}, {0,0,0,0,0}, 15000, {"android"},
		TAR_IGNORE, POS_STANDING, 1, SKILL_IMM, 0,
		"", "!error!", "", "", "", "", ""  },

// Updated:
	{	"bash", "", &gsn_bash, skill_bash, TRUE,
        TRUE, TRUE, {"kick"}, {35}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 24, SKILL_IMM, 5*PULSE_SECOND,
		"bash", "!error!", "", "", "", "", ""  },

	{	"kick", "", &gsn_kick, skill_kick, TRUE,
		TRUE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 15, SKILL_IMM, 5*PULSE_SECOND,
		"kick", "!error!", "", "", "", "", ""  },

	{	"sweep", "", &gsn_sweep, skill_sweep, TRUE,
		TRUE, TRUE, {"kick"}, {25}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 12, SKILL_IMM, 3*PULSE_SECOND,
		"sweep", "!error!", "", "", "", "", ""  },

	{	"throat shot", "", &gsn_throat_shot, skill_throat_shot, TRUE,
		TRUE, TRUE, {"hand to hand"}, {10}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 12, SKILL_IMM, 2*PULSE_SECOND,
		"throat shot", "!error!", "", "", "", "", ""  },

	{	"knee", "", &gsn_knee, skill_knee, TRUE,
		TRUE, TRUE, {"kick"}, {15}, {0,0,0,20,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 12, SKILL_IMM, 5*PULSE_SECOND,
		"knee", "!error!", "", "", "", "", ""  },

	{	"elbow", "", &gsn_elbow, skill_elbow, TRUE,
		TRUE, TRUE, {"hand to hand"}, {25}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 9, SKILL_IMM, 2*PULSE_SECOND,
		"elbow", "!error!", "", "", "", "", ""  },

	{	"eye gouge", "", &gsn_eye_gouge, skill_eye_gouge, TRUE,
		TRUE, TRUE, {"throat shot"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 10, SKILL_IMM, 5*PULSE_SECOND,
		"eye gouge", "Your eyes no longer hurt, and you can see!", "", "", "", "", ""  },

	{	"heart shot", "", &gsn_heart_shot, skill_heart_shot, TRUE,
		TRUE, TRUE, {"eye gouge"}, {25}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 24, SKILL_IMM, 5*PULSE_SECOND,
		"heart shot", "!error!", "", "", "", "", ""  },

	{	"hyperpunch", "", &gsn_hyperpunch, skill_hyperpunch, TRUE,
		TRUE, TRUE, {"hand to hand"}, {60}, {0,0,0,0,0}, 0, {},
		TAR_AREA_OFF, POS_FIGHTING, 24, SKILL_IMM, 10*PULSE_SECOND,
		"", "!error!", "", "", "", "", ""  },

/* DBZ abilites */

// HACK:
// Kamehameha must be the first ki skill in the table, and all the ki skills
// must come at the end of the table! Search for FIRST_KI_SN for references.
// Basically, FIRST_KI_SN lets us determine which skills
// are ki skills or not (every sn >= FIRST_KI_SN is a ki skill) (FIRST_KI_SN = gsn_kamehameha)
	{	"kamehameha", "", &gsn_kamehameha, skill_kamehameha, TRUE,
		TRUE, TRUE, {"energy beam"}, {30}, {0,0,0,0,0}, 0, {},
		TAR_HYBRID100, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"blast", "!error!", "", "You place your palms together and begin to create a ball of energy.", "$n places $s palms together and begins to create a ball of energy.", "You release a massive energy beam!", "$n releases a massive energy beam!" },

    {   "focus",       "", &gsn_focus, skill_focus, TRUE,
        TRUE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
        TAR_CHAR_OFFENSIVE, POS_FIGHTING, 12, SKILL_DELAY, 4*PULSE_SECOND,
        "focused ki", "!error!", "", "You focus an amount of ki in a clenched fist.",  "$n focuses an amount of ki in a clenched fist.", "You open your fist and let your ki escape!", "$n opens $s fist and lets $s ki escape!" },

    {	"energy beam", "beam", &gsn_energy_beam, skill_energy_beam, TRUE,
		TRUE, TRUE, {"focus"}, {10}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 6*PULSE_SECOND,
		"energy beam", "!error!", "", "A point of light begins to grow between your palms.", "A point of light begins grow between $n's palms.", "You release a beam of energy!", "$n releases a beam of energy!" },

	{	"energy slash", "energyslash", &gsn_energy_slash, skill_energy_slash, TRUE,
		TRUE, TRUE, {"energy beam", "sword"}, {20, 10}, {0,10,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 6*PULSE_SECOND,
		"energy slash", "!error!", "", "You begin to charge your weapon.", "$n begins to charge $s weapon.", "You unleash your weapon!", "$n unleashes $s weapon!" },

	{	"final flash", "", &gsn_finalflash, skill_finalflash, TRUE,
		TRUE, TRUE, {"kamehameha"}, {25}, {0,0,0,0,0}, 0, {},
        TAR_HYBRID100, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"final flash", "!error!", "", "You stretch your arms out and build up energy in either palm.", "$n stretches $s arms out and builds up energy in either palm.", "With an amazing flash, you throw your arms forward and release!", "With an amazing flash, $n throws $s arms forward and releases!" },

	{	"eye beam", "eyebeam", &gsn_eyebeam, skill_eyebeam, TRUE,
		TRUE, TRUE, {"energy beam"}, {10}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"eye beam", "!error!", "", "You stare at your target.", "$n begins to stare at $s target.", "A beam erupts from your eyes!", "A beam erupts from $n's eyes!" },

	{	"mouth beam", "", &gsn_mouthbeam, skill_mouthbeam, TRUE,
		TRUE, TRUE, {"eye beam"}, {15}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"mouth beam", "!error!", "", "Your mouth slowly opens.", "$n's mouth slowly opens.", "A beam erupts from your mouth!", "A beam erupts from $n's mouth!"},

	{	"finger beam", "", &gsn_fingerbeam, skill_fingerbeam, TRUE,
		TRUE, TRUE, {"mouth beam"}, {25}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 20*PULSE_SECOND,
		"finger beam", "!error!", "", "You point at your target.", "$n points at $s target.", "A beam erupts from your finger!", "A beam erupts from $n's finger!" },

	{	"special beam cannon", "sbc", &gsn_specialbeam, skill_specialbeam, TRUE,
		TRUE, TRUE, {"energy beam"}, {15}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"special beam", "!error!", "", "You concentrate and point your fingers at your target.", "$n concentrates and points $s fingers at $s target.", "A yellow beam, rotating around a thin ray, ignites from your fingers!", "A green beam, rotating around a thin ray, ignites from $n's fingers!" },

	{	"masenko", "", &gsn_masenko, skill_masenko, TRUE,
		TRUE, TRUE, {"speical beam cannon"}, {30}, {0,0,0,0,0}, 0, {},
		TAR_HYBRID100, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"energy ball", "!error!", "", "With your hands behind your head, you begin to weave a ball of energy into existence.", "With $s hands behind $s head, $n begins to weave a ball of energy into existence.", "You throw your ball of energy forward, sending it exploding in a giant splash!", "$n throws $s ball of energy forward, sending it exploding in a giant splash!" },

	{	"energy ball", "ball", &gsn_energy_ball, skill_energy_ball, TRUE,
		TRUE, TRUE, {"focus"}, {25}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 6*PULSE_SECOND,
		"energy ball", "!error!", "", "Placing your palms together, you begin to generate a small ball of energy.", "Placing $s palms together, $n begin to generate a small ball of energy.", "You throw a small ball of energy!", "$n throws a small ball of energy!" },

	{	"solar flare", "", &gsn_solarflare, skill_solarflare, TRUE,
		TRUE, TRUE, {"energy ball", "energy beam"}, {10,10}, {0,0,0,0,0}, 0, {},
		TAR_AREA_OFF, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"flash", "The blinding light resides and you can once again see.", "", "You create a bright light.", "$n creates a bright light.", "A blinding light erupts from you!", "A blinding light erupts from $n!" },

	{	"spirit bomb", "", &gsn_spirit_bomb, skill_spirit_bomb, TRUE,
		TRUE, TRUE, {"energy ball"}, {20}, {0,0,0,0,0}, 0, {},
		TAR_AREA_OFF, POS_FIGHTING, 1, SKILL_CHARGE, 120*PULSE_SECOND,
		"spirit bomb", "!error!", "", "You begin to charge an enormous ball of energy.", "$n begins to charge an enormous ball of energy.", "You release a massive ball of energy!", "$n relases a massive ball of energy!" },

	{	"death ball", "", &gsn_death_ball, skill_death_ball, TRUE,
		TRUE, TRUE, {"finger beam", "spirit bomb"}, {5,10}, {0,0,0,0,0}, 0, {},
		TAR_AREA_OFF, POS_FIGHTING, 1, SKILL_CHARGE, 120*PULSE_SECOND,
		"death ball", "!error!", "", "You point your finger out, creating a swirling red ball of energy at the tip.", "$n points $s finger out, creating a swirling red ball of energy at the tip.", "With a flick of the finger, you throw your planet-sized ball of coalescing, red energy!", "With a flick of the finger, $n throws $s planet-sized ball of coalescing, red energy!" },

	{	"power bomb", "powerbomb", &gsn_power_bomb, skill_power_bomb, TRUE,
		TRUE, TRUE, {"death ball"}, {25}, {0,0,0,0,0}, 0, {},
		TAR_AREA_OFF, POS_FIGHTING, 2, SKILL_CHARGE, 40*PULSE_SECOND,
		"power bomb", "!error!", "", "You begin to charge an enormous ball of energy.", "$n begins to charge an enormous ball of energy.", "You release a massive ball of energy!", "$n relases a massive ball of energy!" },

	{	"scattershot", "", &gsn_scattershot, skill_scattershot, TRUE,
		TRUE, TRUE, {"energy ball"}, {10}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 4, SKILL_DELAY, 8*PULSE_SECOND,
		"ball", "!error!", "", "You concentrate and point your fingers at your target.", "$n concentrates and points $s fingers at $s target.", "You release a stream of energy balls!", "$n releases a stream of energy balls!" },

	{	"destructo disk", "", &gsn_destructo_disk, skill_destructo_disk, TRUE,
		TRUE, TRUE, {"scattershot"}, {10}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_OFFENSIVE, POS_FIGHTING, 3, SKILL_CHARGE, 10*PULSE_SECOND,
		"destructo disk", "!error!", "", "Placing your hands at your sides, energy begins to build in your palms.", "Placing $s hands at $s sides, energy begins to build in $n's palms.", "You launch your destructo disk spinning towards your foe!", "$n launches $s destructo disk spinning towards $s foe!" },

	{	"timestop", "", &gsn_timestop, skill_timestop, TRUE,
		FALSE, TRUE, {}, {0}, {0,0,0,0,0}, 0, {},
		TAR_AREA_OFF, POS_FIGHTING, 2, SKILL_CHARGE, 20*PULSE_SECOND,
		"!error!", "!error!", "", "You concentrate.", "$n concentrates.", "You finish and time seems to  s l ow   d  o  w   n.", "$n finishes and time seems to  s l ow   d  o  w   n." },

	{	"heal", "", &gsn_heal, skill_heal, TRUE,
		FALSE, TRUE, {}, {}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_DEFENSIVE, POS_FIGHTING, 4, SKILL_CHARGE, 10*PULSE_SECOND,
		"!error!", "!error!", "", "You begin to utter some words.", "$n begins to utters some words.", "You finish your littany!", "$n finishes $s littany!" },

	{	"revive", "", &gsn_revive, skill_revive, TRUE,
		FALSE, TRUE, {}, {}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_DEFENSIVE, POS_FIGHTING, 4, SKILL_DELAY, 25*PULSE_SECOND,
		"!error!", "!error!", "", "You begin to shout some words.", "$n begins to shout some words.", "You finish your shout!", "$n finishes $s shout!" },

	{	"regeneration", "", &gsn_regen, skill_regen, TRUE,
		FALSE, TRUE, {}, {}, {0,0,0,0,0}, 0, {},
		TAR_CHAR_DEFENSIVE, POS_FIGHTING, 4, SKILL_DELAY, 40*PULSE_SECOND,
		"!error!", "Your stamina suddenly weakens.", "", "You begin to utter some words.", "$n begins to utters some words.", "You finish your littany!", "$n finishes $s littany!" }
};

