#include "Search.h"

Search::Search() {
	for (int i = 1; i < 32; i++) {
		for (int j = 1; j < 32; j++) {
			LMRTable[i][j] = static_cast<int>(0.25 * log(i) * log(j) + 0.7);
		}
	}
	Reset();
}

void Search::Reset() {
	Depth = 0;
	ResetStatistics();
	InitOpeningBook();
}

void Search::ResetStatistics() {
	Depth = 0;
	Statistics.SelDepth = 0;
	Statistics.Nodes = 0;
	Statistics.QuiescenceNodes = 0;
	Statistics.Evaluations = 0;
	Statistics.BetaCutoffs = 0;
	Statistics.FirstMoveBetaCutoffs = 0;
	Statistics.TranspositionQueries = 0;
	Statistics.TranspositionHits = 0;
}

// Perft methods ----------------------------------------------------------------------------------

const void Search::Perft(Board& board, const int depth, const PerftType type) {
	Board b = board.Copy();
	const bool startingPosition = b.Hash() == 0x463b96181691fc9c;
	const uint64_t startingPerfts[] = { 1, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860 };

	auto t0 = Clock::now();
	const uint64_t r = PerftRecursive(b, depth, depth, type);
	auto t1 = Clock::now();

	const float seconds = static_cast<float>((t1 - t0).count() / 1e9);
	const float speed = r / seconds / 1000000;
	if ((type == PerftType::Normal) || (type == PerftType::PerftDiv)) {
		cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps | No bulk counting" << endl;
		if (startingPosition && (depth <= 6) && (startingPerfts[depth] != r)) cout << "Uh-oh. (expected: " << startingPerfts[depth] << ")" << endl;
	}
	else cout << r << endl;
}

const uint64_t Search::PerftRecursive(Board& board, const int depth, const int originalDepth, const PerftType type) {
	std::vector<Move> moves;
	board.GenerateMoves(moves, MoveGen::All, Legality::Pseudolegal);
	if ((type == PerftType::PerftDiv) && (originalDepth == depth)) cout << "Legal moves (" << moves.size() << "): " << endl;
	uint64_t count = 0;
	for (const Move& m : moves) {
		if (!board.IsLegalMove(m)) continue;
		uint64_t r;
		if (depth == 1) {
			r = 1;
			count += r;
		}
		else {
			Board b = board.Copy();
			b.Push(m);
			r = PerftRecursive(b, depth - 1, originalDepth, type);
			count += r;
		}
		if ((originalDepth == depth) && (type == PerftType::PerftDiv)) cout << " - " << m.ToString() << " : " << r << endl;
	}
	return count;
}

// Time management --------------------------------------------------------------------------------

const SearchConstraints Search::CalculateConstraints(const SearchParams params, const bool turn) {
	SearchConstraints constraints = SearchConstraints();
	constraints.MaxNodes = -1;
	constraints.MaxDepth = -1;
	constraints.SearchTimeMin = -1;
	constraints.SearchTimeMax = -1;

	// Nodes && depth && movetime 
	if (params.nodes != 0) constraints.MaxNodes = params.nodes;
	if (params.depth != 0) constraints.MaxDepth = params.depth;
	if (params.movetime != 0) {
		constraints.SearchTimeMin = params.movetime;
		constraints.SearchTimeMax = params.movetime;
	}
	if ((constraints.MaxDepth != -1) || (constraints.MaxNodes != -1)) return constraints;
	if ((constraints.SearchTimeMin != -1) || (constraints.SearchTimeMax != -1)) return constraints;

	// Handle wtime, btime, winc, binc
	const int myTime = turn ? params.wtime : params.btime;
	const int myInc = turn ? params.winc : params.binc;
	if (myTime != 0) {

		int maxTime;
		int minTime;
		if (params.movestogo > 0) {
			// Repeating time control
			maxTime = static_cast<int>((myTime / params.movestogo * 2.2));
			minTime = static_cast<int>((myTime / params.movestogo * 0.5));
			maxTime = std::min(maxTime, static_cast<int>(myTime * 0.8));
		}
		else {
			// Sudden death 
			maxTime = static_cast<int>(myTime * 0.333);
			minTime = static_cast<int>((myTime + myInc * 10.0) * 0.0175);
		}

		constraints.SearchTimeMax = maxTime;
		constraints.SearchTimeMin = minTime;
		if (constraints.SearchTimeMin > constraints.SearchTimeMax) constraints.SearchTimeMin = constraints.SearchTimeMax;
		return constraints;
	}

	// Default: go infinite
	return constraints;
}

