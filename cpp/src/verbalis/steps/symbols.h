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
 * Reads the symbols left in a text as words, in two passes.
 * A reading with a condition looks at the text as written, so it is read before numbers are: p<0.05, Disney+, 50%+1.
 * A plain reading is read after them. A symbol with neither stays as written: <i>, Año~
 */
class SymbolReader {
public:
  explicit SymbolReader(Readings symbols);

  // Reads the symbols with a reading whose condition holds (before numbers), or with a plain reading (after)
  void apply(Document& document, bool conditional) const;

private:
  Readings symbols_;
  Keys keys_;
};

} // namespace verbalis::steps
