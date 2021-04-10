/* $Id: rnd.h,v 1.11 2004/01/02 20:27:23 dvd Exp $ */

#include <stdarg.h>
#include "type.h"

#ifndef RND_H
#define RND_H 1

#define RND_ER_LOOPST 0
#define RND_ER_LOOPEL 1
#define RND_ER_CTYPE 2
#define RND_ER_BADSTART 3
#define RND_ER_BADMORE 4
#define RND_ER_BADEXPT 5
#define RND_ER_BADLIST 6
#define RND_ER_BADATTR 7

extern void rnd_default_verror_handler(rnv_t *rnv, int erno,va_list ap);

extern void rnd_init(rnv_t *rnv, rnd_st_t *rnd_st, rn_st_t *rn_st);
extern void rnd_clear(void);
extern void rnd_dispose(rnd_st_t *rnd_st);

extern int rnd_fixup(rnv_t *rnv, rnd_st_t *rnd_st, rn_st_t *rn_st, int start);

#endif
