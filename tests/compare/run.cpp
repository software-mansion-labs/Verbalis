// Verbalizes JSON Lines from stdin: each {"lang": ..., "input": ...} line gets an "output" field. Uses only the public
// API, so the same file builds against any version of the library (see compare.sh).
#include <verbalis/verbalizer.h>

#include <nlohmann/json.hpp>

#include <iostream>
#include <map>
#include <string>

int main() {
  std::map<std::string, verbalis::Verbalizer> verbalizers;
  std::string line;
  while (std::getline(std::cin, line)) {
    auto record = nlohmann::json::parse(line);
    std::string language = record["lang"];
    auto it = verbalizers.try_emplace(language, language).first;
    record["output"] = it->second.process(record["input"].get<std::string>());
    std::cout << record.dump() << "\n";
  }
}
