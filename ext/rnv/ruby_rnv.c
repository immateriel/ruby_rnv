#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h> /*open,close*/
#include <sys/types.h>
#include <unistd.h> /*open,read,close*/
#include <string.h> /*strerror*/
#include <errno.h>
#include <assert.h>

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

#include <ruby.h>
#include <ruby/io.h>

#define LEN_T XCL_LEN_T
#define LIM_T XCL_LIM_T

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

  rnv_t *rnv;
  rn_st_t *rn_st;
  rnc_st_t *rnc_st;
  rnx_st_t *rnx_st;
  drv_st_t *drv_st;
  rx_st_t *rx_st;
  rnd_st_t *rnd_st;

} document_t;

VALUE RNV;

// convert error code to symbol
ID errno_to_id(int erno)
{
  ID id;
  switch (erno)
  {
  case (ERBIT_RNC | RNC_ER_IO):
    id = rb_intern("rnc_er_io");
    break;
  case (ERBIT_RNC | RNC_ER_UTF):
    id = rb_intern("rnc_er_urf");
    break;
  case (ERBIT_RNC | RNC_ER_XESC):
    id = rb_intern("rnc_er_xesc");
    break;
  case (ERBIT_RNC | RNC_ER_LEXP):
    id = rb_intern("rnc_er_lexp");
    break;
  case (ERBIT_RNC | RNC_ER_LLIT):
    id = rb_intern("rnc_er_llit");
    break;
  case (ERBIT_RNC | RNC_ER_LILL):
    id = rb_intern("rnc_er_lill");
    break;
  case (ERBIT_RNC | RNC_ER_SEXP):
    id = rb_intern("rnc_er_sexp");
    break;
  case (ERBIT_RNC | RNC_ER_SILL):
    id = rb_intern("rnc_er_still");
    break;
  case (ERBIT_RNC | RNC_ER_NOTGR):
    id = rb_intern("rnc_er_notgr");
    break;
  case (ERBIT_RNC | RNC_ER_EXT):
    id = rb_intern("rnc_er_ext");
    break;
  case (ERBIT_RNC | RNC_ER_DUPNS):
    id = rb_intern("rnc_er_dupns");
    break;
  case (ERBIT_RNC | RNC_ER_DUPDT):
    id = rb_intern("rnc_er_dupdt");
    break;
  case (ERBIT_RNC | RNC_ER_DFLTNS):
    id = rb_intern("rnc_er_dfltns");
    break;
  case (ERBIT_RNC | RNC_ER_DFLTDT):
    id = rb_intern("rnc_er_dfltdt");
    break;
  case (ERBIT_RNC | RNC_ER_NONS):
    id = rb_intern("rnc_er_nons");
    break;
  case (ERBIT_RNC | RNC_ER_NODT):
    id = rb_intern("rnc_er_nodt");
    break;
  case (ERBIT_RNC | RNC_ER_NCEX):
    id = rb_intern("rnc_er_ncex");
    break;
  case (ERBIT_RNC | RNC_ER_2HEADS):
    id = rb_intern("rnc_er_2heads");
    break;
  case (ERBIT_RNC | RNC_ER_COMBINE):
    id = rb_intern("rnc_er_combine");
    break;
  case (ERBIT_RNC | RNC_ER_OVRIDE):
    id = rb_intern("rnc_er_ovride");
    break;
  case (ERBIT_RNC | RNC_ER_EXPT):
    id = rb_intern("rnc_er_excpt");
    break;
  case (ERBIT_RNC | RNC_ER_INCONT):
    id = rb_intern("rnc_er_incont");
    break;
  case (ERBIT_RNC | RNC_ER_NOSTART):
    id = rb_intern("rnc_er_nostart");
    break;
  case (ERBIT_RNC | RNC_ER_UNDEF):
    id = rb_intern("rnc_er_undef");
    break;

  case (ERBIT_RND | RND_ER_LOOPST):
    id = rb_intern("rnd_er_loopst");
    break;
  case (ERBIT_RND | RND_ER_LOOPEL):
    id = rb_intern("rnd_er_loopel");
    break;
  case (ERBIT_RND | RND_ER_CTYPE):
    id = rb_intern("rnd_er_ctype");
    break;
  case (ERBIT_RND | RND_ER_BADSTART):
    id = rb_intern("rnd_er_badstart");
    break;
  case (ERBIT_RND | RND_ER_BADMORE):
    id = rb_intern("rnd_er_badmore");
    break;
  case (ERBIT_RND | RND_ER_BADEXPT):
    id = rb_intern("rnd_er_badexpt");
    break;
  case (ERBIT_RND | RND_ER_BADLIST):
    id = rb_intern("rnd_er_badlist");
    break;
  case (ERBIT_RND | RND_ER_BADATTR):
    id = rb_intern("rnd_er_badattr");
    break;

  case (ERBIT_RX | RX_ER_BADCH):
    id = rb_intern("rx_er_badch");
    break;
  case (ERBIT_RX | RX_ER_UNFIN):
    id = rb_intern("rx_er_unfin");
    break;
  case (ERBIT_RX | RX_ER_NOLSQ):
    id = rb_intern("rx_er_nolsq");
    break;
  case (ERBIT_RX | RX_ER_NORSQ):
    id = rb_intern("rx_er_norsq");
    break;
  case (ERBIT_RX | RX_ER_NOLCU):
    id = rb_intern("rx_er_nolcu");
    break;
  case (ERBIT_RX | RX_ER_NORCU):
    id = rb_intern("rx_er_norcu");
    break;
  case (ERBIT_RX | RX_ER_NOLPA):
    id = rb_intern("rx_er_nolpa");
    break;
  case (ERBIT_RX | RX_ER_NORPA):
    id = rb_intern("rx_er_norpa");
    break;
  case (ERBIT_RX | RX_ER_BADCL):
    id = rb_intern("rx_er_badcl");
    break;
  case (ERBIT_RX | RX_ER_NODGT):
    id = rb_intern("rx_er_nodgt");
    break;
  case (ERBIT_RX | RX_ER_DNUOB):
    id = rb_intern("rx_er_dnuob");
    break;
  case (ERBIT_RX | RX_ER_NOTRC):
    id = rb_intern("rx_er_notrc");
    break;

  case (ERBIT_XSD | XSD_ER_TYP):
    id = rb_intern("xsd_er_typ");
    break;
  case (ERBIT_XSD | XSD_ER_PAR):
    id = rb_intern("xsd_er_par");
    break;
  case (ERBIT_XSD | XSD_ER_PARVAL):
    id = rb_intern("xsd_er_parval");
    break;
  case (ERBIT_XSD | XSD_ER_VAL):
    id = rb_intern("xsd_er_val");
    break;
  case (ERBIT_XSD | XSD_ER_NPAT):
    id = rb_intern("xsd_er_npat");
    break;
  case (ERBIT_XSD | XSD_ER_WS):
    id = rb_intern("xsd_er_ws");
    break;
  case (ERBIT_XSD | XSD_ER_ENUM):
    id = rb_intern("xsd_er_enum");
    break;

  case (ERBIT_DRV | DRV_ER_NODTL):
    id = rb_intern("drv_er_nodtl");
    break;

  case (ERBIT_RNV | RNV_ER_ELEM):
    id = rb_intern("rnv_er_elem");
    break;
  case (ERBIT_RNV | RNV_ER_AKEY):
    id = rb_intern("rnv_er_akey");
    break;
  case (ERBIT_RNV | RNV_ER_AVAL):
    id = rb_intern("rnv_er_aval");
    break;
  case (ERBIT_RNV | RNV_ER_EMIS):
    id = rb_intern("rnv_er_emis");
    break;
  case (ERBIT_RNV | RNV_ER_AMIS):
    id = rb_intern("rnv_er_amis");
    break;
  case (ERBIT_RNV | RNV_ER_UFIN):
    id = rb_intern("rnv_er_ufin");
    break;
  case (ERBIT_RNV | RNV_ER_TEXT):
    id = rb_intern("rnv_er_text");
    break;
  case (ERBIT_RNV | RNV_ER_NOTX):
    id = rb_intern("rnv_er_notx");
    break;

  default:
    id = rb_intern("unknown");
    break;
  }
  return id;
}

