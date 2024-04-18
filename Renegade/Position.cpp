#include "Position.h"

// Constructor ------------------------------------------------------------------------------------

Position::Position(const std::string& fen) {
	const std::vector<std::string> parts = Split(fen);

	// Reserve memory, add starting state
	States.reserve(512);
	Hashes.reserve(512);
	States.push_back(Board());
	Board& board = States.back();
	
	// Place pieces on the board
	int square = 56;
	for (const char& f : parts[0]) {
		switch (f) {
		case '/': square -= 16; break;
		case '1': square += 1; break;
		case '2': square += 2; break;
		case '3': square += 3; break;
		case '4': square += 4; break;
		case '5': square += 5; break;
		case '6': square += 6; break;
		case '7': square += 7; break;
		case '8': square += 8; break;
		case 'P': board.AddPiece<Piece::WhitePawn>(square); square += 1; break;
		case 'N': board.AddPiece<Piece::WhiteKnight>(square); square += 1; break;
		case 'B': board.AddPiece<Piece::WhiteBishop>(square); square += 1; break;
		case 'R': board.AddPiece<Piece::WhiteRook>(square); square += 1; break;
		case 'Q': board.AddPiece<Piece::WhiteQueen>(square); square += 1; break;
		case 'K': board.AddPiece<Piece::WhiteKing>(square); square += 1; break;
		case 'p': board.AddPiece<Piece::BlackPawn>(square); square += 1; break;
		case 'n': board.AddPiece<Piece::BlackKnight>(square); square += 1; break;
		case 'b': board.AddPiece<Piece::BlackBishop>(square); square += 1; break;
		case 'r': board.AddPiece<Piece::BlackRook>(square); square += 1; break;
		case 'q': board.AddPiece<Piece::BlackQueen>(square); square += 1; break;
		case 'k': board.AddPiece<Piece::BlackKing>(square); square += 1; break;
		}
	}

	// Other information
	board.Turn = (parts[1] == "w") ? Side::White : Side::Black;

	for (const char& f : parts[2]) {
		switch (f) {
		case 'K': board.WhiteRightToShortCastle = true; break;
		case 'Q': board.WhiteRightToLongCastle = true; break;
		case 'k': board.BlackRightToShortCastle = true; break;
		case 'q': board.BlackRightToLongCastle = true; break;
		}
	}
	CastlingConfig = { 0, 7, 56, 63 };

	if (parts[3] != "-") {
		board.EnPassantSquare = SquareToNum(parts[3]);
	}

	board.HalfmoveClock = stoi(parts[4]);
	board.FullmoveClock = stoi(parts[5]);

	Hashes.push_back(board.CalculateHash());
}

// Pushing moves ----------------------------------------------------------------------------------

void Position::Push(const Move& move) {
	assert(move.IsNotNull());

	States.push_back(Board(CurrentState()));
	Board& board = CurrentState();

	board.HalfmoveClock += 1;
	board.ApplyMove(move);

	// Increment timers
	board.Turn = !board.Turn;
	if (board.Turn == Side::White) board.FullmoveClock += 1;

	Hashes.push_back(board.CalculateHash());
	assert(States.size() == Hashes.size());
}

void Position::PushNullMove() {
	States.push_back(Board(CurrentState()));
	Board& board = CurrentState();
	board.Turn = !board.Turn;
	if (board.EnPassantSquare == -1) {
		Hashes.push_back(Hashes.back() ^ Zobrist[780]);
	}
	else {
		board.EnPassantSquare = -1;
		Hashes.push_back(board.CalculateHash());
	}
	return;
}

