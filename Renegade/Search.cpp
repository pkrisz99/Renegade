#include "Search.h"

Search::Search() {
	constexpr double lmrMultiplier = 0.4;
	constexpr double lmrBase = 0.7;
	for (int i = 1; i < 32; i++) {
		for (int j = 1; j < 32; j++) {
			LMRTable[i][j] = static_cast<int>(lmrMultiplier * log(i) * log(j) + lmrBase);
		}
	}
	ResetStatistics();
	Accumulators = std::make_unique<std::array<AccumulatorRepresentation, MaxDepth + 1>>();
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
	Statistics.AlphaBetaCalls = 0;
}


void Search::ResetState(const bool clearTT) {
	Heuristics.ClearHistory();
	Heuristics.ClearKillerAndCounterMoves();
	Heuristics.ResetPvTable();
	if (clearTT) {
		Heuristics.ClearTranspositionTable();
		Age = 0;
	}
}

// Perft methods ----------------------------------------------------------------------------------

void Search::Perft(Board& board, const int depth, const PerftType type) const {
	Board b = board;
	const bool startingPosition = b.Hash() == 0x463b96181691fc9c;
	const uint64_t startingPerfts[] = { 1, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860 };

	const auto startTime = Clock::now();
	const uint64_t r = PerftRecursive(b, depth, depth, type);
	const auto endTime = Clock::now();

	const float seconds = static_cast<float>((endTime - startTime).count() / 1e9);
	const float speed = r / seconds / 1000000;
	cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps | No bulk counting" << endl;
	if (startingPosition && (depth <= 6) && (startingPerfts[depth] != r)) cout << "Uh-oh. (expected: " << startingPerfts[depth] << ")" << endl;
}

uint64_t Search::PerftRecursive(Board& board, const int depth, const int originalDepth, const PerftType type) const {
	MoveList moves{};
	board.GenerateMoves(moves, MoveGen::All, Legality::Pseudolegal);
	if ((type == PerftType::PerftDiv) && (originalDepth == depth)) cout << "Legal moves (" << moves.size() << "): " << endl;
	uint64_t count = 0;
	for (const auto& m : moves) {
		if (!board.IsLegalMove(m.move)) continue;
		uint64_t r;
		if (depth == 1) {
			r = 1;
			count += r;
		}
		else {
			Board b = board;
			b.Push(m.move);
			r = PerftRecursive(b, depth - 1, originalDepth, type);
			count += r;
		}
		if ((originalDepth == depth) && (type == PerftType::PerftDiv)) cout << " - " << m.move.ToString() << " : " << r << endl;
	}
	return count;
}

// Time management --------------------------------------------------------------------------------

SearchConstraints Search::CalculateConstraints(const SearchParams params, const bool turn) const {
	SearchConstraints constraints;

	// Handle nodes, depth, movetime 
	if (params.nodes != 0) constraints.MaxNodes = params.nodes;
	if (params.softnodes != 0) constraints.SoftNodes = params.softnodes;
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
			maxTime = static_cast<int>(myTime / params.movestogo * 2.5);
			minTime = static_cast<int>(myTime / params.movestogo * 0.5);
			maxTime = std::min(maxTime, static_cast<int>(myTime * 0.8));
		}
		else {
			// Time control with increment
			maxTime = static_cast<int>(myTime * 0.25);
			minTime = static_cast<int>(myTime * 0.019 + myInc * 0.7);
		}

		constraints.SearchTimeMax = maxTime;
		constraints.SearchTimeMin = minTime;
		if (constraints.SearchTimeMin > constraints.SearchTimeMax) constraints.SearchTimeMin = constraints.SearchTimeMax;
		return constraints;
	}

	// Default: go infinite
	return constraints;
}

