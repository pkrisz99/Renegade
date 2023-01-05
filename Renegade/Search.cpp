#include "Search.h"

Search::Search() {
	Reset();
}

void Search::Reset() {
	EvaluatedNodes = 0;
	EvaluatedQuiescenceNodes = 0;
	SelDepth = 0;
	Depth = 0;
	//Heuristics = Heuristics();
	InitOpeningBook();
}

// Perft methods ----------------------------------------------------------------------------------

const void Search::Perft(Board board, const int depth, const PerftType type) {
	Board b = board.Copy();
	b.DrawCheck = false;

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
	std::vector<Move> moves = board.GenerateLegalMoves(board.Turn);
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

// Time allocation
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
	if (myTime != 0) {
		int maxTime = (int)(myTime * 0.5);
		int minTime = (int)(myTime * 0.015);
		constraints.SearchTimeMax = maxTime;
		constraints.SearchTimeMin = minTime;
		return constraints;
	}

	// Default: go infinite
	return constraints;
}

// Negamax search routine and handling ------------------------------------------------------------

Evaluation Search::SearchMoves(Board &board, SearchParams params, EngineSettings settings) {

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
			Evaluation e;
			cout << "bestmove " << bookMove << endl;
			return e;
		}
	}

	// Iterative deepening
	Evaluation e = Evaluation();
	while (!finished) {
		Heuristics.ClearEntries();
		Depth += 1;
		SelDepth = 0;
		Explored = true;
		int result = SearchRecursive(board, Depth, 0, NegativeInfinity, PositiveInfinity, true);
		Heuristics.SetHashSize(settings.Hash);

		// Check limits
		auto currentTime = Clock::now();
		elapsedMs = (int)((currentTime - StartSearchTime).count() / 1e6);
		if ((elapsedMs >= Constraints.SearchTimeMin) && (Constraints.SearchTimeMin != -1)) finished = true;
		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if ((Depth >= 100) || (Explored)) finished = true;
		if (Aborting) {
			e.nodes = EvaluatedNodes;
			e.qnodes = EvaluatedQuiescenceNodes;
			e.time = elapsedMs;
			e.nps = (int)(EvaluatedNodes * 1e9 / (currentTime - StartSearchTime).count());
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
		e.nps = (int)(EvaluatedNodes * 1e9 / (currentTime - StartSearchTime).count());
		e.hashfull = Heuristics.GetHashfull();
		e.pv = Heuristics.GetPvLine();

		Heuristics.SetPv(e.pv);
		PrintInfo(e, settings);
	}
	PrintBestmove(e.BestMove());

	Heuristics.ClearEntries();
	Heuristics.ClearPv();
	return e;
}

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

	// Return result for terminal nodes
	if (depth <= 0) {
		//int e = StaticEvaluation(board, level);
		int e = SearchQuiescence(board, level, alpha, beta, true);
		if (board.State == GameState::Playing) Explored = false;
		//Heuristics.AddEntry(hash, e, ScoreType::Exact);
		return e;
	}

	// Calculate hash
	uint64_t hash = board.Hash(true);

	// Check hash
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
	bool kingBits = board.Turn == Turn::White ? board.WhiteKingBits : board.BlackKingBits;
	bool inCheck = (board.AttackedSquares & kingBits) != 0;
	int remainingPieces = Popcount(board.GetOccupancy());
	int reduction = 2;
	if (!inCheck && (depth >= reduction + 1) && canNullMove && (level > 1) && (remainingPieces > 5)) {
		Move m = Move();
		m.SetFlag(MoveFlag::NullMove);
		Board b = board.Copy();
		b.Push(m);
		int nullMoveEval = SearchRecursive(b, depth - 1 - reduction, level + 1, -beta, -beta + 1, false);
		if ((-nullMoveEval >= beta) && (abs(nullMoveEval) < MateEval - 1000)) return beta;
	}

	// Initalize variables, and generate moves - if there are no legal moves, we'll return alpha
	int bestScore = NoEval;
	std::vector<Move> bestMoves;
	std::vector<Move> pseudoMoves = board.GeneratePseudoLegalMoves(board.Turn);

	// Move ordering
	const float phase = CalculateGamePhase(board);
	std::vector<std::tuple<Move, int>> order = vector<std::tuple<Move, int>>();
	for (const Move& m : pseudoMoves) {
		int orderScore = Heuristics.CalculateMoveOrderScore(board, m, level, phase);
		order.push_back({ m, orderScore });
	}
	std::sort(order.begin(), order.end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Iterate through legal moves
	bool pvSearch = false;
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	for (const std::tuple<Move, int>& o : order) {
		Move m = get<0>(o);
		if (!board.IsLegalMove(m, board.Turn)) continue;
		legalMoveCount += 1;
		Board b = board.Copy();
		b.Push(m);
		//eval childEval = SearchRecursive(b, depth - 1, level + 1, -beta, -alpha);

		// Principal variation search - questionable gains
		int childEval;
		if (!pvSearch) {
			childEval = SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
		}
		else {
			childEval = SearchRecursive(b, depth - 1, level + 1, -alpha - 1, -alpha, true);
			if ((alpha < -childEval) && (-childEval < beta)) {
				childEval = SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
			}
		}

		int childScore = -childEval;

		if (childScore > bestScore) {
			bestScore = childScore;

			if (bestScore >= beta) {
				//bool capture = board.GetPieceAt(get<0>(o).to) != Piece::None;
				if (true) Heuristics.AddKillerMove(m, level);
				int e = beta;
				Heuristics.AddEntry(hash, e, ScoreType::LowerBound);
				return e;
			}

			if (bestScore > alpha) {
				scoreType = ScoreType::Exact;
				alpha = bestScore;
				pvSearch = true;
				Heuristics.UpdatePvTable(m, level);
			}
		}

	}

	// Case when there was no legal move 
	if (legalMoveCount == 0) {
		int e = StaticEvaluation(board, level);
		Heuristics.AddEntry(hash, e, ScoreType::Exact);
		return e;
	}

	int e = alpha;
	Heuristics.AddEntry(hash, e, scoreType);
	return e;
}

