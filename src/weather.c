/*--------------------------------------------------------------------------
              .88b  d88. db    db d8888b.   .d888b. db   dD
              88'YbdP`88 88    88 88  `8D   VP  `8D 88 ,8P'
              88  88  88 88    88 88   88      odD' 88,8P
              88  88  88 88    88 88   88    .88'   88`8b
              88  88  88 88b  d88 88  .8D   j88.    88 `88.
              YP  YP  YP ~Y8888P' Y8888D'   888888D YP   YD
This material is copyrighted (c) 1999 - 2000 by Thomas J Whiting 
(twhiting@hawmps.2y.net). Usage of this material  means that you have read
and agree to all of the licenses in the ../licenses directory. None of these
licenses may ever be removed.
----------------------------------------------------------------------------
A LOT of time has gone into this code by a LOT of people. Not just on
this individual code, but on all of the codebases this even takes a piece
of. I hope that you find this code in some way useful and you decide to
contribute a small bit to it. There's still a lot of work yet to do.
---------------------------------------------------------------------------*/

// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "db.h"
void    lightning        args( ( void ) );
void    ice	         args( ( void ) );
void    hail	         args( ( void ) );
void    blizzard         args( ( void ) );
void    fog               args( ( void ) );
void    weather_update   args( ( void ) );


/*
 * Update the weather.
 */
