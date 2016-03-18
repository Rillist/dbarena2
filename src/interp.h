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
*	ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*	ROM has been brought to you by the ROM consortium                      *
*	    Russ Taylor (rtaylor@hypercube.org)                                *
*	    Gabrielle Taylor (gtaylor@hypercube.org)                           *
*	    Brian Moore (zump@rom.org)                                         *
*	By using this code, you have agreed to follow the terms of the         *
*	ROM license, in the file Rom24/doc/rom.license                         *
****************************************************************************/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

/* this is a listing of all the commands and command related data */

/* wrapper function for safe command execution */
void do_function args((CHAR_DATA *ch, DO_FUN *do_fun, char *argument));

/* for command types */
#define L8 IMPLEMENTOR 
#define L7 HEADCODER   
#define L6 CODER       
#define L5 HEADBUILDER 
#define L4 ENFORCER
#define L3 BUILDER     
#define IM LEVEL_IMMORTAL
#define HE LEVEL_HERO

#define COM_INGORE	1


/*
 * Structure for a command in the command lookup table.
 */
struct	cmd_type
{
    char * const	name;
    DO_FUN *		do_fun;
    sh_int		position;
    sh_int		level;
    sh_int		log;
    sh_int      show;
    bool        wait; // true if skill cannot be used when waiting (wait or skill_wait)
};

typedef struct CmdNode_ CmdNode;
typedef struct SocialNode_ SocialNode;
typedef struct SkillNode_ SkillNode;

struct CmdNode_ {
    int      nNum;
	CmdNode *pRight;
	CmdNode *pLeft;
};

struct SocialNode_ {
    int         nNum;
	SocialNode *pRight;
	SocialNode *pLeft;
};

struct SkillNode_ {
    int        nSn;
	char      *szName; // Necessary to deal with actual names and 'syntax'
	SkillNode *pRight;
	SkillNode *pLeft;
};

/* the command table itself */
extern	const	struct	cmd_type	cmd_table	[];

/*
 * Command functions.
 * Defined in act_*.c (mostly, others mainly in fight.c).
 */
