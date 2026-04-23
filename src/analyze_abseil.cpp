#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "flakliness_calculator.h"
#include "report_generator.h"
#include "xml_parser.h"

using namespace std;
using namespace std::filesystem;

string quotePath(const path& filePath) {
  return "\"" + filePath.string() + "\"";
}

path findAbseilDirectory() {
  vector<path> candidates;
  candidates.push_back("abseil-cpp");
  candidates.push_back(path("..") / "abseil-cpp");
  candidates.push_back(path("..") / ".." / "abseil-cpp");

  for (const path& candidate : candidates) {
    path absolutePath = absolute(candidate);
    if (is_directory(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

bool parseIntegerText(const string& text, int& value) {
  if (text.empty()) {
    return false;
  }

  stringstream input(text);
  int parsed = 0;
  char extra = '\0';
  input >> parsed;
  if (input.fail() || (input >> extra)) {
    return false;
  }

  value = parsed;
  return true;
}

int promptIntValue(const string& label, int defaultValue, int minValue,
                   int maxValue) {
  while (true) {
    cout << label << " [" << defaultValue << "]: ";

    string input;
    getline(cin, input);
    if (input.empty()) {
      return defaultValue;
    }

    int value = 0;
    if (parseIntegerText(input, value) && value >= minValue &&
        value <= maxValue) {
      return value;
    }

    cout << "Enter a value between " << minValue << " and " << maxValue
         << ".\n";
  }
}

bool hasXmlSet(const path& dir, const string& prefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    if (!exists(dir / (prefix + to_string(i) + ".xml"))) {
      return false;
    }
  }
  return true;
}

void runTestsAndGenerateXML(const path& testExe, const path& outputDir,
                            const string& prefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    path xmlFile = outputDir / (prefix + to_string(i) + ".xml");
    string command = quotePath(testExe) +
                     " --gtest_output=xml:" + quotePath(xmlFile) + " > " +
                     "nul 2>&1";
    cout << "  Test run #" << i << "/" << runs << "...\r" << flush;
    system(command.c_str());
  }
  cout << "  Completed " << runs << " runs!\n";
}

void loadXmlResults(const path& dir, const string& prefix, int runs,
                    const string& label,
                    map<string, vector<bool>>& testResults) {
  cout << "\n[" << label << "]\n";

  for (int i = 1; i <= runs; i++) {
    path xmlFile = dir / (prefix + to_string(i) + ".xml");
    XMLParser parser;

    if (parser.parseGoogleTestXML(xmlFile.string())) {
      cout << "  Parsed run #" << i << "\n";

      vector<TestSuite> suites = parser.getTestSuites();
      for (const TestSuite& suite : suites) {
        for (const TestResult& test : suite.testResults) {
          string fullTestName = suite.name + "." + test.name;
          testResults[fullTestName].push_back(test.status == "passed");
        }
      }
    } else {
      cout << "  Warning: Could not parse " << label << " run #" << i << "\n";
    }
  }
}

int main() {
  cout << "FlakeHound++ - Abseil Test Analysis\n\n";

  path abseilDir = findAbseilDirectory();
  if (abseilDir.empty()) {
    cerr << "Error: could not find the abseil-cpp directory.\n";
    return 1;
  }

  int runs = promptIntValue("How many Abseil runs?", 20, 2, 100);

  path asciiExe = abseilDir / "absl_ascii_test.exe";
  path bernoulliExe = abseilDir / "absl_bernoulli_test.exe";
  path outputReport = current_path() / "reports" / "abseil_flakiness_report.txt";

  bool haveAsciiXml = hasXmlSet(abseilDir, "run_", runs);
  bool haveBernoulliXml = hasXmlSet(abseilDir, "bernoulli_run_", runs);

  cout << "Step 1: Preparing XML inputs...\n";
  if (haveAsciiXml && haveBernoulliXml) {
    cout << "Using existing XML files in " << abseilDir.string() << "\n";
  } else if (exists(asciiExe) && exists(bernoulliExe)) {
    cout << "[ASCII Tests - " << runs << " runs]\n";
    runTestsAndGenerateXML(asciiExe, abseilDir, "run_", runs);

    cout << "\n[Bernoulli Tests - " << runs << " runs]\n";
    runTestsAndGenerateXML(bernoulliExe, abseilDir, "bernoulli_run_", runs);
  } else {
    cerr << "Error: missing both reusable XML files and runnable Abseil test "
            "executables.\n";
    return 1;
  }

  cout << "\nStep 2: Parsing and analyzing XMLs...\n";

  map<string, vector<bool>> testResults;
  loadXmlResults(abseilDir, "run_", runs, "ASCII Tests", testResults);
  loadXmlResults(abseilDir, "bernoulli_run_", runs, "Bernoulli Tests",
                 testResults);

  cout << "\nAnalyzing " << testResults.size() << " unique tests...\n\n";

  FlakinessCalculator calculator;
  vector<FlakinessScore> scores = calculator.calculateForAllTests(testResults);

  cout << "Flakiness Summary:\n";
  for (const FlakinessScore& score : scores) {
    cout << "  " << score.testName << ": " << score.passes << "/"
         << score.totalRuns << " (" << score.category << ")\n";
  }

  ReportGenerator reporter;
  reporter.generateReport(outputReport.string(), scores);

  cout << "\nAbseil Analysis Complete!\n";
  cout << "Report: " << outputReport.string() << "\n";

  return 0;
}
