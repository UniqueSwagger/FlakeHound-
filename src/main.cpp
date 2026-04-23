#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "flakliness_calculator.h"
#include "report_generator.h"
#include "static_analyzer.h"
#include "test_runner.h"
#include "xml_parser.h"

using namespace std;
using namespace std::filesystem;

struct DemoRuntimeConfig {
  int randomPassThreshold = 5;
  int timingPassThreshold = 5;
};

struct CliOptions {
  int runs = 8;
  string reportPath = (path("reports") / "flakiness_report.txt").string();
  DemoRuntimeConfig demoConfig;
};

struct StaticContext {
  bool available = false;
  path sourcePath;
  StaticAnalysisReport fileReport;
  map<string, double> riskByTest;
  map<string, vector<string>> causesByTest;
};

const double fixedFlakyThreshold = 0.4;

string quotePath(const path& filePath) {
  return "\"" + filePath.string() + "\"";
}

path findFirstExisting(const vector<path>& candidates,
                       bool expectDirectory = false) {
  for (const path& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }

    path absolutePath = absolute(candidate);
    bool existsNow =
        expectDirectory ? is_directory(absolutePath) : exists(absolutePath);
    if (existsNow) {
      return absolutePath;
    }
  }

  return {};
}

path findDemoExecutable() {
  vector<path> candidates;
  candidates.push_back("demo_tests.exe");
  candidates.push_back("demo_tests");
  candidates.push_back(path("demo") / "demo_tests.exe");
  candidates.push_back(path("demo") / "demo_tests");
  return findFirstExisting(candidates);
}

path findDemoSource() {
  vector<path> candidates;
  candidates.push_back(path("demo") / "demo_tests.cpp");
  return findFirstExisting(candidates);
}

path findAbseilDirectory() {
  vector<path> candidates;
  candidates.push_back("abseil-cpp");
  candidates.push_back(path("..") / "abseil-cpp");
  candidates.push_back(path("..") / ".." / "abseil-cpp");
  return findFirstExisting(candidates, true);
}