// A
DECLARE_DO_FUN( do_accept       );
DECLARE_DO_FUN(	do_addki		);
DECLARE_DO_FUN(	do_advance		);
DECLARE_DO_FUN( do_aedit		);
DECLARE_DO_FUN( do_affects		);
DECLARE_DO_FUN( do_afk			);
DECLARE_DO_FUN( do_alia			);
DECLARE_DO_FUN( do_alias		);
DECLARE_DO_FUN( do_alist		);
DECLARE_DO_FUN(	do_allow		);
DECLARE_DO_FUN( do_answer		);
DECLARE_DO_FUN(	do_areas		);
DECLARE_DO_FUN( do_asave		);
DECLARE_DO_FUN(	do_at			);
DECLARE_DO_FUN( do_attack       );
DECLARE_DO_FUN(	do_auction		);
DECLARE_DO_FUN( do_autoall		);
DECLARE_DO_FUN( do_autoassist	);
DECLARE_DO_FUN( do_autoattack   );
DECLARE_DO_FUN( do_autoexit		);
DECLARE_DO_FUN( do_autozenni	);
DECLARE_DO_FUN( do_autolist		);
DECLARE_DO_FUN( do_autoloot		);
DECLARE_DO_FUN( do_autosac		);
DECLARE_DO_FUN( do_autosplit	);
DECLARE_DO_FUN( do_autoweather  );
// B
DECLARE_DO_FUN( do_backup       );
DECLARE_DO_FUN( do_balance      );
DECLARE_DO_FUN(	do_bamfin		);
DECLARE_DO_FUN(	do_bamfout		);
DECLARE_DO_FUN(	do_ban			);
DECLARE_DO_FUN( do_berserk		);
DECLARE_DO_FUN( do_board		);
DECLARE_DO_FUN(	do_brandish		);
DECLARE_DO_FUN( do_brief		);
DECLARE_DO_FUN(	do_bug			);
DECLARE_DO_FUN(	do_buy			);
// C
DECLARE_DO_FUN( do_cancel       );
DECLARE_DO_FUN(	do_cast			);
DECLARE_DO_FUN( do_channels		);
DECLARE_DO_FUN( do_clantalk		);
DECLARE_DO_FUN( do_cleader      );
DECLARE_DO_FUN( do_clone		);
DECLARE_DO_FUN(	do_close		);
DECLARE_DO_FUN(	do_colour		);
DECLARE_DO_FUN(	do_commands		);
DECLARE_DO_FUN( do_combatinfo   );
DECLARE_DO_FUN( do_combatstat   );
DECLARE_DO_FUN( do_combine		);
DECLARE_DO_FUN( do_compact		);
DECLARE_DO_FUN(	do_compare		);
DECLARE_DO_FUN(	do_consider		);
DECLARE_DO_FUN(	do_copyover		);
DECLARE_DO_FUN( do_count		);
DECLARE_DO_FUN( do_clanremove   );
DECLARE_DO_FUN(	do_credits		);
DECLARE_DO_FUN( do_customskill  );
// D
DECLARE_DO_FUN( do_deaf			);
DECLARE_DO_FUN( do_defend       );
DECLARE_DO_FUN( do_delet		);
DECLARE_DO_FUN( do_delete		);
DECLARE_DO_FUN(	do_deny			);
DECLARE_DO_FUN(	do_description	);
DECLARE_DO_FUN(	do_disarm		);
DECLARE_DO_FUN(	do_disconnect	);
DECLARE_DO_FUN(	do_down			);
DECLARE_DO_FUN(	do_drink		);
DECLARE_DO_FUN(	do_drop			);
DECLARE_DO_FUN( do_dump			);
// E
DECLARE_DO_FUN(	do_east			);
DECLARE_DO_FUN(	do_eat			);
DECLARE_DO_FUN(	do_echo			);
DECLARE_DO_FUN(	do_emote		);
DECLARE_DO_FUN( do_enter		);
DECLARE_DO_FUN(	do_equipment	);
DECLARE_DO_FUN(	do_examine		);
DECLARE_DO_FUN(	do_exits		);
// F
DECLARE_DO_FUN(	do_fill			);
DECLARE_DO_FUN(	do_finishingmove );
DECLARE_DO_FUN( do_flag			);
DECLARE_DO_FUN(	do_flee			);
DECLARE_DO_FUN( do_fly          );
DECLARE_DO_FUN(	do_follow		);
DECLARE_DO_FUN(	do_force		);
DECLARE_DO_FUN(	do_freeze		);
DECLARE_DO_FUN( do_fvlist       );
DECLARE_DO_FUN( do_fuse         );
// G
DECLARE_DO_FUN(	do_get			);
DECLARE_DO_FUN(	do_give			);
DECLARE_DO_FUN(	do_givecustom	);
DECLARE_DO_FUN( do_god          );
DECLARE_DO_FUN( do_gossip		);
DECLARE_DO_FUN(	do_goto			);
DECLARE_DO_FUN( do_grant        );
DECLARE_DO_FUN( do_grats		);
DECLARE_DO_FUN(	do_group		);
DECLARE_DO_FUN(	do_gtell		);
DECLARE_DO_FUN( do_guild	    );
// H
DECLARE_DO_FUN( do_hedit		);
DECLARE_DO_FUN(	do_help			);
DECLARE_DO_FUN(	do_hide			);
DECLARE_DO_FUN(	do_holylight	);
DECLARE_DO_FUN(	do_hyper        );
// I
DECLARE_DO_FUN(	do_immtalk		);
DECLARE_DO_FUN(	do_immtitle		);
DECLARE_DO_FUN(	do_invade		);
DECLARE_DO_FUN( do_incinerat    );
DECLARE_DO_FUN( do_incinerate   );
DECLARE_DO_FUN( do_incognito	);
DECLARE_DO_FUN( do_imotd		);
DECLARE_DO_FUN(	do_inventory	);
DECLARE_DO_FUN(	do_invis		);
// J
DECLARE_DO_FUN( do_join         );
// K
DECLARE_DO_FUN( do_kaioken      );
// L
DECLARE_DO_FUN(	do_list			);
DECLARE_DO_FUN(	do_listen		);
DECLARE_DO_FUN( do_listprog     );
DECLARE_DO_FUN( do_load			);
DECLARE_DO_FUN(	do_lock			);
DECLARE_DO_FUN(	do_log			);
DECLARE_DO_FUN( do_loner        );
DECLARE_DO_FUN(	do_look			);
// M
DECLARE_DO_FUN( do_medit		);
DECLARE_DO_FUN( do_meditate     );
DECLARE_DO_FUN(	do_memory		);
DECLARE_DO_FUN(	do_mfind		);
DECLARE_DO_FUN( do_mission      );
DECLARE_DO_FUN( do_mlist		);
DECLARE_DO_FUN(	do_mload		);
DECLARE_DO_FUN( do_mpedit		);
DECLARE_DO_FUN( do_mplist		);
DECLARE_DO_FUN(	do_mset			);
DECLARE_DO_FUN(	do_mstat		);
DECLARE_DO_FUN(	do_mwhere		);
DECLARE_DO_FUN( do_mob			);
DECLARE_DO_FUN( do_motd			);
DECLARE_DO_FUN( do_mpstat		);
DECLARE_DO_FUN( do_mpdump		);
DECLARE_DO_FUN( do_multilink    );
DECLARE_DO_FUN( do_music		);
DECLARE_DO_FUN( do_mystic       );
// N
DECLARE_DO_FUN( do_newlock		);
DECLARE_DO_FUN( do_nochannels	);
DECLARE_DO_FUN(	do_noemote		);
DECLARE_DO_FUN( do_nofollow		);
DECLARE_DO_FUN( do_noloot		);
DECLARE_DO_FUN(	do_north		);
DECLARE_DO_FUN(	do_noshout		);
DECLARE_DO_FUN( do_nosummon		);
DECLARE_DO_FUN(	do_note 		);
DECLARE_DO_FUN(	do_notell		);
// O
DECLARE_DO_FUN( do_oedit		);
DECLARE_DO_FUN(	do_ofind		);
DECLARE_DO_FUN( do_olc			);
DECLARE_DO_FUN( do_olist		);
DECLARE_DO_FUN(	do_oload		);
DECLARE_DO_FUN( do_ooc          );
DECLARE_DO_FUN( do_opdump       );
DECLARE_DO_FUN( do_opedit       );
DECLARE_DO_FUN(	do_open			);
DECLARE_DO_FUN( do_oplist		);
DECLARE_DO_FUN( do_opstat       );
DECLARE_DO_FUN(	do_order		);
DECLARE_DO_FUN(	do_oset			);
DECLARE_DO_FUN(	do_ostat		);
DECLARE_DO_FUN( do_owhere		);
// P
DECLARE_DO_FUN(	do_pardon		);
DECLARE_DO_FUN(	do_password		);
DECLARE_DO_FUN(	do_peace		);
DECLARE_DO_FUN( do_pecho		);
DECLARE_DO_FUN( do_permban		);
DECLARE_DO_FUN(	do_pick			);
DECLARE_DO_FUN( do_play			);
DECLARE_DO_FUN( do_pmote		);
DECLARE_DO_FUN( do_pose         );
DECLARE_DO_FUN( do_pour			);
DECLARE_DO_FUN( do_power        );
DECLARE_DO_FUN( do_powerstruggle);
DECLARE_DO_FUN( do_prefi		);
DECLARE_DO_FUN( do_prefix		);
DECLARE_DO_FUN( do_prompt		);
DECLARE_DO_FUN( do_protect		);
DECLARE_DO_FUN(	do_purge		);
DECLARE_DO_FUN( do_pushup       );
DECLARE_DO_FUN(	do_put			);
// Q
DECLARE_DO_FUN(	do_qmconfig		);
DECLARE_DO_FUN( do_qmread		);
DECLARE_DO_FUN(	do_quaff		);
DECLARE_DO_FUN( do_quest		);
DECLARE_DO_FUN( do_question		);
DECLARE_DO_FUN(	do_qui			);
DECLARE_DO_FUN( do_quiet		);
DECLARE_DO_FUN(	do_quit			);
DECLARE_DO_FUN( do_quote		);
// R
DECLARE_DO_FUN( do_randobj      );
DECLARE_DO_FUN( do_read			);
DECLARE_DO_FUN(	do_reboo		);
DECLARE_DO_FUN(	do_reboot		);
DECLARE_DO_FUN(	do_recall		);
DECLARE_DO_FUN(	do_recho		);
DECLARE_DO_FUN(	do_recite		);
DECLARE_DO_FUN( do_redit		);
DECLARE_DO_FUN( do_release      );
DECLARE_DO_FUN(	do_remove		);
DECLARE_DO_FUN( do_rename       );
DECLARE_DO_FUN( do_replay		);
DECLARE_DO_FUN(	do_reply		);
DECLARE_DO_FUN(	do_report		);
DECLARE_DO_FUN(	do_rescue		);
DECLARE_DO_FUN( do_resets		);
DECLARE_DO_FUN(	do_rest			);
DECLARE_DO_FUN(	do_restore		);
DECLARE_DO_FUN( do_retreat      );
DECLARE_DO_FUN(	do_return		);
DECLARE_DO_FUN( do_reveal       );
DECLARE_DO_FUN( do_revert       );
DECLARE_DO_FUN( do_reward       );
DECLARE_DO_FUN( do_rlist		);
DECLARE_DO_FUN( do_rpedit       );
DECLARE_DO_FUN( do_rpdump       );
DECLARE_DO_FUN( do_rplist		);
DECLARE_DO_FUN( do_rpstat       );
DECLARE_DO_FUN(	do_rset			);
DECLARE_DO_FUN(	do_rstat		);
DECLARE_DO_FUN( do_rules		);
// S
DECLARE_DO_FUN(	do_sacrifice	);
DECLARE_DO_FUN(	do_save			);
DECLARE_DO_FUN(	do_say			);
DECLARE_DO_FUN(	do_scan			);
DECLARE_DO_FUN(	do_score		);
DECLARE_DO_FUN( do_scroll		);
DECLARE_DO_FUN( do_selffuse     );
DECLARE_DO_FUN(	do_sell			);
DECLARE_DO_FUN( do_set			);
DECLARE_DO_FUN(	do_shout		);
DECLARE_DO_FUN( do_show			);
DECLARE_DO_FUN(	do_shutdow		);
DECLARE_DO_FUN(	do_shutdown		);
DECLARE_DO_FUN( do_sit			);
DECLARE_DO_FUN( do_skillchange	);
DECLARE_DO_FUN( do_skillprereq  );
DECLARE_DO_FUN( do_skills		);
DECLARE_DO_FUN( do_skillstat	);
DECLARE_DO_FUN(	do_sla			);
DECLARE_DO_FUN(	do_slay			);
DECLARE_DO_FUN(	do_sleep		);
DECLARE_DO_FUN(	do_slookup		);
DECLARE_DO_FUN( do_smote		);
DECLARE_DO_FUN(	do_sneak		);
DECLARE_DO_FUN(	do_snoop		);
DECLARE_DO_FUN( do_socials		);
DECLARE_DO_FUN(	do_south		);
DECLARE_DO_FUN( do_sockets		);
DECLARE_DO_FUN( do_spells		);
DECLARE_DO_FUN(	do_split		);
DECLARE_DO_FUN(	do_sset			);
DECLARE_DO_FUN( do_ssj1         );
DECLARE_DO_FUN( do_ssj2         );
DECLARE_DO_FUN( do_ssj3         );
DECLARE_DO_FUN( do_ssj4         );
DECLARE_DO_FUN( do_ssj5         );
DECLARE_DO_FUN( do_stance       );
DECLARE_DO_FUN(	do_stand		);
DECLARE_DO_FUN( do_stat			);
DECLARE_DO_FUN(	do_steal		);
DECLARE_DO_FUN( do_story		);
DECLARE_DO_FUN(	do_stretch		);
DECLARE_DO_FUN( do_string		);
DECLARE_DO_FUN(	do_study		);
DECLARE_DO_FUN( do_super        );
DECLARE_DO_FUN( do_suppress     );
DECLARE_DO_FUN(	do_surrender	);
DECLARE_DO_FUN(	do_switch		);
// T
DECLARE_DO_FUN(	do_teach		);
DECLARE_DO_FUN(	do_tell			);
DECLARE_DO_FUN(	do_telnetga		);
DECLARE_DO_FUN(	do_time			);
DECLARE_DO_FUN(	do_title		);
DECLARE_DO_FUN( do_toplist      );
DECLARE_DO_FUN(	do_transfer		);
DECLARE_DO_FUN(	do_transdown	);
DECLARE_DO_FUN(	do_transup		);
DECLARE_DO_FUN(	do_trust		);
DECLARE_DO_FUN(	do_typo			);
// U
DECLARE_DO_FUN( do_unalias		);
DECLARE_DO_FUN( do_unfuse       );
DECLARE_DO_FUN(	do_unlock		);
DECLARE_DO_FUN(	do_unwait		);
DECLARE_DO_FUN(	do_up			);
DECLARE_DO_FUN(	do_upgrade		);
// V
DECLARE_DO_FUN(	do_value		);
DECLARE_DO_FUN( do_varlimit     );
DECLARE_DO_FUN(	do_visible		);
DECLARE_DO_FUN( do_violate		);
DECLARE_DO_FUN( do_vnum			);
// W
DECLARE_DO_FUN(	do_wake			);
DECLARE_DO_FUN(	do_wear			);
DECLARE_DO_FUN(	do_weather		);
DECLARE_DO_FUN(	do_west			);
DECLARE_DO_FUN(	do_where		);
DECLARE_DO_FUN(	do_who			);
DECLARE_DO_FUN(	do_wimpy		);
DECLARE_DO_FUN(	do_wizhelp		);
DECLARE_DO_FUN(	do_wizlock		);
DECLARE_DO_FUN( do_wizlist		);
DECLARE_DO_FUN( do_wiznet		);
DECLARE_DO_FUN(	do_wpeace		);
DECLARE_DO_FUN( do_worth		);
// X
// Y
DECLARE_DO_FUN(	do_yell			);
// Z
DECLARE_DO_FUN(	do_zap			);
DECLARE_DO_FUN( do_zecho		);