void weather_update( void )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    buf[0] = '\0';

    switch (++time_info.hour) {
        case  5:
            weather_info.sunlight = SUN_LIGHT;
            strcat( buf, "{xThe {yday{x has begun.\n\r" );
            break;

        case  6:
            weather_info.sunlight = SUN_RISE;
            strcat( buf, "{xThe {Ysun{x rises in the east.\n\r" );
            break;

        case 19:
            weather_info.sunlight = SUN_SET;
            strcat( buf, "{xThe {Ysun{x slowly disappears in the west.\n\r" );
            break;

        case 20:
            weather_info.sunlight = SUN_DARK;
            strcat( buf, "{xThe {Dnight{x has begun.\n\r" );
            break;

        case 24:
            time_info.hour = 0;
            time_info.day++;
            break;
    }

    if (buf[0] != '\0') {
        for ( d = descriptor_list; d != NULL; d = d->next ) {
            if (d->connected == CON_PLAYING
                && IsOutdoors(d->character)
                && IS_AWAKE(d->character) )
                sendch (buf, d->character);
        }
    }

    if (time_info.day >= 35) {
	    time_info.day = 0;
	    time_info.month++;
    }

    if (time_info.month >= 12) {
	    time_info.month = 0;
	    time_info.year++;
    }

    /*  
	    Weather change.
     */ 
    switch (weather_info.sky) {
        case SKY_CLOUDLESS:
      	     if (number_chance(10) && time_info.hour <= 6)
                weather_info.sky = SKY_FOGGY;
            else if (number_chance(15))
    	           weather_info.sky = SKY_CLOUDY;
            else if (number_chance(30))
                weather_info.sky = SKY_RAINING;
            else if (number_chance(45))
                weather_info.sky = SKY_CLOUDLESS;
            break;

        case SKY_CLOUDY:
            if (number_chance(15))
        	    weather_info.sky = SKY_SNOWING;
	    	else if (number_chance(15))
	            weather_info.sky = SKY_HAILSTORM;
	    	else if (number_chance(15))
	            weather_info.sky = SKY_THUNDERSTORM;
	    	else if (number_chance(15))
	    	    weather_info.sky = SKY_ICESTORM;
	        else if (number_chance(15))
	            weather_info.sky = SKY_CLOUDLESS;
	        else if (number_chance(25))
	    	    weather_info.sky = SKY_CLOUDY;
            //break;

       case SKY_RAINING:
            if (number_chance(15)) {
                weather_info.sky = SKY_LIGHTNING;
                lightning ( );
            }
            else if (number_chance(10)) {
                weather_info.sky = SKY_HAILSTORM;
	            hail();
            }
            else if (number_chance(10))
                weather_info.sky = SKY_THUNDERSTORM;
            else if (number_chance(10))
                weather_info.sky = SKY_CLOUDY;
            else  if (number_chance(55))
                weather_info.sky = SKY_RAINING;
            break;

        case SKY_SNOWING:
            if (number_chance(15))
	            weather_info.sky = SKY_BLIZZARD;
            else if (number_chance(15))
        	    weather_info.sky = SKY_CLOUDY;
            else if (number_chance(15))
        	    weather_info.sky = SKY_RAINING;
        	else if (number_chance(55))
        	    weather_info.sky = SKY_SNOWING;
        	break;
        
        case SKY_LIGHTNING:
            if (number_chance(15))
        	    weather_info.sky = SKY_THUNDERSTORM;
            else if (number_chance(15))
                weather_info.sky = SKY_RAINING;
            else if (number_chance(15))
                weather_info.sky = SKY_CLOUDY;
            else if (number_chance(15))
        	    weather_info.sky = SKY_HAILSTORM;
            else if (number_chance(40))
        	    weather_info.sky = SKY_LIGHTNING;
        	break;
    
        case SKY_FOGGY:
            if (number_chance(45))
                weather_info.sky = SKY_CLOUDY;
            else if (number_chance(55)) {
                weather_info.sky = SKY_FOGGY;
                fog();
        	}
            break;

        case SKY_THUNDERSTORM:
            if (number_chance(15))
                weather_info.sky = SKY_RAINING;
            else if (number_chance(15))
                weather_info.sky = SKY_CLOUDY;
            else if (number_chance(15))
                weather_info.sky = SKY_LIGHTNING;
            else if (number_chance(15)) {
        	    weather_info.sky = SKY_HAILSTORM;
                hail ();
            }
            else if (number_chance(40))
        	    weather_info.sky = SKY_THUNDERSTORM;
        	break;

        case SKY_HAILSTORM:
            if (number_chance(15))
        	    weather_info.sky = SKY_CLOUDY;
            else if (number_chance(30))
        	    weather_info.sky = SKY_RAINING;
            else if (number_chance(55)) {
	            weather_info.sky = SKY_HAILSTORM;
	            hail();
        	}
	        break;

        case SKY_ICESTORM:
            if (number_chance(15))
        	    weather_info.sky = SKY_CLOUDY;
            else if (number_chance(15))
	    	    weather_info.sky = SKY_BLIZZARD;
            else if (number_chance(15))
        	    weather_info.sky = SKY_SNOWING;
            else if (number_chance(55)) {
                ice();
                weather_info.sky = SKY_ICESTORM;
            }
	        break;

        case SKY_BLIZZARD:
            if (number_chance(15))
        	    weather_info.sky = SKY_SNOWING;
            else if (number_chance(15)) {
                weather_info.sky = SKY_ICESTORM;
                blizzard();
                ice();
            }
            else if (number_chance(15)) {
                weather_info.sky = SKY_CLOUDY;
                blizzard();
        	}
            else if (number_chance(55)) {
                weather_info.sky = SKY_BLIZZARD;
                blizzard();
            }
            break;
            
        default:
            weather_info.sky = SKY_CLOUDLESS;
            break;
    }
    
    return;
}






