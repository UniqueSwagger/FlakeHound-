#include "test_runner.h"

using namespace std;

TestRunner::TestRunner(string& executable) : executable_(executable) {}

TestRunner::~TestRunner() {}

vector<RunResult> TestRunner::runMultipleTimes(int times) { return results_; }

vector<RunResult> TestRunner::runWithVariations(int times) { return results_; }

map<string, int> TestRunner::getExecutionStats() {
  map<string, int> stats;
  return stats;
}

bool TestRunner::executeTest(string& outFile) { return true; }

void TestRunner::applyEnvironmentVariation(int runNumber) {}