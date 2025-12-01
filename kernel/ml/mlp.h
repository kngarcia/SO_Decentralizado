/* kernel/ml/mlp.h */
#ifndef MLP_H
#define MLP_H

#define MLP_MAX_LAYERS 5
#define MLP_MAX_NEURONS 128

typedef struct {
    int num_layers;
    int layer_sizes[MLP_MAX_LAYERS];
    float weights[MLP_MAX_LAYERS - 1][MLP_MAX_NEURONS][MLP_MAX_NEURONS];
    float biases[MLP_MAX_LAYERS - 1][MLP_MAX_NEURONS];
    int trained;
} mlp_model_t;

/* Initialize MLP */
void mlp_init(mlp_model_t *model, int *layer_sizes, int n_layers);

/* Train MLP using backpropagation */
float mlp_train(mlp_model_t *model, float **X, float **y, int n_samples, 
                float learning_rate, int epochs);

/* Predict using MLP */
void mlp_predict(mlp_model_t *model, float *input, float *output);

/* Evaluate accuracy */
float mlp_evaluate(mlp_model_t *model, float **X, float **y, int n_samples);

#endif /* MLP_H */
