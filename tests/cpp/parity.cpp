// Checks that Verbalis reproduces the parity outputs: every input of tests/parity/outputs/*.jsonl must verbalize
// to exactly its recorded output, and every case of tests/parity/api.jsonl (options, offsets, and changes) must
// produce exactly its recorded text and changes. Prints a summary per language and category, and the first mismatches.
#include <verbalis/verbalizer.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace {

verbalis::Options options_of(const nlohmann::json& json) {
  verbalis::Options options;
  options.normalize = json.value("normalize", true);
  options.trim = json.value("trim", true);
  options.terms = json.value("terms", true);
  options.numbers = json.value("numbers", true);
  options.symbols = json.value("symbols", true);
  options.abbreviations = json.value("abbreviations", std::map<std::string, std::string>{});
  options.symbol_words = json.value("symbol_words", std::map<std::string, std::string>{});
  return options;
}

// The API cases which differ from their recorded text or changes
int check_api(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::string line;
  int checked = 0;
  int failed = 0;
  while (std::getline(file, line)) {
    auto record = nlohmann::json::parse(line);
    verbalis::Verbalizer verbalizer(record["lang"].get<std::string>(), options_of(record["options"]));
    verbalis::Result result = verbalizer.process_with_changes(record["input"].get<std::string>());

    nlohmann::json changes = nlohmann::json::array();
    for (const auto& change : result.changes) {
      changes.push_back({change.start, change.end, change.replacement});
    }
    checked++;
    if (result.text != record["output"].get<std::string>() || changes != record["changes"]) {
      failed++;
      std::cerr << "[api " << record["lang"].get<std::string>()
                << "]\n  input:    " << record["input"].get<std::string>()
                << "\n  expected: " << record["output"].get<std::string>() << " " << record["changes"].dump()
                << "\n  got:      " << result.text << " " << changes.dump() << "\n";
    }
  }
  std::cout << "api: " << checked - failed << "/" << checked << " match\n";
  return checked > 0 ? failed : 1;
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) {
    std::cerr << "usage: verbalis_parity <outputs directory> [api cases .jsonl]\n";
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

  int api_failed = argc == 3 ? check_api(argv[2]) : 0;
  return failed == 0 && checked > 0 && api_failed == 0 ? 0 : 1;
}
