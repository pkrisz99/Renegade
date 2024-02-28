#include "Evaluation.h"

int ClassicalEvaluate(const Board& board, const EvaluationFeatures& weights) {

	// Renegade's classical evaluation function
	// This was replaced by a significantly stronger than NNUE evaluation

	TaperedScore materialScore, pqstScore, pawnStructureScore, threatScore, mobilityScore, kingScore;
	const uint64_t occupancy = board.GetOccupancy();
	const uint64_t whitePieces = board.GetOccupancy(Turn::White);
	const uint64_t blackPieces = board.GetOccupancy(Turn::Black);
	const float phase = CalculateGamePhase(board);
	const uint64_t whitePawnAttacks = board.GetPawnAttacks<Turn::White>();
	const uint64_t blackPawnAttacks = board.GetPawnAttacks<Turn::Black>();
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
			materialScore += weights.GetMaterial(pieceType);
			pqstScore += weights.GetPSQT(pieceType, sq);
		}
		else {
			materialScore -= weights.GetMaterial(pieceType);
			pqstScore -= weights.GetPSQT(pieceType, Mirror(sq));
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
				pawnStructureScore += weights.GetIsolatedPawnEval(file);
			}
			// Passed pawn evaluation
			if (((WhitePassedPawnMask[sq] & board.BlackPawnBits) == 0) && ((WhitePassedPawnFilter[sq] & board.WhitePawnBits) == 0)) {
				pawnStructureScore += weights.GetPassedPawnEval(sq);
				if (SquareBit(sq + 8) & blackPieces) pawnStructureScore += weights.GetBlockedPasserEval(rank);
			}
			// Threats
			if (attacks & board.BlackKnightBits) threatScore += weights.GetPawnThreat(PieceType::Knight) * Popcount(attacks & board.BlackKnightBits);
			if (attacks & board.BlackBishopBits) threatScore += weights.GetPawnThreat(PieceType::Bishop) * Popcount(attacks & board.BlackBishopBits);
			if (attacks & board.BlackRookBits) threatScore += weights.GetPawnThreat(PieceType::Rook) * Popcount(attacks & board.BlackRookBits);
			if (attacks & board.BlackQueenBits) threatScore += weights.GetPawnThreat(PieceType::Queen) * Popcount(attacks & board.BlackQueenBits);
			// Pawn is supported?
			if (whitePawnAttacks & SquareBit(sq)) pawnStructureScore += weights.GetPawnSupportingPawn(rank);
			// Pawn phalanx
			if ((file != 7) && (board.GetPieceAt(sq + 1) == Piece::WhitePawn)) pawnStructureScore += weights.GetPawnPhalanx(rank);
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
				pawnStructureScore -= weights.GetIsolatedPawnEval(7 - file);
			}
			// Passed pawn evaluation
			if (((BlackPassedPawnMask[sq] & board.WhitePawnBits) == 0) && ((BlackPassedPawnFilter[sq] & board.BlackPawnBits) == 0)) {
				pawnStructureScore -= weights.GetPassedPawnEval(Mirror(sq));
				if (SquareBit(sq - 8) & whitePieces) pawnStructureScore -= weights.GetBlockedPasserEval(7 - rank);
			}
			// Threats
			if (attacks & board.WhiteKnightBits) threatScore -= weights.GetPawnThreat(PieceType::Knight) * Popcount(attacks & board.WhiteKnightBits);
			if (attacks & board.WhiteBishopBits) threatScore -= weights.GetPawnThreat(PieceType::Bishop) * Popcount(attacks & board.WhiteBishopBits);
			if (attacks & board.WhiteRookBits) threatScore -= weights.GetPawnThreat(PieceType::Rook) * Popcount(attacks & board.WhiteRookBits);
			if (attacks & board.WhiteQueenBits) threatScore -= weights.GetPawnThreat(PieceType::Queen) * Popcount(attacks & board.WhiteQueenBits);
			// Pawn is supported?
			if (blackPawnAttacks & SquareBit(sq)) pawnStructureScore -= weights.GetPawnSupportingPawn(7 - rank);
			// Pawn phalanx
			if ((file != 7) && (board.GetPieceAt(sq + 1) == Piece::BlackPawn)) pawnStructureScore -= weights.GetPawnPhalanx(7 - rank);
			break;

		case Piece::WhiteKnight:
			// Get attacks, mobility & update king danger scores
			attacks = KnightMoveBits[sq];
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights.GetKnightMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Knight];
				blackDangerPieces += 1;
			}
			// Knight outposts
			if (OutpostFilter[sq]) {
				if ((board.GetPieceAt(sq - 7) == Piece::WhitePawn) || (board.GetPieceAt(sq - 9) == Piece::WhitePawn)) {
					threatScore += weights.GetKnightOutpostEval();
				}
			}
			// Threats
			threatScore += weights.GetKnightThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			threatScore += weights.GetKnightThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			threatScore += weights.GetKnightThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			threatScore += weights.GetKnightThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;

		case Piece::BlackKnight:
			// Get attacks, mobility & update king danger scores
			attacks = KnightMoveBits[sq];
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights.GetKnightMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Knight];
				whiteDangerPieces += 1;
			}
			// Knight outposts
			if (OutpostFilter[sq]) {
				if ((board.GetPieceAt(sq + 7) == Piece::BlackPawn) || (board.GetPieceAt(sq + 9) == Piece::BlackPawn)) {
					threatScore -= weights.GetKnightOutpostEval();
				}
			}
			// Threats
			threatScore -= weights.GetKnightThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			threatScore -= weights.GetKnightThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			threatScore -= weights.GetKnightThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			threatScore -= weights.GetKnightThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;

		case Piece::WhiteBishop:
			// Get attacks, mobility & update king danger scores
			attacks = GetBishopAttacks(sq, occupancy);
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights.GetBishopMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Bishop];
				blackDangerPieces += 1;
			}
			// Threats
			threatScore += weights.GetBishopThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			threatScore += weights.GetBishopThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			threatScore += weights.GetBishopThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			threatScore += weights.GetBishopThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;

		case Piece::BlackBishop:
			// Get attacks, mobility & update king danger scores
			attacks = GetBishopAttacks(sq, occupancy);
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights.GetBishopMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Bishop];
				whiteDangerPieces += 1;
			}
			// Threats
			threatScore -= weights.GetBishopThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			threatScore -= weights.GetBishopThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			threatScore -= weights.GetBishopThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			threatScore -= weights.GetBishopThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;

		case Piece::WhiteRook:
			// Get attacks, mobility & update king danger scores
			attacks = GetRookAttacks(sq, occupancy);
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights.GetRookMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Rook];
				blackDangerPieces += 1;
			}
			// Rook on open or semi-open file
			if ((board.WhitePawnBits & Files[file]) == 0) {
				if ((board.BlackPawnBits & Files[file]) == 0) { // open file
					mobilityScore += weights.GetRookOnOpenFileBonus(sq);
				}
				else { // semi-open file
					mobilityScore += weights.GetRookOnSemiOpenFileBonus(sq);
				}
			}
			// Threats
			threatScore += weights.GetRookThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			threatScore += weights.GetRookThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			threatScore += weights.GetRookThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			threatScore += weights.GetRookThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;

		case Piece::BlackRook:
			// Get attacks, mobility & update king danger scores
			attacks = GetRookAttacks(sq, occupancy);
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights.GetRookMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Rook];
				whiteDangerPieces += 1;
			}
			// Rook on open or semi-open file
			if ((board.BlackPawnBits & Files[file]) == 0) {
				if ((board.WhitePawnBits & Files[file]) == 0) { // open file
					mobilityScore -= weights.GetRookOnOpenFileBonus(Mirror(sq));
				}
				else { // semi-open file
					mobilityScore -= weights.GetRookOnSemiOpenFileBonus(Mirror(sq));
				}
			}
			// Threats
			threatScore -= weights.GetRookThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			threatScore -= weights.GetRookThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			threatScore -= weights.GetRookThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			threatScore -= weights.GetRookThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;

		case Piece::WhiteQueen:
			// Get attacks, mobility & update king danger scores
			attacks = GetQueenAttacks(sq, occupancy);
			mobility = attacks & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights.GetQueenMobility(Popcount(mobility & ~blackPawnAttacks));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Queen];
				blackDangerPieces += 1;
			}
			// Threats
			threatScore += weights.GetQueenThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			threatScore += weights.GetQueenThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			threatScore += weights.GetQueenThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			threatScore += weights.GetQueenThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			break;

		case Piece::BlackQueen:
			// Get attacks, mobility & update king danger scores
			attacks = GetQueenAttacks(sq, occupancy);
			mobility = attacks & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights.GetQueenMobility(Popcount(mobility & ~whitePawnAttacks));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Queen];
				whiteDangerPieces += 1;
			}
			// Threats
			threatScore -= weights.GetQueenThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			threatScore -= weights.GetQueenThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			threatScore -= weights.GetQueenThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			threatScore -= weights.GetQueenThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			break;

		case Piece::WhiteKing:
			attacks = KingMoveBits[sq];
			mobility = attacks & ~whitePieces;
			// King on open or semi-open file
			if ((board.WhitePawnBits & Files[file]) == 0) {
				if ((board.BlackPawnBits & Files[file]) == 0) { // open file
					kingScore += weights.GetKingOnOpenFileEval(sq);
				}
				else { // semi-open file
					kingScore += weights.GetKingOnSemiOpenFileEval(sq);
				}
			}
			// Threats
			threatScore += weights.GetKingThreat(PieceType::Pawn) * Popcount(mobility & board.BlackPawnBits);
			threatScore += weights.GetKingThreat(PieceType::Knight) * Popcount(mobility & board.BlackKnightBits);
			threatScore += weights.GetKingThreat(PieceType::Bishop) * Popcount(mobility & board.BlackBishopBits);
			threatScore += weights.GetKingThreat(PieceType::Rook) * Popcount(mobility & board.BlackRookBits);
			threatScore += weights.GetKingThreat(PieceType::Queen) * Popcount(mobility & board.BlackQueenBits);
			break;
		case Piece::BlackKing:
			attacks = KingMoveBits[sq];
			mobility = attacks & ~blackPieces;
			// King on open or semi-open file
			if ((board.BlackPawnBits & Files[file]) == 0) {
				if ((board.WhitePawnBits & Files[file]) == 0) { // open file
					kingScore -= weights.GetKingOnOpenFileEval(Mirror(sq));
				}
				else { // semi-open file
					kingScore -= weights.GetKingOnSemiOpenFileEval(Mirror(sq));
				}
			}
			// Threats
			threatScore -= weights.GetKingThreat(PieceType::Pawn) * Popcount(mobility & board.WhitePawnBits);
			threatScore -= weights.GetKingThreat(PieceType::Knight) * Popcount(mobility & board.WhiteKnightBits);
			threatScore -= weights.GetKingThreat(PieceType::Bishop) * Popcount(mobility & board.WhiteBishopBits);
			threatScore -= weights.GetKingThreat(PieceType::Rook) * Popcount(mobility & board.WhiteRookBits);
			threatScore -= weights.GetKingThreat(PieceType::Queen) * Popcount(mobility & board.WhiteQueenBits);
			break;
		}
	}

	// King safety
	const int whiteKingSafetyFinal = whiteDangerScore * weights.DangerMultipliers[std::min(whiteDangerPieces, 7)] / 100;
	const int blackKingSafetyFinal = blackDangerScore * weights.DangerMultipliers[std::min(blackDangerPieces, 7)] / 100;
	if (whiteKingSafetyFinal != 0) kingScore += weights.GetKingDanger(std::min(whiteKingSafetyFinal, 25));
	if (blackKingSafetyFinal != 0) kingScore -= weights.GetKingDanger(std::min(blackKingSafetyFinal, 25));

	// Bishop pair bonus
	if (Popcount(board.WhiteBishopBits) >= 2) materialScore += weights.GetBishopPairEval();
	if (Popcount(board.BlackBishopBits) >= 2) materialScore -= weights.GetBishopPairEval();

	// Doubled & tripled pawn penalties
	for (int i = 0; i < 8; i++) {
		const int whitePawnsOnFile = Popcount(board.WhitePawnBits & Files[i]);
		const int blackPawnsOnFile = Popcount(board.BlackPawnBits & Files[i]);
		if (whitePawnsOnFile == 2) pawnStructureScore += weights.GetDoubledPawnEval();
		else if (whitePawnsOnFile > 2) pawnStructureScore += weights.GetTripledPawnEval();
		if (blackPawnsOnFile == 2) pawnStructureScore -= weights.GetDoubledPawnEval();
		else if (blackPawnsOnFile > 2) pawnStructureScore -= weights.GetTripledPawnEval();
	}

	// Get untapered score
	const TaperedScore taperedScore = materialScore + pqstScore + pawnStructureScore + threatScore + mobilityScore + kingScore;
	int score = LinearTaper(taperedScore.early, taperedScore.late, phase);

	// Convert to the correct perspective
	if (!board.Turn) score *= -1;

	// Tempo bonus
	const int tempo = LinearTaper(weights.GetTempoBonus(), phase);
	score += tempo;

	// Drawish endgame
	const bool drawish = IsDrawishEndgame(board, whitePieces, blackPieces);
	if (drawish) score /= 8;

	// If we're interested how the score is calculated
	/*
	if constexpr (true) {
		cout << "Hand-crafted evaluation components:\n";
		cout << "- Material:       " << LinearTaper(materialScore, phase) << "    <- " << materialScore << '\n';
		cout << "- Piece-square:   " << LinearTaper(pqstScore, phase) << "    <- " << pqstScore << '\n';
		cout << "- Pawn structure: " << LinearTaper(pawnStructureScore, phase) << "    <- " << pawnStructureScore << '\n';
		cout << "- Threats:        " << LinearTaper(threatScore, phase) << "    <- " << threatScore << '\n';
		cout << "- Mobility:       " << LinearTaper(mobilityScore, phase) << "    <- " << mobilityScore << '\n';
		cout << "- King safety:    " << LinearTaper(kingScore, phase) << "    <- " << kingScore << '\n';
		cout << "- Tempo:          " << (board.Turn == Turn::White ? tempo : -tempo) << "    <- " << weights.GetTempoBonus() << endl;
	}*/

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