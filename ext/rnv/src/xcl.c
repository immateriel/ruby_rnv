/* rnv command line tool keeped for reference, actually not used */
/* $Id: xcl.c,v 1.44 2006/01/29 18:29:57 dvd Exp $ */
#include "type.h"

#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h> /*open,close*/
#include <sys/types.h>
#include <unistd.h> /*open,read,close*/
#include <string.h> /*strerror*/
#include <errno.h>
#include <assert.h>
#include <expat.h>
#include "m.h"
#include "s.h"
#include "erbit.h"
#include "drv.h"
#include "rnl.h"
#include "rnv.h"
#include "rnx.h"
#include "ll.h"
#include "er.h"

#define RNV_VERSION "1.7.8"

extern int rn_notAllowed, rx_compact, drv_compact;

  typedef struct xcl_st {
   int peipe;
   int verbose;
   int nexp;
   int rnck;
   char * xml;
   int start;
   int current;
   int previous;
   int lastline;
   int lastcol;
   int level;
   int ok;
   char * text;
   int len_txt;
   int n_txt;

   rnv_t *rnv;
   drv_st_t *drv_st; 
   rn_st_t *rn_st;
   rx_st_t *rx_st;
  } xcl_st_t;

#define LEN_T XCL_LEN_T
#define LIM_T XCL_LIM_T

#define BUFSIZE 1024

/* maximum number of candidates to display */
#define NEXP 16

#define XCL_ER_IO 0
#define XCL_ER_XML 1
#define XCL_ER_XENT 2

#define PIXGFILE "davidashen-net-xg-file"
#define PIXGPOS "davidashen-net-xg-pos"

static XML_Parser expat = NULL;
static int mixed = 0;
static char *xgfile = NULL, *xgpos = NULL;

#define err(msg) (*rnv->verror_handler)(rnv,erno,msg "\n", ap);
static void verror_handler(rnv_t *rnv, xcl_st_t *xcl_st, rnx_st_t *rnx_st, int erno, va_list ap)
{
  if (erno & ERBIT_RNL)
  {
    rnl_default_verror_handler(rnv, erno & ~ERBIT_RNL, ap);
  }
  else
  {
    int line = XML_GetCurrentLineNumber(expat), col = XML_GetCurrentColumnNumber(expat);
    if (line != xcl_st->lastline || col != xcl_st->lastcol)
    {
      xcl_st->lastline = line;
      xcl_st->lastcol = col;
      if (xgfile)
        (*er_printf)("%s:%s: error: ", xgfile, xgpos);
      else
        (*er_printf)("%s:%i:%i: error: ", xcl_st->xml, line, col);
      if (erno & ERBIT_RNV)
      {
        rnv_default_verror_handler(rnv, erno & ~ERBIT_RNV, ap);
        if (xcl_st->nexp)
        {
          int req = 2, i = 0;
          char *s;
          while (req--)
          {
            rnx_expected(rnv, rnx_st, xcl_st->previous, req);
            if (i == rnv->rnx_n_exp)
              continue;
            if (rnv->rnx_n_exp > xcl_st->nexp)
              break;
            (*er_printf)((char *)(req ? "required:\n" : "allowed:\n"));
            for (; i != rnv->rnx_n_exp; ++i)
            {
              (*er_printf)("\t%s\n", s = rnx_p2str(rnv, rnv->rnx_exp[i]));
              m_free(s);
            }
          }
        }
      }
      else
      {
        switch (erno)
        {
        case XCL_ER_IO:
          err("%s");
          break;
        case XCL_ER_XML:
          err("%s");
          break;
        case XCL_ER_XENT:
          err("pipe through xx to expand external entities");
          break;
        default:
          assert(0);
        }
      }
    }
  }
}

static void verror_handler_rnl(rnv_t *rnv, xcl_st_t *xcl_st, rnx_st_t *rnx_st, int erno, va_list ap) { verror_handler(rnv, xcl_st, rnx_st, erno | ERBIT_RNL, ap); }
static void verror_handler_rnv(rnv_t *rnv, xcl_st_t *xcl_st, rnx_st_t *rnx_st, int erno, va_list ap) { verror_handler(rnv, xcl_st, rnx_st, erno | ERBIT_RNV, ap); }

