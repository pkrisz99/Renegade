#include "Neurals.h"

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

INCBIN(DefaultNetwork, "renegade-net-10.bin");
const NetworkRepresentation* Network;
std::unique_ptr<NetworkRepresentation> ExternalNetwork;

int NeuralEvaluate2(const AccumulatorRepresentation& acc, const bool turn) {
	const std::array<int16_t, HiddenSize>& hiddenFriendly = (turn == Turn::White) ? acc.White : acc.Black;
	const std::array<int16_t, HiddenSize>& hiddenOpponent = (turn == Turn::White) ? acc.Black : acc.White;
	int32_t output = 0;

	// Constants
	constexpr int scale = 400;
	constexpr int qa = 181;
	constexpr int qb = 64;
	constexpr int q = qa * qb;
	constexpr int chunkSize = 16;

	
	// Calculate output

	// This uses qa=181, a trick to make SIMD more efficient
	// Idea by JW (akimbo)

	auto sum = _mm256_setzero_si256();
	for (int i = 0; i < (HiddenSize / chunkSize); i++) {
		const auto v = SquareClippedReLU(_mm256_load_si256((__m256i*) &hiddenFriendly[chunkSize * i]));
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[chunkSize * i]);
		const auto p = _mm256_madd_epi16(v, w);
		sum = _mm256_add_epi32(sum, p);
	}
	output = ExtractInteger(sum);

	sum = _mm256_setzero_si256();
	for (int i = 0; i < (HiddenSize / chunkSize); i++) {
		const auto v = SquareClippedReLU(_mm256_load_si256((__m256i*) &hiddenOpponent[chunkSize * i]));
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[chunkSize * i + HiddenSize]);
		const auto p = _mm256_madd_epi16(v, w);
		sum = _mm256_add_epi32(sum, p);
	}
	output += ExtractInteger(sum);


	//for (int i = 0; i < HiddenSize; i++) output += SquareClippedReLU(hiddenFriendly[i]) * Network->OutputWeights[i];
	//for (int i = 0; i < HiddenSize; i++) output += SquareClippedReLU(hiddenOpponent[i]) * Network->OutputWeights[i + HiddenSize];

	output = (output / qa + Network->OutputBias) * scale / q; // Square clipped relu
	//output = (output + Network->OutputBias) * scale / q; // Clipped relu

	return std::clamp(output, -MateThreshold + 1, MateThreshold - 1);
}

int NeuralEvaluate(const Board& board) {
	
	// Initialize arrays
	alignas(64) std::array<int16_t, HiddenSize> hiddenWhite = std::array<int16_t, HiddenSize>();
	alignas(64) std::array<int16_t, HiddenSize> hiddenBlack = std::array<int16_t, HiddenSize>();
	for (int i = 0; i < HiddenSize; i++) hiddenWhite[i] = Network->FeatureBias[i];
	for (int i = 0; i < HiddenSize; i++) hiddenBlack[i] = Network->FeatureBias[i];

	AccumulatorRepresentation acc{};
	acc.Reset();

	uint64_t bits = board.GetOccupancy();
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		const int piece = board.GetPieceAt(sq);
		acc.UpdateFeature<true>(FeatureIndexes(piece, sq));
	}

	return NeuralEvaluate2(acc, board.Turn);
}

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

	cout << "Read '" << filename << "' network probably successfully (please be careful though)" << endl;
	Board b;
	cout << "Startpos eval: " << NeuralEvaluate(b) << " cp\n" << endl;
}

void LoadDefaultNetwork() {
	ExternalNetwork.reset();
	//LoadExternalNetwork(...);
	Network = reinterpret_cast<const NetworkRepresentation*>(gDefaultNetworkData); // Incbin magic
}