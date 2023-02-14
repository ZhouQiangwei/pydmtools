#include <Python.h>
#include <structmember.h>
#include "binaMeth.h"

#define pybinaMethVersion "0.1.1"

typedef struct {
    PyObject_HEAD
    binaMethFile_t *bm;
    int32_t lastTid; //The TID of the last written entry (or -1)
    uint32_t lastSpan; //The span of the last written entry (if applicable)
    uint32_t lastStep; //The step of the last written entry (if applicable)
    uint32_t lastStart; //The next start position (if applicable)
    int lastType; //The type of the last written entry
} pybinaMethFile_t;

static PyObject *pyBmOpen(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject *pyBmEnter(pybinaMethFile_t *self, PyObject *args);
static PyObject *pyBmClose(pybinaMethFile_t *pybm, PyObject *args);
static PyObject *pyBmGetChroms(pybinaMethFile_t *pybm, PyObject *args);
static PyObject *pyIsbinaMeth(pybinaMethFile_t *pybm, PyObject *args);
static PyObject *pyBmGetStats(pybinaMethFile_t *pybm, PyObject *args, PyObject *kwds);
#ifdef WITHNUMPY
static PyObject *pyBmGetValues(pybinaMethFile_t *pybm, PyObject *args, PyObject *kwds);
#else
static PyObject *pyBmGetValues(pybinaMethFile_t *pybm, PyObject *args);
#endif
static PyObject *pyBmGetIntervals(pybinaMethFile_t *pybm, PyObject *args, PyObject *kwds);
static PyObject *pyBmGetHeader(pybinaMethFile_t *pybm, PyObject *args);
static PyObject *pyBmAddHeader(pybinaMethFile_t *pybm, PyObject *args, PyObject *kwds);
static PyObject *pyBmAddEntries(pybinaMethFile_t *pybm, PyObject *args, PyObject *kwds);
static void pyBmDealloc(pybinaMethFile_t *pybm);

//The function types aren't actually correct...
static PyMethodDef bmMethods[] = {
    {"openfile", (PyCFunction)pyBmOpen, METH_VARARGS|METH_KEYWORDS,
"Open a binaMeth or bigBed file. For remote files, give a URL starting with HTTP,\n\
FTP, or HTTPS.\n\
\n\
Optional arguments:\n\
    mode: An optional mode. The default is 'r', which opens a file for reading.\n\
          If you specify a mode containing 'w' then you'll instead open a file\n\
          for writing. Note that you then need to add an appropriate header\n\
          before use. For bigBed files, only reading is supported.\n\
\n\
Returns:\n\
   A binaMethFile object on success, otherwise None.\n\
\n\
Arguments:\n\
    file: The name of a binaMeth file.\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.openfile(\"some_file.bm\")\n"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef bmObjMethods[] = {
    {"header", (PyCFunction)pyBmGetHeader, METH_VARARGS,
"Returns the header of a binaMeth file. This contains information such as: \n\
  * The version number of the file ('version').\n\
  * The number of zoom levels ('nLevels').\n\
  * The number of bases covered ('nBasesCovered').\n\
  * The minimum value ('minVal').\n\
  * The maximum value ('maxVal').\n\
  * The sum of all values ('sumData').\n\
  * The sum of the square of all values ('sumSquared').\n\
These are returned as a dictionary.\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"some_file.bm\")\n\
>>> bm.header()\n\
{'maxVal': 2L, 'sumData': 272L, 'minVal': 0L, 'version': 4L,\n\
'sumSquared': 500L, 'nLevels': 1L, 'nBasesCovered': 154L}\n\
>>> bm.close()\n"},
    {"close", (PyCFunction)pyBmClose, METH_VARARGS,
"Close a binaMeth file.\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"some_file.bm\")\n\
>>> bm.close()\n"},
    {"isbinaMeth", (PyCFunction)pyIsbinaMeth, METH_VARARGS,
"Returns True if the object is a binaMeth file (otherwise False).\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"some_file.binaMeth\")\n\
>>> bm.isbinaMeth()\n\
True\n"},
    {"chroms", (PyCFunction)pyBmGetChroms, METH_VARARGS,
"Return a chromosome: length dictionary. The order is typically not\n\
alphabetical and the lengths are long (thus the 'L' suffix).\n\
\n\
Optional arguments:\n\
    chrom: An optional chromosome name\n\
\n\
Returns:\n\
    A list of chromosome lengths or a dictionary of them.\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"test/test.bm\")\n\
>>> bm.chroms()\n\
{'1': 195471971L, '10': 130694993L}\n\
\n\
Note that you may optionally supply a specific chromosome:\n\
\n\
>>> bm.chroms(\"chr1\")\n\
195471971L\n\
\n\
If you specify a non-existant chromosome then no output is produced:\n\
\n\
>>> bm.chroms(\"foo\")\n\
>>>\n"},
    {"stats", (PyCFunction)pyBmGetStats, METH_VARARGS|METH_KEYWORDS,
"Return summary statistics for a given range. On error, this function throws a\n\
runtime exception.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
\n\
Keyword arguments:\n\
    start: Starting position\n\
    end:   Ending position\n\
    type:  Summary type (mean, min, max, coverage, std, sum), default 'mean'.\n\
    nBins: Number of bins into which the range should be divided before\n\
           computing summary statistics. The default is 1.\n\
    exact: By default, pybinaMeth uses the same method as Kent's tools from UCSC\n\
           for computing statistics. This means that 'zoom levels' may be\n\
           used, rather than actual values (please see the pybinaMeth repository\n\
           on github for further information on this). To avoid this behaviour,\n\
           simply specify 'exact=True'. Note that values returned will then\n\
           differ from what UCSC, IGV, and similar other tools will report.\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"test/test.bm\")\n\
>>> bm.stats(\"1\", 0, 3)\n\
[0.2000000054637591]\n\
\n\
This is the mean value over the range 1:1-3 (in 1-based coordinates). If\n\
the start and end positions aren't given the entire chromosome is used.\n\
There are additional optional parameters 'type' and 'nBins'. 'type'\n\
specifies the type of summary information to calculate, which is 'mean'\n\
by default. Other possibilites for 'type' are: 'min' (minimum value),\n\
'max' (maximum value), 'coverage' (number of covered bases), and 'std'\n\
 (standard deviation). 'nBins' defines how many bins the region will be\n\
 divided into and defaults to 1.\n\
\n\
>>> bm.stats(\"1\", 0, 3, type=\"min\")\n\
[0.10000000149011612]\n\
>>> bm.stats(\"1\", 0, 3, type=\"max\")\n\
[0.30000001192092896]\n\
>>> bm.stats(\"1\", 0, 10, type=\"coverage\")\n\
[0.30000000000000004]\n\
>>> bm.stats(\"1\", 0, 3, type=\"std\")\n\
[0.10000000521540645]\n\
>>> bm.stats(\"1\",99,200, type=\"max\", nBins=2)\n\
[1.399999976158142, 1.5]\n"},
#ifdef WITHNUMPY
    {"getvalues", (PyCFunction)pyBmGetValues, METH_VARARGS|METH_KEYWORDS,
"Retrieve the value stored for each position (or None). On error, a runtime\n\
exception is thrown.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
    start: Starting position\n\
    end:   Ending position\n\
\n\
Optional arguments:\n\
    numpy: If True, return a numpy array rather than a list of values. This\n\
           is generally more memory efficient. Note that this option is only\n\
           available if pybinaMeth was installed with numpy support (check the\n\
           pybinaMeth.numpy() function).\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"test/test.bm\")\n\
>>> bm.getvalues(\"1\", 0, 3)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896]\n\
\n\
The length of the returned list will always match the length of the\n\
range. Any uncovered bases will have a value of None.\n\
\n\
>>> bm.getvalues(\"1\", 0, 4)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896, None]\n\
\n"},
#else
    {"getvalues", (PyCFunction)pyBmGetValues, METH_VARARGS,
"Retrieve the value stored for each position (or None). On error, a runtime\n\
exception is thrown.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
    start: Starting position\n\
    end:   Ending position\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"test/test.bm\")\n\
>>> bm.getvalues(\"1\", 0, 3)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896]\n\
\n\
The length of the returned list will always match the length of the\n\
range. Any uncovered bases will have a value of None.\n\
\n\
>>> bm.getvalues(\"1\", 0, 4)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896, None]\n\
\n"},
#endif
    {"intervals", (PyCFunction)pyBmGetIntervals, METH_VARARGS|METH_KEYWORDS,
"Retrieve each interval covering a part of a chromosome/region. On error, a\n\
runtime exception is thrown.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
\n\
Keyword arguments:\n\
    start: Starting position\n\
    end:   Ending position\n\
\n\
If start and end aren't specified, the entire chromosome is returned.\n\
The returned object is a tuple containing the starting position, end\n\
position, and value of each interval in the file. As with all binaMeth\n\
positions, those returned are 0-based half-open (e.g., a start of 0 and\n\
end of 10 specifies the first 10 positions).\n\
\n\
>>> import pybinaMeth\n\
>>> bm = pybinaMeth.open(\"test/test.bm\")\n\
>>> bm.intervals(\"1\", 0, 3)\n\
((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224),\n\
 (2, 3, 0.30000001192092896))\n\
>>> bm.close()"},
    {"addHeader", (PyCFunction)pyBmAddHeader, METH_VARARGS|METH_KEYWORDS,
"Adds a header to a file opened for writing. This MUST be called before adding\n\
any entries. On error, a runtime exception is thrown.\n\
\n\
Positional arguments:\n\
    cl:    A chromosome list, of the form (('chr1', 1000), ('chr2', 2000), ...).\n\
           In other words, each element of the list is a tuple containing a\n\
           chromosome name and its associated length.\n\
\n\
Keyword arguments:\n\
    maxZooms:  The maximum number of zoom levels. The value must be >=0. The\n\
               default is 10.\n\
\n\
>>> import pybinaMeth\n\
>>> import tempfile\n\
>>> import os\n\
>>> ofile = tempfile.NamedTemporaryFile(delete=False)\n\
>>> oname = ofile.name\n\
>>> ofile.close()\n\
>>> bm = pybinaMeth.open(oname, 'w')\n\
>>> bm.addHeader([(\"1\", 1000000), (\"2\", 1500000)], maxZooms=0)\n\
>>> bm.close()\n\
>>> os.remove(oname)"},
    {"addEntries", (PyCFunction)pyBmAddEntries, METH_VARARGS|METH_KEYWORDS,
"Adds one or more entries to a binaMeth file. This returns nothing, but throws a\n\
runtime exception on error.\n\
\n\
This function always accepts an optional 'validate' option. If set to 'True',\n\
which is the default, the input entries are checked to ensure that they come\n\
after previously entered entries. This comes with significant overhead, so if\n\
this is instead 'False' then this validation is not performed.\n\
\n\
There are three manners in which entries can be stored in binaMeth files.\n\
\n\
\n\
bedGraph-like entries (12 bytes each):\n\
\n\
Positional arguments:\n\
    chrom:  A list of chromosome. These MUST match those added with addHeader().\n\
    starts: A list of start positions. These are 0-based.\n\
\n\
Keyword arguments:\n\
    ends:   A list of end positions. These are 0-based half open, so a start of\n\
            0 and end of 10 specifies the first 10 bases.\n\
    values: A list of values.\n\
\n\
\n\
Variable-step entries (8 bytes each):\n\
\n\
Positional arguments:\n\
    chrom:  A chromosome name. This MUST match one added with addHeader().\n\
    starts: A list of start positions. These are 0-based.\n\
\n\
Keyword arguments:\n\
    values: A list of values.\n\
    span:   A span width. This is an integer value and specifies how many bases\n\
            each entry describes. An entry with a start position of 0 and a span\n\
            of 10 describes the first 10 bases.\n\
\n\
\n\
Fixed-step entries (4 bytes each):\n\
\n\
Positional arguments:\n\
    chrom:  A chromosome name. This MUST match one added with addHeader().\n\
    starts: A start position. These are 0-based. The start position of each\n\
            entry starts 'step' after the previous and describes 'span' bases.\n\
\n\
Keyword arguments:\n\
    values: A list of values.\n\
    span:   A span width. This is an integer value and specifies how many bases\n\
            each entry describes. An entry with a start position of 0 and a span\n\
            of 10 describes the first 10 bases.\n\
    step:   A step width. Each subsequent entry begins this number of bases\n\
            after the previous. So if the first entry has a start of 0 and step\n\
            or 30, the second entry will start at 30.\n\
\n\
>>> import pybinaMeth\n\
>>> import tempfile\n\
>>> import os\n\
>>> ofile = tempfile.NamedTemporaryFile(delete=False)\n\
>>> oname = ofile.name\n\
>>> ofile.close()\n\
>>> bm = pybinaMeth.open(oname, 'w')\n\
>>> bm.addHeader([(\"1\", 1000000), (\"2\", 1500000)])\n\
>>> #Add some bedGraph-like entries\n\
>>> bm.addEntries([\"1\", \"1\", \"1\"], [0, 100, 125], ends=[5, 120, 126], values=[0.0, 1.0, 200.0])\n\
>>> #Variable-step entries, the span 500-520, 600-620, and 635-655\n\
>>> bm.addEntries(\"1\", [500, 600, 635], values=[-2.0, 150.0, 25.0], span=20)\n\
>>> #Fixed-step entries, the bases described are 900-920, 930-950, and 960-980\n\
>>> bm.addEntries(\"1\", 900, values=[-5.0, -20.0, 25.0], span=20, step=30)\n\
>>> #This only works due to using validate=False. Obviously the file is then corrupt.\n\
>>> bm.addEntries([\"1\", \"1\", \"1\"], [0, 100, 125], ends=[5, 120, 126], values=[0.0, 1.0, 200.0], validate=False)\n\
>>> bm.close()\n\
>>> os.remove(oname)"},
    {"__enter__", (PyCFunction)pyBmEnter, METH_NOARGS, NULL},
    {"__exit__", (PyCFunction)pyBmClose, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
struct pybinaMethmodule_state {
    PyObject *error;
};

#define GETSTATE(m) ((struct pybinaMethmodule_state*)PyModule_GetState(m))

static PyModuleDef pybinaMethmodule = {
    PyModuleDef_HEAD_INIT,
    "pybinaMeth",
    "A python module for binaMeth file access",
    -1,
    bmMethods,
    NULL, NULL, NULL, NULL
};
#endif

//Should set tp_dealloc, tp_print, tp_repr, tp_str, tp_members
static PyTypeObject binaMethFile = {
#if PY_MAJOR_VERSION >= 3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,              /*ob_size*/
#endif
    "pybinaMeth.binaMethFile",     /*tp_name*/
    sizeof(pybinaMethFile_t),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)pyBmDealloc,     /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash*/
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro*/
    0,                         /*tp_as_buffer*/
#if PY_MAJOR_VERSION >= 3
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
#else
    Py_TPFLAGS_HAVE_CLASS,     /*tp_flags*/
#endif
    "binaMeth File",             /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    0,                         /*tp_richcompare*/
    0,                         /*tp_weaklistoffset*/
    0,                         /*tp_iter*/
    0,                         /*tp_iternext*/
    bmObjMethods,                 /*tp_methods*/
    0,                         /*tp_members*/
    0,                         /*tp_getset*/
    0,                         /*tp_base*/
    0,                         /*tp_dict*/
    0,                         /*tp_descr_get*/
    0,                         /*tp_descr_set*/
    0,                         /*tp_dictoffset*/
    0,                         /*tp_init*/
    0,                         /*tp_alloc*/
    0,                         /*tp_new*/
    0,0,0,0,0,0
};
