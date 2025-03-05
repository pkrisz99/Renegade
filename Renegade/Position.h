#pragma once
#include "Board.h"
#include "Move.h"
#include "Settings.h"
#include "Utils.h"

// Magic lookup tables
uint64_t GetBishopAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetRookAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetQueenAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetConnectingRay(const uint8_t from, const uint64_t to);

class Position
{
public:

	Position(const std::string& fen);
	Position() : Position(FEN::StartPos) {};
	Position(const int frcWhite, const int frcBlack);

	void PushMove(const Move& move);
	void PushNullMove();
	bool PushUCI(const std::string& str);
	void PopMove();

	void GenerateMoves(MoveList& moves, const MoveGen moveGen, const Legality legality) const;
	bool IsDrawn(const bool threefold) const;

	bool IsLegalMove(const Move& m) const;
	bool IsMoveQuiet(const Move& move) const;

	inline Board& CurrentState() {
		return States.back();
	}

	inline const Board& CurrentState() const {
		return States.back();
	}

	inline uint64_t GetOccupancy() const {
		const Board& b = States.back();
		return b.WhitePawnBits | b.WhiteKnightBits | b.WhiteBishopBits | b.WhiteRookBits | b.WhiteQueenBits | b.WhiteKingBits
			| b.BlackPawnBits | b.BlackKnightBits | b.BlackBishopBits | b.BlackRookBits | b.BlackQueenBits | b.BlackKingBits;
	}

	inline uint64_t GetOccupancy(const bool side) const {
		const Board& b = States.back();
		if (side == Side::White) return b.WhitePawnBits | b.WhiteKnightBits | b.WhiteBishopBits | b.WhiteRookBits | b.WhiteQueenBits | b.WhiteKingBits;
		else return b.BlackPawnBits | b.BlackKnightBits | b.BlackBishopBits | b.BlackRookBits | b.BlackQueenBits | b.BlackKingBits;
	}

	inline uint8_t GetPieceAt(const uint8_t square) const {
		return States.back().GetPieceAt(square);
	}

	inline bool Turn() const {
		return States.back().Turn;
	}

	inline bool IsInCheck() const {
		const uint8_t kingSq = (Turn() == Side::White) ? WhiteKingSquare() : BlackKingSquare();
		return IsSquareThreatened(kingSq);
	}

	inline uint64_t Hash() const {
		return Hashes.back();
	}

	inline int GetPly() const {
		const Board& b = States.back();
		return (b.FullmoveClock - 1) * 2 + (b.Turn == Side::White ? 0 : 1);
	}

	inline bool ZugzwangUnlikely() const {
		const Board& b = States.back();
		if (b.Turn == Side::White) return (b.WhiteKnightBits | b.WhiteBishopBits | b.WhiteRookBits | b.WhiteQueenBits) != 0ull;
		else return (b.BlackKnightBits | b.BlackBishopBits | b.BlackRookBits | b.BlackQueenBits) != 0ull;
	}

	inline int GetGamePhase() const {
		// 24 at the beginning of the game -> 0 for kings and pawns only
		return (Popcount(WhiteKnightBits() | BlackKnightBits()))
			+ (Popcount(WhiteBishopBits() | BlackBishopBits()))
			+ (Popcount(WhiteRookBits() | BlackRookBits())) * 2
			+ (Popcount(WhiteQueenBits() | BlackQueenBits())) * 4;
	}

	template <bool side>
	inline uint64_t GetPawnAttacks() const {
		const Board& b = CurrentState();
		if constexpr (side == Side::White) return ((b.WhitePawnBits & ~File[0]) << 7) | ((b.WhitePawnBits & ~File[7]) << 9);
		else return ((b.BlackPawnBits & ~File[0]) >> 9) | ((b.BlackPawnBits & ~File[7]) >> 7);
	}

	inline uint64_t GenerateKnightAttacks(const int from) const {
		return KnightMoveBits[from];
	}

	inline uint64_t GenerateKingAttacks(const int from) const {
		return KingMoveBits[from];
	}

	inline const MoveAndPiece& GetPreviousMove(const int plies) const {
		assert(plies > 0);
		assert(plies <= Moves.size());
		return Moves[Moves.size() - plies];
	}

	inline bool IsPreviousMoveNull() const {
		return Moves.size() != 0 && Moves.back().move == NullMove;
	}

