#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "report_generator.h"
#include "static_analyzer.h"

using namespace std;
using namespace std::filesystem;

string defaultReportPath(const path& sourceFile) {
  path outputDir = "reports";
  string stem = sourceFile.stem().string();
  return (outputDir / (stem + "_static_analysis.txt")).string();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: analyze_static <source-file> [report-file]\n";
    return 1;
  }

  path sourceFile = argv[1];
  if (!exists(sourceFile)) {
    cerr << "Error: source file not found: " << sourceFile.string() << "\n";
    return 1;
  }

  StaticAnalyzer analyzer;
  StaticAnalysisReport report = analyzer.analyzeFile(sourceFile.string());
  string textReport = analyzer.generateTextReport(report);

  cout << textReport;

  ReportGenerator generator;
  string outputPath =
      argc >= 3 ? argv[2] : defaultReportPath(absolute(sourceFile));
  generator.saveTextReport(outputPath, textReport);

  return 0;
}
