#include "Evaluation.h"

int EvaluateBoard(const Board& board, const EvaluationFeatures& weights) {

	// Renegade's evaluation function

	TaperedScore taperedScore = S(0, 0);
	const uint64_t occupancy = board.GetOccupancy();
	const uint64_t whitePieces = board.GetOccupancy(PieceColor::White);
	const uint64_t blackPieces = board.GetOccupancy(PieceColor::Black);
	const float phase = CalculateGamePhase(board);
	const uint64_t whitePawnAttacks = board.GetWhitePawnAttacks();
	const uint64_t blackPawnAttacks = board.GetBlackPawnAttacks();
	uint64_t whiteAttacks = 0, blackAttacks = 0;

	int whiteDangerScore = 0;
	int blackDangerScore = 0;
	int whiteDangerPieces = 0;
	int blackDangerPieces = 0;

	const int whiteKingSquare = LsbSquare(board.WhiteKingBits);
	const uint64_t whiteKingZone = KingArea[whiteKingSquare];
	const int blackKingSquare = LsbSquare(board.BlackKingBits);
	const uint64_t blackKingZone = KingArea[blackKingSquare];

	uint64_t piecesOnBoard = occupancy;
	while (piecesOnBoard != 0) {
		const int sq = Popsquare(piecesOnBoard);
		const int piece = board.GetPieceAt(sq);
		const int pieceType = TypeOfPiece(piece);
		const int pieceColor = ColorOfPiece(piece);
		const int file = GetSquareFile(sq);
		const int rank = GetSquareRank(sq);
		uint64_t mobility = 0, attacks = 0;

		// Material and piece-square tables
		if (pieceColor == PieceColor::White) {
			taperedScore += weights.GetMaterial(pieceType);
			taperedScore += weights.GetPSQT(pieceType, sq);
		}
		else {
			taperedScore -= weights.GetMaterial(pieceType);
			taperedScore -= weights.GetPSQT(pieceType, Mirror[sq]);
		}

		// Piece-specific evaluation
		switch (piece) {

		case Piece::WhitePawn:
			// Get attacks & update king danger scores
			attacks = WhitePawnAttacks[sq] & ~whitePieces;
			whiteAttacks |= attacks;
			if ((blackKingZone & attacks) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Pawn];
				blackDangerPieces += 1;
			}
			// Isolated pawn evaluation
			if ((board.WhitePawnBits & IsolatedPawnMask[file]) == 0) {
				taperedScore += weights.GetIsolatedPawnEval(file);
			}
			// Passed pawn evaluation
			if (((WhitePassedPawnMask[sq] & board.BlackPawnBits) == 0) && ((WhitePassedPawnFilter[sq] & board.WhitePawnBits) == 0)) {
				taperedScore += weights.GetPassedPawnEval(sq);
				if (SquareBits[sq + 8] & blackPieces) taperedScore += weights.GetBlockedPasserEval(rank);
			}
			// Threats
			if (attacks & board.BlackKnightBits) taperedScore += weights.GetPawnThreat(PieceType::Knight) * Popcount(attacks & board.BlackKnightBits);
			if (attacks & board.BlackBishopBits) taperedScore += weights.GetPawnThreat(PieceType::Bishop) * Popcount(attacks & board.BlackBishopBits);
			if (attacks & board.BlackRookBits) taperedScore += weights.GetPawnThreat(PieceType::Rook) * Popcount(attacks & board.BlackRookBits);
			if (attacks & board.BlackQueenBits) taperedScore += weights.GetPawnThreat(PieceType::Queen) * Popcount(attacks & board.BlackQueenBits);
			// Pawn is supported?
			if (whitePawnAttacks & SquareBits[sq]) taperedScore += weights.GetPawnSupportingPawn(rank);
			// Pawn phalanx
			if ((file != 7) && (board.GetPieceAt(sq + 1) == Piece::WhitePawn)) taperedScore += weights.GetPawnPhalanx(rank);
			break;

		case Piece::BlackPawn:
			// Get attacks & update king danger scores
			attacks = BlackPawnAttacks[sq] & ~blackPieces;
			blackAttacks |= attacks;
			if ((whiteKingZone & attacks) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Pawn];
				whiteDangerPieces += 1;
			}
			// Isolated pawn evaluation
			if ((board.BlackPawnBits & IsolatedPawnMask[file]) == 0) {
				taperedScore -= weights.GetIsolatedPawnEval(7 - file);
			}
			// Passed pawn evaluation
			if (((BlackPassedPawnMask[sq] & board.WhitePawnBits) == 0) && ((BlackPassedPawnFilter[sq] & board.BlackPawnBits) == 0)) {
				taperedScore -= weights.GetPassedPawnEval(Mirror[sq]);
				if (SquareBits[sq - 8] & whitePieces) taperedScore -= weights.GetBlockedPasserEval(7 - rank);
			}
			// Threats
			if (attacks & board.WhiteKnightBits) taperedScore -= weights.GetPawnThreat(PieceType::Knight) * Popcount(attacks & board.WhiteKnightBits);
			if (attacks & board.WhiteBishopBits) taperedScore -= weights.GetPawnThreat(PieceType::Bishop) * Popcount(attacks & board.WhiteBishopBits);
			if (attacks & board.WhiteRookBits) taperedScore -= weights.GetPawnThreat(PieceType::Rook) * Popcount(attacks & board.WhiteRookBits);
			if (attacks & board.WhiteQueenBits) taperedScore -= weights.GetPawnThreat(PieceType::Queen) * Popcount(attacks & board.WhiteQueenBits);
			// Pawn is supported?
			if (blackPawnAttacks & SquareBits[sq]) taperedScore -= weights.GetPawnSupportingPawn(7 - rank);
			// Pawn phalanx
			if ((file != 7) && (board.GetPieceAt(sq + 1) == Piece::BlackPawn)) taperedScore -= weights.GetPawnPhalanx(7 - rank);
			break;

		case Piece::WhiteKnight:
			// Get attacks, mobility & update king danger scores
			attacks = KnightMoveBits[sq];
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetKnightMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Knight];
				blackDangerPieces += 1;
			}
			// Knight outposts
			if (OutpostFilter[sq]) {
				if ((board.GetPieceAt(sq - 7) == Piece::WhitePawn) || (board.GetPieceAt(sq - 9) == Piece::WhitePawn)) {
					taperedScore += weights.GetKnightOutpostEval();
				}
			}
			// Threats
			taperedScore += weights.GetKnightThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			taperedScore += weights.GetKnightThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			taperedScore += weights.GetKnightThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			taperedScore += weights.GetKnightThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;

		case Piece::BlackKnight:
			// Get attacks, mobility & update king danger scores
			attacks = KnightMoveBits[sq];
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetKnightMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Knight];
				whiteDangerPieces += 1;
			}
			// Knight outposts
			if (OutpostFilter[sq]) {
				if ((board.GetPieceAt(sq + 7) == Piece::BlackPawn) || (board.GetPieceAt(sq + 9) == Piece::BlackPawn)) {
					taperedScore -= weights.GetKnightOutpostEval();
				}
			}
			// Threats
			taperedScore -= weights.GetKnightThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			taperedScore -= weights.GetKnightThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			taperedScore -= weights.GetKnightThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			taperedScore -= weights.GetKnightThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;

		case Piece::WhiteBishop:
			// Get attacks, mobility & update king danger scores
			attacks = GetBishopAttacks(sq, occupancy);
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetBishopMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Bishop];
				blackDangerPieces += 1;
			}
			// Threats
			taperedScore += weights.GetBishopThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			taperedScore += weights.GetBishopThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			taperedScore += weights.GetBishopThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			taperedScore += weights.GetBishopThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;

		case Piece::BlackBishop:
			// Get attacks, mobility & update king danger scores
			attacks = GetBishopAttacks(sq, occupancy);
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetBishopMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Bishop];
				whiteDangerPieces += 1;
			}
			// Threats
			taperedScore -= weights.GetBishopThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			taperedScore -= weights.GetBishopThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			taperedScore -= weights.GetBishopThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			taperedScore -= weights.GetBishopThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;

		case Piece::WhiteRook:
			// Get attacks, mobility & update king danger scores
			attacks = GetRookAttacks(sq, occupancy);
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetRookMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Rook];
				blackDangerPieces += 1;
			}
			// Rook on open or semi-open file
			if ((board.WhitePawnBits & Files[file]) == 0) {
				if ((board.BlackPawnBits & Files[file]) == 0) { // open file
					taperedScore += weights.GetRookOnOpenFileBonus(sq);
				}
				else { // semi-open file
					taperedScore += weights.GetRookOnSemiOpenFileBonus(sq);
				}
			}
			// Threats
			taperedScore += weights.GetRookThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			taperedScore += weights.GetRookThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			taperedScore += weights.GetRookThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			taperedScore += weights.GetRookThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;

		case Piece::BlackRook:
			// Get attacks, mobility & update king danger scores
			attacks = GetRookAttacks(sq, occupancy);
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetRookMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Rook];
				whiteDangerPieces += 1;
			}
			// Rook on open or semi-open file
			if ((board.BlackPawnBits & Files[file]) == 0) {
				if ((board.WhitePawnBits & Files[file]) == 0) { // open file
					taperedScore -= weights.GetRookOnOpenFileBonus(Mirror[sq]);
				}
				else { // semi-open file
					taperedScore -= weights.GetRookOnSemiOpenFileBonus(Mirror[sq]);
				}
			}
			// Threats
			taperedScore -= weights.GetRookThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			taperedScore -= weights.GetRookThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			taperedScore -= weights.GetRookThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			taperedScore -= weights.GetRookThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;

		case Piece::WhiteQueen:
			// Get attacks, mobility & update king danger scores
			attacks = GetQueenAttacks(sq, occupancy);
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetQueenMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Queen];
				blackDangerPieces += 1;
			}
			// Threats
			taperedScore += weights.GetQueenThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			taperedScore += weights.GetQueenThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			taperedScore += weights.GetQueenThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			taperedScore += weights.GetQueenThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			break;

		case Piece::BlackQueen:
			// Get attacks, mobility & update king danger scores
			attacks = GetQueenAttacks(sq, occupancy);
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetQueenMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Queen];
				whiteDangerPieces += 1;
			}
			// Threats
			taperedScore -= weights.GetQueenThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			taperedScore -= weights.GetQueenThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			taperedScore -= weights.GetQueenThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			taperedScore -= weights.GetQueenThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			break;

		case Piece::WhiteKing:
			attacks = KingMoveBits[sq];
			mobility = attacks & ~whitePieces;
			// King on open or semi-open file
			if ((board.WhitePawnBits & Files[file]) == 0) {
				if ((board.BlackPawnBits & Files[file]) == 0) { // open file
					taperedScore += weights.GetKingOnOpenFileEval(sq);
				}
				else { // semi-open file
					taperedScore += weights.GetKingOnSemiOpenFileEval(sq);
				}
			}
			// Threats
			taperedScore += weights.GetKingThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			taperedScore += weights.GetKingThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			taperedScore += weights.GetKingThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			taperedScore += weights.GetKingThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			taperedScore += weights.GetKingThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;
		case Piece::BlackKing:
			attacks = KingMoveBits[sq];
			mobility = attacks & ~blackPieces;
			// King on open or semi-open file
			if ((board.BlackPawnBits & Files[file]) == 0) {
				if ((board.WhitePawnBits & Files[file]) == 0) { // open file
					taperedScore -= weights.GetKingOnOpenFileEval(Mirror[sq]);
				}
				else { // semi-open file
					taperedScore -= weights.GetKingOnSemiOpenFileEval(Mirror[sq]);
				}
			}
			// Threats
			taperedScore -= weights.GetKingThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			taperedScore -= weights.GetKingThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			taperedScore -= weights.GetKingThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			taperedScore -= weights.GetKingThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			taperedScore -= weights.GetKingThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;
		}
	}

	// King safety
	const int whiteKingSafetyFinal = whiteDangerScore * weights.DangerMultipliers[std::min(whiteDangerPieces, 7)] / 100;
	const int blackKingSafetyFinal = blackDangerScore * weights.DangerMultipliers[std::min(blackDangerPieces, 7)] / 100;
	if (whiteKingSafetyFinal != 0) taperedScore += weights.GetKingDanger(std::min(whiteKingSafetyFinal, 25));
	if (blackKingSafetyFinal != 0) taperedScore -= weights.GetKingDanger(std::min(blackKingSafetyFinal, 25));

	// Bishop pair bonus
	if (Popcount(board.WhiteBishopBits) >= 2) taperedScore += weights.GetBishopPairEval();
	if (Popcount(board.BlackBishopBits) >= 2) taperedScore -= weights.GetBishopPairEval();

	// Doubled & tripled pawn penalties
	for (int i = 0; i < 8; i++) {
		const int whitePawnsOnFile = Popcount(board.WhitePawnBits & Files[i]);
		const int blackPawnsOnFile = Popcount(board.BlackPawnBits & Files[i]);
		if (whitePawnsOnFile == 2) taperedScore += weights.GetDoubledPawnEval();
		else if (whitePawnsOnFile > 2) taperedScore += weights.GetTripledPawnEval();
		if (blackPawnsOnFile == 2) taperedScore -= weights.GetDoubledPawnEval();
		else if (blackPawnsOnFile > 2) taperedScore -= weights.GetTripledPawnEval();
	}

	// Get untapered score
	int score = LinearTaper(taperedScore.early, taperedScore.late, phase);

	// Convert to the correct perspective
	if (!board.Turn) score *= -1;

	// Tempo bonus
	score += LinearTaper(weights.GetTempoBonus().early, weights.GetTempoBonus().late, phase);

	// Drawish endgame
	if (IsDrawishEndgame(board, whitePieces, blackPieces)) score /= 8;

	return score;
}

