#include "Search.h"

// This is the main search code of Renegade. If you're reading this, you're probably interested in
// what this engine does under the hood, and I'm happy for that, feel free to try some ideas from here!

Search::Search() {
	constexpr double lmrMultiplier = 0.40;
	constexpr double lmrBase = 0.76;
	for (int i = 1; i < 32; i++) {
		for (int j = 1; j < 32; j++) {
			LMRTable[i][j] = static_cast<int>(lmrMultiplier * std::log(i) * std::log(j) + lmrBase);
		}
	}
	StartThreads(1);
}

void ThreadData::ResetStatistics() {
	RootDepth = 0;
	SelDepth = 0;
	Nodes = 0;
}

void Search::ResetState(const bool clearTT) {
	for (ThreadData& t : Threads) t.History.ClearAll();
	if (clearTT) TranspositionTable.Clear(Settings::Threads);
}

void Search::StartThreads(const int threadCount) {
	assert(Threads.size() == 0);
	LoadedThreadCount.store(0);
	for (int i = 0; i < threadCount; i++) {
		ThreadData& t = Threads.emplace_back();
		t.threadId = i;
		t.Thread = std::thread([&] { Loop(t); });
	}
	while (LoadedThreadCount.load() < static_cast<int>(Threads.size())) {};
}

void Search::StopThreads() {
	StopSearch();
	for (ThreadData& t : Threads) {
		std::unique_lock<std::mutex> lock(t.Mutex);
		t.Action = ThreadAction::Exit;
		lock.unlock();
		t.CondVar.notify_one();
	}
	for (ThreadData& t : Threads) t.Thread.join();
	Threads.clear();
}

void Search::SetThreadCount(const int threadCount) {
	StopThreads();
	StartThreads(threadCount);
}

Results Search::SearchSinglethreaded(const Position& pos, const SearchParams& params) {
	StartSearchTime = Clock::now();
	Aborting.store(false);
	TranspositionTable.IncreaseAge();
	ThreadData& t = Threads.front();
	t.singlethreaded = true;
	t.CurrentPosition = pos;
	t.result = {};
	t.ResetStatistics();
	Constraints = CalculateConstraints(params, pos.Turn());

	SearchMoves(t);
	t.singlethreaded = false;
	return t.result;
}

void Search::StartSearch(Position& position, const SearchParams params) {

	StartSearchTime = Clock::now();
	TranspositionTable.IncreaseAge();

	MoveList rootLegalMoves{};
	position.GenerateMoves(rootLegalMoves, MoveGen::All, Legality::Legal);

	// Handle no legal moves
	if (rootLegalMoves.size() == 0) {
		cout << "info string No legal moves!" << endl;
		PrintBestmove(NullMove);
		return;
	}

	Constraints = CalculateConstraints(params, position.Turn());

	// Reduce time for one legal move
	if (rootLegalMoves.size() == 1 && (params.wtime != 0 || params.btime != 0)) {
		cout << "info string Only one legal move, time allocated for search is reduced" << endl;
		Constraints.SearchTimeMin = std::min(Constraints.SearchTimeMin, 200);
		Constraints.SearchTimeMax = std::min(Constraints.SearchTimeMax, 2000);
	}

	// Fire up the threads
	Aborting.store(false);
	ActiveThreadCount.store(Threads.size());
	for (ThreadData& t : Threads) {
		t.CurrentPosition = position;
		t.result = {};
		t.ResetStatistics();
	}
	for (ThreadData& t : Threads) {
		std::unique_lock<std::mutex> lock(t.Mutex);
		t.Action = ThreadAction::Search;
		lock.unlock();
		t.CondVar.notify_one();
	}
}

void Search::StopSearch() {
	Aborting.store(true);
	WaitUntilReady();
}

