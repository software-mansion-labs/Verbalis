#include "utf8.h"

namespace verbalis::text {

std::u32string utf8_to_u32(std::string_view utf8) {
  // By the length of a sequence: the payload bits of its lead byte, and the smallest code point it may encode
  constexpr char32_t kLeadBits[] = {0, 0x7F, 0x1F, 0x0F, 0x07};
  constexpr char32_t kMinimum[] = {0, 0, 0x80, 0x800, 0x10000};

  std::u32string result;
  result.reserve(utf8.size());
  for (size_t i = 0; i < utf8.size();) {
    auto lead = static_cast<unsigned char>(utf8[i]);
    size_t length = lead < 0x80           ? 1 :
                    (lead & 0xE0) == 0xC0 ? 2 :
                    (lead & 0xF0) == 0xE0 ? 3 :
                    (lead & 0xF8) == 0xF0 ? 4 :
                                            0;
    char32_t codepoint = lead & kLeadBits[length];
    size_t read = 1;
    for (; read < length && i + read < utf8.size(); read++) {
      auto byte = static_cast<unsigned char>(utf8[i + read]);
      if ((byte & 0xC0) != 0x80) { // Not a '10xxxxxx' continuation byte
        break;
      }
      codepoint = (codepoint << 6) | (byte & 0x3Fu);
    }

    // A bad lead byte, a cut-off or overlong sequence, a surrogate, or a code point past U+10FFFF: one replacement
    // character for the first byte, and decoding resumes at the next one, so no valid character is lost
    bool valid = length > 0 && read == length && codepoint >= kMinimum[length] && codepoint <= 0x10FFFF &&
                 (codepoint < 0xD800 || codepoint > 0xDFFF);
    if (!valid) {
      result.push_back(0xFFFD);
      i++;
      continue;
    }
    result.push_back(codepoint);
    i += length;
  }
  return result;
}

std::string u32_to_utf8(std::u32string_view u32) {
  std::string result;
  result.reserve(u32.size());

  for (char32_t c : u32) {
    if (c <= 0x7F) { // Standard ASCII character
      result.push_back(static_cast<char>(c));
    } else if (c <= 0x7FF) {                                          // 2-byte unicode character (max 11 bits)
      result.push_back(static_cast<char>(0xC0 | (c >> 6)));           // '110' + 5 bits
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));         // '10' + 6 bits
    } else if (c <= 0xFFFF) {                                         // 3-byte unicode character (max 16 bits)
      result.push_back(static_cast<char>(0xE0 | (c >> 12)));          // '1110' + 4 bits
      result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));  // '10' + 6 bits
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));         // '10' + 6 bits
    } else if (c <= 0x10FFFF) {                                       // 4-byte unicode character (max 21 bits)
      result.push_back(static_cast<char>(0xF0 | (c >> 18)));          // '11110' + 3 bits
      result.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F))); // '10' + 6 bits
      result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));  // '10' + 6 bits
      result.push_back(static_cast<char>(0x80 | (c & 0x3F)));         // '10' + 6 bits
    }
  }

  return result;
}

} // namespace verbalis::text
