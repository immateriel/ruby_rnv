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

#include <ruby.h>
#include <ruby/io.h>

#define LEN_T XCL_LEN_T
#define LIM_T XCL_LIM_T

typedef struct validator
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

  rnv_t *rnv;
  rn_st_t *rn_st;
  rnc_st_t *rnc_st;
  rnx_st_t *rnx_st;
  drv_st_t *drv_st;
  rx_st_t *rx_st;
  rnd_st_t *rnd_st;

} validator_t;

#include "src/erbit.h"
#include "src/rnc.h"
#include "src/rnd.h"
#include "src/rx.h"
#include "src/xsd.h"

// convert error code to symbol
ID errno_to_id(int erno) {
  ID id;
  switch(erno) {
  case (ERBIT_RNC|RNC_ER_IO): id=rb_intern("rnc_er_io"); break;
  case (ERBIT_RNC|RNC_ER_UTF): id=rb_intern("rnc_er_urf"); break;
  case (ERBIT_RNC|RNC_ER_XESC): id=rb_intern("rnc_er_xesc"); break;
  case (ERBIT_RNC|RNC_ER_LEXP): id=rb_intern("rnc_er_lexp"); break;
  case (ERBIT_RNC|RNC_ER_LLIT): id=rb_intern("rnc_er_llit"); break;
  case (ERBIT_RNC|RNC_ER_LILL): id=rb_intern("rnc_er_lill"); break;
  case (ERBIT_RNC|RNC_ER_SEXP): id=rb_intern("rnc_er_sexp"); break;
  case (ERBIT_RNC|RNC_ER_SILL): id=rb_intern("rnc_er_still"); break;
  case (ERBIT_RNC|RNC_ER_NOTGR): id=rb_intern("rnc_er_notgr"); break;
  case (ERBIT_RNC|RNC_ER_EXT): id=rb_intern("rnc_er_ext"); break;
  case (ERBIT_RNC|RNC_ER_DUPNS): id=rb_intern("rnc_er_dupns"); break;
  case (ERBIT_RNC|RNC_ER_DUPDT): id=rb_intern("rnc_er_dupdt"); break;
  case (ERBIT_RNC|RNC_ER_DFLTNS): id=rb_intern("rnc_er_dfltns"); break;
  case (ERBIT_RNC|RNC_ER_DFLTDT): id=rb_intern("rnc_er_dfltdt"); break;
  case (ERBIT_RNC|RNC_ER_NONS): id=rb_intern("rnc_er_nons"); break;
  case (ERBIT_RNC|RNC_ER_NODT): id=rb_intern("rnc_er_nodt"); break;
  case (ERBIT_RNC|RNC_ER_NCEX): id=rb_intern("rnc_er_ncex"); break;
  case (ERBIT_RNC|RNC_ER_2HEADS): id=rb_intern("rnc_er_2heads"); break;
  case (ERBIT_RNC|RNC_ER_COMBINE): id=rb_intern("rnc_er_combine"); break;
  case (ERBIT_RNC|RNC_ER_OVRIDE): id=rb_intern("rnc_er_ovride"); break;
  case (ERBIT_RNC|RNC_ER_EXPT): id=rb_intern("rnc_er_excpt"); break;
  case (ERBIT_RNC|RNC_ER_INCONT): id=rb_intern("rnc_er_incont"); break;
  case (ERBIT_RNC|RNC_ER_NOSTART): id=rb_intern("rnc_er_nostart"); break;
  case (ERBIT_RNC|RNC_ER_UNDEF): id=rb_intern("rnc_er_undef"); break;

  case (ERBIT_RND|RND_ER_LOOPST): id=rb_intern("rnd_er_loopst"); break;
  case (ERBIT_RND|RND_ER_LOOPEL): id=rb_intern("rnd_er_loopel"); break;
  case (ERBIT_RND|RND_ER_CTYPE): id=rb_intern("rnd_er_ctype"); break;
  case (ERBIT_RND|RND_ER_BADSTART): id=rb_intern("rnd_er_badstart"); break;
  case (ERBIT_RND|RND_ER_BADMORE): id=rb_intern("rnd_er_badmore"); break;
  case (ERBIT_RND|RND_ER_BADEXPT): id=rb_intern("rnd_er_badexpt"); break;
  case (ERBIT_RND|RND_ER_BADLIST): id=rb_intern("rnd_er_badlist"); break;
  case (ERBIT_RND|RND_ER_BADATTR): id=rb_intern("rnd_er_badattr"); break;

  case (ERBIT_RX|RX_ER_BADCH): id=rb_intern("rx_er_badch"); break;
  case (ERBIT_RX|RX_ER_UNFIN): id=rb_intern("rx_er_unfin"); break;
  case (ERBIT_RX|RX_ER_NOLSQ): id=rb_intern("rx_er_nolsq"); break;
  case (ERBIT_RX|RX_ER_NORSQ): id=rb_intern("rx_er_norsq"); break;
  case (ERBIT_RX|RX_ER_NOLCU): id=rb_intern("rx_er_nolcu"); break;
  case (ERBIT_RX|RX_ER_NORCU): id=rb_intern("rx_er_norcu"); break;
  case (ERBIT_RX|RX_ER_NOLPA): id=rb_intern("rx_er_nolpa"); break;
  case (ERBIT_RX|RX_ER_NORPA): id=rb_intern("rx_er_norpa"); break;
  case (ERBIT_RX|RX_ER_BADCL): id=rb_intern("rx_er_badcl"); break;
  case (ERBIT_RX|RX_ER_NODGT): id=rb_intern("rx_er_nodgt"); break;
  case (ERBIT_RX|RX_ER_DNUOB): id=rb_intern("rx_er_dnuob"); break;
  case (ERBIT_RX|RX_ER_NOTRC): id=rb_intern("rx_er_notrc"); break;

  case (ERBIT_XSD|XSD_ER_TYP): id=rb_intern("xsd_er_typ"); break;
  case (ERBIT_XSD|XSD_ER_PAR): id=rb_intern("xsd_er_par"); break;
  case (ERBIT_XSD|XSD_ER_PARVAL): id=rb_intern("xsd_er_parval"); break;
  case (ERBIT_XSD|XSD_ER_VAL): id=rb_intern("xsd_er_val"); break;
  case (ERBIT_XSD|XSD_ER_NPAT): id=rb_intern("xsd_er_npat"); break;
  case (ERBIT_XSD|XSD_ER_WS): id=rb_intern("xsd_er_ws"); break;
  case (ERBIT_XSD|XSD_ER_ENUM): id=rb_intern("xsd_er_enum"); break;

  case (ERBIT_DRV|DRV_ER_NODTL): id=rb_intern("drv_er_nodtl"); break;

  case (ERBIT_RNV|RNV_ER_ELEM): id=rb_intern("rnv_er_elem"); break;
  case (ERBIT_RNV|RNV_ER_AKEY): id=rb_intern("rnv_er_akey"); break;
  case (ERBIT_RNV|RNV_ER_AVAL): id=rb_intern("rnv_er_aval"); break;
  case (ERBIT_RNV|RNV_ER_EMIS): id=rb_intern("rnv_er_emis"); break;
  case (ERBIT_RNV|RNV_ER_AMIS): id=rb_intern("rnv_er_amis"); break;
  case (ERBIT_RNV|RNV_ER_UFIN): id=rb_intern("rnv_er_ufin"); break;
  case (ERBIT_RNV|RNV_ER_TEXT): id=rb_intern("rnv_er_text"); break;
  case (ERBIT_RNV|RNV_ER_NOTX): id=rb_intern("rnv_er_notx"); break;

  default: id=rb_intern("unknown"); break;

  }
  return id;
}

