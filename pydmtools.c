#include <Python.h>
#include <inttypes.h>
#include "pydmtools.h"

#ifdef WITHNUMPY
#include <float.h>
#include "numpy/npy_common.h"
#include "numpy/halffloat.h"
#include "numpy/ndarrayobject.h"
#include "numpy/arrayscalars.h"

int lsize = NPY_SIZEOF_LONG;

//Raises an exception on error, which should be checked
uint32_t getNumpyU32(PyArrayObject *obj, Py_ssize_t i) {
    int dtype;
    char *p;
    uint32_t o = 0;
    npy_intp stride;

    //Get the dtype
    dtype = PyArray_TYPE(obj);
    //Get the stride
    stride = PyArray_STRIDE(obj, 0);
    p = PyArray_BYTES(obj) + i*stride;

    switch(dtype) {
    case NPY_INT8:
        if(((int8_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int8_t *) p)[0];
        break;
    case NPY_INT16:
        if(((int16_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int16_t *) p)[0];
        break;
    case NPY_INT32:
        if(((int32_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int32_t *) p)[0];
        break;
    case NPY_INT64:
        if(((int64_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int64_t *) p)[0];
        break;
    case NPY_UINT8:
        o += ((uint8_t *) p)[0];
        break;
    case NPY_UINT16:
        o += ((uint16_t *) p)[0];
        break;
    case NPY_UINT32:
        o += ((uint32_t *) p)[0];
        break;
    case NPY_UINT64:
        if(((uint64_t *) p)[0] > (uint32_t) -1) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer larger than possible for a 32bit unsigned integer!\n");
            goto error;
        }
        o += ((uint64_t *) p)[0];
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Received unknown data type for conversion to uint32_t!\n");
        goto error;
        break;
    }
    return o;

error:
    return 0;
};

long getNumpyL(PyObject *obj) {
    short s;
    int i;
    long l;
    long long ll;
    unsigned short us;
    unsigned int ui;
    unsigned long ul;
    unsigned long long ull;
    
    if(!PyArray_IsIntegerScalar(obj)) {
        PyErr_SetString(PyExc_RuntimeError, "Received non-Integer scalar type for conversion to long!\n");
        return 0;
    }

    if(PyArray_IsScalar(obj, Short)) {
        s = ((PyShortScalarObject *)obj)->obval;
        l = s;
    } else if(PyArray_IsScalar(obj, Int)) {
        i = ((PyLongScalarObject *)obj)->obval;
        l = i;
    } else if(PyArray_IsScalar(obj, Long)) {
        l = ((PyLongScalarObject *)obj)->obval;
    } else if(PyArray_IsScalar(obj, LongLong)) {
        ll = ((PyLongScalarObject *)obj)->obval;
        l = ll;
    } else if(PyArray_IsScalar(obj, UShort)) {
        us = ((PyLongScalarObject *)obj)->obval;
        l = us;
    } else if(PyArray_IsScalar(obj, UInt)) {
        ui = ((PyLongScalarObject *)obj)->obval;
        l = ui;
    } else if(PyArray_IsScalar(obj, ULong)) {
        ul = ((PyLongScalarObject *)obj)->obval;
        l = ul;
    } else if(PyArray_IsScalar(obj, ULongLong)) {
        ull = ((PyLongScalarObject *)obj)->obval;
        l = ull;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "Received unknown scalar type for conversion to long!\n");
        return 0;
    }

    return l;
}

//Raises an exception on error, which should be checked
float getNumpyF(PyArrayObject *obj, Py_ssize_t i) {
    int dtype;
    char *p;
    float o = 0.0;
    npy_intp stride;

    //Get the dtype
    dtype = PyArray_TYPE(obj);
    //Get the stride
    stride = PyArray_STRIDE(obj, 0);
    p = PyArray_BYTES(obj) + i*stride;

    switch(dtype) {
    case NPY_FLOAT16:
        return npy_half_to_float(((npy_half*)p)[0]);
    case NPY_FLOAT32:
        return ((float*)p)[0];
    case NPY_FLOAT64:
        if(((double*)p)[0] > FLT_MAX) {
            PyErr_SetString(PyExc_RuntimeError, "Received a floating point value greater than possible for a 32-bit float!\n");
            goto error;
        }
        if(((double*)p)[0] < -FLT_MAX) {
            PyErr_SetString(PyExc_RuntimeError, "Received a floating point value less than possible for a 32-bit float!\n");
            goto error;
        }
        o += ((double*)p)[0];
        return o;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Received unknown data type for conversion to float!\n");
        goto error;
        break;
    }
    return o;

error:
    return 0;
}

//The calling function needs to free the result
char *getNumpyStr(PyArrayObject *obj, Py_ssize_t i) {
    char *p , *o = NULL;
    npy_intp stride, j;
    int dtype;

    //Get the dtype
    dtype = PyArray_TYPE(obj);
    //Get the stride
    stride = PyArray_STRIDE(obj, 0);
    p = PyArray_BYTES(obj) + i*stride;

    switch(dtype) {
    case NPY_STRING:
        o = calloc(1, stride + 1);
        strncpy(o, p, stride);
        return o;
    case NPY_UNICODE:
        o = calloc(1, stride/4 + 1);
        for(j=0; j<stride/4; j++) o[j] = (char) ((uint32_t*)p)[j];
        return o;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Received unknown data type!\n");
        break;
    }
    return NULL;
}
#endif

//Return 1 if there are any entries at all
int hasEntries(binaMethFile_t *bm) {
    if(bm->hdr->indexOffset != 0) return 1;  // No index, no entries pybinaMeth issue #111
    //if(bm->hdr->nBasesCovered > 0) return 1;  // Sometimes headers are broken
    return 0;
}

PyObject* pyBmEnter(pybinaMethFile_t*self, PyObject *args) {
    binaMethFile_t *bm = self->bm;

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not opened!");
        return NULL;
    }

    Py_INCREF(self);

    return (PyObject*) self;
}

PyObject* pyBmOpen(PyObject *self, PyObject *args, PyObject *kwds) {
    char *fname = NULL;
    char *mode = "r";
    pybinaMethFile_t *pybm;
    binaMethFile_t *bm = NULL;
    uint32_t write_type = 0x8000;
    char *pcover = "N", *pstrand = "N", *pcontext = "N", *pend = "Y", *pID = "N";
    static char *kwd_list[] = {"fname", "mode", "end", "cover", "strand", "context", "ID", NULL};

    //if(!PyArg_ParseTuple(pyFname, "s|ssssss", &fname, &mode, &pend, &pcover, &pstrand, &pcontext, &pID)) goto error;
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|ssssss", kwd_list, &fname, &mode, &pend, &pcover, &pstrand, &pcontext, &pID)) goto error;
    //Open the local/remote file
    if(strchr(mode, 'w') != NULL || bmIsBinaMeth(fname, NULL)) {
        bm = bmOpen(fname, NULL, mode);
    } else {
        fprintf(stderr, "Please check the mode and filename!\n");
        //bm = bbOpen(fname, NULL);
    }

    if(!bm) {
        fprintf(stderr, "[pyBmOpen] bm is NULL!\n");
        goto error;
    }
    if(!mode || !strchr(mode, 'w')) {
        if(!bm->cl) goto error;
    }

    pybm = PyObject_New(pybinaMethFile_t, &binaMethFile);
    if(!pybm) {
        fprintf(stderr, "[pyBmOpen] PyObject_New() returned NULL (out of memory?)!\n");
        goto error;
    }
    
    if(!mode || !strchr(mode, 'w')) {
        //change type to version
        bm->type = bm->hdr->version;
    }else{
        if(strcmp(pcover, "Y") == 0){
            write_type |= BM_COVER;
        }else if(strcmp(pstrand, "Y") == 0){
            write_type |= BM_STRAND;
        }else if(strcmp(pcontext, "Y") == 0){
            write_type |= BM_CONTEXT;
        }else if(strcmp(pID, "Y") == 0){
            write_type |= BM_ID;
        }else if(strcmp(pend, "Y") == 0){
            write_type |= BM_END;
        }
        bm->type = write_type;
    }
//fprintf(stderr, "type %d, %s %s %s %s %s\n", bm->type, pcover, pstrand, pcontext, pID, pend);

    pybm->bm = bm;
    pybm->lastTid = -1;
    pybm->lastType = -1;
    pybm->lastSpan = (uint32_t) -1;
    pybm->lastStep = (uint32_t) -1;
    pybm->lastStart = (uint32_t) -1;
    return (PyObject*) pybm;

error:
    if(bm) bmClose(bm);
    PyErr_SetString(PyExc_RuntimeError, "Received an error during file opening!");
    return NULL;
}

static void pyBmDealloc(pybinaMethFile_t *self) {
    if(self->bm) bmClose(self->bm);
    PyObject_DEL(self);
}

static PyObject *pyBmClose(pybinaMethFile_t *self, PyObject *args) {
    bmClose(self->bm);
    self->bm = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

//Accessor for the header (version, nLevels, nBasesCovered, minVal, maxVal, sumData, sumSquared
static PyObject *pyBmGetHeader(pybinaMethFile_t *self, PyObject *args) {
    binaMethFile_t *bm = self->bm;
    PyObject *ret, *val;

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not opened!");
        return NULL;
    }
    if(bm->isWrite == 1) {
        PyErr_SetString(PyExc_RuntimeError, "The header cannot be accessed in files opened for writing!");
        return NULL;
    }

    ret = PyDict_New();
    val = PyLong_FromUnsignedLong(bm->hdr->version);
    if(PyDict_SetItemString(ret, "version", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromUnsignedLong(bm->hdr->nLevels);
    if(PyDict_SetItemString(ret, "nLevels", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromUnsignedLongLong(bm->hdr->nBasesCovered);
    if(PyDict_SetItemString(ret, "nBasesCovered", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromDouble(bm->hdr->minVal);
    if(PyDict_SetItemString(ret, "minVal", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromDouble(bm->hdr->maxVal);
    if(PyDict_SetItemString(ret, "maxVal", val) == -1) goto error;
    Py_DECREF(val);
    val = PyFloat_FromDouble(bm->hdr->sumData);
    if(PyDict_SetItemString(ret, "sumData", val) == -1) goto error;
    Py_DECREF(val);
    val = PyFloat_FromDouble(bm->hdr->sumSquared);
    if(PyDict_SetItemString(ret, "sumSquared", val) == -1) goto error;
    Py_DECREF(val);

    return ret;

error :
    Py_XDECREF(val);
    Py_XDECREF(ret);
    PyErr_SetString(PyExc_RuntimeError, "Received an error while getting the binaMeth header!");
    return NULL;
}

//Accessor for the chroms, args is optional
static PyObject *pyBmGetChroms(pybinaMethFile_t *self, PyObject *args) {
    PyObject *ret = NULL, *val;
    binaMethFile_t *bm = self->bm;
    char *chrom = NULL;
    uint32_t i;

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not opened!");
        return NULL;
    }

    if(bm->isWrite == 1) {
        PyErr_SetString(PyExc_RuntimeError, "Chromosomes cannot be accessed in files opened for writing!");
        return NULL;
    }

    if(!(PyArg_ParseTuple(args, "|s", &chrom)) || !chrom) {
        ret = PyDict_New();
        for(i=0; i<bm->cl->nKeys; i++) {
            val = PyLong_FromUnsignedLong(bm->cl->len[i]);
            if(PyDict_SetItemString(ret, bm->cl->chrom[i], val) == -1) goto error;
            Py_DECREF(val);
        }
    } else {
        for(i=0; i<bm->cl->nKeys; i++) {
            if(strcmp(bm->cl->chrom[i],chrom) == 0) {
                ret = PyLong_FromUnsignedLong(bm->cl->len[i]);
                break;
            }
        }
    }

    if(!ret) {
        Py_INCREF(Py_None);
        ret = Py_None;
    }

    return ret;

error :
    Py_XDECREF(val);
    Py_XDECREF(ret);
    PyErr_SetString(PyExc_RuntimeError, "Received an error while adding an item to the output dictionary!");
    return NULL;
}

enum bmStatsType char2enum(char *s) {
    if(strcmp(s, "mean") == 0) return mean;
    if(strcmp(s, "std") == 0) return stdev;
    if(strcmp(s, "dev") == 0) return dev;
    if(strcmp(s, "max") == 0) return max;
    if(strcmp(s, "min") == 0) return min;
    if(strcmp(s, "cov") == 0) return cov;
    if(strcmp(s, "coverage") == 0) return cov;
    if(strcmp(s, "sum") == 0) return sum;
    if(strcmp(s, "weight") == 0) return weighted;
    return -1;
};

enum bmStatsType strand2enum(char *s) {
    if(strcmp(s, ".") == 0) return 2;
    if(strcmp(s, "+") == 0) return 0;
    if(strcmp(s, "-") == 0) return 1;
    return -1;
};

enum bmStatsType context2enum(char *s) {
    if(strcmp(s, "C") == 0) return 0;
    if(strcmp(s, "ALL") == 0) return 0;
    if(strcmp(s, "CG") == 0) return 1;
    if(strcmp(s, "CHG") == 0) return 2;
    if(strcmp(s, "CHH") == 0) return 3;
    return -1;
};

//Fetch summary statistics, default is the mean of the entire chromosome.
static PyObject *pyBmGetStats(pybinaMethFile_t *self, PyObject *args, PyObject *kwds) {
    binaMethFile_t *bm = self->bm;
    double *val;
    uint32_t start, end = -1, tid;
    unsigned long startl = 0, endl = -1, nBinsl = 1, stepl = 0;
    static char *kwd_list[] = {"chrom", "start", "end", "type", "nBins", "step", "strand", "context", "exact", "numpy", NULL};
    char *chrom, *type = "mean", *strand=".", *context="C";
    PyObject *ret, *exact = Py_False, *starto = NULL, *endo = NULL, *nBinso = NULL, *stepo = NULL;
    PyObject *outputNumpy = Py_False;
    int i;
    uint32_t nBins = 1, step = 0;
    errno = 0; //In the off-chance that something elsewhere got an error and didn't clear it...

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not open!");
        return NULL;
    }

    if(bm->isWrite == 1) {
        PyErr_SetString(PyExc_RuntimeError, "Statistics cannot be accessed in files opened for writing!");
        return NULL;
    }

    if(bm->type == 1) {
        PyErr_SetString(PyExc_RuntimeError, "bigBed files have no statistics!");
        return NULL;
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|OOsOOssOO", kwd_list, &chrom, &starto, &endo, &type, &nBinso, &stepo, &strand, &context, &exact, &outputNumpy)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply at least a chromosome!");
        return NULL;
    }

    //Check inputs, reset to defaults if nothing was input
    //if(!nBins) nBins = 1; //For some reason, not specifying this overrides the default!
    if(!type) type = "mean";
    tid = bmGetTid(bm, chrom);

    if(starto) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(starto, Integer)) {
            startl = (long) getNumpyL(starto);
        } else 
#endif
        if(PyLong_Check(starto)) {
            startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(starto)) {
            startl = PyInt_AsLong(starto);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
            return NULL;
        }
    }

    if(endo) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(endo, Integer)) {
            endl = (long) getNumpyL(endo);
        } else 
#endif
        if(PyLong_Check(endo)) {
            endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(endo)) {
            endl = PyInt_AsLong(endo);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
            return NULL;
        }
    }

        if(nBinso) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(nBinso, Integer)) {
            nBinsl = (long) getNumpyL(nBinso);
        } else 
#endif
        if(PyLong_Check(nBinso)) {
            nBinsl = PyLong_AsLong(nBinso);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(nBinso)) {
            nBinsl = PyInt_AsLong(nBinso);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The nBins coordinate must be a number!");
            return NULL;
        }
    }

        if(stepo) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(stepo, Integer)) {
            stepl = (long) getNumpyL(stepo);
        } else 
