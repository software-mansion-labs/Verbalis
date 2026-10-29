"""What is specific to the Python API. The shared cases (outputs, options, changes) are in tests/parity/api.jsonl."""

import threading

import pytest

from verbalis import Options, Verbalizer


def test_changes_count_code_points():
    # The emoji before "5" is one code point, so the offsets index the Python string
    text = "🍎 5 apples & pears"
    result = Verbalizer("en-us").process_with_changes(text)

    assert [text[c.start : c.end] for c in result.changes] == ["5", " & "]


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