	inline uint64_t GetThreats() const {
		assert(Threats.back() != 0ull);
		return Threats.back();
	}

	inline bool IsSquareThreatened(const uint8_t sq) const {
		assert(Threats.back() != 0ull);
		return CheckBit(Threats.back(), sq);
	}

	inline uint64_t GetMaterialKey() const {
		return States.back().CalculateMaterialKey();
	}

	inline uint64_t GetPawnKey() const {
		return MurmurHash3(WhitePawnBits()) ^ MurmurHash3(BlackPawnBits() ^ Zobrist[780]);
	}

	inline uint64_t ApproximateHashAfterMove(const Move& move) const {
		// Calculate the approximate hash after a move on the current board
		// This is to make prefetching more efficient
		// It doesn't need to be perfect, just good enough, it handles most quiet moves and captures
		uint64_t hash = Hashes.back() ^ Zobrist[780];
		const uint8_t movedPiece = GetPieceAt(move.from);
		const uint8_t capturedPiece = GetPieceAt(move.to);
		constexpr std::array<uint8_t, 15> pieceMapping = { 255, 0, 1, 2, 3, 4, 5, 255, 255, 6, 7, 8, 9, 10, 11 };
		hash ^= Zobrist[64 * pieceMapping[movedPiece] + move.from];
		hash ^= Zobrist[64 * pieceMapping[movedPiece] + move.to];
		if (capturedPiece != Piece::None) hash ^= Zobrist[64 * pieceMapping[capturedPiece] + move.to];
		return hash;
	}

	uint64_t AttackersOfSquare(const bool attackingSide, const uint8_t square) const;

	inline uint64_t WhitePawnBits() const { return States.back().WhitePawnBits; }
	inline uint64_t WhiteKnightBits() const { return States.back().WhiteKnightBits; }
	inline uint64_t WhiteBishopBits() const { return States.back().WhiteBishopBits; }
	inline uint64_t WhiteRookBits() const { return States.back().WhiteRookBits; }
	inline uint64_t WhiteQueenBits() const { return States.back().WhiteQueenBits; }
	inline uint64_t WhiteKingBits() const { return States.back().WhiteKingBits; }
	inline uint64_t BlackPawnBits() const { return States.back().BlackPawnBits; }
	inline uint64_t BlackKnightBits() const { return States.back().BlackKnightBits; }
	inline uint64_t BlackBishopBits() const { return States.back().BlackBishopBits; }
	inline uint64_t BlackRookBits() const { return States.back().BlackRookBits; }
	inline uint64_t BlackQueenBits() const { return States.back().BlackQueenBits; }
	inline uint64_t BlackKingBits() const { return States.back().BlackKingBits; }

	inline uint8_t WhiteKingSquare() const { return LsbSquare(States.back().WhiteKingBits); }
	inline uint8_t BlackKingSquare() const { return LsbSquare(States.back().BlackKingBits); }

	uint64_t GetAttackersOfSquare(const uint8_t square, const uint64_t occupied) const;
	std::string GetFEN() const;
	GameState GetGameState() const;
	bool StaticExchangeEval(const Move& move, const int threshold) const;

	std::vector<Board> States{};
	std::vector<uint64_t> Hashes{};
	std::vector<uint64_t> Threats{};
	std::vector<MoveAndPiece> Moves{};
	CastlingConfiguration CastlingConfig{};

private:

	// Functions for move generation
	template <bool side, MoveGen moveGen> void GeneratePseudolegalMoves(MoveList& moves) const;
	template <bool side, MoveGen moveGen> void GenerateKnightMoves(MoveList& moves, const int home) const;
	template <bool side, MoveGen moveGen> void GenerateKingMoves(MoveList& moves, const int home) const;
	template <bool side, MoveGen moveGen> void GeneratePawnMoves(MoveList& moves, const int home) const;
	template <bool side> void GenerateCastlingMoves(MoveList& moves) const;
	template <bool side, int pieceType, MoveGen moveGen> void GenerateSlidingMoves(MoveList& moves, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy) const;

	bool IsSquareAttacked(const bool attackingSide, const uint8_t square, const uint64_t occupancy) const;
	uint64_t CalculateAttackedSquares(const bool attackingSide) const;
};

