#include "Position.h"

// Constructor ------------------------------------------------------------------------------------

Position::Position(const std::string& fen) {
	const std::vector<std::string> parts = Split(fen);

	// Reserve memory, add starting state
	States.reserve(512);
	Moves.reserve(512);
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

	board.Turn = (parts[1] == "w") ? Side::White : Side::Black;
	if (board.Turn == Side::White) board.BoardHash ^= Zobrist.SideToMove;

	// Castling rights

	CastlingConfig = { 0, 7, 56, 63 }; // Default

	for (const char& f : parts[2]) {
		if (Settings::Chess960) {

			// Chess960
			// KQkq is ambiguous and it's currently not supported

			if (f >= 'A' && f <= 'H') {
				const uint8_t rookFile = f - 'A';
				const uint8_t kingFile = GetSquareFile(LsbSquare(board.WhiteKingBits));
				if (rookFile > kingFile) {
					board.SetWhiteShortCastlingRight<true>();
					CastlingConfig.WhiteShortCastleRookSquare = rookFile;
				}
				else {
					board.SetWhiteLongCastlingRight<true>();
					CastlingConfig.WhiteLongCastleRookSquare = rookFile;
				}
			}
			else if (f >= 'a' && f <= 'h') {
				const uint8_t rookFile = f - 'a';
				const uint8_t kingFile = GetSquareFile(LsbSquare(board.BlackKingBits));
				if (rookFile > kingFile) {
					board.SetBlackShortCastlingRight<true>();
					CastlingConfig.BlackShortCastleRookSquare = rookFile + 56;
				}
				else {
					board.SetBlackLongCastlingRight<true>();
					CastlingConfig.BlackLongCastleRookSquare = rookFile + 56;
				}
			}
		}
		else {
			// Normal chess
			switch (f) {
			case 'K': board.SetWhiteShortCastlingRight<true>(); break;
			case 'Q': board.SetWhiteLongCastlingRight<true>(); break;
			case 'k': board.SetBlackShortCastlingRight<true>(); break;
			case 'q': board.SetBlackLongCastlingRight<true>(); break;
			}
		}
	}

	if (parts[3] != "-") {
		// There are two possible conventions, here we make sure to use the correct one
		// So that "position startpos moves ..." and "position fen ..." yield the same hash for the same position
		const uint8_t epSquare = SquareToNum(parts[3]);
		if (board.Turn == Side::White) {
			const bool pawnOnLeft = GetSquareFile(epSquare) != 0 && GetPieceAt(epSquare - 9) == Piece::WhitePawn;
			const bool pawnOnRight = GetSquareFile(epSquare) != 7 && GetPieceAt(epSquare - 7) == Piece::WhitePawn;
			if (pawnOnLeft || pawnOnRight) {
				board.EnPassantSquare = epSquare;
				board.BoardHash ^= Zobrist.EnPassant[GetSquareFile(epSquare)];
			}
		}
		else {
			const bool pawnOnLeft = GetSquareFile(epSquare) != 0 && GetPieceAt(epSquare + 7) == Piece::BlackPawn;
			const bool pawnOnRight = GetSquareFile(epSquare) != 7 && GetPieceAt(epSquare + 9) == Piece::BlackPawn;
			if (pawnOnLeft || pawnOnRight) {
				board.EnPassantSquare = epSquare;
				board.BoardHash ^= Zobrist.EnPassant[GetSquareFile(epSquare)];
			}
		}
	}

	board.HalfmoveClock = std::stoi(parts[4]);
	board.FullmoveClock = std::stoi(parts[5]);
	board.Threats = CalculateAttackedSquares(!Turn());
}

