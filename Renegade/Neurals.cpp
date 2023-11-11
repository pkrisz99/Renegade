#include "Neurals.h"

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

void LoadNetwork() {
	Network = new NetworkRepresentation;
	std::ifstream file(NetworkName + ".bin", std::ios::binary);
	file.read((char*)Network, sizeof(NetworkRepresentation));
}