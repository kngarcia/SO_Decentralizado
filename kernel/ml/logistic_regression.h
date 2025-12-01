/* kernel/ml/logistic_regression.h */
#ifndef LOGISTIC_REGRESSION_H
#define LOGISTIC_REGRESSION_H

#define LOGREG_MAX_FEATURES 32
#define LOGREG_MAX_SAMPLES 256

typedef struct {
    int num_features;
    float weights[LOGREG_MAX_FEATURES + 1];  /* +1 for bias */
    int trained;
} logistic_regression_t;

typedef struct {
    int num_samples;
    int num_features;
    float features[LOGREG_MAX_SAMPLES][LOGREG_MAX_FEATURES];
    float labels[LOGREG_MAX_SAMPLES];  /* 0 or 1 for binary classification */
} logreg_dataset_t;

/* Initialize model */
void logreg_init(logistic_regression_t *model, int num_features);

/* Predict probability */
float logreg_predict_proba(logistic_regression_t *model, float *features);

/* Predict class (0 or 1) */
int logreg_predict(logistic_regression_t *model, float *features);

/* Train model */
float logreg_train(logistic_regression_t *model, logreg_dataset_t *dataset,
                   float learning_rate, int num_iterations);

/* Evaluate accuracy */
float logreg_evaluate(logistic_regression_t *model, logreg_dataset_t *dataset);

#endif /* LOGISTIC_REGRESSION_H */
