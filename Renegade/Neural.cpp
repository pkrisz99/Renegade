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
const AlignedNetworkRepresentation* Network;
std::unique_ptr<AlignedNetworkRepresentation> ExternalNetwork;

// Evaluating the position ------------------------------------------------------------------------

int NeuralEvaluate(const Position& position, const AccumulatorRepresentation& acc) {
	const bool turn = position.Turn();
	const std::array<int16_t, L1Size>& hiddenFriendly = (turn == Side::White) ? acc.White : acc.Black;
	const std::array<int16_t, L1Size>& hiddenOpponent = (turn == Side::White) ? acc.Black : acc.White;
	int32_t output = 0;

	alignas(64) std::array<float, L3Size> l2Out = Network->L2Biases;


	// Clipped ReLU
	auto CReLU = [](const int16_t value) {
		return std::clamp<int32_t>(value, 0, QA);
		};

	// Activate features, perform pairwise
	alignas(64) std::array<int32_t, L1Size> ftOut = {};
	for (int i = 0; i < L1Size / 2; i++) {
		const auto l = CReLU(hiddenFriendly[i]);
		const auto r = CReLU(hiddenFriendly[L1Size / 2 + i]);
		ftOut[i] = l * r;
	}
	for (int i = 0; i < L1Size / 2; i++) {
		const auto l = CReLU(hiddenOpponent[i]);
		const auto r = CReLU(hiddenOpponent[L1Size / 2 + i]);
		ftOut[i + L1Size / 2] = l * r;
	}

	// Propagate L1
	std::array<int32_t, L2Size> l1Sums = {};
	std::array<float, L2Size> l1Out = {};
	for (int i = 0; i < L1Size; i++) {
		for (int j = 0; j < L2Size; j++) {
			l1Sums[j] += ftOut[i] * Network->L1Weights[i][j];
		}
	}
	constexpr float hmm = 1.f / static_cast<float>(QA * QA * QB);
	for (int i = 0; i < L2Size; i++) {
		auto x = std::fma(static_cast<float>(l1Sums[i]), hmm, Network->L1Biases[i]);
		x = std::clamp(x, 0.f, 1.f);
		l1Out[i] = x * x;
	}

	// Propagate L2
	std::array<float, L3Size> l2Sums = Network->L2Biases;
	for (int i = 0; i < L2Size; i++) {
		for (int j = 0; j < L3Size; j++) {
			l2Sums[j] = std::fma(l1Out[i], Network->L2Weights[i][j], l2Sums[j]);
		}
	}
	for (int i = 0; i < L3Size; i++) {
		auto x = std::clamp(l2Sums[i], 0.f, 1.f);
		l2Out[i] = x * x;
	}

	// Propagate L3
	float l3Sums = 0.f;
	for (int i = 0; i < L3Size; i++) {
		l3Sums = std::fma(l2Out[i], Network->L3Weights[i], l3Sums);
	}

	float l3Out = l3Sums + Network->L3Bias;

	output = l3Out * Scale;

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
	}

	// Case 3: Copy the previous state over - normal incremental update
	newAcc = oldAcc;
	const uint8_t whiteKingSq = pos.WhiteKingSquare();
	const uint8_t blackKingSq = pos.BlackKingSquare();

	// No longer activate the previous position of the moved piece
	newAcc.RemoveFeature(FeatureIndexes(movedPiece, m.from, whiteKingSq, blackKingSq));

	// No longer activate the position of the captured piece (if any)
	if (capturedPiece != Piece::None) {
		newAcc.RemoveFeature(FeatureIndexes(capturedPiece, m.to, whiteKingSq, blackKingSq));
	}

	// Activate the new position of the moved piece
	if (!m.IsPromotion() && !m.IsCastling()) {
		newAcc.AddFeature(FeatureIndexes(movedPiece, m.to, whiteKingSq, blackKingSq));
	}
	else if (m.IsPromotion()) {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		newAcc.AddFeature(FeatureIndexes(promotionPiece, m.to, whiteKingSq, blackKingSq));
	}

	// Special cases
	switch (m.flag) {
	case MoveFlag::None: break;

	case MoveFlag::ShortCastle:
		if (ColorOfPiece(movedPiece) == PieceColor::White) {
			newAcc.AddFeature(FeatureIndexes(Piece::WhiteKing, Squares::G1, whiteKingSq, blackKingSq));
			newAcc.AddFeature(FeatureIndexes(Piece::WhiteRook, Squares::F1, whiteKingSq, blackKingSq));
		}
		else {
			newAcc.AddFeature(FeatureIndexes(Piece::BlackKing, Squares::G8, whiteKingSq, blackKingSq));
			newAcc.AddFeature(FeatureIndexes(Piece::BlackRook, Squares::F8, whiteKingSq, blackKingSq));
		}
		break;

	case MoveFlag::LongCastle:
		if (ColorOfPiece(movedPiece) == PieceColor::White) {
			newAcc.AddFeature(FeatureIndexes(Piece::WhiteKing, Squares::C1, whiteKingSq, blackKingSq));
			newAcc.AddFeature(FeatureIndexes(Piece::WhiteRook, Squares::D1, whiteKingSq, blackKingSq));
		}
		else {
			newAcc.AddFeature(FeatureIndexes(Piece::BlackKing, Squares::C8, whiteKingSq, blackKingSq));
			newAcc.AddFeature(FeatureIndexes(Piece::BlackRook, Squares::D8, whiteKingSq, blackKingSq));
		}
		break;

	case MoveFlag::EnPassantPerformed:
		if (movedPiece == Piece::WhitePawn) newAcc.RemoveFeature(FeatureIndexes(Piece::BlackPawn, m.to - 8, whiteKingSq, blackKingSq));
		else newAcc.RemoveFeature(FeatureIndexes(Piece::WhitePawn, m.to + 8, whiteKingSq, blackKingSq));
		break;
	}
}

