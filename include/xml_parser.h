#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <map>
#include <string>
#include <vector>
using namespace std;
struct TestResult {
  string name;
  string status;  // "passed", "failed", "skipped"
  double time;
  string message;
};

struct TestSuite {
  string name;
  int tests;
  int failures;
  int errors;
  double time;
  vector<TestResult> testResults;
};

class XMLParser {
 public:
  XMLParser();
  ~XMLParser();

  bool parseGoogleTestXML(string filename);

  vector<TestSuite> getTestSuites();

  vector<TestResult> getAllTests();

 private:
  vector<TestSuite> testSuites;

  // Helper functions
  string extractAttribute(string line, string attrName);
  string extractTagContent(string xml, string tagName);
};

#endif