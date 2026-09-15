#ifndef GUARD_CONSTANTS_PROJECT_VERSION_H
#define GUARD_CONSTANTS_PROJECT_VERSION_H

// ============================================================================
// Phase 11E - this hack's own version identity, distinct from
// include/constants/expansion.h's upstream pokeemerald-expansion version.
// docs/SPEC.md "Terminal Run Reports" requires a project/ROM version field in
// every finalized report; nothing else in the tree names this hack's own
// version, so this is that identity. Bump PATCH for a released build that
// changes report-visible behavior; MAJOR/MINOR are free for the project to
// use however it likes (they are not consumed by any generation system).
// ============================================================================

#define PROJECT_VERSION_MAJOR 0
#define PROJECT_VERSION_MINOR 11
#define PROJECT_VERSION_PATCH 5

// Packed for compact storage in struct RunReportRecord: 4/6/6 bits (fits u16).
#define PROJECT_VERSION_ID \
    (((PROJECT_VERSION_MAJOR & 0xF) << 12) | ((PROJECT_VERSION_MINOR & 0x3F) << 6) | (PROJECT_VERSION_PATCH & 0x3F))

#endif // GUARD_CONSTANTS_PROJECT_VERSION_H
