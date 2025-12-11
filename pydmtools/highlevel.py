"""High-level helpers built on the low-level :mod:`pydmtools` API.

The functions in this module provide lightweight conveniences for common
single-cell methylation tasks while delegating all file IO to the existing
`pydmtools` extension.
"""
from __future__ import annotations

from importlib import import_module
from typing import Dict, Iterable, Iterator, List, Optional, Sequence, Tuple, Union

# Lazy imports for optional dependencies


def _require_pandas():
    try:
        return import_module("pandas")
    except ImportError as exc:  # pragma: no cover - thin wrapper
        raise ImportError("entries_to_df requires pandas to be installed") from exc


def _require_scipy_sparse():
    try:
        return import_module("scipy.sparse")
    except ImportError as exc:  # pragma: no cover - thin wrapper
        raise ImportError("region_matrix with sparse=True requires scipy to be installed") from exc


# Public API -----------------------------------------------------------------


def entries_to_df(dm, chrom: str, start: Optional[int] = None, end: Optional[int] = None,
                  fields: Optional[Sequence[str]] = None, chunksize: Optional[int] = None):
    """
    Return entries from a dm file as a pandas DataFrame.

    Parameters
    ----------
    dm : dmtools file object (from ``pydmtools.openfile``)
    chrom : str
        Chromosome name.
    start : int or None
        0-based start coordinate. If ``None``, start from 0.
    end : int or None
        End coordinate (exclusive). If ``None``, use chromosome length.
    fields : list[str] or None
        Names of fields to include as columns. If ``None``, default to commonly
        used fields: ["chrom", "start", "end", "value", "coverage", "strand",
        "context", "id"]. Only include columns that actually exist in this dm
        file.
    chunksize : int or None
        If ``None``, return a single DataFrame with all entries. If an integer,
        return a generator that yields DataFrame chunks with at most
        ``chunksize`` rows.

    Returns
    -------
    pandas.DataFrame or generator of pandas.DataFrame
    """
    pd = _require_pandas()

    chroms = dm.chroms()
    start = 0 if start is None else start
    end = chroms[chrom] if end is None else end

    # Determine which fields are available
    header = dm.header()
    available_fields = set(header.get("fields", []))
    default_fields = ["chrom", "start", "end", "value", "coverage", "strand", "context", "id"]
    selected_fields = list(default_fields if fields is None else fields)
    selected_fields = [f for f in selected_fields if f == "chrom" or f in available_fields or f in {"start", "end", "value"}]

    def _build_frame(entries: List[Dict]) -> 'pd.DataFrame':
        rows = []
        for entry in entries:
            row = {key: entry.get(key) for key in selected_fields if key != "chrom"}
            row["chrom"] = chrom
            rows.append(row)
        return pd.DataFrame(rows, columns=selected_fields)

    entries = dm.entries(chrom, start, end)
    if chunksize is None:
        return _build_frame(entries)

    def _generator() -> Iterator['pd.DataFrame']:
        for i in range(0, len(entries), chunksize):
            yield _build_frame(entries[i:i + chunksize])

    return _generator()


def per_cell_qc(dm, context: Optional[str] = "CG", min_coverage: int = 1):
    """
    Compute per-cell QC metrics from a dm file that has an ID field (cell_id).

    Parameters
    ----------
    dm : dmtools file object
    context : str
        Context filter, e.g. ``"CG"`` or ``"C"``. If ``None``, do not filter
        by context.
    min_coverage : int
        Minimum coverage per site to be included in QC.

    Returns
    -------
    pandas.DataFrame
        One row per cell_id with columns: ``cell_id``, ``n_sites``,
        ``total_coverage``, ``mean_coverage``, ``mean_meth``.
    """
    pd = _require_pandas()

    header = dm.header()
    if "id" not in header.get("fields", []):
        raise ValueError("per_cell_qc requires a dm file with ID (cell_id) field")

    chrom_lengths = dm.chroms()
    stats: Dict[str, Dict[str, float]] = {}

    for chrom, length in chrom_lengths.items():
        for entry in dm.entries(chrom, 0, length):
            cov = entry.get("coverage", 0)
            if cov is None or cov < min_coverage:
                continue
            if context is not None and entry.get("context") != context:
                continue
            cell_id = entry.get("id")
            if cell_id is None:
                continue
            cell_stats = stats.setdefault(cell_id, {"n_sites": 0, "total_coverage": 0, "sum_meth_weighted": 0.0})
            cell_stats["n_sites"] += 1
            cell_stats["total_coverage"] += cov
            cell_stats["sum_meth_weighted"] += (entry.get("value", 0.0) or 0.0) * cov

    rows = []
    for cell_id, cell_stats in stats.items():
        n_sites = cell_stats["n_sites"]
        total_cov = cell_stats["total_coverage"]
        mean_cov = total_cov / n_sites if n_sites else 0
        mean_meth = cell_stats["sum_meth_weighted"] / total_cov if total_cov else 0
        rows.append({
            "cell_id": cell_id,
            "n_sites": n_sites,
            "total_coverage": total_cov,
            "mean_coverage": mean_cov,
            "mean_meth": mean_meth,
        })

    return pd.DataFrame(rows, columns=["cell_id", "n_sites", "total_coverage", "mean_coverage", "mean_meth"])


