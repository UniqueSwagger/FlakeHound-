#include <filesystem>
#include <iostream>
#include <vector>

#include "xml_parser.h"

using namespace std;

filesystem::path findXmlFile(int argc, char* argv[]) {
  vector<filesystem::path> candidates;

  if (argc >= 2) {
    candidates.push_back(argv[1]);
  }

  candidates.push_back(filesystem::path("demo") / "sample_gtest_results.xml");
  candidates.push_back(filesystem::path("build") / "abseil_test_results.xml");
  candidates.push_back(filesystem::path("abseil-cpp") / "run_1.xml");
  candidates.push_back(filesystem::path("..") / "demo" /
                       "sample_gtest_results.xml");

  for (const filesystem::path& candidate : candidates) {
    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::exists(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

int main(int argc, char* argv[]) {
  cout << "Testing XMLParser on GoogleTest XML output\n\n";

  filesystem::path xmlFile = findXmlFile(argc, argv);
  if (xmlFile.empty()) {
    cout << "Failed to locate a GoogleTest XML file.\n";
    return 1;
  }

  XMLParser parser;
  bool success = parser.parseGoogleTestXML(xmlFile.string());

  if (!success) {
    cout << "Failed to parse XML file: " << xmlFile.string() << "\n";
    return 1;
  }

  cout << "Successfully parsed XML!\n\n";

  vector<TestSuite> suites = parser.getTestSuites();
  cout << "Found " << suites.size() << " test suites\n\n";

  for (const TestSuite& suite : suites) {
    cout << "Suite: " << suite.name << "\n";
    cout << "  Tests: " << suite.tests << "\n";
    cout << "  Failures: " << suite.failures << "\n";
    cout << "  Time: " << suite.time << "s\n";
    cout << "  Test cases:\n";

    for (const TestResult& test : suite.testResults) {
      cout << "    - " << test.name << " [" << test.status << "] " << test.time
           << "s\n";
    }
    cout << "\n";
  }

  vector<TestResult> allTests = parser.getAllTests();
  cout << "Total tests found: " << allTests.size() << "\n";

  return allTests.empty() ? 1 : 0;
}
