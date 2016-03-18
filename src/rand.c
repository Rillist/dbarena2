
// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "olc.h"
#include "db.h"
#include "rand.h"

// **Locals**
RoomData        *AddRoom          args ( ( int nX, int nY, MapData *pMap ) );
char            *GenRoomDescr     args ( ( ) );
int              GetRandMobVnum   args ( ( int nDiffLow, int nDiffHigh ) );
void             CreateMap        args ( ( int nMapVnum, MapData *pMap ) );
RoomData        *Dig              args ( ( RoomData *pRoom, int nDir, int nState, MapData *pMap ) );
AREA_DATA       *GenerateRandArea args ( ( int nVnumLow, int nDiffLow, int nDiffHigh ) );
void             AddWpnSpec       args ( ( OBJ_DATA *pObj, WpnEffectData *pEffect, WpnSpecData *pSpec ) );
// Prototypes from mem.c
AREA_DATA	    *new_area		  args ( ( void ) );
EXIT_DATA	    *new_exit		  args ( ( void ) );
ROOM_INDEX_DATA *new_room_index	  args ( ( void ) );

// ie north is (0,-1) relative to a position (1 up), east (1,0) (1 right)
int nExitDirEW[6] = {0,1,0,-1,0,0};
int nExitDirNS[6] = {-1,0,1,0,0,0};
// Opposite directions for each direction
int nOppDir[6] = {DIR_SOUTH, DIR_WEST, DIR_NORTH, DIR_EAST, DIR_DOWN, DIR_UP};

#define DESCR_COUNT 12
DescrData ForestDescr[] = {
    {6, "There is a %s here, %s.", {"stone", "boulder", "tombstone"}, {"covered with moss","littered with bones","cracked asunder"}, {"","",""}},
    {5, "A %s %s tree launches towards the sky, %s.", {"small", "large", "massive"}, {"oak", "maple", "pine"}, {"casting deep shadows", "covering the ground with leaves", "offering solace from the sun"}},
    {2, "%s footprints run through a patch of %s, evidence of a recent %s.", {"Fox", "Deer", "Boar"}, {"sand", "mud", "dirt"}, {"hunt", "rainfall", "expedition"}},
    {2, "A %s spring %s %s, offering water to thirsty passerbys.", {"small, crystalline", "large", "beautiful"}, {"erupts", "trickles", "flows"}, {"from under a tree", "out of a rock", "into a pool"}},
    {3, "Birds %s in the %s %s.", {"chirp", "sing", "flutter"}, {"nearby", "surrounding", "immense"}, {"trees", "foliage", "grass"}},
    {1, "Bloods %s on a %s, %s.", {"lays splattered", "lays speckled", "flows down"}, {"rock", "tree", "trunk"}, {"indicating a recent battle", "signs of a fight", "the remains of an unfortunate victim"}},
    {3, "A %s, %s %s sits off in the woods.", {"split", "decaying", "rotten"}, {"ancient", "newly-fallen", "bare"}, {"tree", "tree trunk", "branch"}},
    {1, "A%s skull is %s on a %s, warding off visitors.", {"n icerian", " saiya-jin", " human"}, {"impaled", "ruthelessly stuck", "smashed onto"}, {"pike", "spear", "stick"}},
    {1, "A monument of Antor is erected here. He flexes his muscles, showing off his godly powers. Obviously, he is quite strong.", {"","",""},{"","",""},{"","",""}},
    {8, "Sunlight %s the %s, highlighting %s.", {"filters through", "plays amidst", "beams through"}, {"overhead branches", "foliage", "high-reaching trees"}, {"various flowers on the forest flower", "animals playing in the leaves", "cast aside stones and pebbles"}},
    {4, "%s can heard from deep within.", {"Chattering", "Rustling", "Singing"}, {"","",""}, {"","",""}},
    {8, "The %s continues, %s and %s.", {"path", "trail", "walk"}, {"although it is quite muddy", "torn apart by many feet", "marked with flagstones"}, {"covered by many small twigs", "littered with cast-aside garbage", "encroached upon by many persistant weeds"}},
    {0, "", {"","",""}, {"","",""}, {"","",""}} // Dont include in DESCR_COUNT
};

RoomData *room_free = NULL;

RoomData *new_room (void) {
    static RoomData room_zero;
    RoomData *room;

    if (room_free == NULL)
        room = alloc_perm (sizeof (*room));
    else {
        room = room_free;
        room_free = room_free->pNext;
    }

    *room = room_zero;
    VALIDATE (room);
    return room;
}
void free_room (RoomData * room){
    if (!IS_VALID (room))
        return;
    INVALIDATE (room);

    room->pNext = room_free;
    room_free = room;
}

RoomData *AddRoom (int nX, int nY, MapData *pMap) {
    RoomData *pNewRoom;
    int i;

    pNewRoom = new_room();
    pNewRoom->nX = nX;
    pNewRoom->nY = nY;
    pNewRoom->nNumber = pMap->nRoomCount;
    ++pMap->nRoomCount;
    pMap->pRoomGrid[nX][nY] = pNewRoom;
    for (i = 0; i < 6; ++i)
        pNewRoom->pExit[i] = NULL;
    pNewRoom->pRoomIndex = NULL;
    pNewRoom->pNextProcess = NULL;
    pNewRoom->nState = 0;
    pNewRoom->nMobDiff = 0;
    pNewRoom->pNext = pMap->pTopRoom;
    pMap->pTopRoom = pNewRoom;
    return pNewRoom;
}