// Skills
DECLARE_SKILL_FUN( skill_bash			);
DECLARE_SKILL_FUN( skill_death_ball     );
DECLARE_SKILL_FUN( skill_destructo_disk );
DECLARE_SKILL_FUN( skill_elbow          );
DECLARE_SKILL_FUN( skill_energy_ball    );
DECLARE_SKILL_FUN( skill_energy_beam    );
DECLARE_SKILL_FUN( skill_energy_slash   );
DECLARE_SKILL_FUN( skill_eyebeam        );
DECLARE_SKILL_FUN( skill_eye_gouge      );
DECLARE_SKILL_FUN( skill_finalflash     );
DECLARE_SKILL_FUN( skill_fingerbeam     );
DECLARE_SKILL_FUN( skill_focus          );
DECLARE_SKILL_FUN( skill_heal           );
DECLARE_SKILL_FUN( skill_heart_shot     );
DECLARE_SKILL_FUN( skill_hyperpunch     );
DECLARE_SKILL_FUN( skill_kamehameha     );
DECLARE_SKILL_FUN( skill_kick           );
DECLARE_SKILL_FUN( skill_knee           );
DECLARE_SKILL_FUN( skill_masenko        );
DECLARE_SKILL_FUN( skill_mouthbeam      );
DECLARE_SKILL_FUN( skill_power_bomb     );
DECLARE_SKILL_FUN( skill_regen          );
DECLARE_SKILL_FUN( skill_revive         );
DECLARE_SKILL_FUN( skill_scattershot    );
DECLARE_SKILL_FUN( skill_solarflare     );
DECLARE_SKILL_FUN( skill_specialbeam    );
DECLARE_SKILL_FUN( skill_spirit_bomb    );
DECLARE_SKILL_FUN( skill_sweep          );
DECLARE_SKILL_FUN( skill_timestop       );
DECLARE_SKILL_FUN( skill_throat_shot    );
