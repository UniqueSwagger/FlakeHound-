#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "flakliness_calculator.h"

using namespace std;

int main() {
  map<string, vector<bool>> testResults = {
      {"stable_case", {true, true, true, true}},
      {"toggle_case", {true, false, true, false}}};

  map<string, double> staticRiskScores = {{"stable_case", 0.10},
                                          {"toggle_case", 0.85}};
  map<string, vector<string>> likelyCauses = {
      {"toggle_case", {"Randomness detected without any stability guarantees."}}};

  FlakinessCalculator calculator;
  vector<FlakinessScore> scores = calculator.calculateForAllTests(
      testResults, staticRiskScores, likelyCauses);

  if (scores.size() != 2) {
    cerr << "Expected 2 flakiness scores\n";
    return 1;
  }

  const FlakinessScore& top = scores.front();
  if (top.testName != "toggle_case") {
    cerr << "Expected toggle_case to rank first\n";
    return 1;
  }

  if (top.transitions != 3) {
    cerr << "Expected 3 status transitions, got " << top.transitions << "\n";
    return 1;
  }

  if (top.transitionRate <= 0.9) {
    cerr << "Expected a high transition rate\n";
    return 1;
  }

  if (top.rankingScore <= scores.back().rankingScore) {
    cerr << "Expected toggle_case to have the highest ranking score\n";
    return 1;
  }

  if (top.likelyCauses.empty()) {
    cerr << "Expected static likely causes to be preserved\n";
    return 1;
  }

  cout << "Flakiness calculator smoke test passed.\n";
  return 0;
}
