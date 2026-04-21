#include <chrono>
#include <cstdlib>
#include <thread>

int shared_counter;

int flaky_computation() {
  int value;
  if (std::rand() % 2 == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return value + shared_counter;
}