#endif
        if(PyLong_Check(stepo)) {
            stepl = PyLong_AsLong(stepo);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(stepo)) {
            stepl = PyInt_AsLong(stepo);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The step coordinate must be a number!");
            return NULL;
        }
    }

    if(endl == (unsigned long) -1 && tid != (uint32_t) -1) endl = bm->cl->len[tid];
    if(tid == (uint32_t) -1 || startl > end || endl > end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }
    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bm->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    nBins = (uint32_t) nBinsl;
    step = (uint32_t) stepl;

    if(char2enum(type) == doesNotExist) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid type!");
        return NULL;
    }

    //Return a list of None if there are no entries at all
    if(!hasEntries(bm)) {
#ifdef WITHNUMPY
        if(outputNumpy == Py_True) {
            val = malloc(sizeof(double)*nBinsl);
            for(i=0; i<nBinsl; i++) {
                val[i] = NPY_NAN;
            }
            npy_intp len = nBinsl;
            ret = PyArray_SimpleNewFromData(1, &len, NPY_FLOAT64, (void *) val);
            //This will break if numpy ever stops using malloc!
            PyArray_ENABLEFLAGS((PyArrayObject*) ret, NPY_ARRAY_OWNDATA);
        } else {
#endif
            ret = PyList_New(nBinsl);
            for(i=0; i<nBinsl; i++) {
                Py_INCREF(Py_None);
                PyList_SetItem(ret, i, Py_None);
            }
#ifdef WITHNUMPY
        }
#endif
        return ret;
    }

    uint8_t strandt = (uint8_t) strand2enum(strand);
    uint8_t contextt = (uint8_t) context2enum(context);
    //Get the actual statistics
    if(exact == Py_True) {
        val = bmStatsFromFull(bm, chrom, start, end, nBins, step, char2enum(type), strandt, contextt);
    } else {
        val = bmStats(bm, chrom, start, end, nBins, step, char2enum(type), strandt, contextt); 
    }

    if(!val) {
        PyErr_SetString(PyExc_RuntimeError, "An error was encountered while fetching statistics.");
        return NULL;
    }

