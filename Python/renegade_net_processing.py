import numpy as np

INPUT_BUCKETS = 4

FEATURE_SIZE = 768
L1_SIZE = 1408
L2_SIZE = 8
L3_SIZE = 16
OUTPUT_SIZE = 1

QA = 255
QB = 64

NET_PATH = "renegade-pw-mr-unquant.bin"


if __name__ == '__main__':
    data = np.fromfile(NET_PATH, dtype=np.float32)
    print(len(data))

    # 1. Load everything in floats --------------------------------------------

    feature_weights_end = INPUT_BUCKETS * FEATURE_SIZE * L1_SIZE
    feature_biases_end = feature_weights_end + L1_SIZE
    feature_weights = data[:feature_weights_end]
    feature_biases = data[feature_weights_end:feature_biases_end]

    l1_weights_end = feature_biases_end + L1_SIZE * L2_SIZE
    l1_biases_end = l1_weights_end + L2_SIZE
    l1_weights = data[feature_biases_end:l1_weights_end]
    l1_biases = data[l1_weights_end:l1_biases_end]
    print(l1_weights.nbytes)
    print(l1_biases.nbytes)
    print()

    l2_weights_end = l1_biases_end + L2_SIZE * L3_SIZE
    l2_biases_end = l2_weights_end + L3_SIZE
    l2_weights = data[l1_biases_end:l2_weights_end]
    l2_biases = data[l2_weights_end:l2_biases_end]
    print(l2_weights.nbytes)
    print(l2_biases.nbytes)

    l3_weights_end = l2_biases_end + L3_SIZE * 1
    l3_biases_end = l3_weights_end + 1
    l3_weights = data[l2_biases_end:l3_weights_end]
    l3_bias = data[l3_weights_end:l3_biases_end]
    print(l3_weights.nbytes)
    print(l3_bias.nbytes)


    rest = data[l3_biases_end:]
    assert(len(rest) == 0)

    # 2. Partial quantization -------------------------------------------------

    feature_weights_q = (feature_weights * QA).astype(np.int16)
    feature_biases_q = (feature_biases * QA).astype(np.int16)
    l1_weights_q = (l1_weights * QB).astype(np.int16)

    # 3. Putting back together ------------------------------------------------

    quantized_part = np.array([], dtype=np.int16)
    unquantized_part = np.array([], dtype=np.float32)

    quantized_part = np.append(quantized_part, feature_weights_q.flatten())
    quantized_part = np.append(quantized_part, feature_biases_q.flatten())
    quantized_part = np.append(quantized_part, l1_weights_q.flatten())

    unquantized_part = np.append(unquantized_part, l1_biases.flatten())
    unquantized_part = np.append(unquantized_part, l2_weights.flatten())
    unquantized_part = np.append(unquantized_part, l2_biases.flatten())
    unquantized_part = np.append(unquantized_part, l3_weights.flatten())
    unquantized_part = np.append(unquantized_part, l3_bias.flatten())

    # 4. Save file -------------------------------------------------------------

    saved = bytearray([])
    saved.extend(quantized_part.tobytes())
    saved.extend(unquantized_part.tobytes())
    print("Saved size:", len(saved))

    with open(NET_PATH + "_processed2", 'wb') as f:
        f.write(saved)
    print("Complete.")
