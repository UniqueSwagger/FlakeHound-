#ifndef FLAKINESS_CALCULATOR_H
#define FLAKINESS_CALCULATOR_H

#include <map>
#include <string>
#include <vector>

#include "logistic_regression.h"

using namespace std;

struct FlakinessScore {
  string testName;
  double coefficient;  // 0.0 to 1.0
  int totalRuns;
  int failures;
  int passes;
  int transitions;
  double transitionRate;
  double wilsonScore;
  double staticRiskScore;
  double rankingScore;
  string category;  // "stable", "mildly_flaky", "highly_flaky"
  vector<string> likelyCauses;
};

class FlakinessCalculator {
 public:
  FlakinessCalculator();
  ~FlakinessCalculator();

  FlakinessScore calculateFlakiness(const string& testName,
                                    const vector<bool>& testResults,
                                    double staticRiskScore = 0.0,
                                    const vector<string>& likelyCauses = {});
  vector<FlakinessScore> calculateForAllTests(
      map<string, vector<bool>>& allTestResults,
      const map<string, double>& staticRiskScores = {},
      const map<string, vector<string>>& likelyCauses = {});

 private:
  LogisticRegressionSGD rankingModel;
  bool rankingModelReady;

  void initializeRankingModel();
  double computeWilsonScore(int passes, int totalRuns, double z = 1.96);
  int countTransitions(const vector<bool>& testResults) const;
  double computeTransitionRate(int transitions, int totalRuns) const;
  double predictRankingScore(const FlakinessScore& score) const;
  string categorizeFlakiness(int passes, int failures, double coefficient);
};
#endif  // !FLAKINESS_CALCULATOR_H