bool Search::ShouldAbort() {
	if (Aborting.load(std::memory_order_relaxed)) return true;
	if ((Constraints.MaxNodes != -1) && (Statistics.Nodes >= Constraints.MaxNodes) && (Depth > 1)) {
		Aborting.store(true, std::memory_order_relaxed);
		return true;
	}
	if ((Statistics.Nodes % 1024 == 0) && (Constraints.SearchTimeMax != -1) && (Depth > 1)) {
		const auto now = Clock::now();
		const int elapsedMs = static_cast<int>((now - StartSearchTime).count() / 1e6);
		if (elapsedMs >= Constraints.SearchTimeMax) {
			Aborting.store(true, std::memory_order_relaxed);
			return true;
		}
	}
	return false;
}

// Negamax search routine and handling ------------------------------------------------------------

Results Search::SearchMoves(Board board, const SearchParams params, const EngineSettings settings, const bool display) {

	StartSearchTime = Clock::now();
	Aborting.store(false, std::memory_order_relaxed);
	int elapsedMs = 0;
	ResetStatistics();
	bool finished = false;
	if (Age < 32000) Age += 1;

	SetupAccumulators(board);
	std::fill(ExcludedMoves.begin(), ExcludedMoves.end(), EmptyMove);
	std::fill(CutoffCount.begin(), CutoffCount.end(), 0);
	std::fill(DoubleExtensions.begin(), DoubleExtensions.end(), 0);

	// Early exit for only one legal move (no legal moves are handled separately)
	if (!DatagenMode && ((params.wtime != 0) || (params.btime != 0))) {
		MoveList rootLegalMoves{};
		board.GenerateMoves(rootLegalMoves, MoveGen::All, Legality::Legal);
		if (rootLegalMoves.size() == 1) {
			const int eval = Evaluate(board, 0);
			cout << "info string Only one legal move!" << endl;
			cout << "info depth 1 score cp " << ToCentipawns(eval, board.GetPlys()) << " nodes 0" << endl;
			const Move onlyMove = rootLegalMoves[0].move;
			PrintBestmove(onlyMove);
			Aborting.store(true, std::memory_order_relaxed);
			return Results(eval, {onlyMove}, 1, Statistics, 0, 0, 0, board.GetPlys()); // hack: rootLegalMoves is a vector already
		}
	}

	Constraints = CalculateConstraints(params, board.Turn);
	std::memset(&RootNodeCounts, 0, sizeof(RootNodeCounts));

	if (settings.ExtendedOutput) {
		cout << "info string Renegade searching for time: (" << Constraints.SearchTimeMin << ".." << Constraints.SearchTimeMax
			<< ") depth: " << Constraints.MaxDepth << " nodes: " << Constraints.MaxNodes << endl;
	}

	// Iterative deepening
	Results e = Results();
	e.ply = board.GetPlys();
	int result = NoEval;
	while (!finished) {
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
			int windowSize = 20;
			int searchDepth = Depth;

			while (true) {
				if (Aborting.load(std::memory_order_relaxed)) break;
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

				result = SearchRecursive(board, searchDepth, 0, alpha, beta, true);

				if (result <= alpha) {
					alpha = std::max(alpha - windowSize, NegativeInfinity);
					beta = (alpha + beta) / 2;
					searchDepth = Depth;
				}
				else if (result >= beta) {
					beta = std::min(beta + windowSize, PositiveInfinity);
					
					// Obtain PV line
					/*std::vector<Move> tempPvLine = std::vector<Move>();
					Heuristics.GeneratePvLine(tempPvLine);
					if (!Aborting && (tempPvLine.size() >= 1) && (tempPvLine[0] != EmptyMove)) {
						e.pv.clear();
						Heuristics.GeneratePvLine(e.pv);
					}*/
					
					// Reduce depth on fail-high
					// Elo difference : 2.4 + / -5.3,
					// if (!IsMateScore(result) && (searchDepth > 1)) searchDepth -= 1;
				}
				else {
					// Success!
					break;
				}

				windowSize += windowSize / 2;

			}

		}

		// Check search limits
		const auto currentTime = Clock::now();
		elapsedMs = static_cast<int>((currentTime - StartSearchTime).count() / 1e6);

		if (Constraints.SearchTimeMin != -1) {
			const int originalSoftTimeLimit = Constraints.SearchTimeMin;
			const Move& bestMove = Heuristics.PvTable[0][0];
			const double bestMoveFraction = static_cast<double>(RootNodeCounts[bestMove.from][bestMove.to]) / static_cast<double>(Statistics.Nodes);
			const int adjustedSoftTimeLimit = originalSoftTimeLimit * static_cast<float>(Depth >= 10 ? (1.5 - bestMoveFraction) * 1.35: 1.0);
			//cout << RootNodeCounts[bestMove.from][bestMove.to] << " " << bestMoveFraction << " " << adjustedSoftTimeLimit << endl;
			if (elapsedMs >= adjustedSoftTimeLimit) finished = true;
		}


		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if (Depth >= MaxDepth) finished = true;
		if ((Statistics.Nodes >= Constraints.SoftNodes) && (Constraints.SoftNodes != -1)) finished = true;
		if (std::abs(result) == MateEval) finished = true; // Don't search if position is checkmate
		if (Aborting.load(std::memory_order_relaxed)) {
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

		// Obtaining PV line
		e.pv.clear();
		Heuristics.GeneratePvLine(e.pv);
		if (display) PrintInfo(e, settings);
	}
	if (display) PrintBestmove(e.BestMove());

	for (int i = 0; i < MaxDepth; i++) {
		MoveStack[i].move = EmptyMove;
		MoveStack[i].piece = 0;
	}

	Heuristics.ClearKillerAndCounterMoves();
	Heuristics.ResetPvTable();
	Aborting.store(true, std::memory_order_relaxed);
	return e;
}