void Search::Loop(ThreadData& t) {
	LoadedThreadCount.fetch_add(1);

	while (true) {

		std::unique_lock<std::mutex> lock(t.Mutex);
		t.CondVar.wait(lock, [&] { return t.Action != ThreadAction::Sleep; });

		if (t.Action == ThreadAction::Exit) break;
		else {
			SearchMoves(t);
			if (t.IsMainThread()) PrintBestmove(t.result.BestMove());
		}

		t.Action = ThreadAction::Sleep;
		ActiveThreadCount.fetch_sub(1);
		t.CondVar.notify_one();
	}

	std::unique_lock<std::mutex> lock(t.Mutex);
	t.Exited = true;
	lock.unlock();
	t.CondVar.notify_all();
}

void Search::WaitUntilReady() {
	for (ThreadData& t : Threads) {
		std::unique_lock<std::mutex> lock(t.Mutex);
		t.CondVar.wait(lock, [&] { return t.Action == ThreadAction::Sleep; });
	}
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
	if (constraints.MaxDepth != -1 || constraints.MaxNodes != -1) return constraints;
	if (constraints.SearchTimeMin != -1 || constraints.SearchTimeMax != -1) return constraints;

	// Handle wtime, btime, winc, binc
	const int myTime = turn ? params.wtime : params.btime;
	const int myInc = turn ? params.winc : params.binc;
	if (myTime != 0) {
		int minTime, maxTime;

		if (params.movestogo > 0) {
			// Repeating time control
			minTime = static_cast<int>(myTime / params.movestogo * 0.5);
			maxTime = static_cast<int>(myTime / params.movestogo * 2.5);
			maxTime = std::min(maxTime, static_cast<int>(myTime * 0.8));
		}
		else {
			// Time control with increment
			minTime = static_cast<int>(myTime * 0.025 + myInc * 0.7);
			maxTime = static_cast<int>(myTime * 0.25);
		}

		constraints.SearchTimeMin = std::min(minTime, maxTime);
		constraints.SearchTimeMax = maxTime;
		return constraints;
	}

	// Default: go infinite
	return constraints;
}

bool Search::ShouldAbort(const ThreadData& t) {
	if (Aborting.load(std::memory_order_relaxed) && (t.RootDepth > 1 || !t.IsMainThread())) return true;
	if (!t.IsMainThread()) return false; // TODO: ensure limits when multithreaded

	if (Constraints.MaxNodes != -1 && t.Nodes >= Constraints.MaxNodes && t.RootDepth > 1) {
		Aborting.store(true, std::memory_order_relaxed);
		return true;
	}
	if (t.Nodes % 1024 == 0 && Constraints.SearchTimeMax != -1 && t.RootDepth > 1) {
		const auto now = Clock::now();
		const int elapsedMs = static_cast<int>((now - StartSearchTime).count() / 1e6);
		if (elapsedMs >= Constraints.SearchTimeMax) {
			Aborting.store(true, std::memory_order_relaxed);
			return true;
		}
	}
	return false;
}

// Alpha-beta search routine and handling ---------------------------------------------------------

