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

struct CliOptions {
  filesystem::path requestedExecutable;
  filesystem::path requestedSource;
  int runs = 10;
  string reportPath =
      (filesystem::path("reports") / "flakiness_report.txt").string();
  string nameFilter;
  int minRuns = 1;
  string sortBy = "ranking";
};

struct StaticContext {
  bool available = false;
  filesystem::path sourcePath;
  StaticAnalysisReport fileReport;
  map<string, double> riskByTest;
  map<string, vector<string>> causesByTest;
};

string quotePath(const filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

string nullDevice() {
#ifdef _WIN32
  return "nul";
#else
  return "/dev/null";
#endif
}

bool startsWith(const string& value, const string& prefix) {
  return value.rfind(prefix, 0) == 0;
}

string toLowerCopy(string value) {
  transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return (char)(tolower(c)); });
  return value;
}

CliOptions parseArguments(int argc, char* argv[]) {
  CliOptions options;
  vector<string> positional;

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];

    if (startsWith(arg, "--filter=")) {
      options.nameFilter = arg.substr(string("--filter=").length());
    } else if (startsWith(arg, "--min-runs=")) {
      int parsed = atoi(arg.substr(string("--min-runs=").length()).c_str());
      if (parsed > 0) {
        options.minRuns = parsed;
      }
    } else if (startsWith(arg, "--sort=")) {
      options.sortBy = toLowerCopy(arg.substr(string("--sort=").length()));
    } else if (startsWith(arg, "--source=")) {
      options.requestedSource = arg.substr(string("--source=").length());
    } else {
      positional.push_back(arg);
    }
  }

  if (!positional.empty()) {
    options.requestedExecutable = positional[0];
  }
  if (positional.size() >= 2) {
    int parsedRuns = atoi(positional[1].c_str());
    if (parsedRuns > 0) {
      options.runs = parsedRuns;
    }
  }
  if (positional.size() >= 3) {
    options.reportPath = positional[2];
  }

  return options;
}

