// Checks that Verbalis reproduces the parity baseline: every input of tests/parity/baseline/*.jsonl must verbalize
// to exactly its recorded output. Prints a summary per language and category, and the first mismatches.
#include <verbalis/verbalizer.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: verbalis_parity <baseline directory>\n";
    return 2;
  }

  constexpr int kShown = 10;
  int checked = 0;
  int failed = 0;
  std::map<std::string, std::pair<int, int>> totals; // language/category -> (checked, failed)

  for (const auto& entry : std::filesystem::directory_iterator(argv[1])) {
    if (entry.path().extension() != ".jsonl") {
      continue;
    }
    std::ifstream file(entry.path());
    std::string line;
    std::map<std::string, verbalis::Verbalizer> verbalizers;
    while (std::getline(file, line)) {
      auto record = nlohmann::json::parse(line);
      std::string language = record["lang"];
      std::string key = language + "/" + record["category"].get<std::string>();
      auto it = verbalizers.try_emplace(language, language).first;
      std::string output = it->second.process(record["input"].get<std::string>());

      checked++;
      totals[key].first++;
      if (output != record["output"].get<std::string>()) {
        failed++;
        totals[key].second++;
        if (failed <= kShown) {
          std::cerr << "[" << key << "]\n  input:    " << record["input"].get<std::string>()
                    << "\n  expected: " << record["output"].get<std::string>() << "\n  got:      " << output << "\n";
        }
      }
    }
  }

  for (const auto& [key, counts] : totals) {
    std::cout << key << ": " << counts.first - counts.second << "/" << counts.first << "\n";
  }
  std::cout << "parity: " << checked - failed << "/" << checked << " match\n";
  return failed == 0 && checked > 0 ? 0 : 1;
}