char *GenRoomDescr (DescrData *pDescr) {
    char szBuf[MAX_STRING_LENGTH];
    int nLastLine = -1, nLineCount = 0;
    //DescrData *pDescr = ForestDescr;
    szBuf[0] = '\0';
    while (nLineCount < 2) {
        int nLine = 0;//, nLineNum = 0, nChance, nCur = 0;
        int nArg;
        char szArg1[40], szArg2[40], szArg3[40];
        // Find the line to use
        /*
        while (TRUE) {
            if (pDescr[nCur].szLine[0] == '\0') // Last line
                break;
            else if ((nChance = number_range(1,1000)) > nLineNum) {
                nLineNum = nChance;
                nLine = nCur;
                nCur++;
            }
        }
        */
        do {
            nLine = number_range(0,DESCR_COUNT-1);
            if (number_range(1,10) > pDescr[nLine].nWeight)
                nLine = nLastLine;
        } while (nLine == nLastLine);
        if (nLastLine == -1)
            nLastLine = nLine;
        ++nLineCount;

        // Find the args
        szArg1[0] = '\0';
        szArg2[0] = '\0';
        szArg3[0] = '\0';
        while (pDescr[nLine].szOption1[0][0] != '\0') { // Make sure there is actually an option
            nArg = number_range(0,2);
            if (pDescr[nLine].szOption1[nArg][0] == '\0')
                continue;
            strcpy (szArg1, pDescr[nLine].szOption1[nArg]);
            break;
        }
        while (pDescr[nLine].szOption2[0][0] != '\0') {
            nArg = number_range(0,2);
            if (pDescr[nLine].szOption2[nArg][0] == '\0')
                continue;
            strcpy (szArg2, pDescr[nLine].szOption2[nArg]);
            break;
        }
        while (pDescr[nLine].szOption3[0][0] != '\0') {
            nArg = number_range(0,2);
            if (pDescr[nLine].szOption3[nArg][0] == '\0')
                continue;
            strcpy (szArg3, pDescr[nLine].szOption3[nArg]);
            break;
        }
        if (szArg1[0] == '\0')
            sprintf (szBuf+strlen(szBuf), pDescr[nLine].szLine);
        else if (szArg2[0] == '\0')
            sprintf (szBuf+strlen(szBuf), pDescr[nLine].szLine, szArg1);
        else if (szArg3[0] == '\0')
            sprintf (szBuf+strlen(szBuf), pDescr[nLine].szLine, szArg1, szArg2);
        else
            sprintf (szBuf+strlen(szBuf), pDescr[nLine].szLine, szArg1, szArg2, szArg3);
    }
    return (format_string(str_dup(szBuf)));
}

int GetRandMobVnum (int nDiffLow, int nDiffHigh) {
    CHAR_DATA *target = NULL;
    CHAR_DATA *pCreated = NULL;
    MOB_INDEX_DATA *pIndex;
    int nVnum, nCount = 0;
    MOB_INDEX_DATA *pList[250];

    // Populate the list
    for (nVnum = 0; nVnum < top_vnum_mob; ++nVnum) {
        if ((pIndex = get_mob_index (nVnum)) == NULL)
            continue;
        if (!pIndex->pCreated) // No mobs of this vnum loaded
            pCreated = create_mobile (pIndex);
        target = pIndex->pCreated;
        if (//IS_NPC(target)AL(target) || (ch->alignment > 0 ? IS_EVIL(target) : IS_GOOD(target)))
            target->nDifficulty >= nDiffLow
            && target->nDifficulty <= nDiffHigh
            && !IS_SET(target->act, ACT_IS_HEALER)
            && !IS_SET(target->act, ACT_IS_CHANGER)
            && !IS_SET(target->imm_flags, IMM_SUMMON)
            && !IS_SET(target->affected_by, AFF_CHARM)
            && target->pIndexData->mprogs == NULL) {
            if (nCount >= 250)
                pList[number_range(0,249)] = pIndex;
            else
                pList[nCount++] = pIndex;
        }
        if (pCreated) {
            extract_char (pCreated, TRUE);
            pCreated = NULL;
        }
    }
    if (nCount == 0)
        return (0);
    pIndex = pList[number_range(0, nCount - 1)]; // Random index
    return (pIndex->vnum);
}

