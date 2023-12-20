#include "Search.h"

Search::Search() {
	const double lmrMultiplier = 0.4;
	const double lmrBase = 0.7;
	for (int i = 1; i < 32; i++) {
		for (int j = 1; j < 32; j++) {
			LMRTable[i][j] = static_cast<int>(lmrMultiplier * log(i) * log(j) + lmrBase);
		}
	}
	ResetStatistics();
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
	Heuristics.ClearHistoryTable();
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
			Board b = board;
			b.Push(m);
			r = PerftRecursive(b, depth - 1, originalDepth, type);
			count += r;
		}
		if ((originalDepth == depth) && (type == PerftType::PerftDiv)) cout << " - " << m.ToString() << " : " << r << endl;
	}
	return count;
}

// Time management --------------------------------------------------------------------------------

const SearchConstraints Search::CalculateConstraints(const SearchParams params, const bool turn) const {
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
			maxTime = static_cast<int>(myTime / params.movestogo * 2.2);
			minTime = static_cast<int>(myTime / params.movestogo * 0.5);
			maxTime = std::min(maxTime, static_cast<int>(myTime * 0.8));
		}
		else {
			// Sudden death 
			maxTime = static_cast<int>(myTime * 0.20);
			minTime = static_cast<int>(myTime * 0.016 + myInc * 0.4);
		}

		constraints.SearchTimeMax = maxTime;
		constraints.SearchTimeMin = minTime;
		if (constraints.SearchTimeMin > constraints.SearchTimeMax) constraints.SearchTimeMin = constraints.SearchTimeMax;
		return constraints;
	}

	// Default: go infinite
	return constraints;
}

inline bool Search::ShouldAbort() const {
	if (Aborting) return true;
	if ((Constraints.MaxNodes != -1) && (Statistics.Nodes >= Constraints.MaxNodes) && (Depth > 1)) {
		return true;
	}
	if ((Statistics.Nodes % 1024 == 0) && (Constraints.SearchTimeMax != -1) && (Depth > 1)) {
		const auto now = Clock::now();
		const int elapsedMs = static_cast<int>((now - StartSearchTime).count() / 1e6);
		if (elapsedMs >= Constraints.SearchTimeMax) return true;
	}
	return false;
}

// Negamax search routine and handling ------------------------------------------------------------

const Results Search::SearchMoves(Board board, const SearchParams params, const EngineSettings settings, const bool display) {

	StartSearchTime = Clock::now();
	int elapsedMs = 0;
	Depth = 0;
	ResetStatistics();
	Aborting = false;
	Depth = 0;
	bool finished = false;
	if (Age < 32000) Age += 1;

	SetupAccumulators(board);

	// Early exit for only one legal move (no legal moves are handled separately)
	if (!DatagenMode && ((params.wtime != 0) || (params.btime != 0))) {
		std::vector<Move> rootLegalMoves;
		board.GenerateMoves(rootLegalMoves, MoveGen::All, Legality::Legal);
		if (rootLegalMoves.size() == 1) {
			const int eval = Evaluate(board);
			cout << "info depth 1 score cp " << eval << " nodes 0" << endl;
			cout << "info string Only one legal move!" << endl;
			PrintBestmove(rootLegalMoves.front());
			return Results(eval, rootLegalMoves, 1, Statistics, 0, 0, 0); // hack: rootLegalMoves is a vector already
		}
	}

	Constraints = CalculateConstraints(params, board.Turn);

	if (settings.ExtendedOutput) {
		cout << "info string Renegade searching for time: (" << Constraints.SearchTimeMin << ".." << Constraints.SearchTimeMax
			<< ") depth: " << Constraints.MaxDepth << " nodes: " << Constraints.MaxNodes << endl;
	}

	// Iterative deepening
	Results e = Results();
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
			int windowSize = 25;
			int searchDepth = Depth;

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

				windowSize *= 2;

			}

		}

		// Check search limits
		auto currentTime = Clock::now();
		elapsedMs = static_cast<int>((currentTime - StartSearchTime).count() / 1e6);
		if ((elapsedMs >= Constraints.SearchTimeMin) && (Constraints.SearchTimeMin != -1)) finished = true;
		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if (Depth >= MaxDepth) finished = true;
		if ((Statistics.Nodes >= Constraints.SoftNodes) && (Constraints.SoftNodes != -1)) finished = true;
		if (std::abs(result) == MateEval) finished = true; // Don't search if position is checkmate
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
	Heuristics.AgeHistory();
	Aborting = true;
	return e;
}

