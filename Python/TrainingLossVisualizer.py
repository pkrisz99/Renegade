"""
 A small script for plotting and comparing running loss
 - copy bullet's console output into the directory specified below
 - irrelevant lines are discarded
 - note that training loss is usually not an indicator of strength
"""

import matplotlib.pyplot as plt
from os import listdir
from os.path import isfile, join

folder_name = r"training_logs"

if __name__ == '__main__':

    runs = {}

    # Get the list of files to process
    files_list = [f for f in listdir(folder_name) if isfile(join(folder_name, f))]

    # Read files, save losses
    for filename in files_list:
        loss, superbatches = [], []
        with open(folder_name + "/" + filename) as file:
            for line in file:
                line = line.strip()
                if not line.startswith("superbatch "):
                    continue
                words = line.split()
                loss.append(float(words[8]))
                superbatches.append(int(words[1]))
        runs[filename] = superbatches, loss

    # Visualize with matplotlib
    for filename, run in runs.items():
        print(filename)
        plt.plot(*run)
    plt.grid()
    plt.title("Running loss during training")
    plt.xlabel("Superbatch number")
    plt.ylabel("Running loss")
    plt.legend([filename for filename, run in runs.items()])
    plt.show()
