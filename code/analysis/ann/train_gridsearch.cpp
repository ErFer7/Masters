#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "fann/build/include/fann.h"
#include "fann/build/include/fann_data.h"

using std::cerr;
using std::cout;
using std::endl;

const unsigned int SEED = 20260330;
const unsigned int NUM_INPUT = 16;
const unsigned int NUM_OUTPUT = 1;

const float DESIRED_ERROR = 0.000002f;
const unsigned int EPOCHS_BETWEEN_REPORTS = 100;
const float ACCURACY_ERROR = 0.02f;
const unsigned int MAX_TRAININGS = 8;

inline float calculated_error(float target, float actual) {
    float error = target - actual;
    if (error < 0) error *= -1;

    return error;
}

int main(int argc, char *argv[]) {
    // Expected at least: binary + epochs + off_lr + on_lr + num_hidden + train + test + model = 8 args
    if (argc < 8) {
        cerr << "Usage: " << argv[0] << " <epochs> <offline_lr> <online_lr> <num_hidden> [hidden_1 ... hidden_N] "
             << "<train_data> <test_data> <output_model> [core1_data core2_data ...]" << endl;
        return 1;
    }

    // 1. Parse Hyperparameters
    int arg_idx = 1;
    unsigned int max_epochs = std::stoi(argv[arg_idx++]);
    float offline_lr = std::stof(argv[arg_idx++]);
    float online_lr = std::stof(argv[arg_idx++]);
    unsigned int num_hidden_layers = std::stoi(argv[arg_idx++]);

    std::vector<unsigned int> layers;
    layers.push_back(NUM_INPUT);
    for (unsigned int i = 0; i < num_hidden_layers; i++) {
        layers.push_back(std::stoi(argv[arg_idx++]));
    }
    layers.push_back(NUM_OUTPUT);

    // 2. Parse Files
    const char *train_file = argv[arg_idx++];
    const char *test_file = argv[arg_idx++];
    const char *output_model = argv[arg_idx++];

    int num_core_datasets = argc - arg_idx;
    int core_args_start = arg_idx;

    // --- Core FANN Setup ---
    fann_disable_seed_rand();
    srand(SEED);

    fann *ann = fann_create_standard_array(layers.size(), layers.data());
    fann_train_data *train_data = fann_read_train_from_file(train_file);
    fann_train_data *test_data = fann_read_train_from_file(test_file);

    fann_set_training_algorithm(ann, FANN_TRAIN_INCREMENTAL);

    // Apply OFFLINE learning rate
    fann_set_learning_rate(ann, offline_lr);

    fann_set_activation_function_hidden(ann, FANN_SIGMOID_STEPWISE);
    fann_set_activation_function_output(ann, FANN_LINEAR);
    fann_set_train_error_function(ann, FANN_ERRORFUNC_LINEAR);

    // fann_set_error_log(ann, NULL); // Uncomment to mute FANN outputs during grid search

    fann_train_on_data(ann, train_data, max_epochs, EPOCHS_BETWEEN_REPORTS, DESIRED_ERROR);
    fann_save(ann, output_model);

    fann_reset_MSE(ann);

    // --- Core Retraining / Evaluation ---
    for (int cpu = 0; cpu < num_core_datasets; cpu++) {
        const char *core_file = argv[core_args_start + cpu];

        fann_destroy(ann);
        ann = fann_create_from_file(output_model);
        fann_train_data *core_data = fann_read_train_from_file(core_file);

        // Apply ONLINE learning rate
        fann_set_learning_rate(ann, online_lr);

        unsigned int accuracy_error = 0;
        unsigned int count = 0;
        unsigned int retrain_count = 0;
        float runs = 0;
        float mispredicted_runs = 0;

        for (unsigned int i = 0; i < core_data->num_data; i++) {
            fann_type *output = fann_run(ann, core_data->input[i]);
            runs++;
            float error = calculated_error(core_data->output[i][0], output[0]);

            if (error > ACCURACY_ERROR) {
                accuracy_error++;
                mispredicted_runs++;
                count = MAX_TRAININGS;
                fann_reset_MSE(ann);

                while (error > ACCURACY_ERROR && count > 0) {
                    fann_train(ann, core_data->input[i], core_data->output[i]);
                    output = fann_run(ann, core_data->input[i]);
                    runs++;
                    error = calculated_error(core_data->output[i][0], output[0]);
                    if (error > ACCURACY_ERROR) mispredicted_runs++;
                    count--;
                }
                retrain_count++;
            }
        }

        cout << "Accuracy after online adaptation (Core " << cpu + 1 << "): " << std::fixed << std::setprecision(6)
             << (float)(core_data->num_data - accuracy_error) / core_data->num_data
             << ", error count: " << accuracy_error << ", retrains required: " << retrain_count
             << ", Real accuracy: " << ((runs - mispredicted_runs) / runs) << endl;

        fann_destroy_train(core_data);
    }

    fann_destroy_train(train_data);
    fann_destroy_train(test_data);
    fann_destroy(ann);

    return 0;
}