/*
New weather command. Reads the new weather stats as well as  tells you
what time of day it is (morning, noon, night)
Added so that players don't HAVE to have autoweather enabled*/
void do_weather (CHAR_DATA *ch, char *argument) {
    char *suf;
    int day;

    day = time_info.day + 1;

         if ( day > 4 && day <  20 ) suf = "th";
    else if ( day % 10 ==  1       ) suf = "st";
    else if ( day % 10 ==  2       ) suf = "nd";
    else if ( day % 10 ==  3       ) suf = "rd";
    else                             suf = "th";


    if (!IsOutdoors(ch)) {
	    sendch( "You can't see the weather indoors.\n\r", ch );
	    return;
    }
    sendch("{x\n\r",ch);
    show_weather (ch);
    printf_to_char (ch, "{g[{G Time  {g] {WIt is %d o'clock %s{x\n\r",
	                (time_info.hour % 12 == 0) ? 12 : time_info.hour %12,
	                time_info.hour >= 12 ? "pm" : "am");
    return;

}

bool IsOutdoors (CHAR_DATA *ch) {
    if (!IS_SET(ch->in_room->room_flags, ROOM_INDOORS) &&
        ch->in_room->sector_type != SECT_INSIDE)
        return TRUE;
    else
        return FALSE;
}

void show_weather(CHAR_DATA *ch) {
    if (weather_info.sky == SKY_RAINING)
        sendch ("{g[{GWeather{G] {WThere is a cold rain trickling down.{x\n\r",ch);
    else if (weather_info.sky == SKY_CLOUDY)
        printf_to_char (ch, "{g[{GWeather{g] {W%s{x.\n\r", weather_info.change >= 0 ? "A warm breeze can be felt about" : "A cold breeze can be felt about" );
    else if (weather_info.sky == SKY_CLOUDLESS)
        sendch ("{g[{GWeather{g] {WThe sky is beautiful, not a cloud around.{x\n\r",ch);
    else if (weather_info.sky == SKY_THUNDERSTORM)
        sendch ("{g[{GWeather{g] {WThe skies thunder as a storm approaches.{x\n\r",ch);
    else if (weather_info.sky == SKY_ICESTORM)
        sendch ("{g[{GWeather{g] {WSheets of ice appear to be falling from the sky.{x\n\r",ch);
    else if (weather_info.sky == SKY_HAILSTORM)
        sendch ("{g[{GWeather{g] {WIcey golfball-like substances are falling from the sky.{x\n\r",ch);
    else if (weather_info.sky == SKY_SNOWING)
        sendch ("{g[{GWeather{g] {WA light snow is falling.{x\n\r",ch);
    else if (weather_info.sky == SKY_BLIZZARD)
        sendch ("{g[{GWeather{g] {WThere is a blizzard about.{x\n\r",ch);
    else if (weather_info.sky == SKY_FOGGY)
        sendch ("{g[{GWeather{g] {WA misty haze covers the horizon.{x\n\r",ch);
    else if (weather_info.sky == SKY_LIGHTNING)
        sendch ("{g[{GWeather{g] {WA {yLightning{W storm is approaching {x\n\r",ch);
    return;
}

void lightning (void) {
    DESCRIPTOR_DATA *d;
    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->connected == CON_PLAYING
            && IsOutdoors (d->character)
            && IS_AWAKE (d->character)
            && number_chance(10)
            && !IS_IMMORTAL(d->character)
            && weather_info.sky == SKY_LIGHTNING) {

            sendch ("{x{RYou see a brilliant flash come down from the sky and then black out!{x\n\r",d->character);
            act ("$n has been struck by lightning!", d->character, NULL, NULL,TO_ROOM);

            damage (d->character, d->character, d->character->max_hit / 10, TYPE_UNDEFINED, DAM_LIGHTNING, FALSE);
            wait (d->character, 15 * PULSE_SECOND);
        }
    }
}