static void windup(xcl_st_t *xcl_st);
static void init(rnv_t *rnv, xcl_st_t *xcl_st, rn_st_t *rn_st, rnd_st_t *rnd_st, rnc_st_t *rnc_st, rnx_st_t *rnx_st, drv_st_t *drv_st, rx_st_t *rx_st)
{
    rnv->verror_handler=&verror_default_handler;
    rnl_init(rnv, rn_st, rnd_st, rnc_st);
    //rnl_verror_handler = &verror_handler_rnl;
    rnv_init(rnv, drv_st, rn_st, rx_st);
    //rnv_verror_handler = &verror_handler_rnv;
    rnx_init(rnv, rnx_st);
    xcl_st->text = (char *)m_alloc(xcl_st->len_txt = LEN_T, sizeof(char));
    windup(xcl_st);
}

static void clear(xcl_st_t *xcl_st)
{
  if (xcl_st->len_txt > LIM_T)
  {
    m_free(xcl_st->text);
    xcl_st->text = (char *)m_alloc(xcl_st->len_txt = LEN_T, sizeof(char));
  }
  windup(xcl_st);
}

static void windup(xcl_st_t *xcl_st)
{
  xcl_st->text[xcl_st->n_txt = 0] = '\0';
  xcl_st->level = 0;
  xcl_st->lastline = xcl_st->lastcol = -1;
}

static void error_handler(rnv_t *rnv, xcl_st_t *xcl_st, rnx_st_t *rnx_st, int erno, ...)
{
  va_list ap;
  va_start(ap, erno);
  verror_handler(rnv, xcl_st, rnx_st, erno, ap);
  va_end(ap);
}

static void flush_text(rnv_t *rnv, xcl_st_t *xcl_st, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st)
{
  xcl_st->ok = rnv_text(rnv, drv_st, rn_st, rx_st, &xcl_st->current, &xcl_st->previous, xcl_st->text, xcl_st->n_txt, mixed) && xcl_st->ok;
  xcl_st->text[xcl_st->n_txt = 0] = '\0';
}

static void start_element(void *userData, const char *name, const char **attrs)
{
  xcl_st_t *xcl_st = (xcl_st_t *)userData;
  rnv_t *rnv = xcl_st->rnv;
  drv_st_t *drv_st = xcl_st->drv_st;
  rn_st_t *rn_st = xcl_st->rn_st;
  rx_st_t *rx_st = xcl_st->rx_st;

  if (xcl_st->current != rnv->rn_notAllowed)
  {
    mixed = 1;
    flush_text(rnv, xcl_st, drv_st, rn_st, rx_st);
    xcl_st->ok = rnv_start_tag(rnv, drv_st, rn_st, rx_st, &xcl_st->current, &xcl_st->previous, (char *)name, (char **)attrs) && xcl_st->ok;
    mixed = 0;
  }
  else
  {
    ++xcl_st->level;
  }
}

static void end_element(void *userData, const char *name)
{
  xcl_st_t *xcl_st = (xcl_st_t *)userData;
  rnv_t *rnv = xcl_st->rnv;
  drv_st_t *drv_st = xcl_st->drv_st;
  rn_st_t *rn_st = xcl_st->rn_st;
  rx_st_t *rx_st = xcl_st->rx_st;

  if (xcl_st->current != rnv->rn_notAllowed)
  {
    flush_text(rnv, xcl_st, drv_st, rn_st, rx_st);
    xcl_st->ok = rnv_end_tag(rnv, drv_st, rn_st, &xcl_st->current, &xcl_st->previous, (char *)name) && xcl_st->ok;
    mixed = 1;
  }
  else
  {
    if (xcl_st->level == 0)
      xcl_st->current = xcl_st->previous;
    else
      --xcl_st->level;
  }
}

