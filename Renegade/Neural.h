#pragma once
#include "Position.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>

#ifdef __AVX2__
#include <immintrin.h>
#endif

// This is the code for the NNUE evaluation
// Renegade uses a horizontally mirrored perspective net with input buckets based on the king's
// position, and output buckets based on the remaining piece count

// The engine's neural network is trained purely on self-play
// A king tropism-only evaluation was the starting point:
// for each piece: score += (15 - (manhattan distance to opponent's king)) * 6

// Network constants
#define NETWORK_NAME "renegade-net-35.bin"

constexpr int FeatureSize = 768;
constexpr int HiddenSize = 1600;
constexpr int Scale = 400;
constexpr int QA = 255;
constexpr int QB = 64;

constexpr int InputBucketCount = 14;
constexpr std::array<int, 32> InputBucketMap = {
	 0,  1,  2,  3,
	 4,  5,  6,  7,
	 8,  8,  9,  9,
	10, 10, 11, 11,
	10, 10, 11, 11,
	12, 12, 13, 13,
	12, 12, 13, 13,
	12, 12, 13, 13,
};
constexpr int OutputBucketCount = 8;


struct alignas(64) NetworkRepresentation {
	alignas(64) MultiArray<int16_t, InputBucketCount, FeatureSize, HiddenSize> FeatureWeights;
	alignas(64) MultiArray<int16_t, HiddenSize> FeatureBias;
	alignas(64) MultiArray<int16_t, OutputBucketCount, HiddenSize * 2> OutputWeights;
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

inline bool IsRefreshRequired(const uint8_t piece, const Move& move, const bool side) {
	// If our king didn't move then it can't be a refresh
	if ((side == Side::White && piece != Piece::WhiteKing) || (side == Side::Black && piece != Piece::BlackKing))
		return false;

	// Get the real 'to' square in case of castling
	const uint8_t from = move.from;
	const uint8_t to = [&] {
		if (!move.IsCastling()) return move.to;

		if (side == Side::White) return (move.flag == MoveFlag::ShortCastle) ? Squares::G1 : Squares::C1;
		else return (move.flag == MoveFlag::ShortCastle) ? Squares::G8 : Squares::C8;
	}();

	// Refresh due to horizontal mirroring or bucket change
	if ((GetSquareFile(from) < 4) != (GetSquareFile(to) < 4)) return true;
	if (GetInputBucket(from, side) != GetInputBucket(to, side)) return true;
	return false;
}


struct alignas(64) AccumulatorRepresentation {

	std::array<std::array<int16_t, HiddenSize>, 2> Accumulator;
	std::array<uint8_t, 2> ActiveBucket;
	std::array<uint8_t, 2> KingSquare;
	std::array<bool, 2> Correct;

	Move move;
	uint8_t movedPiece, capturedPiece;


	void RefreshBoth(const Position& pos) {
		RefreshSide(Side::White, pos.CurrentState());
		RefreshSide(Side::Black, pos.CurrentState());
	}

	void RefreshSide(const bool side, const Board& b) {
		for (int i = 0; i < HiddenSize; i++) Accumulator[side][i] = Network->FeatureBias[i];
		KingSquare[side] = LsbSquare(side == Side::White ? b.WhiteKingBits : b.BlackKingBits);
		ActiveBucket[side] = GetInputBucket(KingSquare[side], side);
		
		uint64_t bits = b.GetOccupancy();
		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = b.GetPieceAt(sq);
			AddFeatureForSide(side, piece, sq);
		}
		Correct[side] = true;
	}

	void AddFeatureForSide(const bool side, const uint8_t piece, const uint8_t sq) {
		const int feature = FeatureIndex(side, piece, sq);
		const int bucket = ActiveBucket[side];
		for (int i = 0; i < HiddenSize; i++) Accumulator[side][i] += Network->FeatureWeights[bucket][feature][i];
	}

	// Fused NNUE updates are generally a speedup, however it seems to depend on the exact machine:
	// failed to gain when tested on cloud workers, even though there was around a ~5% nps increase
	// locally. For this reason this code stays for now, but it requires further investigation. Is
	// it possible, that this optimization no longer gains due to better compilers getting better?

	void SubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const bool side) {
		const auto features1 = FeatureIndex(side, f1.piece, f1.square);
		const auto features2 = FeatureIndex(side, f2.piece, f2.square);
		const int bucket = ActiveBucket[side];
		for (int i = 0; i < HiddenSize; i++) Accumulator[side][i] +=
			- Network->FeatureWeights[bucket][features1][i]
			+ Network->FeatureWeights[bucket][features2][i];
	}

	void SubSubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const PieceAndSquare& f3, const bool side) {
		const int bucket = ActiveBucket[side];
		const auto features1 = FeatureIndex(side, f1.piece, f1.square);
		const auto features2 = FeatureIndex(side, f2.piece, f2.square);
		const auto features3 = FeatureIndex(side, f3.piece, f3.square);
		for (int i = 0; i < HiddenSize; i++) Accumulator[side][i] +=
			- Network->FeatureWeights[bucket][features1][i]
			- Network->FeatureWeights[bucket][features2][i]
			+ Network->FeatureWeights[bucket][features3][i];
	}

	inline int FeatureIndex(const bool perspective, const uint8_t piece, const uint8_t sq) const {
		const uint8_t pieceColor = ColorOfPiece(piece);
		const uint8_t pieceType = TypeOfPiece(piece);
		constexpr int colorOffset = 64 * 6;

		const uint8_t transform = (GetSquareFile(KingSquare[perspective]) < 4) ? 0 : 7;
		const int featureIndex = (pieceColor == (SideToPieceColor(perspective)) ? 0 : colorOffset) + (pieceType - 1) * 64
			+ ((perspective == Side::White) ? (sq ^ transform) : Mirror(sq ^ transform));
		
		return featureIndex;
	}
};

struct alignas(64) BucketCacheEntry {
	alignas(64) std::array<int16_t, HiddenSize> cachedAcc;
	std::array<uint64_t, 12> featureBits{};

	BucketCacheEntry() {
		for (int i = 0; i < HiddenSize; i++) cachedAcc[i] = Network->FeatureBias[i];
	}
};

struct EvaluationState {
	std::array<AccumulatorRepresentation, MaxDepth + 1> AccumulatorStack;
	int CurrentIndex;
	MultiArray<BucketCacheEntry, 2, InputBucketCount * 2> BucketCache;

	inline void PushState(const Position& pos, const Move move, const uint8_t movedPiece, const uint8_t capturedPiece) {
		CurrentIndex += 1;
		AccumulatorRepresentation& current = AccumulatorStack[CurrentIndex];
		current.move = move;
		current.movedPiece = movedPiece;
		current.capturedPiece = capturedPiece;
		current.Correct = { false, false };
		current.KingSquare[Side::White] = pos.WhiteKingSquare();
		current.KingSquare[Side::Black] = pos.BlackKingSquare();
		current.ActiveBucket[Side::White] = GetInputBucket(current.KingSquare[Side::White], Side::White);
		current.ActiveBucket[Side::Black] = GetInputBucket(current.KingSquare[Side::Black], Side::Black);
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
	void UpdateIncrementally(const bool side, const int accIndex);
	void UpdateFromBucketCache(const Position& pos, const int accIndex, const bool side);
};
