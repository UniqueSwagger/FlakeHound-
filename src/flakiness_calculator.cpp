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
    double failureRate = (double)score.failures / score.totalRuns;
    score.coefficient = 1.0 - abs(2.0 * failureRate - 1.0);
  } else {
    score.coefficient = 0.0;
  }
  score.wilsonScore = computeWilsonScore(score.passes, score.totalRuns);

  score.category = categorizeFlakiness(score.coefficient);

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

string FlakinessCalculator::categorizeFlakiness(double coefficient) {
  if (coefficient < 0.1) {
    return "stable";  // less than 10% flakiness
  } else if (coefficient < 0.5) {
    return "mildly_flaky";  // 10-50% flakiness
  } else {
    return "highly_flaky";  // more than 50% flakiness
  }
}
