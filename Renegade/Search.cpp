#include "Search.h"

Search::Search() {
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

const void Search::Perft(Board &board, const int depth, const PerftType type) {
	Board b = board.Copy();
	bool startingPosition = b.Hash() == 0x463b96181691fc9c;
	const uint64_t startingPerfts[] = { 1, 20, 400, 8902, 197281, 4865609, 119060324 };

	auto t0 = Clock::now();
	const int r = PerftRecursive(b, depth, depth, type);
	auto t1 = Clock::now();

	const float seconds = (float)((t1 - t0).count() / 1e9);
	const float speed = r / seconds / 1000000;
	if ((type == PerftType::Normal) || (type == PerftType::PerftDiv)) {
		cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps | No bulk counting" << endl;
		if (startingPosition && (depth <= 6) && (startingPerfts[depth] != r)) cout << "Uh-oh. (expected: " << startingPerfts[depth] << ")" << endl;
	}
	else cout << r << endl;
}

const int Search::PerftRecursive(Board &board, const int depth, const int originalDepth, const PerftType type) {
	std::vector<Move> moves;
	board.GenerateLegalMoves(moves, board.Turn);
	if ((type == PerftType::PerftDiv) && (originalDepth == depth)) cout << "Legal moves (" << moves.size() << "): " << endl;
	int count = 0;
	for (Move m : moves) {
		int r;
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
	int myTime = turn ? params.wtime : params.btime;
	int myInc = turn ? params.winc : params.binc;
	if (myTime != 0) {

		int maxTime;
		int minTime;
		if (params.movestogo > 0) {
			// Repeating time control
			maxTime = static_cast<int>((myTime / params.movestogo * 2));
			minTime = static_cast<int>((myTime / params.movestogo * 0.3));
			maxTime = std::min(maxTime, myTime / 2);
		}
		else {
			// Sudden death 
			maxTime = static_cast<int>(myTime * 0.333);
			minTime = static_cast<int>((myTime + myInc * 10.0) * 0.015);
		}

		constraints.SearchTimeMax = maxTime;
		constraints.SearchTimeMin = minTime;
		if (constraints.SearchTimeMin > constraints.SearchTimeMax) constraints.SearchTimeMin = constraints.SearchTimeMax;
		return constraints;
	}

	// Default: go infinite
	return constraints;
}

// Negamax search routine and handling ------------------------------------------------------------

Results Search::SearchMoves(Board &board, SearchParams params, EngineSettings settings) {

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
			cout << "bestmove " << bookMove << endl;
			return e;
		}
	}

	// Iterative deepening
	Results e = Results();
	while (!finished) {
		FollowingPV = true;
		Heuristics.ClearEntries();
		Depth += 1;
		Statistics.SelDepth = 0;
		int result = SearchRecursive(board, Depth, 0, NegativeInfinity, PositiveInfinity, true);

		// Check limits
		auto currentTime = Clock::now();
		elapsedMs = static_cast<int>((currentTime - StartSearchTime).count() / 1e6);
		if ((elapsedMs >= Constraints.SearchTimeMin) && (Constraints.SearchTimeMin != -1)) finished = true;
		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if ((Depth >= 32)) finished = true;
		if (Aborting) {
			e.stats = Statistics;
			e.time = elapsedMs;
			e.nps = static_cast<int>(Statistics.Nodes * 1e9 / (currentTime - StartSearchTime).count());
			e.hashfull = Heuristics.GetHashfull();
			PrintInfo(e, settings);
			break;
		}

		// Send info
		e.score = result;
		e.depth = Depth;
		e.stats = Statistics;
		e.time = elapsedMs;
		e.nps = static_cast<int>(Statistics.Nodes * 1e9 / (currentTime - StartSearchTime).count());
		e.hashfull = Heuristics.GetHashfull();

		// Check PV validity (note & todo: may not be correct)
		e.pv.clear();
		std::vector<Move> returnedPVs = Heuristics.GeneratePvLine();
		Board b = board.Copy();
		for (const Move& m : returnedPVs) {
			std::vector<Move> legalMoves;
			b.GenerateLegalMoves(legalMoves, b.Turn);
			if (std::find(legalMoves.begin(), legalMoves.end(), m) != legalMoves.end()) {
				e.pv.push_back(m);
				b.Push(m);
			}
			else break;
		}

		Heuristics.SetPvLine(e.pv);
		PrintInfo(e, settings);
	}
	PrintBestmove(e.BestMove());

	Heuristics.ClearEntries();
	Heuristics.ClearPvLine();
	Heuristics.ClearHistoryTable();
	Aborting = true;
	return e;
}

