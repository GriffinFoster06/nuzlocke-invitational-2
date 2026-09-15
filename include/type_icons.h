#ifndef GUARD_TYPE_ICONS_H
#define GUARD_TYPE_ICONS_H

void LoadTypeIcons(enum BattlerId battler);

// ----------------------------------------------------------------------------
// Phase 11D (docs/SPEC.md "Type icons"): a non-battle, non-animated variant
// of the same compact icon sprites for static list/HUD contexts - Party, PC,
// Summary starter selection, and the persistent per-side battle HUD icon
// (distinct from LoadTypeIcons above, which is scoped to move-selection
// only). Shares gfx/palette loading and the OAM/anim template with the
// existing battle icons; the caller owns position/visibility and destroys
// the returned sprite id itself - no slide/hide/bounce behavior is attached.
// ----------------------------------------------------------------------------
void TypeIcons_LoadGraphics(void);
u8 CreateStaticTypeIconSprite(enum Type type, s16 x, s16 y, u8 subpriority);

// The already-tuned per-battler-position resting coordinates the existing
// move-selection icons animate to/from (sTypeIconPositions[position][isDoubles]).
// Phase 11D's persistent healthbox icon reuses these as its (fixed, relative
// to that battler's healthbox base position) placement so both features
// agree on where a battler's type icon(s) belong on screen.
extern const struct Coords16 sTypeIconPositions[][2];

// The illusion/Tera-aware "what type does this battler publicly show right
// now" lookup the move-selection icons already use - shared so the
// persistent healthbox icon never reveals an Illusion's real type or shows
// a stale pre-Tera type.
enum Type GetMonPublicType(enum BattlerId battlerId, u32 typeNum);

#define TYPE_ICON_TAG 0x2720
#define TYPE_ICON_TAG_2 0x2721
#define NUM_FRAMES_HIDE_TYPE_ICON 10

#define tMonPosition      data[0]
#define tBattlerId        data[1]
#define tHideIconTimer    data[2]
#define tVerticalPosition data[3]

#define TYPE_ICON_1_FRAME(monType) ((monType - 1) * 2)
#define TYPE_ICON_2_FRAME(monType) ((monType - 11) * 2)

#endif // GUARD_TYPE_ICONS_H