inline bool IsDrawishEndgame(const Board& board, const uint64_t whitePieces, const uint64_t blackPieces) {
	// Drawish endgame detection
	// To avoid simplifying down to a non-winning endgame with a nominal material advantage
	// This list is not complete, and probably should be expanded even more (e.g. by including pawns)
	// Source: Chess Programming Wiki + https://www.madchess.net/2021/04/08/madchess-3-0-beta-4d22dec-endgame-eval-scaling/
	bool endgame = (Popcount(whitePieces) <= 3) && (Popcount(blackPieces) <= 3);
	if (!endgame) return false;

	// Variables for easy access
	const bool pawnless = (board.WhitePawnBits | board.BlackPawnBits) == 0ULL;
	const bool queenless = (board.WhiteQueenBits | board.BlackQueenBits) == 0ULL;
	const bool queenful = (Popcount(board.WhiteQueenBits | board.BlackQueenBits) > 0)
		&& (Popcount(board.WhiteQueenBits) <= 1) && (Popcount(board.BlackQueenBits) <= 1);
	const bool potentiallyDrawishQueenless = queenless && pawnless && endgame;
	const bool potentiallyDrawishQueenful = queenful && pawnless && endgame;
	const int whiteExtras = Popcount(whitePieces) - 1;
	const int blackExtras = Popcount(blackPieces) - 1;
	const int whiteMinors = Popcount(board.WhiteKnightBits | board.WhiteBishopBits);
	const int blackMinors = Popcount(board.BlackKnightBits | board.BlackBishopBits);
	const int whiteKnights = Popcount(board.WhiteKnightBits);
	const int blackKnights = Popcount(board.BlackKnightBits);
	const int whiteBishops = Popcount(board.WhiteBishopBits);
	const int blackBishops = Popcount(board.BlackBishopBits);
	const int whiteRooks = Popcount(board.WhiteRookBits);
	const int blackRooks = Popcount(board.BlackRookBits);

	// Endgames with no queens
	if (potentiallyDrawishQueenless) {
		bool drawish =
			// 2 minor pieces (no bishop pair) vs 1 minor piece
			((whiteExtras == 2) && (blackExtras == 1) && (whiteMinors == 2) && (whiteBishops != 2) && (blackMinors == 1)) ||
			((whiteExtras == 1) && (blackExtras == 2) && (blackMinors == 2) && (whiteMinors == 1) && (blackBishops != 2)) ||
			// 2 knights vs king
			((whiteExtras == 0) && (blackExtras == 2) && (blackKnights == 2)) ||
			((whiteExtras == 2) && (blackExtras == 0) && (whiteKnights == 2)) ||
			// minor piece vs minor piece
			((whiteExtras == 1) && (blackExtras == 1) && (whiteMinors == 1) && (blackMinors == 1)) ||
			// minor piece vs 2 knights
			((whiteExtras == 1) && (blackExtras == 2) && (whiteMinors == 1) && (blackKnights == 2)) ||
			((whiteExtras == 2) && (blackExtras == 1) && (whiteKnights == 2) && (blackMinors == 1)) ||
			// 2 bishop vs 1 bishop
			((whiteExtras == 2) && (blackExtras == 1) && (whiteBishops == 2) && (blackBishops == 1)) ||
			((whiteExtras == 1) && (blackExtras == 2) && (whiteBishops == 1) && (blackBishops == 2)) ||
			// rook vs rook
			((whiteExtras == 1) && (blackExtras == 1) && (whiteRooks == 1) && (blackRooks == 1)) ||
			// 2 rooks vs 2 rooks
			((whiteExtras == 1) && (blackExtras == 1) && (whiteRooks == 2) && (blackRooks == 2)) ||
			// rook vs rook + minor piece
			((whiteExtras == 2) && (blackExtras == 1) && (whiteRooks == 1) && (blackRooks == 1) && (whiteMinors == 1)) ||
			((whiteExtras == 1) && (blackExtras == 2) && (whiteRooks == 1) && (blackRooks == 1) && (blackMinors == 1)) ||
			// 2 rooks vs rook + minor
			((whiteExtras == 2) && (blackExtras == 2) && (whiteRooks == 2) && (blackRooks == 1) && (blackMinors == 1)) ||
			((whiteExtras == 2) && (blackExtras == 2) && (whiteRooks == 1) && (blackRooks == 2) && (whiteMinors == 1));
		if (drawish) return true;
	}

	if (potentiallyDrawishQueenful) {
		int whiteQueens = Popcount(board.WhiteQueenBits);
		int blackQueens = Popcount(board.BlackQueenBits);
		bool drawish =
			// queen vs queen
			((whiteExtras == 1) && (blackExtras == 1) && (whiteQueens == 1) && (blackQueens == 1)) ||
			// queen vs 2 bishops
			((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackBishops == 2)) ||
			((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteBishops == 2)) ||
			// queen vs 2 knights
			((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackKnights == 2)) ||
			((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteKnights == 2)) ||
			// queen vs 2 rooks
			((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackRooks == 2)) ||
			((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteRooks == 2)) ||
			// queen vs rook + minor
			((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackRooks == 1) && (blackMinors == 1)) ||
			((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteRooks == 1) && (whiteMinors == 1));
		if (drawish) return true;
	}
	return false;
}