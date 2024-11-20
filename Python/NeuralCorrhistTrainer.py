import math

import keras
import keras.models
import pandas as pd
import random
import numpy as np

from keras.applications.densenet import layers


def load_trained_model():
    loaded_model = keras.models.load_model("last.keras")
    length = len(input_keys)
    input1 = np.full(shape=(1, length), fill_value=0.0, dtype=float)
    input2 = np.full(shape=(1, length), fill_value=0.5, dtype=float)
    input3 = np.full(shape=(1, length), fill_value=1.0, dtype=float)
    input4 = np.full(shape=(1, length), fill_value=1.5, dtype=float)
    print(loaded_model.predict(input1))
    print(loaded_model.predict(input2))
    print(loaded_model.predict(input3))
    print(loaded_model.predict(input4))


if __name__ == '__main__':

    # 1. Load and shuffle data
    seed = 2024
    df = pd.read_csv("training_dump.csv")
    df = df.sample(frac=1, random_state=seed).reset_index(drop=True)

    # 2. Normalize

    def clamp(x, x_min, x_max):
        return min(x_max, max(x_min, x))

    def sigmoid_norm(x):
        x = clamp(x, -20.0, 20.0)
        return 1 / (1 + math.e**(-x))

    # transform output:
    df['correctionAmount'] = df['correctionAmount'].apply(lambda x: sigmoid_norm(x / 16))
    # transform inputs:
    df['rawEval'] = df['rawEval'].apply(lambda x: sigmoid_norm(x / 256))
    df['depth'] = df['depth'].apply(lambda x: clamp(x, 1, 10) / 10)
    df['pawnHistory'] = df['pawnHistory'].apply(lambda x: x / 16)
    df['materialHistory'] = df['materialHistory'].apply(lambda x: x / 16)
    df['followUpHistory'] = df['followUpHistory'].apply(lambda x: x / 16)

    # define used ones:
    output_key = ["correctionAmount"]
    input_keys = ["rawEval", "pvNode", "pawnHistory", "materialHistory", "followUpHistory"]

    print(df)

    # 3. Split
    TRAIN_SPLIT = 0.8
    split_index = int(len(df) * TRAIN_SPLIT)
    train = df[:split_index]
    test = df[split_index:]

    x_train = train[input_keys]
    y_train = train[output_key]
    x_test = test[input_keys]
    y_test = test[output_key]

    # 4. Train

    model = keras.Sequential([
        keras.Input(shape=len(input_keys)),
        layers.Dense(8, activation="relu"),
        layers.Dense(4, activation="relu"),
        layers.Dense(1),
    ])

    model.summary()

    batch_size = 256
    epochs = 2

    model.compile(loss="mse", optimizer="adam")
    model.fit(x_train, y_train, validation_data=(x_test, y_test), batch_size=batch_size, epochs=epochs, validation_split=0.1)
    model.save("last.keras")

    saved = bytearray()

    for layer_index, layer in enumerate(model.layers):
        print()

        if layer_index == 0:
            w_name = "InputWeights"
            b_name = "InputBiases"
        elif layer_index == 1:
            w_name = "L1Weights"
            b_name = "L1Biases"
        else:
            w_name = "L2Weights"
            b_name = "L2Bias"

        saved.extend(layer.get_weights()[0])

        for i, x in enumerate(layer.get_weights()[0]):
            print("%s[%2d] = { " % (w_name, i), end='')
            for j, y in enumerate(x):
                print(y, end=', ' if j != len(x) - 1 else '')
            print("};")

        print("%s = { " % b_name, end='')
        for i, x in enumerate(layer.get_weights()[1]):
            print(x, end=', ' if i != len(layer.get_weights()[1]) - 1 else '')
        print("};")

        saved.extend(layer.get_weights()[1])

    print()

    with open("last_binary.bin", "wb") as f:
        f.write(saved)

    load_trained_model()

    # 5. Quantize