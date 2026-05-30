#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "audio recognition engine" << std::endl;
    return 1;
  }

  std::string mode = argv[1];
  std::cout << "mode" << std::endl;

  return 0;
}