Position::Position(const int frcWhite, const int frcBlack) {

	assert(Settings::Chess960);
	assert(0 <= frcWhite && frcWhite < 960);
	assert(0 <= frcBlack && frcBlack < 960);

	// Reserve memory, add starting state
	States.reserve(512);
	Moves.reserve(512);
	States.push_back(Board());
	Board& board = States.back();

	// Find n-th free square
	auto fnfsq = [](const std::array<uint8_t, 8>& pieces, const int nth) {
		int i = 0;
		for (int f = 0; f < 8; f++) {
			if (pieces[f] != PieceType::None) continue;
			if (i == nth) return f;
			i += 1;
		}
		assert(false);
		return 0;
	};

	// Generate an array of piece types
	// https://en.wikipedia.org/wiki/Fischer_random_chess_numbering_scheme
	auto layout = [&](const int frcIndex) {
		std::array<uint8_t, 8> pieces{};

		// Place bishops and queen
		const auto [n2, b1] = std::div(frcIndex, 4);
		pieces[b1 * 2 + 1] = PieceType::Bishop;
		const auto [n3, b2] = std::div(n2, 4);
		pieces[b2 * 2] = PieceType::Bishop;
		const auto [n4, q] = std::div(n3, 6);
		pieces[fnfsq(pieces, q)] = PieceType::Queen;

		// Place knights (oh boy)
		constexpr uint8_t knight = PieceType::Knight;
		switch (n4) {
		case 0: pieces[fnfsq(pieces, 0)] = knight; pieces[fnfsq(pieces, 0)] = knight; break;
		case 1: pieces[fnfsq(pieces, 0)] = knight; pieces[fnfsq(pieces, 1)] = knight; break;
		case 2: pieces[fnfsq(pieces, 0)] = knight; pieces[fnfsq(pieces, 2)] = knight; break;
		case 3: pieces[fnfsq(pieces, 0)] = knight; pieces[fnfsq(pieces, 3)] = knight; break;
		case 4: pieces[fnfsq(pieces, 1)] = knight; pieces[fnfsq(pieces, 1)] = knight; break;
		case 5: pieces[fnfsq(pieces, 1)] = knight; pieces[fnfsq(pieces, 2)] = knight; break;
		case 6: pieces[fnfsq(pieces, 1)] = knight; pieces[fnfsq(pieces, 3)] = knight; break;
		case 7: pieces[fnfsq(pieces, 2)] = knight; pieces[fnfsq(pieces, 2)] = knight; break;
		case 8: pieces[fnfsq(pieces, 2)] = knight; pieces[fnfsq(pieces, 3)] = knight; break;
		case 9: pieces[fnfsq(pieces, 3)] = knight; pieces[fnfsq(pieces, 3)] = knight; break;
		}

		// Place rooks and king
		pieces[fnfsq(pieces, 0)] = PieceType::Rook;
		pieces[fnfsq(pieces, 0)] = PieceType::King;
		pieces[fnfsq(pieces, 0)] = PieceType::Rook;

		return pieces;
	};

	// Add pieces from the generated layout to the board
	const std::array<uint8_t, 8> whiteLayout = layout(frcWhite);
	const std::array<uint8_t, 8> blackLayout = layout(frcBlack);
	for (int i = 0; i < 8; i++) {
		switch (whiteLayout[i]) {
		case PieceType::Knight: board.AddPiece<Piece::WhiteKnight>(i); break;
		case PieceType::Bishop: board.AddPiece<Piece::WhiteBishop>(i); break;
		case PieceType::Rook: board.AddPiece<Piece::WhiteRook>(i); break;
		case PieceType::Queen: board.AddPiece<Piece::WhiteQueen>(i); break;
		case PieceType::King: board.AddPiece<Piece::WhiteKing>(i); break;
		}
	}
	for (int i = 0; i < 8; i++) {
		switch (blackLayout[i]) {
		case PieceType::Knight: board.AddPiece<Piece::BlackKnight>(56 + i); break;
		case PieceType::Bishop: board.AddPiece<Piece::BlackBishop>(56 + i); break;
		case PieceType::Rook: board.AddPiece<Piece::BlackRook>(56 + i); break;
		case PieceType::Queen: board.AddPiece<Piece::BlackQueen>(56 + i); break;
		case PieceType::King: board.AddPiece<Piece::BlackKing>(56 + i); break;
		}
	}

	// Place pawns
	for (int i = 0; i < 8; i++) {
		board.AddPiece<Piece::WhitePawn>(8 + i);
		board.AddPiece<Piece::BlackPawn>(48 + i);
	}

	// Castling
	board.SetWhiteShortCastlingRight<true>();
	board.SetWhiteLongCastlingRight<true>();
	board.SetBlackShortCastlingRight<true>();
	board.SetBlackLongCastlingRight<true>();
	CastlingConfig.WhiteLongCastleRookSquare = LsbSquare(board.WhiteRookBits);
	CastlingConfig.WhiteShortCastleRookSquare = MsbSquare(board.WhiteRookBits);
	CastlingConfig.BlackLongCastleRookSquare = LsbSquare(board.BlackRookBits);
	CastlingConfig.BlackShortCastleRookSquare = MsbSquare(board.BlackRookBits);

	// Other
	board.Turn = Side::White;
	board.BoardHash ^= Zobrist.SideToMove;
	board.EnPassantSquare = -1;
	board.HalfmoveClock = 0;
	board.FullmoveClock = 1;
	board.Threats = CalculateAttackedSquares(!Turn());
}

// Pushing moves ----------------------------------------------------------------------------------

void Position::PushMove(const Move& move) {
	assert(!move.IsNull());

	States.push_back(Board(CurrentState()));
	Board& board = CurrentState();
	const uint8_t movedPiece = board.GetPieceAt(move.from);

	board.ApplyMove(move, CastlingConfig);
	board.Threats = CalculateAttackedSquares(!Turn());

	Moves.push_back({ move, movedPiece });
	assert(States.size() - 1 == Moves.size());
}

void Position::PushNullMove() {
	States.push_back(Board(CurrentState()));
	Board& board = CurrentState();

	board.Turn = !board.Turn;
	if (board.Turn == Side::White) board.FullmoveClock += 1;
	board.Threats = CalculateAttackedSquares(!Turn());
	board.BoardHash ^= Zobrist.SideToMove;

	if (board.EnPassantSquare != -1) {
		board.BoardHash ^= Zobrist.EnPassant[GetSquareFile(board.EnPassantSquare)];
		board.EnPassantSquare = -1;
	}

	Moves.push_back({ NullMove, Piece::None });
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
	if (!Settings::Chess960) {
		using namespace Squares;
		if (piece == Piece::WhiteKing && sq1 == E1) {
			if (sq2 == C1) move = { E1, A1, MoveFlag::LongCastle };
			else if (sq2 == G1) move = { E1, H1, MoveFlag::ShortCastle};
		}
		else if (piece == Piece::BlackKing && sq1 == E8) {
			if (sq2 == C8) move = { E8, A8, MoveFlag::LongCastle };
			else if (sq2 == G8) move = { E8, H8, MoveFlag::ShortCastle };
		}
	}
	else {
		if ((piece == Piece::WhiteKing && capturedPiece == Piece::WhiteRook)
			|| (piece == Piece::BlackKing && capturedPiece == Piece::BlackRook)) {
			move.flag = (move.from < move.to) ? MoveFlag::ShortCastle : MoveFlag::LongCastle;
		}
	}

	// En passant possibility
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		const uint8_t r1 = GetSquareRank(sq1);
		const uint8_t r2 = GetSquareRank(sq2);
		if (std::abs(r2 - r1) == 2) move.flag = MoveFlag::EnPassantPossible;
	}

	// En passant performed
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		if ((TypeOfPiece(capturedPiece) == PieceType::None) && (GetSquareFile(sq1) != GetSquareFile(sq2))) {
			move.flag = MoveFlag::EnPassantPerformed;
		}
	}

	// Generate the list of valid moves
	MoveList legalMoves{};
	GenerateAllLegalMoves(legalMoves);
	const bool valid = std::any_of(legalMoves.begin(), legalMoves.end(), [&](const ScoredMove& sm) {
		return sm.move == move;
	});

	// Make the move if valid
	if (!valid) return false;
	else {
		PushMove(move);
		return true;
	}
}

void Position::PopMove() {
	States.pop_back();
	Moves.pop_back();
}

// Generating moves -------------------------------------------------------------------------------

