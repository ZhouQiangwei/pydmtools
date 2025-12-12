<p>
<a href="https://dmtools-docs.readthedocs.io/en/latest/function/pydmtools.html" target="_blank"><img src="https://img.shields.io/badge/docs-8A2BE2?link=https%3A%2F%2Fdmtools-docs.rtfd.io"></a>
<a href="http://bioconda.github.io"><img src="https://img.shields.io/badge/install%20with-bioconda-brightgreen.svg"></a>
<a href="https://pypistats.org/packages/pydmtools"><img alt="PyPI Downloads" src="https://img.shields.io/pypi/dm/pydmtools?label=PyPI%20download"></a>
<a href="https://anaconda.org/bioconda/pydmtools"><img alt="License" src="https://anaconda.org/bioconda/pydmtools/badges/license.svg"></a>
<a href="https://anaconda.org/bioconda/pydmtools"><img alt="Conda Downloads" src="https://img.shields.io/conda/d/bioconda/pydmtools"></a>
</p>

# pyDMtools
A python extension, written in C, for quick access to DM files and access to and creation of DM files. This extension uses [dmtools](https://github.com/ZhouQiangwei/dmtools) for local and remote file access.

Table of Contents
=================

  * [Installation](#installation)
    * [Requirements](#requirements)
  * [Usage](#usage)
    * [Load the extension](#load-the-extension)
    * [Open a DM file](#open-a-dm-file)
    * [Access the list of chromosomes and their lengths](#access-the-list-of-chromosomes-and-their-lengths)
    * [Inspect the header](#inspect-the-header)
* [Compute summary information on a range](#compute-summary-information-on-a-range)
* [Retrieve values for individual bases in a range](#retrieve-values-for-individual-bases-in-a-range)
* [Retrieve all intervals in a range](#retrieve-all-intervals-in-a-range)
* [Retrieve rich entry records (coverage/strand/context/id)](#retrieve-rich-entry-records-coveragestrandcontextid)
* [Preparing a DM file for writing](#preparing-a-dm-file-for-writing)
* [Adding entries (values, coverage, strand, context, id)](#adding-entries-values-coverage-strand-context-id)
* [Close a DM file](#close-a-dm-file)
  * [A note on coordinates](#a-note-on-coordinates)

# Installation
You can install this extension directly from github with:

    pip install pydmtools
    
    OR
    
    pip install pydmtools==0.1.5

or with conda

    conda install pydmtools -c bioconda

## Requirements

The follow non-python requirements must be installed:

 - libcurl (and the `curl-config` config)
 - zlib

The headers and libraries for these are required.

# Usage
pydmtools mirrors the dmtools command-line functionality for reading and writing the DM (DNA methylation) binary format. The snippets below highlight the most common tasks, how the Python API maps to dmtools concepts, and what each argument means.

## Load the extension

```python
>>> import pydmtools as pydm
```

## Open a DM file

```python
>>> dm = pydm.openfile("tests/test.dm")
```

`openfile` raises a Python exception when the file cannot be opened. Files are opened in read mode by default. Pass a mode containing "w" to create or replace a DM file for writing:

```python
>>> dm = pydm.openfile("test/output.dm", "w")
```

`binaMethFile` implements the context manager protocol so files are closed even if an exception is raised:

```python
>>> with pydm.openfile("tests/test.dm") as dm:
...     print(dm.chroms())
```

DM and bigBed share the same container. Use `dm.isbinaMeth()` to check whether the opened file is a DM file before calling DM-only helpers such as `values` or `stats`:

```python
>>> dm.isbinaMeth()
True
```

Files opened for writing cannot be queried for intervals or statistics until they are closed and reopened for reading.

## Access the list of chromosomes and their lengths

`dm.chroms()` returns a chromosome-length mapping derived from the on-disk header:

```python
>>> dm.chroms()
{'chr1': 195471971, 'chr10': 130694993}
>>> dm.chroms("chr1")
195471971
```

Unknown chromosome names return `None`.

## Inspect the header

`dm.header()` returns the full header dictionary including zoom-level metadata and summary statistics maintained by dmtools:

```python
>>> dm.header()
{'version': 61951, 'nLevels': 1, 'nBasesCovered': 2669, 'minVal': 0, 'maxVal': 1, 'sumData': 128.4087, 'sumSquared': 97.2676}
```

The header also reports a `type` bitmask derived directly from the DM header
`version`. Each bit mirrors the layout flags used by the dmtools C library so
you can tell whether the file actually stores coverage, strand, context, and
other metadata before trying to read them. pydmtools also derives a `fields`
list from this bitmask so you can see exactly what the DM file stores (for
example `['end', 'coverage', 'strand', 'context', 'id']`). The raw mask uses the
following module-level constants:

* `BM_COVER` (coverage values)
* `BM_STRAND` (strand flags)
* `BM_CONTEXT` (methylation contexts)
* `BM_ID` (string identifiers)
* `BM_END` (records contain explicit end coordinates)
* `BM_MAGIC` (marks the file as DM rather than bigBed)

These constants match the current `dmtools` implementation on the
`codex/investigate-memory-management-issues-in-dmtools` branch; `pydmtools`
masks the header version with the DM layout bits so the `type`/`fields` view
reflects exactly which optional columns are present.

When writing, pydmtools encodes your requested layout into the header version so readers can inspect `header()["type"]` (or
`header()["fields"]`) before choosing which columns to request:

```python
>>> dm.header()["type"] & pydm.BM_CONTEXT
0x1c0
>>> dm.header()["fields"]
['end', 'coverage', 'strand', 'context', 'id']
```

## Compute summary information on a range

`dm.stats()` mirrors `dmtools stats` and supports all core statistics plus methylation-aware filters:

```python
>>> dm.stats("chr1", 0, 10000)            # mean is the default
[0.2000000054637591]
>>> dm.stats("chr1", 0, 1000, type="max")
[0.30000001192092896]
>>> dm.stats("chr1", 0, 10, type="coverage")  # covered base fraction
[0.30000000000000004]
>>> dm.stats("chr1", 0, 3, type="std")         # standard deviation
[0.10000000521540645]
```

Additional options:

* `type`: one of `mean`, `min`, `max`, `coverage` (or `cov`), `sum`, `std`, `dev`, or `weight` (coverage-weighted methylation mean).
* `nBins`: split the queried region into this many bins before computing stats (defaults to 1).
* `step`: override the bin step size when using `nBins`.
* `strand`: filter intervals by strand (`"."`, `"+"`, or `"-"`).
* `context`: filter methylation context (`"C"/"ALL"`, `"CG"`, `"CHG"`, `"CHH"`).
* `exact`: when `True`, bypasses zoom-level aggregation and forces full-resolution computation.
* `numpy`: return a NumPy array when NumPy support is available (see [Numpy](#numpy)).

If `start`/`end` are omitted, the entire chromosome is used. When a region contains no data, the returned list (or array) contains `None`/`nan` placeholders.

## Retrieve values for individual bases in a range

`dm.getvalues()` returns the base-resolution values from the file:

```python
>>> dm.getvalues("chr1", 0, 4)
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896, nan]
```

The length of the result matches the queried range; uncovered bases are reported as `nan` (or `None` when NumPy is not requested). Add `numpy=True` to receive a NumPy array if available.

## Retrieve all intervals in a range

Use `dm.intervals()` to pull the stored intervals (start, end, value) that overlap a region:

```python
>>> dm.intervals("chr1", 0, 3)
((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896))
```

Omitting `start`/`end` returns all intervals on the chromosome. For bigBed inputs that lack numeric values, use the dmtools `entries` interface instead of `intervals`/`values`.

## Retrieve rich entry records (coverage/strand/context/id)

When the DM header indicates additional fields (coverage, strand, methylation context, ID), `dm.entries()` returns per-record dictionaries with everything decoded for you:

```python
>>> recs = dm.entries("chr1", 0, 10)
>>> recs[0]
{'start': 0, 'end': 1, 'value': 0.1, 'coverage': 7, 'strand': '+', 'context': 'CG', 'id': 'read1'}
```

By default, `entries` only includes metadata that the file actually stores. If you explicitly request a column that the header
bitmask says is missing, pydmtools raises a clear error. You can always drop stored columns with keyword flags:

```python
# Drop coverage even if present
>>> dm.entries("chr1", 0, 10, with_coverage=False)[0]["coverage"]
None
```

## Preparing a DM file for writing

1. Open the output in write mode: `dm = pydm.openfile("output.dm", "w")`.
2. Add a header describing chromosomes and lengths **in order** using `dm.addHeader()`:

```python
>>> dm.addHeader([("chr1", 1_000_000), ("chr2", 1_500_000)], maxZooms=0)
```

`maxZooms` controls how many zoom levels dmtools will build (default 10). Many genome browsers expect at least one zoom level, so avoid setting `maxZooms=0` unless you know downstream tools can cope.

## Adding entries (values, coverage, strand, context, id)

`dm.addEntries()` matches the dmtools CLI and accepts the three canonical DM encodings. All coordinate inputs are 0-based half-open. Optional per-entry metadata fields allow you to store dmtools methylation attributes alongside values:

* `coverages`: list/array of uint16 coverage counts for each value.
* `strands`: list/array of strand codes (`0`/`+`, `1`/`-`, `2`/`.`).
* `contexts`: list/array of methylation context codes (`0`/`C`/`ALL`, `1`/`CG`, `2`/`CHG`, `3`/`CHH`).
* `entryid`: list/array of string identifiers per row.

pydmtools updates the DM header's `version` layout bits automatically when you
pass any of these optional columns, so subsequent readers can discover the
presence of coverage/strand/context/ID without guesswork.

Entries must be added in sorted order by chromosome and start; pass `validate=False` to skip ordering checks (useful for pre-sorted streams, but unsafe otherwise).

### bedGraph-like intervals

```python
dm.addEntries(
    ["chr1", "chr1", "chr1"],
    [0, 100, 125],
    ends=[5, 120, 126],
    values=[0.0, 1.0, 200.0],
    coverages=[10, 5, 2],
    strands=["+", "-", "."],
    contexts=["CG", "CHH", "C"],
    entryid=["read1", "read2", "read3"],
)
```

### Variable-step intervals

All spans share the same width and chromosome. Starts are provided individually.

```python
dm.addEntries("chr1", [500, 600, 635], values=[-2.0, 150.0, 25.0], span=20)
```

### Fixed-step intervals

The span and step are fixed; only the first start is supplied.

```python
dm.addEntries("chr1", 900, values=[-5.0, -20.0, 25.0], span=20, step=30)
```

### High-level conveniences

Lightweight helpers in :mod:`pydmtools.highlevel` can turn query results into
data frames, compute per-cell QC summaries, or aggregate a simple cell × region
matrix:

```
import pydmtools as pydm
from pydmtools.highlevel import entries_to_df, per_cell_qc, region_matrix

with pydm.openfile("singlecell.dm") as dm:
    df = entries_to_df(dm, "chr1", 0, 100000)
    qc = per_cell_qc(dm, context="CG", min_coverage=3)
    regions = [("chr1", 0, 10000), ("chr1", 10000, 20000)]
    X, cell_ids, regs = region_matrix(dm, regions, sparse=True, return_mapping=True)
```

## Close a DM file

Call `dm.close()` (or rely on the context manager) after writing. Closing flushes buffered entries, writes the index, and builds zoom levels, which may take some time on large files.
# Numpy

As of version 0.1.5, pydmtools supports input of coordinates using numpy integers and vectors in some functions **if numpy was installed prior to installing pydmtools**. To determine if pydmtools was installed with numpy support by checking the `numpy` accessor:

    >>> import pydmtools as pydm
    >>> pydm.numpy
    1

If `pydm.numpy` is `1`, then pydmtools was compiled with numpy support. This means that `addEntries()` can accept numpy coordinates:

    >>> import pydmtools as pydm
    >>> import numpy
    >>> dm = pydm.openfile("test/outnp.dm", "w")
    >>> dm.addHeader([("chr1", 1000)], maxZooms=0)
    >>> chroms = np.array(["chr1"] * 10)
    >>> starts = np.array([0, 10, 20, 30, 40, 50, 60, 70, 80, 90], dtype=np.int64)
    >>> ends = np.array([5, 15, 25, 35, 45, 55, 65, 75, 85, 95], dtype=np.int64)
    >>> values0 = np.array(np.random.random_sample(10), dtype=np.float64)
    >>> dm.addEntries(chroms, starts, ends=ends, values=values0)
    >>> dm.close()

Additionally, `getvalues()` can directly output a numpy vector:

    >>> dm = dm.openfile("test/outnp.dm")
    >>> dm.values('1', 0, 10, numpy=True)
    [ 0.74336642  0.74336642  0.74336642  0.74336642  0.74336642         nan
         nan         nan         nan         nan]
    >>> type(dm.values('1', 0, 10, numpy=True))
    <type 'numpy.ndarray'>


# A note on coordinates and library using
DM files use 1-based coordinates. And pydmtools and dmtools are based on [libbigwig](https://github.com/dpryan79/libBigWig) and [pydmtools](https://github.com/deeptools/pydmtools)

## libdm sync

To refresh the vendored `libdm` C sources to match the latest `dmtools` implementation, run `libdm/update_from_dmtools.sh` when network access to GitHub is available. The script copies the upstream files from the `codex/investigate-memory-management-issues-in-dmtools` branch.
