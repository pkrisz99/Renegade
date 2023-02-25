#include "Search.h"

Search::Search() {
	Reset();
}

void Search::Reset() {
	EvaluatedNodes = 0;
	EvaluatedQuiescenceNodes = 0;
	SelDepth = 0;
	Depth = 0;
	InitOpeningBook();
}

// Perft methods ----------------------------------------------------------------------------------

const void Search::Perft(Board board, const int depth, const PerftType type) {
	Board b = board.Copy();

	auto t0 = Clock::now();
	const int r = PerftRecursive(b, depth, depth, type);
	auto t1 = Clock::now();

	const float seconds = (float)((t1 - t0).count() / 1e9);
	const float speed = r / seconds / 1000000;
	if ((type == PerftType::Normal) || (type == PerftType::PerftDiv)) {
		cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps | No bulk counting" << endl;
	}
	else cout << r << endl;
}

const int Search::PerftRecursive(Board board, const int depth, const int originalDepth, const PerftType type) {
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

// Time management
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
	EvaluatedNodes = 0;
	EvaluatedQuiescenceNodes = 0;
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
		std::string bookMove = GetBookMove(board.Hash(false));
		if (bookMove != "") {
			Results e;
			cout << "bestmove " << bookMove << endl;
			return e;
		}
	}

	// Iterative deepening
	Results e = Results();
	while (!finished) {
		Heuristics.ClearEntries();
		if (Heuristics.GetHashfull() > 500) Heuristics.ResetHashStructure(); // Just in case if we overallocate
		Depth += 1;
		SelDepth = 0;
		int result = SearchRecursive(board, Depth, 0, NegativeInfinity, PositiveInfinity, true);
		Heuristics.SetHashSize(settings.Hash);

		// Check limits
		auto currentTime = Clock::now();
		elapsedMs = static_cast<int>((currentTime - StartSearchTime).count() / 1e6);
		if ((elapsedMs >= Constraints.SearchTimeMin) && (Constraints.SearchTimeMin != -1)) finished = true;
		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if ((Depth >= 32)) finished = true;
		if (Aborting) {
			e.nodes = EvaluatedNodes;
			e.qnodes = EvaluatedQuiescenceNodes;
			e.time = elapsedMs;
			e.nps = static_cast<int>(EvaluatedNodes * 1e9 / (currentTime - StartSearchTime).count());
			e.hashfull = Heuristics.GetHashfull();
			PrintInfo(e, settings);
			break;
		}

		// Send info
		e.score = result;
		e.depth = Depth;
		e.seldepth = SelDepth;
		e.nodes = EvaluatedNodes;
		e.qnodes = EvaluatedQuiescenceNodes;
		e.time = elapsedMs;
		e.nps = static_cast<int>(EvaluatedNodes * 1e9 / (currentTime - StartSearchTime).count());
		e.hashfull = Heuristics.GetHashfull();

		// Check PV validity (note & todo: may not be correct)
		e.pv.clear();
		std::vector<Move> returnedPVs = Heuristics.GetPvLine();
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

		Heuristics.SetPv(e.pv);
		PrintInfo(e, settings);
	}
	PrintBestmove(e.BestMove());

	Heuristics.ClearEntries();
	if (Heuristics.GetHashfull() > 500) Heuristics.ResetHashStructure();
	Heuristics.ClearPv();
	Aborting = true;
	return e;
}