static void characters(void *userData, const char *s, int len)
{
  xcl_st_t *xcl_st = (xcl_st_t *)userData;
  rnv_t *rnv = xcl_st->rnv;

  if (xcl_st->current != rnv->rn_notAllowed)
  {
    int newlen_txt = xcl_st->n_txt + len + 1;
    if (newlen_txt <= LIM_T && LIM_T < xcl_st->len_txt)
      newlen_txt = LIM_T;
    else if (newlen_txt < xcl_st->len_txt)
      newlen_txt = xcl_st->len_txt;
    if (xcl_st->len_txt != newlen_txt)
      xcl_st->text = (char *)m_stretch(xcl_st->text, xcl_st->len_txt = newlen_txt, xcl_st->n_txt, sizeof(char));
    memcpy(xcl_st->text + xcl_st->n_txt, s, len);
    xcl_st->n_txt += len;
    xcl_st->text[xcl_st->n_txt] = '\0'; /* '\0' guarantees that the text is bounded, and strto[ld] work for data */
  }
}

static void processingInstruction(void *userData,
                                  const char *target, const char *data)
{
  if (strcmp(PIXGFILE, target) == 0)
  {
    if (xgfile)
      m_free(xgfile);
    xgfile = s_clone((char *)data);
  }
  else if (strcmp(PIXGPOS, target) == 0)
  {
    if (xgpos)
      m_free(xgpos);
    xgpos = s_clone((char *)data);
    *strchr(xgpos, ' ') = ':';
  }
}

static int pipeout(rnv_t *rnv, xcl_st_t *xcl_st, rnx_st_t *rnx_st, void *buf, int len)
{
  int ofs = 0, iw, lenw = len;
  for (;;)
  {
    if ((iw = write(1, (char *)buf + ofs, lenw)) == -1)
    {
      error_handler(rnv, xcl_st, rnx_st, XCL_ER_IO, strerror(errno));
      return 0;
    }
    ofs += iw;
    lenw -= iw;
    if (lenw == 0)
      return 1;
  }
}

static int process(rnv_t *rnv, xcl_st_t *xcl_st, rnx_st_t *rnx_st, int fd)
{
  void *buf;
  int len;
  for (;;)
  {
    buf = XML_GetBuffer(expat, BUFSIZE);
    len = read(fd, buf, BUFSIZE);
    if (len < 0)
    {
      error_handler(rnv, xcl_st, rnx_st, XCL_ER_IO, xcl_st->xml, strerror(errno));
      goto ERROR;
    }
    if (xcl_st->peipe)
      xcl_st->peipe = xcl_st->peipe && pipeout(rnv, xcl_st, rnx_st, buf, len);
    if (!XML_ParseBuffer(expat, len, len == 0))
      goto PARSE_ERROR;
    if (len == 0)
      break;
  }
  return xcl_st->ok;

PARSE_ERROR:
  error_handler(rnv, xcl_st, rnx_st, XCL_ER_XML, XML_ErrorString(XML_GetErrorCode(expat)));
  while (xcl_st->peipe && (len = read(fd, buf, BUFSIZE)) != 0)
    xcl_st->peipe = xcl_st->peipe && pipeout(rnv, xcl_st, rnx_st, buf, len);
ERROR:
  return 0;
}

static int externalEntityRef(XML_Parser p, const char *context,
                             const char *base, const char *systemId, const char *publicId)
{
  //error_handler(rnv, xcl_st, rnx_st, XCL_ER_XENT);
  return 1;
}

