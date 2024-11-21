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
#define NETWORK_NAME "renegade-net-29.bin"
#endif

constexpr int FeatureSize = 768;
constexpr int HiddenSize = 1408;
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


struct alignas(64) NetworkRepresentation {
	std::array<std::array<std::array<int16_t, HiddenSize>, FeatureSize>, InputBucketCount> FeatureWeights;
	std::array<int16_t, HiddenSize> FeatureBias;
	std::array<int16_t, HiddenSize * 2> OutputWeights;
	int16_t OutputBias;
};

extern const NetworkRepresentation* Network;


struct PieceAndSquare {
	uint8_t piece, square;
};

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

	std::array<int16_t, HiddenSize> White;
	std::array<int16_t, HiddenSize> Black;
	uint8_t WhiteBucket, BlackBucket;
	uint8_t WhiteKingSquare, BlackKingSquare;

	void Reset() {
		for (int i = 0; i < HiddenSize; i++) White[i] = Network->FeatureBias[i];
		for (int i = 0; i < HiddenSize; i++) Black[i] = Network->FeatureBias[i];
		WhiteBucket = 0;
		BlackBucket = 0;
		WhiteKingSquare = 0;
		BlackKingSquare = 0;
	}

	void Refresh(const Position& pos) {
		Reset();
		uint64_t bits = pos.GetOccupancy();
		WhiteKingSquare = pos.WhiteKingSquare();
		BlackKingSquare = pos.BlackKingSquare();
		WhiteBucket = GetInputBucket(WhiteKingSquare, Side::White);
		BlackBucket = GetInputBucket(BlackKingSquare, Side::Black);

		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = pos.GetPieceAt(sq);
			AddFeature(piece, sq);
		}
	}

	void AddFeature(const uint8_t piece, const uint8_t sq) {
		const auto features = FeatureIndexes(piece, sq);
		for (int i = 0; i < HiddenSize; i++) White[i] += Network->FeatureWeights[WhiteBucket][features.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] += Network->FeatureWeights[BlackBucket][features.second][i];
	}

	void SubtractFeature(const uint8_t piece, const uint8_t sq) {
		const auto features = FeatureIndexes(piece, sq);
		for (int i = 0; i < HiddenSize; i++) White[i] -= Network->FeatureWeights[WhiteBucket][features.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] -= Network->FeatureWeights[BlackBucket][features.second][i];
	}

	void SubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2) {
		const auto features1 = FeatureIndexes(f1.piece, f1.square);
		const auto features2 = FeatureIndexes(f2.piece, f2.square);

		for (int i = 0; i < HiddenSize; i++) White[i] +=
			- Network->FeatureWeights[WhiteBucket][features1.first][i]
			+ Network->FeatureWeights[WhiteBucket][features2.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] +=
			- Network->FeatureWeights[BlackBucket][features1.second][i]
			+ Network->FeatureWeights[BlackBucket][features2.second][i];
	}

	void SubSubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const PieceAndSquare& f3) {
		const auto features1 = FeatureIndexes(f1.piece, f1.square);
		const auto features2 = FeatureIndexes(f2.piece, f2.square);
		const auto features3 = FeatureIndexes(f3.piece, f3.square);

		for (int i = 0; i < HiddenSize; i++) White[i] +=
			- Network->FeatureWeights[WhiteBucket][features1.first][i]
			- Network->FeatureWeights[WhiteBucket][features2.first][i]
			+ Network->FeatureWeights[WhiteBucket][features3.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] +=
			- Network->FeatureWeights[BlackBucket][features1.second][i]
			- Network->FeatureWeights[BlackBucket][features2.second][i]
			+ Network->FeatureWeights[BlackBucket][features3.second][i];
	}

	void SetKingSquare(const bool side, const uint8_t square) {
		if (side == Side::White) WhiteKingSquare = square;
		else BlackKingSquare = square;
	}

	void SetActiveBucket(const bool side, const uint8_t bucket) {
		if (side == Side::White) WhiteBucket = bucket;
		else BlackBucket = bucket;
	}

	inline std::pair<int, int> FeatureIndexes(const uint8_t piece, const uint8_t sq) {
		const uint8_t pieceColor = ColorOfPiece(piece);
		const uint8_t pieceType = TypeOfPiece(piece);
		constexpr int colorOffset = 64 * 6;

		const uint8_t whiteTransform = (GetSquareFile(WhiteKingSquare) < 4) ? 0 : 7;
		const uint8_t blackTransform = (GetSquareFile(BlackKingSquare) < 4) ? 0 : 7;

		const int whiteFeatureIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + (sq ^ whiteTransform);
		const int blackFeatureIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq ^ blackTransform);
		return { whiteFeatureIndex, blackFeatureIndex };
	}

};

int NeuralEvaluate(const Position &position);
int NeuralEvaluate(const Position& position, const AccumulatorRepresentation& acc);

void UpdateAccumulator(const Position& pos, const AccumulatorRepresentation& oldAcc, AccumulatorRepresentation& newAcc,
	const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece);

void LoadDefaultNetwork();
void LoadExternalNetwork(const std::string& filename);