// Loading an external network (MSVC fallback) ----------------------------------------------------

void LoadExternalNetwork(const std::string& filename) {

	std::ifstream ifs(filename, std::ios::binary);
	if (!ifs) {
		cout << "Failed to load network: " << filename << endl;
		return;
	}

	std::unique_ptr<UnalignedNetworkRepresentation> loaded = std::make_unique<UnalignedNetworkRepresentation>();
	ifs.read((char*)loaded.get(), sizeof(UnalignedNetworkRepresentation));

	std::unique_ptr<AlignedNetworkRepresentation> aligned = std::make_unique<AlignedNetworkRepresentation>();

	aligned->FeatureWeights = loaded->FeatureWeights;
	aligned->FeatureBias = loaded->FeatureBias;
	aligned->L1Weights = loaded->L1Weights;
	aligned->L1Biases = loaded->L1Biases;
	aligned->L2Weights = loaded->L2Weights;
	aligned->L2Biases = loaded->L2Biases;
	aligned->L3Weights = loaded->L3Weights;
	aligned->L3Bias = loaded->L3Bias;

	std::swap(ExternalNetwork, aligned);
	Network = ExternalNetwork.get();

	const Position pos{};
	const int startposEval = NeuralEvaluate(pos);
	if (std::abs(startposEval) < 300 && startposEval != 0) cout << "Loaded '" << filename << "' external network probably successfully";
	else cout << "Loaded '" << filename << "', but it stinks";
	cout << " (startpos raw eval: " << startposEval << ")" << endl;
}

void LoadDefaultNetwork() {
	ExternalNetwork.reset();
	LoadExternalNetwork(NETWORK_NAME);

#if !!defined(_MSC_VER) || defined(__clang__)
	// Include binary in the executable file via incbin (good)
	auto temp1 = reinterpret_cast<const UnalignedNetworkRepresentation*>(gDefaultNetworkData);
	//Network = reinterpret_cast<const AlignedNetworkRepresentation*>(gDefaultNetworkData);

#else
	// Load network file from disk at runtime (bad)
	
#endif
}