// Recursively called during the negamax search
int Search::SearchRecursive(Board &board, int depth, int level, int alpha, int beta, bool canNullMove) {

	// Check limits
	if (Aborting) return NoEval;
	if ((Constraints.MaxNodes != -1) && (EvaluatedNodes >= Constraints.MaxNodes) && (Depth > 1)) {
		Aborting = true;
		return NoEval;
	}
	if ((EvaluatedNodes % 1000 == 0) && (Constraints.SearchTimeMax != -1) && (Depth > 1)) {
		auto now = Clock::now();
		int elapsedMs = (int)((now - StartSearchTime).count() / 1e6);
		if (elapsedMs >= Constraints.SearchTimeMax) {
			Aborting = true;
			return NoEval;
		}
	}

	// Check extensions
	uint64_t kingBits = board.Turn == Turn::White ? board.WhiteKingBits : board.BlackKingBits;
	bool inCheck = (board.AttackedSquares & kingBits) != 0;
	if (inCheck && (depth == 0) && (level < Depth + 10)) depth = 1;
	bool pvNode = beta - alpha > 1;

	// Return result for terminal nodes
	if (depth <= 0) {
		if (depth < 0) cout << "Check depth: " << depth << endl;
		int e = SearchQuiescence(board, level, alpha, beta, true);
		//Heuristics.AddEntry(hash, e, ScoreType::Exact);
		return e;
	}

	// Check for draws
	if (board.IsDraw()) {
		int score = 0;
		if (score >= beta) return beta;
		if (score < alpha) return alpha;
		return score;
	}

	// Calculate and check hash
	uint64_t hash = board.Hash(true);
	std::tuple<bool, HashEntry> retrieved = Heuristics.RetrieveEntry(hash);
	if (get<0>(retrieved)) {
		HashEntry entry = get<1>(retrieved);
		int score = NoEval;
		bool usable = true;
		if (entry.scoreType == ScoreType::Exact) score = entry.score;
		else if ((entry.scoreType == ScoreType::UpperBound) && (entry.score <= alpha)) score = alpha;
		else if ((entry.scoreType == ScoreType::LowerBound) && (entry.score >= beta)) score = beta;
		else usable = false;
		if (usable) return score;
	}

	// Null-move pruning
	int friendlyPieces = Popcount(board.GetOccupancy(TurnToPieceColor(board.Turn)));
	int friendlyPawns = board.Turn == Turn::White ? Popcount(board.WhitePawnBits) : Popcount(board.BlackPawnBits);
	int reduction = 2;
	if (!inCheck && (depth >= reduction + 1) && canNullMove && (level > 1) && ((friendlyPieces - friendlyPawns) > 2) && !pvNode) {
		Move m = Move();
		m.SetFlag(MoveFlag::NullMove);
		Boards[level] = board;
		Boards[level].Push(m);
		int nullMoveEval = SearchRecursive(Boards[level], depth - 1 - reduction, level + 1, -beta, -beta + 1, false);
		if ((-nullMoveEval >= beta) && !IsMateScore(nullMoveEval)) return beta;
	}

	// Futility pruning
	int staticEval = NoEval;
	const int futilityMargins[] = { 100, 200 };
	bool futilityPrunable = false;
	if ((depth <= 2) && !inCheck && !pvNode) {
		staticEval = EvaluateBoard(board, level);
		if ((depth == 1) && (staticEval + futilityMargins[0] < alpha)) futilityPrunable = true;
		else if ((depth == 2) && (staticEval + futilityMargins[1] < alpha)) futilityPrunable = true;
	}

	// Razoring (?)
	const int razoringMargin = 400;
	if ((depth < 2) && (level > 2) && !inCheck && !pvNode) {
		if (staticEval == NoEval) staticEval = EvaluateBoard(board, level);
		if (staticEval + razoringMargin < alpha) {
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
	for (const Move& m : MoveList) {
		int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, pvNode);
		MoveOrder[level].push_back({ m, orderScore });
	}
	std::sort(MoveOrder[level].begin(), MoveOrder[level].end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Iterate through legal moves
	bool zeroWindow = false;
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	bool doLateMoveReductions = true;
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
		bool doPvSearch = true;

		// Late move reductions
		/*
		if (doLateMoveReductions) {
			if ((legalMoveCount > 4) && !pvNode && !inCheck && isQuiet && (depth >= 3) && canNullMove) {
				int reduction = 1;
				int reducedScore = -SearchRecursive(b, depth - 1 - reduction, level + 1, -alpha - 1, -alpha, true);
				if (reducedScore <= alpha) {
					childEval = -reducedScore;
					// pvSearch = true;
				}
			}
		}*/

		// Principal variation search
		if (doPvSearch) {
			if (!zeroWindow) {
				score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
			}
			else {
				score = -SearchRecursive(b, depth - 1, level + 1, -alpha - 1, -alpha, true);
				if (alpha < score) {
					score = -SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
				}
			}
		}

		// Checking alpha-beta bounds
		if (score >= beta) {
			if (isQuiet) Heuristics.AddKillerMove(m, level);
			Heuristics.AddEntry(hash, beta, ScoreType::LowerBound);
			return beta;
		}
		if (score > alpha) {
			scoreType = ScoreType::Exact;
			alpha = score;
			zeroWindow = true;
			Heuristics.UpdatePvTable(m, level, depth == 1);
			doLateMoveReductions = false;
		}

	}

	// There was no legal move --> game over 
	if (legalMoveCount == 0) {
		int e;
		if (!inCheck) e = 0;
		else e = -MateEval + (level + 1) / 2;

		if (e >= beta) {
			Heuristics.AddEntry(hash, beta, ScoreType::LowerBound);
			return beta;
		}
		if (e < alpha) {
			Heuristics.AddEntry(hash, alpha, ScoreType::UpperBound);
			return alpha;
		}
		Heuristics.AddEntry(hash, e, ScoreType::Exact);
		return e;
	}

	int e = alpha;
	Heuristics.AddEntry(hash, e, scoreType);
	return e;
}

// Quiescence search: for captures (incl. en passant) and promotions only
int Search::SearchQuiescence(Board &board, int level, int alpha, int beta, bool rootNode) {
	MoveList.clear();
	board.GeneratePseudoLegalMoves(MoveList, board.Turn, true);
	if (!rootNode) EvaluatedQuiescenceNodes += 1;

	// Calculate delta pruning margin (~11% node reduction @ depth=8)
	int delta = CalculateDeltaMargin(board);

	// Update alpha-beta bounds, return alpha if no captures left
	int staticEval = StaticEvaluation(board, level, true);
	if (staticEval >= beta) return beta;
	if (staticEval < alpha - 1000) return alpha; // Delta pruning
	if (staticEval > alpha) alpha = staticEval;

	// Order capture moves
	const float phase = CalculateGamePhase(board);
	MoveOrder[level].clear();
	for (const Move& m : MoveList) {
		int orderScore = Heuristics.CalculateOrderScore(board, m, level, phase, false);
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
	EvaluatedNodes += 1;
	if (level > SelDepth) SelDepth = level;
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
	if ((abs(e.score) > MateEval - 1000) && (abs(e.score) <= MateEval)) {
		int movesToMate = MateEval - abs(e.score);
		if (e.score > 0) score = "mate " + std::to_string(movesToMate);
		else score = "mate -" + std::to_string(movesToMate);
	}
	else {
		score = "cp " + std::to_string(e.score);
	}

	std::string extended;
	if (settings.ExtendedOutput) {
		extended = " qnodes " + std::to_string(e.qnodes);
	}

	cout << "info depth " << e.depth << " seldepth " << e.seldepth << " score " << score << " nodes " << e.nodes << " nps " << e.nps
		<< " time " << e.time << " hashfull " << e.hashfull << extended << " pv";

	for (Move move : e.pv)
		cout << " " << move.ToString();
	cout << endl;
}

const void Search::PrintBestmove(Move move) {
	cout << "bestmove " << move.ToString() << endl;
}