// Logic for evaluating endgames

#include "Position.h"
#include "Utils.h"

static int16_t EvaluateKRvK(const Position& pos) {
	// Triggered when there are 3 pieces, two kings and one rook
	const bool turn = pos.Turn();
	const bool stmWinning = (turn == Side::White && (Popcount(pos.GetOccupancy(Side::White)) == 2)) || (turn == Side::Black && (Popcount(pos.GetOccupancy(Side::Black)) == 2));
	const uint8_t winningKingSq = (turn == Side::White) ? pos.WhiteKingSquare() : pos.BlackKingSquare();
	const uint8_t winningRookSq = (turn == Side::White) ? LsbSquare(pos.WhiteRookBits()) : LsbSquare(pos.BlackRookBits());
	const uint8_t losingKingSq = (turn == Side::White) ? pos.BlackKingSquare() : pos.WhiteKingSquare();

	// This should be easily winning - progress is made by pushing the losing king to the edge
	// Dumb scenarios (stalemate, losing the rook) should be handled by the search
	const int losingKingRank = GetSquareRank(losingKingSq);
	const int losingKingFile = GetSquareFile(losingKingSq);
	const int distanceToEnd = std::min(losingKingRank, 7 - losingKingRank);
	const int distanceToSide = std::min(losingKingFile, 7 - losingKingFile);
	const int progress = std::max(8 - distanceToEnd - distanceToSide, 1);
	return (30000 - progress) * (stmWinning ? 1 : -1);
}

static int16_t EvaluateKQvK(const Position& pos) {
	// Triggered when there are 3 pieces, two kings and one queen
	const bool turn = pos.Turn();
	const bool stmWinning = (turn == Side::White && (Popcount(pos.GetOccupancy(Side::White)) == 2)) || (turn == Side::Black && (Popcount(pos.GetOccupancy(Side::Black)) == 2));
	const uint8_t winningKingSq = (turn == Side::White) ? pos.WhiteKingSquare() : pos.BlackKingSquare();
	const uint8_t winningPawnSq = (turn == Side::White) ? LsbSquare(pos.WhitePawnBits()) : LsbSquare(pos.BlackPawnBits());
	const uint8_t losingKingSq = (turn == Side::White) ? pos.BlackKingSquare() : pos.WhiteKingSquare();

	const int losingKingRank = GetSquareRank(losingKingSq);
	const int losingKingFile = GetSquareFile(losingKingSq);
	const int distanceToEnd = std::min(losingKingRank, 7 - losingKingRank);
	const int distanceToSide = std::min(losingKingFile, 7 - losingKingFile);
	const int progress = std::max(8 - distanceToEnd - distanceToSide, 1);
	return (30000 - progress) * (stmWinning ? 1 : -1);
}

static std::optional<int16_t> EvaluateSpecificEndgame(const Position& pos) {
	assert(Popcount(pos.GetOccupancy()) == 3);
	const int rookCount = Popcount(pos.WhiteRookBits() | pos.BlackRookBits());
	const int queenCount = Popcount(pos.WhiteQueenBits() | pos.BlackQueenBits());
	if (rookCount == 1) return std::optional<int16_t>(EvaluateKRvK(pos));
	if (queenCount == 1) return std::optional<int16_t>(EvaluateKQvK(pos));
	return std::nullopt;
}