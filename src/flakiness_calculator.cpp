// flakiness calculator with Wilson confidence interval

#include <algorithm>
#include <cmath>

#include "flakliness_calculator.h"

using namespace std;

vector<double> buildRankingFeatures(const FlakinessScore& score) {
  double failRate =
      score.totalRuns > 0 ? (double)(score.failures) / score.totalRuns : 0.0;
  double runCoverage = min(1.0, (double)(score.totalRuns) / 10.0);

  return {score.coefficient,       score.transitionRate,         failRate,
          1.0 - score.wilsonScore, score.staticRiskScore * 0.25, runCoverage};
}

FlakinessCalculator::FlakinessCalculator(double threshold) {
  rankingModelReady = false;
  flakyThreshold = threshold;
  initializeRankingModel();
}

FlakinessScore FlakinessCalculator::calculateFlakiness(
    const string& testName, const vector<bool>& testResults,
    double staticRiskScore, const vector<string>& likelyCauses) {
  FlakinessScore score;
  score.testName = testName;
  score.totalRuns = (int)(testResults.size());
  score.passes = 0;
  score.failures = 0;
  score.transitions = countTransitions(testResults);
  score.transitionRate =
      computeTransitionRate(score.transitions, score.totalRuns);
  score.staticRiskScore = staticRiskScore;
  score.rankingScore = 0.0;
  score.likelyCauses = likelyCauses;

  for (bool result : testResults) {
    if (result) {
      score.passes++;
    } else {
      score.failures++;
    }
  }

  if (score.totalRuns > 0) {
    double passRate = (double)(score.passes) / score.totalRuns;
    double failRate = (double)(score.failures) / score.totalRuns;
    score.coefficient = 2.0 * min(passRate, failRate);
  } else {
    score.coefficient = 0.0;
  }

  score.wilsonScore = computeWilsonScore(score.passes, score.totalRuns);
  score.category =
      categorizeFlakiness(score.passes, score.failures, score.coefficient);
  score.rankingScore = predictRankingScore(score);

  return score;
}

vector<FlakinessScore> FlakinessCalculator::calculateForAllTests(
    map<string, vector<bool>>& allTestResults,
    const map<string, double>& staticRiskScores,
    const map<string, vector<string>>& likelyCauses) {
  vector<FlakinessScore> scores;

  for (auto& entry : allTestResults) {
    const string& testName = entry.first;
    const vector<bool>& results = entry.second;

    double staticRisk = 0.0;
    auto riskIt = staticRiskScores.find(testName);
    if (riskIt != staticRiskScores.end()) {
      staticRisk = riskIt->second;
    }

    vector<string> causes;
    auto causesIt = likelyCauses.find(testName);
    if (causesIt != likelyCauses.end()) {
      causes = causesIt->second;
    }

    scores.push_back(calculateFlakiness(testName, results, staticRisk, causes));
  }

  sort(scores.begin(), scores.end(),
       [](const FlakinessScore& a, const FlakinessScore& b) {
         if (a.rankingScore == b.rankingScore) {
           if (a.coefficient == b.coefficient) {
             return a.testName < b.testName;
           }
           return a.coefficient > b.coefficient;
         }
         return a.rankingScore > b.rankingScore;
       });

  return scores;
}

void FlakinessCalculator::initializeRankingModel() {
  if (rankingModelReady) {
    return;
  }

  // Feature order: flakiness coefficient, transition rate, fail rate,
  // inverse Wilson score, scaled static pre-run risk, run coverage.
  vector<vector<double>> features = {
      {0.00, 0.00, 0.00, 0.05, 0.00, 1.0}, {0.05, 0.00, 0.05, 0.10, 0.02, 0.8},
      {0.10, 0.10, 0.05, 0.15, 0.03, 0.8}, {0.00, 0.00, 1.00, 1.00, 0.02, 1.0},
      {0.00, 0.00, 0.00, 0.20, 0.20, 1.0}, {0.35, 0.30, 0.30, 0.35, 0.05, 0.8},
      {0.45, 0.50, 0.40, 0.40, 0.08, 1.0}, {0.60, 0.65, 0.50, 0.45, 0.10, 1.0},
      {0.75, 0.75, 0.50, 0.35, 0.12, 1.0}, {0.25, 0.30, 0.15, 0.25, 0.18, 0.8}};
  // Labels: 0 = lower flakiness priority, 1 = higher flakiness priority.
  vector<int> labels = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1};

  rankingModel.initialize(features.front().size());
  rankingModel.train(features, labels, 600, 0.05, 0.0005);
  rankingModelReady = true;
}

double FlakinessCalculator::computeWilsonScore(int passes, int totalRuns,
                                               double z) {
  if (totalRuns == 0) {
    return 0.0;
  }
  double p = (double)(passes) / totalRuns;
  double n = totalRuns;

  double numerator = p + (z * z) / (2 * n);
  double denominator = 1 + (z * z) / n;
  double sqrtTerm = sqrt((p * (1 - p) / n) + (z * z) / (4 * n * n));

  double wilsonScore = (numerator - z * sqrtTerm) / denominator;
  if (wilsonScore < 0.0) {
    wilsonScore = 0.0;
  }
  if (wilsonScore > 1.0) {
    wilsonScore = 1.0;
  }
  return wilsonScore;
}

int FlakinessCalculator::countTransitions(
    const vector<bool>& testResults) const {
  if (testResults.size() < 2) {
    return 0;
  }

  int transitions = 0;
  for (int i = 1; i < testResults.size(); i++) {
    if (testResults[i] != testResults[i - 1]) {
      transitions++;
    }
  }
  return transitions;
}

double FlakinessCalculator::computeTransitionRate(int transitions,
                                                  int totalRuns) const {
  if (totalRuns < 2) {
    return 0.0;
  }

  return (double)(transitions) / (totalRuns - 1);
}

double FlakinessCalculator::predictRankingScore(
    const FlakinessScore& score) const {
  if (!rankingModelReady) {
    return score.coefficient;
  }

  return rankingModel.predictProbability(buildRankingFeatures(score));
}

string FlakinessCalculator::categorizeFlakiness(int passes, int failures,
                                                double coefficient) {
  int total = passes + failures;
  if (total == 0) {
    return "no_data";
  }

  if (failures == 0) {
    return "stable";
  }
  if (passes == 0) {
    return "always_failing";
  }

  if (coefficient < flakyThreshold) {
    return "mildly_flaky";
  }
  return "highly_flaky";
}
