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
	assert(acc.WhiteGood);
	assert(acc.BlackGood);

	const bool turn = position.Turn();
	const std::array<int16_t, HiddenSize>& hiddenFriendly = (turn == Side::White) ? acc.WhiteAccumulator : acc.BlackAccumulator;
	const std::array<int16_t, HiddenSize>& hiddenOpponent = (turn == Side::White) ? acc.BlackAccumulator : acc.WhiteAccumulator;
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

// Incremental accumulator updates ----------------------------------------------------------------

void AccumulatorRepresentation::UpdateIncrementally(const bool side, const AccumulatorRepresentation& oldAcc) {

	// Ensure the base accumulator is already up to date
	assert(oldAcc.WhiteGood || side == Side::Black);
	assert(oldAcc.BlackGood || side == Side::White);

	// After completing this, it's guaranteed that the accumulator will be up to date for the given side
	if (side == Side::White) Correct[Side::White] = true;
	else Correct[Side::Black] = true;

	// Copy over the previous state (possible future optimization by deferring this and adding the accumulator change?)
	if (side == Side::White) WhiteAccumulator = oldAcc.WhiteAccumulator;
	else BlackAccumulator = oldAcc.BlackAccumulator;
	
	// For null-moves nothing changes, we're done here
	if (move.IsNull()) return;

	// Handle various cases of incremental updating
	// (a) regular non-capture move
	if (capturedPiece == Piece::None && !move.IsPromotion() && move.flag != MoveFlag::EnPassantPerformed) {
		SubAddFeature({ movedPiece, move.from }, { movedPiece, move.to }, side);
		return;
	}

	// (b) regular capture move
	if (capturedPiece != Piece::None && !move.IsPromotion() && move.flag != MoveFlag::EnPassantPerformed && !move.IsCastling()) {
		SubSubAddFeature({ movedPiece, move.from }, { capturedPiece, move.to }, { movedPiece, move.to }, side);
		return;
	}

	// (c) castling
	if (move.IsCastling()) {
		const bool castlingSide = ColorOfPiece(movedPiece) == PieceColor::White;
		const bool shortCastle = move.flag == MoveFlag::ShortCastle;
		const uint8_t rookPiece = castlingSide == Side::White ? Piece::WhiteRook : Piece::BlackRook;
		const uint8_t newKingFile = shortCastle ? 6 : 2;
		const uint8_t newRookFile = shortCastle ? 5 : 3;
		const uint8_t newKingSquare = newKingFile + (castlingSide == Side::Black) * 56;
		const uint8_t newRookSquare = newRookFile + (castlingSide == Side::Black) * 56;
		SubAddFeature({ movedPiece, move.from }, { movedPiece, newKingSquare }, side);
		SubAddFeature({ rookPiece, move.to }, { rookPiece, newRookSquare }, side);
		return;
	}

	// (d) promotion - with optional capture
	if (move.IsPromotion()) {
		const uint8_t promotionPiece = move.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		if (capturedPiece == Piece::None) SubAddFeature({ movedPiece, move.from }, { promotionPiece, move.to }, side);
		else SubSubAddFeature({ movedPiece, move.from }, { capturedPiece, move.to }, { promotionPiece, move.to }, side);
		return;
	}

	// (e) en passant
	if (move.flag == MoveFlag::EnPassantPerformed) {
		const uint8_t victimPiece = movedPiece == Piece::WhitePawn ? Piece::BlackPawn : Piece::WhitePawn;
		const uint8_t victimSquare = movedPiece == Piece::WhitePawn ? (move.to - 8) : (move.to + 8);
		SubSubAddFeature({ movedPiece, move.from }, { victimPiece, victimSquare }, { movedPiece, move.to }, side);
		return;
	}
}

