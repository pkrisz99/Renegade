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

	alignas(64) std::array<int16_t, HiddenSize> WhiteAccumulator;
	alignas(64) std::array<int16_t, HiddenSize> BlackAccumulator;
	std::array<uint8_t, 2> ActiveBucket;
	std::array<uint8_t, 2> KingSquare;
	std::array<bool, 2> Correct;

	Move move;
	uint8_t movedPiece, capturedPiece;


	void RefreshBoth(const Position& pos) {
		RefreshWhite(pos.CurrentState());
		RefreshBlack(pos.CurrentState());
	}

	void RefreshWhite(const Board& b) {
		for (int i = 0; i < HiddenSize; i++) WhiteAccumulator[i] = Network->FeatureBias[i];
		KingSquare[Side::White] = LsbSquare(b.WhiteKingBits);
		ActiveBucket[Side::White] = GetInputBucket(KingSquare[Side::White], Side::White);
		
		uint64_t bits = b.GetOccupancy();
		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = b.GetPieceAt(sq);
			AddFeatureWhite(piece, sq);
		}
		Correct[Side::White] = true;
	}

	void RefreshBlack(const Board& b) {
		for (int i = 0; i < HiddenSize; i++) BlackAccumulator[i] = Network->FeatureBias[i];
		KingSquare[Side::Black] = LsbSquare(b.BlackKingBits);
		ActiveBucket[Side::Black] = GetInputBucket(KingSquare[Side::Black], Side::Black);

		uint64_t bits = b.GetOccupancy();
		while (bits) {
			const uint8_t sq = Popsquare(bits);
			const uint8_t piece = b.GetPieceAt(sq);
			AddFeatureBlack(piece, sq);
		}
		Correct[Side::Black] = true;
	}

	void AddFeatureWhite(const uint8_t piece, const uint8_t sq) {
		const auto feature = FeatureIndexes(piece, sq).first;
		const int bucket = ActiveBucket[Side::White];
		for (int i = 0; i < HiddenSize; i++) WhiteAccumulator[i] += Network->FeatureWeights[bucket][feature][i];
	}

	void AddFeatureBlack(const uint8_t piece, const uint8_t sq) {
		const auto feature = FeatureIndexes(piece, sq).second;
		const int bucket = ActiveBucket[Side::Black];
		for (int i = 0; i < HiddenSize; i++) BlackAccumulator[i] += Network->FeatureWeights[bucket][feature][i];
	}

	void AddFeatureBoth(const uint8_t piece, const uint8_t sq) {
		const auto features = FeatureIndexes(piece, sq);
		const int wbucket = ActiveBucket[Side::White];
		const int bbucket = ActiveBucket[Side::Black];
		for (int i = 0; i < HiddenSize; i++) WhiteAccumulator[i] += Network->FeatureWeights[wbucket][features.first][i];
		for (int i = 0; i < HiddenSize; i++) BlackAccumulator[i] += Network->FeatureWeights[bbucket][features.second][i];
	}

	// Fused NNUE updates are generally a speedup, however it seems to depend on the exact machine:
	// failed to gain when tested on cloud workers, even though there was around a ~5% nps increase
	// locally. For this reason this code stays for now, but it requires further investigation. Is
	// it possible, that this optimization no longer gains due to better compilers getting better?

	void SubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const bool side) {
		const auto features1 = FeatureIndexes(f1.piece, f1.square);
		const auto features2 = FeatureIndexes(f2.piece, f2.square);

		if (side == Side::White) {
			const int wbucket = ActiveBucket[Side::White];
			for (int i = 0; i < HiddenSize; i++) WhiteAccumulator[i] +=
				- Network->FeatureWeights[wbucket][features1.first][i]
				+ Network->FeatureWeights[wbucket][features2.first][i];
		}
		else {
			const int bbucket = ActiveBucket[Side::Black];
			for (int i = 0; i < HiddenSize; i++) BlackAccumulator[i] +=
				- Network->FeatureWeights[bbucket][features1.second][i]
				+ Network->FeatureWeights[bbucket][features2.second][i];
		}
	}

	void SubSubAddFeature(const PieceAndSquare& f1, const PieceAndSquare& f2, const PieceAndSquare& f3, const bool side) {
		const auto features1 = FeatureIndexes(f1.piece, f1.square);
		const auto features2 = FeatureIndexes(f2.piece, f2.square);
		const auto features3 = FeatureIndexes(f3.piece, f3.square);

		if (side == Side::White) {
			const int wbucket = ActiveBucket[Side::White];
			for (int i = 0; i < HiddenSize; i++) WhiteAccumulator[i] +=
				- Network->FeatureWeights[wbucket][features1.first][i]
				- Network->FeatureWeights[wbucket][features2.first][i]
				+ Network->FeatureWeights[wbucket][features3.first][i];
		}
		else {
			const int bbucket = ActiveBucket[Side::Black];
			for (int i = 0; i < HiddenSize; i++) BlackAccumulator[i] +=
				- Network->FeatureWeights[bbucket][features1.second][i]
				- Network->FeatureWeights[bbucket][features2.second][i]
				+ Network->FeatureWeights[bbucket][features3.second][i];
		}
	}

	void SetKingSquare(const bool side, const uint8_t square) {
		if (side == Side::White) KingSquare[Side::White] = square;
		else KingSquare[Side::Black] = square;
	}

	void SetActiveBucket(const bool side, const uint8_t bucket) {
		if (side == Side::White) ActiveBucket[Side::White] = bucket;
		else ActiveBucket[Side::Black] = bucket;
	}

	inline std::pair<int, int> FeatureIndexes(const uint8_t piece, const uint8_t sq) const {
		const uint8_t pieceColor = ColorOfPiece(piece);
		const uint8_t pieceType = TypeOfPiece(piece);
		constexpr int colorOffset = 64 * 6;

		const uint8_t whiteTransform = (GetSquareFile(KingSquare[Side::White]) < 4) ? 0 : 7;
		const uint8_t blackTransform = (GetSquareFile(KingSquare[Side::Black]) < 4) ? 0 : 7;

		const int whiteFeatureIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + (sq ^ whiteTransform);
		const int blackFeatureIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq ^ blackTransform);
		return { whiteFeatureIndex, blackFeatureIndex };
	}

	void UpdateIncrementally(const bool side, const AccumulatorRepresentation& oldAcc);

};

struct alignas(64) BucketCacheItem {
	std::array<int16_t, HiddenSize> cachedAcc;
	std::array<uint64_t, 12> featureBits{};

	BucketCacheItem() {
		for (int i = 0; i < HiddenSize; i++) cachedAcc[i] = Network->FeatureBias[i];
	}
};

struct EvaluationState {
	std::array<AccumulatorRepresentation, MaxDepth + 1> AccumulatorStack;
	int CurrentIndex;
	MultiArray<BucketCacheItem, 2, InputBucketCount * 2> BucketCache;

	inline void PushState(const Position& pos, const Move move, const uint8_t movedPiece, const uint8_t capturedPiece) {
		CurrentIndex += 1;
		AccumulatorRepresentation& current = AccumulatorStack[CurrentIndex];
		current.move = move;
		current.movedPiece = movedPiece;
		current.capturedPiece = capturedPiece;
		current.Correct[Side::White] = false;
		current.Correct[Side::Black] = false;
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
	void UpdateFromBucketCache(const Position& pos, const int accIndex, const bool side);
};
