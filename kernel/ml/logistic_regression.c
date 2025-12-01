/* kernel/ml/logistic_regression.c
 * Logistic regression for binary classification
 */

#include "logistic_regression.h"
#include <stdint.h>
#include <stddef.h>

extern void show_string(const char *s);
extern void *memset(void *s, int c, size_t n);

/* Sigmoid activation function */
static float sigmoid(float x) {
    /* Approximation: sigmoid(x) ≈ 1 / (1 + e^-x)
     * For kernel space, use Taylor series approximation
     */
    if (x > 5.0f) return 1.0f;
    if (x < -5.0f) return 0.0f;
    
    /* Simple approximation: sigmoid(x) ≈ 0.5 + 0.25*x for small x */
    if (x > -2.0f && x < 2.0f) {
        return 0.5f + 0.25f * x;
    }
    
    /* Better approximation using exp approximation */
    float exp_neg_x = 1.0f;
    float term = 1.0f;
    for (int i = 1; i <= 10; i++) {
        term = term * (-x) / i;
        exp_neg_x += term;
    }
    
    return 1.0f / (1.0f + exp_neg_x);
}

/* Initialize logistic regression model */
void logreg_init(logistic_regression_t *model, int num_features) {
    if (!model || num_features < 1 || num_features > LOGREG_MAX_FEATURES) return;
    
    memset(model, 0, sizeof(logistic_regression_t));
    model->num_features = num_features;
    model->trained = 0;
    
    /* Initialize weights to small values */
    for (int i = 0; i <= num_features; i++) {
        model->weights[i] = 0.01f * (i % 7);
    }
}

/* Predict probability (0 to 1) */
float logreg_predict_proba(logistic_regression_t *model, float *features) {
    if (!model || !features) return 0.0f;
    
    /* Compute linear combination: z = w0 + w1*x1 + w2*x2 + ... */
    float z = model->weights[0];  /* Bias */
    
    for (int i = 0; i < model->num_features; i++) {
        z += model->weights[i + 1] * features[i];
    }
    
    /* Apply sigmoid to get probability */
    return sigmoid(z);
}

/* Predict class (0 or 1) */
int logreg_predict(logistic_regression_t *model, float *features) {
    float prob = logreg_predict_proba(model, features);
    return (prob >= 0.5f) ? 1 : 0;
}

/* Train logistic regression model using gradient descent */
float logreg_train(logistic_regression_t *model, logreg_dataset_t *dataset,
                   float learning_rate, int num_iterations) {
    if (!model || !dataset) return -1.0f;
    if (dataset->num_samples < 1) return -1.0f;
    if (model->num_features != dataset->num_features) return -1.0f;
    
    float loss = 0.0f;
    
    /* Gradient descent with binary cross-entropy loss */
    for (int iter = 0; iter < num_iterations; iter++) {
        float gradients[LOGREG_MAX_FEATURES + 1];
        memset(gradients, 0, sizeof(gradients));
        
        loss = 0.0f;
        
        /* For each training sample */
        for (int s = 0; s < dataset->num_samples; s++) {
            /* Predict probability */
            float prob = logreg_predict_proba(model, dataset->features[s]);
            
            /* True label (0 or 1) */
            float y_true = dataset->labels[s];
            
            /* Binary cross-entropy loss: -[y*log(p) + (1-y)*log(1-p)] */
            /* Simplified: just track error for now */
            float error = prob - y_true;
            loss += error * error;  /* MSE approximation */
            
            /* Compute gradients */
            /* For logistic regression: gradient = (predicted - actual) * feature */
            gradients[0] += error;  /* Bias gradient */
            
            for (int i = 0; i < model->num_features; i++) {
                gradients[i + 1] += error * dataset->features[s][i];
            }
        }
        
        /* Average gradients */
        float n_samples = (float)dataset->num_samples;
        for (int i = 0; i <= model->num_features; i++) {
            gradients[i] /= n_samples;
        }
        
        /* Update weights: w = w - learning_rate * gradient */
        for (int i = 0; i <= model->num_features; i++) {
            model->weights[i] -= learning_rate * gradients[i];
        }
        
        /* Average loss */
        loss /= n_samples;
    }
    
    model->trained = 1;
    return loss;
}

/* Evaluate model accuracy */
float logreg_evaluate(logistic_regression_t *model, logreg_dataset_t *dataset) {
    if (!model || !dataset || !model->trained) return 0.0f;
    
    int correct = 0;
    
    for (int s = 0; s < dataset->num_samples; s++) {
        int predicted = logreg_predict(model, dataset->features[s]);
        int actual = (int)dataset->labels[s];
        
        if (predicted == actual) {
            correct++;
        }
    }
    
    return (float)correct / (float)dataset->num_samples;
}
