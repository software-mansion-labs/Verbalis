"""Rewrites text the way it is read aloud: numbers, dates, symbols, abbreviations, and web addresses as words."""

from importlib.metadata import version

from ._verbalis import Change, Options, Result, Verbalizer

__version__ = version("verbalis")
__all__ = ["Change", "Options", "Result", "Verbalizer", "__version__"]
