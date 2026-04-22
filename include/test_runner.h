#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <map>
#include <string>
#include <vector>

using namespace std;

struct RunResult {
  int runNumber;
  string output;
  int totalTests;
  int passedTests;
  int failedTests;
};

class TestRunner {
 public:
  TestRunner(string exePath);

  vector<RunResult> runMultipleTimes(int times);

  map<string, int> getStats();

 private:
  string executable;
  vector<RunResult> allResults;

  RunResult runOnce(int runNumber);

  void parseOutput(RunResult& result);
};

#endif
