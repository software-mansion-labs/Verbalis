#include "terms.h"

#include <verbalis/text/context.h>
#include <verbalis/text/strings.h>
#include <verbalis/text/unicode.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace verbalis::steps {

using namespace text;


namespace {
// Whether the labels of a host end with a top-level domain, which makes a bare host an address: example.com, gov.in
bool is_domain(const std::vector<std::u32string_view>& labels) {
  static const std::unordered_set<std::u32string> domains = {
      U"com", U"org", U"net", U"edu",  U"gov", U"io", U"pl", U"de", U"fr", U"es", U"it", U"pt", U"uk",
      U"eu",  U"at",  U"ch",  U"info", U"tv",  U"co", U"br", U"me", U"mx", U"us", U"ru", U"nl"};
  // India's only under its second-level domains: rbi.org.in, but not the Polish m.in.
  static const std::unordered_set<std::u32string> indian = {U"gov", U"nic", U"org", U"co", U"ac", U"edu", U"net"};
  size_t n = labels.size();
  std::u32string last = strings::to_lower(labels[n - 1]);
  return n >= 2 &&
         (domains.contains(last) || (n >= 3 && last == U"in" && indian.contains(strings::to_lower(labels[n - 2]))));
}

// Whether a part of a path is a file name, by its extension: report.pdf
bool is_file(std::u32string_view part) {
  static const std::unordered_set<std::u32string> extensions = {
      U"pdf",  U"jpg", U"jpeg", U"png", U"gif",  U"csv", U"txt", U"doc", U"docx", U"xls", U"xlsx", U"ppt",
      U"pptx", U"zip", U"mp3",  U"mp4", U"html", U"htm", U"php", U"js",  U"py",   U"exe", U"json", U"xml"};
  size_t dot = part.rfind(U'.');
  return dot != std::u32string_view::npos && dot > 0 && extensions.contains(strings::to_lower(part.substr(dot + 1)));
}

// A character of a part of a path: Users, report.pdf, my_file-2
bool is_path(char32_t c) {
  return unicode::is_latin(c) || unicode::isdigit(c) || c == U'.' || c == U'-' || c == U'_';
}

// A character of the user name of an email: jan.kowalski+news
bool is_user(char32_t c) {
  return unicode::isword(c) || strings::contains(std::u32string_view(U".-+%"), c);
}

// A character of a host label: my-site
bool is_label(char32_t c) {
  return unicode::isword(c) || c == U'-';
}

// A part of an address with its dots (and hyphens, if the language reads them) read: jan.kowalski -> jan dot kowalski
// A part of an address with its separators read, and its digits apart, to be read one by one: a1 -> a 1
std::u32string dotted(std::u32string_view part, const WebWords& web) {
  std::u32string words;
  for (size_t i = 0; i < part.size(); ++i) {
    char32_t c = part[i];
    const std::u32string& word = c == U'.' ? web.dot : c == U'-' ? web.hyphen : c == U'_' ? web.underscore : U"";
    if (!word.empty()) {
      words += U" " + word + U" ";
    } else if (unicode::isdigit(c)) {
      bool apart = !words.empty() && words.back() != U' ';
      bool next = i + 1 < part.size() && part[i + 1] != U'.' && part[i + 1] != U'-' && part[i + 1] != U'_';
      std::u32string digit = web.digits.size() == 10 ? web.digits[c - U'0'] : std::u32string(1, c);
      words += (apart ? U" " : U"") + digit + (next ? U" " : U"");
    } else {
      words += c;
    }
  }
  return words;
}
} // namespace

TermReader::TermReader(Terms terms) : terms_(std::move(terms)) {
  for (const auto& [abbreviation, readings] : terms_.abbreviations) {
    keys_.push_back(abbreviation);
  }
}