filesystem::path findExecutablePath(const CliOptions& options) {
  vector<filesystem::path> candidates;

  if (!options.requestedExecutable.empty()) {
    candidates.push_back(options.requestedExecutable);
  }

  candidates.push_back("demo_tests");
  candidates.push_back("demo_tests.exe");
  candidates.push_back(filesystem::path("demo") / "demo_tests");
  candidates.push_back(filesystem::path("demo") / "demo_tests.exe");
  candidates.push_back(filesystem::path("build") / "bin" / "demo_tests");
  candidates.push_back(filesystem::path("build") / "bin" / "demo_tests.exe");

  for (const filesystem::path& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }

    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::exists(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

filesystem::path findSourcePath(const CliOptions& options,
                                const filesystem::path& executablePath) {
  vector<filesystem::path> candidates;

  if (!options.requestedSource.empty()) {
    candidates.push_back(options.requestedSource);
  }

  const string stem = executablePath.stem().string();
  candidates.push_back(filesystem::path("demo") / (stem + ".cpp"));
  candidates.push_back(filesystem::path("src") / (stem + ".cpp"));
  candidates.push_back(executablePath.parent_path() / (stem + ".cpp"));
  candidates.push_back(filesystem::path(__FILE__).parent_path().parent_path() /
                       "demo" / (stem + ".cpp"));

  for (const filesystem::path& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }

    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::exists(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

filesystem::path findDemoSourcePath() {
  vector<filesystem::path> candidates;
  candidates.push_back(filesystem::path("demo") / "demo_tests.cpp");
  candidates.push_back(filesystem::path(__FILE__).parent_path().parent_path() /
                       "demo" / "demo_tests.cpp");

  for (const filesystem::path& candidate : candidates) {
    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::exists(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

filesystem::path findAbseilDirectory() {
  vector<filesystem::path> candidates;
  candidates.push_back("abseil-cpp");
  candidates.push_back(filesystem::path("..") / "abseil-cpp");
  candidates.push_back(filesystem::path("..") / ".." / "abseil-cpp");
  candidates.push_back(filesystem::path(__FILE__).parent_path().parent_path() /
                       "abseil-cpp");

  for (const filesystem::path& candidate : candidates) {
    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::is_directory(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

string readFile(const filesystem::path& filePath) {
  ifstream input(filePath);
  stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool isIdentifierChar(char c) {
  return isalnum((unsigned char)(c)) || c == '_';
}

string extractFunctionSource(const string& source, const string& functionName) {
  const string signatureNeedle = functionName + "(";
  size_t namePos = source.find(signatureNeedle);

  while (namePos != string::npos) {
    if (namePos > 0 && isIdentifierChar(source[namePos - 1])) {
      namePos = source.find(signatureNeedle, namePos + signatureNeedle.length());
      continue;
    }

    size_t bracePos = source.find('{', namePos);
    if (bracePos == string::npos) {
      return "";
    }

    int depth = 0;
    size_t endPos = bracePos;
    for (; endPos < source.size(); endPos++) {
      if (source[endPos] == '{') {
        depth++;
      } else if (source[endPos] == '}') {
        depth--;
        if (depth == 0) {
          size_t startPos = source.rfind('\n', namePos);
          if (startPos == string::npos) {
            startPos = 0;
          } else {
            startPos++;
          }
          return source.substr(startPos, endPos - startPos + 1);
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

  string name = line.substr(prefix.size());
  size_t detailPos = name.find(" (");
  if (detailPos != string::npos) {
    name = name.substr(0, detailPos);
  }

  return name;
}

map<string, vector<bool>> collectTestResults(const vector<RunResult>& results) {
  map<string, vector<bool>> testResults;

  for (const RunResult& result : results) {
    size_t pos = 0;
    while (pos < result.output.length()) {
      size_t newlinePos = result.output.find('\n', pos);
      if (newlinePos == string::npos) {
        newlinePos = result.output.length();
      }

      string line = result.output.substr(pos, newlinePos - pos);
      string testName = extractTestName(line, "PASS: ");
      bool passed = true;

      if (testName.empty()) {
        testName = extractTestName(line, "FAIL: ");
        passed = false;
      }

      if (!testName.empty()) {
        testResults[testName].push_back(passed);
      }

      pos = newlinePos + 1;
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

    string message = finding.message;
    if (seen.insert(message).second) {
      causes.push_back(message);
    }
  }

  return causes;
}

StaticContext analyzeStaticContext(const filesystem::path& sourcePath,
                                   const map<string, vector<bool>>& testResults) {
  StaticContext context;
  if (sourcePath.empty() || !filesystem::exists(sourcePath)) {
    return context;
  }

  StaticAnalyzer analyzer;
  string sourceCode = readFile(sourcePath);
  context.available = true;
  context.sourcePath = sourcePath;
  context.fileReport = analyzer.analyzeSourceCode(sourcePath.string(), sourceCode);

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

void applyFilters(vector<FlakinessScore>& scores, const CliOptions& options) {
  string filterLower = toLowerCopy(options.nameFilter);

  scores.erase(remove_if(scores.begin(), scores.end(),
                         [&](const FlakinessScore& score) {
                           if (score.totalRuns < options.minRuns) {
                             return true;
                           }

                           if (!filterLower.empty()) {
                             return toLowerCopy(score.testName).find(filterLower) ==
                                    string::npos;
                           }

                           return false;
                         }),
               scores.end());
}

void sortScores(vector<FlakinessScore>& scores, const string& sortBy) {
  if (sortBy == "name") {
    sort(scores.begin(), scores.end(),
         [](const FlakinessScore& a, const FlakinessScore& b) {
           return a.testName < b.testName;
         });
    return;
  }

  if (sortBy == "coefficient") {
    sort(scores.begin(), scores.end(),
         [](const FlakinessScore& a, const FlakinessScore& b) {
           return a.coefficient > b.coefficient;
         });
    return;
  }

  if (sortBy == "wilson") {
    sort(scores.begin(), scores.end(),
         [](const FlakinessScore& a, const FlakinessScore& b) {
           return a.wilsonScore < b.wilsonScore;
         });
    return;
  }

  if (sortBy == "transition" || sortBy == "transitions") {
    sort(scores.begin(), scores.end(),
         [](const FlakinessScore& a, const FlakinessScore& b) {
           return a.transitionRate > b.transitionRate;
         });
    return;
  }

  if (sortBy == "static" || sortBy == "risk") {
    sort(scores.begin(), scores.end(),
         [](const FlakinessScore& a, const FlakinessScore& b) {
           return a.staticRiskScore > b.staticRiskScore;
         });
    return;
  }

  sort(scores.begin(), scores.end(),
       [](const FlakinessScore& a, const FlakinessScore& b) {
         return a.rankingScore > b.rankingScore;
       });
}

string buildCombinedReport(const vector<FlakinessScore>& scores,
                           const CliOptions& options,
                           const StaticContext& staticContext,
                           ReportGenerator& reportGenerator) {
  vector<FlakinessScore> reportScores = scores;
  string report = reportGenerator.generateTextReport(reportScores);

  report += "\nAnalysis Settings\n";
  report += "Runs Requested: " + to_string(options.runs) + "\n";
  report += "Minimum Run Filter: " + to_string(options.minRuns) + "\n";
  report += "Name Filter: " +
            (options.nameFilter.empty() ? string("<none>") : options.nameFilter) +
            "\n";
  report += "Sort Order: " + options.sortBy + "\n";

  if (!staticContext.available) {
    report += "\nStatic Analysis Summary\n";
    report += "No source file was found for static analysis.\n";
    return report;
  }

  report += "\nStatic Analysis Summary\n";
  report += "Source File: " + staticContext.sourcePath.string() + "\n";
  report += "Predicted File Risk: ";
  {
    stringstream line;
    line << fixed << setprecision(4) << staticContext.fileReport.mlRiskScore
         << " (" << staticContext.fileReport.riskCategory << ")";
    report += line.str();
  }
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
  bool emittedCorrelation = false;
  for (const FlakinessScore& score : scores) {
    if (score.likelyCauses.empty()) {
      continue;
    }

    emittedCorrelation = true;
    report += score.testName + "\n";
    report += " - Dynamic Category: " + score.category + "\n";
    for (const string& cause : score.likelyCauses) {
      report += " - Static Cause: " + cause + "\n";
    }
  }

  if (!emittedCorrelation) {
    report += "No per-test static root-cause correlation was found.\n";
  }

  return report;
}

bool hasXmlSet(const filesystem::path& dir, const string& prefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    if (!filesystem::exists(dir / (prefix + to_string(i) + ".xml"))) {
      return false;
    }
  }
  return true;
}

void runTestsAndGenerateXML(const filesystem::path& testExe,
                            const filesystem::path& outputDir,
                            const string& prefix, int runs) {
  for (int i = 1; i <= runs; i++) {
    filesystem::path xmlFile = outputDir / (prefix + to_string(i) + ".xml");
    string command = quotePath(testExe) + " --gtest_output=xml:" +
                     quotePath(xmlFile) + " > " + nullDevice() + " 2>&1";
    cout << "  Test run #" << i << "/" << runs << "...\r" << flush;
    system(command.c_str());
  }
  cout << "  Completed " << runs << " runs!\n";
}

void loadXmlResults(const filesystem::path& dir, const string& prefix, int runs,
                    const string& label,
                    map<string, vector<bool>>& testResults) {
  cout << "\n[" << label << "]\n";

  for (int i = 1; i <= runs; i++) {
    filesystem::path xmlFile = dir / (prefix + to_string(i) + ".xml");
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

string defaultStaticReportPath(const filesystem::path& sourceFile) {
  filesystem::path outputDir = "reports";
  string stem = sourceFile.stem().string();
  return (outputDir / (stem + "_static_analysis.txt")).string();
}

int runDynamicDemo(const CliOptions& options) {
  cout << "FlakeHound++ - Test Runner Demo\n";

  filesystem::path executablePath = findExecutablePath(options);
  if (executablePath.empty()) {
    cerr << "Error: could not find a demo test executable.\n";
    cerr << "Pass the executable path as the first argument.\n";
    return 1;
  }

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

  filesystem::path sourcePath = findSourcePath(options, executablePath);
  StaticContext staticContext = analyzeStaticContext(sourcePath, testResults);

  if (staticContext.available) {
    cout << "Static analysis source: " << staticContext.sourcePath.string()
         << "\n";
  } else {
    cout << "Static analysis source: not found\n";
  }

  FlakinessCalculator calculator;
  vector<FlakinessScore> scores = calculator.calculateForAllTests(
      testResults, staticContext.riskByTest, staticContext.causesByTest);

  applyFilters(scores, options);
  sortScores(scores, options.sortBy);

  if (scores.empty()) {
    cerr << "Error: filters removed all tests from the report.\n";
    return 1;
  }

  cout << "Flakiness Analysis\n\n";
  for (const FlakinessScore& score : scores) {
    cout << score.testName << ": PASSED " << score.passes << "/"
         << score.totalRuns << " times (Category: " << score.category
         << ", Rank: " << fixed << setprecision(3) << score.rankingScore
         << ")\n";
  }
  cout << "\n";

  ReportGenerator reportGenerator;
  string reportText =
      buildCombinedReport(scores, options, staticContext, reportGenerator);
  reportGenerator.saveTextReport(options.reportPath, reportText);

  cout << "FlakeHound++ Analysis Complete!\n";
  cout << "Report: " << options.reportPath << "\n";

  return 0;
}

int runStaticAnalysis(const filesystem::path& sourceFile,
                      const string& reportPath) {
  cout << "FlakeHound++ - Static Analysis\n\n";

  if (sourceFile.empty() || !filesystem::exists(sourceFile)) {
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

int runAbseilAnalysis(int runs, const filesystem::path& reportPath) {
  cout << "FlakeHound++ - Abseil Test Analysis\n\n";

  filesystem::path abseilDir = findAbseilDirectory();
  if (abseilDir.empty()) {
    cerr << "Error: could not find the abseil-cpp directory.\n";
    return 1;
  }

  filesystem::path asciiExe = abseilDir / "absl_ascii_test.exe";
  filesystem::path bernoulliExe = abseilDir / "absl_bernoulli_test.exe";

  bool haveAsciiXml = hasXmlSet(abseilDir, "run_", runs);
  bool haveBernoulliXml = hasXmlSet(abseilDir, "bernoulli_run_", runs);

  cout << "Step 1: Preparing XML inputs...\n";
  if (haveAsciiXml && haveBernoulliXml) {
    cout << "Using existing XML files in " << abseilDir.string() << "\n";
  } else if (filesystem::exists(asciiExe) && filesystem::exists(bernoulliExe)) {
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
  cout << "0. Exit\n";
  cout << "\nChoose an option: ";
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
      CliOptions options;
      options.runs = 8;
      options.reportPath = (filesystem::path("reports") / "flakiness_report.txt").string();
      int result = runDynamicDemo(options);
      cout << "\n";
      if (result != 0) {
        return result;
      }
      continue;
    }

    if (choice == "2") {
      filesystem::path demoSource = findDemoSourcePath();
      string reportPath = defaultStaticReportPath(demoSource);
      int result = runStaticAnalysis(demoSource, reportPath);
      cout << "\n";
      if (result != 0) {
        return result;
      }
      continue;
    }

    if (choice == "3") {
      int result = runAbseilAnalysis(
          20, filesystem::path("reports") / "abseil_flakiness_report.txt");
      cout << "\n";
      if (result != 0) {
        return result;
      }
      continue;
    }

    if (choice == "4") {
      CliOptions options;
      options.runs = 8;
      options.reportPath = (filesystem::path("reports") / "flakiness_report.txt").string();

      int result = runDynamicDemo(options);
      if (result != 0) {
        return result;
      }

      filesystem::path demoSource = findDemoSourcePath();
      result = runStaticAnalysis(demoSource, defaultStaticReportPath(demoSource));
      if (result != 0) {
        return result;
      }

      result = runAbseilAnalysis(
          20, filesystem::path("reports") / "abseil_flakiness_report.txt");
      if (result != 0) {
        return result;
      }

      cout << "\nAll analyses completed.\n\n";
      continue;
    }

    cout << "Invalid option. Try again.\n\n";
  }
}

int main(int argc, char* argv[]) {
  if (argc == 1) {
    return runMenu();
  }

  CliOptions options = parseArguments(argc, argv);
  return runDynamicDemo(options);
}
