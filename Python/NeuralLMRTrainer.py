import math

import keras
import keras.models
import pandas as pd
import random
import numpy as np

from keras.applications.densenet import layers


def load_trained_model():
    loaded_model = keras.models.load_model("last.keras")

    input1 = np.full(shape=(1, 8), fill_value=0.0, dtype=float)
    input2 = np.full(shape=(1, 8), fill_value=0.5, dtype=float)
    input3 = np.full(shape=(1, 8), fill_value=1.0, dtype=float)
    input4 = np.full(shape=(1, 8), fill_value=1.5, dtype=float)
    print(loaded_model.predict(input1))
    print(loaded_model.predict(input2))
    print(loaded_model.predict(input3))
    print(loaded_model.predict(input4))


if __name__ == '__main__':

    # 1. Load and shuffle data
    seed = 2024
    df = pd.read_csv("lmr_dump.csv")
    df = df.sample(frac=1, random_state=seed).reset_index(drop=True)

    # 2. Normalize
    def ln_norm(x): return math.log(x)
    def sigmoid_norm(x): return 1 / (1 + math.e**(-x))

    df['sucRedAmount'] = df['sucRedAmount'].apply(lambda x: ln_norm(x) if x != 0 else 0)
    df['moveCount'] = df['moveCount'].apply(lambda x: ln_norm(x))
    df['depth'] = df['depth'].apply(lambda x: ln_norm(x))
    df['ttPV'] = df['ttPV']
    df['pvNode'] = df['pvNode']
    df['cutNode'] = df['cutNode']
    df['historyScore'] = df['historyScore'].apply(lambda x: sigmoid_norm(x / 20000))
    df['evalDiff1'] = df['evalDiff1'].apply(lambda x: sigmoid_norm(x / 32))
    df['evalDiff2'] = df['evalDiff2'].apply(lambda x: sigmoid_norm(x / 128))

    # 3. Split
    TRAIN_SPLIT = 0.8
    split_index = int(len(df) * TRAIN_SPLIT)
    train = df[:split_index]
    test = df[split_index:]

    output_key = ["sucRedAmount"]
    input_keys = ["depth", "moveCount", "pvNode", "ttPV", "cutNode", "historyScore", "evalDiff1", "evalDiff2"]

    x_train = train[input_keys]
    y_train = train[output_key]
    x_test = test[input_keys]
    y_test = test[output_key]

    # 4. Train

    model = keras.Sequential([
        keras.Input(shape=len(input_keys)),
        layers.Dense(16, activation="relu"),
        layers.Dense(8, activation="relu"),
        layers.Dense(1),
    ])

    model.summary()

    batch_size = 128
    epochs = 3

    model.compile(loss="mse", optimizer="adam")
    model.fit(x_train, y_train, validation_data=(x_test, y_test), batch_size=batch_size, epochs=epochs, validation_split=0.1)
    model.save("last.keras")

    saved = bytearray()

    for layer_index, layer in enumerate(model.layers):
        print()
        print()

        if layer_index == 0:
            w_name = "net.InputWeights"
            b_name = "net.InputBiases"
        elif layer_index == 1:
            w_name = "net.L1Weights"
            b_name = "net.L1Biases"
        else:
            w_name = "net.L2Weights"
            b_name = "net.L2Bias"

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

    with open("last_binary.bin", "wb") as f:
        f.write(saved)

    load_trained_model()

    # 5. Quantize