void TermReader::apply(Document& document) const {
  std::u32string_view text = document.text();

  for (size_t position = 0; position < text.size();) {
    // Terms start at word boundaries, but not after a dot: e.g. is not g.
    std::optional<Term> term;
    if (is_clear_before(text, position, U".")) {
      term = read_uuid(text, position);
      if (!term) {
        term = read_address(text, position);
      }
      if (!term) {
        term = read_path(text, position);
      }
      if (!term) {
        term = read_abbreviation(text, position);
      }
    }

    if (!term) {
      position++;
      continue;
    }
    document.replace(term->span, std::move(term->words));
    position = term->span.end;
  }
}

std::optional<TermReader::Term> TermReader::read_uuid(std::u32string_view text, size_t position) const {
  const WebWords& web = terms_.web;
  if (web.digits.size() != 10) {
    return std::nullopt;
  }
  auto hex = [](char32_t c) { return unicode::isdigit(c) || (c >= U'a' && c <= U'f') || (c >= U'A' && c <= U'F'); };
  size_t end = position;
  for (size_t length : {8, 4, 4, 4, 12}) {
    if (end > position && (end >= text.size() || text[end++] != U'-')) {
      return std::nullopt;
    }
    for (size_t i = 0; i < length; ++i, ++end) {
      if (end >= text.size() || !hex(text[end])) {
        return std::nullopt;
      }
    }
  }
  if (end < text.size() && unicode::isword(text[end])) {
    return std::nullopt;
  }

  std::vector<std::u32string> words;
  for (char32_t c : text.substr(position, end - position)) {
    if (c != U'-') {
      words.push_back(unicode::isdigit(c) ? web.digits[c - U'0'] : std::u32string(1, unicode::tolower(c)));
    }
  }
  return Term{.span = {position, end}, .words = join(words)};
}

std::optional<TermReader::Term> TermReader::read_address(std::u32string_view text, size_t position) const {
  const WebWords& web = terms_.web;

  // 1. An optional scheme, which is not read: https://
  size_t host = position;
  while (host < text.size() && unicode::is_latin(text[host])) {
    host++;
  }
  bool scheme = host > position && text.substr(host).starts_with(U"://");
  host = scheme ? host + 3 : position;

  // 2. The user of an email: jan.kowalski@
  std::u32string_view user;
  if (size_t at = host; !scheme) {
    while (at < text.size() && is_user(text[at])) {
      at++;
    }
    if (at > host && at < text.size() && text[at] == U'@') {
      user = text.substr(host, at - host);
      host = at + 1;
    }
  }

  // 3. The host: labels joined by dots
  std::vector<std::u32string_view> labels;
  size_t end = host;
  while (true) {
    size_t label_end = end;
    while (label_end < text.size() && is_label(text[label_end])) {
      label_end++;
    }
    if (label_end == end) {
      break;
    }
    labels.push_back(text.substr(end, label_end - end));
    end = label_end;
    if (end + 1 >= text.size() || text[end] != U'.' || !is_label(text[end + 1])) {
      break;
    }
    end++;
  }

  // A bare host needs a known top-level domain: example.com, but not file.txt or e.g
  bool www = !labels.empty() && strings::to_lower(labels[0]) == U"www";
  bool domain = !labels.empty() && is_domain(labels);
  if (labels.size() < 2 || !(scheme || www || domain || !user.empty())) {
    return std::nullopt;
  }

  // 4. An optional path, without the punctuation closing it: (see example.com/news).
  size_t path_end = end;
  if (end < text.size() && text[end] == U'/') {
    while (path_end < text.size() && !unicode::isspace(text[path_end])) {
      path_end++;
    }
    while (path_end > end + 1 && strings::contains(kPunctuation, text[path_end - 1])) {
      path_end--;
    }
  }

  // 5. The words: jan at example dot com, w w w dot example dot com slash news
  std::u32string words = user.empty() ? U"" : dotted(user, web) + U" " + web.at + U" ";
  for (size_t i = 0; i < labels.size(); ++i) {
    bool spelled = i == 0 && www;
    words += (i > 0 ? U" " + web.dot + U" " : U"") + (spelled ? U"w w w" : dotted(labels[i], web));
  }
  for (auto path = text.substr(end, path_end - end); !path.empty();) {
    size_t next = path.find(U'/', 1);
    std::u32string_view segment = path.substr(1, next == std::u32string_view::npos ? next : next - 1);
    words += U" " + web.slash + (segment.empty() ? U"" : U" " + dotted(segment, web));
    path = next == std::u32string_view::npos ? U"" : path.substr(next);
  }
  return Term{.span = {position, path_end}, .words = words};
}

