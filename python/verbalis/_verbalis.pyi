"""
Rewrites text the way it is read aloud: numbers, dates, symbols, abbreviations, and web addresses as words.
"""

from collections.abc import Mapping


class Options:
    """Which steps run, and readings added to the language's own."""

    def __init__(self, *, normalize: bool = True, trim: bool = True, terms: bool = True, numbers: bool = True, symbols: bool = True, abbreviations: Mapping[str, str] = {}, symbol_words: Mapping[str, str] = {}) -> None: ...

    @property
    def normalize(self) -> bool:
        """
        One form for equivalent characters: quotes, apostrophes, dashes, ellipses.
        """

    @normalize.setter
    def normalize(self, arg: bool, /) -> None: ...

    @property
    def trim(self) -> bool:
        """Single spaces between words, none at the edges or before punctuation."""

    @trim.setter
    def trim(self, arg: bool, /) -> None: ...

    @property
    def terms(self) -> bool:
        """Abbreviations and web addresses as words."""

    @terms.setter
    def terms(self, arg: bool, /) -> None: ...

    @property
    def numbers(self) -> bool:
        """Numbers, dates, times, money, and units as words."""

    @numbers.setter
    def numbers(self, arg: bool, /) -> None: ...

    @property
    def symbols(self) -> bool:
        """Remaining symbols as words."""

    @symbols.setter
    def symbols(self, arg: bool, /) -> None: ...

    @property
    def abbreviations(self) -> dict[str, str]:
        """Added or replaced abbreviation readings."""

    @abbreviations.setter
    def abbreviations(self, arg: Mapping[str, str], /) -> None: ...

    @property
    def symbol_words(self) -> dict[str, str]:
        """Added or replaced symbol readings."""

    @symbol_words.setter
    def symbol_words(self, arg: Mapping[str, str], /) -> None: ...

class Change:
    """
    One edit of the input: text[start:end], in code points, became replacement.
    """

    @property
    def start(self) -> int: ...

    @property
    def end(self) -> int: ...

    @property
    def replacement(self) -> str: ...

    def __repr__(self) -> str: ...

class Result:
    """A verbalized text, with the edits that turned the input into it."""

    @property
    def text(self) -> str: ...

    @property
    def changes(self) -> list[Change]: ...

class Verbalizer:
    """
    Rewrites text the way it is read aloud. Immutable, so one instance can serve many threads.
    """

    def __init__(self, language: str, options: Options = ...) -> None:
        """
        Raises ValueError if the language isn't supported; see Verbalizer.languages().
        """

    def process(self, text: str) -> str:
        """The text as it is read aloud."""

    def process_with_changes(self, text: str) -> Result:
        """
        The text as it is read aloud, with the edits of the input that produced it, in order.
        """

    @staticmethod
    def languages() -> list[str]:
        """The supported language codes."""
