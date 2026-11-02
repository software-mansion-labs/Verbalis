// A minimal Hermes host: runs a JavaScript or Hermes bytecode file with print(text) and now() (milliseconds).
#include <chrono>
#include <fstream>
#include <hermes/hermes.h>
#include <iostream>
#include <jsi/jsi.h>
#include <sstream>

using namespace facebook;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: host <file.js|file.hbc>\n";
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  std::stringstream source;
  source << file.rdbuf();
  auto runtime = facebook::hermes::makeHermesRuntime();
  jsi::Runtime& r = *runtime;
  r.global().setProperty(
      r, "print",
      jsi::Function::createFromHostFunction(r, jsi::PropNameID::forAscii(r, "print"), 1,
                                            [](jsi::Runtime& r, const jsi::Value&, const jsi::Value* args, size_t) {
                                              std::cout << args[0].toString(r).utf8(r) << std::endl;
                                              return jsi::Value::undefined();
                                            }));
  r.global().setProperty(
      r, "now",
      jsi::Function::createFromHostFunction(
          r, jsi::PropNameID::forAscii(r, "now"), 0, [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t) {
            auto since = std::chrono::steady_clock::now().time_since_epoch();
            return jsi::Value(std::chrono::duration<double, std::milli>(since).count());
          }));
  try {
    r.evaluateJavaScript(std::make_shared<jsi::StringBuffer>(source.str()), argv[1]);
  } catch (const jsi::JSError& error) {
    std::cerr << error.what() << std::endl;
    return 1;
  }
  return 0;
}