static void validate(rnv_t *rnv, xcl_st_t *xcl_st, drv_st_t *drv_st, rn_st_t *rn_st, rnx_st_t *rnx_st, int fd)
{
  xcl_st->previous = xcl_st->current = xcl_st->start;
  expat = XML_ParserCreateNS(NULL, ':');
  XML_SetParamEntityParsing(expat, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetElementHandler(expat, &start_element, &end_element);
  XML_SetCharacterDataHandler(expat, &characters);
  XML_SetExternalEntityRefHandler(expat, &externalEntityRef);
  XML_SetProcessingInstructionHandler(expat, &processingInstruction);
  XML_SetUserData(expat, xcl_st);
  xcl_st->ok = process(rnv, xcl_st, rnx_st, fd);
  XML_ParserFree(expat);
}

static void version(void) { (*er_printf)("rnv version %s\n", RNV_VERSION); }
static void usage(void) { (*er_printf)("usage: rnv {-[qnspcvh?]} schema.rnc {document.xml}\n"); }

int main(int argc, char **argv)
{
  rnv_t *rnv;
  rn_st_t *rn_st;
  rnc_st_t *rnc_st;
  rnx_st_t *rnx_st;
  drv_st_t *drv_st;
  rx_st_t *rx_st;
  rnd_st_t *rnd_st;
  xcl_st_t *xcl_st;

  rnv = malloc(sizeof(rnv_t));
  rn_st = malloc(sizeof(rn_st_t));
  rnc_st = malloc(sizeof(rnc_st_t));
  rnx_st = malloc(sizeof(rnx_st_t));
  drv_st = malloc(sizeof(drv_st_t));
  rx_st = malloc(sizeof(rx_st_t));
  rnd_st = malloc(sizeof(rnd_st_t));
  xcl_st = malloc(sizeof(xcl_st_t));

  xcl_st->rnv = rnv;
  xcl_st->drv_st = drv_st;
  xcl_st->rn_st = rn_st;
  xcl_st->rx_st = rx_st;

  init(rnv, xcl_st, rn_st, rnc_st, rnd_st, rnx_st, drv_st, rx_st);

  xcl_st->peipe = 0;
  xcl_st->verbose = 1;
  xcl_st->nexp = NEXP;
  xcl_st->rnck = 0;
  while (*(++argv) && **argv == '-')
  {
    int i = 1;
    for (;;)
    {
      switch (*(*argv + i))
      {
      case '\0':
        goto END_OF_OPTIONS;
      case 'q':
        xcl_st->verbose = 0;
        xcl_st->nexp = 0;
        break;
      case 'n':
        if (*(argv + 1))
          xcl_st->nexp = atoi(*(++argv));
        goto END_OF_OPTIONS;
      case 's':
        rnv->drv_compact = 1;
        rnv->rx_compact = 1;
        break;
      case 'p':
        xcl_st->peipe = 1;
        break;
      case 'c':
        xcl_st->rnck = 1;
        break;
#if DXL_EXC
      case 'd':
        dxl_cmd = *(argv + 1);
        if (*(argv + 1))
          ++argv;
        goto END_OF_OPTIONS;
#endif
#if DSL_SCM
      case 'e':
        dsl_ld(*(argv + 1));
        if (*(argv + 1))
          ++argv;
        goto END_OF_OPTIONS;
#endif
      case 'v':
        version();
        break;
      case 'h':
      case '?':
        usage();
        return 1;
      default:
        (*er_printf)("unknown option '-%c'\n", *(*argv + i));
        break;
      }
      ++i;
    }
  END_OF_OPTIONS:;
  }

  if (!*(argv))
  {
    usage();
    return 1;
  }

  if ((xcl_st->ok = xcl_st->start = rnl_fn(rnv, rnc_st, rn_st, rnd_st, *(argv++))))
  {
    if (*argv)
    {
      do
      {
        int fd;
        xcl_st->xml = *argv;
        if ((fd = open(xcl_st->xml, O_RDONLY)) == -1)
        {
          (*er_printf)("I/O error (%s): %s\n", xcl_st->xml, strerror(errno));
          xcl_st->ok = 0;
          continue;
        }
        if (xcl_st->verbose)
          (*er_printf)("%s\n", xcl_st->xml);
        validate(rnv, xcl_st, drv_st, rn_st, rnx_st, fd);
        close(fd);
        clear(xcl_st);
      } while (*(++argv));
      if (!xcl_st->ok && xcl_st->verbose)
        (*er_printf)("error: some documents are invalid\n");
    }
    else
    {
      if (!xcl_st->rnck)
      {
        xcl_st->xml = "stdin";
        validate(rnv, xcl_st, drv_st, rn_st, rnx_st, 0);
        clear(xcl_st);
        if (!xcl_st->ok && xcl_st->verbose)
          (*er_printf)("error: invalid input\n");
      }
    }
  }

  free(rnv);
  free(rn_st);
  free(rnc_st);
  free(rnx_st);
  free(drv_st);
  free(rx_st);
  free(rnd_st);
  free(xcl_st);

  return xcl_st->ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
