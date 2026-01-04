#pragma once
#include "Board.h"
#include "Magics.h"
#include "Move.h"
#include "Settings.h"
#include "Utils.h"

// Magic lookup tables
uint64_t GetBishopAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetRookAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetQueenAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetShortConnectingRay(const uint8_t from, const uint8_t to);
uint64_t GetLongConnectingRay(const uint8_t from, const uint8_t to);
uint64_t GetCuckooHash(const int index);
Move& GetCuckooMove(const int index);

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
	bool IsDrawn(const int level) const;
	bool HasUpcomingRepetition(const int level) const;

	bool IsPseudoLegalMove(const Move& m) const;
	bool IsLegalMove(const Move& m) const;
	bool IsMoveQuiet(const Move& move) const;


	void GenerateNoisyPseudoLegalMoves(MoveList& moves) const;
	void GenerateQuietPseudoLegalMoves(MoveList& moves) const;
	void GenerateAllPseudoLegalMoves(MoveList& moves) const;
	void GenerateAllLegalMoves(MoveList& moves) const;

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
		return States.back().BoardHash;
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
		assert(plies <= static_cast<int>(Moves.size()));
		return Moves[Moves.size() - plies];
	}

	inline const Board& GetPreviousState(const int plies) const {
		assert(plies > 0);
		assert(plies < static_cast<int>(States.size()));
		return States[States.size() - plies - 1];
	}

	inline bool IsPreviousMoveNull() const {
		return Moves.size() != 0 && Moves.back().move == NullMove;
	}

	inline uint64_t GetThreats() const {
		return States.back().Threats;
	}

	inline bool IsSquareThreatened(const uint8_t sq) const {
		return CheckBit(States.back().Threats, sq);
	}

	[[maybe_unused]] inline uint64_t GetMaterialHash() const {
		return States.back().CalculateMaterialHash();
	}

	inline uint64_t GetPawnHash() const {
		return States.back().CalculatePawnHash();
	}

	inline std::pair<uint64_t, uint64_t> GetNonPawnHashes() const {
		return { States.back().WhiteNonPawnHash, States.back().BlackNonPawnHash };
	}

	inline uint64_t ApproximateHashAfterMove(const Move& move) const {
		// Calculate the approximate hash after a move on the current board
		// This is to make prefetching more efficient
		// It doesn't need to be perfect, just good enough, it handles most quiet moves and captures
		uint64_t hash = States.back().BoardHash ^ Zobrist.SideToMove;
		const uint8_t movedPiece = GetPieceAt(move.from);
		const uint8_t capturedPiece = GetPieceAt(move.to);
		hash ^= Zobrist.PieceSquare[movedPiece][move.from];
		hash ^= Zobrist.PieceSquare[movedPiece][move.to];
		if (capturedPiece != Piece::None) hash ^= Zobrist.PieceSquare[capturedPiece][move.to];
		return hash;
	}

	uint64_t AttackersOfSquare(const bool attackingSide, const uint8_t square) const;
	std::pair<uint64_t, uint64_t> GetPinnedBitboard() const;

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
	std::vector<MoveAndPiece> Moves{};
	CastlingConfiguration CastlingConfig{};

private:

	// Functions for move generation
	template <bool side, MoveGen moveGen> void GeneratePseudolegalMoves(MoveList& moves) const;
	template <bool side> void GeneratePawnMovesNoisy(MoveList& moves) const;
	template <bool side> void GeneratePawnMovesQuiet(MoveList& moves) const;
	template <bool side> void GenerateCastlingMoves(MoveList& moves) const;
	template <bool side, uint8_t pieceType, MoveGen moveGen> void GenerateSlidingMoves(MoveList& moves, const uint8_t home, const uint64_t friendlyOccupancy, const uint64_t opponentOccupancy) const;

	bool IsSquareAttacked(const bool attackingSide, const uint8_t square, const uint64_t occupancy) const;
	uint64_t CalculateAttackedSquares(const bool attackingSide) const;
};

