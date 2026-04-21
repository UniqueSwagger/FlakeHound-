#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include <string>
#include <vector>

#include "flakliness_calculator.h"

using namespace std;

class ReportGenerator {
 public:
  ReportGenerator();
  ~ReportGenerator();

  void generateReport(string filename, vector<FlakinessScore>& scores);
  void saveTextReport(const string& filename, const string& reportText);

  string generateTextReport(vector<FlakinessScore>& scores);
};

#endif
