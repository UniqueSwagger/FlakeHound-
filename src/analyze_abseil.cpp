#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "flakliness_calculator.h"
#include "report_generator.h"
#include "test_runner.h"
#include "xml_parser.h"

using namespace std;

void runTestsAndGenerateXML(string testExe, string xmlPrefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    string xmlFile = xmlPrefix + to_string(i) + ".xml";
    string command = testExe + " --gtest_output=xml:" + xmlFile + " > nul 2>&1";
    cout << "  Test run #" << i << "/" << runs << "...\r" << flush;
    system(command.c_str());
  }
  cout << "  Completed " << runs << " runs!\n";
}

int main() {
  cout << "FlakeHound++ - Abseil Test Analysis\n\n";

  string xml_dir = "C:\\Users\\srahm\\Documents\\FlakeHound++\\abseil-cpp\\";

  cout << "Step 1: Running tests and generating XMLs...\n";

  cout << "[ASCII Tests - 20 runs]\n";
  runTestsAndGenerateXML(xml_dir + "absl_ascii_test.exe", xml_dir + "run_", 20);

  cout << "\n[Bernoulli Tests - 20 runs]\n";
  runTestsAndGenerateXML(xml_dir + "absl_bernoulli_test.exe",
                         xml_dir + "bernoulli_run_", 20);

  cout << "\nStep 2: Parsing and analyzing XMLs...\n";

  map<string, vector<bool>> testResults;

  cout << "\n[ASCII Tests]\n";
  for (int i = 1; i <= 20; i++) {
    string xmlFile = xml_dir + "run_" + to_string(i) + ".xml";
    XMLParser parser;

    if (parser.parseGoogleTestXML(xmlFile)) {
      cout << "  Parsed run #" << i << "\n";

      vector<TestSuite> suites = parser.getTestSuites();
      for (auto& suite : suites) {
        for (auto& test : suite.testResults) {
          string fullTestName = suite.name + "." + test.name;
          bool passed = (test.status == "passed");
          testResults[fullTestName].push_back(passed);
        }
      }
    } else {
      cout << "  Warning: Could not parse ASCII run #" << i << "\n";
    }
  }

  cout << "\n[Bernoulli Tests]\n";
  for (int i = 1; i <= 20; i++) {
    string xmlFile = xml_dir + "bernoulli_run_" + to_string(i) + ".xml";
    XMLParser parser;

    if (parser.parseGoogleTestXML(xmlFile)) {
      cout << "  Parsed run #" << i << "\n";

      vector<TestSuite> suites = parser.getTestSuites();
      for (auto& suite : suites) {
        for (auto& test : suite.testResults) {
          string fullTestName = suite.name + "." + test.name;
          bool passed = (test.status == "passed");
          testResults[fullTestName].push_back(passed);
        }
      }
    } else {
      cout << "  Warning: Could not parse Bernoulli run #" << i << "\n";
    }
  }

  cout << "\nAnalyzing " << testResults.size() << " unique tests...\n\n";

  FlakinessCalculator calculator;
  vector<FlakinessScore> scores;

  for (auto& entry : testResults) {
    string testName = entry.first;
    vector<bool> results = entry.second;
    FlakinessScore score = calculator.calculateFlakiness(testName, results);
    scores.push_back(score);
  }

  sort(scores.begin(), scores.end(), [](FlakinessScore& a, FlakinessScore& b) {
    return a.coefficient > b.coefficient;
  });

  cout << "Flakiness Summary:\n";
  for (auto& score : scores) {
    cout << "  " << score.testName << ": " << score.passes << "/"
         << score.totalRuns << " (" << score.category << ")\n";
  }

  ReportGenerator reporter;
  reporter.generateReport("..\\reports\\abseil_flakiness_report.txt", scores);

  cout << "\nAbseil Analysis Complete!\n";

  return 0;
}