void Search::SearchMoves(ThreadData& t) {

	// Reset before starting (takes a fraction of a millisecond)
	t.result = {};
	t.ResetStatistics();
	t.ResetPvTable();
	std::fill(t.ExcludedMoves.begin(), t.ExcludedMoves.end(), NullMove);
	std::fill(t.SuperSingular.begin(), t.SuperSingular.end(), false);
	std::fill(t.CutoffCount.begin(), t.CutoffCount.end(), 0);
	std::memset(&t.RootNodeCounts, 0, sizeof(t.RootNodeCounts));
	t.History.ClearRefutations();
	t.EvalState.Reset(t.CurrentPosition);

	// Iterative deepening
	t.result.ply = t.CurrentPosition.GetPly();
	int score = NoEval;
	bool finished = false;

	while (!finished) {
		t.ResetPvTable();
		t.RootDepth += 1;
		t.SelDepth = 0;

		// Obtain score
		if (t.RootDepth < 5) {
			// Regular negamax for shallow depths
			score = SearchRecursive<true>(t, t.RootDepth, 0, NegativeInfinity, PositiveInfinity, false);
		}
		else {
			// Aspiration windows
			int windowSize = 12;
			int searchDepth = t.RootDepth;

			while (true) {
				if (Aborting.load(std::memory_order_relaxed)) break;
				int alpha, beta;
				if (windowSize < 500) {
					alpha = std::max(score - windowSize, NegativeInfinity);
					beta = std::min(score + windowSize, PositiveInfinity);
				}
				else {
					alpha = NegativeInfinity;
					beta = PositiveInfinity;
				}

				//if (!settings.UciOutput) cout << "[" << alpha << ".." << beta << "] ";

				score = SearchRecursive<true>(t, searchDepth, 0, alpha, beta, false);

				if (score <= alpha) {
					alpha = std::max(alpha - windowSize, NegativeInfinity);
					beta = (alpha + beta) / 2;
					searchDepth = t.RootDepth;
				}
				else if (score >= beta) {
					beta = std::min(beta + windowSize, PositiveInfinity);
					
					// Reduce depth on fail-high
					if (!IsMateScore(score) && searchDepth > 1) searchDepth -= 1;
				}
				else {
					// Success!
					break;
				}

				windowSize += windowSize / 3;
			}

		}

		// Check search limits on the main thread
		const auto currentTime = Clock::now();
		const int elapsedMs = static_cast<int>((currentTime - StartSearchTime).count() / 1e6);
		if (t.IsMainThread() && Constraints.SearchTimeMin != -1) {
			int softTimeLimit = Constraints.SearchTimeMin;
			if (Constraints.SearchTimeMin != Constraints.SearchTimeMax) {
				const Move& bestMove = t.PvTable[0][0];
				const double bestMoveFraction = t.RootNodeCounts[bestMove.from][bestMove.to] / static_cast<double>(t.Nodes);
				softTimeLimit = softTimeLimit * (t.RootDepth >= 10 ? (1.5 - bestMoveFraction) * 1.35 : 1.0);
			}
			if (elapsedMs >= softTimeLimit) finished = true;				
		}

		if (t.RootDepth >= Constraints.MaxDepth && Constraints.MaxDepth != -1) finished = true;
		if (t.RootDepth >= MaxDepth) finished = true;
		if (t.Nodes >= Constraints.SoftNodes && Constraints.SoftNodes != -1) finished = true;

		const bool aborting = Aborting.load(std::memory_order_relaxed);
		if (aborting && !t.singlethreaded && t.RootDepth > 1) {
			t.result.nodes = t.Nodes;
			t.result.time = elapsedMs;
			t.result.nps = static_cast<uint64_t>(t.Nodes * 1e9 / (currentTime - StartSearchTime).count());
			t.result.hashfull = TranspositionTable.GetHashfull();
			break;
		}

		// Save info
		if (!t.singlethreaded || !aborting) {
			t.result.score = score;
			t.result.depth = t.RootDepth;
			t.result.pv = t.GeneratePvLine();
		}
		t.result.seldepth = t.SelDepth;
		t.result.nodes = t.Nodes;
		t.result.time = elapsedMs;
		t.result.nps = static_cast<int>(t.Nodes * 1e9 / (currentTime - StartSearchTime).count());
		t.result.hashfull = TranspositionTable.GetHashfull();

		// Display search information
		if (t.IsMainThread() && !t.singlethreaded) {
			if (!finished) PrintInfo(AggregateThreadResults());
		}
	}

	// Main thread should wait others finishing before displaying the final best move
	if (t.IsMainThread() && !t.singlethreaded) {
		Aborting.store(true);
		while (ActiveThreadCount.load() > 1) {};
		PrintInfo(AggregateThreadResults());
	}

}

Results Search::AggregateThreadResults() const {
	Results sumResult{};

	// Values from the main
	sumResult.depth = Threads.front().result.depth;
	sumResult.score = Threads.front().result.score;
	sumResult.ply = Threads.front().result.ply;
	sumResult.pv = Threads.front().result.pv;

	// Values from multiple threads
	for (const ThreadData& t : Threads) sumResult.seldepth = std::max(sumResult.seldepth, t.SelDepth);
	for (const ThreadData& t : Threads) sumResult.nodes += t.Nodes;

	// Other data
	const auto currentTime = Clock::now();
	const uint64_t elapsedNs = std::max((currentTime - StartSearchTime).count(), int64_t{1});
	const uint64_t elapsedMs = elapsedNs / 1'000'000;
	sumResult.time = elapsedMs;
	sumResult.nps = static_cast<uint64_t>(sumResult.nodes * 1e9 / elapsedNs);
	sumResult.hashfull = TranspositionTable.GetHashfull();
	sumResult.threads = Threads.size();

	return sumResult;
}

