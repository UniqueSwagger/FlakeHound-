#ifndef LOGISTIC_REGRESSION_H
#define LOGISTIC_REGRESSION_H

#include <cstddef>
#include <vector>

using namespace std;

class LogisticRegressionSGD {
 public:
  LogisticRegressionSGD();
  explicit LogisticRegressionSGD(int featureCount);
  ~LogisticRegressionSGD();

  void initialize(int featureCount);
  void train(const vector<vector<double>>& features, const vector<int>& labels,
             int epochs = 400, double learningRate = 0.05,
             double regularization = 0.001);
  double predictProbability(const vector<double>& features) const;
  int predict(const vector<double>& features, double threshold = 0.5) const;

  vector<double> getWeights() const;
  double getBias() const;

 private:
  vector<double> weights;
  double bias;

  static double sigmoid(double x);
  double linearScore(const vector<double>& features) const;
};

#endif
