/* kernel/ml/decision_tree.c
 * Decision tree implementation using CART algorithm
 */

#include "decision_tree.h"
#include <stdint.h>
#include <stddef.h>

extern void *memset(void *s, int c, size_t n);
extern void serial_puts(const char *s);

/* Calculate Gini impurity */
static float gini_impurity(int *labels, int n_samples) {
    if (n_samples == 0) return 0.0f;
    
    int count_0 = 0, count_1 = 0;
    
    for (int i = 0; i < n_samples; i++) {
        if (labels[i] == 0) count_0++;
        else count_1++;
    }
    
    float p0 = (float)count_0 / n_samples;
    float p1 = (float)count_1 / n_samples;
    
    return 1.0f - (p0 * p0 + p1 * p1);
}

/* Find best split */
static void find_best_split(float **X, int *y, int n_samples, int n_features,
                           int *best_feature, float *best_threshold, float *best_gain) {
    *best_gain = -1.0f;
    *best_feature = 0;
    *best_threshold = 0.0f;
    
    float parent_gini = gini_impurity(y, n_samples);
    
    /* Try each feature */
    for (int feature = 0; feature < n_features; feature++) {
        /* Try different thresholds (simplified: use midpoints) */
        for (int i = 0; i < n_samples - 1; i++) {
            float threshold = (X[i][feature] + X[i + 1][feature]) / 2.0f;
            
            /* Split samples */
            int left_count = 0, right_count = 0;
            int left_labels[DT_MAX_SAMPLES];
            int right_labels[DT_MAX_SAMPLES];
            
            for (int j = 0; j < n_samples; j++) {
                if (X[j][feature] <= threshold) {
                    left_labels[left_count++] = y[j];
                } else {
                    right_labels[right_count++] = y[j];
                }
            }
            
            if (left_count == 0 || right_count == 0) continue;
            
            /* Calculate weighted Gini */
            float left_gini = gini_impurity(left_labels, left_count);
            float right_gini = gini_impurity(right_labels, right_count);
            
            float weighted_gini = (left_count * left_gini + right_count * right_gini) / n_samples;
            float gain = parent_gini - weighted_gini;
            
            if (gain > *best_gain) {
                *best_gain = gain;
                *best_feature = feature;
                *best_threshold = threshold;
            }
        }
    }
}

/* Get majority class */
static int majority_class(int *labels, int n_samples) {
    int count_0 = 0, count_1 = 0;
    
    for (int i = 0; i < n_samples; i++) {
        if (labels[i] == 0) count_0++;
        else count_1++;
    }
    
    return (count_0 > count_1) ? 0 : 1;
}

/* Recursive tree building */
static int build_tree_recursive(dt_model_t *tree, int node_idx, float **X, int *y, 
                               int n_samples, int depth, int max_depth, int min_samples_split) {
    if (node_idx >= DT_MAX_NODES) return -1;
    
    dt_node_t *node = &tree->nodes[node_idx];
    node->is_leaf = 0;
    node->value = 0;
    
    /* Check stopping criteria */
    if (depth >= max_depth || n_samples < min_samples_split) {
        node->is_leaf = 1;
        node->value = majority_class(y, n_samples);
        node->samples = n_samples;
        return 0;
    }
    
    /* Find best split */
    int best_feature;
    float best_threshold, best_gain;
    find_best_split(X, y, n_samples, tree->num_features, 
                   &best_feature, &best_threshold, &best_gain);
    
    /* If no good split found, make leaf */
    if (best_gain < 0.01f) {
        node->is_leaf = 1;
        node->value = majority_class(y, n_samples);
        node->samples = n_samples;
        return 0;
    }
    
    /* Create internal node */
    node->feature_index = best_feature;
    node->threshold = best_threshold;
    node->samples = n_samples;
    
    /* Split data */
    float *left_X[DT_MAX_SAMPLES];
    float *right_X[DT_MAX_SAMPLES];
    int left_y[DT_MAX_SAMPLES];
    int right_y[DT_MAX_SAMPLES];
    int left_count = 0, right_count = 0;
    
    for (int i = 0; i < n_samples; i++) {
        if (X[i][best_feature] <= best_threshold) {
            left_X[left_count] = X[i];
            left_y[left_count] = y[i];
            left_count++;
        } else {
            right_X[right_count] = X[i];
            right_y[right_count] = y[i];
            right_count++;
        }
    }
    
    /* Recursively build left and right subtrees */
    int left_child = tree->num_nodes++;
    int right_child = tree->num_nodes++;
    
    node->left_child = left_child;
    node->right_child = right_child;
    
    build_tree_recursive(tree, left_child, left_X, left_y, left_count, depth + 1, max_depth, min_samples_split);
    build_tree_recursive(tree, right_child, right_X, right_y, right_count, depth + 1, max_depth, min_samples_split);
    
    return 0;
}

/* Initialize decision tree */
void dt_init(dt_model_t *tree, int num_features) {
    if (!tree || num_features < 1 || num_features > DT_MAX_FEATURES) return;
    
    memset(tree, 0, sizeof(dt_model_t));
    tree->num_features = num_features;
    tree->num_nodes = 0;
    tree->trained = 0;
}

/* Train decision tree */
int dt_train(dt_model_t *tree, float **X, int *y, int n_samples, int max_depth, int min_samples_split) {
    if (!tree || !X || !y || n_samples < 1) return -1;
    
    serial_puts("[dt] Training decision tree...\n");
    
    tree->num_nodes = 1;  /* Start with root */
    tree->max_depth = max_depth;
    tree->min_samples_split = min_samples_split;
    
    build_tree_recursive(tree, 0, X, y, n_samples, 0, max_depth, min_samples_split);
    
    tree->trained = 1;
    
    serial_puts("[dt] Training complete. Nodes: ");
    extern void serial_put_hex(uint64_t val);
    serial_put_hex(tree->num_nodes);
    serial_puts("\n");
    
    return 0;
}

/* Predict using decision tree */
int dt_predict(dt_model_t *tree, float *x) {
    if (!tree || !x || !tree->trained) return 0;
    
    int node_idx = 0;  /* Start at root */
    
    while (1) {
        dt_node_t *node = &tree->nodes[node_idx];
        
        if (node->is_leaf) {
            return node->value;
        }
        
        /* Navigate tree */
        if (x[node->feature_index] <= node->threshold) {
            node_idx = node->left_child;
        } else {
            node_idx = node->right_child;
        }
    }
    
    return 0;
}

/* Evaluate accuracy */
float dt_evaluate(dt_model_t *tree, float **X, int *y, int n_samples) {
    if (!tree || !X || !y || !tree->trained) return 0.0f;
    
    int correct = 0;
    for (int i = 0; i < n_samples; i++) {
        int predicted = dt_predict(tree, X[i]);
        if (predicted == y[i]) {
            correct++;
        }
    }
    
    return (float)correct / (float)n_samples;
}