void CreateMap (int nMapVnum, MapData *pMap) {
    int nXLower, nXUpper, nYLower, nYUpper, x, y;
    char buf[MAX_STRING_LENGTH];
    RoomData *pCur;
    OBJ_INDEX_DATA *pMapObj;
    bool bBreak;

    pMapObj = get_obj_index(nMapVnum);
    // Trim it down
    bBreak = FALSE;
    for (nYLower = 0; nYLower < GRIDSIZE; ++nYLower) {
        for (x = 0; x < GRIDSIZE; ++x) {
            if (pMap->pRoomGrid[x][nYLower] != NULL) {
                bBreak = TRUE;
                break;
            }
        }
        if (bBreak)
            break;
    }
    bBreak = FALSE;
    for (nYUpper = GRIDSIZE-1; nYUpper >= 0; --nYUpper) {
        for (x = 0; x < GRIDSIZE; ++x) {
            if (pMap->pRoomGrid[x][nYUpper] != NULL) {
                bBreak = TRUE;
                break;
            }
        }
        if (bBreak)
            break;
    }
    bBreak = FALSE;
    for (nXLower = 0; nXLower < GRIDSIZE; ++nXLower) {
        for (y = 0; y < GRIDSIZE; ++y) {
            if (pMap->pRoomGrid[nXLower][y] != NULL) {
                bBreak = TRUE;
                break;
            }
        }
        if (bBreak)
            break;
    }
    bBreak = FALSE;
    for (nXUpper = GRIDSIZE-1; nXUpper >= 0; --nXUpper) {
        for (y = 0; y < GRIDSIZE; ++y) {
            if (pMap->pRoomGrid[nXUpper][y] != NULL) {
                bBreak = TRUE;
                break;
            }
        }
        if (bBreak)
            break;
    }

    strcpy (buf, "{x");
    for (y = nYLower; y <= nYUpper; ++y) {
        for (x = nXLower; x <= nXUpper; ++x) {
            if ((pCur = pMap->pRoomGrid[x][y]) == NULL)
                strcat (buf, "  ");
            else if (pCur->pExit[DIR_NORTH])
                strcat (buf, " |");
            else
                strcat (buf, "  ");

        }
        strcat (buf, "\n\r");
        for (x = nXLower; x <= nXUpper; ++x) {
            if ((pCur = pMap->pRoomGrid[x][y]) == NULL)
                strcat (buf, "  ");
            else if (pCur->pExit[DIR_WEST])
                strcat (buf, (x == GRIDSIZE/2 && y == GRIDSIZE/2) ? "-{RC{x" : "-O");
            else
                strcat (buf, (x == GRIDSIZE/2 && y == GRIDSIZE/2) ? " {RC{x" : " O");

        }
        strcat (buf, "\n\r");
    }
    free_string (pMapObj->description);
    pMapObj->description = str_dup (buf);
}

// Tries to add a room from a specified room out in a direction
RoomData *Dig (RoomData *pRoom, int nDir, int nState, MapData *pMap) {
    RoomData *pDest;
    // Check if we go off the bounds of the grid
    if (pRoom->nX + nExitDirEW[nDir] <  0 ||
        pRoom->nX + nExitDirEW[nDir] > GRIDSIZE - 1)
        return NULL;
    else if (pRoom->nY + nExitDirNS[nDir] < 0 ||
        pRoom->nY + nExitDirNS[nDir] > GRIDSIZE - 1)
        return NULL;
    if ((pDest = pMap->pRoomGrid[pRoom->nX + nExitDirEW[nDir]][pRoom->nY + nExitDirNS[nDir]])) {
        // Connect anyways?
        if (number_chance(50))
            return NULL;
        pRoom->pExit[nDir] = pDest;
        pDest->pExit[nOppDir[nDir]] = pRoom;
    }
    else {
        pDest = AddRoom (pRoom->nX + nExitDirEW[nDir], pRoom->nY + nExitDirNS[nDir], pMap);
        pDest->nState = nState;
        pDest->nMobDiff = pRoom->nMobDiff + (number_range(1,5) == 1 ? 2 : 0);
        // Link exits
        pRoom->pExit[nDir] = pDest;
        pDest->pExit[nOppDir[nDir]] = pRoom;
        // Add to the list of rooms to process
        if (pMap->pProcessLast != NULL)
            pMap->pProcessLast->pNextProcess = pDest;
        else
            pMap->pProcess = pDest;
        pMap->pProcessLast = pDest;
    }
    return pDest;
}

#define CHANCE_FUZZ (map.nRoomCount < 25 ? 3 : 1)
//((nMaxRooms - 2*map.nRoomCount) / 10)

ShapeData dungeon = {15, 15, 35,   75,  0};
ShapeData forest =  {1,   1,  0,  100, 10};

