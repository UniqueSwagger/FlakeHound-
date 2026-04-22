#include "xml_parser.h"

#include <fstream>
#include <sstream>

using namespace std;

bool XMLParser::parseGoogleTestXML(const string& filename) {
  testSuites.clear();

  ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  stringstream buffer;
  buffer << file.rdbuf();
  string xml = buffer.str();

  int pos = 0;
  while ((pos = xml.find("<testsuite", pos)) != string::npos) {
    char nextChar = (pos + 10 < xml.size()) ? xml[pos + 10] : '\0';
    if (nextChar != ' ' && nextChar != '>' && nextChar != '\t' &&
        nextChar != '\n' && nextChar != '\r') {
      pos += 10;
      continue;
    }

    int suiteEnd = xml.find("</testsuite>", pos);
    if (suiteEnd == string::npos) break;

    int tagEnd = xml.find('>', pos);
    if (tagEnd == string::npos) break;

    string suiteTag = xml.substr(pos, tagEnd - pos + 1);
    string suiteBody = xml.substr(tagEnd + 1, suiteEnd - tagEnd - 1);

    TestSuite suite;
    suite.name = extractAttribute(suiteTag, "name");
    suite.tests = atoi(extractAttribute(suiteTag, "tests").c_str());
    suite.failures = atoi(extractAttribute(suiteTag, "failures").c_str());
    suite.errors = atoi(extractAttribute(suiteTag, "errors").c_str());
    suite.time = atof(extractAttribute(suiteTag, "time").c_str());

    int testPos = 0;
    while ((testPos = suiteBody.find("<testcase", testPos)) != string::npos) {
      int testTagEnd = suiteBody.find('>', testPos);
      if (testTagEnd == string::npos) break;

      int testClose = suiteBody.find("</testcase>", testPos);
      int selfClose = suiteBody.find("/>", testPos);

      bool isSelfClosing =
          (selfClose != string::npos && selfClose < testTagEnd &&
           (testClose == string::npos || selfClose < testClose));

      string testTag = suiteBody.substr(testPos, testTagEnd - testPos + 1);
      string testInner = "";
      int nextPos = 0;

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

      if (testInner.find("<failure") != string::npos ||
          testInner.find("<error") != string::npos) {
        result.status = "failed";
        result.message = extractAttribute(testInner, "message");
      } else if (testInner.find("<skipped") != string::npos) {
        result.status = "skipped";
        result.message = extractAttribute(testInner, "message");
      }

      suite.testResults.push_back(result);

      testPos = nextPos;
    }

    testSuites.push_back(suite);
    pos = suiteEnd + string("</testsuite>").length();
  }

  return !testSuites.empty();
}

vector<TestSuite> XMLParser::getTestSuites() const { return testSuites; }

vector<TestResult> XMLParser::getAllTests() const {
  vector<TestResult> allTests;
  for (auto& suite : testSuites) {
    allTests.insert(allTests.end(), suite.testResults.begin(),
                    suite.testResults.end());
  }
  return allTests;
}

string XMLParser::extractAttribute(const string& line,
                                   const string& attrName) const {
  string pattern = attrName + "=\"";
  int start = line.find(pattern);
  if (start == string::npos) return "";
  start += pattern.length();
  int end = line.find('"', start);
  if (end == string::npos) return "";
  return line.substr(start, end - start);
}

string XMLParser::extractTagContent(const string& xml,
                                    const string& tagName) const {
  string openTag = "<" + tagName + ">";
  string closeTag = "</" + tagName + ">";
  int start = xml.find(openTag);
  if (start == string::npos) return "";
  start += openTag.length();
  int end = xml.find(closeTag, start);
  if (end == string::npos) return "";
  return xml.substr(start, end - start);
}