bool Position::PushUCI(const std::string& str) {
	const uint8_t sq1 = SquareToNum(str.substr(0, 2));
	const uint8_t sq2 = SquareToNum(str.substr(2, 2));
	const char extra = str[4];
	Move move = Move(sq1, sq2);
	const uint8_t piece = CurrentState().GetPieceAt(sq1);
	const uint8_t capturedPiece = CurrentState().GetPieceAt(sq2);

	// Promotions
	switch (extra) {
	case 'q': move.flag = MoveFlag::PromotionToQueen; break;
	case 'r': move.flag = MoveFlag::PromotionToRook; break;
	case 'b': move.flag = MoveFlag::PromotionToBishop; break;
	case 'n': move.flag = MoveFlag::PromotionToKnight; break;
	}

	// Castling
	if ((piece == Piece::WhiteKing) && (sq1 == 4) && (sq2 == 2)) move = { 4, 0, MoveFlag::LongCastle };
	else if ((piece == Piece::WhiteKing) && (sq1 == 4) && (sq2 == 6)) move = { 4, 7, MoveFlag::ShortCastle };
	else if ((piece == Piece::BlackKing) && (sq1 == 60) && (sq2 == 58)) move = { 60, 56, MoveFlag::LongCastle };
	else if ((piece == Piece::BlackKing) && (sq1 == 60) && (sq2 == 62)) move = { 60, 63, MoveFlag::ShortCastle };

	// En passant possibility
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		const uint8_t f1 = sq1 / 8;
		const uint8_t f2 = sq2 / 8;
		if (std::abs(f2 - f1) > 1) move.flag = MoveFlag::EnPassantPossible;
	}

	// En passant performed
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		if ((TypeOfPiece(capturedPiece) == 0) && (GetSquareFile(sq1) != GetSquareFile(sq2))) {
			move.flag = MoveFlag::EnPassantPerformed;
		}
	}

	// Generate the list of valid moves
	MoveList legalMoves{};
	GenerateMoves(legalMoves, MoveGen::All, Legality::Legal);
	bool valid = false;
	for (const auto& m : legalMoves) {
		if ((m.move.to == move.to) && (m.move.from == move.from) && (m.move.flag == move.flag)) {
			valid = true;
			break;
		}
	}

	// Make the move if valid
	if (valid) {
		Push(move);
		return true;
	}
	else {
		return false;
	}
}

void Position::Pop() {
	States.pop_back();
	Hashes.pop_back();
}

// Generating moves -------------------------------------------------------------------------------

template <bool side, MoveGen moveGen>
void Position::GenerateKingMoves(MoveList& moves, const int home) const {
	constexpr uint8_t friendlyPieceColor = (side == Side::White) ? PieceColor::White : PieceColor::Black;
	constexpr uint8_t opponentPieceColor = (side == Side::White) ? PieceColor::Black : PieceColor::White;
	uint64_t bits = KingMoveBits[home];
	while (bits) {
		const uint8_t l = Popsquare(bits);
		if (ColorOfPiece(GetPieceAt(l)) == friendlyPieceColor) continue;
		if ((moveGen == MoveGen::All) || (ColorOfPiece(GetPieceAt(l)) == opponentPieceColor)) moves.pushUnscored(Move(home, l));
	}
}

template <bool side, MoveGen moveGen>
void Position::GenerateKnightMoves(MoveList& moves, const int home) const {
	constexpr uint8_t friendlyPieceColor = (side == Side::White) ? PieceColor::White : PieceColor::Black;
	constexpr uint8_t opponentPieceColor = (side == Side::White) ? PieceColor::Black : PieceColor::White;
	uint64_t bits = KnightMoveBits[home];
	while (bits) {
		const uint8_t l = Popsquare(bits);
		if (ColorOfPiece(GetPieceAt(l)) == friendlyPieceColor) continue;
		if ((moveGen == MoveGen::All) || (ColorOfPiece(GetPieceAt(l)) == opponentPieceColor)) moves.pushUnscored(Move(home, l));
	}
}

template <bool side, int pieceType, MoveGen moveGen>
void Position::GenerateSlidingMoves(MoveList& moves, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy) const {
	const uint64_t friendlyOccupance = (side == PieceColor::White) ? whiteOccupancy : blackOccupancy;
	const uint64_t opponentOccupance = (side == PieceColor::White) ? blackOccupancy : whiteOccupancy;
	const uint64_t occupancy = whiteOccupancy | blackOccupancy;
	uint64_t map = 0;

	if constexpr (pieceType == PieceType::Rook) map = GetRookAttacks(home, occupancy) & ~friendlyOccupance;
	if constexpr (pieceType == PieceType::Bishop) map = GetBishopAttacks(home, occupancy) & ~friendlyOccupance;
	if constexpr (pieceType == PieceType::Queen) map = GetQueenAttacks(home, occupancy) & ~friendlyOccupance;

	if constexpr (moveGen == MoveGen::Noisy) map &= opponentOccupance;
	if (map == 0) return;

	while (map != 0) {
		const int sq = Popsquare(map);
		moves.pushUnscored(Move(home, sq));
	}
}

