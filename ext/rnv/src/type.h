#ifndef TYPE_H
#define TYPE_H

#include <stdarg.h>
#include "ht.h"
#include "sc.h"

typedef struct rn_st
{
  struct hashtable ht_p;
  struct hashtable ht_nc;
  struct hashtable ht_s;
  int i_p;
  int i_nc;
  int i_s;
  int BASE_P;
  int base_p;
  int i_ref;
  int len_p;
  int len_nc;
  int len_s;
  int adding_ps;
} rn_st_t;

typedef struct rnc_st
{
  int len_p;
  char *path;
  struct sc_stack nss;
  struct sc_stack dts;
  struct sc_stack defs;
  struct sc_stack refs;
  struct sc_stack prefs;
} rnc_st_t;

typedef struct rnd_st
{
  int len_f;
  int n_f;
  int *flat;
  int errors;
} rnd_st_t;

typedef struct rnv rnv_t;
typedef struct rx_st
{
  char *regex;
  int *pattern;
  struct hashtable ht_r;
  struct hashtable ht_p;
  struct hashtable ht_2;
  int i_p;
  int len_p;
  int i_r;
  int len_r;
  int i_2;
  int len_2;
  int empty;
  int notAllowed;
  int any;
  int i_m;
  int len_m;
  struct hashtable ht_m;
  int r0;
  int ri;
  int sym;
  int val;
  int errors;
  int (*memo)[3];
  int (*r2p)[2];

  int rx_compact;

  // FIXME: for error handlers
  rnv_t *rnv;

} rx_st_t;

typedef struct drv_st
{
  struct dtl *dtl;
  int len_dtl;
  int n_dtl;
  int i_m;
  int len_m;
  struct hashtable ht_m;
  int (*memo)[5];

  int drv_compact;
} drv_st_t;

typedef struct rnv
{
  int *rn_pattern;
  int *rn_nameclass;
  char *rn_string;
  int rn_empty;
  int rn_text;
  int rn_notAllowed;
  int rn_dt_string;
  int rn_dt_token;
  int rn_xsd_uri;

  int rnx_n_exp;
  int *rnx_exp;
  int rnx_len_exp;

  int (*verror_handler)(rnv_t *rnv, int erno, char *format,va_list ap);

  void (*drv_verror_handler)(rnv_t *rnv, int erno, va_list ap);
  void (*rnc_verror_handler)(rnv_t *rnv, int erno, va_list ap);
  void (*rnd_verror_handler)(rnv_t *rnv, int erno, va_list ap);
  void (*rnl_verror_handler)(rnv_t *rnv, int erno, va_list ap);
  void (*rnv_verror_handler)(rnv_t *rnv, int erno, va_list ap);
  void (*rx_verror_handler)(rnv_t *rnv, int erno, va_list ap);
  void (*xsd_verror_handler)(rnv_t *rnv, int erno, va_list ap);

  void *user_data;

} rnv_t;

#endif // TYPE_H
