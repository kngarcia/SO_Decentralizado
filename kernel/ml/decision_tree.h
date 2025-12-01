/* kernel/ml/decision_tree.h */
#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#define DT_MAX_FEATURES 32
#define DT_MAX_SAMPLES 256
#define DT_MAX_NODES 512

typedef struct {
    int is_leaf;
    int feature_index;
    float threshold;
    int value;  /* Class label for leaf nodes */
    int left_child;
    int right_child;
    int samples;
} dt_node_t;

typedef struct {
    int num_features;
    int num_nodes;
    int max_depth;
    int min_samples_split;
    dt_node_t nodes[DT_MAX_NODES];
    int trained;
} dt_model_t;

/* Initialize decision tree */
void dt_init(dt_model_t *tree, int num_features);

/* Train decision tree */
int dt_train(dt_model_t *tree, float **X, int *y, int n_samples, 
             int max_depth, int min_samples_split);

/* Predict class */
int dt_predict(dt_model_t *tree, float *x);

/* Evaluate accuracy */
float dt_evaluate(dt_model_t *tree, float **X, int *y, int n_samples);

#endif /* DECISION_TREE_H */
