/*
 * lib/inttypes.h
 * © suhas pai
 */

#pragma once

#define PRId8 "d"
#define PRIu8 "u"
#define PRIx8 "x"
#define PRIX8 "X"

#define PRId16 "d"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"

#define PRId32 "d"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"

#define __PRI64_LEN_MODIFIER__ "l"

#define PRId64 __PRI64_LEN_MODIFIER__ "d"
#define PRIu64 __PRI64_LEN_MODIFIER__ "u"
#define PRIx64 __PRI64_LEN_MODIFIER__ "x"
#define PRIX64 __PRI64_LEN_MODIFIER__ "X"