void EvaluationState::UpdateFromBucketCache(const Position& pos, const int accIndex, const bool side) {
	// Bucket caches
	// (I know this part is horrible)

	// Get the cache entry to be updated
	const uint8_t whiteKingSq = pos.WhiteKingSquare();
	const int inputBucket = GetInputBucket(whiteKingSq, Side::White);
	const int bucketCacheIndex = inputBucket + (GetSquareFile(whiteKingSq) >= 4 ? InputBucketCount : 0);
	const int whiteKingFile = GetSquareFile(whiteKingSq);
	BucketCacheItem& cache = BucketCache[0][bucketCacheIndex];

	// Calculate the feature boolean array for the current position
	std::array<uint64_t, 12> featureBits;
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

	// Compare it with the cached entry
	StaticVector<int, 32> adds{};
	StaticVector<int, 32> subs{};
	for (int i = 0; i < 12; i++) {
		uint64_t toBeAdded = featureBits[i] & ~cache.featureBits[i];
		uint64_t toBeSubbed = cache.featureBits[i] & ~featureBits[i];

		while (toBeAdded) {
			const uint8_t sq = Popsquare(toBeAdded);
			const int feature = ((whiteKingFile < 4) ? sq : (sq ^ 7)) + i * 64;
			adds.push(feature);
			for (int i = 0; i < HiddenSize; i++) cache.cachedAcc[i] += Network->FeatureWeights[inputBucket][feature][i];
		}

		while (toBeSubbed) {
			const uint8_t sq = Popsquare(toBeSubbed);
			const int feature = ((whiteKingFile < 4) ? sq : (sq ^ 7)) + i * 64;
			subs.push(feature);
			for (int i = 0; i < HiddenSize; i++) cache.cachedAcc[i] -= Network->FeatureWeights[inputBucket][feature][i];
		}
	}
	//cout << "+" << (int)adds.size() << "  -" << (int)subs.size() << endl;
	cache.featureBits = featureBits;
	AccumulatorStack[accIndex].WhiteAccumulator = cache.cachedAcc;
	AccumulatorStack[accIndex].Correct[Side::White] = true;
	AccumulatorStack[accIndex].KingSquare[Side::White] = whiteKingSq;
	AccumulatorStack[accIndex].ActiveBucket[Side::White] = inputBucket;
}

// Evaluate call ----------------------------------------------------------------------------------

int16_t EvaluationState::Evaluate(const Position& pos) {

	// For evaluating, we need to make sure the accumulator is up-to-date for both sides
	// If we need a refresh, it's important to know, that the accumulator stack and the position stack are indexed differently
	const int basePositionIndex = pos.States.size() - CurrentIndex - 1;

	// Update white accumulators
	if (!AccumulatorStack[CurrentIndex].Correct[Side::White]) {
		const std::optional<int> latestUpdated = [&] {
			for (int i = CurrentIndex; i >= 0; i--) {
				if (AccumulatorStack[i].Correct[Side::White]) return std::optional<int>(i);
				if (AccumulatorStack[i].movedPiece == Piece::WhiteKing && IsRefreshRequired(AccumulatorStack[i].move, Side::White)) {
					return std::optional<int>(std::nullopt);
				}
			}
			assert(false);
		}();

		if (latestUpdated.has_value()) {
			for (int i = latestUpdated.value() + 1; i <= CurrentIndex; i++) {
				AccumulatorStack[i].UpdateIncrementally(Side::White, AccumulatorStack[i - 1]);
			}
		}
		else {
			UpdateFromBucketCache(pos, CurrentIndex, Side::White);
		}
	}

	// Update black accumulators
	if (!AccumulatorStack[CurrentIndex].Correct[Side::Black]) {
		const int latestUpdated = [&] {
			for (int i = CurrentIndex - 1; i >= 0; i--) {
				if (AccumulatorStack[i].Correct[Side::Black]) return i;
			}
			assert(false);
		}();

		for (int i = latestUpdated + 1; i <= CurrentIndex; i++) {
			if (AccumulatorStack[i].movedPiece == Piece::BlackKing && IsRefreshRequired(AccumulatorStack[i].move, Side::Black)) {
				AccumulatorStack[i].RefreshBlack(pos.States[basePositionIndex + i]);
			}
			else {
				AccumulatorStack[i].UpdateIncrementally(Side::Black, AccumulatorStack[i - 1]);
			}
		}




	}

	//int good = NeuralEvaluate(pos);
	//int bad = NeuralEvaluate(pos, AccumulatorStack[CurrentIndex]);
	//assert(good == bad);

	return NeuralEvaluate(pos, AccumulatorStack[CurrentIndex]);
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