AREA_DATA *GenerateRandArea (int nVnumLow, int nDiffLow, int nDiffHigh) {
    MapData map;
    //RoomData *pRoomGrid[GRIDSIZE][GRIDSIZE];
    //RoomData *pTopRoom = NULL;
    //int nRoomCount = 0;
    AREA_DATA *pArea;
    RoomData *pCur, *pNext;//, *pProcess, *pProcessLast;
    int nMaxRooms = number_range(1000, 1999), nExit, x, y;
    ShapeData *pExitChance = &forest;

    map.nRoomCount = 0;
    map.pTopRoom = NULL;
    // Clear out the grid
    for (y = 0; y < GRIDSIZE; ++y)
        for (x = 0; x < GRIDSIZE; ++x)
            map.pRoomGrid[x][y] = NULL;
    // Start with a single room
    map.pProcess = AddRoom(GRIDSIZE/2,GRIDSIZE/2, &map);
    do
        map.pProcess->nState = number_range(FIRST_STATE, LAST_STATE);
    while (map.pProcess->nState == ROOM);
    map.pProcess->nMobDiff = 10;
    map.pProcessLast = map.pProcess;
    // Now go through the process list
    while ((pCur = map.pProcess) && map.nRoomCount < nMaxRooms) {
        map.pProcess = map.pProcess->pNextProcess;
        if (map.pProcess == NULL)
            map.pProcessLast = NULL;

        // have arrays with different sets of values for each chance,
        // depending on land type
        if (pCur->nState == HALL_N || pCur->nState == HALL_NS) {
            if (number_chance(pExitChance->nFourway*CHANCE_FUZZ))
                Dig (pCur, DIR_NORTH, HALL_IN, &map);
            else if (number_chance(pExitChance->nIntersection*CHANCE_FUZZ))
                Dig (pCur, DIR_NORTH, HALL_EW, &map);
            else if (number_chance(pExitChance->nRoom) && map.nRoomCount >= 25)
                Dig (pCur, DIR_NORTH, ROOM, &map);
            else if (number_chance(pExitChance->nWinding*CHANCE_FUZZ))
                Dig (pCur, number_chance(50) ? DIR_WEST : DIR_EAST, HALL_N, &map);
            else if (number_chance(pExitChance->nContinue*CHANCE_FUZZ))
                Dig (pCur, DIR_NORTH, HALL_NS, &map);
        }
        if (pCur->nState == HALL_E || pCur->nState == HALL_EW) {
            if (number_chance(pExitChance->nFourway*CHANCE_FUZZ))
                Dig (pCur, DIR_EAST, HALL_IN, &map);
            else if (number_chance(pExitChance->nIntersection*CHANCE_FUZZ))
                Dig (pCur, DIR_EAST, HALL_NS, &map);
            else if (number_chance(pExitChance->nRoom) && map.nRoomCount >= 25)
                Dig (pCur, DIR_EAST, ROOM, &map);
            else if (number_chance(pExitChance->nWinding*CHANCE_FUZZ))
                Dig (pCur, number_chance(50) ? DIR_NORTH : DIR_SOUTH, HALL_E, &map);
            else if (number_chance(pExitChance->nContinue*CHANCE_FUZZ))
                Dig (pCur, DIR_EAST, HALL_EW, &map);
        }
        if (pCur->nState == HALL_S || pCur->nState == HALL_NS) {
            if (number_chance(pExitChance->nFourway*CHANCE_FUZZ))
                Dig (pCur, DIR_SOUTH, HALL_IN, &map);
            else if (number_chance(pExitChance->nIntersection*CHANCE_FUZZ))
                Dig (pCur, DIR_SOUTH, HALL_EW, &map);
            else if (number_chance(pExitChance->nRoom) && map.nRoomCount >= 25)
                Dig (pCur, DIR_SOUTH, ROOM, &map);
            else if (number_chance(pExitChance->nWinding*CHANCE_FUZZ))
                Dig (pCur, number_chance(50) ? DIR_WEST : DIR_EAST, HALL_S, &map);
            else if (number_chance(pExitChance->nContinue*CHANCE_FUZZ))
                Dig (pCur, DIR_SOUTH, HALL_NS, &map);
        }
        if (pCur->nState == HALL_W || pCur->nState == HALL_EW) {
            if (number_chance(pExitChance->nFourway*CHANCE_FUZZ))
                Dig (pCur, DIR_WEST, HALL_IN, &map);
            else if (number_chance(pExitChance->nIntersection*CHANCE_FUZZ))
                Dig (pCur, DIR_WEST, HALL_NS, &map);
            else if (number_chance(pExitChance->nRoom) && map.nRoomCount >= 25)
                Dig (pCur, DIR_WEST, ROOM, &map);
            else if (number_chance(pExitChance->nWinding*CHANCE_FUZZ))
                Dig (pCur, number_chance(50) ? DIR_NORTH : DIR_SOUTH, HALL_W, &map);
            else if (number_chance(pExitChance->nContinue*CHANCE_FUZZ))
                Dig (pCur, DIR_WEST, HALL_EW, &map);
        }
        if (pCur->nState == HALL_IN) {
            Dig (pCur, DIR_NORTH, HALL_NS, &map);
            Dig (pCur, DIR_SOUTH, HALL_NS, &map);
            Dig (pCur, DIR_EAST, HALL_EW, &map);
            Dig (pCur, DIR_WEST, HALL_EW, &map);
        }
           /*
            case ROOM:
                if (number_chance(20*CHANCE_FUZZ))
                    Dig (pCur, DIR_NORTH, number_chance(25) ? ROOM : HALL_NS, &map);
                if (number_chance(20*CHANCE_FUZZ))
                    Dig (pCur, DIR_SOUTH, number_chance(25) ? ROOM : HALL_NS, &map);
                if (number_chance(20*CHANCE_FUZZ))
                    Dig (pCur, DIR_EAST, number_chance(25) ? ROOM : HALL_EW, &map);
                if (number_chance(20*CHANCE_FUZZ))
                    Dig (pCur, DIR_WEST, number_chance(25) ? ROOM : HALL_EW, &map);
                break;*/
    }

    // Now convert this prototype to an actual in-game area
    {
        ROOM_INDEX_DATA *pRoom;
        ROOM_INDEX_DATA *toRoom;
        EXIT_DATA *pExit;
        RESET_DATA *pReset;
        int iHash, i, nMobCount;
        //int nMob1 = 0, nMob2 = 0, nMob3 = 0;
        int nMobVnumList[15][3];

        for (i = 0; i < 15; ++i) {
            nMobVnumList[i][1] = GetRandMobVnum(i * 150, (i+1) * 150);
            nMobVnumList[i][2] = GetRandMobVnum(i * 150, (i+1) * 150);
            nMobVnumList[i][3] = GetRandMobVnum(i * 150, (i+1) * 150);
        }

        // Pick three different mobs to populate the area with
        //nMob1 = GetRandMobVnum(nDiffLow,nDiffHigh);
        //nMob2 = GetRandMobVnum(nDiffLow,nDiffHigh);
        //nMob3 = GetRandMobVnum(nDiffLow,nDiffHigh);

        pArea = new_area ();
        area_last->next = pArea;
        area_last = pArea;
        pArea->name = str_dup ("Random Realm");
        pArea->credits = str_dup ("The cOmPuTeR");
        pArea->file_name = str_dup ("rand0000.are");
        pArea->area_flags = AREA_DONTSAVE;
        pArea->min_vnum = nVnumLow;
        pArea->max_vnum = nVnumLow+map.nRoomCount;
        // Now create rooms
        for (pCur = map.pTopRoom; pCur; pCur = pCur->pNext) {
            pRoom = new_room_index ();
            pRoom->area = pArea;
            pRoom->vnum = nVnumLow + pCur->nNumber;
            pRoom->description = GenRoomDescr(ForestDescr);
            pCur->pRoomIndex = pRoom; // For ease of linkage

            if (pRoom->vnum > top_vnum_room)
                top_vnum_room = pRoom->vnum;

            iHash = pRoom->vnum % MAX_KEY_HASH;
            pRoom->next = room_index_hash[iHash];
            room_index_hash[iHash] = pRoom;

            // Add a mob reset
            if (nMobVnumList[UMIN(15,pCur->nMobDiff/150)][1] != 0) {
                nMobCount = number_range(1, 25);
                if (nMobCount == 25)
                    nMobCount = 5;
                else if (nMobCount >= 23)
                    nMobCount = 4;
                else if (nMobCount >= 21)
                    nMobCount = 3;
                else if (nMobCount >= 16)
                    nMobCount = 2;
                else
                    nMobCount = 1;
                for (i = 0; i < nMobCount; ++i) {
                    pReset = new_reset_data ();
                    pReset->command = 'M';
                    pReset->arg1 = number_range(1,10);
                    if (pReset->arg1 == 10 && nMobVnumList[UMIN(15,pCur->nMobDiff/150)][3] != 0)
                        pReset->arg1 = nMobVnumList[UMIN(15,pCur->nMobDiff/150)][3];
                    else if (pReset->arg1 >= 7 && nMobVnumList[UMIN(15,pCur->nMobDiff/150)][2] != 0)
                        pReset->arg1 = nMobVnumList[UMIN(15,pCur->nMobDiff/150)][2];
                    else
                        pReset->arg1 = nMobVnumList[UMIN(15,pCur->nMobDiff/150)][1];
                    pReset->arg2 = 100;
                    pReset->arg3 = pRoom->vnum;
                    pReset->arg4 = 3; // number in room
                    add_reset (pRoom, pReset, 0);
                }
            }
        }
        // Link the rooms
        for (pCur = map.pTopRoom; pCur; pCur = pCur->pNext) {
            pRoom = pCur->pRoomIndex;
            for (nExit = 0; nExit < 6; ++nExit) {
                // No exit
                if (pCur->pExit[nExit] == NULL)
                    continue;
                if ((toRoom = pCur->pExit[nExit]->pRoomIndex) == NULL)
                    continue;

                // Other side's exit already exists
                if (toRoom->exit[nOppDir[nExit]])
                    continue;

                if (!pRoom->exit[nExit])
                    pRoom->exit[nExit] = new_exit ();

                pRoom->exit[nExit]->u1.to_room = toRoom;
                pRoom->exit[nExit]->orig_door = nExit;

                nExit = nOppDir[nExit];
                pExit = new_exit ();
                pExit->u1.to_room = pRoom;
                pExit->orig_door = nExit;
                toRoom->exit[nExit] = pExit;
            }
        }
    }

    // Create some standard items
    {
        OBJ_INDEX_DATA *pObj;
        RESET_DATA *pReset;
        int iHash;

        // The Map
        pObj = new_obj_index ();
        iHash = nVnumLow % MAX_KEY_HASH;
        pObj->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObj;
        pObj->vnum = nVnumLow;
        pObj->area = pArea;
        pObj->reset_num = 0;
        pObj->name = str_dup("map");
        pObj->short_descr = str_dup("A Map");
        pObj->description = str_dup("This is the map");
        pObj->material = str_dup("paper");
		pObj->durability = 1;
        pObj->item_type = ITEM_TRASH;
        pObj->extra_flags = 0;
        pObj->wear_flags = ITEM_TAKE|ITEM_HOLD;
        pObj->value[0] = 0;
        pObj->value[1] = 0;
        pObj->value[2] = 0;
        pObj->value[3] = 0;
        pObj->value[4] = 0;
        pObj->llPl = 1;
        pObj->weight = 1;
        pObj->cost = 1;
        CreateMap (nVnumLow, &map);

        // The Portal Out
        pObj = new_obj_index ();
        iHash = (nVnumLow+1) % MAX_KEY_HASH;
        pObj->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObj;
        pObj->vnum = nVnumLow+1;
        pObj->area = pArea;
        pObj->reset_num = 0;
        pObj->name = str_dup("rift");
        pObj->short_descr = str_dup("The Rift");
        pObj->description = str_dup("The Rift");
        pObj->material = str_dup("unknown");
		pObj->durability = 1;
        pObj->item_type = ITEM_PORTAL;
        pObj->extra_flags = ITEM_NOPURGE;
        pObj->wear_flags = 0;
        pObj->value[0] = 0;
        pObj->value[1] = 0;
        pObj->value[2] = 0;
        pObj->value[3] = 201;
        pObj->value[4] = 0;
        pObj->llPl = 1;
        pObj->weight = 1;
        pObj->cost = 1;
        // Add to the starting room
        pReset = new_reset_data ();
        pReset->command = 'O';
        pReset->arg1 = nVnumLow+1;
        pReset->arg2 = 0;
        pReset->arg3 = nVnumLow;
        pReset->arg4 = 0;
        add_reset (get_room_index(nVnumLow), pReset, 0);

        if (nVnumLow+1 > top_vnum_obj)
            top_vnum_obj = nVnumLow+1;
        newobjs += 2;
    }

    // Delete the prototype rooms
    for (pCur = map.pTopRoom; pCur; pCur = pNext) {
        pNext = pCur->pNext;
        free_room (pCur);
    }
    map.pTopRoom = NULL;
    for (y = 0; y < GRIDSIZE; ++y)
        for (x = 0; x < GRIDSIZE; ++x)
            map.pRoomGrid[x][y] = NULL;
    map.nRoomCount = 0;
    return (pArea);
}