// Main search function for a node, recursively called during the alpha-beta search
template<bool pvNode>
int Search::SearchRecursive(ThreadData& t, int depth, const int level, int alpha, int beta, const bool cutNode) {

	Position& position = t.CurrentPosition;
	const bool rootNode = (level == 0);
	assert(pvNode || beta - alpha == 1);
	assert(!(rootNode && !pvNode));
	assert(!pvNode || !cutNode);

	// Check search limits
	if (ShouldAbort(t)) return NoEval;
	t.InitPvLength(level);
	if (level >= MaxDepth) return Evaluate(t, position);
	if (level > t.SelDepth) t.SelDepth = level;

	// Mate distance pruning
	if (!rootNode) {
		alpha = std::max(alpha, -MateEval + level);
		beta = std::min(beta, MateEval - level - 1);
		if (alpha >= beta) return alpha;
	}

	// Check for draws
	if (!rootNode && position.IsDrawn(level)) return DrawEvaluation(t);

	// Check extensions
	const bool inCheck = position.IsInCheck();
	if (!rootNode && inCheck) depth += 1;

	// Drop into quiescence search at depth 0
	if (depth <= 0) {
		return SearchQuiescence<pvNode>(t, level, alpha, beta);
	}

	const Move excludedMove = t.ExcludedMoves[level];
	const bool singularSearch = !excludedMove.IsNull();
	MovePicker& movePicker = t.MovePickerStack[level];

	// Probe the transposition table
	TranspositionEntry ttEntry;
	int ttEval = NoEval;
	Move ttMove = NullMove;
	bool found = false;
	const uint64_t hash = position.Hash();

	if (!singularSearch) {
		found = TranspositionTable.Probe(hash, ttEntry, level);
		if (found) {
			if constexpr (!pvNode) {
				// The branch was already analyzed to the same or greater depth, so we can return the result if the score is alright
				if (ttEntry.IsCutoffPermitted(depth, alpha, beta)) return ttEntry.score;
			}
			ttEval = ttEntry.score;
			ttMove = Move(ttEntry.packedMove);
		}
	}

	const bool singularCandidate = found && !rootNode && !singularSearch && (depth > 7)
		&& (ttEntry.depth >= depth - 3) && (ttEntry.scoreType != ScoreType::UpperBound) && !IsMateScore(ttEval);
	const bool ttPV = pvNode || (found && ttEntry.ttPv);
	
	// Obtain the evaluation of the position
	int rawEval = NoEval;
	int staticEval = NoEval;
	int eval = NoEval;

	if (!singularSearch) {
		rawEval = [&] {
			if (inCheck) return static_cast<int16_t>(NoEval);
			if (found) return ttEntry.rawEval;
			return static_cast<int16_t>(Evaluate(t, position));
		}();
		staticEval = t.History.ApplyCorrection(position, rawEval);
		eval = staticEval;

		if (ttEval != NoEval) {
			if (ttEntry.scoreType == ScoreType::Exact
				|| (ttEntry.scoreType == ScoreType::LowerBound && staticEval < ttEval)
				|| (ttEntry.scoreType == ScoreType::UpperBound && staticEval > ttEval)) eval = ttEval; // can't be true when in check
		}
		t.StaticEvalStack[level] = staticEval;
		t.EvalStack[level] = eval;
		t.SuperSingular[level] = false;
	}
	else {
		staticEval = t.StaticEvalStack[level];
		eval = t.EvalStack[level];
	}

	const bool improving = (level >= 2) && !inCheck && (t.StaticEvalStack[level] > t.StaticEvalStack[level - 2]);
	bool futilityPrunable = false;

	// Whole-node pruning techniques
	if (!pvNode && !inCheck && !singularSearch) {

		// Reverse futility pruning
		if (depth <= 7 && !IsMateScore(beta)) {
			const int rfpMargin = depth * 103 - improving * 86;
			if (eval - rfpMargin > beta) return (eval + beta) / 2;
		}

		// Null-move pruning
		if (depth >= 3 && eval >= beta && !position.IsPreviousMoveNull() && position.ZugzwangUnlikely()) {
			TranspositionTable.Prefetch(position.Hash() ^ Zobrist.SideToMove);
			const int nmpReduction = [&] {
				const int defaultReduction = 4 + depth / 3 + std::min((eval - beta) / 241, 3);
				return std::min(defaultReduction, depth);
			}();
			position.PushNullMove();
			t.EvalState.PushState(position, NullMove, Piece::None, Piece::None);
			const int nmpScore = -SearchRecursive<false>(t, depth - nmpReduction, level + 1, -beta, -beta + 1, !cutNode);
			position.PopMove();
			t.EvalState.PopState();
			if (nmpScore >= beta) {
				return IsMateScore(nmpScore) ? beta : nmpScore;
			}
		}

		// Futility pruning
		if (depth <= 5 && !IsMateScore(beta)) {
			const int futilityMargin = 45 + depth * 112 + improving * 44;
			futilityPrunable = (eval + futilityMargin < alpha);
		}
	}

	// Internal iterative reductions
	if (depth >= 6 && ttMove.IsNull() && !singularSearch) {
		depth -= 1;
	}

	// Initialize variables, generate and order moves (in singular search we've already done these)
	if (!singularSearch) {
		movePicker.Initialize(MoveGen::All, position, t.History, ttMove, level);

		// Resetting killers and fail-high cutoff counts
		if (level + 2 < MaxDepth) t.History.ResetKillerForPly(level + 2);
		if (level + 1 < MaxDepth) t.CutoffCount[level + 1] = 0;
	}
	
	// Iterate through legal moves
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	int failLowCount = 0;
	int failHighCount = 0;
	Move bestMove = NullMove;
	int bestScore = NegativeInfinity;

	StaticVector<Move, MaxMoveCount> quietsTried;
	StaticVector<Move, MaxMoveCount> capturesTried;

	while (movePicker.HasNext()) {
		const auto& [m, order] = movePicker.Get();
		if (m == excludedMove) continue;
		if (!position.IsLegalMove(m)) continue;
		legalMoveCount += 1;
		const bool isQuiet = position.IsMoveQuiet(m);

		if (isQuiet) quietsTried.push(m);
		else capturesTried.push(m);

		// Moves loop pruning techniques
		if (!pvNode && !IsLosingMateScore(bestScore)) {

			// Late-move pruning
			if (depth <= 4 && isQuiet && !inCheck) {
				const int lmpCount = 3 + depth * (depth - !improving);
				if (legalMoveCount > lmpCount) break;
			}

			// Performing futility pruning
			if (futilityPrunable && isQuiet && order < 32768 && alpha < MateThreshold) {
				bestScore = (bestScore + alpha) / 2;
				break;
			}

			// Main search SEE pruning
			if (position.IsSquareThreatened(m.to)) {
				const int seeMargin = isQuiet ? (-50 * depth) : (-100 * depth);
				if (!position.StaticExchangeEval(m, seeMargin)) continue;
			}
		}

		// Singular extensions
		int extension = 0;
		if (singularCandidate && m == ttMove && level < t.RootDepth * 2) {
			const int singularMargin = depth * 2;
			const int singularBeta = std::max(ttEval - singularMargin, -MateEval);
			const int singularDepth = (depth - 1) / 2;
			t.ExcludedMoves[level] = m;
			const int singularScore = SearchRecursive<false>(t, singularDepth, level, singularBeta - 1, singularBeta, cutNode);
			t.ExcludedMoves[level] = NullMove;
			t.MovePickerStack[level].index = 1;
				
			if (singularScore < singularBeta) {
				// Successful extension
				const bool doubleExtend = (!pvNode && (singularScore < singularBeta - 23)) || t.SuperSingular[level];
				extension = 1 + doubleExtend;
			}
			else {
				// Extension check failed
				if (!pvNode && singularBeta >= beta) return singularBeta;
				else if (cutNode) extension = -1;
			}
		}

		// Push move
		const uint8_t movedPiece = position.GetPieceAt(m.from);
		const uint8_t capturedPiece = position.GetPieceAt(m.to);
		const uint64_t nodesBefore = t.Nodes;

		TranspositionTable.Prefetch(position.ApproximateHashAfterMove(m));
		position.PushMove(m);
		t.Nodes += 1;
		int score = NoEval;
		failHighCount = 0;
		t.EvalState.PushState(position, m, movedPiece, capturedPiece);
		bool deepen = false;

		// Principal variation search & late-move reductions
		if (depth >= 3 && (legalMoveCount >= (3 + pvNode * 2 + rootNode * 2)) && isQuiet) {
			
			int reduction = LMRTable[std::min(depth, 31)][std::min(failLowCount, 31)];
			if (!ttPV) reduction += 1;
			if (t.CutoffCount[level] < 4) reduction -= 1;
			if (std::abs(order) < MovePicker::MaxRegularQuietOrder) reduction -= std::clamp(order / 20100, -2, 2);
			if (cutNode) reduction += 1;
			if (improving) reduction -= 1;
			reduction = std::max(reduction, 0);

			const int reducedDepth = std::clamp(depth - 1 - reduction, 0, depth - 1);
			score = -SearchRecursive<false>(t, reducedDepth, level + 1, -alpha - 1, -alpha, true);
			failHighCount += (score > alpha);

			if (score > alpha && reducedDepth < depth - 1) {
				deepen = score > (bestScore + 32 + depth * 5);
				score = -SearchRecursive<false>(t, depth - 1 + deepen, level + 1, -alpha - 1, -alpha, !cutNode);
				failHighCount += (score > alpha);
			}
		}
		else if (!pvNode || legalMoveCount > 1) {
			score = -SearchRecursive<false>(t, depth - 1 + extension, level + 1, -alpha - 1, -alpha, !cutNode);
			failHighCount += (score > alpha);
		}

		if (pvNode && (legalMoveCount == 1 || score > alpha)) {
			score = -SearchRecursive<true>(t, depth - 1 + extension + deepen, level + 1, -beta, -alpha, false);
			failHighCount += (score >= beta);
		}

		position.PopMove();
		t.EvalState.PopState();

		// Update node count table for the root, this is used for time management
		if (rootNode) t.RootNodeCounts[m.from][m.to] += t.Nodes - nodesBefore;

		failLowCount += (score <= alpha);

		// If the score beats our previous best, check if it raises alpha or causes a fail-high
		if (score > bestScore) {

			bestScore = score;
			if (pvNode && !ShouldAbort(t)) t.UpdatePvTable(m, level);

			// Raise alpha
			if (score > alpha) {
				bestMove = m;
				scoreType = ScoreType::Exact;
				alpha = score;
			}

			// Fail-high
			if (score >= beta) {
				scoreType = ScoreType::LowerBound;
				break;
			}
		}
	}

	// There was no legal move --> return mate or stalemate score
	if (legalMoveCount == 0) {
		if (singularSearch) {
			t.SuperSingular[level] = true;
			return alpha; // always extend if we have only one legal move
		}
		return inCheck ? LosingMateScore(level) : 0;
	}

	const bool aborting = ShouldAbort(t);

	// Update search history and statistics when having a cutoff
	if (bestScore >= beta && !aborting) {

		if (level != 0) t.CutoffCount[level - 1] += 1;
		const bool quietBestMove = position.IsMoveQuiet(bestMove);

		// Increment history scores for the move causing the cutoff 
		if (quietBestMove) {
			t.History.UpdateQuietHistory<Bonus>(position, bestMove, level, depth, failHighCount);
			t.History.SetKillerMove(bestMove, level);
			t.History.SetPositionalMove(position, bestMove);
			if (level > 0) t.History.SetCountermove(position.GetPreviousMove(1).move, bestMove);
		}
		else {
			t.History.UpdateCaptureHistory<Bonus>(position, bestMove, depth, failHighCount);
		}

		// Decrement history scores for all previously tried moves
		if (quietBestMove) quietsTried.pop(); // don't decrement for the current move
		else capturesTried.pop();

		for (const Move& qt : quietsTried) t.History.UpdateQuietHistory<Penalty>(position, qt, level, depth, 1);
		for (const Move& ct : capturesTried) t.History.UpdateCaptureHistory<Penalty>(position, ct, depth, 1);
	}

	// Update evaluation correction history
	if (!aborting && !singularSearch) {
		const bool updateCorrection = [&] {
			if (inCheck) return false;
			if (!bestMove.IsNull() && !position.IsMoveQuiet(bestMove)) return false;
			return (scoreType == ScoreType::Exact)
				   || (scoreType == ScoreType::UpperBound && bestScore < staticEval)
				   || (scoreType == ScoreType::LowerBound && bestScore > staticEval);
		}();
		if (updateCorrection) t.History.UpdateCorrection(position, (rawEval + staticEval) / 2, bestScore, depth);
	}

	// Store node search results into the transposition table
	if (!aborting && !singularSearch) {
		TranspositionTable.Store(hash, depth, bestScore, scoreType, rawEval, bestMove, level, ttPV);
	}

	// Return the best score (fail-soft)
	return bestScore;
}

