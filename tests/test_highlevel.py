import numpy as np
import pandas as pd
import pytest
import pydmtools as pydm
from pydmtools.highlevel import entries_to_df, per_cell_qc, region_matrix, write_matrix_mtx


def make_test_dm(tmp_path):
    fname = tmp_path / "sc.dm"
    dm = pydm.openfile(str(fname), "w")
    dm.addHeader([("chr1", 1000)], maxZooms=0)
    dm.addEntries(
        ["chr1", "chr1", "chr1", "chr1"],
        [10, 20, 30, 40],
        ends=[11, 21, 31, 41],
        values=[0.2, 0.8, 0.4, 0.6],
        coverages=[5, 3, 2, 4],
        contexts=["CG", "CG", "CH", "CG"],
        strands=["+", "-", "+", "-"],
        entryid=["cellA", "cellA", "cellB", "cellB"],
    )
    dm.close()
    return pydm.openfile(str(fname))


def test_entries_to_df_returns_expected_columns(tmp_path):
    dm = make_test_dm(tmp_path)
    df = entries_to_df(dm, "chr1")
    assert set(["chrom", "start", "end", "value"]).issubset(df.columns)
    assert "id" in df.columns
    assert len(df) == 4


def test_per_cell_qc_counts_sites_and_means(tmp_path):
    dm = make_test_dm(tmp_path)
    qc = per_cell_qc(dm, context="CG", min_coverage=3)
    assert set(qc.columns) == {"cell_id", "n_sites", "total_coverage", "mean_coverage", "mean_meth"}
    cellA = qc[qc["cell_id"] == "cellA"].iloc[0]
    assert cellA["n_sites"] == 2
    assert cellA["total_coverage"] == 8
    np.testing.assert_allclose(cellA["mean_meth"], (0.2*5 + 0.8*3)/8)


def test_region_matrix_shape_and_mapping(tmp_path):
    dm = make_test_dm(tmp_path)
    regions = [("chr1", 0, 15), ("chr1", 15, 35), ("chr1", 35, 50)]
    matrix, cell_ids, region_info = region_matrix(dm, regions, sparse=False, return_mapping=True)
    assert matrix.shape == (2, 3)
    assert set(cell_ids) == {"cellA", "cellB"}
    assert np.isclose(matrix[cell_ids.index("cellA"), 0], 0.2)
    assert np.isclose(matrix[cell_ids.index("cellA"), 1], 0.8)
    assert np.isclose(matrix[cell_ids.index("cellB"), 1], 0.4)
    assert np.isclose(matrix[cell_ids.index("cellB"), 2], 0.6)
    assert region_info[0]["chrom"] == "chr1"


def test_write_matrix_mtx_outputs(tmp_path):
    pytest.importorskip("scipy.io")
    dm = make_test_dm(tmp_path)
    regions = [("chr1", 0, 15), ("chr1", 15, 35), ("chr1", 35, 50)]
    out_prefix = tmp_path / "test_sc"

    matrix, cell_ids, _ = write_matrix_mtx(dm, regions, out_prefix=str(out_prefix), sparse=True, return_matrix=True)

    mtx_path = out_prefix.with_suffix(".mtx")
    barcodes_path = out_prefix.with_suffix(".barcodes.tsv")
    features_path = out_prefix.with_suffix(".features.tsv")

    assert mtx_path.exists() and mtx_path.stat().st_size > 0
    assert barcodes_path.exists() and barcodes_path.stat().st_size > 0
    assert features_path.exists() and features_path.stat().st_size > 0

    barcodes = barcodes_path.read_text().strip().splitlines()
    features = features_path.read_text().strip().splitlines()

    assert len(barcodes) == matrix.shape[0] == len(cell_ids)
    assert len(features) == matrix.shape[1]
