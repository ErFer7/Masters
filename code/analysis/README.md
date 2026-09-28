# epos-ann-scheduling-tools

## How to setup

1. Install poetry if you don't have it
2. Run `poetry install --no-root`. This will create the local venv and install
   the dependencies.
3. Run `source .venv/bin/activate`.

# How to use

### Parsing

Run `python3 parse.py <run.txt> <run_data.json>`

### Merging the run data into a single dataset

Run `python3 merge.py <run_data_directory> <dataset.json>`

### Fixing the utilization

Run `python3 fix_utilization.py <dataset.json> <fixed_dataset.json>`

### Plotting

Run `python3 plot.py <dataset.json>`

### Analyzing

Run `python3 select_features.py <dataset.json>`
