#pragma once

namespace Audiobeam {
struct audiobeam_DataQueue {
    float **sample_queue;
    int head;
    int tail;
    unsigned char full;
};

struct audiobeam_Delays {
    float **delay_values;
    long int max_delay;
};

struct audiobeam_PreprocessedDelays {
    float delay;
    int low;
    int high;
    float offset;
};

#undef FLT_MAX
#define FLT_MAX 999e999

#define SOUND_SPEED 342
static const int SAMPLING_RATE = 16000;
#define CARTESIAN_DISTANCE(x1, y1, z1, x2, y2, z2) \
    (sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2)));

#define NUM_MIC 15
#define ANGLE_ENERGY_WINDOW_SIZE 400
#define GRID_STEP_SIZE 0.003  // .3cm
#define NUM_DIRS 7
#define NUM_TILES 16

#define MIC_HORIZ_SPACE 0.038257
#define MIC_VERT_SPACE 0.015001
#define TWO23 8388608.0  // 2^23
#define BUFFER_SIZE 384  // No of input-tupels (each with NUM_MIC elements)
#define NUM_MIC_IN_CHAIN 32
#define NUM_BOARDS_IN_CHAIN 16
#define INPUT_LENGTH 5760

#define INTERPOLATE(low_value, high_value, offset) (((high_value - low_value) * (offset)) + low_value)

class Audiobeam {
   private:
    static const unsigned int AUDIOBEAM_HEAP_SIZE = 10000;

   public:
    Audiobeam();

    ~Audiobeam() { delete[] audiobeam_input; }

    int run();

   private:
    inline void reset() {
        audiobeam_freeHeapPos = 0;
        audiobeam_input_pos = 0;
        audiobeam_checksum = 0;
    }

    void audiobeam_preprocess_delays(struct audiobeam_PreprocessedDelays prep_delays[], float *delays);
    float *audiobeam_parse_line(float *float_arr, int num_mic);
    long int audiobeam_find_max_in_arr(float *arr, int size);
    long int audiobeam_find_min_in_arr(float *arr, int size);
    int audiobeam_wrapped_inc_offset(int i, int offset, int max_i);
    int audiobeam_wrapped_dec_offset(int i, int offset, int max_i);
    int audiobeam_wrapped_inc(int i, int max_i);
    int audiobeam_wrapped_dec(int i, int max_i);
    struct audiobeam_DataQueue *audiobeam_init_data_queue(int max_delay, int num_mic);
    struct audiobeam_Delays *audiobeam_init_delays(int num_angles, int num_mic);
    void audiobeam_calc_distances(float *source_location,
                                  float audiobeam_mic_locations[15][3],
                                  float *distances,
                                  int num_mic);
    void audiobeam_calc_delays(float *distances, float *delays, int sound_speed, int sampling_rate, int num_mic);
    void audiobeam_adjust_delays(float *delays, int num_mic);
    float *audiobeam_calc_weights_lr(int num_mic);
    float *audiobeam_calc_weights_left_only(int num_mic);
    float audiobeam_calculate_energy(float *samples, int num_samples);
    float audiobeam_do_beamforming(struct audiobeam_PreprocessedDelays preprocessed_delays[],
                                   float **sample_queue,
                                   int queue_head,
                                   long int max_delay,
                                   int num_mic,
                                   float *weights);
    int audiobeam_process_signal(struct audiobeam_Delays *delays,
                                 int num_mic,
                                 float sampling_rate,
                                 float **beamform_results,
                                 struct audiobeam_DataQueue *queue,
                                 int num_beams,
                                 int window,
                                 float *weights);
    int audiobeam_calc_beamforming_result(struct audiobeam_Delays *delays,
                                          float **beamform_results,
                                          float *energies,
                                          struct audiobeam_DataQueue *queue,
                                          int num_beams,
                                          int window,
                                          int hamming);
    void audiobeam_calc_single_pos(float source_location[3], float audiobeam_mic_locations[15][3], int hamming);

   private:
    void *audiobeam_malloc(unsigned int numberOfBytes) {
        void *currentPos = (void *)&audiobeam_simulated_heap[audiobeam_freeHeapPos];
        /* Get a 4-byte address for alignment purposes */
        audiobeam_freeHeapPos += ((numberOfBytes + 4) & (unsigned int)0xfffffffc);
        return currentPos;
    }

   private:
    float *audiobeam_input;
    float audiobeam_mic_locations[15][3];
    float audiobeam_source_location[3];
    float audiobeam_origin_location[3];
    int audiobeam_input_pos;
    int audiobeam_checksum;
    unsigned int audiobeam_freeHeapPos;
    char audiobeam_simulated_heap[AUDIOBEAM_HEAP_SIZE];  // TODO: This should be in the heap
    static const float audiobeam_input_data[5760];
    static const float audiobeam_mic_locations_data[15][3];
    static const float audiobeam_source_location_data[3];
    static const float audiobeam_origin_location_data[3];
    static const unsigned int audiobeam_mic_locations_data_size;
    static const unsigned int audiobeam_input_data_size;
    static const unsigned int audiobeam_source_location_data_size;
    static const unsigned int audiobeam_origin_location_data_size;
};
}  // namespace Audiobeam
