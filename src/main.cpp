
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "flakliness_calculator.h"
#include "report_generator.h"
#include "test_runner.h"

using namespace std;

int main() {
  cout << "FlakeHound++ - Test Runner Demo\n";

  TestRunner runner(".\\demo_tests.exe");
  vector<RunResult> results = runner.runMultipleTimes(10);
  map<string, int> stats = runner.getStats();

  cout << "Overall Results\n";
  cout << "Total Runs: " << stats["runs"] << "\n";
  cout << "Total Tests Passed: " << stats["total_passed"] << "\n";
  cout << "Total Tests Failed: " << stats["total_failed"] << "\n";
  cout << "Total Tests: " << stats["total_tests"] << "\n\n";

  cout << "Flakiness Analysis\n";

  map<string, vector<bool>> testResults;
  testResults["test_addition"] = vector<bool>();
  testResults["test_string_length"] = vector<bool>();
  testResults["test_random_number"] = vector<bool>();
  testResults["test_timing_based"] = vector<bool>();

  for (auto& result : results) {
    string output = result.output;

    size_t pos = 0;
    while (pos < output.length()) {
      size_t newline_pos = output.find('\n', pos);
      if (newline_pos == string::npos) newline_pos = output.length();

      string line = output.substr(pos, newline_pos - pos);

      if (line.find("PASS: test_addition") != string::npos) {
        testResults["test_addition"].push_back(true);
      } else if (line.find("FAIL: test_addition") != string::npos) {
        testResults["test_addition"].push_back(false);
      }

      if (line.find("PASS: test_string_length") != string::npos) {
        testResults["test_string_length"].push_back(true);
      } else if (line.find("FAIL: test_string_length") != string::npos) {
        testResults["test_string_length"].push_back(false);
      }

      if (line.find("PASS: test_random_number") != string::npos) {
        testResults["test_random_number"].push_back(true);
      } else if (line.find("FAIL: test_random_number") != string::npos) {
        testResults["test_random_number"].push_back(false);
      }

      if (line.find("PASS: test_timing_based") != string::npos) {
        testResults["test_timing_based"].push_back(true);
      } else if (line.find("FAIL: test_timing_based") != string::npos) {
        testResults["test_timing_based"].push_back(false);
      }

      pos = newline_pos + 1;
    }
  }

  int test1_passes = 0, test2_passes = 0, test3_passes = 0, test4_passes = 0;

  for (bool b : testResults["test_addition"])
    if (b) test1_passes++;
  for (bool b : testResults["test_string_length"])
    if (b) test2_passes++;
  for (bool b : testResults["test_random_number"])
    if (b) test3_passes++;
  for (bool b : testResults["test_timing_based"])
    if (b) test4_passes++;

  FlakinessCalculator calculator;

  string test1_name = "test_addition";
  vector<bool> test1_results;
  for (bool b : testResults["test_addition"]) test1_results.push_back(b);
  FlakinessScore score1 =
      calculator.calculateFlakiness(test1_name, test1_results);

  string test2_name = "test_string_length";
  vector<bool> test2_results;
  for (bool b : testResults["test_string_length"]) test2_results.push_back(b);
  FlakinessScore score2 =
      calculator.calculateFlakiness(test2_name, test2_results);

  string test3_name = "test_random_number";
  vector<bool> test3_results;
  for (bool b : testResults["test_random_number"]) test3_results.push_back(b);
  FlakinessScore score3 =
      calculator.calculateFlakiness(test3_name, test3_results);

  string test4_name = "test_timing_based";
  vector<bool> test4_results;
  for (bool b : testResults["test_timing_based"]) test4_results.push_back(b);
  FlakinessScore score4 =
      calculator.calculateFlakiness(test4_name, test4_results);

  vector<FlakinessScore> scores;
  scores.push_back(score1);
  scores.push_back(score2);
  scores.push_back(score3);
  scores.push_back(score4);

  cout << "Flakiness Analysis\n";
  cout << "\n";
  for (auto& score : scores) {
    cout << score.testName << ": PASSED " << score.passes << "/"
         << score.totalRuns << " times (Category: " << score.category << ")\n";
  }
  cout << "\n";
  ReportGenerator reportGen;
  reportGen.generateReport("reports/flakiness_report.txt", scores);

  cout << "FlakeHound++ Analysis Complete!\n";

  return 0;
}
