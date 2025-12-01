/* kernel/ml/svm.c
 * Support Vector Machine (SVM) implementation
 * Linear kernel with simplified SMO algorithm
 */

#include "svm.h"
#include <stdint.h>
#include <stddef.h>

extern void *memset(void *s, int c, size_t n);
extern void serial_puts(const char *s);

/* Dot product of two vectors */
static float dot_product(float *a, float *b, int n) {
    float result = 0.0f;
    for (int i = 0; i < n; i++) {
        result += a[i] * b[i];
    }
    return result;
}

/* Clip value to [L, H] range */
static float clip(float value, float L, float H) {
    if (value < L) return L;
    if (value > H) return H;
    return value;
}

/* Initialize SVM model */
void svm_init(svm_model_t *model, int num_features) {
    if (!model || num_features < 1 || num_features > SVM_MAX_FEATURES) return;
    
    memset(model, 0, sizeof(svm_model_t));
    model->num_features = num_features;
    model->num_support_vectors = 0;
    model->bias = 0.0f;
    model->trained = 0;
}

/* Kernel function (linear kernel) */
static float kernel(float *x1, float *x2, int n) {
    return dot_product(x1, x2, n);
}

/* Predict using SVM */
int svm_predict(svm_model_t *model, float *x) {
    if (!model || !x || !model->trained) return 0;
    
    float sum = model->bias;
    
    /* Compute: f(x) = sum(alpha_i * y_i * K(x_i, x)) + b */
    for (int i = 0; i < model->num_support_vectors; i++) {
        float k = kernel(model->support_vectors[i], x, model->num_features);
        sum += model->alphas[i] * model->sv_labels[i] * k;
    }
    
    return (sum >= 0.0f) ? 1 : -1;
}

/* Simplified SMO training algorithm */
int svm_train(svm_model_t *model, float **X, int *y, int n_samples, 
              float C, float tol, int max_iter) {
    if (!model || !X || !y || n_samples < 1) return -1;
    if (n_samples > SVM_MAX_SAMPLES) return -1;
    
    serial_puts("[svm] Training with SMO algorithm...\n");
    
    /* Initialize alphas to zero */
    float alphas[SVM_MAX_SAMPLES];
    for (int i = 0; i < n_samples; i++) {
        alphas[i] = 0.0f;
    }
    
    float b = 0.0f;
    
    /* SMO main loop */
    int iter = 0;
    int num_changed = 0;
    int examine_all = 1;
    
    while ((num_changed > 0 || examine_all) && iter < max_iter) {
        num_changed = 0;
        
        if (examine_all) {
            /* Loop over all samples */
            for (int i = 0; i < n_samples; i++) {
                /* Compute E_i = f(x_i) - y_i */
                float f_i = b;
                for (int j = 0; j < n_samples; j++) {
                    f_i += alphas[j] * y[j] * kernel(X[j], X[i], model->num_features);
                }
                float E_i = f_i - y[i];
                
                /* Check KKT conditions */
                if ((y[i] * E_i < -tol && alphas[i] < C) ||
                    (y[i] * E_i > tol && alphas[i] > 0)) {
                    
                    /* Select second alpha */
                    int j = (i + 1) % n_samples;
                    
                    float f_j = b;
                    for (int k = 0; k < n_samples; k++) {
                        f_j += alphas[k] * y[k] * kernel(X[k], X[j], model->num_features);
                    }
                    float E_j = f_j - y[j];
                    
                    float alpha_i_old = alphas[i];
                    float alpha_j_old = alphas[j];
                    
                    /* Compute L and H */
                    float L, H;
                    if (y[i] != y[j]) {
                        L = (alphas[j] - alphas[i] > 0) ? alphas[j] - alphas[i] : 0;
                        H = (C + alphas[j] - alphas[i] < C) ? C + alphas[j] - alphas[i] : C;
                    } else {
                        L = (alphas[i] + alphas[j] - C > 0) ? alphas[i] + alphas[j] - C : 0;
                        H = (alphas[i] + alphas[j] < C) ? alphas[i] + alphas[j] : C;
                    }
                    
                    if (L == H) continue;
                    
                    /* Compute eta */
                    float eta = 2 * kernel(X[i], X[j], model->num_features) -
                                kernel(X[i], X[i], model->num_features) -
                                kernel(X[j], X[j], model->num_features);
                    
                    if (eta >= 0) continue;
                    
                    /* Update alpha_j */
                    alphas[j] = alpha_j_old - (y[j] * (E_i - E_j)) / eta;
                    alphas[j] = clip(alphas[j], L, H);
                    
                    if (alphas[j] < 1e-5) alphas[j] = 0;
                    if (alphas[j] > C - 1e-5) alphas[j] = C;
                    
                    if (alphas[j] == alpha_j_old) continue;
                    
                    /* Update alpha_i */
                    alphas[i] = alpha_i_old + y[i] * y[j] * (alpha_j_old - alphas[j]);
                    
                    /* Update bias */
                    float b1 = b - E_i - y[i] * (alphas[i] - alpha_i_old) * kernel(X[i], X[i], model->num_features) -
                               y[j] * (alphas[j] - alpha_j_old) * kernel(X[i], X[j], model->num_features);
                    
                    float b2 = b - E_j - y[i] * (alphas[i] - alpha_i_old) * kernel(X[i], X[j], model->num_features) -
                               y[j] * (alphas[j] - alpha_j_old) * kernel(X[j], X[j], model->num_features);
                    
                    if (alphas[i] > 0 && alphas[i] < C) {
                        b = b1;
                    } else if (alphas[j] > 0 && alphas[j] < C) {
                        b = b2;
                    } else {
                        b = (b1 + b2) / 2.0f;
                    }
                    
                    num_changed++;
                }
            }
        }
        
        if (examine_all) {
            examine_all = 0;
        } else if (num_changed == 0) {
            examine_all = 1;
        }
        
        iter++;
    }
    
    /* Extract support vectors */
    model->num_support_vectors = 0;
    for (int i = 0; i < n_samples && model->num_support_vectors < SVM_MAX_SUPPORT_VECTORS; i++) {
        if (alphas[i] > 0) {
            int sv_idx = model->num_support_vectors;
            
            /* Copy support vector */
            for (int j = 0; j < model->num_features; j++) {
                model->support_vectors[sv_idx][j] = X[i][j];
            }
            model->alphas[sv_idx] = alphas[i];
            model->sv_labels[sv_idx] = y[i];
            
            model->num_support_vectors++;
        }
    }
    
    model->bias = b;
    model->trained = 1;
    
    serial_puts("[svm] Training complete. Support vectors: ");
    extern void serial_put_hex(uint64_t val);
    serial_put_hex(model->num_support_vectors);
    serial_puts("\n");
    
    return 0;
}

/* Evaluate SVM accuracy */
float svm_evaluate(svm_model_t *model, float **X, int *y, int n_samples) {
    if (!model || !X || !y || !model->trained) return 0.0f;
    
    int correct = 0;
    for (int i = 0; i < n_samples; i++) {
        int predicted = svm_predict(model, X[i]);
        if (predicted == y[i]) {
            correct++;
        }
    }
    
    return (float)correct / (float)n_samples;
}
