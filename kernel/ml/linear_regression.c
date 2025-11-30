/* kernel/ml/linear_regression.c
 * Linear regression implementation
 */

#include "linear_regression.h"
#include <stdint.h>
#include <stddef.h>

extern void show_string(const char *s);
extern void *memset(void *s, int c, size_t n);

/* Simple sqrt approximation using Newton's method */
static float sqrt_approx(float x) {
    if (x <= 0.0f) return 0.0f;
    
    float guess = x / 2.0f;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

void lr_init(linear_regression_t *model, int num_features) {
    if (!model || num_features < 1 || num_features > LR_MAX_FEATURES) return;
    
    memset(model, 0, sizeof(linear_regression_t));
    model->num_features = num_features;
    model->trained = 0;
    
    /* Initialize weights to small random values
     * For simplicity, use 0.1 * (i % 7) as pseudo-random
     */
    for (int i = 0; i <= num_features; i++) {
        model->weights[i] = 0.01f * (i % 7);
    }
}

float lr_predict(linear_regression_t *model, float *features) {
    if (!model || !features) return 0.0f;
    
    /* y = w0 + w1*x1 + w2*x2 + ... */
    float prediction = model->weights[0];  // Bias term
    
    for (int i = 0; i < model->num_features; i++) {
        prediction += model->weights[i + 1] * features[i];
    }
    
    return prediction;
}

float lr_train(linear_regression_t *model, lr_dataset_t *dataset,
               float learning_rate, int num_iterations) {
    if (!model || !dataset) return -1.0f;
    if (dataset->num_samples < 1) return -1.0f;
    if (model->num_features != dataset->num_features) return -1.0f;
    
    float loss = 0.0f;
    
    /* Gradient descent */
    for (int iter = 0; iter < num_iterations; iter++) {
        /* Accumulate gradients */
        float gradients[LR_MAX_FEATURES + 1];
        memset(gradients, 0, sizeof(gradients));
        
        loss = 0.0f;
        
        /* For each training sample */
        for (int s = 0; s < dataset->num_samples; s++) {
            /* Predict */
            float prediction = lr_predict(model, dataset->features[s]);
            
            /* Error = prediction - actual */
            float error = prediction - dataset->labels[s];
            
            /* Accumulate loss (MSE) */
            loss += error * error;
            
            /* Gradient of bias: error */
            gradients[0] += error;
            
            /* Gradient of weights: error * feature */
            for (int f = 0; f < model->num_features; f++) {
                gradients[f + 1] += error * dataset->features[s][f];
            }
        }
        
        /* Average loss */
        loss /= dataset->num_samples;
        
        /* Update weights: w = w - learning_rate * gradient */
        for (int i = 0; i <= model->num_features; i++) {
            model->weights[i] -= learning_rate * (gradients[i] / dataset->num_samples);
        }
        
        /* Print progress every 100 iterations */
        if (iter % 100 == 0) {
            show_string("[ml] Iteration ");
            extern void show_int(int val);
            show_int(iter);
            show_string(", Loss: ");
            show_int((int)(loss * 1000));  // Print as integer (loss * 1000)
            show_string("\n");
        }
    }
    
    model->trained = 1;
    return loss;
}

float lr_evaluate(linear_regression_t *model, lr_dataset_t *dataset) {
    if (!model || !dataset || !model->trained) return -1.0f;
    
    float mse = 0.0f;
    
    for (int s = 0; s < dataset->num_samples; s++) {
        float prediction = lr_predict(model, dataset->features[s]);
        float error = prediction - dataset->labels[s];
        mse += error * error;
    }
    
    mse /= dataset->num_samples;
    return mse;
}

void lr_get_weights(linear_regression_t *model, float *weights) {
    if (!model || !weights) return;
    
    for (int i = 0; i <= model->num_features; i++) {
        weights[i] = model->weights[i];
    }
}

void lr_load_weights(linear_regression_t *model, float *weights) {
    if (!model || !weights) return;
    
    for (int i = 0; i <= model->num_features; i++) {
        model->weights[i] = weights[i];
    }
    
    model->trained = 1;
}