void ruby_verror_handler(rnv_t *rnv, int erno, char *format, va_list ap)
{
  VALUE self = (VALUE)rnv->user_data;
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  VALUE errors = rb_iv_get(self, "@errors");

  VALUE error_array = rb_ary_new2(2);
  VALUE error_str = rb_vsprintf(format, ap);
  VALUE error_erno = ID2SYM(errno_to_id(erno));
  //VALUE error_erno = INT2NUM(erno);
  rb_ary_push(error_array, error_erno);
  rb_ary_push(error_array, error_str);

  rb_ary_push(error_array, rb_iv_get(self, "@last_line"));
  rb_ary_push(error_array, rb_iv_get(self, "@last_col"));

  rb_ary_push(errors, error_array);
}

void validator_free(validator_t *validator)
{
  // FIXME : introduce *_delete functions
  free(validator->rnd_st->flat);
  free(validator->rnd_st);

  ht_dispose(&validator->rx_st->ht_r);
  ht_dispose(&validator->rx_st->ht_p);
  ht_dispose(&validator->rx_st->ht_2);
  ht_dispose(&validator->rx_st->ht_m);
  free(validator->rx_st->regex);
  free(validator->rx_st->pattern);
  free(validator->rx_st);

  free(validator->drv_st->dtl);
  ht_dispose(&validator->drv_st->ht_m);
  free(validator->drv_st);

  free(validator->rnx_st);

  free(validator->rnc_st->path);
  free(validator->rnc_st);

  ht_dispose(&validator->rn_st->ht_p);
  ht_dispose(&validator->rn_st->ht_nc);
  ht_dispose(&validator->rn_st->ht_s);

  free(validator->rn_st);

  free(validator->rnv->rn_pattern);
  free(validator->rnv->rn_nameclass);
  free(validator->rnv->rn_string);
  free(validator->rnv->rnx_exp);

  free(validator->rnv);
  ruby_xfree(validator);
}