const inline bool Search::ShouldAbort() {
	if (Aborting) return true;
	if ((Constraints.MaxNodes != -1) && (Statistics.Nodes >= Constraints.MaxNodes) && (Depth > 1)) {
		return true;
	}
	if ((Statistics.Nodes % 1024 == 0) && (Constraints.SearchTimeMax != -1) && (Depth > 1)) {
		auto now = Clock::now();
		const int elapsedMs = static_cast<int>((now - StartSearchTime).count() / 1e6);
		if (elapsedMs >= Constraints.SearchTimeMax) {
			return true;
		}
	}
	return false;
}

// Negamax search routine and handling ------------------------------------------------------------

Results Search::SearchMoves(Board &board, const SearchParams params, const EngineSettings settings, const bool display) {

	StartSearchTime = Clock::now();
	int elapsedMs = 0;
	Depth = 0;
	ResetStatistics();
	Aborting = false;
	Depth = 0;
	bool finished = false;

	Constraints = CalculateConstraints(params, board.Turn);

	if (settings.ExtendedOutput) {
		cout << "info string Renegade searching for time: (" << Constraints.SearchTimeMin << ".." << Constraints.SearchTimeMax
			<< ") depth: " << Constraints.MaxDepth << " nodes: " << Constraints.MaxNodes << endl;
	}

	// Check for book moves
	if (settings.UseBook) {
		std::string bookMove = GetBookMove(board.Hash());
		if (bookMove != "") {
			Results e;
			if (display) cout << "bestmove " << bookMove << endl;
			return e;
		}
	}

	// Iterative deepening
	Results e = Results();
	int result = NoEval;
	while (!finished) {
		FollowingPV = true;
		Heuristics.ResetPvTable();
		Depth += 1;
		Statistics.SelDepth = Depth;

		// Obtain score
		if (Depth < 5) {
			// Regular negamax for shallow depths
			result = SearchRecursive(board, Depth, 0, NegativeInfinity, PositiveInfinity, true);
		}
		else {
			// Aspiration windows
			int windowSize = 25;

			while (true) {
				if (Aborting) break;
				int alpha, beta;
				if (windowSize < 500) {
					alpha = std::max(result - windowSize, NegativeInfinity);
					beta = std::min(result + windowSize, PositiveInfinity);
				}
				else {
					alpha = NegativeInfinity;
					beta = PositiveInfinity;
				}

				//if (!settings.UciOutput) cout << "[" << alpha << ".." << beta << "] ";

				result = SearchRecursive(board, Depth, 0, alpha, beta, true);

				if (result <= alpha) {
					alpha = std::max(result - windowSize, NegativeInfinity);
					beta = (alpha + beta) / 2;
				}
				else if (result >= beta) {
					beta = std::min(result + windowSize, PositiveInfinity);
				}
				else {
					// Success!
					break;
				}

				windowSize *= 2;

			}

		}
		
		// Reduce allocated time if found mate (impatience in action)
		if (IsMateScore(result)) {
			if (Constraints.SearchTimeMin != -1) Constraints.SearchTimeMin = static_cast<int>(Constraints.SearchTimeMin * 0.8);
			if (Constraints.SearchTimeMax != -1) Constraints.SearchTimeMax = static_cast<int>(Constraints.SearchTimeMax * 0.8);
		}

		// Check search limits
		auto currentTime = Clock::now();
		elapsedMs = static_cast<int>((currentTime - StartSearchTime).count() / 1e6);
		if ((elapsedMs >= Constraints.SearchTimeMin) && (Constraints.SearchTimeMin != -1)) finished = true;
		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if ((Depth >= 31)) finished = true;
		if (Aborting) {
			e.stats = Statistics;
			e.time = elapsedMs;
			e.nps = static_cast<int>(Statistics.Nodes * 1e9 / (currentTime - StartSearchTime).count());
			e.hashfull = Heuristics.GetHashfull();
			if (display) PrintInfo(e, settings);
			break;
		}

		// Send info
		e.score = result;
		e.depth = Depth;
		e.stats = Statistics;
		e.time = elapsedMs;
		e.nps = static_cast<int>(Statistics.Nodes * 1e9 / (currentTime - StartSearchTime).count());
		e.hashfull = Heuristics.GetHashfull();

		// Checking PV validity
		e.pv.clear();
		std::vector<Move> returnedPVs = Heuristics.GeneratePvLine();
		Board b = board.Copy();
		for (const Move& m : returnedPVs) {
			std::vector<Move> legalMoves;
			b.GenerateMoves(legalMoves, MoveGen::All, Legality::Legal);
			if (std::find(legalMoves.begin(), legalMoves.end(), m) != legalMoves.end()) {
				e.pv.push_back(m);
				b.Push(m);
			}
			else break;
		}

		Heuristics.SetPvLine(e.pv);
		if (display) PrintInfo(e, settings);
	}
	if (display) PrintBestmove(e.BestMove());

	std::fill(std::begin(MoveStack), std::end(MoveStack), EmptyMove);
	Heuristics.ClearKillerAndCounterMoves();
	Heuristics.ResetPvTable();
	Heuristics.ClearPvLine();
	Heuristics.AgeHistory();
	Aborting = true;
	return e;
}

