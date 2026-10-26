#pragma once

#include <verbalis/text/document.h>
#include <verbalis/text/readings.h>
#include <verbalis/text/scan.h>

#include <string>
#include <vector>

namespace verbalis::steps {

using text::Document;
using text::Keys;
using text::Readings;


/**
 * Reads the symbols left in a text as words, after numbers are read.
 * A symbol whose readings all have unmet conditions stays as written.
 */
class SymbolReader {
public:
  explicit SymbolReader(Readings symbols);

  void apply(Document& document) const;

private:
  Readings symbols_;
  Keys keys_;
};

} // namespace verbalis::steps