VALUE rb_validator_alloc(VALUE klass)
{
  validator_t *validator = ruby_xmalloc(sizeof(validator_t));

  validator->rnv = malloc(sizeof(rnv_t));
  validator->rn_st = malloc(sizeof(rn_st_t));
  validator->rnc_st = malloc(sizeof(rnc_st_t));
  validator->rnx_st = malloc(sizeof(rnx_st_t));
  validator->drv_st = malloc(sizeof(drv_st_t));
  validator->rx_st = malloc(sizeof(rx_st_t));
  validator->rnd_st = malloc(sizeof(rnd_st_t));

  return Data_Wrap_Struct(klass, NULL, validator_free, validator);
}

VALUE rb_validator_init(VALUE self)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  rnl_init(validator->rnv, validator->rn_st, validator->rnc_st);
  rnv_init(validator->rnv, validator->drv_st, validator->rn_st, validator->rx_st);
  rnx_init(validator->rnv, validator->rnx_st);

  validator->opened = 0;
  validator->rnv->user_data = self;
  validator->rnv->verror_handler = &ruby_verror_handler;

  VALUE errors = rb_iv_set(self, "@errors", rb_ary_new2(0));

  return self;
}

static void validator_load(validator_t *validator)
{
  validator->ok = validator->start = validator->current = validator->previous = validator->opened;
  validator->len_txt = LEN_T;
  validator->text = (char *)m_alloc(validator->len_txt, sizeof(char));

  validator->text[0] = '\0';
  validator->n_txt = 0;
  validator->mixed = 0;
}

VALUE rb_validator_load_string(VALUE self, VALUE r_str)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  Check_Type(r_str, T_STRING);

  VALUE r_fn = rb_str_new2("");
  validator->fn = RSTRING_PTR(r_fn);

  validator->opened = rnl_s(validator->rnv,
                            validator->rnc_st,
                            validator->rn_st,
                            validator->rnd_st,
                            validator->fn,
                            RSTRING_PTR(r_str), RSTRING_LEN(r_str));

  validator_load(validator);
  return INT2NUM(validator->ok);
}

VALUE rb_validator_load_file(VALUE self, VALUE r_fn)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  switch (TYPE(r_fn))
  {
  case T_STRING:
    validator->fn = RSTRING_PTR(r_fn);

    validator->opened = rnl_fn(validator->rnv,
                               validator->rnc_st,
                               validator->rn_st,
                               validator->rnd_st,
                               validator->fn);

    break;
  case T_FILE: // TODO
  default:
    rb_raise(rb_eTypeError, "invalid argument");
    break;
  }

  validator_load(validator);
  return INT2NUM(validator->ok);
}

VALUE rb_validator_errors(VALUE self)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);
  return rb_iv_get(self, "@errors");
}

