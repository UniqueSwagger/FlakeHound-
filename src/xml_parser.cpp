#include "xml_parser.h"

using namespace std;

XMLParser::XMLParser() {}

XMLParser::~XMLParser() {}

bool XMLParser::parseGoogleTestXML(string& filename) { return false; }

vector<TestSuite> XMLParser::getTestSuites() { return testSuites; }

vector<TestResult> XMLParser::getAllTests() {
  vector<TestResult> allTests;
  for (auto& suite : testSuites) {
    allTests.insert(allTests.end(), suite.testResults.begin(),
                    suite.testResults.end());
  }
  return allTests;
}

string XMLParser::extractAttribute(string line, string attrName) { return ""; }

string XMLParser::extractTagContent(string xml, string tagName) { return ""; }