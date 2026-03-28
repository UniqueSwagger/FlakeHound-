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

  bool parseGoogleTestXML(const string& filename);

  vector<TestSuite> getTestSuites() const;

  vector<TestResult> getAllTests() const;

 private:
  vector<TestSuite> testSuites;

  // Helper functions
  string extractAttribute(const string& line, const string& attrName) const;
  string extractTagContent(const string& xml, const string& tagName) const;
};

#endif