void blizzard (void) {
    DESCRIPTOR_DATA *d;
    int nNum = number_range(1, 10);
    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->connected == CON_PLAYING
            && IsOutdoors(d->character)
            && weather_info.sky == SKY_BLIZZARD) {
            if (nNum <= 6)
                sendch("{RThe sky before you is a mist of white blur. Perhaps you should find a safe place indoors.{x \n\r",d->character);
            else if (nNum <= 8 ) {
                sendch("{RNot being able to see where you are going, you slip and fall face first!{x \n\r",d->character );
                act( "$n falls face first into the oncoming drifts!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 25, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
            else if (nNum == 9) {
                sendch("{RYou've managed to slide and fall, hard, onto your back!{x \n\r",d->character);
                act( "$n has planted themselves right on $s back!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 20, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
            else {
                sendch("{RYour body twitches and as hypothermia sets in.{x \n\r", d->character);
                act( "You watch as hypothermia begins to set into $n!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 15, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
        }
    }
}

void ice (void) {
    DESCRIPTOR_DATA *d;
    int nNum = number_range(1, 10);
    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->connected == CON_PLAYING
            && IsOutdoors(d->character)
            && weather_info.sky == SKY_ICESTORM) {
            if (nNum <= 6)
                sendch ("{RIt's starting to rain sheets of ice. Perhaps you should find a way inside.{x \n\r",d->character);
            else if (nNum <= 8 ) {
                sendch("{RThe ice around your feet firms up and causes you to fall face first!{x \n\r",d->character );
                act( "$n falls face first into the ground!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 25, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
            else if (nNum == 9) {
                sendch("{RYou've managed to slide and fall, hard, onto your back!{x \n\r",d->character);
                act( "$n has planted themselves right on $s back!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 20, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
            else {
                sendch("{RYour body twitches and as hypothermia sets in.{x \n\r", d->character);
                act( "You watch as hypothermia begins to set into $n!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 15, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
        }
    }
}

void hail (void) {
    DESCRIPTOR_DATA *d;
    int nNum = number_range(1, 10);
    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->connected == CON_PLAYING
            && IsOutdoors(d->character)
            && weather_info.sky == SKY_HAILSTORM) {
            if (nNum <= 6)
                sendch ("{RWas that a golfball or hail? It might be a good idea to find yourself a way indoors quickly.{x\n\r",d->character);
            else if (nNum <= 9 ) {
                sendch("{RYou are hit in the face by hail!{x \n\r",d->character );
                act( "You watch in ammusement as $n is hit in the face by a ball of ice.", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 25, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
            else {
                sendch("{RYour body twitches as hypothermia sets in!{x \n\r", d->character);
                act( "You watch as hypothermia begins to set into $n!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 15, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
        }
    }
}

void fog (void) {
    DESCRIPTOR_DATA *d;
    int nNum = number_range(1, 10);
    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->connected == CON_PLAYING
            && IsOutdoors(d->character)
            && weather_info.sky == SKY_FOGGY) {
            if (nNum <= 8)
                sendch ("{RThe morning fog is as thick as pea soup. Perhaps you should find your way indoors.{x \n\r",d->character);
            else {
                sendch("{RNot being able to see where you are going, you trip over yourself and fall!{x \n\r",d->character );
                act( "$n trips right over $mself!", d->character, NULL, NULL,TO_ROOM);
                damage (d->character, d->character, d->character->max_hit / 20, TYPE_UNDEFINED, DAM_COLD, FALSE);
            }
        }
    }
}

/*
void spell_control_weather(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    if ( !str_cmp( target_name, "better" ) )
{

if (weather_info.sky == SKY_CLOUDLESS)
{
send_to_char("But it's so beautiful outside already\n\r",ch);
return;
}
else
if (weather_info.sky == SKY_CLOUDY)
{
send_to_char("You recite the ancient spell and the clouds part in obedience\n\r",ch);
act( "$n makes a strange movement with their hands and the clouds part.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_CLOUDLESS;
return;
}
else
if (weather_info.sky == SKY_RAINING)
{
send_to_char("You recite the ancient spell and the  rain stops in obedience\n\r",ch);
act( "$n makes a strange movement with their hands and the rain stops.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_CLOUDY;
return;
}
else
if (weather_info.sky == SKY_LIGHTNING)
{
send_to_char("You recite the ancient spell and the lightning ceases in obedience\n\r",ch);
act( "$n makes a strange movement with their hands and the lightning stops.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_THUNDERSTORM;
return;
}
else
if (weather_info.sky == SKY_THUNDERSTORM)
{
send_to_char("You recite the ancient spell and the storm ceases\n\r",ch);
act( "$n makes a strange movement with their hands and the  storm ceases.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_RAINING;
return;
}
else
if (weather_info.sky == SKY_SNOWING)
{
send_to_char("You recite the ancient spell and the snow ceases in obedience\n\r",ch);
act( "$n makes a strange movement with their hands and the snow ceases.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_CLOUDY;
return;
}
else
if (weather_info.sky == SKY_BLIZZARD)
{
send_to_char("You recite the ancient spell and the  horizon clears\n\r",ch);
act( "$n makes a strange movement with their hands and the horizon clears.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_SNOWING;
return;
}
else
if (weather_info.sky == SKY_ICESTORM)
{
send_to_char("You recite the ancient spell and the  horizon clears\n\r",ch);
act( "$n makes a strange movement with their hands and the horizon clears.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_RAINING;
return;
}
else
if (weather_info.sky == SKY_HAILSTORM)
{
send_to_char("You recite the ancient spell and the  horizon clears\n\r",ch);
act( "$n makes a strange movement with their hands and the horizon clears.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_RAINING;
return;
}
else
if (weather_info.sky == SKY_FOGGY)
{
send_to_char("You recite the ancient spell and the  horizon clears\n\r",ch);
act( "$n makes a strange movement with their hands and the horizon clears.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_CLOUDY;
return;
}
else 
{
            send_to_char("Bad Weather Call. Please notify the imms.\n\r",ch);
}

}
    else if ( !str_cmp( target_name, "worse" ) )
{

if (weather_info.sky == SKY_CLOUDLESS)
{
send_to_char("You recite the ancient spell and the clouds  come at your command.\n\r",ch);
act( "$n makes a strange movement with their hands and the clouds  darken the sky.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_CLOUDY;
return;
}
else
if (weather_info.sky == SKY_CLOUDY)
{
send_to_char("You recite the ancient spell and the clouds trickle down rain\n\r",ch);
act( "$n makes a strange movement with their hands and the clouds open up to rain.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_RAINING;
return;
}
else
if (weather_info.sky == SKY_RAINING)
{
send_to_char("You recite the ancient spell and the  rain  turns to hail\n\r",ch);
act( "$n makes a strange movement with their hands and the rain  turns to hail.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_HAILSTORM;
return;
}
else
if (weather_info.sky == SKY_LIGHTNING)
{
send_to_char("You recite the ancient spell and the  clouds send down sheets of ice\n\r",ch);
act( "$n makes a strange movement with their hands and the lightning turns to ice.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_ICESTORM;
return;
}
else
if (weather_info.sky == SKY_THUNDERSTORM)
{
send_to_char("You recite the ancient spell and the clouds clap in thunderous approval\n\r",ch);
act( "$n makes a strange movement with their hands and the  clouds clap in thunder.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_LIGHTNING;
return;
}
else
if (weather_info.sky == SKY_SNOWING)
{
send_to_char("You recite the ancient spell and the snow increases in obedience\n\r",ch);
act( "$n makes a strange movement with their hands and the snow turns to a blizzard.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_BLIZZARD;
return;
}
else
if (weather_info.sky == SKY_BLIZZARD)
{
send_to_char("It's already as bad as it can get\n\r",ch);
return;
}
else
if (weather_info.sky == SKY_ICESTORM)
{
send_to_char("It's already as bad as it can get\n\r",ch);
return;
}
else
if (weather_info.sky == SKY_HAILSTORM)
{
send_to_char("It's already as bad as it can get\n\r",ch);
return;
}
else
if (weather_info.sky == SKY_FOGGY)
{
send_to_char("You recite the ancient spell and the  horizon clears\n\r",ch);
act( "$n makes a strange movement with their hands and the horizon clears.", ch, NULL, NULL, TO_ROOM);
weather_info.sky = SKY_CLOUDY;
return;

}
else
{
            send_to_char("Bad Weather Call. Please notify the imms.\n\r",ch);
}

}
    else
        send_to_char ("Do you want it to get better or worse?\n\r", ch );

    sendch ("Ok.\n\r", ch );
    return;
}
*/

void do_wset (CHAR_DATA *ch, char *argument) {
    char arg1 [MIL];
    argument = one_argument( argument, arg1 );
    
    if (arg1[0] == '\0') {
        sendch ("Syntax:\n\r",ch);
        sendch ("  set weather <condition> \n\r", ch);
        sendch ("Condition can be :\n\r",ch);
        sendch ("   hail  lightning     icestorm   blizzard  snowing\n\r",ch);
        sendch ("   fog   thunderstorm  cloudless  cloudy    rain\n\r",ch);
        return;
    }
 
    if( !str_cmp(arg1,"cloudless")) {
        weather_info.sky = SKY_CLOUDLESS;
        sendch ("You wave your hands and in reverence to you, the clouds dissapear \n\r", ch);
        act( "$n makes a strange movement with their hands and the clouds part.", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"fog")) {
        weather_info.sky = SKY_FOGGY;
        sendch ("You wave your hands and in reverence to you, a mist vapors the horizon \n\r", ch);
        act( "$n makes a strange movement with their hands and a mist vapors the horizon.", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"hail")) {
        weather_info.sky = SKY_HAILSTORM;
        sendch ("You wave your hands and in reverence to you, hailstones fall from the sky \n\r", ch);
        act( "$n makes a strange movement with their hands and hailstones fall from the sky.", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"icestorm")) {
        weather_info.sky = SKY_ICESTORM;
        sendch ("You wave your hands and in reverence to you, it starts raining ice \n\r", ch);
        act( "$n makes a strange movement with their hands and it starts raining ice.", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"blizzard")) {
        weather_info.sky = SKY_BLIZZARD;
        sendch ("You wave your hands and in reverence to you, snowflakes cover the horizon making it impossible to see.\n\r", ch);
        act( "$n makes a strange movement with their hands and snowflakes cover the horizon making it impossibile to see", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"snowing")) {
        weather_info.sky = SKY_SNOWING;
        sendch ("You wave your hands and in reverence to you, snowflakes fall from the sky..\n\r", ch);
        act( "$n makes a strange movement with their hands and snowflakes fall from the sky", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"rain")) {
        weather_info.sky = SKY_RAINING;
        sendch ("You wave your hands and in reverence to you, a warm rain starts to fall.\n\r", ch);
        act( "$n makes a strange movement with their hands and a warm rain starts to fall", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"lightning")) {
        weather_info.sky = SKY_LIGHTNING;
        sendch ("You wave your hands and in reverence to you, lightning pierces the sky. \n\r", ch);
        act( "$n makes a strange movement with their hands and lightning pierces the sky", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"thunderstorm")) {
        weather_info.sky = SKY_THUNDERSTORM;
        sendch ("You wave your hands and in reverence to you, The clouds clap in thunder.\n\r", ch);
        act( "$n makes a strange movement with their hands and the clouds clap in thunder", ch, NULL, NULL, TO_ROOM);
    }
    else if (!str_cmp(arg1,"cloudy")) {
        weather_info.sky = SKY_CLOUDY;
        sendch ("You wave your hands and in reverence to you, clouds cover the horizon, threatening rain.\n\r", ch);
        act( "$n makes a strange movement with their hands and clouds cover the horizon, threatening rain", ch, NULL, NULL, TO_ROOM);
    }
    else
        // Echo syntax
        do_function (ch, &do_wset, "");
}