std::optional<TermReader::Term> TermReader::read_path(std::u32string_view text, size_t position) const {
  const WebWords& web = terms_.web;
  if (web.backslash.empty()) {
    return std::nullopt;
  }

  // 1. The token, without the punctuation closing it: (see /etc/apache).
  size_t end = position;
  while (end < text.size() && !unicode::isspace(text[end])) {
    end++;
  }
  while (end > position && (strings::contains(kPunctuation, text[end - 1]) || text[end - 1] == U'।')) {
    end--;
  }
  std::u32string_view token = text.substr(position, end - position);

  // 2. An optional drive, then the parts between the separators: C:\Users\HP
  std::vector<std::u32string> words;
  char32_t separator = U'/';
  if (token.size() > 3 && unicode::is_latin(token[0]) && token[1] == U':' && token[2] == U'\\') {
    words = {std::u32string(1, token[0]), web.colon};
    token = token.substr(2);
    separator = U'\\';
  }
  std::vector<std::u32string_view> parts;
  for (size_t start = 0;;) {
    size_t next = token.find(separator, start);
    parts.push_back(token.substr(start, next == std::u32string_view::npos ? next : next - start));
    if (next == std::u32string_view::npos) {
      break;
    }
    start = next + 1;
  }

  // 3. A path from the root (/etc/apache), a drive, or from a folder of lowercase words (home/desktop, but not
  // PCB/PCM or km/h); or a file name
  bool rooted = parts[0].empty() || !words.empty();
  bool valid = std::ranges::all_of(parts, [](auto part) { return std::ranges::all_of(part, is_path); });
  auto named = [](std::u32string_view part) {
    return part.size() >= 3 && std::ranges::all_of(part, [](char32_t c) { return c >= U'a' && c <= U'z'; });
  };
  size_t named_parts = std::ranges::count_if(parts, [](auto part) { return !part.empty(); });
  bool path = rooted ? named_parts >= 2 : parts.size() >= 2 && std::ranges::all_of(parts, named);
  bool file = parts.size() == 1 && is_file(parts[0]) && unicode::is_latin(parts[0][0]);
  if (!valid || !(path || file)) {
    return std::nullopt;
  }

  // 4. The words: फॉरवर्ड स्लैश etc फॉरवर्ड स्लैश apache, report डॉट pdf
  const std::u32string& slash = separator == U'/' ? web.slash : web.backslash;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      words.push_back(slash);
    }
    words.push_back(dotted(parts[i], web));
  }
  return Term{.span = {position, end}, .words = join(words)};
}

std::optional<TermReader::Term> TermReader::read_abbreviation(std::u32string_view text, size_t position) const {
  for (auto key : keys_at(text, position, keys_)) {
    // An abbreviation ending with a letter ends a word, or touches a number: nr5
    size_t end = position + key.size();
    if (unicode::isword(key.back()) && !is_clear_after(text, end) && !unicode::isdigit(text[end])) {
      continue;
    }

    auto words = read(terms_.abbreviations.at(std::u32string(key)), Context(text, {position, end}));
    if (!words) {
      continue;
    }

    // A dot which also ends the text stays with it; a word right after gets a space: nr5 -> numer 5
    if (key.ends_with(U'.') && is_sentence_end(text, end)) {
      *words += U".";
    }
    if (end < text.size() && unicode::isword(text[end])) {
      *words += U" ";
    }
    return Term{.span = {position, end}, .words = *words};
  }
  return std::nullopt;
}

} // namespace verbalis::steps
