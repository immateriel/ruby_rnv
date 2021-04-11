#include "ruby_rnv.h"

#define LEN_T XCL_LEN_T
#define LIM_T XCL_LIM_T

VALUE RNV, SchemaNotLoaded, Error, DataTypeLibrary, Document;

extern int ruby_verror_handler(void *data, int erno, char *format, va_list ap);

VALUE rb_datatype_push_error(VALUE self, VALUE r_msg)
{
  document_t *document;
  VALUE r_doc = rb_iv_get(self, "@document");
  Data_Get_Struct(r_doc, document_t, document);

  Check_Type(r_msg, T_STRING);

  document->rnv->verror_handler((void *)r_doc, ERBIT_DTL, RSTRING_PTR(r_msg), NULL);

  // skip next error since DTL will push his own
  document->skip_next_error = 1;

  return Qnil;
}

void document_free(document_t *document)
{
  rnd_dispose(document->rnd_st);
  rx_dispose(document->rx_st);
  drv_dispose(document->drv_st);

  rnc_dispose(document->rnc_st);

  rn_dispose(document->rn_st);

  rnv_dispose(document->rnv);

  if (document->text)
    free(document->text);

  ruby_xfree(document);
}

VALUE rb_document_alloc(VALUE klass)
{
  document_t *document = ruby_xmalloc(sizeof(document_t));

  document->rnv = calloc(1, sizeof(rnv_t));
  document->rn_st = calloc(1, sizeof(rn_st_t));
  document->rnc_st = calloc(1, sizeof(rnc_st_t));
  document->drv_st = calloc(1, sizeof(drv_st_t));
  document->rx_st = calloc(1, sizeof(rx_st_t));
  document->rnd_st = calloc(1, sizeof(rnd_st_t));

  return Data_Wrap_Struct(klass, NULL, document_free, document);
}

VALUE rb_document_init(VALUE self)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  rnl_init(document->rnv, document->rnc_st, document->rn_st, document->rnd_st);
  rnv_init(document->rnv, document->drv_st, document->rn_st, document->rx_st);
  rnx_init(document->rnv);

  document->opened = document->ok = 0;
  document->skip_next_error = 0;
  
  document->rnv->user_data = (void *)self;
  document->rnv->verror_handler = &ruby_verror_handler;

  document->nexp = 16; /* maximum number of candidates to display */
  document->text = NULL;

  rb_iv_set(self, "@errors", rb_ary_new2(0));

  rb_iv_set(self, "@libraries", rb_hash_new());

  //document->rx_st->rx_compact = 1;
  //document->drv_st->drv_compact = 1;

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
 * @return [Boolean]
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

  if (document->opened)
  {
    document_load(document);
    return Qtrue;
  }
  else
    return Qfalse;
}

/*
 * load schema from a file
 * @param [String] r_fn filename
 * @return [Boolean]
 */
VALUE rb_document_load_file(VALUE self, VALUE r_fn)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  Check_Type(r_fn, T_STRING);

  document->fn = RSTRING_PTR(r_fn);

  document->opened = rnl_fn(document->rnv,
                            document->rnc_st,
                            document->rn_st,
                            document->rnd_st,
                            document->fn);

  if (document->opened)
  {
    document_load(document);
    return Qtrue;
  }
  else
    return Qfalse;
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

static int rb_dtl_equal(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ, char *val, char *s, int n)
{
  VALUE self = (VALUE)rnv->user_data;
  VALUE libraries = rb_iv_get(self, "@libraries");
  VALUE lib = rb_hash_aref(libraries, INT2FIX(uri));

  // do we need n here, do s always null terminated ?
  VALUE ret = rb_funcall(lib, rb_intern("equal"), 3,
                         rb_str_new2(typ), rb_str_new2(val), rb_str_new2(s));

  return RTEST(ret);
}

static int rb_dtl_allows(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ, char *ps, char *s, int n)
{
  VALUE self = (VALUE)rnv->user_data;
  VALUE libraries = rb_iv_get(self, "@libraries");
  VALUE lib = rb_hash_aref(libraries, INT2FIX(uri));

  // do we need n here, do s always null terminated ?
  VALUE ret = rb_funcall(lib, rb_intern("allows"), 3,
                         rb_str_new2(typ), rb_str_new2(ps), rb_str_new2(s));

  return RTEST(ret);
}

/*
 * add a new datatype library
 * @see https://www.oasis-open.org/committees/relax-ng/spec-20010811.html#IDA1I1R
 * @param [String] r_ns unique ns URL for this datatype
 * @param [RNV::DataTypeLibrary] r_cb_obj
 * @return [nil]
 */
VALUE rb_document_add_dtl(VALUE self, VALUE r_ns, VALUE r_cb_obj)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (document->opened)
  {
    Check_Type(r_ns, T_STRING);

    char *suri = RSTRING_PTR(r_ns);

    drv_add_dtl(document->rnv, document->drv_st, document->rn_st, suri, &rb_dtl_equal, &rb_dtl_allows);

    int uri = document->drv_st->dtl[document->drv_st->n_dtl - 1].uri;

    VALUE libraries = rb_iv_get(self, "@libraries");

    rb_iv_set(r_cb_obj, "@document", self);

    rb_hash_aset(libraries, INT2FIX(uri), r_cb_obj);
  }
  return Qnil;
}

