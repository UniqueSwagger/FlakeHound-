#include "logistic_regression.h"

#include <cmath>
#include <stdexcept>

using namespace std;

LogisticRegressionSGD::LogisticRegressionSGD() : bias(0.0) {}

LogisticRegressionSGD::LogisticRegressionSGD(int featureCount) : bias(0.0) {
  initialize(featureCount);
}

void LogisticRegressionSGD::initialize(int featureCount) {
  weights.assign(featureCount, 0.0);
  bias = 0.0;
}

void LogisticRegressionSGD::train(const vector<vector<double>>& features,
                                  const vector<int>& labels, int epochs,
                                  double learningRate, double regularization) {
  if (features.empty() || labels.empty() || features.size() != labels.size()) {
    return;
  }

  if (weights.empty()) {
    initialize(features.front().size());
  }

  for (const vector<double>& sample : features) {
    if (sample.size() != weights.size()) {
      throw invalid_argument("All feature vectors must have the same size");
    }
  }

  for (int epoch = 0; epoch < epochs; epoch++) {
    for (int i = 0; i < features.size(); i++) {
      const vector<double>& sample = features[i];
      double prediction = predictProbability(sample);
      double error = prediction - labels[i];

      for (int j = 0; j < weights.size(); j++) {
        weights[j] -=
            learningRate * (error * sample[j] + regularization * weights[j]);
      }
      bias -= learningRate * error;
    }
  }
}

double LogisticRegressionSGD::predictProbability(
    const vector<double>& features) const {
  return sigmoid(linearScore(features));
}

int LogisticRegressionSGD::predict(const vector<double>& features,
                                   double threshold) const {
  return predictProbability(features) >= threshold ? 1 : 0;
}

vector<double> LogisticRegressionSGD::getWeights() const { return weights; }

double LogisticRegressionSGD::getBias() const { return bias; }

double LogisticRegressionSGD::sigmoid(double x) {
  if (x >= 0.0) {
    double z = exp(-x);
    return 1.0 / (1.0 + z);
  }

  double z = exp(x);
  return z / (1.0 + z);
}

double LogisticRegressionSGD::linearScore(
    const vector<double>& features) const {
  double score = bias;
  int count = min(weights.size(), features.size());
  for (int i = 0; i < count; i++) {
    score += weights[i] * features[i];
  }
  return score;
}