void GenerateVoid () {
    int i, nLastVnum = VNUM_RANDOM_LOW-1;
    AREA_DATA *pArea, *pLastArea = NULL;
    ROOM_INDEX_DATA *pRoom1, *pRoom2;
    for (i = 0; i < 1; ++i) {
        pArea = GenerateRandArea (nLastVnum+1, i * 150, (i+1) * 150);

        // Link the two areas
        if (pLastArea != NULL) {
            // Pick one in the last half of the vnums
            pRoom1 = get_room_index(number_range((pArea->max_vnum - pArea->min_vnum) / 2 + pArea->min_vnum, pArea->max_vnum));
            pRoom2 = get_room_index(number_range((pLastArea->max_vnum - pLastArea->min_vnum) / 2 + pLastArea->min_vnum, pLastArea->max_vnum));

            // Exit up
            pRoom1->exit[4] = new_exit ();
            pRoom1->exit[4]->u1.to_room = pRoom2;
            pRoom1->exit[4]->orig_door = 4;

            // Exit down
            pRoom2->exit[5] = new_exit ();
            pRoom2->exit[5]->u1.to_room = pRoom1;
            pRoom2->exit[5]->orig_door = 5;
        }
        reset_area (pArea);
        nLastVnum = pArea->max_vnum;
        pLastArea = pArea;
    }
}