static void flush_text(document_t *document)
{
  document->ok = rnv_text(document->rnv, document->drv_st, document->rn_st, document->rx_st,
                          &document->current, &document->previous, document->text, document->n_txt, document->mixed) &&
                 document->ok;
  document->text[document->n_txt = 0] = '\0';
}

/*
 * begin parsing a new document
 * @return [nil]
 */
VALUE rb_document_begin(VALUE self)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (!document->opened)
    rb_raise(SchemaNotLoaded, "schema was not loaded correctly");

  // reset errors
  rb_iv_set(self, "@errors", rb_ary_new2(0));

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
 * @return [Boolean]
 */
VALUE rb_document_characters(VALUE self, VALUE r_str)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (!document->opened)
    rb_raise(SchemaNotLoaded, "schema was not loaded correctly");

  if (document->current != document->rnv->rn_notAllowed)
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

  return RTEST(document->ok);
}

/*
 * to be called by SAX start tag handler
 * @param [String] r_name tag name, must be in the form 'NS_URI:TAG_NAME'
 * @param [Array<Array<String>>] r_attrs flattened array of tag attributes in the form [['NS_URI:ATTR_NAME','ATTR_VALUE']]
 * @return [Boolean]
 */
VALUE rb_document_start_tag(VALUE self, VALUE r_name, VALUE r_attrs)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (!document->opened)
    rb_raise(SchemaNotLoaded, "schema was not loaded correctly");

  if (document->current != document->rnv->rn_notAllowed)
  {
    int i;
    char *name;
    char **attrs;

    Check_Type(r_name, T_STRING);
    name = RSTRING_PTR(r_name);

    Check_Type(r_attrs, T_ARRAY);

    // lazyly flatten with ruby
    VALUE r_flat_attrs = rb_funcall(r_attrs, rb_intern("flatten"), 0);
    unsigned int attrs_len = RARRAY_LEN(r_flat_attrs);

    attrs = malloc(sizeof(char *) * (attrs_len + 1));

    for (i = 0; i < attrs_len; i++)
    {
      attrs[i] = RSTRING_PTR(rb_ary_entry(r_flat_attrs, i));
    }
    attrs[attrs_len] = 0; // zero terminated

    document->mixed = 1;

    flush_text(document);
    document->ok = rnv_start_tag(document->rnv, document->drv_st, document->rn_st, document->rx_st,
                                 &document->current, &document->previous, (char *)name, (char **)attrs) &&
                   document->ok;

    document->mixed = 0;
    free(attrs);
  }

  return RTEST(document->ok);
}

/*
 * to be called by SAX end tag handler
 * @param [String] r_name tag name, must be in the form 'NS_URI:TAG_NAME'
 * @return [Boolean]
 */
VALUE rb_document_end_tag(VALUE self, VALUE r_name)
{
  document_t *document;
  Data_Get_Struct(self, document_t, document);

  if (!document->opened)
    rb_raise(SchemaNotLoaded, "schema was not loaded correctly");

  if (document->current != document->rnv->rn_notAllowed)
  {
    char *name;

    Check_Type(r_name, T_STRING);
    name = RSTRING_PTR(r_name);

    flush_text(document);

    document->ok = rnv_end_tag(document->rnv, document->drv_st, document->rn_st,
                               &document->current, &document->previous, (char *)name) &&
                   document->ok;

    document->mixed = 1;
  }

  return RTEST(document->ok);
}

// The initialization method for this module
void Init_rnv()
{
  RNV = rb_define_module("RNV");

  SchemaNotLoaded = rb_define_class_under(RNV, "SchemaNotLoaded", rb_eStandardError);

  Error = rb_define_class_under(RNV, "Error", rb_cObject);

  /*
   * error symbol code
   * @return [Symbol]
   */
  rb_define_attr(Error, "code", 1, 0);
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

  DataTypeLibrary = rb_define_class_under(RNV, "DataTypeLibrary", rb_cObject);

  /*
   * document in which library is registered
   * @return [RNV::Document]
   */
  rb_define_attr(DataTypeLibrary, "document", 1, 0);

  rb_define_method(DataTypeLibrary, "push_error", rb_datatype_push_error, 1);

  Document = rb_define_class_under(RNV, "Document", rb_cObject);

  rb_define_alloc_func(Document, rb_document_alloc);
  rb_define_method(Document, "initialize", rb_document_init, 0);

  rb_define_method(Document, "load_file", rb_document_load_file, 1);
  rb_define_method(Document, "load_string", rb_document_load_string, 1);
  rb_define_method(Document, "valid?", rb_document_valid, 0);

  rb_define_method(Document, "add_datatype_library", rb_document_add_dtl, 2);

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
