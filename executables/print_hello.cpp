#include <iostream>
#include <cpp_template/tools/tools.h>
#include <cpp_template/tools/named_tools.h>

using namespace cpp_structure::tools;

int main() {
  PrintHello();
  std::cout << std::endl;

  const NamedTools tools("Nikita");
  tools.printHello();
  return 0;
}