// Recursively called during the alpha-beta search
int Search::SearchRecursive(Board& board, int depth, const int level, int alpha, int beta, const bool canNullMove) {

	// Check search limits
	const bool aborting = ShouldAbort();
	if (aborting) return NoEval;
	Heuristics.InitPvLength(level);
	if (level >= MaxDepth) return Evaluate(board, level);

	const bool rootNode = (level == 0);
	const bool pvNode = rootNode || (beta - alpha > 1);
	Statistics.AlphaBetaCalls += 1;

	// Mate distance pruning
	if (!rootNode) {
		alpha = std::max(alpha, -MateEval + level);
		beta = std::min(beta, MateEval - level - 1);
		if (alpha >= beta) return alpha;
	}

	// Check for draws
	if (!rootNode && board.IsDraw(false)) return DrawEvaluation();

	// Check extensions
	const bool inCheck = board.IsInCheck();
	if (!rootNode) {
		if (inCheck) depth += 1;
	}

	// Drop into quiescence search at depth 0
	if (depth <= 0) {
		return SearchQuiescence(board, level, alpha, beta);
	}

	const Move excludedMove = ExcludedMoves[level];
	const bool singularSearch = !excludedMove.IsEmpty();

	// Probe the transposition table
	TranspositionEntry ttEntry;
	int ttEval = NoEval;
	Move ttMove = EmptyMove;
	bool found = false;
	const uint64_t hash = board.Hash();

	if (!singularSearch) {
		found = Heuristics.RetrieveTranspositionEntry(hash, ttEntry, level);
		Statistics.TranspositionQueries += 1;
		if (found) {
			if (!pvNode) {
				// The branch was already analysed to the same or greater depth, so we can return the result if the score is alright
				if (ttEntry.IsCutoffPermitted(depth, alpha, beta)) return ttEntry.score;
			}
			ttEval = ttEntry.score;
			ttMove = Move(ttEntry.moveFrom, ttEntry.moveTo, ttEntry.moveFlag);
			Statistics.TranspositionHits += 1;
		}
	}

	const bool singularCandidate = found && !rootNode && !singularSearch && (depth > 8)
		&& (ttEntry.depth >= depth - 3) && (ttEntry.scoreType != ScoreType::UpperBound) && (std::abs(ttEval) < MateThreshold);
	
	// Obtain the evaluation of the position
	int staticEval = NoEval;
	int eval = NoEval;

	if (!singularSearch) {
		staticEval = inCheck ? NoEval : Evaluate(board, level);
		eval = staticEval;

		if ((ttEval != NoEval) && !inCheck) {  // inCheck is cosmetic
			if ((ttEntry.scoreType == ScoreType::Exact)
				|| ((ttEntry.scoreType == ScoreType::LowerBound) && (staticEval < ttEval))
				|| ((ttEntry.scoreType == ScoreType::UpperBound) && (staticEval > ttEval))) eval = ttEval;
		}
		StaticEvalStack[level] = staticEval;
		EvalStack[level] = eval;
	}
	else {
		staticEval = StaticEvalStack[level];
		eval = EvalStack[level];
	}

	const bool improving = (level >= 2) && !inCheck && (StaticEvalStack[level] > StaticEvalStack[level - 2]); // <- add nullmove condition?
	bool futilityPrunable = false;

	// Whole-node pruning techniques
	if (!pvNode && !inCheck && !singularSearch) {

		// Reverse futility pruning (+128 elo)
		const int rfpMarginDefault[] = { 0, 90, 180, 270, 360, 450, 540, 630 };
		if ((depth <= 7) && (std::abs(beta) < MateThreshold)) {
			const int rfpMargin = improving ? rfpMarginDefault[depth - 1] : rfpMarginDefault[depth];
			if (eval - rfpMargin > beta) return eval;
		}

		// Null-move pruning (+33 elo)
		if ((depth >= 3) && canNullMove && (eval >= beta) && board.ShouldNullMovePrune()) {
			int nmpReduction = 3 + depth / 3 + std::min((eval - beta) / 200, 3);
			nmpReduction = std::min(nmpReduction, depth);
			Boards[level] = board;
			Boards[level].Push(NullMove);
			UpdateAccumulators(NullMove, 0, 0, level);
			MoveStack[level].move = NullMove;
			MoveStack[level].piece = Piece::None;
			const int nullMoveEval = -SearchRecursive(Boards[level], depth - nmpReduction, level + 1, -beta, -beta + 1, false);
			if (nullMoveEval >= beta) {
				return IsMateScore(nullMoveEval) ? beta : nullMoveEval;
			}
		}

		// Futility pruning (+37 elo)
		const int futilityMargins[] = { 0, 130, 230, 330, 430, 530 };
		if ((depth <= 5) && (std::abs(beta) < MateThreshold)) {
			futilityPrunable = (eval + futilityMargins[depth] < alpha);
		}
	}

	// Internal iterative reductions
	if ((depth > 4) && ttMove.IsEmpty() && !singularSearch) {
		depth -= 1;
	}

	// Initalize variables and generate moves
	// (if we are in singular search, we already have the moves)
	const uint64_t opponentAttacks = board.CalculateAttackedSquares(!board.Turn); // ^ to do: make a stack variable for it

	if (!singularSearch) {
		// Generating moves and move ordering
		MoveListStack[level].reset();
		board.GenerateMoves(MoveListStack[level], MoveGen::All, Legality::Pseudolegal);
		OrderMoves(board, MoveListStack[level], level, ttMove, opponentAttacks);

		// Resetting killers and fail-high cutoff counts
		if (level + 2 < MaxDepth) {
			Heuristics.KillerMoves[level + 2][0] = EmptyMove;
			Heuristics.KillerMoves[level + 2][1] = EmptyMove;
		}
		if (level + 1 < MaxDepth) CutoffCount[level + 1] = 0;
		if (level > 0) DoubleExtensions[level] = DoubleExtensions[level - 1];
	}
	MovePicker movePicker(MoveListStack[level]);

	// Iterate through legal moves
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	Move bestMove = EmptyMove;
	int bestScore = NegativeInfinity;

	std::vector<Move> quietsTried;
	quietsTried.reserve(30); // <-- ???

	while (movePicker.hasNext()) {
		const auto& [m, order] = movePicker.get();
		if (m == excludedMove) continue;
		if (!board.IsLegalMove(m)) continue;
		legalMoveCount += 1;
		const bool isQuiet = board.IsMoveQuiet(m);
		if (isQuiet) quietsTried.push_back(m);

		// Moves loop pruning techniques
		if (!pvNode && (bestScore > -MateThreshold) && (order < 90000) && !DatagenMode) {

			// Late-move pruning (+9 elo)
			const int lmpCount[] = { 0, 4, 7, 12, 19 };
			if (isQuiet && !inCheck && (depth < 5)) {
				if (legalMoveCount > lmpCount[depth]) break;
			}

			// Performing futility pruning
			if (isQuiet && (alpha < MateThreshold) && futilityPrunable) break;

			// Main search SEE pruning (+20 elo)
			const int seeQuietMargin[] = { 0, -50, -100, -150, -200, -250, -300, -350 };
			const int seeNoisyMargin[] = { 0, -100, -200, -300, -400, -500, -600, -700 };
			if (depth <= 5) {
				const int seeMargin = isQuiet ? seeQuietMargin[depth] : seeNoisyMargin[depth];
				if (!StaticExchangeEval(board, m, seeMargin)) continue;
			}
		}

		// Singular extensions
		int extension = 0;
		if (singularCandidate && (m == ttMove)) {
			const int singularMargin = depth * 2;
			const int singularBeta = std::max(ttEval - singularMargin, -MateEval);
			const int singularDepth = (depth - 1) / 2;
			ExcludedMoves[level] = m;
			const int singularScore = SearchRecursive(board, singularDepth, level, singularBeta - 1, singularBeta, false);
			ExcludedMoves[level] = EmptyMove;
				
			if (singularScore < singularBeta) {
				if (!pvNode && (singularScore < singularBeta - 30) && (DoubleExtensions[level] < 6)) {
					extension = 2;
					DoubleExtensions[level] += 1;
				}
				else {
					extension = 1;
				}
			}
			else if (!pvNode && (singularBeta >= beta)) return singularBeta; // Multicut
		}

		// Push move
		Boards[level] = board;
		Board& b = Boards[level];
		const uint8_t movedPiece = b.GetPieceAt(m.from);
		const uint8_t capturedPiece = b.GetPieceAt(m.to);
		MoveStack[level].move = m;
		MoveStack[level].piece = movedPiece;
		const uint64_t nodesBefore = Statistics.Nodes;

		b.Push(m);
		Heuristics.PrefetchTranspositionEntry(b.Hash());
		Statistics.Nodes += 1;
		int score = NoEval;
		const bool givingCheck = b.IsInCheck();
		UpdateAccumulators(m, movedPiece, capturedPiece, level);

		if (legalMoveCount == 1) {
			score = -SearchRecursive(b, depth - 1 + extension, level + 1, -beta, -alpha, true);
		}
		else {
			int reduction = 0;

			// Late-move reductions (+119 elo)
			if ((legalMoveCount >= (pvNode ? 6 : 4)) && isQuiet && (depth >= 3)) {
				
				reduction = LMRTable[std::min(depth, 31)][std::min(legalMoveCount, 31)];

				// Less reduction when in check
				if (inCheck) reduction -= 1;

				// More reduction for non-PV nodes
				if (!pvNode) reduction += 1;

				// Less reduction when the next ply only had a few fail-highs
				if (CutoffCount[level] < 4) reduction -= 1;

				// Adjust based on history
				if (std::abs(order) < 80000) reduction -= std::clamp(order / 8192, -2, 2);

				reduction = std::clamp(reduction, 0, depth - 1);
			}

			// Principal variation search
			score = -SearchRecursive(b, depth - 1 - reduction, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (reduction > 0)) score = -SearchRecursive(b, depth - 1, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (score < beta)) score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
		}

		// Update node count table for the root
		if (rootNode) RootNodeCounts[m.from][m.to] += Statistics.Nodes - nodesBefore;

		// Process search results
		if (score > bestScore) {
			bestScore = score;
			if (!aborting && pvNode) Heuristics.UpdatePvTable(m, level);

			// Fail-high
			if (score >= beta) {
				bestMove = m;
				scoreType = ScoreType::LowerBound;

				Statistics.BetaCutoffs += 1;
				if (legalMoveCount == 1) Statistics.FirstMoveBetaCutoffs += 1;
				if (level != 0) CutoffCount[level - 1] += 1;

				if (!aborting) {

					const int16_t historyDelta = std::min(300 * (depth - 1), 2250);

					// If a quiet move causes a fail-high, update move ordering tables
					if (isQuiet) {
						Heuristics.AddKillerMove(m, level);
						const bool fromSquareAttacked = CheckBit(opponentAttacks, m.from);
						const bool toSquareAttacked = CheckBit(opponentAttacks, m.to);
						if (level > 0) Heuristics.AddCountermove(MoveStack[level - 1].move, m);
						if (depth > 1) Heuristics.UpdateHistory(m, historyDelta, movedPiece, depth, MoveStack, level, fromSquareAttacked, toSquareAttacked);
					}

					// Decrement history scores for all previously tried quiet moves
					if (depth > 1) {
						if (isQuiet) quietsTried.pop_back(); // don't decrement for the current quiet move
						for (const Move& previouslyTriedMove : quietsTried) {
							const bool fromSquareAttacked = CheckBit(opponentAttacks, previouslyTriedMove.from);
							const bool toSquareAttacked = CheckBit(opponentAttacks, previouslyTriedMove.to);
							const uint8_t previouslyTriedPiece = board.GetPieceAt(previouslyTriedMove.from);
							Heuristics.UpdateHistory(previouslyTriedMove, -historyDelta, previouslyTriedPiece, depth, MoveStack, level, fromSquareAttacked, toSquareAttacked);
						}
					}
				}
				break;
			}
			
			// Raise alpha
			if (score > alpha) {
				bestMove = m;
				scoreType = ScoreType::Exact;
				alpha = score;
			}
		}
	}

	// There was no legal move --> return mate or stalemate score
	if (legalMoveCount == 0) {
		if (singularSearch) return alpha; // always extend if we have only one legal move
		return inCheck ? LosingMateScore(level) : 0;
	}

	// Return the best score (fail-soft)
	if (!aborting && !singularSearch) Heuristics.AddTranspositionEntry(hash, Age, depth, bestScore, scoreType, bestMove, level);

	return bestScore;
}

