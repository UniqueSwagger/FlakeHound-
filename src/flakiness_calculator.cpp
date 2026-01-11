#include <algorithm>
#include <cmath>

#include "flakliness_calculator.h"

using namespace std;

FlakinessCalculator::FlakinessCalculator() {}

FlakinessCalculator::~FlakinessCalculator() {}

FlakinessScore FlakinessCalculator::calculateFlakiness(
    string& testName, vector<bool>& testResults) {
  FlakinessScore score;
  score.testName = testName;
  score.totalRuns = 0;
  score.passes = 0;
  score.failures = 0;
  score.coefficient = 0.0;
  score.wilsonScore = 0.0;
  score.category = "stable";

  return score;
}

vector<FlakinessScore> FlakinessCalculator::calculateForAllTests(
    map<string, vector<bool>>& allTestResults) {
  vector<FlakinessScore> scores;

  return scores;
}

double FlakinessCalculator::computeWilsonScore(int passes, int totalRuns,
                                               double z) {
  return 0.0;
}

string FlakinessCalculator::categorizeFlakiness(double coefficient) {
  return "stable";
}