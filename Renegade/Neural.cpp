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
	acc.Refresh(position);
	return NeuralEvaluate(position, acc);
}

// Accumulator updates ----------------------------------------------------------------------------

void UpdateAccumulator(const Position& pos, const AccumulatorRepresentation& oldAcc, AccumulatorRepresentation& newAcc,
	const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece) {

	// Case 1: Handling null moves - just copy it over
	if (m.IsNull()) {
		newAcc = oldAcc;
		return;
	}

	// Case 2: King moves - check if a refresh is necessary
	if (TypeOfPiece(movedPiece) == PieceType::King) {
		const bool side = ColorOfPiece(movedPiece) == PieceColor::White ? Side::White : Side::Black;
		if (IsRefreshRequired(m, side)) {
			newAcc.Refresh(pos);
			return;
		}
		newAcc.SetKingSquare(side, m.to);
	}

	// Case 3: Copy the previous state over - normal incremental update
	newAcc = oldAcc;
	const uint8_t whiteKingSq = pos.WhiteKingSquare();
	const uint8_t blackKingSq = pos.BlackKingSquare();

	// (a) regular non-capture move
	if (capturedPiece == Piece::None && !m.IsPromotion() && m.flag != MoveFlag::EnPassantPerformed) {
		newAcc.RemoveFeature(newAcc.FeatureIndexes(movedPiece, m.from));
		newAcc.AddFeature(newAcc.FeatureIndexes(movedPiece, m.to));
		return;
	}

	// (b) regular capture move
	if (capturedPiece != Piece::None && !m.IsPromotion() && m.flag != MoveFlag::EnPassantPerformed && !m.IsCastling()) {
		newAcc.RemoveFeature(newAcc.FeatureIndexes(movedPiece, m.from));
		newAcc.AddFeature(newAcc.FeatureIndexes(movedPiece, m.to));
		newAcc.RemoveFeature(newAcc.FeatureIndexes(capturedPiece, m.to));
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
		newAcc.RemoveFeature(newAcc.FeatureIndexes(movedPiece, m.from));
		newAcc.RemoveFeature(newAcc.FeatureIndexes(rookPiece, m.to));
		newAcc.AddFeature(newAcc.FeatureIndexes(movedPiece, newKingSquare));
		newAcc.AddFeature(newAcc.FeatureIndexes(rookPiece, newRookSquare));
		return;
	}

	// (d) promotion - with optional capture
	if (m.IsPromotion()) {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		if (capturedPiece == Piece::None) {
			newAcc.AddFeature(newAcc.FeatureIndexes(promotionPiece, m.to));
			newAcc.RemoveFeature(newAcc.FeatureIndexes(movedPiece, m.from));
		}
		else {
			newAcc.AddFeature(newAcc.FeatureIndexes(promotionPiece, m.to));
			newAcc.RemoveFeature(newAcc.FeatureIndexes(movedPiece, m.from));
			newAcc.RemoveFeature(newAcc.FeatureIndexes(capturedPiece, m.to));
		}
		return;
	}

	// (e) en passant
	if (m.flag == MoveFlag::EnPassantPerformed) {
		newAcc.RemoveFeature(newAcc.FeatureIndexes(movedPiece, m.from));
		newAcc.AddFeature(newAcc.FeatureIndexes(movedPiece, m.to));
		if (movedPiece == Piece::WhitePawn) newAcc.RemoveFeature(newAcc.FeatureIndexes(Piece::BlackPawn, m.to - 8));
		else newAcc.RemoveFeature(newAcc.FeatureIndexes(Piece::WhitePawn, m.to + 8));
		return;
	}

}

// Loading an external network (MSVC fallback) ----------------------------------------------------

void LoadExternalNetwork(const std::string& filename) {

	std::ifstream ifs(filename, std::ios::binary);
	if (!ifs) {
		cout << "Failed to load network: " << filename << endl;
		return;
	}

	std::unique_ptr<NetworkRepresentation> loadedNetwork = std::make_unique<NetworkRepresentation>();
	ifs.read((char*)loadedNetwork.get(), sizeof(NetworkRepresentation));
	std::swap(ExternalNetwork, loadedNetwork);
	Network = ExternalNetwork.get();

	const Position pos{};
	const int startposEval = NeuralEvaluate(pos);
	if (std::abs(startposEval) < 300 && startposEval != 0) cout << "Loaded '" << filename << "' external network probably successfully";
	else cout << "Loaded '" << filename << "', but it stinks";
	cout << " (startpos raw eval: " << startposEval << ")" << endl;
}

void LoadDefaultNetwork() {
	ExternalNetwork.reset();
#if !defined(_MSC_VER) || defined(__clang__)
	// Include binary in the executable file via incbin (good)
	Network = reinterpret_cast<const NetworkRepresentation*>(gDefaultNetworkData);
#else
	// Load network file from disk at runtime (bad)
	LoadExternalNetwork(NETWORK_NAME);
#endif
}