template <bool side, uint8_t pieceType, MoveGen moveGen>
void Position::GenerateSlidingMoves(MoveList& moves, const uint8_t fromSquare, const uint64_t friendlyOccupancy, const uint64_t opponentOccupancy) const {
	const uint64_t occupancy = friendlyOccupancy | opponentOccupancy;
	uint64_t targets;

	switch (pieceType) {
	case PieceType::Bishop:
		targets = GetBishopAttacks(fromSquare, occupancy) & ~friendlyOccupancy;
		break;
	case PieceType::Rook:
		targets = GetRookAttacks(fromSquare, occupancy) & ~friendlyOccupancy;
		break;
	case PieceType::Queen:
		targets = GetQueenAttacks(fromSquare, occupancy) & ~friendlyOccupancy;
		break;
	default:
		// unreachable
		assert(false);
		break;
	}

	if constexpr (moveGen == MoveGen::Noisy) targets &= opponentOccupancy;
	else targets &= ~opponentOccupancy;

	while (targets) {
		const uint8_t toSquare = Popsquare(targets);
		moves.pushUnscored(Move(fromSquare, toSquare));
	}
}

template <bool side>
void Position::GenerateCastlingMoves(MoveList& moves) const {

	using namespace Squares;
	const Board& b = CurrentState();

	const bool rightToShortCastle = (side == Side::White) ? b.WhiteRightToShortCastle : b.BlackRightToShortCastle;
	const bool rightToLongCastle = (side == Side::White) ? b.WhiteRightToLongCastle : b.BlackRightToLongCastle;
	if (!rightToShortCastle && !rightToLongCastle) return;

	const uint64_t kingSq = (side == Side::White) ? WhiteKingSquare() : BlackKingSquare();
	const uint64_t occupancy = GetOccupancy();

	if (rightToShortCastle) {
		constexpr uint8_t kingTo = (side == Side::White) ? G1 : G8;
		constexpr uint8_t rookTo = (side == Side::White) ? F1 : F8;
		const uint8_t rookSq = (side == Side::White) ? CastlingConfig.WhiteShortCastleRookSquare : CastlingConfig.BlackShortCastleRookSquare;
		const uint64_t rayBetweenKingAndG = GetShortConnectingRay(kingSq, kingTo);
		const uint64_t rayBetweenRookAndF = GetShortConnectingRay(rookSq, rookTo);
		const uint64_t mockOccupancy = occupancy ^ (SquareBit(kingSq) | SquareBit(rookSq));
		const bool empty = !((rayBetweenKingAndG | rayBetweenRookAndF) & mockOccupancy);

		if (empty) {
			const bool safe = !(b.Threats & rayBetweenKingAndG);
			if (safe) moves.pushUnscored(Move(kingSq, rookSq, MoveFlag::ShortCastle));
		}
	}

	if (rightToLongCastle) {
		constexpr uint8_t kingTo = (side == Side::White) ? C1 : C8;
		constexpr uint8_t rookTo = (side == Side::White) ? D1 : D8;
		const uint8_t rookSq = (side == Side::White) ? CastlingConfig.WhiteLongCastleRookSquare : CastlingConfig.BlackLongCastleRookSquare;
		const uint64_t rayBetweenKingAndC = GetShortConnectingRay(kingSq, kingTo);
		const uint64_t rayBetweenRookAndF = GetShortConnectingRay(rookSq, rookTo);
		const uint64_t mockOccupancy = occupancy ^ (SquareBit(kingSq) | SquareBit(rookSq));
		const bool empty = !((rayBetweenKingAndC | rayBetweenRookAndF) & mockOccupancy);

		if (empty) {
			const bool safe = !(b.Threats & rayBetweenKingAndC);
			if (safe) moves.pushUnscored(Move(kingSq, rookSq, MoveFlag::LongCastle));
		}
	}
}