// Recursively called during the negamax search
int Search::SearchRecursive(Board &board, int depth, const int level, int alpha, int beta, const bool canNullMove) {

	// Check search limits
	Aborting = ShouldAbort();
	if (Aborting) return NoEval;
	if (level >= 63) return board.IsInCheck() ? 0 : EvaluateBoard(board, level);

	const bool rootNode = (level == 0);
	const bool pvNode = rootNode || (beta - alpha > 1);
	Statistics.AlphaBetaCalls += 1;

	// Mate distance pruning
	if (!rootNode) {
		alpha = std::max(alpha, -MateEval + level);
		beta = std::min(beta, MateEval - level - 1);
		if (alpha >= beta) return alpha;
	}

	int staticEval = NoEval;

	// Check extensions
	const bool inCheck = board.IsInCheck();
	if (inCheck) depth += 1;

	// Check for draws
	if (board.IsDraw(rootNode)) return 0;

	// Return result for terminal nodes
	if (depth <= 0) {
		return SearchQuiescence(board, level, alpha, beta, true);
	}

	// Calculate hash and probe transposition table
	const uint64_t hash = board.Hash();
	TranspositionEntry entry;
	const bool found = Heuristics.RetrieveTranspositionEntry(hash, depth, entry, level);
	Move transpositionMove;
	Statistics.TranspositionQueries += 1;
	if (found) {
		if ((entry.depth >= depth) && !pvNode) {
			// The branch was already analysed to the same or greater depth, so we can return the result
			int score = NoEval;
			bool usable = true;
			if (entry.scoreType == ScoreType::Exact) score = entry.score;
			else if ((entry.scoreType == ScoreType::UpperBound) && (entry.score <= alpha)) score = alpha;
			else if ((entry.scoreType == ScoreType::LowerBound) && (entry.score >= beta)) score = beta;
			else usable = false;
			if (usable) {
				if ((score > alpha) && (score < beta)) {
					Heuristics.UpdatePvTable(Move(entry.moveFrom, entry.moveTo, entry.moveFlag), level, depth == 1);
				}
				return score;
			}
		}
		else {
			// The branch was not analysed sufficiently, but we can use it for move ordering purposes
			transpositionMove = Move(entry.moveFrom, entry.moveTo, entry.moveFlag);
			staticEval = entry.score;
		}
		Statistics.TranspositionHits += 1;
	}

	// Internal iterative deepening
	if (pvNode && (depth > 3) && transpositionMove.IsEmpty()) {
		SearchRecursive(board, depth - 2, level + 1, -beta, -alpha, true);
		transpositionMove = Heuristics.PvTable[level+1][level + 1];
	}

	// Futility pruning (+37 elo)
	const int futilityMargins[] = { 0, 100, 200, 300, 400, 500 };
	bool futilityPrunable = false;
	if ((depth <= 5) && !inCheck && !pvNode) {
		if (staticEval == NoEval) staticEval = EvaluateBoard(board, level);
		if ((staticEval + futilityMargins[depth] < alpha)) futilityPrunable = true;
	}

	// Reverse futility pruning (+128 elo)
	const int rfpMargin[] = { 0, 70, 150, 240, 340, 450, 580, 720 };
	if ((depth <= 7) && !inCheck && !pvNode) {
		if (staticEval == NoEval) staticEval = EvaluateBoard(board, level);
		if (staticEval - rfpMargin[depth] > beta) return staticEval;
	}

	// Null-move pruning (+33 elo)
	const int friendlyPieces = Popcount(board.GetOccupancy(TurnToPieceColor(board.Turn)));
	const int friendlyPawns = board.Turn == Turn::White ? Popcount(board.WhitePawnBits) : Popcount(board.BlackPawnBits);
	if ((depth >= 3) && !inCheck && canNullMove && ((friendlyPieces - friendlyPawns) > 2) && !pvNode) {
		if (staticEval == NoEval) staticEval = EvaluateBoard(board, level);
		int nmpReduction = 3 + depth / 4 + std::min((staticEval - beta) / 200, 3); // Thanks Discord
		nmpReduction = std::min(nmpReduction, depth - 1);
		if ((staticEval >= beta) && (nmpReduction > 0)) {
			Boards[level] = board;
			Boards[level].Push(NullMove);
			const int nullMoveEval = -SearchRecursive(Boards[level], depth - 1 - nmpReduction, level + 1, -beta, -beta + 1, false);
			if ((nullMoveEval >= beta) && !IsMateScore(nullMoveEval)) return beta;
		}
	}

	// Razoring (neutral)
	/*
	const int razoringMargin[] = { 0, 300, 750 };
	if ((depth < 2) && !inCheck && !pvNode) {
		if (staticEval == NoEval) staticEval = EvaluateBoard(board, level);
		if (staticEval + razoringMargin[depth] < alpha) {
			int e = SearchQuiescence(board, level, alpha, beta, true);
			if (e < alpha) return alpha;
		}
	}*/

	// Initalize variables and generate moves
	MoveList.clear();
	board.GenerateMoves(MoveList, MoveGen::All, Legality::Pseudolegal);

	// Move ordering
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	bool foundPvMove = false;
	for (const Move& m : MoveList) {
		const bool losingCapture = board.IsMoveQuiet(m) ? false : !StaticExchangeEval(board, m, 0);
		const int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, FollowingPV, transpositionMove, 
			(level > 0) ? MoveStack[level-1LL] : EmptyMove, losingCapture);
		if (FollowingPV && Heuristics.IsPvMove(m, level)) foundPvMove = true;
		MoveOrder[level].push_back({ m, orderScore });
	}
	FollowingPV = foundPvMove;
	std::sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Iterate through legal moves
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	Move bestMove;
	for (const auto& [m, order] : MoveOrder[level]) {
		if (!board.IsLegalMove(m)) continue;
		legalMoveCount += 1;
		const bool isQuiet = board.IsMoveQuiet(m);

		// Performing futility pruning
		if (isQuiet && futilityPrunable && !IsMateScore(alpha) && !IsMateScore(beta)) continue;

		// Main search SEE pruning (+20 elo)
		const int seeQuietMargin[] = { 0, -50, -100, -150, -200, -250, -300, -350 };
		const int seeNoisyMargin[] = { 0, -100, -200, -300, -400, -500, -600, -700 };
		if (!rootNode && !pvNode && (depth <= 5) && !IsLosingMateScore(alpha)) {
			if (!StaticExchangeEval(board, m, isQuiet ? seeQuietMargin[depth] : seeNoisyMargin[depth])) continue;
		}

		// Push move
		Boards[level] = board;
		Board& b = Boards[level];
		b.Push(m);
		MoveStack[level] = m;
		Statistics.Nodes += 1;
		int score = NoEval;
		const bool givingCheck = b.IsInCheck();

		if (legalMoveCount == 1) {
			score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
		}
		else {
			int reduction = 0;

			// Late-move pruning (slightly losing)
			/*const int lmpCount[] = { 0, 10, 14, 18, 22 };
			if ((depth < 5) && !pvNode && !inCheck && isQuiet) {
				if (legalMoveCount > lmpCount[depth]) continue;
			}*/

			// Late-move reductions (+119 elo)
			if ((legalMoveCount >= (pvNode ? 6 : 4)) && isQuiet && !inCheck && !givingCheck && (depth >= 3)) {
				reduction = LMRTable[std::min(depth, 31)][std::min(legalMoveCount, 31)];
				// Idea: increase reduction for bad history moves
				// if (!pvNode && (Heuristics.HistoryTables[board.Turn][m.from][m.to] < -3000)) reduction += 1;
			}

			// Principal variation search
			score = -SearchRecursive(b, depth - 1 - reduction, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (reduction > 0)) score = -SearchRecursive(b, depth - 1, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (score < beta)) score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
		}

		// Checking alpha-beta bounds
		if (score >= beta) {
			if (isQuiet) {
				Heuristics.AddKillerMove(m, level);
				if (level > 0) Heuristics.AddCountermove(MoveStack[level - 1], m);
				Heuristics.IncrementHistory(board.Turn, m.from, m.to, depth);
			}
			Heuristics.AddTranspositionEntry(hash, depth, beta, ScoreType::LowerBound, m, level);
			Statistics.BetaCutoffs += 1;
			if (legalMoveCount == 1) Statistics.FirstMoveBetaCutoffs += 1;
			/*
			for (int i = 1; i < pseudoLegalCount; i++) {
				if (LegalAndQuiet[i]) Heuristics.DecrementHistory(board.Turn, m.from, m.to, depth);
			}*/

			return beta;
		}
		if (score > alpha) {
			scoreType = ScoreType::Exact;
			bestMove = m;
			alpha = score;
			Heuristics.UpdatePvTable(m, level, depth == 1);
		}

		// Should only be reduced if there's a beta-cutoff, but that loses strength...
		if (isQuiet) Heuristics.DecrementHistory(board.Turn, m.from, m.to, depth);

	}

	// There was no legal move --> game over 
	if (legalMoveCount == 0) {
		const int e = inCheck ? LosingMateScore(level) : 0;
		Heuristics.AddTranspositionEntry(hash, depth, e, ScoreType::Exact, EmptyMove, level);
		return e;
	}

	// Return alpha otherwise
	const int e = alpha;
	Heuristics.AddTranspositionEntry(hash, depth, e, scoreType, bestMove, level);
	return e;
}

