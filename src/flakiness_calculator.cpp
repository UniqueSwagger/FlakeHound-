// flakiness calculator with Wilson confidence interval

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
  score.totalRuns = testResults.size();
  score.passes = 0;
  score.failures = 0;

  for (bool result : testResults) {
    if (result) {
      score.passes++;
    } else {
      score.failures++;
    }
  }

  if (score.totalRuns > 0) {
    double passRate = static_cast<double>(score.passes) / score.totalRuns;
    double failRate = static_cast<double>(score.failures) / score.totalRuns;
    double minRate = min(passRate, failRate);
    // coefficient near 0 means always passing or always failing; near 1 means
    // 50/50
    score.coefficient = 2.0 * minRate;
  } else {
    score.coefficient = 0.0;
  }
  score.wilsonScore = computeWilsonScore(score.passes, score.totalRuns);

  score.category =
      categorizeFlakiness(score.passes, score.failures, score.coefficient);

  return score;
}

vector<FlakinessScore> FlakinessCalculator::calculateForAllTests(
    map<string, vector<bool>>& allTestResults) {
  vector<FlakinessScore> scores;

  for (auto& entry : allTestResults) {
    string testName = entry.first;
    vector<bool> results = entry.second;
    FlakinessScore score = calculateFlakiness(testName, results);
    scores.push_back(score);
  }

  sort(scores.begin(), scores.end(),
       [](const FlakinessScore& a, const FlakinessScore& b) {
         return a.coefficient > b.coefficient;
       });

  return scores;
}

double FlakinessCalculator::computeWilsonScore(int passes, int totalRuns,
                                               double z) {
  if (totalRuns == 0) return 0.0;

  double p = (double)passes / totalRuns;
  double n = totalRuns;

  // Wilson score interval formula
  double numerator = p + (z * z) / (2 * n);
  double denominator = 1 + (z * z) / n;
  double sqrtTerm = sqrt((p * (1 - p) / n) + (z * z) / (4 * n * n));

  double wilsonScore = (numerator - z * sqrtTerm) / denominator;

  return wilsonScore;
}

string FlakinessCalculator::categorizeFlakiness(int passes, int failures,
                                                double coefficient) {
  int total = passes + failures;
  if (total == 0) return "no_data";

  if (failures == 0) return "stable";        // always passing
  if (passes == 0) return "always_failing";  // never passing

  // otherwise flaky to some degree
  if (coefficient < 0.4) {
    return "mildly_flaky";  // low-but-present flakiness
  } else {
    return "highly_flaky";  // significant flakiness (toward 50/50)
  }
}