#ifdef WITHNUMPY
    if(outputNumpy == Py_True) {
        npy_intp len = nBins;
        ret = PyArray_SimpleNewFromData(1, &len, NPY_FLOAT64, (void *) val);
        //This will break if numpy ever stops using malloc!
        PyArray_ENABLEFLAGS((PyArrayObject*) ret, NPY_ARRAY_OWNDATA);
    } else {
#endif
        ret = PyList_New(nBins);
        for(i=0; i<nBins; i++) {
            if(isnan(val[i])) {
                Py_INCREF(Py_None);
                PyList_SetItem(ret, i, Py_None);
            } else {
                PyList_SetItem(ret, i, PyFloat_FromDouble(val[i]));
            }
        }
        free(val);
#ifdef WITHNUMPY
    }
#endif
    return ret;
}

//Fetch a list of individual values
//For bases with no coverage, the value should be None
#ifdef WITHNUMPY
static PyObject *pyBmGetValues(pybinaMethFile_t *self, PyObject *args, PyObject *kwds) {
#else
static PyObject *pyBmGetValues(pybinaMethFile_t *self, PyObject *args) {
#endif
    binaMethFile_t *bm = self->bm;
    int i;
    uint32_t start, end = -1, tid;
    unsigned long startl, endl;
    char *chrom;
    PyObject *ret, *starto = NULL, *endo = NULL;
    bmOverlappingIntervals_t *o;

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not open!");
        return NULL;
    }

    if(bm->type == 1) {
        PyErr_SetString(PyExc_RuntimeError, "bigBed files have no values! Use 'entries' instead.");
        return NULL;
    }

#ifdef WITHNUMPY
    static char *kwd_list[] = {"chrom", "start", "end", "numpy", NULL};
    PyObject *outputNumpy = Py_False;

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "sOO|O", kwd_list, &chrom, &starto, &endo, &outputNumpy)) {
#else
    if(!PyArg_ParseTuple(args, "sOO", &chrom, &starto, &endo)) {
#endif
        PyErr_SetString(PyExc_RuntimeError, "You must supply a chromosome, start and end position.\n");
        return NULL;
    }

    tid = bmGetTid(bm, chrom);

#ifdef WITHNUMPY
    if(PyArray_IsScalar(starto, Integer)) {
        startl = (long) getNumpyL(starto);
    } else
#endif
    if(PyLong_Check(starto)) {
        startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
    } else if(PyInt_Check(starto)) {
        startl = PyInt_AsLong(starto);
#endif
    } else {
        PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
        return NULL;
    }

#ifdef WITHNUMPY
    if(PyArray_IsScalar(endo, Integer)) {
        endl = (long) getNumpyL(endo);
    } else
#endif
    if(PyLong_Check(endo)) {
        endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
    } else if(PyInt_Check(endo)) {
        endl = PyInt_AsLong(endo);
#endif
    } else {
        PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
        return NULL;
    }

    if(endl == (unsigned long) -1 && tid != (uint32_t) -1) endl = bm->cl->len[tid];
    if(tid == (uint32_t) -1 || startl > end || endl > end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bm->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    if(!hasEntries(self->bm)) {
#ifdef WITHNUMPY
        if(outputNumpy == Py_True) {
            return PyArray_SimpleNew(0, NULL, NPY_FLOAT);
        } else {
#endif
            return PyList_New(0);
#ifdef WITHNUMPY
        }
#endif
    }

    //o = bmGetValues(self->bm, chrom, start, end, 1);
    o = bmGetOverlappingIntervals(self->bm, chrom, start, end);
    if(!o) {
        PyErr_SetString(PyExc_RuntimeError, "An error occurred while fetching values!");
        return NULL;
    }

#ifdef WITHNUMPY
    if(outputNumpy == Py_True) {
        npy_intp len = (int) o->l; //end - start;
        ret = PyArray_SimpleNewFromData(1, &len, NPY_FLOAT, (void *) o->value);
        //This will break if numpy ever stops using malloc!
        PyArray_ENABLEFLAGS((PyArrayObject*) ret, NPY_ARRAY_OWNDATA);
        free(o->start);
        free(o->end);
        free(o);
    } else {
#endif
        //ret = PyList_New(end-start);
        ret = PyList_New((int) o->l);
        for(i=0; i<(int) o->l; i++) {
            //unsigned int ccxx = (unsigned int) o->coverage[0];
            //fprintf(stderr, "ww %f %u\n", o->value[i], o->start[i]);
            PyList_SetItem(ret, i, PyFloat_FromDouble(o->value[i]));
        }
        bmDestroyOverlappingIntervals(o);
#ifdef WITHNUMPY
    }
#endif

    return ret;
}

static PyObject *pyBmGetIntervals(pybinaMethFile_t *self, PyObject *args, PyObject *kwds) {
    binaMethFile_t *bm = self->bm;
    uint32_t start, end = -1, tid, i;
    unsigned long startl = 0, endl = -1;
    static char *kwd_list[] = {"chrom", "start", "end", NULL};
    bmOverlappingIntervals_t *intervals = NULL;
    char *chrom;
    PyObject *ret, *starto = NULL, *endo = NULL;

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not opened!");
        return NULL;
    }

    if(bm->isWrite == 1) {
        PyErr_SetString(PyExc_RuntimeError, "Intervals cannot be accessed in files opened for writing!");
        return NULL;
    }

    if(bm->type == 1) {
        PyErr_SetString(PyExc_RuntimeError, "bigBed files have no intervals! Use 'entries()' instead.");
        return NULL;
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|OO", kwd_list, &chrom, &starto, &endo)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply at least a chromosome.\n");
        return NULL;
    }

    //Sanity check
    tid = bmGetTid(bm, chrom);
    if(endl == (unsigned long) -1 && tid != (uint32_t) -1) endl = bm->cl->len[tid];
    if(tid == (uint32_t) -1 || startl > end || endl > end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    if(starto) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(starto, Integer)) {
            startl = (long) getNumpyL(starto);
        } else
