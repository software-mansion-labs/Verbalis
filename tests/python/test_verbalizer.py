"""The Python API: the same cases as tests/cpp/verbalizer_test.cpp, plus what is specific to Python."""

import threading

import pytest

from verbalis import Options, Verbalizer


def test_process():
    assert Verbalizer("en-us").process("I bought 3 tickets for $49.") == "I bought three tickets for forty-nine dollars."
    assert Verbalizer("pl").process("Mam 2 koty.") == "Mam dwa koty."


def test_changes_count_code_points():
    # The emoji before "5" is one code point, so the offsets index the Python string
    text = "🍎 5 apples & pears"
    result = Verbalizer("en-us").process_with_changes(text)

    assert result.text == "🍎 five apples and pears"
    assert [(c.start, c.end, c.replacement) for c in result.changes] == [(2, 3, "five"), (10, 13, " and ")]
    assert [text[c.start : c.end] for c in result.changes] == ["5", " & "]


def test_options():
    assert Verbalizer("en-us", Options(numbers=False)).process("3 cats & 2 dogs") == "3 cats and 2 dogs"

    custom = Options(abbreviations={"approx.": "approximately"}, symbol_words={"™": "trademark"})
    assert Verbalizer("en-us", custom).process("approx. 10 Acme™") == "approximately ten Acme trademark"


def test_options_fields():
    options = Options()
    assert (options.normalize, options.trim, options.terms, options.numbers, options.symbols) == (True,) * 5
    options.symbols = False
    assert Verbalizer("en-us", options).process("3 & 4") == "three & four"


def test_languages():
    for code in ["de", "en-us", "en-gb", "es", "fr", "hi", "it", "pl", "pt", "zh"]:
        assert code in Verbalizer.languages()

    with pytest.raises(ValueError, match="Unsupported language: xx"):
        Verbalizer("xx")


def test_threads_share_one_instance():
    english = Verbalizer("en-us")
    outputs = []
    threads = [threading.Thread(target=lambda: outputs.append(english.process("I have 2 cats."))) for _ in range(8)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    assert outputs == ["I have two cats."] * 8
