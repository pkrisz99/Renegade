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

struct alignas(64) AccumulatorRepresentation {
	std::array<int16_t, HiddenSize> White;
	std::array<int16_t, HiddenSize> Black;

	void Reset() {
		for (int i = 0; i < HiddenSize; i++) White[i] = Network->FeatureBias[i];
		for (int i = 0; i < HiddenSize; i++) Black[i] = Network->FeatureBias[i];
	}

	void AddFeature(const int whiteFeature, const int blackFeature) {
		for (int i = 0; i < HiddenSize; i++) White[i] += Network->FeatureWeights[whiteFeature][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] += Network->FeatureWeights[blackFeature][i];
	}

	void RemoveFeature(const int whiteFeature, const int blackFeature) {
		for (int i = 0; i < HiddenSize; i++) White[i] -= Network->FeatureWeights[whiteFeature][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] -= Network->FeatureWeights[blackFeature][i];
	}

	template<bool add> void UpdateFeature(const int whiteFeature, const int blackFeature) {
		if constexpr (add) AddFeature(whiteFeature, blackFeature);
		else RemoveFeature(whiteFeature, blackFeature);
	}

	template<bool add> void UpdateFeature(const std::pair<int, int>& features) {
		if constexpr (add) AddFeature(features.first, features.second);
		else RemoveFeature(features.first, features.second);
	}
};

void LoadDefaultNetwork();
void LoadExternalNetwork(const std::string& filename);
int NeuralEvaluate(const Board &board);
int NeuralEvaluate2(const AccumulatorRepresentation& acc, const bool turn);

inline int32_t ClippedReLU(const int16_t value) {
	return std::clamp<int32_t>(value, 0, 255);
}

inline int32_t SquareClippedReLU(const int16_t value) {
	const int32_t x = std::clamp<int32_t>(value, 0, 255);
	return x * x;
}

inline std::pair<int, int> FeatureIndexes(const uint8_t piece, const uint8_t sq) {
	const uint8_t pieceColor = ColorOfPiece(piece);
	const uint8_t pieceType = TypeOfPiece(piece);
	const int colorOffset = 64 * 6;

	const int whiteFeatureIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + sq;
	const int blackFeatureIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq);
	return { whiteFeatureIndex, blackFeatureIndex };
}