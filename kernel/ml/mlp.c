/* kernel/ml/mlp.c
 * Multi-Layer Perceptron (Neural Network) with backpropagation
 */

#include "mlp.h"
#include <stdint.h>
#include <stddef.h>

extern void *memset(void *s, int c, size_t n);
extern void serial_puts(const char *s);

/* Activation functions */
static float relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

static float relu_derivative(float x) {
    return (x > 0.0f) ? 1.0f : 0.0f;
}

static float sigmoid(float x) {
    if (x > 5.0f) return 1.0f;
    if (x < -5.0f) return 0.0f;
    
    /* Approximation */
    float exp_neg_x = 1.0f;
    float term = 1.0f;
    for (int i = 1; i <= 10; i++) {
        term = term * (-x) / i;
        exp_neg_x += term;
    }
    
    return 1.0f / (1.0f + exp_neg_x);
}

static float sigmoid_derivative(float x) {
    float s = sigmoid(x);
    return s * (1.0f - s);
}

/* Initialize MLP */
void mlp_init(mlp_model_t *model, int *layer_sizes, int n_layers) {
    if (!model || !layer_sizes || n_layers < 2 || n_layers > MLP_MAX_LAYERS) return;
    
    memset(model, 0, sizeof(mlp_model_t));
    model->num_layers = n_layers;
    
    for (int i = 0; i < n_layers; i++) {
        model->layer_sizes[i] = layer_sizes[i];
    }
    
    /* Initialize weights with small random values */
    for (int l = 0; l < n_layers - 1; l++) {
        int input_size = model->layer_sizes[l];
        int output_size = model->layer_sizes[l + 1];
        
        for (int i = 0; i < output_size; i++) {
            for (int j = 0; j < input_size; j++) {
                /* Xavier initialization: random between -1/sqrt(n) and 1/sqrt(n) */
                model->weights[l][i][j] = 0.01f * ((i + j) % 13 - 6);
            }
            model->biases[l][i] = 0.01f * (i % 7);
        }
    }
    
    model->trained = 0;
}

/* Forward pass */
static void forward_pass(mlp_model_t *model, float *input, float activations[MLP_MAX_LAYERS][MLP_MAX_NEURONS]) {
    /* Copy input to first layer */
    for (int i = 0; i < model->layer_sizes[0]; i++) {
        activations[0][i] = input[i];
    }
    
    /* Propagate through layers */
    for (int l = 0; l < model->num_layers - 1; l++) {
        int input_size = model->layer_sizes[l];
        int output_size = model->layer_sizes[l + 1];
        
        for (int i = 0; i < output_size; i++) {
            float z = model->biases[l][i];
            
            for (int j = 0; j < input_size; j++) {
                z += model->weights[l][i][j] * activations[l][j];
            }
            
            /* Apply activation function */
            if (l < model->num_layers - 2) {
                activations[l + 1][i] = relu(z);  /* Hidden layers use ReLU */
            } else {
                activations[l + 1][i] = sigmoid(z);  /* Output layer uses sigmoid */
            }
        }
    }
}

/* Backward pass */
static void backward_pass(mlp_model_t *model, float *target, 
                         float activations[MLP_MAX_LAYERS][MLP_MAX_NEURONS],
                         float gradients[MLP_MAX_LAYERS][MLP_MAX_NEURONS]) {
    int output_layer = model->num_layers - 1;
    int output_size = model->layer_sizes[output_layer];
    
    /* Compute output layer error */
    for (int i = 0; i < output_size; i++) {
        float output = activations[output_layer][i];
        gradients[output_layer][i] = (output - target[i]) * sigmoid_derivative(output);
    }
    
    /* Backpropagate through hidden layers */
    for (int l = output_layer - 1; l >= 1; l--) {
        int layer_size = model->layer_sizes[l];
        int next_size = model->layer_sizes[l + 1];
        
        for (int i = 0; i < layer_size; i++) {
            float error = 0.0f;
            
            for (int j = 0; j < next_size; j++) {
                error += model->weights[l][j][i] * gradients[l + 1][j];
            }
            
            gradients[l][i] = error * relu_derivative(activations[l][i]);
        }
    }
}