int ruby_verror_handler(rnv_t *rnv, int erno, char *format, va_list ap)
{
  VALUE self = (VALUE)rnv->user_data;
  document_t *document;

  Data_Get_Struct(self, document_t, document);

  rnx_st_t *rnx_st = document->rnx_st;

  VALUE errors = rb_iv_get(self, "@errors");

  VALUE error_array = rb_ary_new2(2);
  VALUE error_str = rb_vsprintf(format, ap);
  VALUE error_erno = ID2SYM(errno_to_id(erno));

  // lazyly strip with ruby
  rb_funcall(error_str, rb_intern("strip!"), 0);

  VALUE err_class = rb_const_get(RNV, rb_intern("Error"));
  VALUE err_obj = rb_class_new_instance(0, NULL, err_class);
  rb_iv_set(err_obj, "@document", self);
  rb_iv_set(err_obj, "@code", error_erno);
  rb_iv_set(err_obj, "@message", error_str);
  rb_iv_set(err_obj, "@line", rb_iv_get(self, "@last_line"));
  rb_iv_set(err_obj, "@col", rb_iv_get(self, "@last_col"));

  VALUE expected = rb_str_new2("");
  if (erno & ERBIT_RNV)
  {
    if (document->nexp)
    {
      int req = 2, i = 0;
      char *s;
      while (req--)
      {
        rnx_expected(rnv, rnx_st, document->previous, req);
        if (i == rnv->rnx_n_exp)
          continue;
        if (rnv->rnx_n_exp > document->nexp)
          break;

        expected = rb_str_cat2(expected, (char *)(req ? "required:\n" : "allowed:\n"));

        for (; i != rnv->rnx_n_exp; ++i)
        {
          s = rnx_p2str(rnv, rnv->rnx_exp[i]);
          expected = rb_str_cat2(expected, "\t");
          expected = rb_str_cat2(expected, s);
          expected = rb_str_cat2(expected, "\n");
          m_free(s);
        }
      }
    }
  }

  rb_iv_set(err_obj, "@expected", expected);

  rb_ary_push(errors, err_obj);
}