#endif
        if(PyLong_Check(starto)) {
            startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(starto)) {
            startl = PyInt_AsLong(starto);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
            return NULL;
        }
    }

    if(endo) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(endo, Integer)) {
            endl = (long) getNumpyL(endo);
        } else
#endif
        if(PyLong_Check(endo)) {
            endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(endo)) {
            endl = PyInt_AsLong(endo);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
            return NULL;
        }
    }

    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bm->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    //Check for empty files
    if(!hasEntries(bm)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    //Get the intervals
    intervals = bmGetOverlappingIntervals(bm, chrom, start, end);
    if(!intervals) {
        PyErr_SetString(PyExc_RuntimeError, "An error occurred while fetching the overlapping intervals!");
        return NULL;
    }
    if(!intervals->l) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    ret = PyTuple_New(intervals->l);
    for(i=0; i<intervals->l; i++) {
        if(PyTuple_SetItem(ret, i, Py_BuildValue("(iif)", intervals->start[i], intervals->end[i], intervals->value[i]))) {
            Py_DECREF(ret);
            bmDestroyOverlappingIntervals(intervals);
            PyErr_SetString(PyExc_RuntimeError, "An error occurred while constructing the output tuple!");
            return NULL;
        }
    }

    bmDestroyOverlappingIntervals(intervals);
    return ret;
}

#if PY_MAJOR_VERSION >= 3
//Return 1 iff obj is a ready unicode type
int PyString_Check(PyObject *obj) {
    if(PyUnicode_Check(obj)) {
        return PyUnicode_READY(obj)+1;
    }
    return 0;
}

//I don't know what happens if PyBytes_AsString(NULL) is used...
char *PyString_AsString(PyObject *obj) {
    return PyUnicode_AsUTF8(obj);
}
#endif

//Will return 1 for long or int types currently
int isNumeric(PyObject *obj) {
#ifdef WITHNUMPY
    if(PyArray_IsScalar(obj, Integer)) return 1;
#endif
#if PY_MAJOR_VERSION < 3
    if(PyInt_Check(obj)) return 1;
#endif
    return PyLong_Check(obj);
}

//On error, throws a runtime error, so use PyErr_Occurred() after this
uint32_t Numeric2Uint(PyObject *obj) {
    long l;
#if PY_MAJOR_VERSION < 3
    if(PyInt_Check(obj)) {
        return (uint32_t) PyInt_AsLong(obj);
    }
#endif
    l = PyLong_AsLong(obj);
    //Check bounds
    if(l > 0xFFFFFFFF) {
        PyErr_SetString(PyExc_RuntimeError, "Length out of bounds for a binaMeth file!");
        return (uint32_t) -1;
    }
    return (uint32_t) l;
}

