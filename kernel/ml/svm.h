/* kernel/ml/svm.h */
#ifndef SVM_H
#define SVM_H

#define SVM_MAX_FEATURES 32
#define SVM_MAX_SAMPLES 256
#define SVM_MAX_SUPPORT_VECTORS 128

typedef struct {
    int num_features;
    int num_support_vectors;
    float support_vectors[SVM_MAX_SUPPORT_VECTORS][SVM_MAX_FEATURES];
    float alphas[SVM_MAX_SUPPORT_VECTORS];
    int sv_labels[SVM_MAX_SUPPORT_VECTORS];  /* -1 or +1 */
    float bias;
    int trained;
} svm_model_t;

/* Initialize SVM model */
void svm_init(svm_model_t *model, int num_features);

/* Train SVM using SMO algorithm */
int svm_train(svm_model_t *model, float **X, int *y, int n_samples, 
              float C, float tol, int max_iter);

/* Predict class (-1 or +1) */
int svm_predict(svm_model_t *model, float *x);

/* Evaluate accuracy */
float svm_evaluate(svm_model_t *model, float **X, int *y, int n_samples);

#endif /* SVM_H */