string readFile(const path& filePath) {
  ifstream input(filePath);
  stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool isIdentifierChar(char c) {
  return isalnum((unsigned char)(c)) || c == '_';
}

string extractFunctionSource(const string& source, const string& functionName) {
  string signature = functionName + "(";
  int namePos = source.find(signature);

  while (namePos != string::npos) {
    if (namePos > 0 && isIdentifierChar(source[namePos - 1])) {
      namePos = source.find(signature, namePos + signature.length());
      continue;
    }

    int bracePos = source.find('{', namePos);
    if (bracePos == string::npos) {
      return "";
    }

    int depth = 0;
    for (int i = bracePos; i < source.length(); i++) {
      if (source[i] == '{') {
        depth++;
      } else if (source[i] == '}') {
        depth--;
        if (depth == 0) {
          int startPos = source.rfind('\n', namePos);
          if (startPos == string::npos) {
            startPos = 0;
          } else {
            startPos++;
          }
          return source.substr(startPos, i - startPos + 1);
        }
      }
    }

    return "";
  }

  return "";
}

string extractTestName(const string& line, const string& prefix) {
  if (line.rfind(prefix, 0) != 0) {
    return "";
  }

  string name = line.substr(prefix.length());
  int detailPos = name.find(" (");
  if (detailPos != string::npos) {
    name = name.substr(0, detailPos);
  }

  return name;
}

map<string, vector<bool>> collectTestResults(const vector<RunResult>& results) {
  map<string, vector<bool>> testResults;

  for (const RunResult& result : results) {
    istringstream output(result.output);
    string line;

    while (getline(output, line)) {
      string testName = extractTestName(line, "PASS: ");
      bool passed = true;

      if (testName.empty()) {
        testName = extractTestName(line, "FAIL: ");
        passed = false;
      }

      if (!testName.empty()) {
        testResults[testName].push_back(passed);
      }
    }
  }

  return testResults;
}

vector<string> collectCauseMessages(const StaticAnalysisReport& report) {
  vector<string> causes;
  set<string> seen;

  for (const StaticFinding& finding : report.findings) {
    if (finding.category != "flakiness_cause") {
      continue;
    }

    if (seen.insert(finding.message).second) {
      causes.push_back(finding.message);
    }
  }

  return causes;
}

StaticContext analyzeStaticContext(
    const path& sourcePath, const map<string, vector<bool>>& testResults) {
  StaticContext context;
  if (sourcePath.empty() || !exists(sourcePath)) {
    return context;
  }

  StaticAnalyzer analyzer;
  string sourceCode = readFile(sourcePath);

  context.available = true;
  context.sourcePath = sourcePath;
  context.fileReport =
      analyzer.analyzeSourceCode(sourcePath.string(), sourceCode);

  for (const auto& entry : testResults) {
    const string& testName = entry.first;
    string functionSource = extractFunctionSource(sourceCode, testName);

    if (functionSource.empty()) {
      context.riskByTest[testName] = context.fileReport.mlRiskScore;
      continue;
    }

    StaticAnalysisReport testReport =
        analyzer.analyzeSourceCode(testName, functionSource);
    context.riskByTest[testName] = testReport.mlRiskScore;
    context.causesByTest[testName] = collectCauseMessages(testReport);
  }

  return context;
}

bool compareByRanking(const FlakinessScore& a, const FlakinessScore& b) {
  return a.rankingScore > b.rankingScore;
}

void sortScores(vector<FlakinessScore>& scores) {
  sort(scores.begin(), scores.end(), compareByRanking);
}

bool saveDemoRuntimeConfig(const DemoRuntimeConfig& config) {
  path configPath = path("demo") / "demo_runtime_config.txt";
  create_directories(configPath.parent_path());

  ofstream output(configPath);
  if (!output.is_open()) {
    return false;
  }

  output << "random_threshold=" << config.randomPassThreshold << "\n";
  output << "timing_threshold=" << config.timingPassThreshold << "\n";
  return true;
}

string buildCombinedReport(const vector<FlakinessScore>& scores,
                           const CliOptions& options,
                           const StaticContext& staticContext) {
  ReportGenerator generator;
  vector<FlakinessScore> reportScores = scores;
  string report = generator.generateTextReport(reportScores);

  report += "\nAnalysis Settings\n";
  report += "Runs Requested: " + to_string(options.runs) + "\n";
  report += "Flakiness Threshold: 0.40\n";
  report += "\nRandom Demo Pass Cutoff: " +
            to_string(options.demoConfig.randomPassThreshold) + "\n";
  report += "Timing Demo Pass Cutoff: " +
            to_string(options.demoConfig.timingPassThreshold) + "\n";
  report += "Sort Order: ranking\n";

  report += "\nStatic Analysis Summary\n";
  if (!staticContext.available) {
    report += "No source file was found for static analysis.\n";
    return report;
  }

  report += "Source File: " + staticContext.sourcePath.string() + "\n";
  report += "Predicted File Risk: ";

  stringstream line;
  line << fixed << setprecision(4) << staticContext.fileReport.mlRiskScore
       << " (" << staticContext.fileReport.riskCategory << ")";
  report += line.str();
  report += "\nStatic Findings: " +
            to_string((int)(staticContext.fileReport.findings.size())) + "\n";

  vector<string> fileCauses = collectCauseMessages(staticContext.fileReport);
  if (!fileCauses.empty()) {
    report += "Observed Static Risk Factors:\n";
    for (const string& cause : fileCauses) {
      report += " - " + cause + "\n";
    }
  }

  report += "\nRoot Cause Correlation\n";
  bool foundMatch = false;

  for (const FlakinessScore& score : scores) {
    if (score.likelyCauses.empty()) {
      continue;
    }

    foundMatch = true;
    report += score.testName + "\n";
    report += " - Dynamic Category: " + score.category + "\n";
    for (const string& cause : score.likelyCauses) {
      report += " - Static Cause: " + cause + "\n";
    }
  }

  if (!foundMatch) {
    report += "No per-test static root-cause correlation was found.\n";
  }

  return report;
}

bool hasXmlSet(const path& directory, const string& prefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    if (!exists(directory / (prefix + to_string(i) + ".xml"))) {
      return false;
    }
  }

  return true;
}

void runTestsAndGenerateXML(const path& testExecutable, const path& outputDir,
                            const string& prefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    path xmlFile = outputDir / (prefix + to_string(i) + ".xml");
    string command = quotePath(testExecutable) +
                     " --gtest_output=xml:" + quotePath(xmlFile) + " > " +
                     "nul 2>&1";
    cout << "  Test run #" << i << "/" << runs << "...\r" << flush;
    system(command.c_str());
  }

  cout << "  Completed " << runs << " runs!\n";
}