template <bool side, MoveGen moveGen>
void Position::GeneratePawnMoves(MoveList& moves, const int home) const {

	const Board& b = CurrentState();

	constexpr int promotionRank = (side == Side::White) ? 7 : 0;
	constexpr int doublePushRank = (side == Side::White) ? 1 : 6;
	constexpr int forwardDelta = (side == Side::White) ? 8 : -8;
	constexpr int doublePushDelta = (side == Side::White) ? 16 : -16;
	constexpr uint8_t opponentPieceColor = SideToPieceColor(!side);

	// These are like that to avoid bench changing, will be changed to be more intuitive later
	constexpr int capture1Delta = (side == Side::White) ? 7 : -7;
	constexpr int capture2Delta = (side == Side::White) ? 9 : -9;
	constexpr int capture1WrongFile = (side == Side::White) ? 0 : 7;
	constexpr int capture2WrongFile = (side == Side::White) ? 7 : 0;

	const int file = GetSquareFile(home);
	const int rank = GetSquareRank(home);
	int target;

	// 1. Handle moving forward + check promotions
	target = home + forwardDelta;
	if (GetPieceAt(target) == Piece::None) {
		if (GetSquareRank(target) != promotionRank) {
			if constexpr (moveGen == MoveGen::All) moves.pushUnscored(Move(home, target));
		}
		else { // Promote
			moves.pushUnscored(Move(home, target, MoveFlag::PromotionToQueen));
			if constexpr (moveGen == MoveGen::All) {
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToRook));
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToBishop));
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToKnight));
			}
		}
	}

	// 2. Handle captures + check promotions + check en passant
	constexpr std::array<std::pair<int, int>, 2> captureDetails =
	{ std::pair{capture1Delta, capture1WrongFile}, std::pair{capture2Delta, capture2WrongFile} };

	for (const auto& [delta, wrongFile] : captureDetails) {
		target = home + delta;

		if ((file != wrongFile) && ((ColorOfPiece(GetPieceAt(target)) == opponentPieceColor) || (target == b.EnPassantSquare))) {
			if (GetSquareRank(target) != promotionRank) {
				const uint8_t moveFlag = (target == b.EnPassantSquare) ? MoveFlag::EnPassantPerformed : MoveFlag::None;
				moves.pushUnscored(Move(home, target, moveFlag));
			}
			else { // Promote
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToQueen));
				if constexpr (moveGen == MoveGen::All) {
					moves.pushUnscored(Move(home, target, MoveFlag::PromotionToRook));
					moves.pushUnscored(Move(home, target, MoveFlag::PromotionToBishop));
					moves.pushUnscored(Move(home, target, MoveFlag::PromotionToKnight));
				}
			}
		}
	}

	// 3. Handle double pushes
	if (rank == doublePushRank) {
		const bool free1 = GetPieceAt(home + forwardDelta) == Piece::None;
		const bool free2 = GetPieceAt(home + doublePushDelta) == Piece::None;
		if (free1 && free2) {
			if constexpr (moveGen == MoveGen::All) moves.pushUnscored(Move(home, home + doublePushDelta, MoveFlag::EnPassantPossible));
		}
	}

}

