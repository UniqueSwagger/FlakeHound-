#include <filesystem>
#include <iostream>
#include <string>

#include "report_generator.h"
#include "static_analyzer.h"

using namespace std;
using namespace std::filesystem;

string defaultReportPath(const path& sourceFile) {
  path outputDir = "reports";
  string stem = sourceFile.stem().string();
  return (outputDir / (stem + "_static_analysis.txt")).string();
}

int main() {
  cout << "FlakeHound++ - Static Analysis\n\n";

  path sourceFile = path("demo") / "demo_tests.cpp";
  if (!exists(sourceFile)) {
    cerr << "Error: source file not found: " << sourceFile.string() << "\n";
    return 1;
  }

  StaticAnalyzer analyzer;
  StaticAnalysisReport report = analyzer.analyzeFile(sourceFile.string());
  string textReport = analyzer.generateTextReport(report);

  cout << textReport;

  ReportGenerator generator;
  string outputPath = defaultReportPath(absolute(sourceFile));
  generator.saveTextReport(outputPath, textReport);

  return 0;
}