// Quiescence search: for noisy moves only (captures, queen promotions, pawn pushes threatening promotion)
int Search::SearchQuiescence(Board &board, const int level, int alpha, int beta, const bool rootNode) {

	// Check search limits
	Aborting = ShouldAbort();
	if (Aborting) return NoEval;

	// Generate noisy moves
	MoveList.clear();
	board.GenerateMoves(MoveList, MoveGen::Noisy, Legality::Pseudolegal);
	if (!rootNode) {
		Statistics.AlphaBetaCalls += 1;
		Statistics.QSeachAlphaBetaCalls += 1;
	}

	// Update alpha-beta bounds, return alpha if no captures left
	const int staticEval = StaticEvaluation(board, level, true);
	if (staticEval >= beta) return beta;
	//if (staticEval < alpha - 1000) return alpha; // Delta pruning (loses strength)
	if (staticEval > alpha) alpha = staticEval;
	if (level >= 63) return alpha;

	// Order noisy moves
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	for (const Move& m : MoveList) {
		const int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, false, EmptyMove, EmptyMove, false);
		MoveOrder[level].push_back({ m, orderScore });
	}
	std::sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Search recursively
	for (const auto& [m, order] : MoveOrder[level]) {
		if (!board.IsLegalMove(m)) continue;
		if (!StaticExchangeEval(board, m, 0)) continue; // Quiescence search SEE pruning (+39 elo)
		Statistics.Nodes += 1;
		Statistics.QuiescenceNodes += 1;

		Boards[level] = board;
		Board& b = Boards[level];
		b.Push(m);
		const int childEval = -SearchQuiescence(b, level + 1, -beta, -alpha, false);
		if (childEval >= beta) return beta;
		if (childEval > alpha) alpha = childEval;
	}
	return alpha;
}

