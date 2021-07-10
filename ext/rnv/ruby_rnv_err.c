#include "ruby_rnv.h"

extern VALUE RNV, SchemaNotLoaded, Error, DataTypeLibrary, Document;

static VALUE ruby_nc2arr(rnv_t *rnv, int nc);

static VALUE ruby_p2arr(rnv_t *rnv, int p)
{
    ID id;
    VALUE a1;
    VALUE arr = rb_ary_new2(0);

    int dt, ps, val, nc, p1;
    switch (RN_P_TYP(p))
    {
    case RN_P_ERROR:
        id = rb_intern("rn_p_error");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_NOT_ALLOWED:
        id = rb_intern("rn_p_not_allowed");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_EMPTY:
        id = rb_intern("rn_p_empty");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_TEXT:
        id = rb_intern("rn_p_text");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_CHOICE:
        id = rb_intern("rn_p_choice");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_INTERLEAVE:
        id = rb_intern("rn_p_interleave");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_GROUP:
        id = rb_intern("rn_p_group");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_ONE_OR_MORE:
        id = rb_intern("rn_p_one_or_more");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_LIST:
        id = rb_intern("rn_p_list");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_DATA:
        id = rb_intern("rn_p_data");
        rn_Data(p, dt, ps);

        a1 = ruby_nc2arr(rnv, dt);
        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, a1);

        break;
    case RN_P_DATA_EXCEPT:
        id = rb_intern("rn_p_data_except");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_VALUE:
        id = rb_intern("rn_p_value");
        rn_Value(p, dt, val);

        a1 = ruby_nc2arr(rnv, dt);
        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, a1);
        rb_ary_push(arr, rb_str_new2(rnv->rn_string + val));

        break;
    case RN_P_ATTRIBUTE:
        id = rb_intern("rn_p_attribute");
        rn_Attribute(p, nc, p1);

        a1 = ruby_nc2arr(rnv, nc);
        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, a1);

        break;
    case RN_P_ELEMENT:
        id = rb_intern("rn_p_element");
        rn_Element(p, nc, p1);

        a1 = ruby_nc2arr(rnv, nc);
        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, a1);

        break;
    case RN_P_REF:
        id = rb_intern("rn_p_ref");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_P_AFTER:
        id = rb_intern("rn_p_after");
        rb_ary_push(arr, ID2SYM(id));
        break;
    default:
        assert(0);
    }
    return arr;
}

static VALUE ruby_nc2arr(rnv_t *rnv, int nc)
{
    ID id;
    VALUE a1, a2;
    VALUE arr = rb_ary_new2(0);

    int nc1, nc2, uri, name;
    switch (RN_NC_TYP(nc))
    {
    case RN_NC_ERROR:
        id = rb_intern("rn_nc_error");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_NC_NSNAME:
        id = rb_intern("rn_nc_nsname");
        rn_NsName(nc, uri);

        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, rb_str_new2(rnv->rn_string + uri));

        break;
    case RN_NC_QNAME:
        id = rb_intern("rn_nc_qname");
        rn_QName(nc, uri, name);

        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, rb_str_new2(rnv->rn_string + uri));
        rb_ary_push(arr, rb_str_new2(rnv->rn_string + name));

        break;
    case RN_NC_ANY_NAME:
        id = rb_intern("rn_nc_any_name");
        rb_ary_push(arr, ID2SYM(id));
        break;
    case RN_NC_EXCEPT:
        id = rb_intern("rn_nc_except");
        rn_NameClassExcept(nc, nc1, nc2);

        a1 = ruby_nc2arr(rnv, nc1);
        a2 = ruby_nc2arr(rnv, nc2);

        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, a1);
        rb_ary_push(arr, a2);

        break;
    case RN_NC_CHOICE:
        id = rb_intern("rn_nc_choice");
        rn_NameClassChoice(nc, nc1, nc2);

        a1 = ruby_nc2arr(rnv, nc1);
        a2 = ruby_nc2arr(rnv, nc2);

        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, a1);
        rb_ary_push(arr, a2);

        break;
    case RN_NC_DATATYPE:
        id = rb_intern("rn_nc_datatype");
        rn_Datatype(nc, uri, name);

        rb_ary_push(arr, ID2SYM(id));
        rb_ary_push(arr, rb_str_new2(rnv->rn_string + uri));
        rb_ary_push(arr, rb_str_new2(rnv->rn_string + name));

        break;
    default:
        assert(0);
    }

    return arr;
}

