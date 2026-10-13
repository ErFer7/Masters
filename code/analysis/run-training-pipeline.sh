DATASETS_DIR=~/Code/UFSC/Masters/INE410145_D/Masters/masters-dataset/periodic/tc_2026/processed_datasets/
RESULTS_DIR=~/Code/UFSC/Masters/INE410145_D/Masters/results/training/tc_2026/

SELECTED_TASKSET=tc_taskset_4
SELECTED_DATASET=2026-10-03-12-26-experimental-tc-taskset-4-monitoring-50hz-150s-simplified-jobs-virtual-no-idle-forecast.csv
# SELECTED_DATASET=2026-10-07-00-11-experimental-tc-taskset-5-monitoring-50hz-150s-simplified-jobs-virtual-no-idle-forecast.csv

RESULTS_PATH=$RESULTS_DIR/$SELECTED_TASKSET
mkdir -p $RESULTS_PATH

DATASET_PATH=$DATASETS_DIR/$SELECTED_DATASET

python ./scripts/train_linear.py --counters L1_CACHE_MISS \
    BRANCH_JUMP_TARGET_MISPREDICTION \
    --l2-counters GLOBAL_L2_INNER_GET_HIT_L2 GLOBAL_L2_INNER_GET \
    GLOBAL_L2_DEMAND_MISS_HIT_MSHR_ALLOC_HINT \
    GLOBAL_L2_OUTER_ACQUIREBLOCK_NTOB_L2_MISS \
    GLOBAL_L2_INNER_PROBEBLOCK_TON_STORE_MISS \
    GLOBAL_L2_OUTER_RELEASE_TON_EVICTION \
    --pairwise-ratios \
    --protocols temporal \
    --n-folds 2 \
    $RESULTS_PATH \
    $DATASET_PATH
