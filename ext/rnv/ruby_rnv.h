#ifndef RUBY_RNV_H
#define RUBY_RNV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h> /*open,close*/
#include <sys/types.h>
#include <unistd.h> /*open,read,close*/
#include <string.h> /*strerror*/
#include <errno.h>
#include <assert.h>

#include <ruby.h>
#include <ruby/io.h>

#include "src/m.h"
#include "src/s.h"
#include "src/erbit.h"
#include "src/drv.h"
#include "src/rnl.h"
#include "src/rnv.h"
#include "src/rnx.h"
#include "src/ll.h"
#include "src/er.h"
#include "src/erbit.h"
#include "src/rnc.h"
#include "src/rnd.h"
#include "src/rx.h"
#include "src/xsd.h"
#include "src/rn.h"

typedef struct document
{
  char *fn;
  int start;
  int current;
  int previous;
  int lastline;
  int lastcol;
  int level;
  int opened;
  int ok;
  char *text;
  int len_txt;
  int n_txt;
  int mixed;
  int nexp;

  int skip_next_error;

  rnv_t *rnv;
  rn_st_t *rn_st;
  rnc_st_t *rnc_st;
  drv_st_t *drv_st;
  rx_st_t *rx_st;
  rnd_st_t *rnd_st;

} document_t;


#endif