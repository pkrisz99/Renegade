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

INCBIN(DefaultNetwork, NETWORK_NAME);
const NetworkRepresentation* Network;
std::unique_ptr<NetworkRepresentation> ExternalNetwork;

int NeuralEvaluate(const AccumulatorRepresentation& acc, const bool turn) {
	const std::array<int16_t, HiddenSize>& hiddenFriendly = (turn == Side::White) ? acc.White : acc.Black;
	const std::array<int16_t, HiddenSize>& hiddenOpponent = (turn == Side::White) ? acc.Black : acc.White;
	int32_t output = 0;

#ifdef __AVX2__
	// Calculate output with handwritten SIMD (autovec also works, but it's slower)
	// Idea by somelizard, it makes fast QA=255 SCReLU possible (but now the net is still QA=181)
	
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
	output = ExtractInteger(sum);

	sum = _mm256_setzero_si256();
	for (int i = 0; i < (HiddenSize / chunkSize); i++) {
		auto v = _mm256_load_si256((__m256i*) &hiddenOpponent[chunkSize * i]);
		v = _mm256_min_epi16(_mm256_max_epi16(v, min), max);
		const auto w = _mm256_load_si256((__m256i*) &Network->OutputWeights[chunkSize * i + HiddenSize]);
		const auto p = _mm256_madd_epi16(v, _mm256_mullo_epi16(v, w));
		sum = _mm256_add_epi32(sum, p);
	}
	output += ExtractInteger(sum);
#else
	// Slower fallback
	// if you end up here... that's bad.
	for (int i = 0; i < HiddenSize; i++) output += SquareClippedReLU(hiddenFriendly[i]) * Network->OutputWeights[i];
	for (int i = 0; i < HiddenSize; i++) output += SquareClippedReLU(hiddenOpponent[i]) * Network->OutputWeights[i + HiddenSize];
#endif

	constexpr int Q = QA * QB;
	output = (output / QA + Network->OutputBias) * Scale / Q; // Square Clipped ReLU
	//output = (output + Network->OutputBias) * Scale / Q;    // Clipped ReLU

	return std::clamp(output, -MateThreshold + 1, MateThreshold - 1);
}

int NeuralEvaluate(const Position& position) {
	
	// Initialize arrays and accumulators
	alignas(64) std::array<int16_t, HiddenSize> hiddenWhite = std::array<int16_t, HiddenSize>();
	alignas(64) std::array<int16_t, HiddenSize> hiddenBlack = std::array<int16_t, HiddenSize>();
	for (int i = 0; i < HiddenSize; i++) hiddenWhite[i] = Network->FeatureBias[i];
	for (int i = 0; i < HiddenSize; i++) hiddenBlack[i] = Network->FeatureBias[i];
	AccumulatorRepresentation acc{};
	acc.Reset();

	// Iterate through pieces and activate features
	uint64_t bits = position.GetOccupancy();
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		const int piece = position.GetPieceAt(sq);
		acc.AddFeature(FeatureIndexes(piece, sq));
	}

	return NeuralEvaluate(acc, position.Turn());
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