#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <map>
#include <string>
#include <vector>
using namespace std;
struct RunResult {
  int runNumber;
  string xmlOutput;
  bool success;
};

class TestRunner {
 public:
  TestRunner(string& executable);
  ~TestRunner();

  vector<RunResult> runMultipleTimes(int times);

  vector<RunResult> runWithVariations(int times);

  map<string, int> getExecutionStats();

 private:
  string executable_;
  vector<RunResult> results_;

  // helper methods
  bool executeTest(string& outFile);
  void applyEnvironmentVariation(int runnumber);
};

#endif  // !TEST_RUNNER_H