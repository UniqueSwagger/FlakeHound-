#include "xml_parser.h"

#include <fstream>
#include <sstream>

using namespace std;

XMLParser::XMLParser() {}

XMLParser::~XMLParser() {}

bool XMLParser::parseGoogleTestXML(string& filename) {
  testSuites.clear();

  ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  stringstream buffer;
  buffer << file.rdbuf();
  string xml = buffer.str();

  size_t pos = 0;
  while ((pos = xml.find("<testsuite", pos)) != string::npos) {
    size_t suiteEnd = xml.find("</testsuite>", pos);
    if (suiteEnd == string::npos) break;

    size_t tagEnd = xml.find('>', pos);
    if (tagEnd == string::npos) break;

    string suiteTag = xml.substr(pos, tagEnd - pos + 1);
    string suiteBody = xml.substr(tagEnd + 1, suiteEnd - tagEnd - 1);

    TestSuite suite;
    suite.name = extractAttribute(suiteTag, "name");
    suite.tests = atoi(extractAttribute(suiteTag, "tests").c_str());
    suite.failures = atoi(extractAttribute(suiteTag, "failures").c_str());
    suite.errors = atoi(extractAttribute(suiteTag, "errors").c_str());
    suite.time = atof(extractAttribute(suiteTag, "time").c_str());

    size_t testPos = 0;
    while ((testPos = suiteBody.find("<testcase", testPos)) != string::npos) {
      size_t testTagEnd = suiteBody.find('>', testPos);
      if (testTagEnd == string::npos) break;

      size_t testClose = suiteBody.find("</testcase>", testTagEnd);
      size_t selfClose = suiteBody.find("/>", testTagEnd);

      bool isSelfClosing =
          (selfClose != string::npos &&
           (testClose == string::npos || selfClose < testClose));

      string testTag = suiteBody.substr(testPos, testTagEnd - testPos + 1);
      string testInner = "";
      size_t nextPos = 0;

      if (isSelfClosing) {
        nextPos = selfClose + 2;
      } else if (testClose != string::npos) {
        testInner =
            suiteBody.substr(testTagEnd + 1, testClose - testTagEnd - 1);
        nextPos = testClose + string("</testcase>").length();
      } else {
        break;
      }

      TestResult result;
      result.name = extractAttribute(testTag, "name");
      result.time = atof(extractAttribute(testTag, "time").c_str());
      result.status = "passed";
      result.message = "";

      if (testInner.find("<failure") != string::npos) {
        result.status = "failed";
        result.message = extractAttribute(testInner, "message");
      } else if (testInner.find("<skipped") != string::npos) {
        result.status = "skipped";
      }

      suite.testResults.push_back(result);

      testPos = nextPos;
    }

    testSuites.push_back(suite);
    pos = suiteEnd + string("</testsuite>").length();
  }

  return !testSuites.empty();
}

vector<TestSuite> XMLParser::getTestSuites() { return testSuites; }

vector<TestResult> XMLParser::getAllTests() {
  vector<TestResult> allTests;
  for (auto& suite : testSuites) {
    allTests.insert(allTests.end(), suite.testResults.begin(),
                    suite.testResults.end());
  }
  return allTests;
}

string XMLParser::extractAttribute(string line, string attrName) {
  string pattern = attrName + "=\"";
  size_t start = line.find(pattern);
  if (start == string::npos) return "";
  start += pattern.length();
  size_t end = line.find('"', start);
  if (end == string::npos) return "";
  return line.substr(start, end - start);
}

string XMLParser::extractTagContent(string xml, string tagName) {
  string openTag = "<" + tagName + ">";
  string closeTag = "</" + tagName + ">";
  size_t start = xml.find(openTag);
  if (start == string::npos) return "";
  start += openTag.length();
  size_t end = xml.find(closeTag, start);
  if (end == string::npos) return "";
  return xml.substr(start, end - start);
}