template <bool side>
void Position::GenerateCastlingMoves(MoveList& moves) const {

	using namespace Squares;
	const Board& b = CurrentState();

	constexpr auto shortEmpty = (side == Side::White) ? std::array{ F1, G1 } : std::array{ F8, G8 };
	constexpr auto shortSafe = (side == Side::White) ? std::array{ E1, F1, G1 } : std::array{ E8, F8, G8 };
	constexpr auto longEmpty = (side == Side::White) ? std::array{ B1, C1, D1 } : std::array{ B8, C8, D8 };
	constexpr auto longSafe = (side == Side::White) ? std::array{ C1, D1, E1 } : std::array{ C8, D8, E8 };
	constexpr int from = (side == Side::White) ? E1 : E8;
	constexpr int shortRook = (side == Side::White) ? H1 : H8;
	constexpr int longRook = (side == Side::White) ? A1 : A8;

	const bool rightToShortCastle = (side == Side::White) ? b.WhiteRightToShortCastle : b.BlackRightToShortCastle;
	const bool rightToLongCastle = (side == Side::White) ? b.WhiteRightToLongCastle : b.BlackRightToLongCastle;

	if (rightToShortCastle) {
		const bool empty = std::all_of(shortEmpty.begin(), shortEmpty.end(), [&](const int sq) {
			return GetPieceAt(sq) == Piece::None;
			});
		if (empty) {
			const bool safe = std::all_of(shortSafe.begin(), shortSafe.end(), [&](const int sq) {
				return !IsSquareAttacked<!side>(sq);
				});
			if (safe) moves.pushUnscored(Move(from, shortRook, MoveFlag::ShortCastle));
		}
	}
	if (rightToLongCastle) {
		const bool empty = std::all_of(longEmpty.begin(), longEmpty.end(), [&](const int sq) {
			return GetPieceAt(sq) == Piece::None;
			});
		if (empty) {
			const bool safe = std::all_of(longSafe.begin(), longSafe.end(), [&](const int sq) {
				return !IsSquareAttacked<!side>(sq);
				});
			if (safe) moves.pushUnscored(Move(from, longRook, MoveFlag::LongCastle));
		}
	}
}

