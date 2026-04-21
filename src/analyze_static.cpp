#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "report_generator.h"
#include "static_analyzer.h"

using namespace std;

string defaultReportPath(const filesystem::path& sourceFile) {
  filesystem::path outputDir = "reports";
  string stem = sourceFile.stem().string();
  return (outputDir / (stem + "_static_analysis.txt")).string();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: analyze_static <source-file> [report-file]\n";
    return 1;
  }

  filesystem::path sourceFile = argv[1];
  if (!filesystem::exists(sourceFile)) {
    cerr << "Error: source file not found: " << sourceFile.string() << "\n";
    return 1;
  }

  StaticAnalyzer analyzer;
  StaticAnalysisReport report = analyzer.analyzeFile(sourceFile.string());
  string textReport = analyzer.generateTextReport(report);

  cout << textReport;

  ReportGenerator generator;
  string outputPath =
      argc >= 3 ? argv[2] : defaultReportPath(filesystem::absolute(sourceFile));
  generator.saveTextReport(outputPath, textReport);

  return 0;
}