// Recursively called during the negamax search
int Search::SearchRecursive(Board &board, int depth, const int level, int alpha, int beta, const bool canNullMove) {

	// Check search limits
	Aborting = ShouldAbort();
	if (Aborting) return NoEval;
	Heuristics.InitPvLength(level);
	if (level >= MaxDepth) return Evaluate(board);

	const bool rootNode = (level == 0);
	const bool pvNode = rootNode || (beta - alpha > 1);
	Statistics.AlphaBetaCalls += 1;

	// Mate distance pruning
	if (!rootNode) {
		alpha = std::max(alpha, -MateEval + level);
		beta = std::min(beta, MateEval - level - 1);
		if (alpha >= beta) return alpha;
	}

	// Check extensions
	const bool inCheck = board.IsInCheck();
	if (!rootNode) {
		if (inCheck) depth += 1;
	}

	// Check for draws
	//if (rootNode && board.IsDraw(true)) return 0;
	if (!rootNode && board.IsDraw(false)) return DrawEvaluation();

	// Drop into quiescence search at depth 0
	if (depth <= 0) {
		return SearchQuiescence(board, level, alpha, beta);
	}

	// Probe the transposition table
	const uint64_t hash = board.Hash();
	TranspositionEntry ttEntry;
	int ttEval = NoEval;
	const bool found = Heuristics.RetrieveTranspositionEntry(hash, ttEntry, level);
	Move ttMove = EmptyMove;
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
	
	// Obtain the evaluation of the position
	int staticEval = NoEval;
	if ((ttEval == NoEval) || (ttEntry.scoreType != ScoreType::Exact)) staticEval = Evaluate(board);
	if (ttEval != NoEval) {
		if ((ttEntry.scoreType == ScoreType::Exact)
			|| ((ttEntry.scoreType == ScoreType::LowerBound) && (staticEval < ttEval))
			|| ((ttEntry.scoreType == ScoreType::UpperBound) && (staticEval > ttEval))) staticEval = ttEval;
	}
	EvalStack[level] = staticEval;
	const bool improving = (level >= 2) && (EvalStack[level] > EvalStack[level - 2]) && !inCheck;
	// ^ add nullmove condition? (!inCheck is cosmetic for now)

	// Futility pruning (+37 elo)
	const int futilityMargins[] = { 0, 130, 230, 330, 430, 530 };
	bool futilityPrunable = false;
	if ((depth <= 5) && !inCheck && !pvNode && (std::abs(beta) < MateThreshold)) {
		if ((staticEval + futilityMargins[depth] < alpha)) futilityPrunable = true;
	}

	// Reverse futility pruning (+128 elo)
	const int rfpMarginDefault[] = { 0, 70, 150, 240, 340, 450, 580, 720 };
	if ((depth <= 7) && !inCheck && !pvNode && (std::abs(beta) < MateThreshold)) {
		const int rfpMargin = improving ? rfpMarginDefault[depth] / 2 : rfpMarginDefault[depth];
		if (staticEval - rfpMargin > beta) return staticEval;
	}

	// Null-move pruning (+33 elo)
	if ((depth >= 3) && !inCheck && !pvNode && canNullMove && (staticEval >= beta) && board.ShouldNullMovePrune()) {
		int nmpReduction = 3 + depth / 3 + std::min((staticEval - beta) / 200, 3); // Thanks Discord
		nmpReduction = std::min(nmpReduction, depth);
		Boards[level] = board;
		Boards[level].Push(NullMove);
		MoveStack[level].move = NullMove;
		MoveStack[level].piece = Piece::None;
		const int nullMoveEval = -SearchRecursive(Boards[level], depth - nmpReduction, level + 1, -beta, -beta + 1, false);
		if (nullMoveEval >= beta) {
			return IsMateScore(nullMoveEval) ? beta : nullMoveEval;
		}
	}

	// Internal iterative reductions
	if ((depth > 4) && ttMove.IsEmpty()) {
		depth -= 1;
	}

	// Initalize variables and generate moves
	MoveList.clear();
	board.GenerateMoves(MoveList, MoveGen::All, Legality::Pseudolegal);

	// Move ordering
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	for (const Move& m : MoveList) {
		const bool losingCapture = board.IsMoveQuiet(m) ? false : !StaticExchangeEval(board, m, 0);
		const int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, ttMove, MoveStack, losingCapture, true);
		MoveOrder[level].push_back({ m, orderScore });
	}
	std::stable_sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Resetting killers for ply+2
	if (level + 2 <= MaxDepth) {
		Heuristics.KillerMoves[level + 2][0] = EmptyMove;
		Heuristics.KillerMoves[level + 2][1] = EmptyMove;
	}

	// Iterate through legal moves
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0, pseudoLegalCount = 0;
	Move bestMove = EmptyMove;
	int bestScore = NegativeInfinity;

	std::vector<Move> quietsTried;
	quietsTried.reserve(30); // <-- ???

	for (const auto& [m, order] : MoveOrder[level]) {
		pseudoLegalCount += 1;
		if (!board.IsLegalMove(m)) continue;
		legalMoveCount += 1;
		const bool isQuiet = board.IsMoveQuiet(m);
		if (isQuiet) quietsTried.push_back(m);

		// Performing futility pruning
		if (isQuiet && futilityPrunable && !IsMateScore(alpha) && !IsMateScore(beta) && (bestScore > -MateThreshold) && !DatagenMode) break;

		// Main search SEE pruning (+20 elo)
		const int seeQuietMargin[] = { 0, -50, -100, -150, -200, -250, -300, -350 };
		const int seeNoisyMargin[] = { 0, -100, -200, -300, -400, -500, -600, -700 };
		if (!rootNode && !pvNode && (depth <= 5) && !IsLosingMateScore(alpha) && (bestScore > -MateThreshold) && !DatagenMode) {
			if (!StaticExchangeEval(board, m, isQuiet ? seeQuietMargin[depth] : seeNoisyMargin[depth])) continue;
		}

		// Push move
		Boards[level] = board;
		Board& b = Boards[level];
		const uint8_t movedPiece = b.GetPieceAt(m.from);
		const uint8_t capturedPiece = b.GetPieceAt(m.to);
		MoveStack[level].move = m;
		MoveStack[level].piece = movedPiece;

		b.Push(m);
		Heuristics.PrefetchTranspositionEntry(b.Hash());
		Statistics.Nodes += 1;
		int score = NoEval;
		const bool givingCheck = b.IsInCheck();

		if (legalMoveCount == 1) {
			UpdateAccumulators<true>(m, movedPiece, capturedPiece);
			score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
			UpdateAccumulators<false>(m, movedPiece, capturedPiece);
		}
		else {
			int reduction = 0;

			// Late-move pruning (+9 elo)
			const int lmpCount[] = { 0, 4, 7, 12, 19 };
			if ((depth < 5) && !pvNode && !inCheck && isQuiet && !DatagenMode) {
				if (legalMoveCount > lmpCount[depth]) break;
			}

			// Late-move reductions (+119 elo)
			if ((legalMoveCount >= (pvNode ? 6 : 4)) && isQuiet && (depth >= 3)) {
				
				reduction = LMRTable[std::min(depth, 31)][std::min(legalMoveCount, 31)];
				//const bool badCheck = givingCheck && !StaticExchangeEval(board, m, 0);

				// Less reduction if there are checks involved
				if (inCheck || givingCheck) reduction -= 1;

				// More reduction for non-PV nodes
				if (!pvNode) reduction += 1;

				// Adjust based on history
				if (std::abs(order) < 80000) reduction -= std::clamp(order / 8192, -2, 2);

				reduction = std::clamp(reduction, 0, depth - 1);
			}

			// Principal variation search
			UpdateAccumulators<true>(m, movedPiece, capturedPiece);
			score = -SearchRecursive(b, depth - 1 - reduction, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (reduction > 0)) score = -SearchRecursive(b, depth - 1, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (score < beta)) score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
			UpdateAccumulators<false>(m, movedPiece, capturedPiece);
		}

		if (score > bestScore) {
			bestScore = score;
			if (!Aborting) Heuristics.UpdatePvTable(m, level);

			// Checking alpha-beta bounds
			if (score >= beta) {
				bestMove = m;

				if (!Aborting) Heuristics.AddTranspositionEntry(hash, Age, depth, bestScore, ScoreType::LowerBound, m, level);
				Statistics.BetaCutoffs += 1;
				if (legalMoveCount == 1) Statistics.FirstMoveBetaCutoffs += 1;

				if (isQuiet) {
					Heuristics.AddKillerMove(m, level);
					if (level > 0) Heuristics.AddCountermove(MoveStack[level - 1].move, m);
					if (depth > 1) Heuristics.IncrementHistory(m, movedPiece, depth, MoveStack, level);
				}

				// Decrement history scores for all previously tried quiet moves
				if (depth > 1) {
					if (isQuiet) quietsTried.pop_back(); // don't decrement for the current quiet move
					for (const Move& previouslyTriedMove : quietsTried) {
						const uint8_t previouslyTriedPiece = board.GetPieceAt(previouslyTriedMove.from);
						Heuristics.DecrementHistory(previouslyTriedMove, previouslyTriedPiece, depth, MoveStack, level);
					}
				}

				return bestScore;
			}
			if (score > alpha) {
				bestMove = m;
				scoreType = ScoreType::Exact;
				alpha = score;
			}
		}
	}

	// There was no legal move --> return mate or stalemate score
	if (legalMoveCount == 0) {
		return inCheck ? LosingMateScore(level) : 0;
	}

	// Return the best score (fail-soft)
	if (!Aborting) Heuristics.AddTranspositionEntry(hash, Age, depth, bestScore, scoreType, bestMove, level);

	return bestScore;
}

