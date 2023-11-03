/*
 * lib/inttypes.h
 * © suhas pai
 */

#pragma once

#define PRIb8 "b"
#define PRIB8 "B"
#define PRId8 "d"
#define PRIu8 "u"
#define PRIx8 "x"
#define PRIX8 "X"

#define PRIb16 "b"
#define PRIB16 "B"
#define PRId16 "d"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"

#define PRIb32 "b"
#define PRIB32 "B"
#define PRId32 "d"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"

#define __PRI64_LEN_MODIFIER__ "l"

#define PRIb64 __PRI64_LEN_MODIFIER__ "b"
#define PRIB64 __PRI64_LEN_MODIFIER__ "B"
#define PRId64 __PRI64_LEN_MODIFIER__ "d"
#define PRIo64 __PRI64_LEN_MODIFIER__ "o"
#define PRIu64 __PRI64_LEN_MODIFIER__ "u"
#define PRIx64 __PRI64_LEN_MODIFIER__ "x"
#define PRIX64 __PRI64_LEN_MODIFIER__ "X"