// Quiescence search: for noisy moves only (captures, queen promotions)
template <bool pvNode>
int Search::SearchQuiescence(ThreadData& t, const int level, int alpha, int beta) {

	Position& position = t.CurrentPosition;
	assert(pvNode || beta - alpha == 1);

	// Check search limits
	if (ShouldAbort(t)) return NoEval;
	if (level > t.SelDepth) t.SelDepth = level;

	// Probe the transposition table
	const uint64_t hash = position.Hash();
	TranspositionEntry ttEntry;
	const bool found = TranspositionTable.Probe(hash, ttEntry, level);
	if (!pvNode && found && ttEntry.IsCutoffPermitted(0, alpha, beta)) return ttEntry.score;
	Move ttMove = NullMove;
	if (found) ttMove = Move(ttEntry.packedMove);
	const bool ttPV = pvNode || (found && ttEntry.ttPv);

	// Get node evaluation
	const bool inCheck = position.IsInCheck();
	const int rawEval = [&] {
		if (inCheck) return static_cast<int16_t>(NoEval);
		if (found) return ttEntry.rawEval;
		return static_cast<int16_t>(Evaluate(t, position));
	}();
	const int staticEval = (!inCheck) ? t.History.ApplyCorrection(position, rawEval) : LosingMateScore(level);

	// Check alpha-beta bounds
	if (staticEval >= beta) return staticEval;
	if (staticEval > alpha) alpha = staticEval;

	if (level >= MaxDepth) return inCheck ? DrawEvaluation(t) : staticEval;
	if (position.IsDrawn(level)) return DrawEvaluation(t);

	// Generate noisy moves and order them (in check we generate quiets as well)
	MovePicker& movePicker = t.MovePickerStack[level];
	movePicker.Initialize(inCheck ? MoveGen::All : MoveGen::Noisy, position, t.History, ttMove, level);

	// Search recursively until the position is quiet
	int bestScore = staticEval;
	Move bestMove = NullMove;
	int scoreType = ScoreType::UpperBound;
	int futilityScore = std::min(staticEval + 274, MateThreshold - 1);

	while (movePicker.HasNext()) {
		const auto& [m, order] = movePicker.Get();
		if (!position.IsLegalMove(m)) continue;

		// When in check, no longer search quiet moves once we know we're not getting mated
		if (inCheck && bestScore > -MateThreshold && order < MovePicker::MaxRegularQuietOrder) break;

		// Quiescence search SEE pruning
		if (bestScore > -MateThreshold && !position.StaticExchangeEval(m, 0)) continue;

		// Quiescence search futility pruning
		if (!inCheck && alpha >= futilityScore && !position.StaticExchangeEval(m, 1)) { 
			if (bestScore < futilityScore) bestScore = futilityScore;
			continue;
		}

		t.Nodes += 1;
		const uint8_t movedPiece = position.GetPieceAt(m.from);
		const uint8_t capturedPiece = position.GetPieceAt(m.to);
		TranspositionTable.Prefetch(position.ApproximateHashAfterMove(m));
		position.PushMove(m);
		t.EvalState.PushState(position, m, movedPiece, capturedPiece);
		const int score = -SearchQuiescence<pvNode>(t, level + 1, -beta, -alpha);
		position.PopMove();
		t.EvalState.PopState();

		if (score > bestScore) {
			bestScore = score;
			if (bestScore >= beta) {
				scoreType = ScoreType::LowerBound;
				bestMove = m;
				break;
			}
			if (bestScore > alpha) {
				alpha = bestScore;
				bestMove = m;
			}
		}
	}
	if (!ShouldAbort(t)) TranspositionTable.Store(hash, 0, bestScore, scoreType, rawEval, bestMove, level, ttPV);
	return bestScore;
}

