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
	assert(AccumulatorStack[CurrentIndex].WhiteGood);
	assert(AccumulatorStack[CurrentIndex].BlackGood);

	const bool turn = position.Turn();
	const std::array<int16_t, HiddenSize>& hiddenFriendly = (turn == Side::White) ? acc.White : acc.Black;
	const std::array<int16_t, HiddenSize>& hiddenOpponent = (turn == Side::White) ? acc.Black : acc.White;
	int32_t output = 0;

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
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[chunkSize * i]);
		const auto p = _mm256_madd_epi16(v, _mm256_mullo_epi16(v, w));
		sum = _mm256_add_epi32(sum, p);
	}
	output = HorizontalSum(sum);

	sum = _mm256_setzero_si256();
	for (int i = 0; i < (HiddenSize / chunkSize); i++) {
		auto v = _mm256_load_si256((__m256i*) &hiddenOpponent[chunkSize * i]);
		v = _mm256_min_epi16(_mm256_max_epi16(v, min), max);
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[chunkSize * i + HiddenSize]);
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
	for (int i = 0; i < HiddenSize; i++) output += Activation(hiddenFriendly[i]) * Network->OutputWeights[i];
	for (int i = 0; i < HiddenSize; i++) output += Activation(hiddenOpponent[i]) * Network->OutputWeights[i + HiddenSize];
#endif

	constexpr int Q = QA * QB;
	output = (output / QA + Network->OutputBias) * Scale / Q; // for SCReLU

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
	if (side == Side::White) WhiteGood = true;
	else BlackGood = true;

	// Copy over the previous state (possible future optimization by deferring this and adding the accumulator change?)
	if (side == Side::White) White = oldAcc.White;
	else Black = oldAcc.Black;
	
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

// Evaluate call ----------------------------------------------------------------------------------

int16_t EvaluationState::Evaluate(const Position& pos) {

	// For evaluating, we need to make sure the accumulator is up-to-date for both sides
	// If we need a refresh, it's important to know, that the accumulator stack and the position stack are indexed differently
	const int basePositionIndex = pos.States.size() - CurrentIndex - 1;

	// Update white accumulators
	if (!AccumulatorStack[CurrentIndex].WhiteGood) {
		const int latestUpdated = [&] {
			for (int i = CurrentIndex - 1; i >= 0; i--) {
				if (AccumulatorStack[i].WhiteGood) return i;
			}
			assert(false);
		}();

		for (int i = latestUpdated + 1; i <= CurrentIndex; i++) {
			if (AccumulatorStack[i].movedPiece == Piece::WhiteKing && IsRefreshRequired(AccumulatorStack[i].move, Side::White)) {
				AccumulatorStack[i].RefreshWhite(pos.States[basePositionIndex + i]);
			}
			else {
				AccumulatorStack[i].UpdateIncrementally(Side::White, AccumulatorStack[i - 1]);
			}
		}
	}

	// Update black accumulators
	if (!AccumulatorStack[CurrentIndex].BlackGood) {
		const int latestUpdated = [&] {
			for (int i = CurrentIndex - 1; i >= 0; i--) {
				if (AccumulatorStack[i].BlackGood) return i;
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