void Position::GenerateNoisyPseudoLegalMoves(MoveList& moves) const {
	if (CurrentState().Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::Noisy>(moves);
	else GeneratePseudolegalMoves<Side::Black, MoveGen::Noisy>(moves);
}

void Position::GenerateQuietPseudoLegalMoves(MoveList& moves) const {
	if (CurrentState().Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::Quiet>(moves);
	else GeneratePseudolegalMoves<Side::Black, MoveGen::Quiet>(moves);
}

void Position::GenerateAllPseudoLegalMoves(MoveList& moves) const {
	GenerateNoisyPseudoLegalMoves(moves);
	GenerateQuietPseudoLegalMoves(moves);
}

void Position::GenerateAllLegalMoves(MoveList& moves) const {
	MoveList pseudoLegalMoves{};
	GenerateAllPseudoLegalMoves(pseudoLegalMoves);
	for (const auto& m : pseudoLegalMoves) {
		if (IsLegalMove(m.move)) moves.pushUnscored(m.move);
	}
}

template <bool side, MoveGen moveGen>
void Position::GeneratePseudolegalMoves(MoveList& moves) const {
	const uint64_t whiteOccupancy = GetOccupancy(Side::White);
	const uint64_t blackOccupancy = GetOccupancy(Side::Black);
	const uint64_t occupancy = whiteOccupancy | blackOccupancy;
	const uint64_t friendlyOccupancy = (side == Side::White) ? whiteOccupancy : blackOccupancy;
	const uint64_t opponentOccupancy = (side == Side::White) ? blackOccupancy : whiteOccupancy;

	// Pawn moves
	if constexpr (moveGen == MoveGen::Noisy) GeneratePawnMovesNoisy<side>(moves);
	else GeneratePawnMovesQuiet<side>(moves);
	
	// Knight moves
	uint64_t friendlyKnights = (side == Side::White) ? WhiteKnightBits() : BlackKnightBits();
	while (friendlyKnights) {
		const uint8_t fromSquare = Popsquare(friendlyKnights);
		uint64_t targetBitboard = (moveGen == MoveGen::Noisy) ? KnightMoveBits[fromSquare] & opponentOccupancy : KnightMoveBits[fromSquare] & ~occupancy;
		while (targetBitboard) {
			const uint8_t toSquare = Popsquare(targetBitboard);
			moves.pushUnscored(Move(fromSquare, toSquare));
		}
	}

	// King moves
	uint8_t fromSquare = (side == Side::White) ? WhiteKingSquare() : BlackKingSquare();
	uint64_t targetBitboard = (moveGen == MoveGen::Noisy) ? KingMoveBits[fromSquare] & opponentOccupancy : KingMoveBits[fromSquare] & ~occupancy;
	while (targetBitboard) {
		const uint8_t toSquare = Popsquare(targetBitboard);
		moves.pushUnscored(Move(fromSquare, toSquare));
	}
	if constexpr (moveGen == MoveGen::Quiet) GenerateCastlingMoves<side>(moves);

	// Bishop moves
	uint64_t friendlyBishops = (side == Side::White) ? WhiteBishopBits() : BlackBishopBits();
	while (friendlyBishops) {
		const uint8_t fromSquare = Popsquare(friendlyBishops);
		GenerateSlidingMoves<side, PieceType::Bishop, moveGen>(moves, fromSquare, friendlyOccupancy, opponentOccupancy);
	}

	// Rook moves
	uint64_t friendlyRooks = (side == Side::White) ? WhiteRookBits() : BlackRookBits();
	while (friendlyRooks) {
		const uint8_t fromSquare = Popsquare(friendlyRooks);
		GenerateSlidingMoves<side, PieceType::Rook, moveGen>(moves, fromSquare, friendlyOccupancy, opponentOccupancy);
	}

	// Queen moves
	uint64_t friendlyQueens = (side == Side::White) ? WhiteQueenBits() : BlackQueenBits();
	while (friendlyQueens) {
		const uint8_t fromSquare = Popsquare(friendlyQueens);
		GenerateSlidingMoves<side, PieceType::Queen, moveGen>(moves, fromSquare, friendlyOccupancy, opponentOccupancy);
	}
}

template <bool side>
void Position::GeneratePawnMovesNoisy(MoveList& moves) const {
	// This code is rather repetitive, but it has just the right amount of variation that makes abstraction non-trivial
	const Board& b = CurrentState();
	const uint64_t occupancy = b.GetOccupancy();
	const uint64_t opponentOccupancy = b.GetOccupancyForSide(!side);

	// Promotions with capture
	if constexpr (side == Side::White) {
		// Left
		uint64_t destinationsLeft = (b.WhitePawnBits << 7) & opponentOccupancy & Rank[7] & ~File[7];
		while (destinationsLeft) {
			const uint8_t toSq = Popsquare(destinationsLeft);
			const uint8_t fromSq = toSq - 7;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToQueen));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToKnight));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToRook));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToBishop));
		}
		// Right
		uint64_t destinationsRight = (b.WhitePawnBits << 9) & opponentOccupancy & Rank[7] & ~File[0];
		while (destinationsRight) {
			const uint8_t toSq = Popsquare(destinationsRight);
			const uint8_t fromSq = toSq - 9;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToQueen));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToKnight));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToRook));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToBishop));
		}
	}
	else {
		// Left
		uint64_t destinationsLeft = (b.BlackPawnBits >> 9) & opponentOccupancy & Rank[0] & ~File[7];
		while (destinationsLeft) {
			const uint8_t toSq = Popsquare(destinationsLeft);
			const uint8_t fromSq = toSq + 9;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToQueen));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToKnight));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToRook));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToBishop));
		}
		// Right
		uint64_t destinationsRight = (b.BlackPawnBits >> 7) & opponentOccupancy & Rank[0] & ~File[0];
		while (destinationsRight) {
			const uint8_t toSq = Popsquare(destinationsRight);
			const uint8_t fromSq = toSq + 7;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToQueen));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToKnight));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToRook));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToBishop));
		}
	}

	// Queen promotions without capture
	if constexpr (side == Side::White) {
		uint64_t destinations = (b.WhitePawnBits << 8) & ~occupancy & Rank[7];
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq - 8;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToQueen));
		}
	}
	else {
		uint64_t destinations = (b.BlackPawnBits >> 8) & ~occupancy & Rank[0];
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq + 8;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToQueen));
		}
	}

	// Regular captures
	if constexpr (side == Side::White) {
		// Left
		uint64_t destinationsLeft = (b.WhitePawnBits << 7) & opponentOccupancy & ~Rank[7] & ~File[7];
		while (destinationsLeft) {
			const uint8_t toSq = Popsquare(destinationsLeft);
			const uint8_t fromSq = toSq - 7;
			moves.pushUnscored(Move(fromSq, toSq));
		}
		// Right
		uint64_t destinationsRight = (b.WhitePawnBits << 9) & opponentOccupancy & ~Rank[7] & ~File[0];
		while (destinationsRight) {
			const uint8_t toSq = Popsquare(destinationsRight);
			const uint8_t fromSq = toSq - 9;
			moves.pushUnscored(Move(fromSq, toSq));
		}
	}
	else {
		// Left
		uint64_t destinationsLeft = (b.BlackPawnBits >> 9) & opponentOccupancy & ~Rank[0] & ~File[7];
		while (destinationsLeft) {
			const uint8_t toSq = Popsquare(destinationsLeft);
			const uint8_t fromSq = toSq + 9;
			moves.pushUnscored(Move(fromSq, toSq));
		}
		// Right
		uint64_t destinationsRight = (b.BlackPawnBits >> 7) & opponentOccupancy & ~Rank[0] & ~File[0];
		while (destinationsRight) {
			const uint8_t toSq = Popsquare(destinationsRight);
			const uint8_t fromSq = toSq + 7;
			moves.pushUnscored(Move(fromSq, toSq));
		}
	}

	// En passant
	if (b.EnPassantSquare != -1) {
		if constexpr (side == Side::White) {
			// Left
			if (b.EnPassantSquare != 47 && GetPieceAt(b.EnPassantSquare - 7) == Piece::WhitePawn) {
				moves.pushUnscored(Move(b.EnPassantSquare - 7, b.EnPassantSquare, MoveFlag::EnPassantPerformed));
			}
			// Right
			if (b.EnPassantSquare != 40 && GetPieceAt(b.EnPassantSquare - 9) == Piece::WhitePawn) {
				moves.pushUnscored(Move(b.EnPassantSquare - 9, b.EnPassantSquare, MoveFlag::EnPassantPerformed));
			}
		}
		else {
			// Left
			if (b.EnPassantSquare != 16 && GetPieceAt(b.EnPassantSquare + 7) == Piece::BlackPawn) {
				moves.pushUnscored(Move(b.EnPassantSquare + 7, b.EnPassantSquare, MoveFlag::EnPassantPerformed));
			}
			// Right
			if (b.EnPassantSquare != 23 && GetPieceAt(b.EnPassantSquare + 9) == Piece::BlackPawn) {
				moves.pushUnscored(Move(b.EnPassantSquare + 9, b.EnPassantSquare, MoveFlag::EnPassantPerformed));
			}
		}
	}

}

