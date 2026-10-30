#include <verbalis/languages/registry.h>
#include <verbalis/steps/normalize.h>
#include <verbalis/steps/symbols.h>
#include <verbalis/steps/terms.h>
#include <verbalis/steps/trim.h>
#include <verbalis/text/document.h>
#include <verbalis/text/utf8.h>
#include <verbalis/verbalizer.h>

#include <optional>
#include <stdexcept>

namespace verbalis {

namespace {
using text::Document;
using text::Readings;

// Readings added by the caller, each one unconditional
Readings readings_of(const std::map<std::string, std::string>& words) {
  Readings readings;
  for (const auto& [written, spoken] : words) {
    readings[text::utf8_to_u32(written)] = {text::Reading(text::utf8_to_u32(spoken))};
  }
  return readings;
}

// A language's table with the caller's readings, which replace those with the same key
Readings merged(const Readings* table, const Readings& extra) {
  Readings result = table ? *table : Readings{};
  for (const auto& [key, readings] : extra) {
    result.insert_or_assign(key, readings);
  }
  return result;
}

const languages::Language& language_of(std::string_view code) {
  const auto* language = languages::find(code);
  if (!language) {
    throw std::invalid_argument("Unsupported language: " + std::string(code));
  }
  return *language;
}
} // namespace

// The steps of a language, in their fixed order: normalize, trim, terms, symbols with a condition, numbers, symbols.
class Verbalizer::Impl {
public:
  Impl(const languages::Language& language, const Options& options)
      : language_(language),
        options_(options),
        normalize_(language.tables().characters ? *language.tables().characters : steps::kDefaultCharacters) {
    const auto& tables = language.tables();
    Readings abbreviations = readings_of(options.abbreviations);
    if (options.terms && (tables.terms || !abbreviations.empty())) {
      steps::Terms terms = tables.terms ? *tables.terms : steps::Terms{};
      terms.abbreviations = merged(&terms.abbreviations, abbreviations);
      terms_.emplace(std::move(terms));
    }
    Readings symbol_words = readings_of(options.symbol_words);
    if (options.symbols && (tables.symbols || !symbol_words.empty())) {
      symbols_.emplace(merged(tables.symbols, symbol_words));
    }
  }

  void run(Document& document) const {
    if (options_.normalize) {
      normalize_.apply(document);
      document.commit();
    }
    if (options_.trim) {
      steps::trim(document);
      document.commit();
    }
    if (terms_) {
      terms_->apply(document);
      document.commit();
    }
    if (symbols_) {
      symbols_->apply(document, true);
      document.commit();
    }
    if (options_.numbers) {
      language_.read_numbers(document);
      document.commit();
    }
    if (symbols_) {
      symbols_->apply(document, false);
      document.commit();
    }
  }

private:
  const languages::Language& language_; // A static instance, which outlives every Verbalizer
  Options options_;
  steps::Normalizer normalize_;
  std::optional<steps::TermReader> terms_;
  std::optional<steps::SymbolReader> symbols_;
};

Verbalizer::Verbalizer(std::string_view language, const Options& options)
    : impl_(std::make_unique<Impl>(language_of(language), options)) {}

Verbalizer::~Verbalizer() = default;
Verbalizer::Verbalizer(Verbalizer&&) noexcept = default;
Verbalizer& Verbalizer::operator=(Verbalizer&&) noexcept = default;

std::string Verbalizer::process(std::string_view text) const {
  Document document(text::utf8_to_u32(text));
  impl_->run(document);
  return text::u32_to_utf8(document.text());
}

Result Verbalizer::process_with_changes(std::string_view text) const {
  Document document(text::utf8_to_u32(text));
  impl_->run(document);

  Result result{.text = text::u32_to_utf8(document.text())};
  for (const auto& [span, replacement] : document.changes()) {
    result.changes.push_back({span.begin, span.end, text::u32_to_utf8(replacement)});
  }
  return result;
}

const std::vector<std::string>& Verbalizer::languages() {
  return languages::codes();
}

} // namespace verbalis