//This runs bmCreateHdr, bmCreateChromList, and bmWriteHdr
PyObject *pyBmAddHeader(pybinaMethFile_t *self, PyObject *args, PyObject *kwds) {
    binaMethFile_t *bm = self->bm;
    char **chroms = NULL;
    int64_t n;
    uint32_t *lengths = NULL, len;
    int32_t maxZooms = 2; //10
    long zoomTmp = 2; //10
    static char *kwd_list[] = {"cl", "maxZooms", NULL};
    PyObject *InputTuple = NULL, *tmpObject, *tmpObject2;
    Py_ssize_t i, pyLen;

    if(!bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not open!");
        return NULL;
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O|k", kwd_list, &InputTuple, &zoomTmp)) {
        PyErr_SetString(PyExc_RuntimeError, "Illegal arguments");
        return NULL;
    }
    maxZooms = zoomTmp;

    //Ensure that we received a list
    if(!PyList_Check(InputTuple)) {
        PyErr_SetString(PyExc_RuntimeError, "You MUST input a list of tuples (e.g., [('chr1', 1000), ('chr2', 2000)]!");
        goto error;
    }
    pyLen = PyList_Size(InputTuple);
    if(pyLen < 1) {
        PyErr_SetString(PyExc_RuntimeError, "You input an empty list!");
        goto error;
    }
    n = pyLen;

    lengths = calloc(n, sizeof(uint32_t));
    chroms = calloc(n, sizeof(char*));
    if(!lengths || !chroms) {
        PyErr_SetString(PyExc_RuntimeError, "Couldn't allocate lengths or chroms!");
        goto error;
    }

    //Convert the tuple into something more useful in C
    for(i=0; i<pyLen; i++) {
        tmpObject = PyList_GetItem(InputTuple, i);
        if(!tmpObject) {
            PyErr_SetString(PyExc_RuntimeError, "Couldn't get a tuple!");
            goto error;
        }
        if(!PyTuple_Check(tmpObject)) {
            PyErr_SetString(PyExc_RuntimeError, "The input list is not made up of tuples!");
            goto error;
        }
        if(PyTuple_Size(tmpObject) != 2) {
            PyErr_SetString(PyExc_RuntimeError, "One tuple does not contain exactly 2 members!");
            goto error;
        }

        //Chromosome
        tmpObject2 = PyTuple_GetItem(tmpObject, 0); //This returns NULL in python3?!?
        if(!PyString_Check(tmpObject2)) {
            PyErr_SetString(PyExc_RuntimeError, "The first element of each tuple MUST be a string!");
            goto error;
        }
        chroms[i] = PyString_AsString(tmpObject2);
        if(!chroms[i]) {
            PyErr_SetString(PyExc_RuntimeError, "Received something other than a string for a chromosome name!");
            goto error;
        }

        //Length
        tmpObject2 = PyTuple_GetItem(tmpObject, 1);
        if(!isNumeric(tmpObject2)) {
            PyErr_SetString(PyExc_RuntimeError, "The second element of each tuple MUST be an integer!");
            goto error;
        }
        len = Numeric2Uint(tmpObject2);
        if(PyErr_Occurred()) goto error;
        if(zoomTmp > 0xFFFFFFFF) {
            PyErr_SetString(PyExc_RuntimeError, "A requested length is longer than what can be stored in a binaMeth file!");
            goto error;
        }
        lengths[i] = len;
    }

    //Create the header
    if(bmCreateHdr(bm, maxZooms)) {
        PyErr_SetString(PyExc_RuntimeError, "Received an error in bmCreateHdr");
        goto error;
    }

    //Create the chromosome list
    bm->cl = bmCreateChromList(chroms, lengths, n);
    if(!bm->cl) {
        PyErr_SetString(PyExc_RuntimeError, "Received an error in bmCreateChromList");
        goto error;
    }

    //Write the header
    if(bmWriteHdr(bm)) {
        PyErr_SetString(PyExc_RuntimeError, "Received an error while writing the binaMeth header");
        goto error;
    }

    if(lengths) free(lengths);
    if(chroms) free(chroms);

    Py_INCREF(Py_None);
    return Py_None;

error:
    if(lengths) free(lengths);
    if(chroms) free(chroms);
    return NULL;
}

//1 on true, 0 on false
int isType0(PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *values) {
    int rv = 0;
    Py_ssize_t i, sz = 0;
    PyObject *tmp;

    if(!PyList_Check(chroms)
#ifdef WITHNUMPY
        && !PyArray_Check(chroms)
#endif
        ) return rv;
    if(!PyList_Check(starts)
#ifdef WITHNUMPY
        && !PyArray_Check(starts)
#endif
        ) return rv;
    if(!PyList_Check(ends)
#ifdef WITHNUMPY
        && !PyArray_Check(ends)
#endif
        ) return rv;
    if(!PyList_Check(values)
#ifdef WITHNUMPY
        && !PyArray_Check(values)
#endif
        ) return rv;
    if(PyList_Check(chroms)) sz = PyList_Size(chroms);
#ifdef WITHNUMPY
    if(PyArray_Check(chroms)) sz += PyArray_Size(chroms);
#endif

    if(PyList_Check(starts)) {
        if(sz != PyList_Size(starts)) return rv;
#ifdef WITHNUMPY
    } else {
        if(sz != PyArray_Size(starts)) return rv;
#endif
    }
    if(PyList_Check(ends)) {
        if(sz != PyList_Size(ends)) return rv;
#ifdef WITHNUMPY
    } else {
        if(sz != PyArray_Size(ends)) return rv;
#endif
    }
    if(PyList_Check(values)) {
        if(sz != PyList_Size(values)) return rv;
#ifdef WITHNUMPY
    } else {
        if(sz != PyArray_Size(values)) return rv;
#endif
    }

    //Ensure chroms contains strings, etc.
    if(PyList_Check(chroms)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(chroms, i);
            if(!PyString_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISSTRING( (PyArrayObject*) chroms)) return rv;
#endif
    }
    if(PyList_Check(starts)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(starts, i);
            if(!isNumeric(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISINTEGER( (PyArrayObject*) starts)) return rv;
#endif
    }
    if(PyList_Check(ends)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(ends, i);
            if(!isNumeric(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISINTEGER( (PyArrayObject*) ends)) return rv;
#endif
    }
    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISFLOAT((PyArrayObject*) values)) return rv;
#endif
    }
    return 1;
}

//single chrom, multiple starts, single span
int isType1(PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span) {
    int rv = 0;
    Py_ssize_t i, sz = 0;
    PyObject *tmp;

    if(!PyString_Check(chroms)) return rv;
    if(!PyList_Check(starts)
#ifdef WITHNUMPY
        && !PyArray_Check(starts)
#endif
        ) return rv;
    if(!PyList_Check(values)
#ifdef WITHNUMPY
        && !PyArray_Check(values)
#endif
        ) return rv;
    if(!isNumeric(span)) return rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif

    if(PyList_Check(values)) if(sz != PyList_Size(values)) return rv;
#ifdef WITHNUMPY
    if(PyArray_Check(values)) if(sz != PyArray_Size(values)) return rv;
#endif

    if(PyList_Check(starts)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(starts, i);
            if(!isNumeric(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISINTEGER( (PyArrayObject*) starts)) return rv;
#endif
    }
    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISFLOAT( (PyArrayObject*) values)) return rv;
#endif
    }
    return 1;
}