// Quiescence search: for noisy moves only (captures, queen promotions)
int Search::SearchQuiescence(Board &board, const int level, int alpha, int beta) {

	// Check search limits
	Aborting = ShouldAbort();
	if (Aborting) return NoEval;

	// Update statistics
	Statistics.AlphaBetaCalls += 1;
	if (level > Statistics.SelDepth) Statistics.SelDepth = level;

	// Update alpha-beta bounds, return alpha if no captures left
	const int staticEval = Evaluate(board);
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

	// Generate noisy moves
	MoveList.clear();
	board.GenerateMoves(MoveList, MoveGen::Noisy, Legality::Pseudolegal);

	// Order noisy moves
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	for (const Move& m : MoveList) {
		const int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, EmptyMove, MoveStack, false, false);
		MoveOrder[level].push_back({ m, orderScore });
	}
	std::stable_sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Search recursively
	int bestScore = staticEval;
	int scoreType = ScoreType::UpperBound;
	for (const auto& [m, order] : MoveOrder[level]) {
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
		UpdateAccumulators<true>(m, movedPiece, capturedPiece);
		const int score = -SearchQuiescence(b, level + 1, -beta, -alpha);
		UpdateAccumulators<false>(m, movedPiece, capturedPiece);

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
	if (!Aborting) Heuristics.AddTranspositionEntry(hash, Age, 0, bestScore, scoreType, EmptyMove, level);
	return bestScore;
}

