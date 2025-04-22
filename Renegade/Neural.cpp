#include "Neural.h"

// Incbin shenanigans

#ifdef _MSC_VER
#define RENEGADE_MSVC
#pragma push_macro("_MSC_VER")
#undef _MSC_VER
#endif

#include "incbin/incbin.h"

#ifdef RENEGADE_MSVC
#pragma pop_macro("_MSC_VER")
#undef RENEGADE_MSVC
#endif

INCBIN(DefaultNetwork, NETWORK_NAME);
const NetworkRepresentation* Network;
std::unique_ptr<NetworkRepresentation> ExternalNetwork;

// Evaluating the position ------------------------------------------------------------------------

int16_t NeuralEvaluate(const Position& position, const AccumulatorRepresentation& acc) {
	assert(acc.Correct[Side::White] && acc.Correct[Side::Black]);

	const bool turn = position.Turn();
	const std::array<int16_t, HiddenSize>& hiddenFriendly = acc.Accumulator[turn];
	const std::array<int16_t, HiddenSize>& hiddenOpponent = acc.Accumulator[!turn];
	int32_t output = 0;

	const int pieceCount = Popcount(position.GetOccupancy());
	const int outputBucket = GetOutputBucket(pieceCount);

#ifdef __AVX2__
	// Calculate output with handwritten SIMD (autovec also works, but it's slower)
	// Idea by somelizard, it makes fast QA=255 SCReLU possible

	auto HorizontalSum = [] (const __m256i sum) {
		const auto upper_128 = _mm256_extracti128_si256(sum, 1);
		const auto lower_128 = _mm256_castsi256_si128(sum);
		const auto sum_128 = _mm_add_epi32(upper_128, lower_128);
		const auto upper_64 = _mm_unpackhi_epi64(sum_128, sum_128);
		const auto sum_64 = _mm_add_epi32(upper_64, sum_128);
		const auto upper_32 = _mm_shuffle_epi32(sum_64, 0b00'00'00'01);
		const auto sum_32 = _mm_add_epi32(upper_32, sum_64);
		return _mm_cvtsi128_si32(sum_32);
	};
	
	constexpr int chunkSize = 16; // for AVX2: 256/16=16
	const auto min = _mm256_setzero_si256();
	const auto max = _mm256_set1_epi16(static_cast<int16_t>(QA));
	
	auto sum = _mm256_setzero_si256();
	for (int i = 0; i < (HiddenSize / chunkSize); i++) {
		auto v = _mm256_load_si256((__m256i*) &hiddenFriendly[chunkSize * i]);
		v = _mm256_min_epi16(_mm256_max_epi16(v, min), max);
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[outputBucket][chunkSize * i]);
		const auto p = _mm256_madd_epi16(v, _mm256_mullo_epi16(v, w));
		sum = _mm256_add_epi32(sum, p);
	}
	output = HorizontalSum(sum);

	sum = _mm256_setzero_si256();
	for (int i = 0; i < (HiddenSize / chunkSize); i++) {
		auto v = _mm256_load_si256((__m256i*) &hiddenOpponent[chunkSize * i]);
		v = _mm256_min_epi16(_mm256_max_epi16(v, min), max);
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[outputBucket][chunkSize * i + HiddenSize]);
		const auto p = _mm256_madd_epi16(v, _mm256_mullo_epi16(v, w));
		sum = _mm256_add_epi32(sum, p);
	}
	output += HorizontalSum(sum);
#else
	// Slower fallback
	// if you end up here... that's bad.
	auto Activation = [] (const int16_t value) {
		const int32_t x = std::clamp<int32_t>(value, 0, QA);
		return x * x;
	};
	for (int i = 0; i < HiddenSize; i++) output += Activation(hiddenFriendly[i]) * Network->OutputWeights[outputBucket][i];
	for (int i = 0; i < HiddenSize; i++) output += Activation(hiddenOpponent[i]) * Network->OutputWeights[outputBucket][i + HiddenSize];
#endif

	constexpr int Q = QA * QB;
	output = (output / QA + Network->OutputBias[outputBucket]) * Scale / Q; // for SCReLU

	// Scale according to material
	const int gamePhase = position.GetGamePhase();
	output = output * (52 + std::min(24, gamePhase)) / 64;

	return std::clamp(output, -MateThreshold + 1, MateThreshold - 1);
}

int16_t NeuralEvaluate(const Position& position) {
	AccumulatorRepresentation acc{};
	acc.RefreshBoth(position);
	return NeuralEvaluate(position, acc);
}

// Evaluation call & accumulator updates ----------------------------------------------------------