int16_t Search::Evaluate(ThreadData& t, const Position& position) {
	return t.EvalState.Evaluate(position);
}

int Search::DrawEvaluation(const ThreadData& t) const {
	// Returns a small randomized score to avoid search getting stuck in threefold lines
	return t.Nodes % 4 - 2;
}

// PV table ---------------------------------------------------------------------------------------

void ThreadData::InitPvLength(const int level) {
	PvLength[level] = level;
}

void ThreadData::UpdatePvTable(const Move& move, const int level) {
	PvTable[level][level] = move;
	for (int nextLevel = level + 1; nextLevel < PvLength[level + 1]; nextLevel++) {
		PvTable[level][nextLevel] = PvTable[level + 1][nextLevel];
	}
	PvLength[level] = PvLength[level + 1];
}

std::vector<Move> ThreadData::GeneratePvLine() const {
	std::vector<Move> list;
	list.reserve(PvLength[0]);

	for (int i = 0; i < PvLength[0]; i++) {
		const Move& m = PvTable[0][i];
		if (m.IsNull()) break;
		list.push_back(m);
	}
	return list;
}

void ThreadData::ResetPvTable() {
	std::memset(&PvTable, 0, sizeof(PvTable));
	std::fill(PvLength.begin(), PvLength.end(), 0);
}

