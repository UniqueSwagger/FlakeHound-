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

  string generateTextReport(vector<FlakinessScore>& scores);
};

#endif