int16_t EvaluationState::Evaluate(const Position& pos) {

	// For evaluating, we need to make sure the accumulator is up-to-date for both sides
	// The accumulators can be updated in two ways, in order of preference:
	// - if applicable, incrementally from the last updated one
	// - reconstructing it using a cache that we keep (colloquially known as "Finny tables")
	
	// Currently not used, but note that the accumulator stack and the position stack are indexed differently:
	// const int basePositionIndex = pos.States.size() - CurrentIndex - 1;

	for (const bool side : {Side::White, Side::Black}) {

		if (!AccumulatorStack[CurrentIndex].Correct[side]) {
			const std::optional<int> latestUpdated = [&] {
				for (int i = CurrentIndex; i >= 0; i--) {
					if (AccumulatorStack[i].Correct[side]) return std::optional<int>(i);
					if (IsRefreshRequired(AccumulatorStack[i].movedPiece, AccumulatorStack[i].move, side)) {
						return std::optional<int>(std::nullopt);
					}
				}
				assert(false);
				return std::optional<int>(std::nullopt);
			}();

			if (latestUpdated.has_value()) {
				for (int i = latestUpdated.value() + 1; i <= CurrentIndex; i++) {
					UpdateIncrementally(side, i);
				}
			}
			else {
				UpdateFromBucketCache(pos, CurrentIndex, side);
			}
		}
	}

	/*
	const int fresh = NeuralEvaluate(pos);
	const int acc = NeuralEvaluate(pos, AccumulatorStack[CurrentIndex]);
	assert(fresh == acc);
	*/

	// Now the accumulators are guaranteed to be correct, so the evaluation can be obtained
	return NeuralEvaluate(pos, AccumulatorStack[CurrentIndex]);
}

