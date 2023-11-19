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

INCBIN(DefaultNetwork, "renegade-net-6.bin");
const NetworkRepresentation* Network;
std::unique_ptr<NetworkRepresentation> ExternalNetwork;


int NeuralEvaluate(const Board& board) {
	
	// Initialize arrays
	alignas(64) std::array<int16_t, HiddenSize> hiddenWhite = std::array<int16_t, HiddenSize>();
	alignas(64) std::array<int16_t, HiddenSize> hiddenBlack = std::array<int16_t, HiddenSize>();
	for (int i = 0; i < HiddenSize; i++) hiddenWhite[i] = (Network->FeatureBias[i]);
	for (int i = 0; i < HiddenSize; i++) hiddenBlack[i] = (Network->FeatureBias[i]);

	// Iterate through inputs
	uint64_t occupancy = board.GetOccupancy();
	while (occupancy) {
		const int sq = Popsquare(occupancy);
		const int piece = board.GetPieceAt(sq);
		const int pieceType = TypeOfPiece(piece);
		const int pieceColor = ColorOfPiece(piece);
		const int colorOffset = 64 * 6;

		// Turn on the right inputs
		const int whiteActivationIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + sq;
		const int blackActivationIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq);
		for (int i = 0; i < HiddenSize; i++) hiddenWhite[i] += Network->FeatureWeights[whiteActivationIndex][i];
		for (int i = 0; i < HiddenSize; i++) hiddenBlack[i] += Network->FeatureWeights[blackActivationIndex][i];
	}
	
	// Flip
	std::array<int16_t, HiddenSize>& hiddenFriendly = ((board.Turn == Turn::White) ? hiddenWhite : hiddenBlack);
	std::array<int16_t, HiddenSize>& hiddenOpponent = ((board.Turn == Turn::White) ? hiddenBlack : hiddenWhite);
	int32_t output = 0;

	// Calculate output
	for (int i = 0; i < HiddenSize; i++) output += CReLU(hiddenFriendly[i]) * Network->OutputWeights[i];
	for (int i = 0; i < HiddenSize; i++) output += CReLU(hiddenOpponent[i]) * Network->OutputWeights[i + HiddenSize];
	const int scale = 400;
	const int q = 255 * 64;
	output = (output + Network->OutputBias) * scale / q;

	return output;

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
	Network = reinterpret_cast<const NetworkRepresentation*>(gDefaultNetworkData); // Incbin magic
}