// Recursively called during the negamax search
int Search::SearchRecursive(Board &board, int depth, int level, int alpha, int beta, bool canNullMove) {

	// Check limits
	if (Aborting) return NoEval;
	if ((Constraints.MaxNodes != -1) && (Statistics.Nodes >= Constraints.MaxNodes) && (Depth > 1)) {
		Aborting = true;
		return NoEval;
	}
	if ((Statistics.Nodes % 1024 == 0) && (Constraints.SearchTimeMax != -1) && (Depth > 1)) {
		auto now = Clock::now();
		int elapsedMs = (int)((now - StartSearchTime).count() / 1e6);
		if (elapsedMs >= Constraints.SearchTimeMax) {
			Aborting = true;
			return NoEval;
		}
	}

	Statistics.Nodes += 1;

	// Check extensions
	uint64_t kingBits = board.Turn == Turn::White ? board.WhiteKingBits : board.BlackKingBits;
	bool inCheck = (board.AttackedSquares & kingBits) != 0;
	if (inCheck && (depth == 0) && (level < Depth + 10)) depth = 1;
	bool pvNode = beta - alpha > 1;

	// Check for draws
	if (board.IsDraw()) return 0;

	// Return result for terminal nodes
	if (depth <= 0) {
		int e = SearchQuiescence(board, level, alpha, beta, true);
		//if (depth < 0) cout << "Check depth: " << depth << endl;
		//Heuristics.AddEntry(hash, e, ScoreType::Exact);
		return e;
	}

	// Calculate hash and probe transposition table
	uint64_t hash = board.Hash();
	TranspositionEntry entry;
	bool found = Heuristics.RetrieveTranspositionEntry(hash, depth, entry, level);
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
		}
		Statistics.TranspositionHits += 1;
	}

	// Null-move pruning
	int friendlyPieces = Popcount(board.GetOccupancy(TurnToPieceColor(board.Turn)));
	int friendlyPawns = board.Turn == Turn::White ? Popcount(board.WhitePawnBits) : Popcount(board.BlackPawnBits);
	int nmpReduction = depth < 6 ? 2 : 3;
	if (!inCheck && (depth >= nmpReduction + 1) && canNullMove && (level > 1) && ((friendlyPieces - friendlyPawns) > 2) && !pvNode) {
		Move m = Move();
		m.SetFlag(MoveFlag::NullMove);
		Boards[level] = board;
		Boards[level].Push(m);
		int nullMoveEval = -SearchRecursive(Boards[level], depth - 1 - nmpReduction, level + 1, -beta, -beta + 1, false);
		if ((nullMoveEval >= beta) && !IsMateScore(nullMoveEval)) return beta;
	}

	// Futility pruning
	int staticEval = NoEval;
	const int futilityMargins[] = { 0, 100, 200, 300 };
	bool futilityPrunable = false;
	if ((depth <= 3) && !inCheck && !pvNode) {
		staticEval = EvaluateBoard(board, level);
		if ((staticEval + futilityMargins[depth] < alpha)) futilityPrunable = true;
	}

	// Razoring (?)
	const int razoringMargin[] = { 0, 300, 750 };
	if ((depth < 2) && !inCheck && !pvNode) {
		if (staticEval == NoEval) staticEval = EvaluateBoard(board, level);
		if (staticEval + razoringMargin[depth] < alpha) {
			int e = SearchQuiescence(board, level, alpha, beta, true);
			if (e < alpha) return alpha;
		}
	}

	// Initalize variables and generate moves
	MoveList.clear();
	board.GeneratePseudoLegalMoves(MoveList, board.Turn, false);

	// Move ordering
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	bool foundPvMove = false;
	for (const Move& m : MoveList) {
		int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, FollowingPV, transpositionMove);
		if (FollowingPV && Heuristics.IsPvMove(m, level)) foundPvMove = true;
		MoveOrder[level].push_back({ m, orderScore });
	}
	FollowingPV = foundPvMove;
	std::sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Iterate through legal moves
	//bool zeroWindow = false;
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	Move bestMove;
	for (const std::tuple<Move, int>& o : MoveOrder[level]) {
		Move m = get<0>(o);
		if (!board.IsLegalMove(m, board.Turn)) continue;
		legalMoveCount += 1;
		Boards[level] = board;
		Board& b = Boards[level];
		bool isQuiet = b.IsMoveQuiet(m);

		// Futility pruning
		if (isQuiet && futilityPrunable && !IsMateScore(alpha) && !IsMateScore(beta)) continue;

		b.Push(m);
		int score = NoEval;
		bool givingCheck = b.Turn == Turn::White ? Popcount(b.AttackedSquares & b.WhiteKingBits) != 0 : Popcount(b.AttackedSquares & b.BlackKingBits) != 0;
		//bool interestingPawnMove = (TypeOfPiece(board.GetPieceAt(m.from)) == PieceType::Pawn)
		//	&& ((phase > 0.8f) || ((board.Turn == Turn::White) && (GetSquareRank(m.to) >= 4)) || ((board.Turn == Turn::Black) && (GetSquareRank(m.to) <= 3)));
		
		if (legalMoveCount == 1) {
			score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
		}
		else {
			int reduction = 0;

			// Late-move reductions
			if ((legalMoveCount >= 5) && !pvNode && !inCheck && !givingCheck && isQuiet && (depth > 3)) {
				//reduction = log(depth) * log(legalMoveCount) / 4 + 1; // +30 , moves >= 5, depth >= 3
				//reduction = log(depth) * log(legalMoveCount) / 4.0 + 0.75; 
			}

			// Principal variation search
			score = -SearchRecursive(b, depth - 1 - reduction, level + 1, -alpha - 1, -alpha, true);
			if ((score > alpha) && (score < beta)) score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
		}

		// Checking alpha-beta bounds
		if (score >= beta) {
			if (isQuiet) Heuristics.AddKillerMove(m, level);
			Heuristics.AddTranspositionEntry(hash, depth, beta, ScoreType::LowerBound, m, level);
			int piece = board.GetPieceAt(m.from);
			if (isQuiet) Heuristics.AddCutoffHistory(board.Turn, m.from, m.to, depth);
			Statistics.BetaCutoffs += 1;
			if (legalMoveCount == 1) Statistics.FirstMoveBetaCutoffs += 1;
			return beta;
		}
		if (score > alpha) {
			scoreType = ScoreType::Exact;
			bestMove = m;
			alpha = score;
			Heuristics.UpdatePvTable(m, level, depth == 1);
		}
		Heuristics.DecrementHistory(board.Turn, m.from, m.to);

	}

	// There was no legal move --> game over 
	if (legalMoveCount == 0) {
		int e = inCheck ? LosingMateScore(level) : 0;
		Heuristics.AddTranspositionEntry(hash, depth, e, ScoreType::Exact, Move(), level);
		return e;
	}

	// Return alpha otherwise
	int e = alpha;
	Heuristics.AddTranspositionEntry(hash, depth, e, scoreType, bestMove, level);
	return e;
}

