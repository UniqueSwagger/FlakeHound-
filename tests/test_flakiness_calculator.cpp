#include <gtest/gtest.h>

#include "flakliness_calculator.h"

TEST(FlakinessCalculatorTest, AllPassing) {
  FlakinessCalculator calc;
  string testName = "test1";
  vector<bool> results = {true, true, true, true, true};
  FlakinessScore score = calc.calculateFlakiness(testName, results);
  EXPECT_EQ(score.category, "stable");
  EXPECT_EQ(score.passes, 5);
  EXPECT_EQ(score.failures, 0);
}

TEST(FlakinessCalculatorTest, AllFailing) {
  FlakinessCalculator calc;
  string testName = "test2";
  vector<bool> results = {false, false, false, false, false};
  FlakinessScore score = calc.calculateFlakiness(testName, results);
  EXPECT_EQ(score.category, "always_failing");
}