const int Search::StaticEvaluation(const Board &board, const int level, const bool checkDraws) {
	Statistics.Evaluations += 1;
	if (level > Statistics.SelDepth) Statistics.SelDepth = level;
	if (checkDraws) {
		if (board.IsDraw(level == 0)) return 0;
	}
	return EvaluateBoard(board, level);
}

// Static exchange evaluation (SEE) ---------------------------------------------------------------

const int seeValues[] = { 0, 100, 300, 300, 500, 1000, 999999 };

const bool Search::StaticExchangeEval(const Board& board, const Move& move, const int threshold) {
	// This is more or less the standard way of doing this
	// The implementation follows Ethereal's method

	// Get the initial piece
	uint8_t victim = TypeOfPiece(board.GetPieceAt(move.from));
	if (move.IsPromotion()) victim = move.GetPromotionPieceType();

	// Get estimated move value
	int estimatedMoveValue = seeValues[TypeOfPiece(board.GetPieceAt(move.to))];
	if (move.IsPromotion()) estimatedMoveValue += seeValues[move.GetPromotionPieceType()] - seeValues[PieceType::Pawn];
	else if (move.flag == MoveFlag::EnPassantPerformed) estimatedMoveValue = seeValues[PieceType::Pawn];

	// Handle trivial cases (losing the piece for nothing still above / having initial gain below threshold)
	int score = -threshold;
	score += estimatedMoveValue;
	if (score < 0) return false;
	score -= seeValues[victim];
	if (score >= 0) return true;

	// Lookups (should be optimized) 
	const uint64_t whitePieces = board.GetOccupancy(PieceColor::White);
	const uint64_t blackPieces = board.GetOccupancy(PieceColor::Black);
	const uint64_t parallels = board.WhiteRookBits | board.BlackRookBits | board.WhiteQueenBits | board.BlackQueenBits; 
	const uint64_t diagonals = board.WhiteBishopBits | board.BlackBishopBits | board.WhiteQueenBits | board.BlackQueenBits;
	uint64_t occupancy = whitePieces | blackPieces;
	SetBitFalse(occupancy, move.from);
	SetBitTrue(occupancy, move.to);
	bool turn = board.Turn;
	if (move.flag == MoveFlag::EnPassantPerformed) {
		if (turn == Turn::White) SetBitFalse(occupancy, move.to - 8);
		else SetBitFalse(occupancy, move.to + 8);
	};
	turn = !turn;
	uint64_t attackers = board.GetAttackersOfSquare(move.to) & occupancy;

	// Pseudo-generating steps
	while (true) {

		uint64_t currentAttackers = attackers & ((turn == Turn::White) ? whitePieces : blackPieces);
		if (!currentAttackers) break;

		// Retrieve the location of the least valuable attacking piece type
		int sq = -1;
		if (turn == Turn::White) {
			if (currentAttackers & board.WhitePawnBits) sq = LsbSquare(currentAttackers & board.WhitePawnBits);
			else if (currentAttackers & board.WhiteKnightBits) sq = LsbSquare(currentAttackers & board.WhiteKnightBits);
			else if (currentAttackers & board.WhiteBishopBits) sq = LsbSquare(currentAttackers & board.WhiteBishopBits);
			else if (currentAttackers & board.WhiteRookBits) sq = LsbSquare(currentAttackers & board.WhiteRookBits);
			else if (currentAttackers & board.WhiteQueenBits) sq = LsbSquare(currentAttackers & board.WhiteQueenBits);
			else if (currentAttackers & board.WhiteKingBits) sq = LsbSquare(currentAttackers & board.WhiteKingBits);
		}
		else {
			if (currentAttackers & board.BlackPawnBits) sq = LsbSquare(currentAttackers & board.BlackPawnBits);
			else if (currentAttackers & board.BlackKnightBits) sq = LsbSquare(currentAttackers & board.BlackKnightBits);
			else if (currentAttackers & board.BlackBishopBits) sq = LsbSquare(currentAttackers & board.BlackBishopBits);
			else if (currentAttackers & board.BlackRookBits) sq = LsbSquare(currentAttackers & board.BlackRookBits);
			else if (currentAttackers & board.BlackQueenBits) sq = LsbSquare(currentAttackers & board.BlackQueenBits);
			else if (currentAttackers & board.BlackKingBits) sq = LsbSquare(currentAttackers & board.BlackKingBits);
		}
		//assert(sq != -1);

		// Update fields
		victim = TypeOfPiece(board.GetPieceAt(sq));
		SetBitFalse(occupancy, sq);

		// Update potentially uncovered sliding pieces
		if ((victim == PieceType::Pawn) || (victim == PieceType::Bishop) || (victim == PieceType::Queen)) {
			attackers |= GetBishopAttacks(move.to, occupancy) & diagonals;
		}
		if ((victim == PieceType::Rook) || (victim == PieceType::Queen)) {
			attackers |= GetRookAttacks(move.to, occupancy) & parallels;
		}

		attackers &= occupancy;
		turn = !turn;

		// Break conditions
		score = -score - seeValues[victim] - 1;
		if (score >= 0) {
			const uint64_t upcomingOccupnacy = (turn == Turn::White) ? whitePieces : blackPieces;
			if ((victim == PieceType::King) && (currentAttackers & upcomingOccupnacy)) {
				turn = !turn;
			}
			break;
		}
	}

	// If after the exchange it's our opponent's turn, that means we won
	return turn != board.Turn;
}

