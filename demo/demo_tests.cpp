#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

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

bool test_random_number() {
  int random_value = rand() % 100;
  if (random_value > 50) {
    cout << "PASS: test_random_number (got " << random_value << ")\n";
    return true;
  } else {
    cout << "FAIL: test_random_number (got " << random_value << ")\n";
    return false;
  }
}

bool test_timing_based() {
  int current_time = time(nullptr) % 10;
  if (current_time > 4) {
    cout << "PASS: test_timing_based (time mod = " << current_time << ")\n";
    return true;
  } else {
    cout << "FAIL: test_timing_based (time mod = " << current_time << ")\n";
    return false;
  }
}

int main() {
  srand(time(nullptr));

  cout << "FlakeHound++ Demo Tests \n\n";

  int passed = 0;
  int failed = 0;

  if (test_addition())
    passed++;
  else
    failed++;
  if (test_string_length())
    passed++;
  else
    failed++;
  if (test_random_number())
    passed++;
  else
    failed++;
  if (test_timing_based())
    passed++;
  else
    failed++;

  cout << "\n Test Summary \n";
  cout << "Passed: " << passed << "\n";
  cout << "Failed: " << failed << "\n";
  cout << "Total: " << (passed + failed) << "\n";

  return 0;
}