/*
 * @return [String]
 */
VALUE rb_error_inspect(VALUE self)
{
  VALUE code = rb_iv_get(self, "@code");
  VALUE message = rb_iv_get(self, "@message");
  VALUE expected = rb_iv_get(self, "@expected");
  VALUE line = rb_iv_get(self, "@line");
  VALUE col = rb_iv_get(self, "@col");

  VALUE ret = rb_str_new2("#<RNV::Error ");
  ret = rb_str_cat2(ret, "code: :");
  ret = rb_str_append(ret, rb_obj_as_string(code));
  ret = rb_str_cat2(ret, ", ");
  ret = rb_str_cat2(ret, "message: '");
  ret = rb_str_append(ret, message);
  ret = rb_str_cat2(ret, "', ");
  ret = rb_str_cat2(ret, "expected: '");
  ret = rb_str_append(ret, expected);
  ret = rb_str_cat2(ret, "', ");
  ret = rb_str_cat2(ret, "line: ");
  ret = rb_str_append(ret, rb_obj_as_string(line));
  ret = rb_str_cat2(ret, ", ");
  ret = rb_str_cat2(ret, "col: ");
  ret = rb_str_append(ret, rb_obj_as_string(col));
  ret = rb_str_cat2(ret, ">");
  return ret;
}

/*
 * @return [String]
 */
VALUE rb_error_to_s(VALUE self)
{
  VALUE message = rb_iv_get(self, "@message");
  VALUE expected = rb_iv_get(self, "@expected");
  VALUE line = rb_iv_get(self, "@line");
  VALUE col = rb_iv_get(self, "@col");

  VALUE ret = rb_str_new2("");

  ret = rb_str_append(ret, rb_obj_as_string(line));
  ret = rb_str_cat2(ret, ":");
  ret = rb_str_append(ret, rb_obj_as_string(col));

  ret = rb_str_cat2(ret, ": error: ");

  ret = rb_str_append(ret, message);
  ret = rb_str_cat2(ret, "\n");
  ret = rb_str_append(ret, expected);

  return ret;
}

void document_free(document_t *document)
{
  // FIXME : introduce *_delete functions
  if (document->rnd_st->flat)
    free(document->rnd_st->flat);
  free(document->rnd_st);

  ht_dispose(&document->rx_st->ht_r);
  ht_dispose(&document->rx_st->ht_p);
  ht_dispose(&document->rx_st->ht_2);
  ht_dispose(&document->rx_st->ht_m);
  if (document->rx_st->regex)
    free(document->rx_st->regex);
  if (document->rx_st->pattern)
    free(document->rx_st->pattern);
  free(document->rx_st);

  if (document->drv_st->dtl)
    free(document->drv_st->dtl);
  ht_dispose(&document->drv_st->ht_m);
  free(document->drv_st);

  free(document->rnx_st);

  if (document->rnc_st->path)
    free(document->rnc_st->path);
  free(document->rnc_st);

  ht_dispose(&document->rn_st->ht_p);
  ht_dispose(&document->rn_st->ht_nc);
  ht_dispose(&document->rn_st->ht_s);

  free(document->rn_st);

  if (document->rnv->rn_pattern)
    free(document->rnv->rn_pattern);
  if (document->rnv->rn_nameclass)
    free(document->rnv->rn_nameclass);
  if (document->rnv->rn_string)
    free(document->rnv->rn_string);
  if (document->rnv->rnx_exp)
    free(document->rnv->rnx_exp);

  free(document->rnv);

  free(document->text);

  ruby_xfree(document);
}

