
// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"


void do_fuse (CHAR_DATA *ch, char *argument) {
    DESCRIPTOR_DATA *d, *desc1, *desc2;
    CHAR_DATA *ch2, *fuse;
    OBJ_DATA *obj, *obj_next;
    char arg[MAX_INPUT_LENGTH];
    char *msg = "***********************************************\n\r"
                "*%s has fused with %s, with %s\n\r"
                "*taking control.  %s will remain\n\r"
                "*conscious of everything happening,\n\r"
                "*but can take no action while the\n\r"
                "*fuse lasts.\n\r"
                "***********************************************\n\r";
    char slave_name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int i, j;

    if (IS_NPC(ch)) {
        sendch ("Players only.\n\r", ch);
        return;
    }

    if (get_skill(ch, gsn_fusion) < 1) {
        sendch ("You do not know how to fuse.\n\r", ch);
        return;
    }

    one_argument (argument, arg);

    if (argument[0] == '\0') {
        sendch ("fuse <target> to fuse with someone.\n\r"
                      "fuse accept to accept a request to fusion.\n\r"
                      "fuse decline to decline the request.\n\r"
                      "fuse cancel to cancel an outgoing request.\n\r", ch);
        return;
    }
    else if (!str_cmp(argument, "accept")) {
        if (ch->pcdata->fusion_request) {
            sendch ("You must first cancel your request to fuse with someone.\n\r", ch);
            return;
        }

        ch2 = ch;
        ch = NULL;

        // Find character who issued request
        for (d = descriptor_list; d; d = d->next) {
            if (d->character && d->character->pcdata &&
                d->character->pcdata->fusion_request == ch2) {
                ch = d->character;
                break;
            }
        }
        if (ch == NULL) {
            sendch ("No one wants to fuse with you!\n\r", ch2);
            return;
        }
		if (IS_FUSED(ch2)) {
            sendch ("You're already fused!\n\r", ch2);
			return;
		}
		if (IS_FUSED(ch)) {
            act ("$e is already fused!", ch2, NULL, ch, TO_CHAR);
			return;
		}

        act ("{r$n fuses with $N!{x", ch, NULL, ch2, TO_NOTVICT);

        desc1 = ch->desc;
        desc2 = ch2->desc;

        fuse = new_char ();
        fuse->pcdata = new_pcdata ();

        fuse->name = str_dup (ch->name);

        // Locate a vowel in the name, around the 5th letter
        for (i = UMIN(strlen(ch->name)-1, 5); i > 1; --i)
            if (LOWER(ch->name[i]) == 'a' || LOWER(ch->name[i]) == 'e' ||
                LOWER(ch->name[i]) == 'i' || LOWER(ch->name[i]) == 'o' ||
                LOWER(ch->name[i]) == 'u' || LOWER(ch->name[i]) == 'y')
                break;

        // Now add the first 3 of the second name after the vowel
        ++i;
        for (j = 0; ch2->name[j] != '\0' && j < 3; ++j)
            fuse->name[i+j] = LOWER(ch2->name[j]);
        fuse->name[i+j] = '\0';

        fuse->id = get_pc_id ();

        // Add the two chars together
        // (All ripped from load_char_obj)

        fuse->race = ch->race;
        fuse->act = ch->act;
        fuse->comm = ch->comm;
        if (ch->prompt)
            fuse->prompt = str_dup (ch->prompt);
        fuse->pcdata->confirm_delete = FALSE;
        fuse->pcdata->board = &boards[DEFAULT_BOARD];
        fuse->pcdata->pwd = str_dup ("");
        if (ch->pcdata->bamfin)
            fuse->pcdata->bamfin = str_dup (ch->pcdata->bamfin);
        if (ch->pcdata->bamfout)
            fuse->pcdata->bamfout = str_dup (ch->pcdata->bamfout);
        fuse->pcdata->title = str_dup (" the {YFused{x.");
        for (i = 0; i < MAX_STATS; i++)
            fuse->perm_stat[i] = ch->perm_stat[i] + ch2->perm_stat[i];
        fuse->pcdata->condition[COND_THIRST] = ch->pcdata->condition[COND_THIRST] + ch2->pcdata->condition[COND_THIRST];
        fuse->pcdata->condition[COND_FULL] = ch->pcdata->condition[COND_FULL] + ch2->pcdata->condition[COND_FULL];
        fuse->pcdata->condition[COND_HUNGER] = ch->pcdata->condition[COND_HUNGER] + ch2->pcdata->condition[COND_HUNGER];
        fuse->pcdata->security = ch->pcdata->security;

        fuse->size = ch->size;
        fuse->affected_by = ch->affected_by;
        fuse->alignment = URANGE(-1000,ch->alignment + ch2->alignment,1000);
        for	(i = 0; i < 4; i++)
            fuse->armor[i] = ch->armor[i] + ch2->armor[i];
        for (i = 0; i < MAX_ALIAS; ++i) {
            if (fuse->pcdata->alias[i] == NULL)
                break;
            fuse->pcdata->alias[i] = str_dup (fuse->pcdata->alias[i]);
            fuse->pcdata->alias_sub[i] = str_dup (fuse->pcdata->alias_sub[i]);
        }
        for (i = 0; i < MAX_STATS; i++) {
            fuse->mod_stat[i] = ch->mod_stat[i] + ch2->mod_stat[i];
            fuse->perm_stat[i] = ch->perm_stat[i] + ch2->perm_stat[i];
        }
        for (i = 0; i < MAX_BOARD; i++)
            fuse->pcdata->last_note[i] = ch->pcdata->last_note[i];
        fuse->clan = ch->clan;
        fuse->comm = ch->comm;
        fuse->damroll = ch->damroll + ch2->damroll;
        fuse->hitroll = ch->hitroll + ch2->hitroll;
        fuse->max_hit = ch->max_hit + ch2->max_hit;
        fuse->hit = fuse->max_hit;
        fuse->max_ki = ch->max_ki + ch2->max_ki;
        fuse->ki = fuse->max_ki;
        fuse->pcdata->perm_hit = ch->pcdata->perm_hit + ch2->pcdata->perm_hit;
        fuse->pcdata->perm_ki = ch->pcdata->perm_ki + ch2->pcdata->perm_ki;
        fuse->invis_level = ch->invis_level;
        fuse->incog_level = ch->incog_level;
        fuse->level = ch->level;
        fuse->played = ch->played + ch2->played;
        fuse->nCurPl = 25;
        fuse->race = ch->race;
        fuse->saving_throw = ch->saving_throw + ch2->saving_throw;
        fuse->lines = ch->lines;
        fuse->sex = ch->sex;
        for (i=0; i<MAX_SKILL; ++i)
            fuse->pcdata->learned[i] = ch->pcdata->learned[i] + ch2->pcdata->learned[i];
        fuse->pcdata->true_sex = ch->pcdata->true_sex;
        fuse->trans_count = ch->trans_count;
        fuse->wimpy = ch->wimpy;
        fuse->wiznet = ch->wiznet;
        fuse->zenni = ch->zenni + ch2->zenni;

        fuse->fuse_control = str_dup (ch->name);
        fuse->fuse_slave = str_dup (ch2->name);
        fuse->fuse_count = 5;

        // Take all the equipment from the controller
        for (obj = ch->carrying; obj; obj = obj_next) {
			obj_next = obj->next_content;
            obj_from_char (obj);
            obj_to_char (obj, fuse);
		}

        ResetDiff(fuse);

		char_to_room(fuse, ch->in_room);

        strcpy (slave_name, ch2->name);
        strcat (slave_name, "Slave");

        fuse->next = char_list;
        char_list = fuse;
        reset_char (fuse);

        // Remove the characters
        save_char_obj (ch);
        save_char_obj (ch2);

        if (ch->pcdata->in_progress)
            free_note (ch->pcdata->in_progress);
        if (ch2->pcdata->in_progress)
            free_note (ch2->pcdata->in_progress);

        extract_char (ch, TRUE);
        extract_char (ch2, TRUE);

        // Set the descriptors, create some place-holder character for the slave
        fuse->desc = desc1;

        fuse->desc->character = fuse;
        fuse->slave = new_char ();
        fuse->slave->desc = desc2;
        fuse->slave->desc->connected = CON_FUSE_SLAVE;
        fuse->slave->desc->character = fuse->slave;
        fuse->slave->pcdata = new_pcdata ();
        fuse->slave->name = str_dup (slave_name);

        sprintf (buf, msg, fuse->fuse_control, fuse->fuse_slave, fuse->fuse_control, fuse->fuse_slave);
        sendch (buf, fuse);

        return;
    }
    else if (!str_cmp(argument, "decline")) {
        ch2 = NULL;

        // Find character who issued request
        for (d = descriptor_list; d; d = d->next) {
            if (d->character && d->character->pcdata &&
                d->character->pcdata->fusion_request == ch) {
                ch2 = d->character;
                break;
            }
        }
        if (ch2 == NULL) {
            sendch ("No one wants to fuse with you!\n\r", ch);
            return;
        }

        sendch ("Your request to fuse has been declined.\n\r", ch2);
        sendch ("You decline the request to fuse.\n\r", ch);
        ch2->pcdata->fusion_request = NULL;
        return;
    }
    else if (!str_cmp(argument, "cancel")) {
        if (ch->pcdata->fusion_request == NULL) {
            sendch ("You haven't asked anyone to fuse.\n\r", ch);
            return;
        }

        sendch ("The request to fuse has been cancelled.\n\r", ch->pcdata->fusion_request);
        sendch ("You have cancelled the request to fuse.\n\r", ch);
        ch->pcdata->fusion_request = NULL;
        return;
    }
    else if ((ch2 = get_char_room(ch, NULL, arg)) == NULL || IS_NPC(ch2)) {
        sendch ("Character not found.\n\r", ch);
        return;
    }
	
	if (IS_FUSED(ch2)) {
        sendch ("You're already fused!\n\r", ch2);
		return;
	}

	if (IS_FUSED(ch)) {
        act ("$e is already fused!", ch2, NULL, ch, TO_CHAR);
		return;
	}

    if (ch->nDifficulty/2 > ch2->nDifficulty || ch2->nDifficulty/2 > ch->nDifficulty) {
        sendch ("You cannot fuse because you and your target are of radically different power.\n\r", ch);
        return;
    }

    if (get_skill(ch2, gsn_kaioken) < 1) {
        sendch ("You cannot fuse because your target does not know how to.\n\r", ch);
        return;
    }


    if (ch->pcdata->fusion_request)
        sendch ("The request to fuse has been cancelled.\n\r", ch->pcdata->fusion_request);

    ch->pcdata->fusion_request = ch2;

    sendch ("You have sent out a request to fuse.\n\r", ch);
    sprintf (buf, "You have been asked by %s to fuse.\n\r", ch->name);
    sendch (buf, ch2);

    return;
}


