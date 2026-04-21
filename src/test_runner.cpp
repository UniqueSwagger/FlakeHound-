#include "test_runner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

using namespace std;

namespace {

bool canWriteToDirectory(const filesystem::path& directory) {
  error_code statusEc;
  filesystem::create_directories(directory, statusEc);
  if (!filesystem::exists(directory, statusEc)) {
    return false;
  }

  filesystem::path probe =
      directory / "flakehound_write_probe.tmp";
  FILE* file = fopen(probe.string().c_str(), "w");
  if (!file) {
    return false;
  }

  fclose(file);
  remove(probe.string().c_str());
  return true;
}

filesystem::path findWritableTempDirectory() {
  vector<filesystem::path> candidates;

  error_code ec;
  filesystem::path detectedTemp = filesystem::temp_directory_path(ec);
  if (!ec && !detectedTemp.empty()) {
    candidates.push_back(detectedTemp);
  }

  candidates.push_back("/tmp");
  candidates.push_back(filesystem::current_path());

  for (const filesystem::path& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }

    if (canWriteToDirectory(candidate)) {
      return candidate;
    }
  }

  return filesystem::current_path();
}

}  // namespace

TestRunner::TestRunner(string exePath) : executable(exePath) {}

TestRunner::~TestRunner() {}

vector<RunResult> TestRunner::runMultipleTimes(int times) {
  allResults.clear();

  cout << "Running tests " << times << " times...\n\n";

  for (int i = 1; i <= times; i++) {
    cout << "Run #" << i << " ... ";
    RunResult result = runOnce(i);
    allResults.push_back(result);
    cout << "Passed: " << result.passedTests
         << ", Failed: " << result.failedTests << "\n";
  }

  return allResults;
}

RunResult TestRunner::runOnce(int runNumber) {
  RunResult result;
  result.runNumber = runNumber;
  result.totalTests = 0;
  result.passedTests = 0;
  result.failedTests = 0;

  filesystem::path tempFile =
      findWritableTempDirectory() /
      ("flakehound_temp_run_" + to_string(runNumber) + "_" +
       to_string(hash<string>{}(executable)) + ".txt");
  string command = executable + " > \"" + tempFile.string() + "\" 2>&1";

  system(command.c_str());

  FILE* file = fopen(tempFile.string().c_str(), "r");
  if (file) {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
      result.output += buffer;
    }
    fclose(file);
  }

  parseOutput(result);

  remove(tempFile.string().c_str());

  return result;
}

void TestRunner::parseOutput(RunResult& result) {
  string output = result.output;

  size_t pos = 0;
  while ((pos = output.find("PASS:", pos)) != string::npos) {
    result.passedTests++;
    pos += 5;
  }

  pos = 0;
  while ((pos = output.find("FAIL:", pos)) != string::npos) {
    result.failedTests++;
    pos += 5;
  }

  result.totalTests = result.passedTests + result.failedTests;
}

map<string, int> TestRunner::getStats() {
  map<string, int> stats;

  int totalRuns = allResults.size();
  int totalPassed = 0;
  int totalFailed = 0;
  int totalTests = 0;

  for (auto& result : allResults) {
    totalPassed += result.passedTests;
    totalFailed += result.failedTests;
    totalTests += result.totalTests;
  }

  stats["runs"] = totalRuns;
  stats["total_passed"] = totalPassed;
  stats["total_failed"] = totalFailed;
  stats["total_tests"] = totalTests;

  return stats;
}