// Quiescence search: for noisy moves only (captures, queen promotions)
int Search::SearchQuiescence(Board& board, const int level, int alpha, int beta) {

	// Check search limits
	const bool aborting = ShouldAbort();
	if (aborting) return NoEval;

	// Update statistics
	Statistics.AlphaBetaCalls += 1;
	if (level > Statistics.SelDepth) Statistics.SelDepth = level;

	// Update alpha-beta bounds, return alpha if no captures left
	const int staticEval = Evaluate(board, level);
	if (staticEval >= beta) return staticEval;
	if (staticEval > alpha) alpha = staticEval;
	if (level >= MaxDepth) return staticEval;
	if (board.IsDraw(false)) return DrawEvaluation(); // maybe staticEval is more sound?

	// Probe the transposition table
	const uint64_t hash = board.Hash();
	TranspositionEntry ttEntry;
	const bool found = Heuristics.RetrieveTranspositionEntry(hash, ttEntry, level);
	Statistics.TranspositionQueries += 1;
	if (found) {
		if (ttEntry.IsCutoffPermitted(0, alpha, beta)) return ttEntry.score;
		Statistics.TranspositionHits += 1;
	}

	// Generate noisy moves and order them
	MoveListStack[level].reset();
	board.GenerateMoves(MoveListStack[level], MoveGen::Noisy, Legality::Pseudolegal);
	OrderMovesQ(board, MoveListStack[level], level);
	MovePicker movePicker(MoveListStack[level]);

	// Search recursively
	int bestScore = staticEval;
	int scoreType = ScoreType::UpperBound;
	while (movePicker.hasNext()) {
		const auto& [m, order] = movePicker.get();
		if (!board.IsLegalMove(m)) continue;
		if (!StaticExchangeEval(board, m, 0)) continue; // Quiescence search SEE pruning (+39 elo)
		Statistics.Nodes += 1;
		Statistics.QuiescenceNodes += 1;

		Boards[level] = board;
		Board& b = Boards[level];
		const uint8_t movedPiece = board.GetPieceAt(m.from);
		const uint8_t capturedPiece = board.GetPieceAt(m.to);
		b.Push(m);
		Heuristics.PrefetchTranspositionEntry(b.Hash());
		UpdateAccumulators(m, movedPiece, capturedPiece, level);
		const int score = -SearchQuiescence(b, level + 1, -beta, -alpha);

		if (score > bestScore) {
			bestScore = score;
			if (bestScore >= beta) {
				scoreType = ScoreType::LowerBound;
				break;
			}
			if (bestScore > alpha) {
				alpha = bestScore;
				scoreType = ScoreType::Exact;
			}
		}
	}
	if (!aborting) Heuristics.AddTranspositionEntry(hash, Age, 0, bestScore, scoreType, EmptyMove, level);
	return bestScore;
}