// Opening book -----------------------------------------------------------------------------------

void Search::InitOpeningBook() {
	std::ifstream ifs("book.bin", std::ios::in | std::ios::binary);

	if (!ifs) return;

	uint64_t buffer[2];

	while (ifs.read(reinterpret_cast<char*>(&buffer), 16)) {
		BookEntry entry;
		int a = _byteswap_ushort(0x0000FFFF & buffer[1]);
		int b = _byteswap_ushort((0xFFFF0000 & buffer[1]) >> 16);
		entry.hash = _byteswap_uint64(buffer[0]);
		entry.to = (0b000000000111111 & a) >> 0;
		entry.from = (0b000111111000000 & a) >> 6;
		entry.promotion = (0b111000000000000 & a) >> 12;
		entry.weight = b;
		entry.learn = 0;
		BookEntries.push_back(entry);
	}
	ifs.close();

}

const std::string Search::GetBookMove(const uint64_t hash) {
	std::vector<std::string> matches;
	std::vector<int> weights;
	int totalWeights = 0;
	for (const BookEntry& e : BookEntries) {
		if (e.hash != hash) continue;

		Move m = Move(e.from, e.to);
		switch (e.promotion) {
		case 1: { m.SetFlag(MoveFlag::PromotionToKnight); break; }
		case 2: { m.SetFlag(MoveFlag::PromotionToBishop); break; }
		case 3: { m.SetFlag(MoveFlag::PromotionToRook); break; }
		case 4: { m.SetFlag(MoveFlag::PromotionToQueen); break; }
		}
		matches.push_back(m.ToString());
		int w = std::max(e.weight, 1);
		weights.push_back(w);
		totalWeights += w;
	}

	if (matches.size() == 0) return "";

	int randomInt = std::rand() % totalWeights + 1;
	int sum = 0;
	for (int i = 0; i < matches.size(); i++) {
		sum += weights[i];
		if (sum >= randomInt) return matches[i];
	}
	return "";
}

