#pragma once
#include "Position.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <immintrin.h>
#include <iterator>
#include <memory>

// This is the code for the NNUE evaluation
// Renegade uses a horizontally mirrored perspective net with input buckets based on the king's
// position, and output buckets based on the remaining piece count

// The engine's neural network is trained purely on self-play
// A king tropism-only evaluation was the starting point:
// for each piece: score += (15 - (manhattan distance to opponent's king)) * 6

// Network constants
#define NETWORK_NAME "renegade-net-30.bin"

constexpr int FeatureSize = 768;
constexpr int HiddenSize = 1408;
constexpr int Scale = 400;
constexpr int QA = 255;
constexpr int QB = 64;

constexpr int InputBucketCount = 16;
constexpr std::array<int, 32> InputBucketMap = {
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  8,  9,  9,
	10, 10, 11, 11,
	12, 12, 13, 13,
	12, 12, 13, 13,
	14, 14, 15, 15,
	14, 14, 15, 15,
};
constexpr int OutputBucketCount = 8;


struct alignas(64) NetworkRepresentation {
	MultiArray<int16_t, InputBucketCount, FeatureSize, HiddenSize> FeatureWeights;
	MultiArray<int16_t, HiddenSize> FeatureBias;
	MultiArray<int16_t, OutputBucketCount, HiddenSize * 2> OutputWeights;
	MultiArray<int16_t, OutputBucketCount> OutputBias;
};

extern const NetworkRepresentation* Network;


struct PieceAndSquare {
	uint8_t piece, square;
};

struct AccumulatorRepresentation;
int16_t NeuralEvaluate(const Position& position);
int16_t NeuralEvaluate(const Position& position, const AccumulatorRepresentation& acc);
void LoadDefaultNetwork();

inline int GetInputBucket(const uint8_t kingSq, const bool side) {
	const uint8_t transform = side == Side::White ? 0 : 56;
	const uint8_t rank = GetSquareRank(kingSq ^ transform);
	const uint8_t file = GetSquareFile(kingSq ^ transform) < 4 ? GetSquareFile(kingSq ^ transform) : (GetSquareFile(kingSq ^ transform) ^ 7);
	return InputBucketMap[rank * 4 + file];
}