// Quiescence search: for captures (incl. en passant) and promotions only
int Search::SearchQuiescence(Board &board, int level, int alpha, int beta, bool rootNode) {
	MoveList.clear();
	board.GeneratePseudoLegalMoves(MoveList, board.Turn, true);
	if (!rootNode) {
		Statistics.Nodes += 1;
		Statistics.QuiescenceNodes += 1;
	}

	// Calculate delta pruning margin (~11% node reduction @ depth=8)
	// int delta = CalculateDeltaMargin(board);

	// Update alpha-beta bounds, return alpha if no captures left
	int staticEval = StaticEvaluation(board, level, true);
	if (staticEval >= beta) return beta;
	if (staticEval < alpha - 1000) return alpha; // Delta pruning
	if (staticEval > alpha) alpha = staticEval;

	// Order capture moves
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	for (const Move& m : MoveList) {
		int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, false, Move());
		MoveOrder[level].push_back({ m, orderScore });
	}
	std::sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Search recursively
	for (const std::tuple<Move, int>& o : MoveOrder[level]) {
		Move m = get<0>(o);
		if (!board.IsLegalMove(m, board.Turn)) continue;

		// Limiting quiescence search to not try underpromotions (experiments show a 0.9% underpromotion rate)
		if ((level <= Depth + 3) && (level >= 5) && (m.IsUnderpromotion())) continue;

		Boards[level] = board;
		Board& b = Boards[level];
		b.Push(m);
		int childEval = -SearchQuiescence(b, level + 1, -beta, -alpha, false);
		if (childEval >= beta) return beta;
		if (childEval > alpha) alpha = childEval;
	}
	return alpha;
}

int Search::StaticEvaluation(Board &board, int level, bool checkDraws) {
	Statistics.Evaluations += 1;
	if (level > Statistics.SelDepth) Statistics.SelDepth = level;
	if (checkDraws) {
		if (board.IsDraw()) return 0;
	}
	return EvaluateBoard(board, level);
}

const int Search::CalculateDeltaMargin(Board &board) {
	if ((board.WhiteQueenBits | board.BlackQueenBits) > 0) return 1000;
	if ((board.WhiteRookBits | board.BlackRookBits) > 0) return 600;
	return 400;
}


// Opening book -----------------------------------------------------------------------------------

void Search::InitOpeningBook() {
	std::ifstream ifs("book.bin", std::ios::in | std::ios::binary);

	if (!ifs) return;

	uint64_t buffer[2];
	int c = 0;

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

const void Search::PrintInfo(const Results e, const EngineSettings settings) {
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

	cout << "info depth " << e.depth << " seldepth " << e.stats.SelDepth << " score " << score << " nodes " << e.stats.Nodes << " nps " << e.nps
		<< " time " << e.time << " hashfull " << e.hashfull << extended << " pv";

	for (Move move : e.pv)
		cout << " " << move.ToString();
	cout << endl;
}

const void Search::PrintBestmove(Move move) {
	cout << "bestmove " << move.ToString() << endl;
}