VALUE rb_document_alloc(VALUE klass)
{
  document_t *document = ruby_xmalloc(sizeof(document_t));

  document->rnv = malloc(sizeof(rnv_t));
  document->rn_st = malloc(sizeof(rn_st_t));
  document->rnc_st = malloc(sizeof(rnc_st_t));
  document->rnx_st = malloc(sizeof(rnx_st_t));
  document->drv_st = malloc(sizeof(drv_st_t));
  document->rx_st = malloc(sizeof(rx_st_t));
  document->rnd_st = malloc(sizeof(rnd_st_t));

  return Data_Wrap_Struct(klass, NULL, document_free, document);
}

VALUE rb_document_init(VALUE self)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  rnl_init(document->rnv, document->rn_st, document->rnc_st, document->rnd_st);
  rnv_init(document->rnv, document->drv_st, document->rn_st, document->rx_st);
  rnx_init(document->rnv, document->rnx_st);

  document->opened = document->ok = 0;
  document->rnv->user_data = (void *)self;
  document->rnv->verror_handler = &ruby_verror_handler;
  document->nexp = 16; /* maximum number of candidates to display */

  rb_iv_set(self, "@errors", rb_ary_new2(0));

  return self;
}

static void document_load(document_t *document)
{
  document->ok = document->current = document->previous = document->start = document->opened;
  document->len_txt = LEN_T;
  document->text = (char *)m_alloc(document->len_txt, sizeof(char));

  document->text[0] = '\0';
  document->n_txt = 0;
  document->mixed = 0;
}

/*
 * load schema from a buffer
 * @param [String] r_str buffer
 * @return [String]
 */
VALUE rb_document_load_string(VALUE self, VALUE r_str)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  Check_Type(r_str, T_STRING);

  VALUE r_fn = rb_str_new2("");
  document->fn = RSTRING_PTR(r_fn);

  document->opened = rnl_s(document->rnv,
                           document->rnc_st,
                           document->rn_st,
                           document->rnd_st,
                           document->fn,
                           RSTRING_PTR(r_str), RSTRING_LEN(r_str));

  document_load(document);
  return INT2NUM(document->ok);
}

/*
 * load schema from a file
 * @param [String] r_fn filename
 * @return [String]
 */
VALUE rb_document_load_file(VALUE self, VALUE r_fn)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  switch (TYPE(r_fn))
  {
  case T_STRING:
    document->fn = RSTRING_PTR(r_fn);

    document->opened = rnl_fn(document->rnv,
                              document->rnc_st,
                              document->rn_st,
                              document->rnd_st,
                              document->fn);

    break;
  case T_FILE: // TODO
  default:
    rb_raise(rb_eTypeError, "invalid argument");
    break;
  }

  document_load(document);
  return INT2NUM(document->ok);
}

/*
 * is current document valid ?
 * @return [Array<RNV::Error>]
 */
VALUE rb_document_valid(VALUE self)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (document->ok)
    return Qtrue;
  else
    return Qfalse;
}

static void flush_text(document_t *document)
{
  document->ok = rnv_text(document->rnv, document->drv_st, document->rn_st, document->rx_st,
                          &document->current, &document->previous, document->text, document->n_txt, document->mixed) &&
                 document->ok;
  document->text[document->n_txt = 0] = '\0';
}

/*
 * begin a new document
 * @return [nil]
 */
VALUE rb_document_begin(VALUE self)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  m_free(document->text);
  document->text = (char *)m_alloc(document->len_txt = LEN_T, sizeof(char));

  document->ok = document->current = document->previous = document->start;

  document->text[0] = '\0';
  document->n_txt = 0;
  document->mixed = 0;

  return Qnil;
}

/*
 * to be called by SAX characters handler
 * @param [String] r_str characters
 * @return [Integer]
 */
VALUE rb_document_characters(VALUE self, VALUE r_str)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (document->opened && document->current != document->rnv->rn_notAllowed)
  {
    Check_Type(r_str, T_STRING);
    char *s = RSTRING_PTR(r_str);
    int len = RSTRING_LEN(r_str);

    int newlen_txt = document->n_txt + len + 1;
    if (newlen_txt <= LIM_T && LIM_T < document->len_txt)
      newlen_txt = LIM_T;
    else if (newlen_txt < document->len_txt)
      newlen_txt = document->len_txt;
    if (document->len_txt != newlen_txt)
    {
      document->text = (char *)m_stretch(document->text, document->len_txt = newlen_txt, document->n_txt, sizeof(char));
    }

    memcpy(document->text + document->n_txt, s, len);
    document->n_txt += len;
    document->text[document->n_txt] = '\0'; /* '\0' guarantees that the text is bounded, and strto[ld] work for data */
  }

  return INT2NUM(document->ok);
}