void do_unfuse (CHAR_DATA *ch, char *argument) {
    if (IS_NPC(ch)) {
        sendch ("Players only.\n\r", ch);
        return;
    }

    if (!IS_FUSED(ch)) {
        sendch ("You're not fused!\n\r", ch);
        return;
    }

    unfuse (ch);

    return;
}

// NOTE:
// When unfusing a character, the character that goes in is
// not the same as the one that goes out! If you unfuse a
// character, then do something to it, you'll be working on
// the fused one.  Look at the descriptors to get access to
// the true character and its other fused half
void unfuse (CHAR_DATA *ch) {
    DESCRIPTOR_DATA *desc1, *desc2 = NULL, d; // d is a temporary descriptor
    CHAR_DATA *ch1, *ch2;
    OBJ_DATA *obj, *obj_next;
    ROOM_INDEX_DATA *in_room;
    char fuse_control[40], fuse_slave[40];

    if (IS_NPC(ch) || !IS_FUSED(ch))
        return;

    sendch ("{rThe fuse is broken!{x\n\r", ch);

    desc1 = ch->desc;
    if (ch->slave)
        desc2 = ch->slave->desc;

    if (desc2)
        desc2->connected = CON_PLAYING;

    strcpy (fuse_control, ch->fuse_control);
    strcpy (fuse_slave, ch->fuse_slave);
    in_room = ch->in_room;

    if (desc1) {
        load_char_obj (desc1, fuse_control);
        ch1 = desc1->character;
    }
    else {
        load_char_obj (&d, fuse_control);
        ch1 = d.character;
        ch1->desc = NULL;
    }
    if (desc2) {
        load_char_obj (desc2, fuse_slave);
        ch2 = desc2->character;
    }
    else {
        load_char_obj (&d, fuse_slave);
        ch2 = d.character;
        ch2->desc = NULL;
    }

    char_to_room(ch1, in_room);
    char_to_room(ch2, in_room);

    ch1->next = ch2;
    ch2->next = char_list;
    char_list = ch1;

    // Take all the equipment from the previous controlling character
    for (obj = ch1->carrying; obj; obj = obj_next) {
		obj_next = obj->next_content;
        obj_from_char (obj);
        extract_obj (obj);
	}

    // Give all equipment to the new character
    for (obj = ch->carrying; obj; obj = obj_next) {
		obj_next = obj->next_content;
        obj_from_char (obj);
        obj_to_char (obj, ch1);
	}

    if (ch->pcdata->in_progress)
        free_note (ch->pcdata->in_progress);
    extract_char (ch, TRUE);

    // Extracting the fused character will set the descriptors to NULL -- set them back
    ch1->desc = desc1;
    if (desc1)
        desc1->character = ch1;

    reset_char (ch1);
    reset_char (ch2);

    act ("{r$n seperates from the fusion with $N!{x", ch1, NULL, ch2, TO_NOTVICT);
    act ("{rYou break from the fusion with $N.{x", ch1, NULL, ch2, TO_CHAR);
    act ("{rYou break from the fusion with $N.{x", ch2, NULL, ch1, TO_CHAR);
}


