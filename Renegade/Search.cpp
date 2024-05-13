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
	History.ClearHistory();
	History.ClearKillerAndCounterMoves();
	ResetPvTable();
	if (clearTT) TranspositionTable.Clear();
}

// Perft methods ----------------------------------------------------------------------------------

void Search::Perft(Position& position, const int depth, const PerftType type) const {
	//Board b = board;
	const bool startingPosition = position.Hash() == 0x463b96181691fc9c;
	const uint64_t startingPerfts[] = { 1, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860 };

	const auto startTime = Clock::now();
	const uint64_t r = PerftRecursive(position, depth, depth, type);
	const auto endTime = Clock::now();

	const float seconds = static_cast<float>((endTime - startTime).count() / 1e9);
	const float speed = r / seconds / 1000000;
	cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps | No bulk counting" << endl;
	if (startingPosition && (depth <= 6) && (startingPerfts[depth] != r)) cout << "Uh-oh. (expected: " << startingPerfts[depth] << ")" << endl;
}

uint64_t Search::PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const {
	MoveList moves{};
	position.GenerateMoves(moves, MoveGen::All, Legality::Pseudolegal);

	if ((type == PerftType::PerftDiv) && (originalDepth == depth)) cout << "Legal moves (" << moves.size() << "): " << endl;
	uint64_t count = 0;
	for (const auto& m : moves) {
		if (!position.IsLegalMove(m.move)) continue;
		uint64_t r;
		if (depth == 1) {
			r = 1;
			count += 1;
		}
		else {
			position.Push(m.move);
			r = PerftRecursive(position, depth - 1, originalDepth, type);
			position.Pop();
			count += r;
		}
		if ((originalDepth == depth) && (type == PerftType::PerftDiv))
			cout << " - " << m.move.ToString(Settings::Chess960) << " : " << r << endl;
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

Results Search::SearchMoves(Position& position, const SearchParams params, const bool display) {

	StartSearchTime = Clock::now();
	Aborting.store(false, std::memory_order_relaxed);
	int elapsedMs = 0;
	ResetStatistics();
	bool finished = false;
	TranspositionTable.IncreaseAge();

	SetupAccumulators(position);
	std::fill(ExcludedMoves.begin(), ExcludedMoves.end(), EmptyMove);
	std::fill(CutoffCount.begin(), CutoffCount.end(), 0);
	std::fill(DoubleExtensions.begin(), DoubleExtensions.end(), 0);

	// Early exit for only one legal move (no legal moves are handled separately)
	if (!DatagenMode && ((params.wtime != 0) || (params.btime != 0))) {
		MoveList rootLegalMoves{};
		position.GenerateMoves(rootLegalMoves, MoveGen::All, Legality::Legal);
		if (rootLegalMoves.size() == 1) {
			const int eval = Evaluate(position, 0);
			cout << "info string Only one legal move!" << endl;
			cout << "info depth 1 score cp " << ToCentipawns(eval, position.GetPlys()) << " nodes 0" << endl;
			const Move onlyMove = rootLegalMoves[0].move;
			PrintBestmove(onlyMove);
			Aborting.store(true, std::memory_order_relaxed);
			return Results(eval, {onlyMove}, 1, Statistics, 0, 0, 0, position.GetPlys()); // hack: rootLegalMoves is a vector already
		}
	}

	Constraints = CalculateConstraints(params, position.Turn());
	std::memset(&RootNodeCounts, 0, sizeof(RootNodeCounts));

	// Iterative deepening
	Results e = Results();
	e.ply = position.GetPlys();
	int result = NoEval;
	while (!finished) {
		ResetPvTable();
		Depth += 1;
		Statistics.SelDepth = Depth;

		// Obtain score
		if (Depth < 5) {
			// Regular negamax for shallow depths
			result = SearchRecursive(position, Depth, 0, NegativeInfinity, PositiveInfinity, true);
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

				result = SearchRecursive(position, searchDepth, 0, alpha, beta, true);

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
					if (!IsMateScore(result) && (searchDepth > 1)) searchDepth -= 1;
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
			const Move& bestMove = PvTable[0][0];
			const double bestMoveFraction = static_cast<double>(RootNodeCounts[bestMove.from][bestMove.to]) / static_cast<double>(Statistics.Nodes);
			const int adjustedSoftTimeLimit = originalSoftTimeLimit * static_cast<float>(Depth >= 10 ? (1.5 - bestMoveFraction) * 1.35 : 1.0);
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
			e.hashfull = TranspositionTable.GetHashfull();
			if (display) PrintInfo(e);
			break;
		}

		// Send info
		e.score = result;
		e.depth = Depth;
		e.stats = Statistics;
		e.time = elapsedMs;
		e.nps = static_cast<int>(Statistics.Nodes * 1e9 / (currentTime - StartSearchTime).count());
		e.hashfull = TranspositionTable.GetHashfull();

		// Obtaining PV line
		e.pv.clear();
		GeneratePvLine(e.pv);
		if (display) PrintInfo(e);
	}
	if (display) PrintBestmove(e.BestMove());

	History.ClearKillerAndCounterMoves();
	ResetPvTable();
	Aborting.store(true, std::memory_order_relaxed);
	return e;
}

// Recursively called during the alpha-beta search
int Search::SearchRecursive(Position& position, int depth, const int level, int alpha, int beta, const bool canNullMove) {

	// Check search limits
	const bool aborting = ShouldAbort();
	if (aborting) return NoEval;
	InitPvLength(level);
	if (level >= MaxDepth) return Evaluate(position, level);

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
	if (!rootNode && position.IsDrawn(false)) return DrawEvaluation();

	// Check extensions
	const bool inCheck = position.IsInCheck();
	if (!rootNode) {
		if (inCheck) depth += 1;
	}

	// Drop into quiescence search at depth 0
	if (depth <= 0) {
		return SearchQuiescence(position, level, alpha, beta);
	}

	const Move excludedMove = ExcludedMoves[level];
	const bool singularSearch = !excludedMove.IsEmpty();

	// Probe the transposition table
	TranspositionEntry ttEntry;
	int ttEval = NoEval;
	Move ttMove = EmptyMove;
	bool found = false;
	const uint64_t hash = position.Hash();

	if (!singularSearch) {
		found = TranspositionTable.Probe(hash, ttEntry, level);
		Statistics.TranspositionQueries += 1;
		if (found) {
			if (!pvNode) {
				// The branch was already analyzed to the same or greater depth, so we can return the result if the score is alright
				if (ttEntry.IsCutoffPermitted(depth, alpha, beta)) return ttEntry.score;
			}
			ttEval = ttEntry.score;
			ttMove = Move(ttEntry.packedMove);
			Statistics.TranspositionHits += 1;
		}
	}

	const bool singularCandidate = found && !rootNode && !singularSearch && (depth > 8)
		&& (ttEntry.depth >= depth - 3) && (ttEntry.scoreType != ScoreType::UpperBound) && (std::abs(ttEval) < MateThreshold);
	
	// Obtain the evaluation of the position
	int staticEval = NoEval;
	int eval = NoEval;

	if (!singularSearch) {
		staticEval = inCheck ? NoEval : Evaluate(position, level);
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
		const int rfpMargin = depth * 90 - improving * 90;
		if ((depth <= 7) && (std::abs(beta) < MateThreshold)) {
			if (eval - rfpMargin > beta) return eval;
		}

		// Null-move pruning (+33 elo)
		if ((depth >= 3) && canNullMove && (eval >= beta) && position.HasNonPawnMaterial()) {
			int nmpReduction = 3 + depth / 3 + std::min((eval - beta) / 200, 3);
			nmpReduction = std::min(nmpReduction, depth);
			position.PushNullMove();
			UpdateAccumulators(NullMove, 0, 0, level);
			const int nullMoveEval = -SearchRecursive(position, depth - nmpReduction, level + 1, -beta, -beta + 1, false);
			position.Pop();
			if (nullMoveEval >= beta) {
				return IsMateScore(nullMoveEval) ? beta : nullMoveEval;
			}
		}

		// Futility pruning (+37 elo)
		const int futilityMargin = 30 + depth * 100;
		if ((depth <= 5) && (std::abs(beta) < MateThreshold)) {
			futilityPrunable = (eval + futilityMargin < alpha);
		}
	}

	// Internal iterative reductions
	if ((depth > 4) && ttMove.IsEmpty() && !singularSearch) {
		depth -= 1;
	}

	// Initialize variables and generate moves
	// (if we are in singular search, we already have the moves)
	const uint64_t opponentAttacks = position.CalculateAttackedSquares(!position.Turn()); // ^ to do: make a stack variable for it

	if (!singularSearch) {
		// Generating moves and move ordering
		MoveListStack[level].reset();
		position.GenerateMoves(MoveListStack[level], MoveGen::All, Legality::Pseudolegal);
		OrderMoves(position, MoveListStack[level], level, ttMove, opponentAttacks);

		// Resetting killers and fail-high cutoff counts
		if (level + 2 < MaxDepth) History.ResetKillersForPly(level + 2);
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
		if (!position.IsLegalMove(m)) continue;
		legalMoveCount += 1;
		const bool isQuiet = position.IsMoveQuiet(m);
		if (isQuiet) quietsTried.push_back(m);

		// Moves loop pruning techniques
		if (!pvNode && (bestScore > -MateThreshold) && (order < 90000) && !DatagenMode) {

			// Late-move pruning (+9 elo)
			if (isQuiet && !inCheck && (depth < 5)) {
				const int lmpCount = 3 + depth * (depth - !improving);
				if (legalMoveCount > lmpCount) break;
			}

			// Performing futility pruning
			if (isQuiet && (alpha < MateThreshold) && futilityPrunable) break;

			// Main search SEE pruning (+20 elo)
			if (depth <= 5) {
				const int seeMargin = isQuiet ? (-50 * depth) : (-100 * depth);
				if (!StaticExchangeEval(position, m, seeMargin)) continue;
			}
		}

		// Singular extensions
		int extension = 0;
		if (singularCandidate && (m == ttMove)) {
			const int singularMargin = depth * 2;
			const int singularBeta = std::max(ttEval - singularMargin, -MateEval);
			const int singularDepth = (depth - 1) / 2;
			ExcludedMoves[level] = m;
			const int singularScore = SearchRecursive(position, singularDepth, level, singularBeta - 1, singularBeta, false);
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
		const uint8_t movedPiece = position.GetPieceAt(m.from);
		const uint8_t capturedPiece = position.GetPieceAt(m.to);
		const uint64_t nodesBefore = Statistics.Nodes;

		position.Push(m);
		TranspositionTable.Prefetch(position.Hash());
		Statistics.Nodes += 1;
		int score = NoEval;
		UpdateAccumulators(m, movedPiece, capturedPiece, level);

		if (legalMoveCount == 1) {
			score = -SearchRecursive(position, depth - 1 + extension, level + 1, -beta, -alpha, true);
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
			score = -SearchRecursive(position, depth - 1 - reduction, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (reduction > 0)) score = -SearchRecursive(position, depth - 1, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (score < beta)) score = -SearchRecursive(position, depth - 1, level + 1, -beta, -alpha, true);
		}
		position.Pop();

		// Update node count table for the root
		if (rootNode) RootNodeCounts[m.from][m.to] += Statistics.Nodes - nodesBefore;

		// Process search results
		if (score > bestScore) {
			bestScore = score;
			if (!aborting && pvNode) UpdatePvTable(m, level);

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
						History.AddKillerMove(m, level);
						const bool fromSquareAttacked = CheckBit(opponentAttacks, m.from);
						const bool toSquareAttacked = CheckBit(opponentAttacks, m.to);
						if (level > 0) History.AddCountermove(position.GetPreviousMove(1).move, m);
						if (depth > 1) History.UpdateHistory(m, historyDelta, movedPiece, depth, position, level, fromSquareAttacked, toSquareAttacked);
					}

					// Decrement history scores for all previously tried quiet moves
					if (depth > 1) {
						if (isQuiet) quietsTried.pop_back(); // don't decrement for the current quiet move
						for (const Move& previouslyTriedMove : quietsTried) {
							const bool fromSquareAttacked = CheckBit(opponentAttacks, previouslyTriedMove.from);
							const bool toSquareAttacked = CheckBit(opponentAttacks, previouslyTriedMove.to);
							const uint8_t previouslyTriedPiece = position.GetPieceAt(previouslyTriedMove.from);
							History.UpdateHistory(previouslyTriedMove, -historyDelta, previouslyTriedPiece, depth, position, level, fromSquareAttacked, toSquareAttacked);
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
	if (!aborting && !singularSearch) TranspositionTable.Store(hash, depth, bestScore, scoreType, bestMove, level);

	return bestScore;
}

// Quiescence search: for noisy moves only (captures, queen promotions)
int Search::SearchQuiescence(Position& position, const int level, int alpha, int beta) {

	// Check search limits
	const bool aborting = ShouldAbort();
	if (aborting) return NoEval;

	// Update statistics
	Statistics.AlphaBetaCalls += 1;
	if (level > Statistics.SelDepth) Statistics.SelDepth = level;

	// Update alpha-beta bounds, return alpha if no captures left
	const int staticEval = Evaluate(position, level);
	if (staticEval >= beta) return staticEval;
	if (staticEval > alpha) alpha = staticEval;
	if (level >= MaxDepth) return staticEval;
	if (position.IsDrawn(false)) return DrawEvaluation(); // maybe staticEval is more sound?

	// Probe the transposition table
	const uint64_t hash = position.Hash();
	TranspositionEntry ttEntry;
	const bool found = TranspositionTable.Probe(hash, ttEntry, level);
	Statistics.TranspositionQueries += 1;
	if (found) {
		if (ttEntry.IsCutoffPermitted(0, alpha, beta)) return ttEntry.score;
		Statistics.TranspositionHits += 1;
	}

	// Generate noisy moves and order them
	MoveListStack[level].reset();
	position.GenerateMoves(MoveListStack[level], MoveGen::Noisy, Legality::Pseudolegal);
	OrderMovesQ(position, MoveListStack[level], level);
	MovePicker movePicker(MoveListStack[level]);

	// Search recursively
	int bestScore = staticEval;
	int scoreType = ScoreType::UpperBound;
	while (movePicker.hasNext()) {
		const auto& [m, order] = movePicker.get();
		if (!position.IsLegalMove(m)) continue;
		if (!StaticExchangeEval(position, m, 0)) continue; // Quiescence search SEE pruning (+39 elo)
		Statistics.Nodes += 1;
		Statistics.QuiescenceNodes += 1;

		const uint8_t movedPiece = position.GetPieceAt(m.from);
		const uint8_t capturedPiece = position.GetPieceAt(m.to);
		position.Push(m);
		TranspositionTable.Prefetch(position.Hash());
		UpdateAccumulators(m, movedPiece, capturedPiece, level);
		const int score = -SearchQuiescence(position, level + 1, -beta, -alpha);
		position.Pop();

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
	if (!aborting) TranspositionTable.Store(hash, 0, bestScore, scoreType, EmptyMove, level);
	return bestScore;
}

int Search::Evaluate(const Position& position, const int level) {
	Statistics.Evaluations += 1;
	//if (NeuralEvaluate((*Accumulators)[level], position.Turn()) != NeuralEvaluate(position)) cout << "!" << endl;
	return NeuralEvaluate((*Accumulators)[level], position.Turn());
}

int Search::DrawEvaluation() const {
	// Returns a small randomized score to avoid search getting stuck in threefold lines
	return Statistics.Nodes % 4 - 2;
}

// Static exchange evaluation (SEE) ---------------------------------------------------------------

bool Search::StaticExchangeEval(const Position& position, const Move& move, const int threshold) const {
	// This is more or less the standard way of doing this
	// The implementation follows Ethereal's method

	constexpr auto seeValues = std::array{ 0, 100, 300, 300, 500, 1000, 999999 };

	// Get the initial piece
	uint8_t victim = TypeOfPiece(position.GetPieceAt(move.from));
	if (move.IsPromotion()) victim = move.GetPromotionPieceType();

	// Get estimated move value
	int estimatedMoveValue = seeValues[TypeOfPiece(position.GetPieceAt(move.to))];
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
	const uint64_t whitePieces = position.GetOccupancy(Side::White);
	const uint64_t blackPieces = position.GetOccupancy(Side::Black);
	const uint64_t parallels = position.WhiteRookBits() | position.BlackRookBits() | position.WhiteQueenBits() | position.BlackQueenBits();
	const uint64_t diagonals = position.WhiteBishopBits() | position.BlackBishopBits() | position.WhiteQueenBits() | position.BlackQueenBits();
	uint64_t occupancy = whitePieces | blackPieces;
	SetBitFalse(occupancy, move.from);
	SetBitTrue(occupancy, move.to);
	bool turn = position.Turn();
	if (move.flag == MoveFlag::EnPassantPerformed) {
		SetBitFalse(occupancy, (turn == Side::White) ? move.to - 8 : move.to + 8);
	}
	turn = !turn;
	uint64_t attackers = position.GetAttackersOfSquare(move.to, occupancy) & occupancy;

	// Pseudo-generating steps
	while (true) {

		uint64_t currentAttackers = attackers & ((turn == Side::White) ? whitePieces : blackPieces);
		if (!currentAttackers) break;

		// Retrieve the location of the least valuable attacking piece type
		int sq = -1;
		if (turn == Side::White) {
			if (currentAttackers & position.WhitePawnBits()) sq = LsbSquare(currentAttackers & position.WhitePawnBits());
			else if (currentAttackers & position.WhiteKnightBits()) sq = LsbSquare(currentAttackers & position.WhiteKnightBits());
			else if (currentAttackers & position.WhiteBishopBits()) sq = LsbSquare(currentAttackers & position.WhiteBishopBits());
			else if (currentAttackers & position.WhiteRookBits()) sq = LsbSquare(currentAttackers & position.WhiteRookBits());
			else if (currentAttackers & position.WhiteQueenBits()) sq = LsbSquare(currentAttackers & position.WhiteQueenBits());
			else if (currentAttackers & position.WhiteKingBits()) sq = LsbSquare(currentAttackers & position.WhiteKingBits());
		}
		else {
			if (currentAttackers & position.BlackPawnBits()) sq = LsbSquare(currentAttackers & position.BlackPawnBits());
			else if (currentAttackers & position.BlackKnightBits()) sq = LsbSquare(currentAttackers & position.BlackKnightBits());
			else if (currentAttackers & position.BlackBishopBits()) sq = LsbSquare(currentAttackers & position.BlackBishopBits());
			else if (currentAttackers & position.BlackRookBits()) sq = LsbSquare(currentAttackers & position.BlackRookBits());
			else if (currentAttackers & position.BlackQueenBits()) sq = LsbSquare(currentAttackers & position.BlackQueenBits());
			else if (currentAttackers & position.BlackKingBits()) sq = LsbSquare(currentAttackers & position.BlackKingBits());
		}
		assert(sq != -1);

		// Update fields
		victim = TypeOfPiece(position.GetPieceAt(sq));
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
	return turn != position.Turn();
}

// Accumulators for neural networks ---------------------------------------------------------------

void Search::SetupAccumulators(const Position& position) {
	(*Accumulators)[0].Reset();
	uint64_t bits = position.GetOccupancy();

	// Turning on the right inputs
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		const int piece = position.GetPieceAt(sq);
		(*Accumulators)[0].AddFeature(FeatureIndexes(piece, sq));
	}
}

void Search::UpdateAccumulators(const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece, const int level) {

	// Copy the previous state over
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
	if (!m.IsPromotion() && !m.IsCastling()) {
		Accumulator.AddFeature( FeatureIndexes(movedPiece, m.to) );
	}
	else if (m.IsPromotion()) {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		Accumulator.AddFeature( FeatureIndexes(promotionPiece, m.to) );
	}

	// Special cases
	switch (m.flag) {
		case MoveFlag::None: break;

		case MoveFlag::ShortCastle:
			if (ColorOfPiece(movedPiece) == PieceColor::White) {
				Accumulator.AddFeature(FeatureIndexes(Piece::WhiteKing, Squares::G1));
				Accumulator.AddFeature(FeatureIndexes(Piece::WhiteRook, Squares::F1));
			}
			else {
				Accumulator.AddFeature(FeatureIndexes(Piece::BlackKing, Squares::G8));
				Accumulator.AddFeature(FeatureIndexes(Piece::BlackRook, Squares::F8));
			}
			break;

		case MoveFlag::LongCastle:
			if (ColorOfPiece(movedPiece) == PieceColor::White) {
				Accumulator.AddFeature(FeatureIndexes(Piece::WhiteKing, Squares::C1));
				Accumulator.AddFeature(FeatureIndexes(Piece::WhiteRook, Squares::D1));
			}
			else {
				Accumulator.AddFeature(FeatureIndexes(Piece::BlackKing, Squares::C8));
				Accumulator.AddFeature(FeatureIndexes(Piece::BlackRook, Squares::D8));
			}
			break;

		// Treat en passant
		case MoveFlag::EnPassantPerformed:
			if (movedPiece == Piece::WhitePawn) Accumulator.RemoveFeature( FeatureIndexes(Piece::BlackPawn, m.to - 8) );
			else Accumulator.RemoveFeature( FeatureIndexes(Piece::WhitePawn, m.to + 8) );
			break;
	}

}

// PV table ---------------------------------------------------------------------------------------

void Search::InitPvLength(const int level) {
	PvLength[level] = level;
}

void Search::UpdatePvTable(const Move& move, const int level) {
	PvTable[level][level] = move;
	for (int nextLevel = level + 1; nextLevel < PvLength[level + 1]; nextLevel++) {
		PvTable[level][nextLevel] = PvTable[level + 1][nextLevel];
	}
	PvLength[level] = PvLength[level + 1];
}

void Search::GeneratePvLine(std::vector<Move>& list) const {
	for (int i = 0; i < PvLength[0]; i++) {
		const Move& m = PvTable[0][i];
		if (m.IsEmpty()) break;
		list.push_back(m);
	}
}

void Search::ResetPvTable() {
	for (int i = 0; i < MaxDepth; i++) {
		for (int j = 0; j < MaxDepth; j++) PvTable[i][j] = EmptyMove;
		PvLength[i] = 0; // ?
	}
}

// Move ordering ----------------------------------------------------------------------------------

int Search::CalculateOrderScore(const Position& position, const Move& m, const int level, const Move& ttMove,
	const bool losingCapture, const bool useMoveStack, const uint64_t opponentAttacks) const {

	const uint8_t movedPiece = position.GetPieceAt(m.from);
	const uint8_t attackingPieceType = TypeOfPiece(movedPiece);
	const uint8_t capturedPieceType = TypeOfPiece(position.GetPieceAt(m.to));
	constexpr std::array<int, 7> values = { 0, 100, 300, 300, 500, 900, 0 };

	// Transposition move
	if (m == ttMove) return 900000;

	// Queen promotions
	if (m.flag == MoveFlag::PromotionToQueen) return 700000 + values[capturedPieceType];

	// Captures
	if (!m.IsCastling()) {
		if (!losingCapture) {
			if (capturedPieceType != PieceType::None) return 600000 + values[capturedPieceType] * 16 - values[attackingPieceType];
			if (m.flag == MoveFlag::EnPassantPerformed) return 600000 + values[PieceType::Pawn] * 16 - values[PieceType::Pawn];
		}
		else {
			if (capturedPieceType != PieceType::None) return -200000 + values[capturedPieceType] * 16 - values[attackingPieceType];
			if (m.flag == MoveFlag::EnPassantPerformed) return -200000 + values[PieceType::Pawn] * 16 - values[PieceType::Pawn];
		}
	}

	// Quiet killer moves
	if (History.IsFirstKillerMove(m, level)) return 100100;
	if (History.IsSecondKillerMove(m, level)) return 100000;

	// Countermove heuristic
	if (level > 0 && useMoveStack && History.IsCountermove(position.GetPreviousMove(1).move, m)) return 99000;

	// Quiet moves
	const bool turn = position.Turn();
	const int historyScore = History.GetHistoryScore(position, m, level, movedPiece, opponentAttacks);

	return historyScore;
}

void Search::OrderMoves(const Position& position, MoveList& ml, const int level, const Move& ttMove, const uint64_t opponentAttacks) {
	for (auto& m : ml) {
		const bool losingCapture = position.IsMoveQuiet(m.move) ? false : !StaticExchangeEval(position, m.move, 0);
		m.orderScore = CalculateOrderScore(position, m.move, level, ttMove, losingCapture, true, opponentAttacks);
	}
}

void Search::OrderMovesQ(const Position& position, MoveList& ml, const int level) {
	for (auto& m : ml) {
		m.orderScore = CalculateOrderScore(position, m.move, level, NullMove, false, false, 0);
	}
}
