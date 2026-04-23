#ifndef DEMO_RUNTIME_CONFIG_H
#define DEMO_RUNTIME_CONFIG_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct DemoConfig {
  int randomPassThreshold = 5;
  int timingPassThreshold = 5;
};

int clampDigitThreshold(int value) {
  if (value < 0) {
    return 0;
  }

  if (value > 9) {
    return 9;
  }

  return value;
}

bool parseConfigLine(const string& line, const string& key, int& value) {
  string prefix = key + "=";
  if (line.rfind(prefix, 0) != 0) {
    return false;
  }

  string numberText = line.substr(prefix.length());
  stringstream input(numberText);
  int parsed = 0;
  char extra = '\0';
  input >> parsed;

  if (input.fail() || (input >> extra)) {
    return false;
  }

  value = clampDigitThreshold(parsed);
  return true;
}

DemoConfig loadDemoConfig() {
  DemoConfig config;
  vector<string> candidates;
  candidates.push_back("demo/demo_runtime_config.txt");
  candidates.push_back("demo_runtime_config.txt");

  for (const string& filePath : candidates) {
    ifstream input(filePath);
    if (!input.is_open()) {
      continue;
    }

    string line;
    while (getline(input, line)) {
      int value = 0;
      if (parseConfigLine(line, "random_threshold", value)) {
        config.randomPassThreshold = value;
      } else if (parseConfigLine(line, "timing_threshold", value)) {
        config.timingPassThreshold = value;
      }
    }

    return config;
  }

  return config;
}

#endif