// Base weapon types
BaseWeaponData BaseWpnTable[BASEITEM_COUNT] = {
// {"name list",        "name",       "on ground name",              "material", weight, cost, class,          low_dam, high_dam, dam_type
 {"dagger",             "dagger",     "A dagger sits here.",           "steel",    20,      1,   WEAPON_DAGGER,  1,       6,        "pierce"},
 {"shortsword sword",   "shortsword", "A shortsword sits here.",       "steel",    50,      4,   WEAPON_SWORD,   1,       8,        "stab"},
 {"longsword sword",    "longsword",  "A longsword sits here.",        "steel",    125,     10,  WEAPON_SWORD,   1,       12,       "slash"},
 {"katana",             "katana",     "A katana collects dust.",       "steel",    150,     85,  WEAPON_SWORD,   2,       14,       "cleave"},
 {"club",               "club",       "A primitive club lies around.", "wood",     30,      1,   WEAPON_MACE,    1,       8,        "crush"},
 {"mace",               "mace",       "A mace is here.",               "iron",     100,     10,  WEAPON_MACE,    1,       12,       "smash"},
 {"flail",              "flail",      "A flail lies in the dirt.",     "steel",    150,     15,  WEAPON_FLAIL,   3,       12,       "twack"},
 {"spear",              "spear",      "A spear sits here.",            "steel",    150,     4,   WEAPON_SPEAR,   1,       10,       "pierce"},
 {"staff",              "staff",      "A staff lies around.",          "wood",     100,     1,   WEAPON_SPEAR,   1,       8,        "pound"},
 {"handaxe axe",        "handaxe",    "A handaxe lies around",         "iron",     50,      3,   WEAPON_AXE,     1,       8,        "chop"},
 {"battleaxe axe",      "battleaxe",  "A battleaxe lies in the dirt.", "steel",    125,     15,  WEAPON_AXE,     1,       13,       "chop"},
 {"irgaak",             "irgaak",     "An irgaak sits here.",          "steel",    300,     100, WEAPON_POLEARM, 2,       20,       "slice"}
};

