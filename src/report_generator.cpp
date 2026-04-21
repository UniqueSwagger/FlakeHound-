
#include "report_generator.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

ReportGenerator::ReportGenerator() {}

ReportGenerator::~ReportGenerator() {}

void ReportGenerator::generateReport(string filename,
                                     vector<FlakinessScore>& scores) {
  string report = generateTextReport(scores);
  saveTextReport(filename, report);
}

void ReportGenerator::saveTextReport(const string& filename,
                                     const string& reportText) {
  std::filesystem::path outPath(filename);
  if (!outPath.parent_path().empty()) {
    std::filesystem::create_directories(outPath.parent_path());
  }

  ofstream file(filename);
  if (file.is_open()) {
    file << reportText;
    file.close();
    cout << "Report saved to: " << filename << "\n";
  } else {
    cout << "Error: Could not create report file\n";
  }
}

string ReportGenerator::generateTextReport(vector<FlakinessScore>& scores) {
  stringstream report;

  report << "FlakeHound++ Analysis Report\n";

  report << "Test Flakiness Summary\n";

  for (int i = 0; i < scores.size(); i++) {
    FlakinessScore& score = scores[i];

    report << (i + 1) << ". " << score.testName << "\n";
    report << "   Status: " << score.category << "\n";
    report << "   Ranking Score: ";
    report << fixed << setprecision(4) << score.rankingScore << "\n";
    report << "   Passes: " << score.passes << "/" << score.totalRuns << "\n";
    report << "   Failures: " << score.failures << "\n";
    report << "   Status Transitions: " << score.transitions << "\n";
    report << "   Transition Rate: ";
    report << fixed << setprecision(4) << score.transitionRate << "\n";
    report << "   Flakiness Coefficient: ";
    report << fixed << setprecision(2) << score.coefficient << "\n";
    report << "   Wilson Score (95% confidence): ";
    report << fixed << setprecision(4) << score.wilsonScore << "\n";
    report << "   Static Pre-run Risk: ";
    report << fixed << setprecision(4) << score.staticRiskScore << "\n";
    if (!score.likelyCauses.empty()) {
      report << "   Likely Causes:\n";
      for (const string& cause : score.likelyCauses) {
        report << "      - " << cause << "\n";
      }
    }
    report << "\n";
  }

  report << "Summary\n";

  int stableCount = 0;
  int alwaysFailingCount = 0;
  int mildlyFlakyCount = 0;
  int highlyFlakyCount = 0;

  for (auto& score : scores) {
    if (score.category == "stable")
      stableCount++;
    else if (score.category == "always_failing")
      alwaysFailingCount++;
    else if (score.category == "mildly_flaky")
      mildlyFlakyCount++;
    else if (score.category == "highly_flaky")
      highlyFlakyCount++;
  }

  report << "Stable Tests (no failures): " << stableCount << "\n";
  report << "Always Failing Tests: " << alwaysFailingCount << "\n";
  report << "Mildly Flaky Tests: " << mildlyFlakyCount << "\n";
  report << "Highly Flaky Tests: " << highlyFlakyCount << "\n";
  report << "Total Tests: " << scores.size() << "\n";

  return report.str();
}