static void flush_text(validator_t *validator)
{
  validator->ok = rnv_text(validator->rnv, validator->drv_st, validator->rn_st, validator->rx_st,
                           &validator->current, &validator->previous, validator->text, validator->n_txt, validator->mixed) &&
                  validator->ok;
  validator->text[validator->n_txt = 0] = '\0';
}

VALUE rb_validator_characters(VALUE self, VALUE r_str)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  if (validator->opened && validator->current != validator->rnv->rn_notAllowed)
  {
    Check_Type(r_str, T_STRING);
    char *s = RSTRING_PTR(r_str);
    int len = RSTRING_LEN(r_str);

    int newlen_txt = validator->n_txt + len + 1;
    if (newlen_txt <= LIM_T && LIM_T < validator->len_txt)
      newlen_txt = LIM_T;
    else if (newlen_txt < validator->len_txt)
      newlen_txt = validator->len_txt;
    if (validator->len_txt != newlen_txt)
    {
      validator->text = (char *)m_stretch(validator->text, validator->len_txt = newlen_txt, validator->n_txt, sizeof(char));
    }

    memcpy(validator->text + validator->n_txt, s, len);
    validator->n_txt += len;
    validator->text[validator->n_txt] = '\0'; /* '\0' guarantees that the text is bounded, and strto[ld] work for data */
  }

  return INT2NUM(validator->ok);
}

VALUE rb_validator_start_tag(VALUE self, VALUE r_name, VALUE r_attrs)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  if (validator->opened && validator->current != validator->rnv->rn_notAllowed)
  {
    char *name;
    char **attrs;

    Check_Type(r_name, T_STRING);
    name = RSTRING_PTR(r_name);

    Check_Type(r_attrs, T_ARRAY);
    unsigned int attrs_len = RARRAY_LEN(r_attrs);

    attrs = malloc(sizeof(char *) * (attrs_len + 1));

    for (int i = 0; i < attrs_len; i++)
    {
      attrs[i] = RSTRING_PTR(rb_ary_entry(r_attrs, i));
    }
    attrs[attrs_len] = 0; // zero terminated

    validator->mixed = 1;

    flush_text(validator);
    //printf("RNV START %d/%d %s %d\n", current, previous, name, attrs_len);
    validator->ok = rnv_start_tag(validator->rnv, validator->drv_st, validator->rn_st, validator->rx_st,
                                  &validator->current, &validator->previous, (char *)name, (char **)attrs) &&
                    validator->ok;

    validator->mixed = 0;
    free(attrs);
  }

  return INT2NUM(validator->ok);
}

VALUE rb_validator_end_tag(VALUE self, VALUE r_name)
{
  validator_t *validator;
  Data_Get_Struct(self, validator_t, validator);

  if (validator->opened && validator->current != validator->rnv->rn_notAllowed)
  {
    char *name;

    Check_Type(r_name, T_STRING);
    name = RSTRING_PTR(r_name);

    flush_text(validator);

    //printf("RNV END %d/%d %s\n", current, previous, name);
    validator->ok = rnv_end_tag(validator->rnv, validator->drv_st, validator->rn_st,
                                &validator->current, &validator->previous, (char *)name) &&
                    validator->ok;

    validator->mixed = 1;
  }

  return INT2NUM(validator->ok);
}

// The initialization method for this module
void Init_rnv()
{
  VALUE RNV = rb_define_module("RNV");

  VALUE Validator = rb_define_class_under(RNV, "Validator", rb_cObject);

  rb_define_alloc_func(Validator, rb_validator_alloc);
  rb_define_method(Validator, "initialize", rb_validator_init, 0);

  rb_define_method(Validator, "load_file", rb_validator_load_file, 1);
  rb_define_method(Validator, "load_string", rb_validator_load_string, 1);
  rb_define_method(Validator, "errors", rb_validator_errors, 0);
  rb_define_method(Validator, "start_tag", rb_validator_start_tag, 2);
  rb_define_method(Validator, "characters", rb_validator_characters, 1);
  rb_define_method(Validator, "end_tag", rb_validator_end_tag, 1);

  rb_define_attr(Validator, "last_line", 1, 1);
  rb_define_attr(Validator, "last_col", 1, 1);
}
