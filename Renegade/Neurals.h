#pragma once
#include "Board.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <memory>

// An early attempt at supporting NNUE evaluation.
// Not meant to be as a permanent solution in its current form.

// Network constants
constexpr int FeatureSize = 768;
constexpr int HiddenSize = 256;

struct alignas(64) NetworkRepresentation {
	std::array<std::array<int16_t, HiddenSize>, FeatureSize> FeatureWeights;
	std::array<int16_t, HiddenSize> FeatureBias;
	std::array<int16_t, HiddenSize * 2> OutputWeights;
	int16_t OutputBias;
};

extern const NetworkRepresentation* Network;

void LoadDefaultNetwork();
void LoadExternalNetwork(const std::string& filename);
int NeuralEvaluate(const Board &board);

inline int32_t ClippedReLU(const int16_t value) {
	return std::clamp<int32_t>(value, 0, 255);
}

inline int32_t SquareClippedReLU(const int16_t value) {
	const int32_t x = std::clamp<int32_t>(value, 0, 255);
	return x * x;
}