template <bool side>
void Position::GeneratePawnMovesQuiet(MoveList& moves) const {

	const Board& b = CurrentState();
	const uint64_t occupancy = b.GetOccupancy();
	
	// Generate double pushes: has to be on the 4th rank afterwards, can't be blocked by another piece
	if constexpr (side == Side::White) {
		uint64_t destinations = (b.WhitePawnBits << 16) & Rank[3] & ~occupancy & (~occupancy << 8);
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq - 16;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::EnPassantPossible));
		}
	}
	else {
		uint64_t destinations = (b.BlackPawnBits >> 16) & Rank[4] & ~occupancy & (~occupancy >> 8);
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq + 16;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::EnPassantPossible));
		}
	}

	// Generate non-promotion single pushes
	if constexpr (side == Side::White) {
		uint64_t destinations = (b.WhitePawnBits << 8) & ~occupancy & ~Rank[7];
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq - 8;
			moves.pushUnscored(Move(fromSq, toSq));
		}
	}
	else {
		uint64_t destinations = (b.BlackPawnBits >> 8) & ~occupancy & ~Rank[0];
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq + 8;
			moves.pushUnscored(Move(fromSq, toSq));
		}
	}

	// Generate quiet underpromotions
	if constexpr (side == Side::White) {
		uint64_t destinations = (b.WhitePawnBits << 8) & ~occupancy & Rank[7];
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq - 8;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToKnight));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToRook));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToBishop));
		}
	}
	else {
		uint64_t destinations = (b.BlackPawnBits >> 8) & ~occupancy & Rank[0];
		while (destinations) {
			const uint8_t toSq = Popsquare(destinations);
			const uint8_t fromSq = toSq + 8;
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToKnight));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToRook));
			moves.pushUnscored(Move(fromSq, toSq, MoveFlag::PromotionToBishop));
		}
	}
}


// Threats and move legality ----------------------------------------------------------------------