const BookEntry Search::GetBookEntry(int item) {
	return BookEntries[item];
}

const int Search::GetBookSize() {
	return static_cast<int>(BookEntries.size());
}


// Communicating the search results ---------------------------------------------------------------

const void Search::PrintInfo(const Results& e, const EngineSettings& settings) {
	if (!settings.UciOutput) {
		PrintPretty(e, settings);
		return;
	}

	std::string score;
	if (IsMateScore(e.score)) {
		int movesToMate = (MateEval - abs(e.score) + 1) / 2;
		if (e.score > 0) score = "mate " + std::to_string(movesToMate);
		else score = "mate -" + std::to_string(movesToMate);
	}
	else {
		score = "cp " + std::to_string(e.score);
	}

	std::string extended;
	if (settings.ExtendedOutput) {
		extended += " evals " + std::to_string(e.stats.Evaluations);
		extended += " qnodes " + std::to_string(e.stats.QuiescenceNodes);
		// extended += " betacutoffs " + std::to_string(e.stats.BetaCutoffs)
		// extended += " fmbc " + std::to_string(e.stats.FirstMoveBetaCutoffs);
		if (e.stats.BetaCutoffs != 0) extended += " fmbcrate " + std::to_string(static_cast<int>(e.stats.FirstMoveBetaCutoffs * 1000 / e.stats.BetaCutoffs));
		extended += " tthits " + std::to_string(e.stats.TranspositionHits);
		if (e.stats.TranspositionQueries != 0) extended += " ttrate " + std::to_string(static_cast<int>(e.stats.TranspositionHits * 1000 / e.stats.TranspositionQueries));
	}

	/*
	cout << "info depth " << e.depth << " seldepth " << e.stats.SelDepth << " score " << score << " nodes " << e.stats.Nodes << " nps " << e.nps
		<< " time " << e.time << " hashfull " << e.hashfull << extended << " pv";*/

	std::string pvString;
	for (const Move& move : e.pv)
		pvString += " " + move.ToString();

	std::string output = std::format("info depth {} seldepth {} score {} nodes {} nps {} time {} hashfull {} pv{}",
		e.depth, e.stats.SelDepth, score, e.stats.Nodes, e.nps, e.time, e.hashfull, pvString);



	cout << output << endl;
	//if (e.time < 50) cout << '\n';
	//else cout << endl;
}