inline int GetOutputBucket(const int pieceCount) {
	constexpr int divisor = (32 + OutputBucketCount - 1) / OutputBucketCount;
	return (pieceCount - 2) / divisor;
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
	Move move;
	uint8_t movedPiece, capturedPiece;
	bool WhiteGood, BlackGood;


	void RefreshBoth(const Position& pos) {
		RefreshWhite(pos.CurrentState());
		RefreshBlack(pos.CurrentState());
	}

	void RefreshWhite(const Board& b) {
		for (int i = 0; i < HiddenSize; i++) White[i] = Network->FeatureBias[i];
		WhiteKingSquare = LsbSquare(b.WhiteKingBits);
		WhiteBucket = GetInputBucket(WhiteKingSquare, Side::White);
		
		uint64_t bits = b.GetOccupancy();
		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = b.GetPieceAt(sq);
			AddFeatureWhite(piece, sq);
		}
		WhiteGood = true;
	}

	void RefreshBlack(const Board& b) {
		for (int i = 0; i < HiddenSize; i++) Black[i] = Network->FeatureBias[i];
		BlackKingSquare = LsbSquare(b.BlackKingBits);
		BlackBucket = GetInputBucket(BlackKingSquare, Side::Black);

		uint64_t bits = b.GetOccupancy();
		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = b.GetPieceAt(sq);
			AddFeatureBlack(piece, sq);
		}
		BlackGood = true;
	}

	void AddFeatureWhite(const uint8_t piece, const uint8_t sq) {
		const auto feature = FeatureIndexes(piece, sq).first;
		for (int i = 0; i < HiddenSize; i++) White[i] += Network->FeatureWeights[WhiteBucket][feature][i];
	}

	void AddFeatureBlack(const uint8_t piece, const uint8_t sq) {
		const auto feature = FeatureIndexes(piece, sq).second;
		for (int i = 0; i < HiddenSize; i++) Black[i] += Network->FeatureWeights[BlackBucket][feature][i];
	}

	void AddFeatureBoth(const uint8_t piece, const uint8_t sq) {
		const auto features = FeatureIndexes(piece, sq);
		for (int i = 0; i < HiddenSize; i++) White[i] += Network->FeatureWeights[WhiteBucket][features.first][i];
		for (int i = 0; i < HiddenSize; i++) Black[i] += Network->FeatureWeights[BlackBucket][features.second][i];
	}

	// Fused NNUE updates are generally a speedup, however it seems to depend on the exact machine:
	// failed to gain when tested on cloud workers, even though there was around a ~5% nps increase
	// locally. For this reason this code stays for now, but it requires further investigation. Is
	// it possible, that this optimization no longer gains due to better compilers getting better?

	void SubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const bool side) {
		const auto features1 = FeatureIndexes(f1.piece, f1.square);
		const auto features2 = FeatureIndexes(f2.piece, f2.square);

		if (side == Side::White) {
			for (int i = 0; i < HiddenSize; i++) White[i] +=
				- Network->FeatureWeights[WhiteBucket][features1.first][i]
				+ Network->FeatureWeights[WhiteBucket][features2.first][i];
		}
		else {
			for (int i = 0; i < HiddenSize; i++) Black[i] +=
				- Network->FeatureWeights[BlackBucket][features1.second][i]
				+ Network->FeatureWeights[BlackBucket][features2.second][i];
		}
	}

	void SubSubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const PieceAndSquare& f3, const bool side) {
		const auto features1 = FeatureIndexes(f1.piece, f1.square);
		const auto features2 = FeatureIndexes(f2.piece, f2.square);
		const auto features3 = FeatureIndexes(f3.piece, f3.square);

		if (side == Side::White) {
			for (int i = 0; i < HiddenSize; i++) White[i] +=
				- Network->FeatureWeights[WhiteBucket][features1.first][i]
				- Network->FeatureWeights[WhiteBucket][features2.first][i]
				+ Network->FeatureWeights[WhiteBucket][features3.first][i];
		}
		else {
			for (int i = 0; i < HiddenSize; i++) Black[i] +=
				- Network->FeatureWeights[BlackBucket][features1.second][i]
				- Network->FeatureWeights[BlackBucket][features2.second][i]
				+ Network->FeatureWeights[BlackBucket][features3.second][i];
		}
	}

	void SetKingSquare(const bool side, const uint8_t square) {
		if (side == Side::White) WhiteKingSquare = square;
		else BlackKingSquare = square;
	}

	void SetActiveBucket(const bool side, const uint8_t bucket) {
		if (side == Side::White) WhiteBucket = bucket;
		else BlackBucket = bucket;
	}

	inline std::pair<int, int> FeatureIndexes(const uint8_t piece, const uint8_t sq) const {
		const uint8_t pieceColor = ColorOfPiece(piece);
		const uint8_t pieceType = TypeOfPiece(piece);
		constexpr int colorOffset = 64 * 6;

		const uint8_t whiteTransform = (GetSquareFile(WhiteKingSquare) < 4) ? 0 : 7;
		const uint8_t blackTransform = (GetSquareFile(BlackKingSquare) < 4) ? 0 : 7;

		const int whiteFeatureIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + (sq ^ whiteTransform);
		const int blackFeatureIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq ^ blackTransform);
		return { whiteFeatureIndex, blackFeatureIndex };
	}

	void UpdateIncrementally(const bool side, const AccumulatorRepresentation& oldAcc);

};

struct EvaluationState {
	std::array<AccumulatorRepresentation, MaxDepth + 1> AccumulatorStack;
	int CurrentIndex;

	inline void PushState(const Position& pos, const Move move, const uint8_t movedPiece, const uint8_t capturedPiece) {
		CurrentIndex += 1;
		AccumulatorRepresentation& current = AccumulatorStack[CurrentIndex];
		current.move = move;
		current.movedPiece = movedPiece;
		current.capturedPiece = capturedPiece;
		current.BlackGood = false;
		current.WhiteGood = false;
		current.WhiteKingSquare = pos.WhiteKingSquare();
		current.BlackKingSquare = pos.BlackKingSquare();
		current.WhiteBucket = GetInputBucket(current.WhiteKingSquare, Side::White);
		current.BlackBucket = GetInputBucket(current.BlackKingSquare, Side::Black);
	}

	inline void PopState() {
		CurrentIndex -= 1;
		assert(CurrentIndex >= 0);
	}

	inline void Reset(const Position& pos) {
		CurrentIndex = 0;
		AccumulatorStack[0].RefreshBoth(pos);
	}

	int16_t Evaluate(const Position& pos);
};