//Single chrom, single start, single span, single step, multiple values
int isType2(PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span, PyObject *step) {
    int rv = 0;
    Py_ssize_t i, sz;
    PyObject *tmp;

    if(!isNumeric(span)) return rv;
    if(!isNumeric(step)) return rv;
    if(!PyString_Check(chroms)) return rv;
    if(!isNumeric(starts)) return rv;

    if(PyList_Check(values)) {
        sz = PyList_Size(values);
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISFLOAT( (PyArrayObject*) values)) return rv;
#endif
    }
    rv = 1;
    return rv;
}

int getType(PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *values, PyObject *span, PyObject *step) {
    if(!chroms) return -1;
    if(!starts) return -1;
    if(!values) return -1;
    if(chroms && starts && ends && values && isType0(chroms, starts, ends, values)) return 0;
    if(chroms && starts && span && values && isType1(chroms, starts, values, span)) return 1;
    if(chroms && starts && values && isType0(chroms, starts, starts, values)) return 0;//new
    if(chroms && starts && values && span && step && isType2(chroms, starts, values, span, step)) return 2;
    return -1;
}

//1: Can use a bmAppend* function. 0: must use a bmAdd* function
int canAppend(pybinaMethFile_t *self, int desiredType, PyObject *chroms, PyObject *starts, PyObject *span, PyObject *step) {
    binaMethFile_t *bm = self->bm;
    Py_ssize_t i, sz = 0;
    uint32_t tid, uspan, ustep, ustart;
    PyObject *tmp;
#ifdef WITHNUMPY
    char *chrom;
#endif

    if(self->lastType == -1) return 0;
    if(self->lastTid == -1) return 0;
    if(self->lastType != desiredType) return 0;

    //We can only append if (A) we have the same type or (B) the same chromosome (and compatible span/step/starts
    if(desiredType == 0) {
        //We need (A) chrom == lastTid and (B) all chroms to be the same
        if(PyList_Check(chroms)) sz = PyList_Size(chroms);
#ifdef WITHNUMPY
        if(PyArray_Check(chroms)) sz = PyArray_Size(chroms);
#endif

        for(i=0; i<sz; i++) {
#ifdef WITHNUMPY
            if(PyArray_Check(chroms)) {
                chrom = getNumpyStr((PyArrayObject*)chroms, i);
                tid = bmGetTid(bm, chrom);
                free(chrom);
            } else {
#endif
                tmp = PyList_GetItem(chroms, i);
                tid = bmGetTid(bm, PyString_AsString(tmp));
#ifdef WITHNUMPY
            }
#endif
            if(tid != (uint32_t) self->lastTid) return 0;
        }

#ifdef WITHNUMPY
        if(PyArray_Check(starts)) {
            ustart = getNumpyU32((PyArrayObject*)starts, 0);
        } else {
#endif
            ustart = Numeric2Uint(PyList_GetItem(starts, 0));
#ifdef WITHNUMPY
        }
#endif
        if(PyErr_Occurred()) return 0;
        if(ustart < self->lastStart) return 0;
        return 1;
    } else if(desiredType == 1) {
        //We need (A) chrom == lastTid, (B) all chroms to be the same, and (C) equal spans
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan != self->lastSpan) return 0;
        if(!PyString_Check(chroms)) return 0;
        tid = bmGetTid(bm, PyString_AsString(chroms));
        if(tid != (uint32_t) self->lastTid) return 0;

#ifdef WITHNUMPY
        if(PyList_Check(starts)) ustart = Numeric2Uint(PyList_GetItem(starts, 0));
        else ustart = getNumpyU32((PyArrayObject*) starts, 0);
#else
        ustart = Numeric2Uint(PyList_GetItem(starts, 0));
#endif
        if(PyErr_Occurred()) return 0;
        if(ustart < self->lastStart) return 0;
        return 1;
    } else if(desiredType == 2) {
        //We need (A) chrom == lastTid, (B) span/step to be equal and (C) compatible starts
        tid = bmGetTid(bm, PyString_AsString(chroms));
        if(tid != (uint32_t) self->lastTid) return 0;
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan != self->lastSpan) return 0;
        ustep = Numeric2Uint(step);
        if(PyErr_Occurred()) return 0;
        if(ustep != self->lastStep) return 0;

        //But is the start position compatible?
        ustart = Numeric2Uint(starts);
        if(PyErr_Occurred()) return 0;
        if(ustart != self->lastStart) return 0;
        return 1;
    }

    return 0;
}

