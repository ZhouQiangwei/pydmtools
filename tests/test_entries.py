import os
import pydmtools as pydm


def test_entries_exposes_metadata(tmp_path):
    out = tmp_path / "with_meta.dm"
    dm = pydm.openfile(str(out), "w")
    dm.addHeader([("chr1", 1000)], maxZooms=0)
    dm.addEntries([
        "chr1",
    ], [0], ends=[2], values=[0.5], coverages=[7], strands=["+"], contexts=["CG"], entryid=["readA"])
    dm.close()

    reader = pydm.openfile(str(out))
    header = reader.header()
    assert header["type"] & pydm.BM_COVER
    assert header["type"] & pydm.BM_STRAND
    assert header["type"] & pydm.BM_CONTEXT
    assert header["type"] & pydm.BM_ID
    assert set(header["fields"]) == {"end", "coverage", "strand", "context", "id"}

    records = reader.entries("chr1", 0, 5)
    assert isinstance(records, list)
    assert records[0]["coverage"] == 7
    assert records[0]["strand"] == "+"
    assert records[0]["context"] == "CG"
    assert records[0]["id"] == "readA"

    # Dropping a stored field is allowed.
    no_cov = reader.entries("chr1", 0, 5, with_coverage=False)
    assert isinstance(no_cov, list)
    assert "coverage" not in no_cov[0] or no_cov[0].get("coverage") is None

    reader.close()


def test_requesting_missing_field_raises(tmp_path):
    out = tmp_path / "no_meta.dm"
    dm = pydm.openfile(str(out), "w")
    dm.addHeader([("chr1", 1000)], maxZooms=0)
    dm.addEntries([
        "chr1",
    ], [0], ends=[2], values=[0.5])
    dm.close()

    reader = pydm.openfile(str(out))
    header = reader.header()
    assert "coverage" not in header["fields"]

    try:
        reader.entries("chr1", 0, 5, with_coverage=True)
    except RuntimeError:
        pass
    else:
        raise AssertionError("Requesting missing coverage should raise")
    reader.close()
