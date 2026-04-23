#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

#include "demo_runtime_config.h"

using namespace std;

bool test_addition() {
  int a = 5;
  int b = 3;
  int result = a + b;
  if (result == 8) {
    cout << "PASS: test_addition\n";
    return true;
  } else {
    cout << "FAIL: test_addition\n";
    return false;
  }
}

bool test_string_length() {
  string text = "hello";
  int length = text.length();
  if (length == 5) {
    cout << "PASS: test_string_length\n";
    return true;
  } else {
    cout << "FAIL: test_string_length\n";
    return false;
  }
}

bool test_random_number(const DemoConfig& config) {
  unsigned seed =
      (unsigned)(chrono::steady_clock::now().time_since_epoch().count());
  srand(seed);
  int randomValue = rand() % 10;

  if (randomValue >= config.randomPassThreshold) {
    cout << "PASS: test_random_number (got " << randomValue
         << ", cutoff " << config.randomPassThreshold << ")\n";
    return true;
  } else {
    cout << "FAIL: test_random_number (got " << randomValue
         << ", cutoff " << config.randomPassThreshold << ")\n";
    return false;
  }
}

bool test_timing_based(const DemoConfig& config) {
  int currentTime =
      (int)(chrono::high_resolution_clock::now().time_since_epoch().count() %
            10);

  if (currentTime >= config.timingPassThreshold) {
    cout << "PASS: test_timing_based (time mod = " << currentTime
         << ", cutoff " << config.timingPassThreshold << ")\n";
    return true;
  } else {
    cout << "FAIL: test_timing_based (time mod = " << currentTime
         << ", cutoff " << config.timingPassThreshold << ")\n";
    return false;
  }
}

int main() {
  DemoConfig config = loadDemoConfig();

  cout << "FlakeHound++ Demo Tests \n\n";
  cout << "Random test cutoff: " << config.randomPassThreshold << "\n";
  cout << "Timing test cutoff: " << config.timingPassThreshold << "\n\n";

  int passed = 0;
  int failed = 0;

  if (test_addition()) {
    passed++;
  } else {
    failed++;
  }

  if (test_string_length()) {
    passed++;
  } else {
    failed++;
  }

  if (test_random_number(config)) {
    passed++;
  } else {
    failed++;
  }

  if (test_timing_based(config)) {
    passed++;
  } else {
    failed++;
  }

  cout << "\nTest Summary\n";
  cout << "Passed: " << passed << "\n";
  cout << "Failed: " << failed << "\n";
  cout << "Total: " << (passed + failed) << "\n";

  return 0;
}