// Perft methods ----------------------------------------------------------------------------------

void Search::Perft(Position& position, const int depth, const PerftType type) const {
	const bool isStartpos = position.Hash() == 0x463b96181691fc9c;
	constexpr std::array<uint64_t, 8> startposPerfts = { 1, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860 };

	const auto startTime = Clock::now();
	const uint64_t r = PerftRecursive(position, depth, depth, type);
	const auto endTime = Clock::now();

	const float seconds = static_cast<float>((endTime - startTime).count() / 1e9);
	const float speed = r / seconds / 1000000;
	cout << "-> Perft(" << depth << ") = " << Console::FormatInteger(r) << " | "
		<< std::setprecision(2) << std::fixed << seconds << " s | "
		<< std::setprecision(3) << speed << " mnps | No bulk counting" << endl;

	if (isStartpos && depth < static_cast<int>(startposPerfts.size()) && startposPerfts[depth] != r)
		cout << "-> Uh-oh. (expected: " << Console::FormatInteger(startposPerfts[depth]) << ")" << endl;
}

uint64_t Search::PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const {
	MoveList moves{};
	position.GenerateMoves(moves, MoveGen::All, Legality::Pseudolegal);

	if (type == PerftType::PerftDiv && originalDepth == depth) cout << "-> Legal moves (" << moves.size() << "): " << endl;
	uint64_t count = 0;
	for (const auto& m : moves) {
		if (!position.IsLegalMove(m.move)) continue;
		uint64_t r;
		if (depth == 1) {
			r = 1;
			count += 1;
		}
		else {
			position.PushMove(m.move);
			r = PerftRecursive(position, depth - 1, originalDepth, type);
			position.PopMove();
			count += r;
		}
		if (originalDepth == depth && type == PerftType::PerftDiv)
			cout << " - " << m.move.ToString(Settings::Chess960) << " : " << r << endl;
	}
	return count;
}