uint64_t Position::CalculateAttackedSquares(const bool attackingSide) const {

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

// Checks if the move is pseudo-legal (it's not obviously wrong, e.g. a pawn castling)
// Does not account for whether the side to move is in check
// The accuracy of this function was tested, and there weren't any discrepancies in thousands of games
bool Position::IsPseudoLegalMove(const Move& m) const {

	// Piece and move must exist
	const uint8_t piece = GetPieceAt(m.from);
	if (piece == Piece::None || m.from == m.to) return false;

	// Piece must be the right color
	const bool turn = Turn();
	const uint8_t pieceColor = ColorOfPiece(piece);
	const uint8_t pieceType = TypeOfPiece(piece);
	if (pieceColor != SideToPieceColor(turn)) return false;

	// Check for invalid move types
	if (pieceType != PieceType::Pawn && (m.IsPromotion() || m.flag == MoveFlag::EnPassantPerformed || m.flag == MoveFlag::EnPassantPossible)) return false;
	if (pieceType != PieceType::King && m.IsCastling()) return false;

	const uint8_t capturedPiece = GetPieceAt(m.to);
	const uint8_t capturedPieceColor = ColorOfPiece(capturedPiece);
	const uint8_t capturedPieceType = TypeOfPiece(capturedPiece);
	const uint64_t occupancy = GetOccupancy();

	// Capturing our own pieces or a king
	if (capturedPiece != Piece::None && !m.IsCastling()) {
		if (pieceColor == capturedPieceColor) return false;
		if (capturedPieceType == PieceType::King) return false;
	}

	// King moves
	if (pieceType == PieceType::King) {
		if (m.IsCastling()) {
			// Castling is encoded as king takes rook
			if (capturedPieceType != PieceType::Rook || capturedPieceColor != pieceColor) return false;

			// The king and the rook must be on the backrank
			const uint8_t fromRankRel = (pieceColor == PieceColor::White) ? GetSquareRank(m.from) : 7 - GetSquareRank(m.from);
			const uint8_t toRankRel = (pieceColor == PieceColor::White) ? GetSquareRank(m.to) : 7 - GetSquareRank(m.to);
			if (fromRankRel != 0 || toRankRel != 0) return false;

			// Are we really castling the right way?
			const bool castleKingside = m.flag == MoveFlag::ShortCastle;
			const bool kingLeftOfRook = m.from < m.to;
			if (castleKingside != kingLeftOfRook) return false;

			// Check castling rights
			const Board& b = CurrentState();
			if (pieceColor == PieceColor::White) {
				if ((castleKingside && !b.WhiteRightToShortCastle) || (!castleKingside && !b.WhiteRightToLongCastle)) return false;
			}
			else {
				if ((castleKingside && !b.BlackRightToShortCastle) || (!castleKingside && !b.BlackRightToLongCastle)) return false;
			}

			const uint8_t kingSqBeforeCastling = m.from;
			const uint8_t kingSqAfterCastling = (castleKingside ? Squares::G1 : Squares::C1) + 56 * (pieceColor == PieceColor::Black);
			const uint8_t rookSqBeforeCastling = m.to;
			const uint8_t rookSqAfterCastling = (castleKingside ? Squares::F1 : Squares::D1) + 56 * (pieceColor == PieceColor::Black);
			const uint64_t rayBetweenKingPositions = GetShortConnectingRay(kingSqBeforeCastling, kingSqAfterCastling);
			const uint64_t rayBetweenRookPositions = GetShortConnectingRay(rookSqBeforeCastling, rookSqAfterCastling);

			// Other pieces as obstacle
			const uint64_t fakeOccupancy = occupancy ^ (SquareBit(kingSqBeforeCastling) | SquareBit(rookSqBeforeCastling));
			const bool empty = !((rayBetweenKingPositions | rayBetweenRookPositions) & fakeOccupancy);
			if (!empty) return false;

			// Can't traverse through check
			const uint64_t opponentAttacks = b.Threats;
			const bool safe = !(opponentAttacks & rayBetweenKingPositions);
			if (!safe) return false;

			return true;
		}
		else {
			return CheckBit(KingMoveBits[m.from], m.to);
		}
	}
	
	// Pawn moves
	else if (pieceType == PieceType::Pawn) {

		const uint8_t fromRankRel = (pieceColor == PieceColor::White) ? GetSquareRank(m.from) : 7 - GetSquareRank(m.from);
		const uint8_t toRankRel = (pieceColor == PieceColor::White) ? GetSquareRank(m.to) : 7 - GetSquareRank(m.to);

		if (m.flag == MoveFlag::EnPassantPerformed) {
			// En passant square should match and is reachable
			if (CurrentState().EnPassantSquare != m.to) return false;
			return (pieceColor == PieceColor::White && CheckBit(WhitePawnAttacks[m.from], m.to))
				|| (pieceColor == PieceColor::Black && CheckBit(BlackPawnAttacks[m.from], m.to));
		}

		else if (m.flag == MoveFlag::EnPassantPossible) {
			// Have to move from the second rank
			if (fromRankRel != 1) return false;

			// Have to arrive at the right square
			if ((pieceColor == PieceColor::White && m.to - m.from != 16)
				|| (pieceColor == PieceColor::Black && m.from - m.to != 16)) return false;

			// The path must be clear
			if (pieceColor == PieceColor::White) {
				if (GetPieceAt(m.from + 8) != Piece::None || GetPieceAt(m.from + 16) != Piece::None) return false;
			}
			else {
				if (GetPieceAt(m.from - 8) != Piece::None || GetPieceAt(m.from - 16) != Piece::None) return false;
			}
		}

		else {
			// Can't move to the eighth rank without promotion / move not to the eighth rank and somehow still promoting
			if (toRankRel == 7 && !m.IsPromotion()) return false;
			if (toRankRel != 7 && m.IsPromotion()) return false;

			if (capturedPiece != Piece::None) {
				// Capture, check if the destination square is attacked from the original square
				if ((pieceColor == PieceColor::White && !CheckBit(WhitePawnAttacks[m.from], m.to))
					|| (pieceColor == PieceColor::Black && !CheckBit(BlackPawnAttacks[m.from], m.to))) return false;
			}
			else {
				// Otherwise the move must be a simple push forward then
				if ((pieceColor == PieceColor::White && m.to - m.from != 8)
					|| (pieceColor == PieceColor::Black && m.from - m.to != 8)) return false;
			}
		}

		return true;
	}

	// Other pieces
	else {
		switch (pieceType) {
		case PieceType::Knight:
			return CheckBit(KnightMoveBits[m.from], m.to);
		case PieceType::Bishop:
			return CheckBit(GetBishopAttacks(m.from, occupancy), m.to);
		case PieceType::Rook:
			return CheckBit(GetRookAttacks(m.from, occupancy), m.to);
		case PieceType::Queen:
			return CheckBit(GetQueenAttacks(m.from, occupancy), m.to);
		default:
			assert(false);
			return false;
		}
	}
}

// This function assumes that the move is at least pseudolegal
// This is terrible currently, but it was pain to get right, and I don't want to touch it with a 10 ft pole
bool Position::IsLegalMove(const Move& m) const {
	
	assert(!m.IsNull());
	if (m.IsCastling()) return true; // Castling is guaranteed to be legal from movegen

	const Board& board = CurrentState();
	const uint8_t movedPiece = GetPieceAt(m.from);

	if (TypeOfPiece(movedPiece) == PieceType::King) {
		// Destination square must not be attacked by the opponent
		const uint8_t kingSq = (board.Turn == Side::White) ? LsbSquare(board.WhiteKingBits) : LsbSquare(board.BlackKingBits);
		const uint64_t occupancy = GetOccupancy() ^ SquareBit(kingSq);
		return !IsSquareAttacked(!board.Turn, m.to, occupancy);
	}

	const uint8_t kingSq = (board.Turn == Side::White) ? LsbSquare(board.WhiteKingBits) : LsbSquare(board.BlackKingBits);
	const uint64_t occupancy = GetOccupancy();

	if (m.flag == MoveFlag::EnPassantPerformed) {
		// After the en passant start rays to see if the king is attacked by an appropiate sliding piece
		const uint8_t epVictimSq = (board.Turn == Side::White) ? board.EnPassantSquare - 8 : board.EnPassantSquare + 8;
		const uint64_t parallelSliders = (board.Turn == Side::White) ? (board.BlackRookBits | board.BlackQueenBits) : (board.WhiteRookBits | board.WhiteQueenBits);
		const uint64_t diagonalSliders = (board.Turn == Side::White) ? (board.BlackBishopBits | board.BlackQueenBits) : (board.WhiteBishopBits | board.WhiteQueenBits);
		const uint64_t approxOccupancy = (occupancy ^ SquareBit(m.from) ^ SquareBit(epVictimSq)) | SquareBit(m.to);
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

bool Position::IsSquareAttacked(const bool attackingSide, const uint8_t square, const uint64_t occupancy) const {
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
		attackers |= KnightMoveBits[square] & b.WhiteKnightBits;
		attackers |= KingMoveBits[square] & b.WhiteKingBits;
		attackers |= ((SquareBit(square) & ~File[0]) >> 9) & b.WhitePawnBits;
		attackers |= ((SquareBit(square) & ~File[7]) >> 7) & b.WhitePawnBits;
		attackers |= GetRookAttacks(square, occupancy) & (b.WhiteRookBits | b.WhiteQueenBits);
		attackers |= GetBishopAttacks(square, occupancy) & (b.WhiteBishopBits | b.WhiteQueenBits);
	}
	else {
		attackers |= KnightMoveBits[square] & b.BlackKnightBits;
		attackers |= KingMoveBits[square] & b.BlackKingBits;
		attackers |= ((SquareBit(square) & ~File[7]) << 9) & b.BlackPawnBits;
		attackers |= ((SquareBit(square) & ~File[0]) << 7) & b.BlackPawnBits;
		attackers |= GetRookAttacks(square, occupancy) & (b.BlackRookBits | b.BlackQueenBits);
		attackers |= GetBishopAttacks(square, occupancy) & (b.BlackBishopBits | b.BlackQueenBits);
	}
	return attackers;
}

// Returns a bitboard of the pinned pieces for each side
std::pair<uint64_t, uint64_t> Position::GetPinnedBitboard() const {

	uint64_t whitePinned{}, blackPinned{};

	const uint8_t whiteKingSquare = WhiteKingSquare();
	const uint8_t blackKingSquare = BlackKingSquare();
	const uint64_t whiteOccupancy = GetOccupancy(Side::White);
	const uint64_t blackOccupancy = GetOccupancy(Side::Black);

	// Calculate pins for white
	const uint64_t blackPotentialParallelPinners = GetRookAttacks(whiteKingSquare, blackOccupancy) & (BlackRookBits() | BlackQueenBits());
	const uint64_t blackPotentialDiagonalPinners = GetBishopAttacks(whiteKingSquare, blackOccupancy) & (BlackBishopBits() | BlackQueenBits());
	uint64_t blackPotentialPinners = blackPotentialParallelPinners | blackPotentialDiagonalPinners;

	while (blackPotentialPinners) {
		const uint8_t sq = Popsquare(blackPotentialPinners);
		const uint64_t whitePiecesBetween = (GetShortConnectingRay(sq, whiteKingSquare) & ~SquareBit(sq) & ~SquareBit(whiteKingSquare)) & GetOccupancy(Side::White);
		if (Popcount(whitePiecesBetween) == 1) whitePinned |= whitePiecesBetween;
	}

	// Calculate pins for black
	const uint64_t whitePotentialParallelPinners = GetRookAttacks(blackKingSquare, whiteOccupancy) & (WhiteRookBits() | WhiteQueenBits());
	const uint64_t whitePotentialDiagonalPinners = GetBishopAttacks(blackKingSquare, whiteOccupancy) & (WhiteBishopBits() | WhiteQueenBits());
	uint64_t whitePotentialPinners = whitePotentialParallelPinners | whitePotentialDiagonalPinners;

	while (whitePotentialPinners) {
		const uint8_t sq = Popsquare(whitePotentialPinners);
		const uint64_t blackPiecesBetween = (GetShortConnectingRay(sq, blackKingSquare) & ~SquareBit(sq) & ~SquareBit(blackKingSquare)) & GetOccupancy(Side::Black);
		if (Popcount(blackPiecesBetween) == 1) blackPinned |= blackPiecesBetween;
	}

	return { whitePinned, blackPinned };
}

// Getting information ----------------------------------------------------------------------------

bool Position::IsDrawn(const int level) const {
	const Board& b = CurrentState();

	// 1. Fifty moves without progress
	if (b.HalfmoveClock >= 100) return true;

	// 2. Threefold repetitions
	const uint64_t hash = Hash();
	const int length = States.size();
	const int currentIndex = length - 1;
	const int materializedUntil = currentIndex - level; // highest index materialized
	int repeated = 1;

	for (int i = currentIndex - 4; i >= std::max(0, currentIndex - b.HalfmoveClock); i -= 2) {
		if (States[i].BoardHash == hash) {
			repeated += 1;
			const bool materialized = materializedUntil >= i;
			if (repeated >= (2 + materialized)) return true;
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

bool Position::HasUpcomingRepetition(const int level) const {
	// Detects if there is a move to make a repetition
	// The basic idea is that instead of directly comparing hashes, we can express their difference, and then use
	// cuckoo tables for a O(1) hash difference lookup to find whether there is move to revert to a previous state.
	// Move legality needs to be checked, but a costly IsLegal() can be avoided
	// Source: https://web.archive.org/web/20201107002606/https://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf
	// See also: Cuckoo table generation in Magics.cpp (its temporary home)

	const int end = std::min(static_cast<size_t>(CurrentState().HalfmoveClock), States.size() - 1);
	if (Moves.back().move.IsNull() || end < 3) return false;

	const auto previousHash = [this](const int d) -> uint64_t {
		return GetPreviousState(d).BoardHash;
	};

	const uint64_t originalHash = CurrentState().BoardHash;
	const uint64_t originalOccupancy = GetOccupancy();	
	uint64_t other = originalHash ^ previousHash(1) ^ Zobrist.SideToMove;

	for (int d = 3; d <= end; d += 2) {
		const uint64_t currentHash = previousHash(d);
		other ^= currentHash ^ previousHash(d - 1) ^ Zobrist.SideToMove;
		if (other != 0) continue;

		// Symmetric difference between the hashes
		const uint64_t diff = originalHash ^ currentHash;

		// Find the possibly reversing move
		const std::optional<int> slot = [&] {
			int slotCandidate = CuckooH1(diff);
			if (diff != GetCuckooHash(slotCandidate)) slotCandidate = CuckooH2(diff);
			if (diff != GetCuckooHash(slotCandidate)) return std::optional<int>(std::nullopt);
			return std::optional<int>(slotCandidate);
		}();
		if (!slot.has_value()) continue;

		const Move& move = GetCuckooMove(slot.value());

		// Make sure there's no piece blocking and preventing the move
		const uint64_t rayBetween = GetShortConnectingRay(move.from, move.to) & ~SquareBit(move.from) & ~SquareBit(move.to);
		if (originalOccupancy & rayBetween) continue;
		
		// Two-fold when repetition is after the root 
		if (level > d) return true;

		// Look for another repetition otherwise
		for (int i = d + 4; i <= end; i += 2) {
			if (previousHash(d) == previousHash(i)) return true;
		}
	}

	return false;
}

bool Position::IsMoveQuiet(const Move& move) const {
	if (move.IsCastling()) return true;
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
			const uint8_t piece = GetPieceAt(Square(r, f));
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

	if (!Settings::Chess960) {
		bool castlingPossible = false;
		if (b.WhiteRightToShortCastle) { result += 'K'; castlingPossible = true; }
		if (b.WhiteRightToLongCastle) { result += 'Q'; castlingPossible = true; }
		if (b.BlackRightToShortCastle) { result += 'k'; castlingPossible = true; }
		if (b.BlackRightToLongCastle) { result += 'q'; castlingPossible = true; }
		if (!castlingPossible) result += '-';
	}
	else {
		bool castlingPossible = false;
		if (b.WhiteRightToShortCastle) { result += ('A' + CastlingConfig.WhiteShortCastleRookSquare); castlingPossible = true; }
		if (b.WhiteRightToLongCastle) { result += ('A' + CastlingConfig.WhiteLongCastleRookSquare); castlingPossible = true; }
		if (b.BlackRightToShortCastle) { result += ('a' + CastlingConfig.BlackShortCastleRookSquare - 56); castlingPossible = true; }
		if (b.BlackRightToLongCastle) { result += ('a' + CastlingConfig.BlackLongCastleRookSquare - 56); castlingPossible = true; }
		if (!castlingPossible) result += '-';
	}
	result += ' ';

	// There are two versions of this: after a double push printing the target square unconditionally, or only when e.p. is a legal move
	// Renegade does the latter
	if (b.EnPassantSquare != -1) result += SquareStrings[b.EnPassantSquare];
	else result += '-';

	result += ' ' + std::to_string(b.HalfmoveClock) + ' ' + std::to_string(b.FullmoveClock);
	return result;
}

GameState Position::GetGameState() const {
	// Check checkmates & stalemates
	MoveList moves{};
	GenerateAllLegalMoves(moves);

	if (moves.size() == 0) {
		if (IsInCheck()) return (Turn() == Side::Black) ? GameState::WhiteVictory : GameState::BlackVictory;
		else return GameState::Drawn; // Stalemate
	}

	// Check other types of draws
	if (IsDrawn(0)) return GameState::Drawn;
	else return GameState::Playing;
}

// Static exchange evaluation (SEE) ---------------------------------------------------------------

bool Position::StaticExchangeEval(const Move& move, const int threshold) const {
	// Approximates the outcome of all captures targeting the move.to square
	// Highly useful for pruning and for move ordering
	// This iterative approach is the standard way of doing this with some additional code for pinned piece handling

	constexpr auto seeValues = std::array{ 0, 100, 300, 300, 500, 1000, 999999 };

	// Get the initial piece
	uint8_t victim = TypeOfPiece(GetPieceAt(move.from));
	if (move.IsPromotion()) victim = move.GetPromotionPieceType();

	// Get estimated move value
	int estimatedMoveValue = seeValues[TypeOfPiece(GetPieceAt(move.to))];
	if (move.IsPromotion()) estimatedMoveValue += seeValues[move.GetPromotionPieceType()] - seeValues[PieceType::Pawn];
	else if (move.flag == MoveFlag::EnPassantPerformed) estimatedMoveValue = seeValues[PieceType::Pawn];
	else if (move.IsCastling()) estimatedMoveValue = 0;

	// Handle trivial cases (losing the piece for nothing still above / having initial gain below threshold)
	int score = -threshold;
	score += estimatedMoveValue;
	if (score < 0) return false;
	score -= seeValues[victim];
	if (score >= 0) return true;

	// Lookups (should be optimized) 
	const uint64_t whitePieces = GetOccupancy(Side::White);
	const uint64_t blackPieces = GetOccupancy(Side::Black);
	const uint64_t parallels = WhiteRookBits() | BlackRookBits() | WhiteQueenBits() | BlackQueenBits();
	const uint64_t diagonals = WhiteBishopBits() | BlackBishopBits() | WhiteQueenBits() | BlackQueenBits();
	uint64_t occupancy = whitePieces | blackPieces;
	SetBitFalse(occupancy, move.from);
	SetBitTrue(occupancy, move.to);
	bool turn = Turn();
	if (move.flag == MoveFlag::EnPassantPerformed) {
		SetBitFalse(occupancy, (turn == Side::White) ? move.to - 8 : move.to + 8);
	}
	turn = !turn;

	// Account for pinned pieces
	const auto [whitePinned, blackPinned] = GetPinnedBitboard();
	const uint64_t whiteAllowedPinned = whitePinned & GetLongConnectingRay(move.to, WhiteKingSquare());
	const uint64_t blackAllowedPinned = blackPinned & GetLongConnectingRay(move.to, BlackKingSquare());
	const uint64_t allowed = ~(whitePinned | blackPinned) | whiteAllowedPinned | blackAllowedPinned;

	uint64_t attackers = GetAttackersOfSquare(move.to, occupancy) & occupancy & allowed;

	// Pseudo-generating steps
	while (true) {

		uint64_t currentAttackers = attackers & ((turn == Side::White) ? whitePieces : blackPieces);
		if (!currentAttackers) break;

		// Retrieve the location of the least valuable attacking piece type
		int sq = -1;
		if (turn == Side::White) {
			if (currentAttackers & WhitePawnBits()) sq = LsbSquare(currentAttackers & WhitePawnBits());
			else if (currentAttackers & WhiteKnightBits()) sq = LsbSquare(currentAttackers & WhiteKnightBits());
			else if (currentAttackers & WhiteBishopBits()) sq = LsbSquare(currentAttackers & WhiteBishopBits());
			else if (currentAttackers & WhiteRookBits()) sq = LsbSquare(currentAttackers & WhiteRookBits());
			else if (currentAttackers & WhiteQueenBits()) sq = LsbSquare(currentAttackers & WhiteQueenBits());
			else if (currentAttackers & WhiteKingBits()) sq = LsbSquare(currentAttackers & WhiteKingBits());
		}
		else {
			if (currentAttackers & BlackPawnBits()) sq = LsbSquare(currentAttackers & BlackPawnBits());
			else if (currentAttackers & BlackKnightBits()) sq = LsbSquare(currentAttackers & BlackKnightBits());
			else if (currentAttackers & BlackBishopBits()) sq = LsbSquare(currentAttackers & BlackBishopBits());
			else if (currentAttackers & BlackRookBits()) sq = LsbSquare(currentAttackers & BlackRookBits());
			else if (currentAttackers & BlackQueenBits()) sq = LsbSquare(currentAttackers & BlackQueenBits());
			else if (currentAttackers & BlackKingBits()) sq = LsbSquare(currentAttackers & BlackKingBits());
		}
		assert(sq != -1);

		// Update fields
		victim = TypeOfPiece(GetPieceAt(sq));
		SetBitFalse(occupancy, sq);

		// Update potentially uncovered sliding pieces
		if (victim == PieceType::Pawn || victim == PieceType::Bishop || victim == PieceType::Queen) {
			attackers |= GetBishopAttacks(move.to, occupancy) & diagonals;
		}
		if (victim == PieceType::Rook || victim == PieceType::Queen) {
			attackers |= GetRookAttacks(move.to, occupancy) & parallels;
		}

		attackers &= occupancy;
		turn = !turn;

		// Break conditions
		score = -score - seeValues[victim] - 1;
		if (score >= 0) {
			const uint64_t upcomingOccupancy = (turn == Side::White) ? whitePieces : blackPieces;
			if (victim == PieceType::King && (currentAttackers & upcomingOccupancy)) {
				turn = !turn;
			}
			break;
		}
	}

	// If after the exchange it's our opponent's turn, that means we won
	return turn != Turn();
}
