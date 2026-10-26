#pragma once

#include <verbalis/processor/common/document.h>
#include <verbalis/processor/layer.h>

#include <string>
#include <string_view>

namespace verbalis::test {

// Applies a single preprocessing layer to a text.
inline std::u32string apply(const processor::Layer& layer, std::u32string_view text) {
  processor::Document document(text);
  layer.apply(document);
  document.commit();
  return std::u32string(document.text());
}

} // namespace verbalis::test
