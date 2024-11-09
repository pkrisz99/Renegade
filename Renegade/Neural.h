#pragma once
#include "Position.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <immintrin.h>
#include <iterator>
#include <memory>

// This is the code for the NNUE evaluation
// Renegade uses a horizontally mirrored perspective net with input buckets based on the king's position

// The engine's neural network is trained purely on self-play
// a king tropism-only evaluation was the starting point:
// for each piece: score += (15 - (manhattan distance to opponent's king)) * 6

// Network constants
#ifndef NETWORK_NAME
#define NETWORK_NAME "raw.bin"
#endif

constexpr int FeatureSize = 768;
constexpr int L1Size = 1408;
constexpr int L2Size = 8;
constexpr int L3Size = 16;

constexpr int Scale = 400;
constexpr int QA = 255;
constexpr int QB = 64;

constexpr int InputBucketCount = 4;
constexpr std::array<int, 32> InputBucketMap = {
	0, 0, 1, 1,
	2, 2, 2, 2,
	2, 2, 2, 2,
	3, 3, 3, 3,
	3, 3, 3, 3,
	3, 3, 3, 3,
	3, 3, 3, 3,
	3, 3, 3, 3,
};


template <int alignment>
struct alignas(alignment) NetworkRepresentation {
	alignas(alignment) std::array<std::array<std::array<float, L1Size>, FeatureSize>, InputBucketCount> FeatureWeights;
	alignas(alignment) std::array<float, L1Size> FeatureBias;
	alignas(alignment) std::array<std::array<float, L2Size>, L1Size> L1Weights;
	alignas(alignment) std::array<float, L2Size> L1Biases;
	alignas(alignment) std::array<std::array<float, L3Size>, L2Size> L2Weights;
	alignas(alignment) std::array<float, L3Size> L2Biases;
	alignas(alignment) std::array<float, L3Size> L3Weights;
	float L3Bias;
};
using UnalignedNetworkRepresentation = NetworkRepresentation<0>;
using AlignedNetworkRepresentation = NetworkRepresentation<64>;

extern const AlignedNetworkRepresentation* Network;

inline std::pair<int, int> FeatureIndexes(const uint8_t piece, const uint8_t sq, const uint8_t whiteKingSq, const uint8_t blackKingSq) {
	const uint8_t pieceColor = ColorOfPiece(piece);
	const uint8_t pieceType = TypeOfPiece(piece);
	constexpr int colorOffset = 64 * 6;

	const uint8_t whiteTransform = (GetSquareFile(whiteKingSq) < 4) ? 0 : 7;
	const uint8_t blackTransform = (GetSquareFile(blackKingSq) < 4) ? 0 : 7;

	const int whiteFeatureIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + (sq ^ whiteTransform);
	const int blackFeatureIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq ^ blackTransform);
	return { whiteFeatureIndex, blackFeatureIndex };
}

inline int GetInputBucket(const uint8_t kingSq, const bool side) {
	const uint8_t transform = side == Side::White ? 0 : 56;
	const uint8_t rank = GetSquareRank(kingSq ^ transform);
	const uint8_t file = GetSquareFile(kingSq ^ transform) < 4 ? GetSquareFile(kingSq ^ transform) : (GetSquareFile(kingSq ^ transform) ^ 7);
	return InputBucketMap[rank * 4 + file];
}

inline bool IsRefreshRequired(const Move& kingMove, const bool side) {
	// Get the real 'to' square in case of castling
	const uint8_t from = kingMove.from;
	const uint8_t to = [&] {
		if (!kingMove.IsCastling()) return kingMove.to;

		if (side == Side::White) return (kingMove.flag == MoveFlag::ShortCastle) ? Squares::G1 : Squares::C1;
		else return (kingMove.flag == MoveFlag::ShortCastle) ? Squares::G8 : Squares::C8;
	}();

	// Refresh due to horizontal mirroring or bucket change
	if ((GetSquareFile(from) < 4) != (GetSquareFile(to) < 4)) return true;
	if (GetInputBucket(from, side) != GetInputBucket(to, side)) return true;
	return false;
}

struct alignas(64) AccumulatorRepresentation {

	std::array<float, L1Size> White;
	std::array<float, L1Size> Black;
	uint8_t WhiteBucket;
	uint8_t BlackBucket;

	void Reset() {
		for (int i = 0; i < L1Size; i++) White[i] = Network->FeatureBias[i];
		for (int i = 0; i < L1Size; i++) Black[i] = Network->FeatureBias[i];
		WhiteBucket = 0;
		BlackBucket = 0;
	}

	void Refresh(const Position& pos) {
		
		Reset();
		uint64_t bits = pos.GetOccupancy();
		const uint8_t whiteKingSq = pos.WhiteKingSquare();
		const uint8_t blackKingSq = pos.BlackKingSquare();
		WhiteBucket = GetInputBucket(whiteKingSq, Side::White);
		BlackBucket = GetInputBucket(blackKingSq, Side::Black);

		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = pos.GetPieceAt(sq);
			AddFeature(FeatureIndexes(piece, sq, whiteKingSq, blackKingSq));
		}
	}

	void AddFeature(const std::pair<int, int>& features) {
		for (int i = 0; i < L1Size; i++) White[i] += Network->FeatureWeights[WhiteBucket][features.first][i];
		for (int i = 0; i < L1Size; i++) Black[i] += Network->FeatureWeights[BlackBucket][features.second][i];
	}

	void RemoveFeature(const std::pair<int, int>& features) {
		for (int i = 0; i < L1Size; i++) White[i] -= Network->FeatureWeights[WhiteBucket][features.first][i];
		for (int i = 0; i < L1Size; i++) Black[i] -= Network->FeatureWeights[BlackBucket][features.second][i];
	}

	void SetActiveBucket(const bool side, const uint8_t bucket) {
		if (side == Side::White) WhiteBucket = bucket;
		else BlackBucket = bucket;
	}

};

int NeuralEvaluate(const Position &position);
int NeuralEvaluate(const Position& position, const AccumulatorRepresentation& acc);

void UpdateAccumulator(const Position& pos, const AccumulatorRepresentation& oldAcc, AccumulatorRepresentation& newAcc,
	const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece);

void LoadDefaultNetwork();
void LoadExternalNetwork(const std::string& filename);