int Search::SearchQuiescence(Board board, int level, int alpha, int beta, bool rootNode) {
	std::vector<Move> captureMoves = board.GenerateNonQuietMoves(board.Turn);
	if (!rootNode) EvaluatedQuiescenceNodes += 1;

	// Update alpha-beta bounds, return alpha if no captures left
	int staticEval = StaticEvaluation(board, level);
	if (staticEval >= beta) return beta;
	if (staticEval < alpha - 1000) return alpha; // Delta pruning
	if (staticEval > alpha) alpha = staticEval;
	if (board.State != GameState::Playing) return alpha;

	// Order capture moves
	const float phase = CalculateGamePhase(board);
	std::vector<std::tuple<Move, int>> order = vector<std::tuple<Move, int>>();
	for (const Move& m : captureMoves) {
		int orderScore = Heuristics.CalculateMoveOrderScore(board, m, level, phase);
		order.push_back({ m, orderScore });
	}
	std::sort(order.begin(), order.end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});

	// Search recursively
	for (const std::tuple<Move, int>& o : order) {
		Move m = get<0>(o);
		if (!board.IsLegalMove(m, board.Turn)) continue;
		Board b = board.Copy();
		b.Push(m);
		int childEval = -SearchQuiescence(b, level + 1, -beta, -alpha, false);
		if (childEval >= beta) return beta;
		if (childEval > alpha) alpha = childEval;
	}
	return alpha;
}

int Search::StaticEvaluation(Board &board, int level) {
	EvaluatedNodes += 1;
	if (level > SelDepth) SelDepth = level;
	return EvaluateBoard(board, level);
}


// Opening book -----------------------------------------------------------------------------------

void Search::InitOpeningBook() {
	std::ifstream ifs("book.bin", std::ios::in | std::ios::binary);

	if (!ifs) return;

	uint64_t buffer[2];
	int c = 0;

	while (ifs.read(reinterpret_cast<char*>(&buffer), 16)) {
		BookEntry entry;
		int b = _byteswap_ushort(0xFFFF & buffer[1]);
		entry.hash = _byteswap_uint64(buffer[0]);
		entry.to = (0b000000000111111 & b) >> 0;
		entry.from = (0b000111111000000 & b) >> 6;
		entry.promotion = (0b111000000000000 & b) >> 12;
		entry.weight = 0;
		entry.learn = 0;
		BookEntries.push_back(entry);
	}
	ifs.close();

}

const std::string Search::GetBookMove(const uint64_t hash) {
	// should take about 2-3 ms for Human.bin (~900k entries) 
	std::vector<string> matches;
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
	}

	//cout << matches.size() << endl;
	if (matches.size() == 0) return "";
	std::srand(std::time(0));
	int random_pos = std::rand() % matches.size();

	return matches[random_pos];
}

const BookEntry Search::GetBookEntry(int item) {
	return BookEntries[item];
}

const int Search::GetBookSize() {
	return BookEntries.size();
}


// Communicating the search results ---------------------------------------------------------------

const void Search::PrintInfo(const Evaluation e, const EngineSettings settings) {
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