/* Train MLP using backpropagation */
float mlp_train(mlp_model_t *model, float **X, float **y, int n_samples, 
                float learning_rate, int epochs) {
    if (!model || !X || !y || n_samples < 1) return -1.0f;
    
    serial_puts("[mlp] Training neural network...\n");
    
    float loss = 0.0f;
    
    for (int epoch = 0; epoch < epochs; epoch++) {
        float epoch_loss = 0.0f;
        
        for (int s = 0; s < n_samples; s++) {
            float activations[MLP_MAX_LAYERS][MLP_MAX_NEURONS];
            float gradients[MLP_MAX_LAYERS][MLP_MAX_NEURONS];
            
            /* Forward pass */
            forward_pass(model, X[s], activations);
            
            /* Compute loss (MSE) */
            int output_size = model->layer_sizes[model->num_layers - 1];
            for (int i = 0; i < output_size; i++) {
                float error = activations[model->num_layers - 1][i] - y[s][i];
                epoch_loss += error * error;
            }
            
            /* Backward pass */
            backward_pass(model, y[s], activations, gradients);
            
            /* Update weights and biases */
            for (int l = 0; l < model->num_layers - 1; l++) {
                int input_size = model->layer_sizes[l];
                int output_size = model->layer_sizes[l + 1];
                
                for (int i = 0; i < output_size; i++) {
                    /* Update bias */
                    model->biases[l][i] -= learning_rate * gradients[l + 1][i];
                    
                    /* Update weights */
                    for (int j = 0; j < input_size; j++) {
                        model->weights[l][i][j] -= learning_rate * gradients[l + 1][i] * activations[l][j];
                    }
                }
            }
        }
        
        loss = epoch_loss / n_samples;
        
        /* Print progress every 100 epochs */
        if (epoch % 100 == 0) {
            serial_puts("[mlp] Epoch ");
            extern void serial_put_hex(uint64_t val);
            serial_put_hex(epoch);
            serial_puts(" loss=");
            serial_put_hex((uint64_t)(loss * 1000));
            serial_puts("\n");
        }
    }
    
    model->trained = 1;
    
    serial_puts("[mlp] Training complete\n");
    return loss;
}

/* Predict using MLP */
void mlp_predict(mlp_model_t *model, float *input, float *output) {
    if (!model || !input || !output || !model->trained) return;
    
    float activations[MLP_MAX_LAYERS][MLP_MAX_NEURONS];
    
    forward_pass(model, input, activations);
    
    /* Copy output layer to result */
    int output_layer = model->num_layers - 1;
    int output_size = model->layer_sizes[output_layer];
    
    for (int i = 0; i < output_size; i++) {
        output[i] = activations[output_layer][i];
    }
}

/* Evaluate MLP accuracy (for classification) */
float mlp_evaluate(mlp_model_t *model, float **X, float **y, int n_samples) {
    if (!model || !X || !y || !model->trained) return 0.0f;
    
    int correct = 0;
    float output[MLP_MAX_NEURONS];
    
    for (int i = 0; i < n_samples; i++) {
        mlp_predict(model, X[i], output);
        
        /* Find predicted class (argmax) */
        int predicted_class = 0;
        float max_val = output[0];
        
        int output_size = model->layer_sizes[model->num_layers - 1];
        for (int j = 1; j < output_size; j++) {
            if (output[j] > max_val) {
                max_val = output[j];
                predicted_class = j;
            }
        }
        
        /* Find true class */
        int true_class = 0;
        max_val = y[i][0];
        for (int j = 1; j < output_size; j++) {
            if (y[i][j] > max_val) {
                max_val = y[i][j];
                true_class = j;
            }
        }
        
        if (predicted_class == true_class) {
            correct++;
        }
    }
    
    return (float)correct / (float)n_samples;
}