/*
 * to be called by SAX start tag handler
 * @param [String] r_name tag name, must be in the form 'NS_URI:TAG_NAME'
 * @param [Array<String>] r_attrs flattened array of tag attributes in the form ['NS_URI:ATTR_NAME','ATTR_VALUE']
 * @return [Integer]
 */
VALUE rb_document_start_tag(VALUE self, VALUE r_name, VALUE r_attrs)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (document->opened && document->current != document->rnv->rn_notAllowed)
  {
    int i;
    char *name;
    char **attrs;

    Check_Type(r_name, T_STRING);
    name = RSTRING_PTR(r_name);

    Check_Type(r_attrs, T_ARRAY);
    unsigned int attrs_len = RARRAY_LEN(r_attrs);

    attrs = malloc(sizeof(char *) * (attrs_len + 1));

    for (i = 0; i < attrs_len; i++)
    {
      attrs[i] = RSTRING_PTR(rb_ary_entry(r_attrs, i));
    }
    attrs[attrs_len] = 0; // zero terminated

    document->mixed = 1;

    flush_text(document);
    //printf("RNV START %d/%d %s %d\n", current, previous, name, attrs_len);
    document->ok = rnv_start_tag(document->rnv, document->drv_st, document->rn_st, document->rx_st,
                                 &document->current, &document->previous, (char *)name, (char **)attrs) &&
                   document->ok;

    document->mixed = 0;
    free(attrs);
  }

  return INT2NUM(document->ok);
}

/*
 * to be called by SAX end tag handler
 * @param [String] r_name tag name, must be in the form 'NS_URI:TAG_NAME'
 * @return [Integer]
 */
VALUE rb_document_end_tag(VALUE self, VALUE r_name)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (document->opened && document->current != document->rnv->rn_notAllowed)
  {
    char *name;

    Check_Type(r_name, T_STRING);
    name = RSTRING_PTR(r_name);

    flush_text(document);

    //printf("RNV END %d/%d %s\n", current, previous, name);
    document->ok = rnv_end_tag(document->rnv, document->drv_st, document->rn_st,
                               &document->current, &document->previous, (char *)name) &&
                   document->ok;

    document->mixed = 1;
  }

  return INT2NUM(document->ok);
}

// The initialization method for this module
void Init_rnv()
{
  RNV = rb_define_module("RNV");

  VALUE Error = rb_define_class_under(RNV, "Error", rb_cObject);

  rb_define_method(Error, "inspect", rb_error_inspect, 0);
  rb_define_method(Error, "to_s", rb_error_to_s, 0);

  /*
   * error symbol code
   * @return [Symbol]
   */
  rb_define_attr(Error, "code", 1, 0);
  /*
   * error message
   * @return [String]
   */
  rb_define_attr(Error, "message", 1, 0);
  /*
   * error line
   * @return [Integer]
   */
  rb_define_attr(Error, "line", 1, 0);
  /*
   * error column
   * @return [Integer]
   */
  rb_define_attr(Error, "col", 1, 0);

  VALUE Document = rb_define_class_under(RNV, "Document", rb_cObject);

  rb_define_alloc_func(Document, rb_document_alloc);
  rb_define_method(Document, "initialize", rb_document_init, 0);

  rb_define_method(Document, "load_file", rb_document_load_file, 1);
  rb_define_method(Document, "load_string", rb_document_load_string, 1);
  rb_define_method(Document, "valid?", rb_document_valid, 0);

  rb_define_method(Document, "start_document", rb_document_begin, 0);
  rb_define_method(Document, "start_tag", rb_document_start_tag, 2);
  rb_define_method(Document, "characters", rb_document_characters, 1);
  rb_define_method(Document, "end_tag", rb_document_end_tag, 1);

  /*
   * last line processed, set by SAX handler
   * @return [Integer]
   */
  rb_define_attr(Document, "last_line", 1, 1);
  /*
   * last column processed, set by SAX handler
   * @return [Integer]
   */
  rb_define_attr(Document, "last_col", 1, 1);

  /*
 * errors from current document
 * @return [Array<RNV::Error>]
 */
  rb_define_attr(Document, "errors", 1, 1);
}
