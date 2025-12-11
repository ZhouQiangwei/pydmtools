"""
Python package wrapper for the pydmtools extension module.

This file imports the compiled extension shipped with the project and
re-exports its public API so ``import pydmtools`` works the same as before,
while also allowing submodules such as :mod:`pydmtools.highlevel`.
"""
from importlib import import_module
from types import ModuleType


def _load_extension() -> ModuleType:
    """Import the compiled extension packaged alongside this module."""
    try:
        return import_module("pydmtools.pydmtools")
    except ModuleNotFoundError as exc:  # pragma: no cover - import-time guard
        raise ImportError(
            "pydmtools native extension is missing. Build/install the project to use the low-level API"
        ) from exc


# Import the compiled extension (packaged as pydmtools.pydmtools)
_extension: ModuleType = _load_extension()

# Re-export everything that is not private from the extension module.
for _name in dir(_extension):
    if _name.startswith("__"):
        continue
    globals()[_name] = getattr(_extension, _name)

__all__ = [name for name in dir(_extension) if not name.startswith("__")]
