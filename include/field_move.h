#ifndef GUARD_FIELD_MOVE_H
#define GUARD_FIELD_MOVE_H

#include "global.h"
#include "constants/field_move.h"

struct FieldMoveUnlock
{
   bool32 (*isUnlockedFunc)(enum FieldMove);
   const u8 *lockedMessage;
};

enum FieldMoveUnlockType
{
    CANT_UNLOCK,
    ALWAYS_UNLOCKED,
    BADGE_UNLOCK,
    FIELD_MOVE_UNLOCK_COUNT
};

struct FieldMoveInfo
{
    bool32 (*fieldMoveFunc)(void);
    enum FieldMoveUnlockType unlockType:3;
    enum Move moveID:11;
    u32 partyMsgID:7;
    u32 arg:8;
    u32 hideIfLocked:1;
    u32 hmFree:1;       // docs/SPEC.md "HM-free traversal": a required field
                        // interaction that HM-Free Traversal frees from the
                        // "a party member must know the move" requirement.
    u32 padding:2;
};

extern const struct FieldMoveInfo gFieldMoveInfo[];
extern const struct FieldMoveUnlock gFieldMoveUnlocks[];

// docs/SPEC.md "HM-free traversal". When SETTING_HM_FREE_TRAVERSAL is on, a
// required field interaction only needs story/badge permission, not a party
// member that knows the move.
bool32 FieldMove_HmFreeOn(void);
bool32 FieldMove_IsHmFree(enum FieldMove fieldMove);
// Party slot to show performing fieldMove: the first mon that knows it, else
// (HM-free only) a sensible stand-in, else PARTY_SIZE.
u32 FieldMove_GetUserSlot(enum FieldMove fieldMove);
// Whether Fly may be triggered from the Town Map / region map (OW_FLAG_POKE_RIDER
// or HM-free with the Fly badge).
bool32 FieldMove_PokeRiderEnabled(void);

static inline bool32 SetUpFieldMove(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].fieldMoveFunc();
}

static inline bool32 IsFieldMoveUnlocked(enum FieldMove fieldMove)
{
    return gFieldMoveUnlocks[gFieldMoveInfo[fieldMove].unlockType].isUnlockedFunc(fieldMove);
}

static inline const u8 *FieldMove_GetLockedMessage(enum FieldMove fieldMove)
{
    return gFieldMoveUnlocks[gFieldMoveInfo[fieldMove].unlockType].lockedMessage;
}

static inline enum Move FieldMove_GetMoveId(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].moveID;
}

static inline u32 FieldMove_GetPartyMsgID(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].partyMsgID;
}

static inline bool32 FieldMove_IsVisible(enum FieldMove fieldMove)
{
    return !gFieldMoveInfo[fieldMove].hideIfLocked || IsFieldMoveUnlocked(fieldMove);
}

#endif //GUARD_FIELD_MOVE_H