const void Search::PrintPretty(const Results& e, const EngineSettings& settings) {
	const std::string green = settings.Colorful ? "\x1b[92m" : "";
	const std::string blue = settings.Colorful ? "\x1b[96m": "";
	const std::string red = settings.Colorful ? "\x1b[91m": "";
	const std::string white = settings.Colorful ? "\x1b[0m": "";
	const std::string gray = settings.Colorful ? "\x1b[90m": "";
	const std::string yellow = settings.Colorful ? "\x1b[93m" : "";

	std::string output1 = std::format("{} {:2d}/{:2d}", 
		white, e.depth, e.stats.SelDepth);

	std::string nodeString;
	if (e.stats.Nodes < 1e9) nodeString = std::to_string(e.stats.Nodes);
	else nodeString = std::format("{:8.3f}", e.stats.Nodes / 1e9) + "b";

	std::string timeString;
	if (e.time < 60000) timeString = std::to_string(e.time) + "ms";
	else {
		int seconds = e.time / 1000;
		int minutes = seconds / 60;
		seconds = seconds - minutes * 60;
		timeString = std::format("{}m{:02d}s", minutes, seconds);
	}

	std::string hashString;
	if (e.hashfull != 1000) hashString = std::format("{:4.1f}", e.hashfull / 10.0);
	else hashString = "100";

	std::string output3 = std::format("{}  {:>9}  {:>7}  {:4d}knps  h={:>4}% {} -> ",
		gray, nodeString, timeString, e.nps / 1000, hashString, white);

	const int neutralMargin = 50;
	std::string scoreColor = blue;
	if (!IsMateScore(e.score)) {
		if (e.score > neutralMargin) scoreColor = green;
		else if (e.score < -neutralMargin) scoreColor = red;
	}
	else {
		scoreColor = yellow;
	}

	std::string output2;
	if (!IsMateScore(e.score))
		output2 = std::format("{} {:+5.2f}  {}", scoreColor, e.score / 100.0, white);
	else {
		std::string output2mate;
		int movesToMate = (MateEval - abs(e.score) + 1) / 2;
		if (e.score > 0) output2mate = "#+" + std::to_string(movesToMate);
		else output2mate = "#-" + std::to_string(movesToMate);
		output2 = std::format("{} {:5}  {}", scoreColor, output2mate, white);
	}

	std::string pvOutput;
	for (int i = 0; i < 5; i++) {
		if (i >= e.pv.size()) break;
		pvOutput += e.pv.at(i).ToString() + " ";
	}
	pvOutput.pop_back();
	if (e.pv.size() > 5) pvOutput += " (+" + std::to_string(e.pv.size() - 5) + ")";
	if (e.pv.size() != 0) pvOutput = white + "[" + pvOutput + white + "]";

	std::string output = output1 + output3 + output2 + pvOutput + white;

	cout << output << endl;
}

const void Search::PrintBestmove(const Move& move) {
	cout << "bestmove " << move.ToString() << endl;
}