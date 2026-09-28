#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "fann/build/include/fann.h"
#include "fann/build/include/fann_data.h"

using std::cerr;
using std::cout;
using std::endl;

const unsigned int SEED = 20260330;

const unsigned int NUM_INPUT = 16;
const unsigned int NUM_OUTPUT = 1;

constexpr unsigned int LAYERS[] = {NUM_INPUT, NUM_OUTPUT};
constexpr unsigned int NUM_LAYERS = sizeof(LAYERS) / sizeof(LAYERS[0]);

const float DESIRED_ERROR = 0.000002f;
const unsigned int MAX_EPOCHS = 1;
const unsigned int EPOCHS_BETWEEN_REPORTS = 100;

const float ACCURACY_ERROR = 0.02f;
const float OFFLINE_LEARNING_RATE = 0.05f;
const float ONLINE_LEARNING_RATE = 0.5f;

const unsigned int MAX_TRAININGS = 8;

void test_fann(fann *ann, fann_train_data *data, std::string label);

inline float calculated_error(float target, float actual);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <train_data> <test_data> <output_model> [core1_data core2_data ...]" << endl;
        return 1;
    }

    cout << "Setting seed as " << SEED << "..." << endl;
    fann_disable_seed_rand();
    srand(SEED);
    cout << "Done" << endl;

    cout << "Creating ANN... ";
    fann *ann = fann_create_standard_array(NUM_LAYERS, LAYERS);
    cout << "Done" << endl;

    cout << "Reading data... ";
    fann_train_data *train_data = fann_read_train_from_file(argv[1]);
    fann_train_data *test_data = fann_read_train_from_file(argv[2]);
    cout << "Done" << endl;

    cout << "Configuring the ANN... ";
    fann_set_training_algorithm(ann, FANN_TRAIN_INCREMENTAL);
    fann_set_learning_rate(ann, OFFLINE_LEARNING_RATE);
    fann_set_activation_function_hidden(ann, FANN_SIGMOID_STEPWISE);
    fann_set_activation_function_output(ann, FANN_LINEAR);
    fann_set_train_error_function(ann, FANN_ERRORFUNC_LINEAR);
    cout << "Done" << endl;

    cout << "ANN parameters:" << endl;
    fann_print_parameters(ann);

    cout << "Training model:" << endl;
    fann_train_on_data(ann, train_data, MAX_EPOCHS, EPOCHS_BETWEEN_REPORTS, DESIRED_ERROR);

    cout << "ANN connections:" << endl;
    fann_print_connections(ann);

    float mse_train = fann_test_data(ann, train_data);
    unsigned int bit_fail_train = fann_get_bit_fail(ann);
    float mse_test = fann_test_data(ann, test_data);
    unsigned int bit_fail_test = fann_get_bit_fail(ann);

    cout << "Train error: " << mse_train << ", Train bit-fail: " << bit_fail_train << ", Test error: " << mse_test
         << ", Test bit-fail: " << bit_fail_test << endl;

    cout << "Saving ANN... ";
    fann_save(ann, argv[3]);
    cout << "Done" << endl;

    cout << "Training error output:" << endl;
    test_fann(ann, train_data, "Training");

    cout << "Test error output:" << endl;
    test_fann(ann, test_data, "Test");

    fann_reset_MSE(ann);

    cout << "Retraining on core-specific datasets:" << endl;

    unsigned int num_core_datasets = argc - 4;
    unsigned int count = 0;
    unsigned int retrain_count = 0;

    for (unsigned int cpu = 0; cpu < num_core_datasets; cpu++) {
        const char *core_file = argv[4 + cpu];
        retrain_count = 0;

        cout << "Retraining Core: " << core_file << endl;

        cout << "Reloading ANN...";
        fann_destroy(ann);
        ann = fann_create_from_file(argv[3]);
        cout << "Done" << endl;

        cout << "Loading core data...";
        fann_train_data *core_data = fann_read_train_from_file(core_file);
        cout << "Done" << endl;

        fann_set_learning_rate(ann, ONLINE_LEARNING_RATE);

        unsigned int accuracy_error = 0;
        float acummulated_error = 0;
        unsigned int max = -1U;
        unsigned int min = 0;
        float runs = 0;
        float mispredicted_runs = 0;

        for (unsigned int i = 0; i < core_data->num_data; i++) {
            fann_type *output = fann_run(ann, core_data->input[i]);
            runs++;

            float error = calculated_error(core_data->output[i][0], output[0]);

            cout << "RUN: Output [" << i << "]: " << output[0] << ", expected: " << core_data->output[i][0]
                 << ", error: " << error << endl;

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

                    if (error > ACCURACY_ERROR) {
                        mispredicted_runs++;
                    }

                    count--;

                    cout << "TRN: Output [" << i << "]: " << output[0] << ", expected: " << core_data->output[i][0]
                         << ", error: " << error << endl;
                }

                if (count == 0) {
                    cout << "Failed to adapt for input " << i << " after " << MAX_TRAININGS << " attempts." << endl;
                }

                retrain_count++;
            }

            acummulated_error += error;

            if (error > max) max = error;
            if (error < min) min = error;
        }

        cout << "Accuracy after online adaptation (Core " << cpu + 1
             << "): " << (float)(core_data->num_data - accuracy_error) / core_data->num_data
             << ", error count: " << accuracy_error << ", retrains required: " << retrain_count
             << ", Real accuracy: " << ((runs - mispredicted_runs) / runs) << endl;

        // cout << "Saving ANN... ";
        // fann_save(ann, std::format("./models/ann_{}.net", cpu + 1).c_str());
        // cout << "Done" << endl;

        // cout << "Re-evaluating after retrain for " << core_file << ":" << endl;
        // test_fann(ann, core_data, std::format("Core {}", cpu + 1));

        fann_destroy_train(core_data);
    }

    cout << "\nCleaning up... ";
    fann_destroy_train(train_data);
    fann_destroy_train(test_data);
    fann_destroy(ann);
    cout << "Done" << endl;

    return 0;
}

void test_fann(fann *ann, fann_train_data *data, std::string label) {
    unsigned int accuracy_error = 0;
    float acummulated_error = 0;
    unsigned int max = -1U;
    unsigned int min = 0;

    for (unsigned int i = 0; i < data->num_data; i++) {
        fann_type *output = fann_run(ann, data->input[i]);
        float error = calculated_error(data->output[i][0], output[0]);

        cout << "Output [" << i << "]: " << output[0] << ", expected: " << data->output[i][0] << ", error: " << error
             << endl;

        if (error > ACCURACY_ERROR) accuracy_error++;

        acummulated_error += error;

        if (error > max) max = error;
        if (error < min) min = error;
    }

    cout << '[' << label << "] " << "Accuracy: " << (float)(data->num_data - accuracy_error) / data->num_data
         << ", error: " << accuracy_error << ", average: " << acummulated_error / data->num_data << endl;
}

inline float calculated_error(float target, float actual) {
    float error = target - actual;
    if (error < 0) error *= -1;

    return error;
}