void EvaluationState::UpdateIncrementally(const bool side, const int accIndex) {

	const AccumulatorRepresentation& o = AccumulatorStack[accIndex - 1];  // o -> old
	AccumulatorRepresentation& c = AccumulatorStack[accIndex];            // c -> current
	const Move& m = c.move;

	// Ensure the base accumulator is already up to date, and copy over the previous state
    // (possible future optimization by deferring this and adding the accumulator change?)
	assert(o.Correct[side]);
	c.Accumulator[side] = o.Accumulator[side];

	// After completing the following, it's guaranteed that the accumulator will be up to date for the given side
	c.Correct[side] = true;
	
	// For null-moves nothing changes, we're done here
	if (m.IsNull()) return;

	// Handle various cases of incremental updating
	// (a) regular non-capture move
	if (c.capturedPiece == Piece::None && !m.IsPromotion() && m.flag != MoveFlag::EnPassantPerformed) {
		c.SubAddFeature({ c.movedPiece, m.from }, { c.movedPiece, m.to }, side);
		return;
	}

	// (b) regular capture move
	if (c.capturedPiece != Piece::None && !m.IsPromotion() && m.flag != MoveFlag::EnPassantPerformed && !m.IsCastling()) {
		c.SubSubAddFeature({ c.movedPiece, m.from }, { c.capturedPiece, m.to }, { c.movedPiece, m.to }, side);
		return;
	}

	// (c) castling
	if (m.IsCastling()) {
		const bool castlingSide = ColorOfPiece(c.movedPiece) == PieceColor::White;
		const bool shortCastle = m.flag == MoveFlag::ShortCastle;
		const uint8_t rookPiece = castlingSide == Side::White ? Piece::WhiteRook : Piece::BlackRook;
		const uint8_t newKingFile = shortCastle ? 6 : 2;
		const uint8_t newRookFile = shortCastle ? 5 : 3;
		const uint8_t newKingSquare = newKingFile + (castlingSide == Side::Black) * 56;
		const uint8_t newRookSquare = newRookFile + (castlingSide == Side::Black) * 56;
		c.SubAddFeature({ c.movedPiece, m.from }, { c.movedPiece, newKingSquare }, side);
		c.SubAddFeature({ rookPiece, m.to }, { rookPiece, newRookSquare }, side);
		return;
	}

	// (d) promotion - with optional capture
	if (m.IsPromotion()) {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(c.movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		if (c.capturedPiece == Piece::None) c.SubAddFeature({ c.movedPiece, m.from }, { promotionPiece, m.to }, side);
		else c.SubSubAddFeature({ c.movedPiece, m.from }, { c.capturedPiece, m.to }, { promotionPiece, m.to }, side);
		return;
	}

	// (e) en passant
	if (c.move.flag == MoveFlag::EnPassantPerformed) {
		const uint8_t victimPiece = c.movedPiece == Piece::WhitePawn ? Piece::BlackPawn : Piece::WhitePawn;
		const uint8_t victimSquare = c.movedPiece == Piece::WhitePawn ? (m.to - 8) : (m.to + 8);
		c.SubSubAddFeature({ c.movedPiece, m.from }, { victimPiece, victimSquare }, { c.movedPiece, m.to }, side);
		return;
	}
}

void EvaluationState::UpdateFromBucketCache(const Position& pos, const int accIndex, const bool side) {

	// Get the cache entry to be updated
	const uint8_t kingSq = AccumulatorStack[CurrentIndex].KingSquare[side];
	const int inputBucket = AccumulatorStack[CurrentIndex].ActiveBucket[side];
	const int mirroring = GetSquareFile(kingSq) >= 4;
	const int bucketCacheIndex = inputBucket + (mirroring * InputBucketCount);
	BucketCacheEntry& cache = BucketCache[side][bucketCacheIndex];

	// Calculate the feature boolean array for the current position
	std::array<uint64_t, 12> featureBits;
	if (side == Side::White) {
		featureBits[0] = pos.CurrentState().WhitePawnBits;
		featureBits[1] = pos.CurrentState().WhiteKnightBits;
		featureBits[2] = pos.CurrentState().WhiteBishopBits;
		featureBits[3] = pos.CurrentState().WhiteRookBits;
		featureBits[4] = pos.CurrentState().WhiteQueenBits;
		featureBits[5] = pos.CurrentState().WhiteKingBits;
		featureBits[6] = pos.CurrentState().BlackPawnBits;
		featureBits[7] = pos.CurrentState().BlackKnightBits;
		featureBits[8] = pos.CurrentState().BlackBishopBits;
		featureBits[9] = pos.CurrentState().BlackRookBits;
		featureBits[10] = pos.CurrentState().BlackQueenBits;
		featureBits[11] = pos.CurrentState().BlackKingBits;
	}
	else {
		featureBits[0] = pos.CurrentState().BlackPawnBits;
		featureBits[1] = pos.CurrentState().BlackKnightBits;
		featureBits[2] = pos.CurrentState().BlackBishopBits;
		featureBits[3] = pos.CurrentState().BlackRookBits;
		featureBits[4] = pos.CurrentState().BlackQueenBits;
		featureBits[5] = pos.CurrentState().BlackKingBits;
		featureBits[6] = pos.CurrentState().WhitePawnBits;
		featureBits[7] = pos.CurrentState().WhiteKnightBits;
		featureBits[8] = pos.CurrentState().WhiteBishopBits;
		featureBits[9] = pos.CurrentState().WhiteRookBits;
		featureBits[10] = pos.CurrentState().WhiteQueenBits;
		featureBits[11] = pos.CurrentState().WhiteKingBits;
	}

	// Compare it with the cached entry
	for (int i = 0; i < 12; i++) {
		uint64_t toBeAdded = featureBits[i] & ~cache.featureBits[i];
		uint64_t toBeSubbed = cache.featureBits[i] & ~featureBits[i];

		while (toBeAdded) {
			const uint8_t sq = Popsquare(toBeAdded);
			const int featureSq = !mirroring ? sq : (sq ^ 7);
			const int feature = (side == Side::White ? featureSq : Mirror(featureSq)) + i * 64;
			for (int i = 0; i < HiddenSize; i++) cache.cachedAcc[i] += Network->FeatureWeights[inputBucket][feature][i];
		}

		while (toBeSubbed) {
			const uint8_t sq = Popsquare(toBeSubbed);
			const int featureSq = !mirroring ? sq : (sq ^ 7);
			const int feature = (side == Side::White ? featureSq : Mirror(featureSq)) + i * 64;
			for (int i = 0; i < HiddenSize; i++) cache.cachedAcc[i] -= Network->FeatureWeights[inputBucket][feature][i];
		}
	}

	// The cached entry is now updated, now copy it to the stack
	cache.featureBits = featureBits;
	AccumulatorStack[accIndex].Accumulator[side] = cache.cachedAcc;
	AccumulatorStack[accIndex].Correct[side] = true;
}

// Loading the neural network ---------------------------------------------------------------------

void LoadDefaultNetwork() {
#if !defined(_MSC_VER) || defined(__clang__)
	// Include binary in the executable file via incbin (good)
	Network = reinterpret_cast<const NetworkRepresentation*>(gDefaultNetworkData);
#else
	// Load network file from disk at runtime (bad)
	std::ifstream ifs(NETWORK_NAME, std::ios::binary);
	if (!ifs) {
		cout << "Failed to load network: " << NETWORK_NAME << endl;
		return;
	}

	std::unique_ptr<NetworkRepresentation> loadedNetwork = std::make_unique<NetworkRepresentation>();
	ifs.read((char*)loadedNetwork.get(), sizeof(NetworkRepresentation));
	std::swap(ExternalNetwork, loadedNetwork);
	Network = ExternalNetwork.get();

	// Check if startpos evaluation actually makes sense
	const Position pos{};
	const int startposEval = NeuralEvaluate(pos);
	if (std::abs(startposEval) < 300 && startposEval != 0) cout << "Loaded '" << NETWORK_NAME << "' network from disk probably successfully";
	else cout << "Loaded '" << NETWORK_NAME << "', but it stinks";
	cout << " (startpos raw eval: " << startposEval << ")" << endl;
#endif
}