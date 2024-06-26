#pragma once
#include "Position.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <immintrin.h>
#include <iterator>
#include <memory>

// This is the code for the NNUE evaluation
// Renegade uses a simple, unbucketed perspective net

// The engine's neural network is trained purely on self-play
// a king tropism-only evaluation was the starting point:
// for each piece: score += (15 - (manhattan distance to opponent's king)) * 6

// Network constants
#ifndef NETWORK_NAME
#define NETWORK_NAME "renegade-net-21.bin"
#endif

constexpr int FeatureSize = 768;
constexpr int HiddenSize = 1024;
constexpr int Scale = 400;
constexpr int QA = 255;
constexpr int QB = 64;

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

	void AddFeature(const std::pair<int, int>& features) {
		for (int i = 0; i < HiddenSize; i++) White[i] += Network->FeatureWeights[features.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] += Network->FeatureWeights[features.second][i];
	}

	void RemoveFeature(const std::pair<int, int>& features) {
		for (int i = 0; i < HiddenSize; i++) White[i] -= Network->FeatureWeights[features.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] -= Network->FeatureWeights[features.second][i];
	}

};

void LoadDefaultNetwork();
void LoadExternalNetwork(const std::string& filename);

int NeuralEvaluate(const Position &position);
int NeuralEvaluate(const AccumulatorRepresentation& acc, const bool turn);

inline std::pair<int, int> FeatureIndexes(const uint8_t piece, const uint8_t sq) {
	const uint8_t pieceColor = ColorOfPiece(piece);
	const uint8_t pieceType = TypeOfPiece(piece);
	constexpr int colorOffset = 64 * 6;

	const int whiteFeatureIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + sq;
	const int blackFeatureIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq);
	return { whiteFeatureIndex, blackFeatureIndex };
}