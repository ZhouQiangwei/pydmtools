"""
Python package wrapper for the pydmtools extension module.

This file imports the compiled extension shipped with the project and
re-exports its public API so ``import pydmtools`` works the same as before,
while also allowing submodules such as :mod:`pydmtools.highlevel`.
"""
from importlib import import_module
from types import ModuleType

try:  # Python 3.8+ standard library
    from importlib.metadata import PackageNotFoundError, version
except ImportError:  # pragma: no cover - fallback for very old Pythons
    from importlib_metadata import PackageNotFoundError, version


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

try:
    __version__ = version("pydmtools")
except PackageNotFoundError:  # pragma: no cover - fallback for editable installs
    __version__ = getattr(_extension, "__version__", "0.0.0")

# Re-export everything that is not private from the extension module.
for _name in dir(_extension):
    if _name.startswith("__"):
        continue
    globals()[_name] = getattr(_extension, _name)

__all__ = [name for name in dir(_extension) if not name.startswith("__")]
__all__.append("__version__")
