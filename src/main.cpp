#include <iostream>
#include <string>
using namespace std;

void printUsage() {
  cout << "FlakeHound++ - Flaky Test Detection Tool\n";
  cout << "=========================================\n\n";
  cout << "Usage: flakehound [options]\n\n";
  cout << "Options:\n";
  cout << "  --run <test_executable> <times>    Run tests N times\n";
  cout << "  --analyze <xml_file>               Analyze test results\n";
  cout << "  --static <source_file>             Static code analysis\n";
  cout << "  --report <output_file>             Generate report\n";
  cout << "  --help                             Show this help\n";
  cout << "\nExample:\n";
  cout << "  flakehound --run demo_tests.exe 100\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printUsage();
    return 0;
  }

  string command = argv[1];

  if (command == "--help" || command == "-h") {
    printUsage();
    return 0;
  }

  cout << "FlakeHound++ v1.0.0\n";
  cout << "Command: " << command << "\n";
  cout << "Implementation in progress...\n";

  return 0;
}