def region_matrix(dm, regions: Union[Sequence[Tuple[str, int, int]], 'object'],
                  sparse: bool = True, return_mapping: bool = False):
    """
    Build a cell × region matrix from a single dm file with ID (cell_id).

    Parameters
    ----------
    dm : dmtools file object
        A single dm file with ID field present.
    regions : list of (chrom, start, end) or pandas.DataFrame
        Region definitions. If DataFrame, must have ``'chrom'``, ``'start'`` and
        ``'end'`` columns.
    sparse : bool
        If ``True``, return a ``scipy.sparse`` CSR matrix. If ``False``, return a
        dense ``numpy.ndarray``.
    return_mapping : bool
        If ``True``, also return ``(cell_ids, regions_info)``.

    Returns
    -------
    matrix or (matrix, cell_ids, regions_info)
    """
    import numpy as np

    if sparse:
        sp = _require_scipy_sparse()
    else:
        sp = None

    header = dm.header()
    if "id" not in header.get("fields", []):
        raise ValueError("region_matrix requires a dm file with ID (cell_id) field")

    pd = None
    if not isinstance(regions, list):
        pd = _require_pandas()

    region_list: List[Tuple[str, int, int]]
    if pd is not None and hasattr(regions, "__iter__"):
        region_list = [(row["chrom"], int(row["start"]), int(row["end"])) for _, row in regions.iterrows()]
    else:
        region_list = list(regions)

    cell_to_idx: Dict[str, int] = {}
    accum: Dict[Tuple[int, int], Tuple[float, float]] = {}

    chrom_lengths = dm.chroms()
    for chrom, length in chrom_lengths.items():
        chrom_regions = [ (idx, reg) for idx, reg in enumerate(region_list) if reg[0] == chrom ]
        if not chrom_regions:
            continue
        for entry in dm.entries(chrom, 0, length):
            pos = entry.get("start")
            cell_id = entry.get("id")
            if pos is None or cell_id is None:
                continue
            for region_idx, (r_chrom, r_start, r_end) in chrom_regions:
                if r_start <= pos < r_end:
                    row_idx = cell_to_idx.setdefault(cell_id, len(cell_to_idx))
                    cov = entry.get("coverage", 0) or 0
                    val = entry.get("value", 0.0) or 0.0
                    key = (row_idx, region_idx)
                    sum_cov, sum_meth = accum.get(key, (0.0, 0.0))
                    sum_cov += cov
                    sum_meth += val * cov
                    accum[key] = (sum_cov, sum_meth)
                    break

    n_cells = len(cell_to_idx)
    n_regions = len(region_list)

    if sparse:
        data = []
        indices = []
        indptr = [0]
        for row_idx in range(n_cells):
            row_entries = [(col, accum[(row_idx, col)]) for col in range(n_regions) if (row_idx, col) in accum]
            for col, (sum_cov, sum_meth) in row_entries:
                data.append(sum_meth / sum_cov if sum_cov else 0.0)
                indices.append(col)
            indptr.append(len(data))
        matrix = sp.csr_matrix((data, indices, indptr), shape=(n_cells, n_regions))
    else:
        matrix = np.zeros((n_cells, n_regions), dtype=float)
        for (row_idx, col_idx), (sum_cov, sum_meth) in accum.items():
            matrix[row_idx, col_idx] = sum_meth / sum_cov if sum_cov else 0.0

    if return_mapping:
        cell_ids = [cell for cell, _ in sorted(cell_to_idx.items(), key=lambda kv: kv[1])]
        regions_info: Union[List[dict], 'pd.DataFrame']
        regions_info = [{"chrom": chrom, "start": start, "end": end} for chrom, start, end in region_list]
        if pd is not None:
            regions_info = _require_pandas().DataFrame(regions_info)
        return matrix, cell_ids, regions_info

    return matrix
