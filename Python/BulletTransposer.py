import numpy as np

# Modify these values as needed
FILE_PATH = "renegade-net-24.bin"
FEATURE_SIZE = 768
HIDDEN_SIZE = 1024
INPUT_BUCKETS = 4
OUTPUT_BUCKETS = 1

"""
 Default bullet network representation:
  - feature weights: InputBucketCount * FeatureSize * HiddenSize
  - feature bias: HiddenSize
  - output weights: (HiddenSize * 2) * OutputBucketCount
  - output bias: OutputBucketCount
"""


class Transposer:

    def __init__(self):
        self.data = None
        self.feature_weights = None
        self.feature_bias = None
        self.output_weights = None
        self.output_bias = None
        self.padding = None

    def load_bullet_net(self):
        self.data = np.fromfile(FILE_PATH, dtype=np.int16)

        features_weights_end = FEATURE_SIZE * HIDDEN_SIZE * INPUT_BUCKETS
        self.feature_weights = self.data[0:features_weights_end].reshape((INPUT_BUCKETS, FEATURE_SIZE, HIDDEN_SIZE))

        feature_bias_end = features_weights_end + HIDDEN_SIZE
        self.feature_bias = self.data[features_weights_end:feature_bias_end]

        output_weights_end = feature_bias_end + HIDDEN_SIZE * OUTPUT_BUCKETS * 2
        self.output_weights = self.data[feature_bias_end:output_weights_end].reshape((HIDDEN_SIZE * 2, OUTPUT_BUCKETS))

        output_bias_end = output_weights_end + OUTPUT_BUCKETS
        self.output_bias = self.data[output_weights_end:output_bias_end]

        self.padding = self.data[output_bias_end:]

    def display_summary(self):
        print("------------------------------------------")
        print("Data length (i16):     ", self.data.shape[0])
        print("------------------------------------------")
        print("Feature weights shape: ", self.feature_weights.shape)
        print("Feature bias shape:    ", self.feature_bias.shape)
        print("Output weights shape:  ", self.output_weights.shape)
        print("Output bias shape:     ", self.output_bias.shape)
        print("Padding shape:         ", self.padding.shape)
        print("------------------------------------------\n")

    def transpose_output_buckets(self):
        print("Old shape:", self.output_weights.shape)
        self.output_weights = self.output_weights.transpose()
        print("New shape:", self.output_weights.shape, "\n")

    def display_padding(self):
        padding_ascii = self.padding.view(np.int8)
        print("Padding (n=%d): '" % len(padding_ascii), end='')
        [print(chr(x), end='') for x in padding_ascii]
        print("'\n")

    def save_modified(self):
        saved = np.array([], dtype=np.int16)
        saved = np.append(saved, self.feature_weights.flatten())
        saved = np.append(saved, self.feature_bias.flatten())
        saved = np.append(saved, self.output_weights.flatten())
        saved = np.append(saved, self.output_bias.flatten())
        saved = np.append(saved, self.padding.flatten())
        saved.tofile(FILE_PATH + "_modified")

    @staticmethod
    def changed_values():
        data1 = np.fromfile(FILE_PATH, dtype=np.int16)
        data2 = np.fromfile(FILE_PATH + "_modified", dtype=np.int16)
        result = data1 == data2
        unique, counts = np.unique(result, return_counts=True)
        print(unique)
        print(counts)
        print()


if __name__ == '__main__':
    tp = Transposer()
    tp.load_bullet_net()
    tp.display_summary()
    tp.display_padding()
    tp.transpose_output_buckets()
    tp.save_modified()
