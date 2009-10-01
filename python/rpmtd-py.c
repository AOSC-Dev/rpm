/** \ingroup py_c
 * \file python/rpmtd-py.c
 */

#include "rpmsystem-py.h"
#include <rpm/rpmtd.h>
#include <rpm/header.h>
#include "rpmtd-py.h"
#include "header-py.h"

/*
 * Convert single tag data item to python object of suitable type
 */
static PyObject * rpmtd_ItemAsPyobj(rpmtd td, rpmTagClass class)
{
    PyObject *res = NULL;

    switch (class) {
    case RPM_STRING_CLASS:
	res = PyString_FromString(rpmtdGetString(td));
	break;
    case RPM_NUMERIC_CLASS:
	res = PyLong_FromLongLong(rpmtdGetNumber(td));
	break;
    case RPM_BINARY_CLASS:
	res = PyString_FromStringAndSize(td->data, td->count);
	break;
    default:
	PyErr_SetString(PyExc_KeyError, "unknown data type");
	break;
    }
    return res;
}

PyObject *rpmtd_AsPyobj(rpmtd td)
{
    PyObject *res = NULL;
    rpmTagType type = rpmTagGetType(td->tag);
    int array = ((type & RPM_MASK_RETURN_TYPE) == RPM_ARRAY_RETURN_TYPE);
    rpmTagClass class = rpmtdClass(td);

    if (!array && rpmtdCount(td) < 1) {
	Py_RETURN_NONE;
    }
    
    if (array) {
	res = PyList_New(0);
	while (rpmtdNext(td) >= 0) {
	    PyList_Append(res, rpmtd_ItemAsPyobj(td, class));
	}
    } else {
	res = rpmtd_ItemAsPyobj(td, class);
    }
    return res;
}

struct rpmtdObject_s {
    PyObject_HEAD
    PyObject *md_dict;
    struct rpmtd_s td;
};

/* string format should never fail but do regular repr just in case it does */
static PyObject *rpmtd_str(rpmtdObject *s)
{
    PyObject *res = NULL;
    char *str = rpmtdFormat(&(s->td), RPMTD_FORMAT_STRING, NULL);
    if (str) {
        res = PyString_FromString(str);
	free(str);
    } else {
	res = PyObject_Repr((PyObject *)s);
    }
    return res;
}

static PyObject *rpmtd_iternext(rpmtdObject *s)
{
    PyObject *next = NULL;

    if (rpmtdNext(&(s->td)) >= 0) {
        Py_INCREF(s);
        next = (PyObject*) s;
    }
    return next;
}

static PyObject *rpmtd_new(PyTypeObject * subtype, PyObject *args, PyObject *kwds)
{
    rpmtdObject *s = NULL;
    Header h = NULL;
    rpmTag tag;
    int raw = 0;
    int noext = 0;
    headerGetFlags flags = (HEADERGET_EXT | HEADERGET_ALLOC);
    char *kwlist[] = { "header", "tag", "raw", "noext", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O&|ii", kwlist,
			hdrFromPyObject, &h, tagNumFromPyObject, &tag,
			&raw, &noext))
	return NULL;

    if (raw) {
	flags |= HEADERGET_RAW;
	noext = 1; /* extensions with raw dont make sense */
    }
    if (noext) flags &= ~HEADERGET_EXT;

    if ((s = (rpmtdObject *)subtype->tp_alloc(subtype, 0)) == NULL)
	return PyErr_NoMemory();

    headerGet(h, tag, &(s->td), flags);

    return (PyObject *) s;
}

static void rpmtd_dealloc(rpmtdObject * s)
{
    rpmtdFreeData(&(s->td));
    s->ob_type->tp_free((PyObject *)s);
}

static int rpmtd_length(rpmtdObject *s)
{
    return rpmtdCount(&(s->td));
}

static PyMappingMethods rpmtd_as_mapping = {
    (lenfunc) rpmtd_length,             /* mp_length */
};

static PyMemberDef rpmtd_members[] = {
    { "tag", T_INT, offsetof(rpmtdObject, td.tag), READONLY, NULL },
    { "type", T_INT, offsetof(rpmtdObject, td.type), READONLY, NULL },
    { NULL }
};

PyTypeObject rpmtd_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/* ob_size */
	"rpm.td",			/* tp_name */
	sizeof(rpmtdObject),		/* tp_size */
	0,				/* tp_itemsize */
	(destructor) rpmtd_dealloc, 	/* tp_dealloc */
	0,				/* tp_print */
	(getattrfunc)0, 		/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	&rpmtd_as_mapping,		/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	(reprfunc)rpmtd_str,		/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	PyObject_GenericSetAttr,	/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/* tp_flags */
	0,				/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	PyObject_SelfIter,		/* tp_iter */
	(iternextfunc)rpmtd_iternext,	/* tp_iternext */
	0,				/* tp_methods */
	rpmtd_members,			/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	rpmtd_new,			/* tp_new */
	0,				/* tp_free */
	0,				/* tp_is_gc */
};
