#pragma once

#include <string>
#include <string_view>

/**
 * UTF-8 and UTF-32 conversions
 *
 * Text arrives and leaves as UTF-8, the encoding for storage and transport. Inside, it is UTF-32: one element per code
 * point, so indexing, scanning, and offsets work the same for every script.
 */
namespace verbalis::text {

/**
 * Converts a UTF-8 string to a UTF-32 string. Each byte that doesn't start a valid sequence becomes U+FFFD, the
 * replacement character, and decoding resumes at the next byte.
 * @param utf8 The UTF-8 string to convert.
 * @return A UTF-32 encoded string.
 */
std::u32string utf8_to_u32(std::string_view utf8);

/**
 * Converts a UTF-32 string to a UTF-8 string.
 * @param u32 The UTF-32 string to convert.
 * @return A UTF-8 encoded string.
 */
std::string u32_to_utf8(std::u32string_view u32);

} // namespace verbalis::text