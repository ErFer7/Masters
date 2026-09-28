all:
	g++ -std=c++23 -O3 train.cpp -o train -lm -Ifann/build/include -Lfann/build/lib -lfann -Wl,-rpath,fann/build/lib

clean:
	rm train
	rm train_gridsearch

gridsearch:
	g++ -std=c++23 -O3 train_gridsearch.cpp -o train_gridsearch -lm -Ifann/build/include -Lfann/build/lib -lfann -Wl,-rpath,fann/build/lib

.PHONY: train
train:
	./train ../data/datasets/ann/taskset-1-heavy.data ../data/datasets/ann/taskset-2-heavy.data ./models/ann.net ../data/datasets/ann/taskset-2-heavy_CORE_1.data ../data/datasets/ann/taskset-2-heavy_CORE_2.data ../data/datasets/ann/taskset-2-heavy_CORE_3.data | tee training_log.txt

.PHONY: train_non_improved
train_non_improved:
	./train ../data/datasets/ann/taskset-1-heavy-non-improved.data ../data/datasets/ann/taskset-2-heavy-non-improved.data ./models/ann_non_improved.net ../data/datasets/ann/taskset-2-heavy-non-improved_CORE_1.data ../data/datasets/ann/taskset-2-heavy-non-improved_CORE_2.data ../data/datasets/ann/taskset-2-heavy-non-improved_CORE_3.data | tee training_log_non_improved.txt

.PHONY: train_low_non_improved
train_low_non_improved:
	./train ../data/datasets/ann/taskset-5-heavy-non-improved.data ../data/datasets/ann/taskset-6-heavy-non-improved.data ./models/ann_low_non_improved.net ../data/datasets/ann/taskset-6-heavy-non-improved_CORE_1.data ../data/datasets/ann/taskset-6-heavy-non-improved_CORE_2.data ../data/datasets/ann/taskset-6-heavy-non-improved_CORE_3.data | tee training_log_low_non_improved.txt

.PHONY: train_low
train_low:
	./train ../data/datasets/ann/taskset-5-heavy.data ../data/datasets/ann/taskset-6-heavy.data ./models/ann_low.net ../data/datasets/ann/taskset-6-heavy_CORE_1.data ../data/datasets/ann/taskset-6-heavy_CORE_2.data ../data/datasets/ann/taskset-6-heavy_CORE_3.data | tee training_log_low.txt

.PHONY: train_new_feature_selection
train_new_feature_selection:
	./train ../data/datasets/ann/taskset-1-heavy-new-feature-selection.data ../data/datasets/ann/taskset-2-heavy-new-feature-selection.data ./models/ann_new_feature_selection.net ../data/datasets/ann/taskset-2-heavy-new-feature-selection-random_CORE_1.data ../data/datasets/ann/taskset-2-heavy-new-feature-selection-random_CORE_2.data ../data/datasets/ann/taskset-2-heavy-new-feature-selection-random_CORE_3.data | tee train_new_feature_selection.txt

.PHONY: train_new_feature_selection_with_info_gain
train_new_feature_selection_with_info_gain:
	./train ../data/datasets/ann/taskset-1-heavy-new-feature-selection-with-info-gain.data ../data/datasets/ann/taskset-2-heavy-new-feature-selection-with-info-gain.data ./models/ann_new_feature_selection_with_info_gain.net ../data/datasets/ann/taskset-2-heavy-new-feature-selection-with-info-gain_CORE_1.data ../data/datasets/ann/taskset-2-heavy-new-feature-selection-with-info-gain_CORE_2.data ../data/datasets/ann/taskset-2-heavy-new-feature-selection-with-info-gain_CORE_3.data | tee train_new_feature_selection_with_info_gain.txt

.PHONY: train_with_variance
train_with_variance:
	./train ../data/datasets/ann/taskset-1-with-variance.data ../data/datasets/ann/taskset-2-with-variance.data ./models/ann_with_variance.net ../data/datasets/ann/taskset-2-with-variance_CORE_1.data ../data/datasets/ann/taskset-2-with-variance_CORE_2.data ../data/datasets/ann/taskset-2-with-variance_CORE_3.data | tee train_with_variance.txt

.PHONY: train_reworked
train_reworked:
	./train ../data/datasets/ann/taskset-1-reworked.data ../data/datasets/ann/taskset-2-reworked.data ./models/ann_reworked.net ../data/datasets/ann/taskset-2-reworked_CORE_1.data ../data/datasets/ann/taskset-2-reworked_CORE_2.data ../data/datasets/ann/taskset-2-reworked_CORE_3.data | tee train_reworked.txt

.ONESHELL:
.PHONY: fann
fann:
	cd fann
	cmake -D CMAKE_INSTALL_PREFIX:PATH=build .
	make install

.ONESHEEL:
.PHONY: cleanfann
.cleanfann:
	cd fann
	make clean
