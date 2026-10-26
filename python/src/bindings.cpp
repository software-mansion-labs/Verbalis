// Python bindings of the public C++ API (cpp/include/verbalis/verbalizer.h), built as verbalis._verbalis.
#include <verbalis/verbalizer.h>

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <string>

namespace nb = nanobind;
using namespace nb::literals;

namespace {

std::string repr(const verbalis::Change& change) {
  return "Change(start=" + std::to_string(change.start) + ", end=" + std::to_string(change.end) +
         ", replacement=" + nb::repr(nb::str(change.replacement.c_str())).c_str() + ")";
}

} // namespace

NB_MODULE(_verbalis, m) {
  m.doc() =
      "Rewrites text the way it is read aloud: numbers, dates, symbols, abbreviations, and web addresses as words.";

  nb::class_<verbalis::Options>(m, "Options", "Which steps run, and readings added to the language's own.")
      .def(
          "__init__",
          [](verbalis::Options* options, bool normalize, bool trim, bool terms, bool numbers, bool symbols,
             std::map<std::string, std::string> abbreviations, std::map<std::string, std::string> symbol_words) {
            new (options) verbalis::Options{.normalize = normalize,
                                            .trim = trim,
                                            .terms = terms,
                                            .numbers = numbers,
                                            .symbols = symbols,
                                            .abbreviations = std::move(abbreviations),
                                            .symbol_words = std::move(symbol_words)};
          },
          nb::kw_only(), "normalize"_a = true, "trim"_a = true, "terms"_a = true, "numbers"_a = true,
          "symbols"_a = true, "abbreviations"_a = std::map<std::string, std::string>{},
          "symbol_words"_a = std::map<std::string, std::string>{})
      .def_rw("normalize", &verbalis::Options::normalize,
              "One form for equivalent characters: quotes, apostrophes, dashes, ellipses.")
      .def_rw("trim", &verbalis::Options::trim, "Single spaces between words, none at the edges or before punctuation.")
      .def_rw("terms", &verbalis::Options::terms, "Abbreviations and web addresses as words.")
      .def_rw("numbers", &verbalis::Options::numbers, "Numbers, dates, times, money, and units as words.")
      .def_rw("symbols", &verbalis::Options::symbols, "Remaining symbols as words.")
      .def_rw("abbreviations", &verbalis::Options::abbreviations, "Added or replaced abbreviation readings.")
      .def_rw("symbol_words", &verbalis::Options::symbol_words, "Added or replaced symbol readings.");

  nb::class_<verbalis::Change>(m, "Change",
                               "One edit of the input: text[start:end], in code points, became replacement.")
      .def_ro("start", &verbalis::Change::start)
      .def_ro("end", &verbalis::Change::end)
      .def_ro("replacement", &verbalis::Change::replacement)
      .def("__repr__", &repr);

  nb::class_<verbalis::Result>(m, "Result", "A verbalized text, with the edits that turned the input into it.")
      .def_ro("text", &verbalis::Result::text)
      .def_ro("changes", &verbalis::Result::changes);

  nb::class_<verbalis::Verbalizer>(m, "Verbalizer",
                                   "Rewrites text the way it is read aloud. Immutable, so one instance can serve "
                                   "many threads.")
      .def(nb::init<std::string_view, const verbalis::Options&>(), "language"_a, "options"_a = verbalis::Options{},
           "Raises ValueError if the language isn't supported; see Verbalizer.languages().")
      .def("process", &verbalis::Verbalizer::process, "text"_a, nb::call_guard<nb::gil_scoped_release>(),
           "The text as it is read aloud.")
      .def("process_with_changes", &verbalis::Verbalizer::process_with_changes, "text"_a,
           nb::call_guard<nb::gil_scoped_release>(),
           "The text as it is read aloud, with the edits of the input that produced it, in order.")
      .def_static("languages", &verbalis::Verbalizer::languages, "The supported language codes.");
}
