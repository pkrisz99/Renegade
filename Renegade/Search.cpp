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

// Perft methods --------------------------------------------------------------

void Search::Perft(Board board, int depth, PerftType type) {
	Board b = board.Copy();
	b.DrawCheck = false;

	auto t0 = Clock::now();
	int r = PerftRecursive(b, depth, depth, type);
	auto t1 = Clock::now();

	float seconds = (float)((t1 - t0).count() / 1e9);
	float speed = r / seconds / 1000000;
	if ((type == PerftType::Normal) || (type == PerftType::PerftDiv)) {
		cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps | No bulk counting" << endl;
	}
	else cout << r << endl;
}

int Search::PerftRecursive(Board board, int depth, int originalDepth, PerftType type) {
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
SearchConstraints Search::CalculateConstraints(SearchParams params, bool turn) {
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

Evaluation Search::SearchMoves(Board board, SearchParams params, EngineSettings settings) {

	StartSearchTime = Clock::now();
	int elapsedMs = 0;
	EvaluatedNodes = 0;
	EvaluatedQuiescenceNodes = 0;
	Aborting = false;
	Depth = 0;
	bool finished = false;

	Constraints = CalculateConstraints(params, board.Turn);

	//cout << "info string Renegade searching for time: (" << Constraints.SearchTimeMin << ".." << Constraints.SearchTimeMax
	//	<< ") depth: " << Constraints.MaxDepth << " nodes: " << Constraints.MaxNodes << endl;

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
		int result = SearchRecursive(board, Depth, 1, NegativeInfinity, PositiveInfinity, true);
		Heuristics.SetHashSize(settings.Hash);

		// Check limits
		auto currentTime = Clock::now();
		elapsedMs = (int)((currentTime - StartSearchTime).count() / 1e6);
		if ((elapsedMs >= Constraints.SearchTimeMin) && (Constraints.SearchTimeMin != -1)) finished = true;
		if ((Depth >= Constraints.MaxDepth) && (Constraints.MaxDepth != -1)) finished = true;
		if ((Depth >= 100) || (Explored)) finished = true;
		if (Aborting) {
			e.nodes = EvaluatedNodes;
			e.time = elapsedMs;
			e.nps = (int)(EvaluatedNodes * 1e9 / (currentTime - StartSearchTime).count());
			e.hashfull = Heuristics.GetHashfull();
			PrintInfo(e);
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
		//for (int i = 0; i < e.pv.size(); i++) cout << e.pv[i].ToString() << endl;
		PrintInfo(e);
	}
	PrintBestmove(e.BestMove());

	Heuristics.ClearEntries();
	Heuristics.ClearPv();
	return e;
}

int Search::SearchRecursive(Board board, int depth, int level, int alpha, int beta, bool canNullMove) {

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
		int e = SearchQuiescence(board, level, alpha, beta);
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
	int remainingPieces = NonZeros(board.GetOccupancy());
	int reduction = 2;
	if (!inCheck && (depth >= reduction + 1) && canNullMove && (level > 1) && (remainingPieces > 5)) {
		Move m = Move();
		m.SetFlag(MoveFlag::NullMove);
		Board b = board.Copy();
		b.Push(m);
		int nullMoveEval = SearchRecursive(b, depth - 1 - reduction, level + 1, -beta, -beta + 1, false);
		if ((-nullMoveEval >= beta) && (abs(nullMoveEval) < MateEval - 1000)) return beta;
	}

	// Initalize variables
	int bestScore = NoEval;
	std::vector<Move> bestMoves;

	// Generate moves - if there are no legal moves, we return the eval
	std::vector<Move> pseudoMoves = board.GenerateMoves(board.Turn);

	// Move ordering
	std::vector<std::tuple<Move, int>> order = vector<std::tuple<Move, int>>();
	for (const Move& m : pseudoMoves) {
		int orderScore = Heuristics.CalculateMoveOrderScore(board, m, level);
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

int Search::SearchQuiescence(Board board, int level, int alpha, int beta) {
	std::vector<Move> captureMoves = board.GenerateCaptureMoves(board.Turn);

	// Update alpha-beta bounds, return static eval if no captures left
	int staticEval = StaticEvaluation(board, level);
	if (staticEval >= beta) return beta;
	if (staticEval < alpha - 1000) return alpha; // Delta pruning
	if (staticEval > alpha) alpha = staticEval;

	// Order capture moves
	std::vector<std::tuple<Move, int>> order = vector<std::tuple<Move, int>>();
	for (const Move& m : captureMoves) {
		int orderScore = Heuristics.CalculateMoveOrderScore(board, m, level);
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
		int childEval = -SearchQuiescence(b, level + 1, -beta, -alpha);
		if (childEval >= beta) return beta;
		if (childEval > alpha) alpha = childEval;
	}
	return alpha;
}

int Search::StaticEvaluation(Board board, int level) {
	EvaluatedNodes += 1;
	if (level > SelDepth) SelDepth = level;
	// 1. is over?
	if (board.State == GameState::Draw) return 0;
	if (board.State == GameState::WhiteVictory) {
		if (board.Turn == Turn::White) return MateEval - (level + 1) / 2;
		if (board.Turn == Turn::Black) return -MateEval + (level + 1) / 2;
	}
	else if (board.State == GameState::BlackVictory) {
		if (board.Turn == Turn::White) return -MateEval + (level + 1) / 2;
		if (board.Turn == Turn::Black) return MateEval - (level + 1) / 2;
	}

	// 2. Materials
	int score = 0;
	score += NonZeros(board.WhitePawnBits) * PawnValue;
	score += NonZeros(board.WhiteKnightBits) * KnightValue;
	score += NonZeros(board.WhiteBishopBits) * BishopValue;
	score += NonZeros(board.WhiteRookBits) * RookValue;
	score += NonZeros(board.WhiteQueenBits) * QueenValue;
	score -= NonZeros(board.BlackPawnBits) * PawnValue;
	score -= NonZeros(board.BlackKnightBits) * KnightValue;
	score -= NonZeros(board.BlackBishopBits) * BishopValue;
	score -= NonZeros(board.BlackRookBits) * RookValue;
	score -= NonZeros(board.BlackQueenBits) * QueenValue;

	if (NonZeros(board.WhiteBishopBits) >= 2) score += 50;
	if (NonZeros(board.BlackBishopBits) >= 2) score -= 50;

	uint64_t occupancy = board.GetOccupancy();
	while (occupancy != 0) {
		uint64_t i = 64 - __lzcnt64(occupancy) - 1;
		SetBitFalse(occupancy, i);
		int piece = board.GetPieceAt(i);
		if (piece == Piece::WhitePawn) score += PawnPSQT[i];
		else if (piece == Piece::WhiteKnight) score += KnightPSQT[i];
		else if (piece == Piece::WhiteBishop) score += BishopPSQT[i];
		else if (piece == Piece::WhiteRook) score += RookPSQT[i];
		else if (piece == Piece::WhiteQueen) score += QueenPSQT[i];

		if (piece == Piece::BlackPawn) score -= PawnPSQT[63 - i];
		else if (piece == Piece::BlackKnight) score -= KnightPSQT[63 - i];
		else if (piece == Piece::BlackBishop) score -= BishopPSQT[63 - i];
		else if (piece == Piece::BlackRook) score -= RookPSQT[63 - i];
		else if (piece == Piece::BlackQueen) score -= QueenPSQT[63 - i];
	}

	if (!board.Turn) score *= -1;
	return score;
}

// Opening book ---------------------------------------------------------------

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

std::string Search::GetBookMove(uint64_t hash) {
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

BookEntry Search::GetBookEntry(int item) {
	return BookEntries[item];
}

int Search::GetBookSize() {
	return BookEntries.size();
}


// Communicating the search results -------------------------------------------

void Search::PrintInfo(Evaluation e) {
	cout << "info depth " << e.depth << " seldepth " << e.seldepth << " score cp " << e.score << " nodes " << e.nodes << /* " qnodes " << e.qnodes << */ " nps " << e.nps
		<< " time " << e.time << " hashfull " << e.hashfull << " pv";

	for (Move move : e.pv)
		cout << " " << move.ToString();
	cout << endl;
}

void Search::PrintBestmove(Move move) {
	cout << "bestmove " << move.ToString() << endl;
}