// Weapon prefixes.
// Sort by level, then rarity -- this is mandatory
WpnEffectData WpnPreTable[WPNPRE_COUNT] = {
// {"name",     "world list",  level, rarity, {{location, add/mult, nMod1, nMod2} ...} }
 {"",           "",              0,   NORMAL, {{LOC_NONE,   ADD,  0, 0, 0, 0},              {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"sharp",      "sharp",         1,   NORMAL, {{LOC_DAM,    ADD,  1, 0, 0, 0},              {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"expensive",  "expensive",     1,   NORMAL, {{LOC_COST,   MULT, 5, 0, 0, 0},              {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"stone",      "stone",         1,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 1, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 1, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"massive",    "massive",       1,   RARE,   {{LOC_DAM,    ADD,  2, 0, 0, 0},              {LOC_WEIGHT, MULT,  3, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"keen",       "keen",          5,   NORMAL, {{LOC_DAM,    ADD,  2, 0, 0, 0},              {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"bronze",     "bronze",       11,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 2, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 2, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"blessed",    "blessed",      10,   RARE,   {{LOC_EXTRA,  NONE, ITEM_BLESS, 0, 0, 0},     {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"iron",       "iron",         21,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 3, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 3, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"steel",      "steel",        31,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 4, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 4, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"swift",      "swift",        35,   RARE,   {{LOC_AFFECT, NONE, 0, 0, -1, AFF_HASTE},     {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"silver",     "silver",       41,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 5, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 5, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"gold",       "gold",         51,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 6, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 6, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"seeking",    "seeking",      50,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 10, 0, 0}, {LOC_NONE,   NONE,  0, 0, 0, 0},             {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"adamantium", "adamantium",   61,   NORMAL, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 7, 0, 0},  {LOC_OBJAFF, NONE, APPLY_DAMROLL, 7, 0, 0},  {LOC_NONE,   NONE, 0, 0, 0, 0},          {LOC_NONE,  NONE, 0, 0, 0, 0}}},
 {"Heightened", "heightened",  100,   UNIQUE, {{LOC_OBJAFF, NONE, APPLY_HITROLL, 25, 0, 0}, {LOC_OBJAFF, NONE, APPLY_DAMROLL, 25, 0, 0}, {LOC_AFFECT, NONE, 0, 0, -1, AFF_HASTE}, {LOC_EXTRA, NONE, ITEM_BLESS|ITEM_HUM, 0, 0, 0}, }}
};
// Weapon suffixes.
// Also sort by level then rarity (has to be this way)
WpnEffectData WpnSufTable[WPNSUF_COUNT] = {
 {"",                 "",              0, NORMAL, {{LOC_NONE,   NONE, 0, 0, 0, 0},         {LOC_NONE,   NONE,   0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}},
 {"of sharpness",     "sharpness",     1, NORMAL, {{LOC_DAM,    ADD,  1, 0, 0, 0},         {LOC_NONE,   NONE,   0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}},
 {"of expensiveness", "expensiveness", 1, NORMAL, {{LOC_COST,   MULT, 5, 0, 0, 0},         {LOC_NONE,   NONE,   0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}},
 {"of massiveness",   "massiveness",   1, RARE,   {{LOC_DAM,    ADD,  2, 0, 0, 0},         {LOC_WEIGHT, MULT,   3, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}},
 {"of humming",       "humming",       3, RARE,   {{LOC_EXTRA,  NONE, ITEM_HUM, 0, 0, 0},  {LOC_COST,   ADD,  150, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}},
 {"of glowing",       "glowing",       3, RARE,   {{LOC_EXTRA,  NONE, ITEM_GLOW, 0, 0, 0}, {LOC_COST,   ADD,  150, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}},
 {"of energy",        "energy",       10, NORMAL, {{LOC_OBJAFF, NONE, APPLY_KI, 50, 0, 0}, {LOC_NONE,   NONE,   0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}, {LOC_NONE, NONE, 0, 0, 0, 0}}}
};

void AddWpnSpec (OBJ_DATA *pObj, WpnEffectData *pEffect, WpnSpecData *pSpec) {
    AFFECT_DATA *pAf;
    switch (pSpec->nLoc) {
        case LOC_EXTRA:
            pObj->extra_flags |= pSpec->nMod1;
            break;
        case LOC_DAM:
            if (pSpec->nOperation == ADD) {
                pObj->value[1] += pSpec->nMod1;
                pObj->value[2] += pSpec->nMod1;
            }
            else {
                pObj->value[1] *= pSpec->nMod1;
                pObj->value[2] *= pSpec->nMod1;
            }
            break;
        case LOC_COST:
            if (pSpec->nOperation == ADD)
                pObj->cost += pSpec->nMod1;
            else
                pObj->cost *= pSpec->nMod1;
            break;
        case LOC_WEIGHT:
            if (pSpec->nOperation == ADD)
                pObj->weight += pSpec->nMod1;
            else
                pObj->weight *= pSpec->nMod1;
            break;
        case LOC_DURAB:
            if (pSpec->nOperation == ADD)
                pObj->durability += pSpec->nMod1;
            else
                pObj->durability *= pSpec->nMod1;
            break;
        case LOC_OBJAFF:
            pAf = new_affect();
            pAf->location = pSpec->nMod1;
            pAf->modifier = pSpec->nMod2;
            pAf->where = TO_OBJECT;
            pAf->type = pSpec->nMod3;
            pAf->duration = -1;
            pAf->bitvector = pSpec->nMod4;
            pAf->skill_lvl = pEffect->nLevel;
            pAf->next = pObj->affected;
            pObj->affected = pAf;
            break;
        case LOC_WPNAFF:
            pAf = new_affect();
            pAf->location = pSpec->nMod1;
            pAf->modifier = pSpec->nMod2;
            pAf->where = TO_WEAPON;
            pAf->type = pSpec->nMod3;
            pAf->duration = -1;
            pAf->bitvector = pSpec->nMod4;
            pAf->skill_lvl = pEffect->nLevel;
            pAf->next = pObj->affected;
            pObj->affected = pAf;
            break;
        case LOC_AFFECT:
            pAf = new_affect();
            pAf->location = pSpec->nMod1;
            pAf->modifier = pSpec->nMod2;
            pAf->where = TO_AFFECTS;
            pAf->type = pSpec->nMod3;
            pAf->duration = -1;
            pAf->bitvector = pSpec->nMod4;
            pAf->skill_lvl = pEffect->nLevel;
            pAf->next = pObj->affected;
            pObj->affected = pAf;
            break;
    }
}


OBJ_DATA *CreateRandWeapon (int nLevel) {
    OBJ_DATA *pObj;
    int nWpn;
    int i, nPrefix;
    int nSuffix;
    int nPreCount = 0, nSufCount = 0;
    char szBuf[MAX_STRING_LENGTH], cLetter;
    bool bPrefix, bSuffix, bVowel;

    nWpn = number_range(1, BASEITEM_COUNT) - 1;

    if (get_obj_index(OBJ_VNUM_RANDOM) == NULL) {
        logstr (LOG_BUG, "CreateRandWeapon: OBJ_VNUM_RANDOM not found");
        return NULL;
    }
    pObj = create_object(get_obj_index(OBJ_VNUM_RANDOM), 0);

    for (i = 0; i < WPNPRE_COUNT; ++i)
        if (nLevel - WpnPreTable[i].nLevel <= 10 &&
            nLevel - WpnPreTable[i].nLevel >= -10)
            ++nPreCount;
    if (nPreCount == 0)
        nPrefix = 0;
    else {
        while (TRUE) {
            nPrefix = number_range(0, WPNPRE_COUNT-1);
            if (nLevel - WpnPreTable[nPrefix].nLevel <= 10 &&
                nLevel - WpnPreTable[nPrefix].nLevel >= -10) {
                if (WpnPreTable[nPrefix].nRarity == NORMAL && number_range (1, 4) < 4)
                    break;
                else if (WpnPreTable[nPrefix].nRarity == RARE && number_range (1, 5) == 1)
                    break;
                else if (WpnPreTable[nPrefix].nRarity == UNIQUE && number_range (1, 20) == 1)
                    break;
            }
        }
    }
    for (i = 0; i < WPNSUF_COUNT; ++i)
        if (nLevel - WpnSufTable[i].nLevel <= 10 &&
            nLevel - WpnSufTable[i].nLevel >= -10)
            ++nSufCount;
    if (nSufCount == 0)
        nSuffix = 0;
    else {
        while (TRUE) {
            nSuffix = number_range(0, WPNSUF_COUNT-1);
            if (nLevel - WpnSufTable[nSuffix].nLevel <= 10 &&
                nLevel - WpnSufTable[nSuffix].nLevel >= -10) {
                if (WpnSufTable[nSuffix].nRarity == NORMAL && number_range (1, 4) < 4)
                    break;
                else if (WpnSufTable[nSuffix].nRarity == RARE && number_range (1, 5) == 1)
                    break;
                else if (WpnSufTable[nSuffix].nRarity == UNIQUE && number_range (1, 20) == 1)
                    break;
            }
        }
    }

    // Create the right name
    cLetter = WpnPreTable[nPrefix].szName[0];
    if (cLetter == '\0') {
        bPrefix = FALSE;
        cLetter = BaseWpnTable[nWpn].szBaseName[0];
    }
    else
        bPrefix = TRUE;
    if (WpnSufTable[nSuffix].szName[0] == '\0')
        bSuffix = FALSE;
    else
        bSuffix = TRUE;
    if (cLetter == 'a' || cLetter == 'e' || cLetter == 'i' || cLetter == 'o' || cLetter == 'u')
        bVowel = TRUE;
    else
        bVowel = FALSE;
    sprintf (szBuf, "a%s %s%s%s%s%s",
             bVowel ? "n" : "", WpnPreTable[nPrefix].szName, bPrefix ? " " : "", BaseWpnTable[nWpn].szBaseName,
             bSuffix ? " " : "", WpnSufTable[nSuffix].szName);
    pObj->short_descr = str_dup(szBuf);
    
    strcpy (szBuf, BaseWpnTable[nWpn].szName);
    if (WpnPreTable[nPrefix].szWordList[0] != '\0') {
        strcat (szBuf, " ");
        strcat (szBuf, WpnPreTable[nPrefix].szWordList);
    }
    if (WpnSufTable[nSuffix].szWordList[0] != '\0') {
        strcat (szBuf, " ");
        strcat (szBuf, WpnSufTable[nSuffix].szWordList);
    }
    pObj->name = str_dup(szBuf);

    pObj->item_type = ITEM_WEAPON;
    pObj->wear_flags = ITEM_TAKE|ITEM_WIELD;
    pObj->extra_flags = ITEM_RANDOM;
    pObj->description = str_dup(BaseWpnTable[nWpn].szLong);
    pObj->material = str_dup(BaseWpnTable[nWpn].szMaterial);
    pObj->weight = BaseWpnTable[nWpn].nWeight;
    pObj->cost = BaseWpnTable[nWpn].nCost;
    pObj->value[0]= BaseWpnTable[nWpn].nClass;
    pObj->value[1] = BaseWpnTable[nWpn].nLowDam;
    pObj->value[2] = BaseWpnTable[nWpn].nHighDam;
    pObj->value[3] = attack_lookup(BaseWpnTable[nWpn].szDamType);
    pObj->value[4] = 0;

    for (i = 0; i < 4; ++i) {
        AddWpnSpec (pObj, &WpnPreTable[nPrefix], &WpnPreTable[nPrefix].spec[i]);
        AddWpnSpec (pObj, &WpnSufTable[nSuffix], &WpnSufTable[nSuffix].spec[i]);
    }

    return pObj;
}



