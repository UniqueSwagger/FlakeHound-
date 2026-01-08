#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <map>
#include <string>
#include <vector>

struct TestResult {
  std::string name;
  std::string status;  // "passed", "failed", "skipped"
  double time;
  std::string message;
};

struct TestSuite {
  std::string name;
  int tests;
  int failures;
  int errors;
  double time;
  std::vector<TestResult> testResults;
};

class XMLParser {
 public:
  XMLParser();
  ~XMLParser();

  bool parseGoogleTestXML(std::string filename);

  std::vector<TestSuite> getTestSuites();

  std::vector<TestResult> getAllTests();

 private:
  std::vector<TestSuite> testSuites;

  // Helper functions
  std::string extractAttribute(std::string line, std::string attrName);
  std::string extractTagContent(std::string xml, std::string tagName);
};

#endif