int Search::Evaluate(const Board &board) {
	Statistics.Evaluations += 1;
	//if (NeuralEvaluate2(Accumulator, board.Turn) != NeuralEvaluate(board)) cout << "!" << endl;
	return NeuralEvaluate2(Accumulator, board.Turn);
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
	uint64_t attackers = board.GetAttackersOfSquare(move.to, occupancy) & occupancy;

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

// Accumulators for neural networks ---------------------------------------------------------------

void Search::SetupAccumulators(const Board& board) {
	Accumulator.Reset();
	uint64_t bits = board.GetOccupancy();

	while (bits) {
		const uint8_t sq = Popsquare(bits);
		const int piece = board.GetPieceAt(sq);
		const int pieceType = TypeOfPiece(piece);
		const int pieceColor = ColorOfPiece(piece);
		const int colorOffset = 64 * 6;

		// Turn on the right inputs
		const int whiteActivationIndex = (pieceColor == PieceColor::White ? 0 : colorOffset) + (pieceType - 1) * 64 + sq;
		const int blackActivationIndex = (pieceColor == PieceColor::Black ? 0 : colorOffset) + (pieceType - 1) * 64 + Mirror(sq);
		for (int i = 0; i < HiddenSize; i++) Accumulator.White[i] += Network->FeatureWeights[whiteActivationIndex][i];
		for (int i = 0; i < HiddenSize; i++) Accumulator.Black[i] += Network->FeatureWeights[blackActivationIndex][i];
	}
}

template <bool push>
void Search::UpdateAccumulators(const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece) {

	// No longer activate the previous position of the moved piece
	Accumulator.UpdateFeature<!push>( FeatureIndexes(movedPiece, m.from) );

	// No longer activate the position of the captured piece (if any)
	if (capturedPiece != Piece::None) {
		Accumulator.UpdateFeature<!push>( FeatureIndexes(capturedPiece, m.to) );
	}

	// Activate the new position of the moved piece
	if (!m.IsPromotion()) {
		Accumulator.UpdateFeature<push>( FeatureIndexes(movedPiece, m.to) );
	}
	else {
		const uint8_t promotionPiece = m.GetPromotionPieceType() + (ColorOfPiece(movedPiece) == PieceColor::Black ? Piece::BlackPieceOffset : 0);
		Accumulator.UpdateFeature<push>( FeatureIndexes(promotionPiece, m.to) );
	}

	// Special cases
	switch (m.flag) {
		case MoveFlag::None: break;

		// Short castling: move the castling rook from H1 to F1 or from H8 to F8
		case MoveFlag::ShortCastle:
			if (ColorOfPiece(movedPiece) == PieceColor::White) {
				Accumulator.UpdateFeature<!push>( FeatureIndexes(Piece::WhiteRook, Squares::H1) );
				Accumulator.UpdateFeature<push>( FeatureIndexes(Piece::WhiteRook, Squares::F1) );
			}
			else {
				Accumulator.UpdateFeature<!push>( FeatureIndexes(Piece::BlackRook, Squares::H8) );
				Accumulator.UpdateFeature<push>( FeatureIndexes(Piece::BlackRook, Squares::F8) );
			}
			break;

		// Long castling: move the castling rook from A1 to D1 or from A8 to D8
		case MoveFlag::LongCastle:
			if (ColorOfPiece(movedPiece) == PieceColor::White) {
				Accumulator.UpdateFeature<!push>( FeatureIndexes(Piece::WhiteRook, Squares::A1) );
				Accumulator.UpdateFeature<push>( FeatureIndexes(Piece::WhiteRook, Squares::D1) );
			}
			else {
				Accumulator.UpdateFeature<!push>( FeatureIndexes(Piece::BlackRook, Squares::A8) );
				Accumulator.UpdateFeature<push>( FeatureIndexes(Piece::BlackRook, Squares::D8) );
			}
			break;

		// Treat en passant
		case MoveFlag::EnPassantPerformed:
			if (movedPiece == Piece::WhitePawn) Accumulator.UpdateFeature<!push>( FeatureIndexes(Piece::BlackPawn, m.to - 8) );
			else Accumulator.UpdateFeature<!push>( FeatureIndexes(Piece::WhitePawn, m.to + 8) );
			break;
	}

}

// Communicating the search results ---------------------------------------------------------------

void Search::PrintInfo(const Results& e, const EngineSettings& settings) const {
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

	/*
	std::string extended;
	if (settings.ExtendedOutput) {
		extended += " evals " + std::to_string(e.stats.Evaluations);
		extended += " qnodes " + std::to_string(e.stats.QuiescenceNodes);
		// extended += " betacutoffs " + std::to_string(e.stats.BetaCutoffs)
		// extended += " fmbc " + std::to_string(e.stats.FirstMoveBetaCutoffs);
		if (e.stats.BetaCutoffs != 0) extended += " fmbcrate " + std::to_string(static_cast<int>(e.stats.FirstMoveBetaCutoffs * 1000 / e.stats.BetaCutoffs));
		extended += " tthits " + std::to_string(e.stats.TranspositionHits);
		if (e.stats.TranspositionQueries != 0) extended += " ttrate " + std::to_string(static_cast<int>(e.stats.TranspositionHits * 1000 / e.stats.TranspositionQueries));
	}*/

	std::string pvString;
	for (const Move& move : e.pv)
		pvString += " " + move.ToString();

#if defined(_MSC_VER)
	std::string output = std::format("info depth {} seldepth {} score {} nodes {} nps {} time {} hashfull {} pv{}",
		e.depth, e.stats.SelDepth, score, e.stats.Nodes, e.nps, e.time, e.hashfull, pvString);
#else
	std::string output = "info depth " + std::to_string(e.depth) + " seldepth " + std::to_string(e.stats.SelDepth)
		+ " score " + score + " nodes " + std::to_string(e.stats.Nodes) + " nps " + std::to_string(e.nps)
		+ " time " + std::to_string(e.time) + " hashfull " + std::to_string(e.hashfull) + " pv" + pvString;
#endif

	cout << output << endl;
}

void Search::PrintPretty(const Results& e, const EngineSettings& settings) const {
#if defined(_MSC_VER)
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
		if (i >= static_cast<int>(e.pv.size())) break;
		pvOutput += e.pv.at(i).ToString() + " ";
	}
	if (pvOutput.length() != 0) pvOutput.pop_back();
	if (e.pv.size() > 5) pvOutput += " (+" + std::to_string(e.pv.size() - 5) + ")";
	if (e.pv.size() != 0) pvOutput = white + "[" + pvOutput + white + "]";

	std::string output = output1 + output3 + output2 + pvOutput + white;

	cout << output << endl;
#endif
}

void Search::PrintBestmove(const Move& move) const {
	cout << "bestmove " << move.ToString() << endl;
}