// convert error code to symbol
static void ruby_parse_error(VALUE err_obj, int erno, va_list ap)
{
    va_list lap;
    ID id;

    if (ap)
        va_copy(lap, ap);

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
    {
        char *ns_uri = va_arg(lap, char *);
        char *tag = va_arg(lap, char *);

        if (ns_uri)
            rb_iv_set(err_obj, "@ns_uri", rb_str_new2(ns_uri));

        rb_iv_set(err_obj, "@element", rb_str_new2(tag));

        id = rb_intern("rnv_er_elem");
    }
    break;
    case (ERBIT_RNV | RNV_ER_AKEY):
    {
        char *ns_uri = va_arg(lap, char *);
        char *attr = va_arg(lap, char *);

        if (ns_uri)
            rb_iv_set(err_obj, "@ns_uri", rb_str_new2(ns_uri));

        rb_iv_set(err_obj, "@attr", rb_str_new2(attr));

        id = rb_intern("rnv_er_akey");
    }
    break;
    case (ERBIT_RNV | RNV_ER_AVAL):
    {
        char *ns_uri = va_arg(lap, char *);
        char *attr = va_arg(lap, char *);
        char *val = va_arg(lap, char *);

        if (ns_uri)
            rb_iv_set(err_obj, "@ns_uri", rb_str_new2(ns_uri));

        rb_iv_set(err_obj, "@attr", rb_str_new2(attr));
        rb_iv_set(err_obj, "@value", rb_str_new2(val));

        id = rb_intern("rnv_er_aval");
    }
    break;
    case (ERBIT_RNV | RNV_ER_EMIS):
        id = rb_intern("rnv_er_emis");
        break;
    case (ERBIT_RNV | RNV_ER_AMIS):
    {
        char *ns_uri = va_arg(lap, char *);
        char *tag = va_arg(lap, char *);

        if (ns_uri)
            rb_iv_set(err_obj, "@ns_uri", rb_str_new2(ns_uri));

        rb_iv_set(err_obj, "@element", rb_str_new2(tag));

        id = rb_intern("rnv_er_amis");
    }
    break;
    case (ERBIT_RNV | RNV_ER_UFIN):
    {
        char *ns_uri = va_arg(lap, char *);
        char *tag = va_arg(lap, char *);

        if (ns_uri)
            rb_iv_set(err_obj, "@ns_uri", rb_str_new2(ns_uri));

        rb_iv_set(err_obj, "@element", rb_str_new2(tag));

        id = rb_intern("rnv_er_ufin");
    }
    break;
    case (ERBIT_RNV | RNV_ER_TEXT):
        id = rb_intern("rnv_er_text");
        break;
    case (ERBIT_RNV | RNV_ER_NOTX):
        id = rb_intern("rnv_er_notx");
        break;
    case (ERBIT_DTL):
        id = rb_intern("dtl");
        break;
    default:
        id = rb_intern("unknown");
        break;
    }

    rb_iv_set(err_obj, "@code", ID2SYM(id));

    //  return id;
}

VALUE ruby_create_error(VALUE self, VALUE line, VALUE col, VALUE xpath, int erno, char *format, va_list ap)
{
    VALUE err_class = Error;
    VALUE err_obj = rb_class_new_instance(0, NULL, err_class);

    ruby_parse_error(err_obj, erno, ap);

    VALUE error_str;

    // do not vsprintf if ap is NULL
    if (ap)
        error_str = rb_vsprintf(format, ap);
    else
        error_str = rb_str_new2(format);

    // lazyly strip with ruby
    rb_funcall(error_str, rb_intern("strip!"), 0);

    rb_iv_set(err_obj, "@document", self);
    rb_iv_set(err_obj, "@original_message", error_str);

    rb_iv_set(err_obj, "@line", line); // set line from sax parser
    rb_iv_set(err_obj, "@col", col);   // set col from sax parser

    rb_iv_set(err_obj, "@xpath", xpath); // set line from sax parser

    rb_iv_set(err_obj, "@required", rb_ary_new2(0));
    rb_iv_set(err_obj, "@allowed", rb_ary_new2(0));
    return err_obj;
}

int ruby_verror_handler(void *data, int erno, char *format, va_list ap)
{
    VALUE self = (VALUE)data;
    document_t *document;

    Data_Get_Struct(self, document_t, document);

    rnv_t *rnv = document->rnv;

    VALUE errors = rb_iv_get(self, "@errors");

    if (erno & ERBIT_RNL || erno & ERBIT_RNC || erno & ERBIT_RND)
    {
        VALUE err_obj = ruby_create_error(self, INT2NUM(-1), INT2NUM(-1), Qnil, erno, format, ap);
        rb_iv_set(err_obj, "@original_expected", rb_str_new2(""));
        rb_ary_push(errors, err_obj);
    }
    else
    {
        VALUE r_last_line = rb_iv_get(self, "@last_line");
        VALUE r_last_col = rb_iv_get(self, "@last_col");
        int last_line = NUM2INT(r_last_line);
        int last_col = NUM2INT(r_last_col);
        int xpath = NUM2INT(r_last_col);

        // only one error per line/col
        if (document->last_line != last_line || document->last_col != last_col)
        {
            document->last_line = last_line;
            document->last_col = last_col;

            VALUE err_obj = ruby_create_error(self, r_last_line, r_last_col, rb_iv_get(self, "@xpath"), erno, format, ap);

            VALUE expected = rb_str_new2("");

            if (erno & ERBIT_RNV)
            {
                if (document->nexp)
                {
                    int req = 2, i = 0;
                    char *s;
                    while (req--)
                    {
                        rnx_expected(rnv, document->previous, req);
                        if (i == rnv->rnx_n_exp)
                            continue;
                        if (rnv->rnx_n_exp > document->nexp)
                            break;

                        expected = rb_str_cat2(expected, (char *)(req ? "required:\n" : "allowed:\n"));
                        VALUE expected_arr = rb_ary_new2(0);

                        for (; i != rnv->rnx_n_exp; ++i)
                        {
                            s = rnx_p2str(rnv, rnv->rnx_exp[i]);
                            expected = rb_str_cat2(expected, "\t");
                            expected = rb_str_cat2(expected, s);
                            expected = rb_str_cat2(expected, "\n");
                            m_free(s);

                            rb_ary_push(expected_arr, ruby_p2arr(rnv, rnv->rnx_exp[i]));
                        }

                        if (req)
                            rb_iv_set(err_obj, "@required", expected_arr);
                        else
                            rb_iv_set(err_obj, "@allowed", expected_arr);
                    }
                }
            }
            rb_iv_set(err_obj, "@original_expected", expected);

            rb_ary_push(errors, err_obj);
        }
    }
}
