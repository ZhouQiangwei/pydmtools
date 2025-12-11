import pathlib
import pytest

pydm = pytest.importorskip("pydmtools")

TEST_DM = pathlib.Path(__file__).resolve().parent.parent / "pydmtest" / "test.dm"


def test_context_manager_returns_self():
    with pydm.openfile(str(TEST_DM)) as dm:
        assert dm is not None
        assert dm.isbinaMeth()


def test_close_is_idempotent():
    dm = pydm.openfile(str(TEST_DM))
    dm.close()
    # A second close should be a no-op rather than crashing
    dm.close()
