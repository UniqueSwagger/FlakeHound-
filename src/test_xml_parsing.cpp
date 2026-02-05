#include <iostream>

#include "xml_parser.h"

using namespace std;

int main() {
  cout << "Testing XMLParser on GoogleTest XML output\n\n";

  XMLParser parser;
  string xmlFile = "abseil_test_results.xml";

  bool success = parser.parseGoogleTestXML(xmlFile);

  if (!success) {
    cout << "Failed to parse XML file: " << xmlFile << "\n";
    return 1;
  }

  cout << "Successfully parsed XML!\n\n";

  vector<TestSuite> suites = parser.getTestSuites();
  cout << "Found " << suites.size() << " test suites\n\n";

  for (auto& suite : suites) {
    cout << "Suite: " << suite.name << "\n";
    cout << "  Tests: " << suite.tests << "\n";
    cout << "  Failures: " << suite.failures << "\n";
    cout << "  Time: " << suite.time << "s\n";
    cout << "  Test cases:\n";

    for (auto& test : suite.testResults) {
      cout << "    - " << test.name << " [" << test.status << "] " << test.time
           << "s\n";
    }
    cout << "\n";
  }

  vector<TestResult> allTests = parser.getAllTests();
  cout << "Total tests found: " << allTests.size() << "\n";

  return 0;
}