void Position::GenerateMoves(MoveList& moves, const MoveGen moveGen, const Legality legality) const {

	const Board& board = CurrentState();

	if (legality == Legality::Pseudolegal) {
		if (moveGen == MoveGen::All) {
			if (board.Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::All>(moves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::All>(moves);
		}
		else if (moveGen == MoveGen::Noisy) {
			if (board.Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::Noisy>(moves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::Noisy>(moves);
		}
	}
	else {
		MoveList legalMoves{};
		if (moveGen == MoveGen::All) {
			if (board.Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::All>(legalMoves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::All>(legalMoves);
		}
		else if (moveGen == MoveGen::Noisy) {
			if (board.Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::Noisy>(legalMoves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::Noisy>(legalMoves);
		}
		for (const auto& m : legalMoves) {
			if (IsLegalMove(m.move)) moves.pushUnscored(m.move);
		}
	}
}

template <bool side, MoveGen moveGen>
void Position::GeneratePseudolegalMoves(MoveList& moves) const {
	const uint64_t whiteOccupancy = GetOccupancy(Side::White);
	const uint64_t blackOccupancy = GetOccupancy(Side::Black);
	uint64_t friendlyOccupancy = (Turn() == Side::White) ? whiteOccupancy : blackOccupancy;

	while (friendlyOccupancy != 0) {
		const int sq = Popsquare(friendlyOccupancy);
		const int type = TypeOfPiece(GetPieceAt(sq));

		switch (type) {
		case PieceType::Pawn:
			GeneratePawnMoves<side, moveGen>(moves, sq);
			break;
		case PieceType::Knight:
			GenerateKnightMoves<side, moveGen>(moves, sq);
			break;
		case PieceType::Bishop:
			GenerateSlidingMoves<side, PieceType::Bishop, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
			break;
		case PieceType::Rook:
			GenerateSlidingMoves<side, PieceType::Rook, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
			break;
		case PieceType::Queen:
			GenerateSlidingMoves<side, PieceType::Queen, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
			break;
		case PieceType::King:
			GenerateKingMoves<side, moveGen>(moves, sq);
			if constexpr (moveGen == MoveGen::All) GenerateCastlingMoves<side>(moves);
			break;

		}
	}
}

// Threats and move legality ----------------------------------------------------------------------

template <bool attackingSide>
uint64_t Position::CalculateAttackedSquaresTemplated() const {

	uint64_t map = 0;
	const Board& b = CurrentState();

	// Pawn attacks
	map = (attackingSide == Side::White) ? GetPawnAttacks<Side::White>() : GetPawnAttacks<Side::Black>();

	// Knight attacks
	uint64_t knightBits = (attackingSide == Side::White) ? b.WhiteKnightBits : b.BlackKnightBits;
	while (knightBits) {
		const uint8_t sq = Popsquare(knightBits);
		map |= GenerateKnightAttacks(sq);
	}

	// King attacks
	uint8_t kingSquare = 63 - Lzcount((attackingSide == Side::White) ? b.WhiteKingBits : b.BlackKingBits);
	map |= KingMoveBits[kingSquare];

	// Sliding pieces
	// 'parallel' comes from being parallel to the axes, better name suggestions welcome
	uint64_t occ = GetOccupancy();
	uint64_t parallelSliders = (attackingSide == Side::White) ? (b.WhiteRookBits | b.WhiteQueenBits) : (b.BlackRookBits | b.BlackQueenBits);
	uint64_t diagonalSliders = (attackingSide == Side::White) ? (b.WhiteBishopBits | b.WhiteQueenBits) : (b.BlackBishopBits | b.BlackQueenBits);

	while (parallelSliders) {
		const uint8_t sq = Popsquare(parallelSliders);
		map |= GetRookAttacks(sq, occ);
	}
	while (diagonalSliders) {
		const uint8_t sq = Popsquare(diagonalSliders);
		map |= GetBishopAttacks(sq, occ);
	}

	return map;
}

uint64_t Position::CalculateAttackedSquares(const bool attackingSide) const {
	return (attackingSide == Side::White) ? CalculateAttackedSquaresTemplated<Side::White>() : CalculateAttackedSquaresTemplated<Side::Black>();
}

uint64_t Position::GetAttackersOfSquare(const uint8_t square, const uint64_t occupied) const {
	// if not being used for SEE: occupied = GetOccupancy();

	// Generate attackers setwise
	// Taking advantage of the fact that for non-pawns, if X attacks Y, then Y attacks X
	const Board& b = States.back();
	const uint64_t pawnAttackers = (WhitePawnAttacks[square] & b.BlackPawnBits) | (BlackPawnAttacks[square] & b.WhitePawnBits);
	const uint64_t knightAttackers = KnightMoveBits[square] & (b.WhiteKnightBits | b.BlackKnightBits);
	const uint64_t bishopAttackers = GetBishopAttacks(square, occupied) & (b.WhiteBishopBits | b.BlackBishopBits | b.WhiteQueenBits | b.BlackQueenBits);
	const uint64_t rookAttackers = GetRookAttacks(square, occupied) & (b.WhiteRookBits | b.BlackRookBits | b.WhiteQueenBits | b.BlackQueenBits);
	const uint64_t kingAttackers = KingMoveBits[square] & (b.WhiteKingBits | b.BlackKingBits);
	return pawnAttackers | knightAttackers | bishopAttackers | rookAttackers | kingAttackers;
}

// This function assumes that the move is at least pseudolegal
// This is terrible currently, but it was pain to get right, and I don't want to touch it with a 10 ft pole
bool Position::IsLegalMove(const Move& m) const {
	
	//if (m.IsNull()) cout << "#" << endl;

	if (m.IsCastling()) return true; // Castling is guaranteed to be legal from movegen

	const Board& board = CurrentState();
	const uint8_t movedPiece = GetPieceAt(m.from);

	if (TypeOfPiece(movedPiece) == PieceType::King) {
		// Destination square must not be attacked by the opponent
		const uint8_t kingSq = (board.Turn == Side::White) ? LsbSquare(board.WhiteKingBits) : LsbSquare(board.BlackKingBits);
		const uint64_t occupancy = GetOccupancy() ^ SquareBit(kingSq);
		return (board.Turn == Side::White) ? !IsSquareAttacked2(Side::Black, m.to, occupancy) : !IsSquareAttacked2(Side::White, m.to, occupancy);
	}

	const uint8_t capturedPiece = GetPieceAt(m.to);
	const uint8_t kingSq = (board.Turn == Side::White) ? LsbSquare(board.WhiteKingBits) : LsbSquare(board.BlackKingBits);
	const uint64_t occupancy = GetOccupancy();

	if (m.flag == MoveFlag::EnPassantPerformed) {
		// After the en passant start rays to see if the king is attacked by an appropiate sliding piece
		// TODO: Checking for only rook attacks is enough, I think?
		const uint8_t epVictimSq = (board.Turn == Side::White) ? board.EnPassantSquare - 8 : board.EnPassantSquare + 8;
		const uint64_t parallelSliders = (board.Turn == Side::White) ? (board.BlackRookBits | board.BlackQueenBits) : (board.WhiteRookBits | board.WhiteQueenBits);
		const uint64_t diagonalSliders = (board.Turn == Side::White) ? (board.BlackBishopBits | board.BlackQueenBits) : (board.WhiteBishopBits | board.WhiteQueenBits);
		const uint64_t approxOccupancy = occupancy ^ SquareBit(m.from) ^ SquareBit(epVictimSq) | SquareBit(m.to);
		return !(GetRookAttacks(kingSq, approxOccupancy) & parallelSliders) && !(GetBishopAttacks(kingSq, approxOccupancy) & diagonalSliders);
	}

	// Regular non-king moves

	const uint64_t checking = AttackersOfSquare(!Turn(), kingSq);
	if (Popcount(checking) > 1) return false; // double checks can only be evaded by a king move

	if (!checking) {
		const uint64_t parallelSliders = ((board.Turn == Side::White) ? (board.BlackRookBits | board.BlackQueenBits) : (board.WhiteRookBits | board.WhiteQueenBits)) & ~SquareBit(m.to);
		const uint64_t diagonalSliders = ((board.Turn == Side::White) ? (board.BlackBishopBits | board.BlackQueenBits) : (board.WhiteBishopBits | board.WhiteQueenBits)) & ~SquareBit(m.to);
		const uint64_t approxOccupancy = (occupancy ^ SquareBit(kingSq) ^ SquareBit(m.from)) | SquareBit(m.to);
		return !(GetRookAttacks(kingSq, approxOccupancy) & parallelSliders) && !(GetBishopAttacks(kingSq, approxOccupancy) & diagonalSliders);
	}
	else {
		if (m.to == LsbSquare(checking)) {
			const uint64_t parallelSliders = ((board.Turn == Side::White) ? (board.BlackRookBits | board.BlackQueenBits) : (board.WhiteRookBits | board.WhiteQueenBits)) & ~SquareBit(m.to);
			const uint64_t diagonalSliders = ((board.Turn == Side::White) ? (board.BlackBishopBits | board.BlackQueenBits) : (board.WhiteBishopBits | board.WhiteQueenBits)) & ~SquareBit(m.to);
			const uint64_t approxOccupancy = (occupancy ^ SquareBit(kingSq) ^ SquareBit(m.from)) | SquareBit(m.to);
			const uint64_t pin1 = GetRookAttacks(kingSq, approxOccupancy) & parallelSliders;
			const uint64_t pin2 = GetBishopAttacks(kingSq, approxOccupancy) & diagonalSliders;
			const uint64_t pins = pin1 | pin2;
			return !pins;
		}
		else {
			if (checking & (board.WhiteKnightBits | board.BlackKnightBits | board.WhiteKingBits | board.BlackKingBits | board.WhitePawnBits | board.BlackPawnBits)) return false;
			const uint64_t parallelSliders = ((board.Turn == Side::White) ? (board.BlackRookBits | board.BlackQueenBits) : (board.WhiteRookBits | board.WhiteQueenBits)) & ~SquareBit(m.to);
			const uint64_t diagonalSliders = ((board.Turn == Side::White) ? (board.BlackBishopBits | board.BlackQueenBits) : (board.WhiteBishopBits | board.WhiteQueenBits)) & ~SquareBit(m.to);
			const uint64_t approxOccupancy = (occupancy ^ SquareBit(kingSq) ^ SquareBit(m.from)) | SquareBit(m.to);
			const uint64_t pin1 = GetRookAttacks(kingSq, approxOccupancy) & parallelSliders;
			const uint64_t pin2 = GetBishopAttacks(kingSq, approxOccupancy) & diagonalSliders;
			const uint64_t pins = pin1 | pin2;
			return !pins;
		}
	}

}



template <bool attackingSide>
bool Position::IsSquareAttacked(const uint8_t square) const {
	const uint64_t occupancy = GetOccupancy();
	const Board& b = CurrentState();

	if constexpr (attackingSide == Side::White) {
		// Attacked by a knight?
		if (KnightMoveBits[square] & b.WhiteKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & b.WhiteKingBits) return true;
		// Attacked by a pawn?
		if (SquareBit(square) & ((b.WhitePawnBits & ~File[0]) << 7)) return true;
		if (SquareBit(square) & ((b.WhitePawnBits & ~File[7]) << 9)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (b.WhiteRookBits | b.WhiteQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (b.WhiteBishopBits | b.WhiteQueenBits)) return true;
		// Okay
		return false;
	}
	else {
		// Attacked by a knight?
		if (KnightMoveBits[square] & b.BlackKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & b.BlackKingBits) return true;
		// Attacked by a pawn?
		if (SquareBit(square) & ((b.BlackPawnBits & ~File[0]) >> 9)) return true;
		if (SquareBit(square) & ((b.BlackPawnBits & ~File[7]) >> 7)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (b.BlackRookBits | b.BlackQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (b.BlackBishopBits | b.BlackQueenBits)) return true;
		// Okay
		return false;
	}
}

// will be simplified somehow
bool Position::IsSquareAttacked2(const bool attackingSide, const uint8_t square, const uint64_t occupancy) const {
	const Board& b = CurrentState();

	if (attackingSide == Side::White) {
		// Attacked by a knight?
		if (KnightMoveBits[square] & b.WhiteKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & b.WhiteKingBits) return true;
		// Attacked by a pawn?
		if (SquareBit(square) & ((b.WhitePawnBits & ~File[0]) << 7)) return true;
		if (SquareBit(square) & ((b.WhitePawnBits & ~File[7]) << 9)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (b.WhiteRookBits | b.WhiteQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (b.WhiteBishopBits | b.WhiteQueenBits)) return true;
		// Okay
		return false;
	}
	else {
		// Attacked by a knight?
		if (KnightMoveBits[square] & b.BlackKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & b.BlackKingBits) return true;
		// Attacked by a pawn?
		if (SquareBit(square) & ((b.BlackPawnBits & ~File[0]) >> 9)) return true;
		if (SquareBit(square) & ((b.BlackPawnBits & ~File[7]) >> 7)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (b.BlackRookBits | b.BlackQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (b.BlackBishopBits | b.BlackQueenBits)) return true;
		// Okay
		return false;
	}
}

uint64_t Position::AttackersOfSquare(const bool attackingSide, const uint8_t square) const {
	const uint64_t occupancy = GetOccupancy();
	const Board& b = CurrentState();
	uint64_t attackers = 0;

	if (attackingSide == Side::White) {
		attackers |= (KnightMoveBits[square] & b.WhiteKnightBits);
		attackers |= (KingMoveBits[square] & b.WhiteKingBits);
		attackers |= ((SquareBit(square) & ~File[0]) >> 9) & b.WhitePawnBits;
		attackers |= ((SquareBit(square) & ~File[7]) >> 7) & b.WhitePawnBits;
		attackers |= (GetRookAttacks(square, occupancy) & (b.WhiteRookBits | b.WhiteQueenBits));
		attackers |= (GetBishopAttacks(square, occupancy) & (b.WhiteBishopBits | b.WhiteQueenBits));
	}
	else {
		attackers |= (KnightMoveBits[square] & b.BlackKnightBits);
		attackers |= (KingMoveBits[square] & b.BlackKingBits);
		attackers |= ((SquareBit(square) & ~File[7]) << 9) & b.BlackPawnBits;
		attackers |= ((SquareBit(square) & ~File[0]) << 7) & b.BlackPawnBits;
		attackers |= (GetRookAttacks(square, occupancy) & (b.BlackRookBits | b.BlackQueenBits));
		attackers |= (GetBishopAttacks(square, occupancy) & (b.BlackBishopBits | b.BlackQueenBits));
	}
	return attackers;
}

// Getting information ----------------------------------------------------------------------------

bool Position::IsDrawn(const bool threefold) const {
	const Board& b = CurrentState();

	// 1. Fifty moves without progress
	if (b.HalfmoveClock >= 100) return true;

	// 2. Threefold repetitions
	const uint64_t hash = Hash();
	const int length = Hashes.size();
	const int threshold = threefold ? 3 : 2;
	int repeated = 0;

	for (int i = length - 1; i >= std::max(0, length - b.HalfmoveClock - 2); i -= 2) {
		if (Hashes[i] == hash) {
			repeated += 1;
			if (repeated >= threshold) return true;
		}
	}

	// 3. Insufficient material check
	// - has pawns or major pieces -> sufficient
	if (b.WhitePawnBits | b.BlackPawnBits) return false;
	if (b.WhiteRookBits | b.BlackRookBits | b.WhiteQueenBits | b.BlackQueenBits) return false;
	// - less than 4 with minor pieces is a draw, more than 4 is not
	const int pieceCount = Popcount(GetOccupancy());
	if (pieceCount > 4) return false;
	if (pieceCount < 4) return true;
	// - for exactly 4 pieces, check for same-color KBvKB
	if (Popcount(b.WhiteBishopBits & LightSquares) == 1 && Popcount(b.BlackBishopBits & LightSquares) == 1) return true;
	if (Popcount(b.WhiteBishopBits & DarkSquares) == 1 && Popcount(b.BlackBishopBits & DarkSquares) == 1) return true;
	return false;
}

bool Position::IsMoveQuiet(const Move& move) const {
	if (move.IsCastling()) return true;
	const uint8_t movedPiece = GetPieceAt(move.from);
	const uint8_t targetPiece = GetPieceAt(move.to);
	if (targetPiece != Piece::None) return false;
	if (move.flag == MoveFlag::PromotionToQueen) return false;
	if (move.flag == MoveFlag::EnPassantPerformed) return false;
	return true;
}

std::string Position::GetFEN() const {
	const Board& b = CurrentState();
	std::string result{};
	for (int r = 7; r >= 0; r--) {
		int spaces = 0;
		for (int f = 0; f < 8; f++) {
			int piece = GetPieceAt(Square(r, f));
			if (piece == 0) {
				spaces += 1;
			}
			else {
				if (spaces != 0) result += std::to_string(spaces);
				result += PieceChars[piece];
				spaces = 0;
			}
		}
		if (spaces != 0) result += std::to_string(spaces);
		if (r != 0) result += '/';
	}

	result += (Turn() == Side::White) ? " w " : " b ";

	bool castlingPossible = false;
	if (b.WhiteRightToShortCastle) { result += 'K'; castlingPossible = true; }
	if (b.WhiteRightToLongCastle) { result += 'Q'; castlingPossible = true; }
	if (b.BlackRightToShortCastle) { result += 'k'; castlingPossible = true; }
	if (b.BlackRightToLongCastle) { result += 'q'; castlingPossible = true; }
	if (!castlingPossible) result += '-';
	result += ' ';

	bool enPassantPossible = false;
	if ((b.EnPassantSquare != -1) && (b.Turn == Side::White)) {
		const bool fromRight = (((b.WhitePawnBits & ~File[0]) << 7) & SquareBit(b.EnPassantSquare));
		const bool fromLeft = (((b.WhitePawnBits & ~File[7]) << 9) & SquareBit(b.EnPassantSquare));
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if ((b.EnPassantSquare != -1) && (b.Turn == Side::Black)) {
		const bool fromRight = (((b.BlackPawnBits & ~File[0]) >> 9) & SquareBit(b.EnPassantSquare));
		const bool fromLeft = (((b.BlackPawnBits & ~File[7]) >> 7) & SquareBit(b.EnPassantSquare));
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if (enPassantPossible) result += SquareStrings[b.EnPassantSquare];
	else result += '-';

	result += ' ' + std::to_string(b.HalfmoveClock) + ' ' + std::to_string(b.FullmoveClock);
	return result;
}

GameState Position::GetGameState() const {
	// Check checkmates & stalemates
	MoveList moves{};
	GenerateMoves(moves, MoveGen::All, Legality::Legal);
	if (moves.size() == 0) {
		if (IsInCheck()) {
			if (Turn() == Side::Black) return GameState::WhiteVictory;
			else return GameState::BlackVictory;
		}
		else {
			return GameState::Draw; // Stalemate
		}
	}

	// Check other types of draws
	if (IsDrawn(true)) return GameState::Draw;
	else return GameState::Playing;
}