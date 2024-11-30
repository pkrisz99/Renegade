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

int NeuralEvaluate(const Position& position, const AccumulatorRepresentation& acc) {
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

int NeuralEvaluate(const Position& position) {
	AccumulatorRepresentation acc{};
	acc.RefreshBoth(position);
	return NeuralEvaluate(position, acc);
}

// Accumulator updates ----------------------------------------------------------------------------

void AccumulatorRepresentation::UpdateFrom(const Position& pos, const AccumulatorRepresentation& oldAcc,
	const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece) {

	// 1. For null-moves nothing changes, we just copy over everything
	if (m.IsNull()) {
		*this = oldAcc;
		return;
	}

	// 2. Determine if we require a refresh (which only can be needed when the king moves)

	const bool side = ColorOfPiece(movedPiece) == PieceColor::White ? Side::White : Side::Black;
	bool keepWhite = true;
	bool keepBlack = true;
	SkipIncrementalForWhite = false;
	SkipIncrementalForBlack = false;

	if (TypeOfPiece(movedPiece) == PieceType::King && IsRefreshRequired(m, side)) {
		if (side == Side::White) keepWhite = false;
		else keepBlack = false;
	}

	if (keepWhite) {
		White = oldAcc.White;
		WhiteBucket = oldAcc.WhiteBucket;
		WhiteKingSquare = pos.WhiteKingSquare();
		SkipIncrementalForWhite = false;
	}
	else {
		SkipIncrementalForWhite = true;
		RefreshWhite(pos);
	}

	if (keepBlack) {
		Black = oldAcc.Black;
		BlackBucket = oldAcc.BlackBucket;
		BlackKingSquare = pos.BlackKingSquare();
		SkipIncrementalForBlack = false;
	}
	else {
		SkipIncrementalForBlack = true;
		RefreshBlack(pos);
	}

	// 3. Perform incremental updates
	
	// (a) regular non-capture move
	if (capturedPiece == Piece::None && !m.IsPromotion() && m.flag != MoveFlag::EnPassantPerformed) {
		SubAddFeature({ movedPiece, m.from }, { movedPiece, m.to });
		return;
	}

	// (b) regular capture move
	if (capturedPiece != Piece::None && !m.IsPromotion() && m.flag != MoveFlag::EnPassantPerformed && !m.IsCastling()) {
		SubSubAddFeature({ movedPiece, m.from }, { capturedPiece, m.to }, { movedPiece, m.to });
		return;
	}

	// (c) castling
	if (m.IsCastling()) {
		const bool side = ColorOfPiece(movedPiece) == PieceColor::White;
		const bool shortCastle = m.flag == MoveFlag::ShortCastle;
		const uint8_t rookPiece = side == Side::White ? Piece::WhiteRook : Piece::BlackRook;
		const uint8_t newKingFile = shortCastle ? 6 : 2;
		const uint8_t newRookFile = shortCastle ? 5 : 3;
		const uint8_t newKingSquare = newKingFile + (side == Side::Black) * 56;
		const uint8_t newRookSquare = newRookFile + (side == Side::Black) * 56;
		SubAddFeature({ movedPiece, m.from }, { movedPiece, newKingSquare });
		SubAddFeature({ rookPiece, m.to }, { rookPiece, newRookSquare });
		return;
	}

	// (d) promotion - with optional capture
	if (m.IsPromotion()) {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		if (capturedPiece == Piece::None) SubAddFeature({ movedPiece, m.from }, { promotionPiece, m.to });
		else SubSubAddFeature({ movedPiece, m.from }, { capturedPiece, m.to }, { promotionPiece, m.to });
		return;
	}

	// (e) en passant
	if (m.flag == MoveFlag::EnPassantPerformed) {
		const uint8_t victimPiece = movedPiece == Piece::WhitePawn ? Piece::BlackPawn : Piece::WhitePawn;
		const uint8_t victimSquare = movedPiece == Piece::WhitePawn ? (m.to - 8) : (m.to + 8);
		SubSubAddFeature({ movedPiece, m.from }, { victimPiece, victimSquare }, { movedPiece, m.to });
		return;
	}
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