#ifndef FLAKINESS_CALCULATOR_H
#define FLAKINESS_CALCULATOR_H

#include <map>
#include <string>
#include <vector>
using namespace std;
struct FlakinessScore {
  string testName;
  double coefficient;  // 0.0 to 1.0
  int totalRuns;
  int failures;
  int passes;
  double wilsonScore;
  string category;  // "stable", "mildly_flaky", "highly_flaky"
};

class FlakinessCalculator {
 public:
  FlakinessCalculator();
  ~FlakinessCalculator();

  FlakinessScore calculateFlakiness(string& testName,
                                    vector<bool>& testResults);
  vector<FlakinessScore> calculateForAllTests(
      map<string, vector<bool>>& allTestResults);

 private:
  double computeWilsonScore(int passes, int totalRuns, double z = 1.96);
  string categorizeFlakiness(double wilsonScore);
};
#endif  // !FLAKINESS_CALCULATOR_H