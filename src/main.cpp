#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "test_runner.h"

using namespace std;

int main() {
  cout << "FlakeHound++ - Test Runner Demo\n";

  TestRunner runner(".\\demo\\demo_tests.exe");
  vector<RunResult> results = runner.runMultipleTimes(10);
  map<string, int> stats = runner.getStats();

  cout << "Overall Results\n";
  cout << "Total Runs: " << stats["runs"] << "\n";
  cout << "Total Tests Passed: " << stats["total_passed"] << "\n";
  cout << "Total Tests Failed: " << stats["total_failed"] << "\n";
  cout << "Total Tests: " << stats["total_tests"] << "\n\n";

  cout << "Flakiness Analysis\n";

  int test1_passes = 0, test2_passes = 0, test3_passes = 0, test4_passes = 0;

  for (auto& result : results) {
    string output = result.output;

    size_t pos = 0;
    while (pos < output.length()) {
      size_t newline_pos = output.find('\n', pos);
      if (newline_pos == string::npos) newline_pos = output.length();

      string line = output.substr(pos, newline_pos - pos);

      if (line.find("PASS: test_addition") != string::npos) {
        test1_passes++;
      }
      if (line.find("PASS: test_string_length") != string::npos) {
        test2_passes++;
      }
      if (line.find("PASS: test_random_number") != string::npos) {
        test3_passes++;
      }
      if (line.find("PASS: test_timing_based") != string::npos) {
        test4_passes++;
      }

      pos = newline_pos + 1;
    }
  }

  cout << "test_addition: PASSED " << test1_passes << "/10 times (Stability: ";
  if (test1_passes == 10)
    cout << "STABLE)\n";
  else
    cout << "FLAKY!)\n";

  cout << "test_string_length: PASSED " << test2_passes
       << "/10 times (Stability: ";
  if (test2_passes == 10)
    cout << "STABLE)\n";
  else
    cout << "FLAKY!)\n";

  cout << "test_random_number: PASSED " << test3_passes
       << "/10 times (Stability: ";
  if (test3_passes == 10)
    cout << "STABLE)\n";
  else if (test3_passes > 5)
    cout << "SOMEWHAT FLAKY)\n";
  else
    cout << "HIGHLY FLAKY!)\n";

  cout << "test_timing_based: PASSED " << test4_passes
       << "/10 times (Stability: ";
  if (test4_passes == 10)
    cout << "STABLE)\n";
  else if (test4_passes > 5)
    cout << "SOMEWHAT FLAKY)\n";
  else
    cout << "HIGHLY FLAKY!)\n";

  return 0;
}