void loadXmlResults(const path& directory, const string& prefix, int runs,
                    const string& label,
                    map<string, vector<bool>>& testResults) {
  cout << "\n[" << label << "]\n";

  for (int i = 1; i <= runs; i++) {
    path xmlFile = directory / (prefix + to_string(i) + ".xml");
    XMLParser parser;

    if (!parser.parseGoogleTestXML(xmlFile.string())) {
      cout << "  Warning: Could not parse " << label << " run #" << i << "\n";
      continue;
    }

    cout << "  Parsed run #" << i << "\n";
    vector<TestSuite> suites = parser.getTestSuites();

    for (const TestSuite& suite : suites) {
      for (const TestResult& test : suite.testResults) {
        string fullTestName = suite.name + "." + test.name;
        testResults[fullTestName].push_back(test.status == "passed");
      }
    }
  }
}

string defaultStaticReportPath(const path& sourceFile) {
  return (path("reports") /
          (sourceFile.stem().string() + "_static_analysis.txt"))
      .string();
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

CliOptions promptDemoOptions() {
  CliOptions options;
  cout << "\nDemo Analysis Settings\n";
  cout << "Press Enter to keep the default value.\n";
  options.runs =
      promptIntValue("How many times should the demo test suite run?", 8, 2, 100);
  options.demoConfig.randomPassThreshold = promptIntValue(
      "From which random value should test_random_number pass? (0-9)", 5, 0,
      9);
  options.demoConfig.timingPassThreshold = promptIntValue(
      "From which time-mod value should test_timing_based pass? (0-9)", 5, 0,
      9);
  return options;
}

int promptAbseilRuns(int defaultRuns = 20) {
  cout << "\nAbseil Analysis Settings\n";
  cout << "Press Enter to keep the default value.\n";
  return promptIntValue("How many runs for Abseil XML analysis?", defaultRuns,
                        2, 100);
}

int runDynamicDemo(const CliOptions& options) {
  cout << "FlakeHound++ - Test Runner Demo\n";

  path executablePath = findDemoExecutable();
  if (executablePath.empty()) {
    cerr << "Error: could not find demo_tests.exe or demo_tests.\n";
    cerr << "Compile the demo test executable first.\n";
    return 1;
  }

  if (!saveDemoRuntimeConfig(options.demoConfig)) {
    cerr << "Error: could not save demo runtime settings.\n";
    return 1;
  }

  cout << "Demo runtime config saved to demo/demo_runtime_config.txt\n";

  TestRunner runner(quotePath(executablePath));
  vector<RunResult> results = runner.runMultipleTimes(options.runs);
  map<string, int> stats = runner.getStats();

  cout << "Overall Results\n";
  cout << "Total Runs: " << stats["runs"] << "\n";
  cout << "Total Tests Passed: " << stats["total_passed"] << "\n";
  cout << "Total Tests Failed: " << stats["total_failed"] << "\n";
  cout << "Total Tests: " << stats["total_tests"] << "\n\n";

  if (stats["total_tests"] == 0) {
    cerr << "Error: the test runner did not capture any PASS/FAIL output.\n";
    return 1;
  }

  map<string, vector<bool>> testResults = collectTestResults(results);
  if (testResults.empty()) {
    cerr << "Error: no per-test results were extracted from the test output.\n";
    return 1;
  }

  path sourcePath = findDemoSource();
  StaticContext staticContext = analyzeStaticContext(sourcePath, testResults);

  cout << "Static analysis source: ";
  if (staticContext.available) {
    cout << staticContext.sourcePath.string() << "\n";
  } else {
    cout << "not found\n";
  }

  FlakinessCalculator calculator(fixedFlakyThreshold);
  vector<FlakinessScore> scores = calculator.calculateForAllTests(
      testResults, staticContext.riskByTest, staticContext.causesByTest);

  sortScores(scores);

  cout << "Flakiness Analysis\n\n";
  cout << "Threshold used: coefficient < " << fixed << setprecision(2)
       << fixedFlakyThreshold
       << " => mildly_flaky, otherwise highly_flaky\n";
  for (const FlakinessScore& score : scores) {
    cout << score.testName << ": PASSED " << score.passes << "/"
         << score.totalRuns << " times (Category: " << score.category
         << ", Rank: " << fixed << setprecision(3) << score.rankingScore
         << ")\n";
  }
  cout << "\n";

  ReportGenerator generator;
  string reportText = buildCombinedReport(scores, options, staticContext);
  generator.saveTextReport(options.reportPath, reportText);

  cout << "FlakeHound++ Analysis Complete!\n";
  cout << "Report: " << options.reportPath << "\n";
  return 0;
}

int runStaticAnalysis(const path& sourceFile, const string& reportPath) {
  cout << "FlakeHound++ - Static Analysis\n\n";

  if (sourceFile.empty() || !exists(sourceFile)) {
    cerr << "Error: source file not found.\n";
    return 1;
  }

  StaticAnalyzer analyzer;
  StaticAnalysisReport report = analyzer.analyzeFile(sourceFile.string());
  string textReport = analyzer.generateTextReport(report);

  cout << textReport;

  ReportGenerator generator;
  generator.saveTextReport(reportPath, textReport);

  cout << "Static analysis complete!\n";
  cout << "Report: " << reportPath << "\n";
  return 0;
}

int runAbseilAnalysis(int runs, const path& reportPath) {
  cout << "FlakeHound++ - Abseil Test Analysis\n\n";

  path abseilDir = findAbseilDirectory();
  if (abseilDir.empty()) {
    cerr << "Error: could not find the abseil-cpp directory.\n";
    return 1;
  }

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

  FlakinessCalculator calculator(fixedFlakyThreshold);
  vector<FlakinessScore> scores = calculator.calculateForAllTests(testResults);

  cout << "Flakiness Summary:\n";
  cout << "Threshold used: coefficient < " << fixed << setprecision(2)
       << fixedFlakyThreshold
       << " => mildly_flaky, otherwise highly_flaky\n";
  for (const FlakinessScore& score : scores) {
    cout << "  " << score.testName << ": " << score.passes << "/"
         << score.totalRuns << " (" << score.category << ")\n";
  }

  ReportGenerator reporter;
  reporter.generateReport(reportPath.string(), scores);

  cout << "\nAbseil Analysis Complete!\n";
  cout << "Report: " << reportPath.string() << "\n";
  return 0;
}

void printMenu() {
  cout << "FlakeHound++ Menu\n";
  cout << "1. Run demo analysis (dynamic + static)\n";
  cout << "2. Run demo static analysis only\n";
  cout << "3. Run Abseil dynamic XML analysis only\n";
  cout << "4. Run everything\n";
  cout << "0. Exit\n\n";
  cout << "Choose an option: ";
}

int runMenu() {
  while (true) {
    printMenu();

    string choice;
    getline(cin, choice);

    if (choice == "0") {
      cout << "Exiting FlakeHound++.\n";
      return 0;
    }

    if (choice == "1") {
      CliOptions options = promptDemoOptions();
      int result = runDynamicDemo(options);
      cout << "\n";
      if (result != 0) {
        return result;
      }
      continue;
    }

    if (choice == "2") {
      path demoSource = findDemoSource();
      int result =
          runStaticAnalysis(demoSource, defaultStaticReportPath(demoSource));
      cout << "\n";
      if (result != 0) {
        return result;
      }
      continue;
    }

    if (choice == "3") {
      int runs = promptAbseilRuns(20);
      int result =
          runAbseilAnalysis(runs, path("reports") / "abseil_flakiness_report.txt");
      cout << "\n";
      if (result != 0) {
        return result;
      }
      continue;
    }

    if (choice == "4") {
      CliOptions options = promptDemoOptions();
      int abseilRuns = promptAbseilRuns(20);

      int result = runDynamicDemo(options);
      if (result != 0) {
        return result;
      }

      path demoSource = findDemoSource();
      result =
          runStaticAnalysis(demoSource, defaultStaticReportPath(demoSource));
      if (result != 0) {
        return result;
      }

      result = runAbseilAnalysis(
          abseilRuns, path("reports") / "abseil_flakiness_report.txt");
      if (result != 0) {
        return result;
      }

      cout << "\nAll analyses completed.\n\n";
      continue;
    }

    cout << "Invalid option. Try again.\n\n";
  }
}

int main() {
  return runMenu();
}
