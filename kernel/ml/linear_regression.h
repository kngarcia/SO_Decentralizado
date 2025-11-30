/* kernel/ml/linear_regression.h
 * Simple linear regression implementation for embedded ML
 * y = mx + b (single feature)
 * y = w0 + w1*x1 + w2*x2 + ... (multiple features)
 */

#ifndef LINEAR_REGRESSION_H
#define LINEAR_REGRESSION_H

#include <stdint.h>

/* Maximum number of features supported */
#define LR_MAX_FEATURES 16
#define LR_MAX_SAMPLES 128

/**
 * Linear regression model structure
 */
typedef struct {
    float weights[LR_MAX_FEATURES + 1];  // w0 (bias) + w1..wN
    int num_features;
    int trained;
} linear_regression_t;

/**
 * Training data structure
 */
typedef struct {
    float features[LR_MAX_SAMPLES][LR_MAX_FEATURES];
    float labels[LR_MAX_SAMPLES];
    int num_samples;
    int num_features;
} lr_dataset_t;

/**
 * Initialize a linear regression model
 * @param model Pointer to model structure
 * @param num_features Number of input features
 */
void lr_init(linear_regression_t *model, int num_features);

/**
 * Train the model using gradient descent
 * @param model Pointer to model structure
 * @param dataset Training data
 * @param learning_rate Learning rate (typically 0.001 - 0.1)
 * @param num_iterations Number of training iterations
 * @return Final loss (mean squared error)
 */
float lr_train(linear_regression_t *model, lr_dataset_t *dataset, 
               float learning_rate, int num_iterations);

/**
 * Make a prediction
 * @param model Trained model
 * @param features Input features array (size = num_features)
 * @return Predicted value
 */
float lr_predict(linear_regression_t *model, float *features);

/**
 * Evaluate model on test set
 * @param model Trained model
 * @param dataset Test data
 * @return Mean squared error
 */
float lr_evaluate(linear_regression_t *model, lr_dataset_t *dataset);

/**
 * Get model parameters
 * @param model Model structure
 * @param weights Output array for weights (size = num_features + 1)
 */
void lr_get_weights(linear_regression_t *model, float *weights);

/**
 * Load pre-trained weights
 * @param model Model structure
 * @param weights Input array of weights (size = num_features + 1)
 */
void lr_load_weights(linear_regression_t *model, float *weights);

#endif /* LINEAR_REGRESSION_H */
