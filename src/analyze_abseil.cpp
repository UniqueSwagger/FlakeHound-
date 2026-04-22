#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "flakliness_calculator.h"
#include "report_generator.h"
#include "xml_parser.h"

using namespace std;
using namespace std::filesystem;

string quotePath(const path& path) { return "\"" + path.string() + "\""; }

string nullDevice() {
#ifdef _WIN32
  return "nul";
#else
  return "/dev/null";
#endif
}

path findAbseilDirectory(int argc, char* argv[]) {
  vector<path> candidates;

  if (argc >= 2) {
    candidates.push_back(argv[1]);
  }

  candidates.push_back("abseil-cpp");
  candidates.push_back(path("..") / "abseil-cpp");
  candidates.push_back(path("..") / ".." / "abseil-cpp");
  candidates.push_back(path(__FILE__).parent_path().parent_path() /
                       "abseil-cpp");

  for (const path& candidate : candidates) {
    path absolutePath = absolute(candidate);
    if (is_directory(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

int parseRuns(int argc, char* argv[]) {
  if (argc >= 3) {
    int requestedRuns = atoi(argv[2]);
    if (requestedRuns > 0) {
      return requestedRuns;
    }
  }

  return 20;
}

path reportPath(int argc, char* argv[]) {
  if (argc >= 4) {
    return absolute(argv[3]);
  }

  return current_path() / "reports" / "abseil_flakiness_report.txt";
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
                     nullDevice() + " 2>&1";
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

int main(int argc, char* argv[]) {
  cout << "FlakeHound++ - Abseil Test Analysis\n\n";

  path abseilDir = findAbseilDirectory(argc, argv);
  if (abseilDir.empty()) {
    cerr << "Error: could not find the abseil-cpp directory.\n";
    cerr << "Pass the path as the first argument if needed.\n";
    return 1;
  }

  int runs = parseRuns(argc, argv);

  path asciiExe = abseilDir / "absl_ascii_test.exe";
  path bernoulliExe = abseilDir / "absl_bernoulli_test.exe";

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
  reporter.generateReport(reportPath(argc, argv).string(), scores);

  cout << "\nAbseil Analysis Complete!\n";

  return 0;
}
