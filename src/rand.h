
// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!

typedef struct MapStruct        MapData;
typedef struct ShapeStruct      ShapeData;
typedef struct RoomStruct       RoomData;
typedef struct DescrStruct      DescrData;
typedef struct BaseWeaponStruct BaseWeaponData;
typedef struct WpnSpecStruct    WpnSpecData;
typedef struct WpnEffectStruct  WpnEffectData;

#define GRIDSIZE       40

#define FIRST_STATE  1
#define LAST_STATE   8
#define HALL_EW 1
#define HALL_NS 2
#define HALL_IN 3 // intersection
#define ROOM    4
#define HALL_N  5
#define HALL_E  6
#define HALL_S  7
#define HALL_W  8

struct MapStruct {
    int       nRoomCount;
    RoomData *pTopRoom;
    RoomData *pProcess;
    RoomData *pProcessLast;
    RoomData *pRoomGrid[GRIDSIZE][GRIDSIZE];
};

struct ShapeStruct {
    int nFourway; // +
    int nIntersection; // T, off in the perpendicular direction
    int nRoom;
    int nContinue; // go in same direction
    int nWinding; // _______|---------|_____....minute changes in direction, perpendicular
};

struct RoomStruct {
    bool     valid;
    RoomData *pNext;
    RoomData *pNextProcess;
    RoomData *pExit[6];
    int       nX;
    int       nY;
    int       nNumber;
    int       nState;
    int       nMobDiff;
    ROOM_INDEX_DATA *pRoomIndex;
};

struct DescrStruct {
    int         nWeight; // 1-10, 10 being the most
    char       *szLine;
    char       *szOption1[3];
    char       *szOption2[3];
    char       *szOption3[3];
};

struct BaseWeaponStruct {
    char *szName;     // Word list for manipulating the object.
    char *szBaseName; // The base name before adding prefixes or suffixes
    char *szLong;     // Description when on the ground
    char *szMaterial;
    int   nWeight;
    int   nCost;
    int   nClass;
    int   nLowDam;
    int   nHighDam;
    char *szDamType;
};

struct WpnSpecStruct {
    int nLoc;
    int nOperation;
    int nMod1;
    int nMod2;
    int nMod3;
    int nMod4;
};

struct WpnEffectStruct {
    char       *szName;
    char       *szWordList; // Words to manipulate the object with
    int         nLevel;
    int         nRarity;
    WpnSpecData spec[4];
};

#define VNUM_RANDOM_LOW  25000
#define VNUM_RANDOM_HIGH 27000

#define BASEITEM_COUNT 12
#define WPNPRE_COUNT   16
#define WPNSUF_COUNT    7

#define NORMAL 1
#define RARE   2
#define UNIQUE 3

#define LOC_NONE   0
#define LOC_EXTRA  1 // Adds extra flags
#define LOC_DAM    2 // Adds to min, max damage
#define LOC_COST   3 // Alters price
#define LOC_WEIGHT 4 // Alters weight
#define LOC_DURAB  5 // Durability
#define LOC_OBJAFF 6 // nMod1: location, nMod2: modifier, nMod3: type (skill, -1 for none), nMod4: bitvector (obj extras, ie ITEM_*)
                     // Changes object flags.
#define LOC_WPNAFF 7 // nMod1: location, nMod2: modifier, nMod3: type (skill, -1 for none), nMod4: bitvector (weapon bits, ie WEAPON_*)
                     // Changes object flags.
#define LOC_AFFECT 8 // nMod1: location, nMod2: modifier, nMod3: type (skill, -1 for none), nMod4: bitvector (aff, ie AFF_*)
                     // Adds effects.

#define NONE 0
#define ADD  1
#define MULT 2

void            GenerateVoid          args ( ( ) );
OBJ_DATA       *CreateRandWeapon      args ( ( int nLevel ) );