int Search::Evaluate(const Board &board, const int level) {
	Statistics.Evaluations += 1;
	//if (NeuralEvaluate(Accumulator, board.Turn) != NeuralEvaluate(board)) cout << "!" << endl;
	//return NeuralEvaluate(board);
	return NeuralEvaluate((*Accumulators)[level], board.Turn);
}

int Search::DrawEvaluation() {
	// Returns a small randomized score to avoid search getting stuck in threefold lines
	return Statistics.Nodes % 4 - 2;
}

// Static exchange evaluation (SEE) ---------------------------------------------------------------

const int seeValues[] = { 0, 100, 300, 300, 500, 1000, 999999 };

bool Search::StaticExchangeEval(const Board& board, const Move& move, const int threshold) const {
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
	const uint64_t whitePieces = board.GetOccupancy(Side::White);
	const uint64_t blackPieces = board.GetOccupancy(Side::Black);
	const uint64_t parallels = board.WhiteRookBits | board.BlackRookBits | board.WhiteQueenBits | board.BlackQueenBits; 
	const uint64_t diagonals = board.WhiteBishopBits | board.BlackBishopBits | board.WhiteQueenBits | board.BlackQueenBits;
	uint64_t occupancy = whitePieces | blackPieces;
	SetBitFalse(occupancy, move.from);
	SetBitTrue(occupancy, move.to);
	bool turn = board.Turn;
	if (move.flag == MoveFlag::EnPassantPerformed) {
		SetBitFalse(occupancy, (turn == Side::White) ? move.to - 8 : move.to + 8);
	}
	turn = !turn;
	uint64_t attackers = board.GetAttackersOfSquare(move.to, occupancy) & occupancy;

	// Pseudo-generating steps
	while (true) {

		uint64_t currentAttackers = attackers & ((turn == Side::White) ? whitePieces : blackPieces);
		if (!currentAttackers) break;

		// Retrieve the location of the least valuable attacking piece type
		int sq = -1;
		if (turn == Side::White) {
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
		assert(sq != -1);

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
			const uint64_t upcomingOccupnacy = (turn == Side::White) ? whitePieces : blackPieces;
			if ((victim == PieceType::King) && (currentAttackers & upcomingOccupnacy)) {
				turn = !turn;
			}
			break;
		}
	}

	// If after the exchange it's our opponent's turn, that means we won
	return turn != board.Turn;
}