//Returns 0 on success, 1 on error. Sets self->lastTid && self->lastStart (unless there was an error)
int PyAddIntervals(pybinaMethFile_t *self, PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *values, PyObject *coverages, PyObject *strands, PyObject *contexts, PyObject *entryid) {
    binaMethFile_t *bm = self->bm;
    Py_ssize_t i, sz = 0;
    char **cchroms = NULL;
    uint32_t n, *ustarts = NULL, *uends = NULL;
    float *fvalues = NULL;
    int rv;
    uint16_t *fcoverages = NULL;
    uint8_t *fstrands = NULL, *fcontexts = NULL;
    char **fentryid = NULL;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
    n = (uint32_t) sz;

    //Allocate space
    cchroms = calloc(n, sizeof(char*));
    ustarts = calloc(n, sizeof(uint32_t));
    uends = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    fcoverages = calloc(n, sizeof(uint16_t));
    fstrands = calloc(n, sizeof(uint8_t));
    fcontexts = calloc(n, sizeof(uint8_t));
    fentryid = calloc(n, sizeof(char*));

    if(!cchroms || !ustarts || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        if(PyList_Check(chroms)) {
            cchroms[i] = PyString_AsString(PyList_GetItem(chroms, i));
#ifdef WITHNUMPY
        } else {
            cchroms[i] = getNumpyStr((PyArrayObject*)chroms, i);
#endif
        }
        if(PyList_Check(starts)) {
            ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
        } else {
            ustarts[i] = getNumpyU32((PyArrayObject*)starts, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        if(bm->type & BM_END){
            if(PyList_Check(ends)) {
                uends[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(ends, i));
    #ifdef WITHNUMPY
            } else {
                uends[i] = getNumpyU32((PyArrayObject*)ends, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }
        if(PyList_Check(values)) {
            fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
#ifdef WITHNUMPY
        } else {
            fvalues[i] = getNumpyF((PyArrayObject*)values, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        
        if(bm->type & BM_COVER){
            if(PyList_Check(coverages)) {
                fcoverages[i] = (uint16_t) PyLong_AsLong(PyList_GetItem(coverages, i));
    #ifdef WITHNUMPY
            } else {
                fcoverages[i] = getNumpyF((PyArrayObject*)coverages, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }

        if(bm->type & BM_STRAND){
            if(PyList_Check(strands)) {
                fstrands[i] = (uint8_t) PyLong_AsLong(PyList_GetItem(strands, i));
    #ifdef WITHNUMPY
            } else {
                fstrands[i] = getNumpyF((PyArrayObject*)strands, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }

        if(bm->type & BM_CONTEXT){
            if(PyList_Check(contexts)) {
                fcontexts[i] = (uint8_t) PyLong_AsLong(PyList_GetItem(contexts, i));
    #ifdef WITHNUMPY
            } else {
                fcontexts[i] = getNumpyF((PyArrayObject*)contexts, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }

        if(bm->type & BM_ID){
            if(PyList_Check(entryid)) {
                fentryid[i] = PyString_AsString(PyList_GetItem(entryid, i));
    #ifdef WITHNUMPY
            } else {
                fentryid[i] = getNumpyStr((PyArrayObject*)entryid, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }
    }

    rv = bmAddIntervals(bm, cchroms, ustarts, uends, fvalues, fcoverages, fstrands, fcontexts,
                fentryid, n);
    if(!rv) {
        self->lastTid = bmGetTid(bm, cchroms[n-1]);
        self->lastStart = uends[n-1];
    }
    if(!PyList_Check(chroms)) {
        for(i=0; i<n; i++) free(cchroms[i]);
    }
    free(cchroms);
    free(ustarts);
    free(uends);
    free(fvalues);
    free(fcoverages);
    free(fstrands);
    free(fcontexts);
    free(fentryid);
    return rv;

error:
    if(cchroms) free(cchroms);
    if(ustarts) free(ustarts);
    if(uends) free(uends);
    if(fvalues) free(fvalues);
    if(fcoverages) free(fcoverages);
    if(fstrands) free(fstrands);
    if(fcontexts) free(fcontexts);
    if(fentryid) free(fentryid);
    return 1;
}

//Returns 0 on success, 1 on error. Update self->lastStart
int PyAppendIntervals(pybinaMethFile_t *self, PyObject *starts, PyObject *ends, PyObject *values, PyObject *coverages, PyObject *strands, PyObject *contexts, PyObject *entryid) {
    binaMethFile_t *bm = self->bm;
    Py_ssize_t i, sz = 0;
    uint32_t n, *ustarts = NULL, *uends = NULL;
    float *fvalues = NULL;
    uint16_t *fcoverages = NULL;
    uint8_t *fstrands = NULL, *fcontexts = NULL;
    char **fentryid = NULL;

    int rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    uends = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    fcoverages = calloc(n, sizeof(uint16_t));
    fstrands = calloc(n, sizeof(uint8_t));
    fcontexts = calloc(n, sizeof(uint8_t));
    fentryid = calloc(n, sizeof(char*));
    if(!ustarts || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        if(PyList_Check(starts)) {
            ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
        } else {
            ustarts[i] = getNumpyU32((PyArrayObject*) starts, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        if(bm->type & BM_END){
            if(PyList_Check(ends)) {
                uends[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(ends, i));
    #ifdef WITHNUMPY
            } else {
                uends[i] = getNumpyU32((PyArrayObject*) ends, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }
        if(PyList_Check(values)) {
            fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
#ifdef WITHNUMPY
        } else {
            fvalues[i] = getNumpyF((PyArrayObject*) values, i);
#endif
        }
        if(PyErr_Occurred()) goto error;

        if(bm->type & BM_COVER){
            if(PyList_Check(coverages)) {
                fcoverages[i] = (uint16_t) PyLong_AsLong(PyList_GetItem(coverages, i));
    #ifdef WITHNUMPY
            } else {
                fcoverages[i] = getNumpyF((PyArrayObject*)coverages, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }

        if(bm->type & BM_STRAND){
            if(PyList_Check(strands)) {
                fstrands[i] = (uint8_t) PyLong_AsLong(PyList_GetItem(strands, i));
    #ifdef WITHNUMPY
            } else {
                fstrands[i] = getNumpyF((PyArrayObject*)strands, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }

        if(bm->type & BM_CONTEXT){
            if(PyList_Check(contexts)) {
                fcontexts[i] = (uint8_t) PyLong_AsLong(PyList_GetItem(contexts, i));
    #ifdef WITHNUMPY
            } else {
                fcontexts[i] = getNumpyF((PyArrayObject*)contexts, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }
        if(bm->type & BM_ID){
            if(PyList_Check(entryid)) {
                fentryid[i] = PyString_AsString(PyList_GetItem(entryid, i));
    #ifdef WITHNUMPY
            } else {
                fentryid[i] = getNumpyStr((PyArrayObject*)entryid, i);
    #endif
            }
            if(PyErr_Occurred()) goto error;
        }
    }
    rv = bmAppendIntervals(bm, ustarts, uends, fvalues, fcoverages, fstrands, fcontexts,
                fentryid, n);
    if(rv) self->lastStart = uends[n-1];
    free(ustarts);
    free(uends);
    free(fvalues);
    free(fcoverages);
    free(fstrands);
    free(fcontexts);
    free(fentryid);
    return rv;

error:
    if(ustarts) free(ustarts);
    if(uends) free(uends);
    if(fvalues) free(fvalues);
    if(fcoverages) free(fcoverages);
    if(fstrands) free(fstrands);
    if(fcontexts) free(fcontexts);
    if(fentryid) free(fentryid);
    return 1;
}

//Checks and ensures that (A) the entries are sorted correctly and don't overlap and (B) that the come after things that have already been added.
//Returns 1 on correct input, 0 on incorrect input
int addEntriesInputOK(pybinaMethFile_t *self, PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *span, PyObject *step, int type) {
    uint32_t lastTid = self->lastTid;
    uint32_t lastEnd = self->lastStart;
    uint32_t cTid, ustart, uend, uspan, ustep;
    Py_ssize_t i, sz = 0;
    PyObject *tmp;
#ifdef WITHNUMPY
    char *tmpStr;
#endif

    if(type == 0) {
        //Each chrom:start-end needs to be properly formed and come after prior entries
        if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
        if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
        if(sz == 0) return 0;
        for(i=0; i<sz; i++) {
#ifdef WITHNUMPY
            if(PyArray_Check(chroms)) {
                tmpStr = getNumpyStr((PyArrayObject*)chroms, i);
                cTid = bmGetTid(self->bm, tmpStr);
                free(tmpStr);
            } else {
#endif
                tmp = PyList_GetItem(chroms, i);
                cTid = bmGetTid(self->bm, PyString_AsString(tmp));
#ifdef WITHNUMPY
            }
#endif
            if(PyErr_Occurred()) {return 0;}
            if(cTid == (uint32_t) -1) {return 0;}

#ifdef WITHNUMPY
            if(PyArray_Check(starts)) {
                ustart = getNumpyU32((PyArrayObject*)starts, i);
            } else {
#endif
                ustart = Numeric2Uint(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
            }
#endif
            if(PyErr_Occurred()) {return 0;}
#ifdef WITHNUMPY
            if(PyArray_Check(ends)) {
                uend = getNumpyU32((PyArrayObject*) ends, i);
            } else {
#endif
                uend = Numeric2Uint(PyList_GetItem(ends, i));
#ifdef WITHNUMPY
            }
#endif
            if(PyErr_Occurred()) {return 0;}

            if(ustart >= uend) {return 0;}
            if(lastTid != (uint32_t) -1) {
                if(lastTid > cTid) {return 0;}
                if(lastTid == cTid) {
                    if(ustart < lastEnd) {return 0;}
                }
            }
            lastTid = cTid;
            lastEnd = uend;
        }
        return 1;
    } else if(type == 1) {
        //each chrom:start-(start+span) needs to be properly formed and come after prior entries
        if(!PyList_Check(starts)
#ifdef WITHNUMPY
            && !PyArray_Check(starts)
#endif
        ) return 0;
        if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
        else if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan < 1) return 0;
        if(sz == 0) return 0;
        cTid = bmGetTid(self->bm, PyString_AsString(chroms));
        if(cTid == (uint32_t) -1) return 0;
        if(lastTid != (uint32_t) -1) {
            if(lastTid > cTid) return 0;
        }
        for(i=0; i<sz; i++) {
#ifdef WITHNUMPY
            if(PyArray_Check(starts)) {
                ustart = getNumpyU32((PyArrayObject*)starts, i);
            } else {
#endif
                ustart = Numeric2Uint(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
            }
#endif
            if(PyErr_Occurred()) return 0;
            uend = ustart + uspan;

            if(lastTid == cTid) {
                if(ustart < lastEnd) return 0;
            }
            lastTid = cTid;
            lastEnd = uend;
        }
        return 1;
    } else if(type == 2) {
        //The chrom and start need to be appropriate
        cTid = bmGetTid(self->bm, PyString_AsString(chroms));
        if(cTid == (uint32_t) -1) return 0;
        ustart = Numeric2Uint(starts);
        if(PyErr_Occurred()) return 0;
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan < 1) return 0;
        ustep = Numeric2Uint(step);
        if(PyErr_Occurred()) return 0;
        if(ustep < 1) return 0;
        if(lastTid != (uint32_t) -1) {
            if(lastTid > cTid) return 0;
            if(lastTid == cTid) {
                if(ustart < lastEnd) return 0;
            }
        }
        return 1;
    }
    return 0;
}

PyObject *pyBmAddEntries(pybinaMethFile_t *self, PyObject *args, PyObject *kwds) {
    static char *kwd_list[] = {"chroms", "starts", "ends", "values", "span", "step", "coverages", "strands", "contexts", "entryid", "validate", NULL};
    PyObject *chroms = NULL, *starts = NULL, *ends = NULL, *values = NULL, *coverages=NULL, *strands=NULL, *contexts=NULL, *span = NULL, *step = NULL, *entryid=NULL;
    PyObject *validate = Py_True;
    int desiredType;

    if(!self->bm) {
        PyErr_SetString(PyExc_RuntimeError, "The binaMeth file handle is not open!");
        return NULL;
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOOOOOOOO", kwd_list, &chroms, &starts, &ends, &values, &span, &step, &coverages, &strands, &contexts, &entryid, &validate)) {
        PyErr_SetString(PyExc_RuntimeError, "Illegal arguments");
        return NULL;
    }

    desiredType = getType(chroms, starts, ends, values, span, step);
    if(desiredType == -1) {
        PyErr_SetString(PyExc_RuntimeError, "You must provide a valid set of entries. These can be comprised of any of the following: \n"
"1. A list of each of chromosomes, start positions, end positions and values.\n");
//"2. A list of each of start positions and values. Also, a chromosome and span must be specified.\n"
//"3. A list values, in which case a single chromosome, start position, span and step must be specified.\n");
        return NULL;
    }

    if(validate == Py_True  && !addEntriesInputOK(self, chroms, starts, ends, span, step, desiredType)) {
        PyErr_SetString(PyExc_RuntimeError, "The entries you tried to add are out of order, precede already added entries, or otherwise use illegal values.\n"
" Please correct this and try again.\n");
        return NULL;
    }

    if(canAppend(self, desiredType, chroms, starts, span, step)) {
        switch(desiredType) {
            case 0:
                if(PyAppendIntervals(self, starts, ends, values, coverages, strands, contexts, entryid)) goto error;
                break;
        }
    } else {
        switch(desiredType) {
            case 0:
                if(PyAddIntervals(self, chroms, starts, ends, values, coverages, strands, contexts, entryid)) goto error;
                break;
        }
    }
    self->lastType = desiredType;

    Py_INCREF(Py_None);
    return Py_None;

error:
    return NULL;
}

static PyObject *pyIsbinaMeth(pybinaMethFile_t *self, PyObject *args) {
    binaMethFile_t *bm = self->bm;
    if(bm->type == 0) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    Py_INCREF(Py_False);
    return Py_False;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pydmtools(void) {
#else
PyMODINIT_FUNC initpydmtools(void) {
#endif
    PyObject *res;
    errno = 0; //just in case

#if PY_MAJOR_VERSION >= 3
    if(Py_AtExit(bmCleanup)) return NULL;
    if(PyType_Ready(&binaMethFile) < 0) return NULL;
    if(bmInit(128000)) return NULL;
    res = PyModule_Create(&pybinaMethmodule);
    if(!res) return NULL;
#else
    if(Py_AtExit(bmCleanup)) return;
    if(PyType_Ready(&binaMethFile) < 0) return;
    if(bmInit(128000)) return;
    res = Py_InitModule3("pybinaMeth", bmMethods, "A module for handling binaMeth files");
#endif

    Py_INCREF(&binaMethFile);
    PyModule_AddObject(res, "pybinaMeth", (PyObject *) &binaMethFile);

#ifdef WITHNUMPY
    //Add the numpy constant
    import_array(); //Needed for numpy stuff to work
    PyModule_AddIntConstant(res, "numpy", 1);
#else
    PyModule_AddIntConstant(res, "numpy", 0);
#endif
#ifdef NOCURL
    PyModule_AddIntConstant(res, "remote", 0);
#else
    PyModule_AddIntConstant(res, "remote", 1);
#endif
    PyModule_AddStringConstant(res, "__version__", pybinaMethVersion);

#if PY_MAJOR_VERSION >= 3
    return res;
#endif
}