// Move ordering ----------------------------------------------------------------------------------

void Search::OrderMoves(const Board& board, MoveList& ml, const int level, const Move& ttMove, const uint64_t opponentAttacks) {
	for (auto& m : ml) {
		const bool losingCapture = board.IsMoveQuiet(m.move) ? false : !StaticExchangeEval(board, m.move, 0);
		m.orderScore = Heuristics.CalculateOrderScore(board, m.move, level, ttMove, MoveStack, losingCapture, true, opponentAttacks);
	}
}

void Search::OrderMovesQ(const Board& board, MoveList& ml, const int level) {
	for (auto& m : ml) {
		m.orderScore = Heuristics.CalculateOrderScore(board, m.move, level, NullMove, MoveStack, false, false, 0);
	}
}

// Accumulators for neural networks ---------------------------------------------------------------

void Search::SetupAccumulators(const Board& board) {
	(*Accumulators)[0].Reset();
	uint64_t bits = board.GetOccupancy();

	// Turning on the right inputs
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		const int piece = board.GetPieceAt(sq);
		(*Accumulators)[0].AddFeature(FeatureIndexes(piece, sq));
	}
}

void Search::UpdateAccumulators(const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece, const int level) {

	// Copy the previous state over
	//std::memcpy(&(*Accumulators)[level + 1], &(*Accumulators)[level], sizeof(AccumulatorRepresentation));
	(*Accumulators)[level + 1] = (*Accumulators)[level];
	if (m.IsNull()) return; // If it's a null move, we're done
	AccumulatorRepresentation& Accumulator = (*Accumulators)[level + 1];

	// No longer activate the previous position of the moved piece
	Accumulator.RemoveFeature( FeatureIndexes(movedPiece, m.from) );

	// No longer activate the position of the captured piece (if any)
	if (capturedPiece != Piece::None) {
		Accumulator.RemoveFeature( FeatureIndexes(capturedPiece, m.to) );
	}

	// Activate the new position of the moved piece
	if (!m.IsPromotion()) {
		Accumulator.AddFeature( FeatureIndexes(movedPiece, m.to) );
	}
	else {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		Accumulator.AddFeature( FeatureIndexes(promotionPiece, m.to) );
	}

	// Special cases
	switch (m.flag) {
		case MoveFlag::None: break;

		// Short castling: move the castling rook from H1 to F1 or from H8 to F8
		case MoveFlag::ShortCastle:
			if (ColorOfPiece(movedPiece) == PieceColor::White) {
				Accumulator.RemoveFeature( FeatureIndexes(Piece::WhiteRook, Squares::H1) );
				Accumulator.AddFeature( FeatureIndexes(Piece::WhiteRook, Squares::F1) );
			}
			else {
				Accumulator.RemoveFeature( FeatureIndexes(Piece::BlackRook, Squares::H8) );
				Accumulator.AddFeature( FeatureIndexes(Piece::BlackRook, Squares::F8) );
			}
			break;

		// Long castling: move the castling rook from A1 to D1 or from A8 to D8
		case MoveFlag::LongCastle:
			if (ColorOfPiece(movedPiece) == PieceColor::White) {
				Accumulator.RemoveFeature( FeatureIndexes(Piece::WhiteRook, Squares::A1) );
				Accumulator.AddFeature( FeatureIndexes(Piece::WhiteRook, Squares::D1) );
			}
			else {
				Accumulator.RemoveFeature( FeatureIndexes(Piece::BlackRook, Squares::A8) );
				Accumulator.AddFeature( FeatureIndexes(Piece::BlackRook, Squares::D8) );
			}
			break;

		// Treat en passant
		case MoveFlag::EnPassantPerformed:
			if (movedPiece == Piece::WhitePawn) Accumulator.RemoveFeature( FeatureIndexes(Piece::BlackPawn, m.to - 8) );
			else Accumulator.RemoveFeature( FeatureIndexes(Piece::WhitePawn, m.to + 8) );
			break;
	}

}