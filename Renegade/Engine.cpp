#include "Engine.h"

Engine::Engine() {
	EvaluatedNodes = 0;
	EvaluatedQuiescenceNodes = 0;
	SelDepth = 0;
	Depth = 0;
	Settings.Hash = 64;
	Settings.QSearch = false;
	Settings.UseBook = false;
	HashSize = 125000 * Settings.Hash;
	//Heuristics = Heuristics();
	InitOpeningBook();
}

void Engine::perft(string fen, int depth, bool verbose) {
	Board board = Board(fen);
	perft(board, depth, verbose);
}

void Engine::perft(Board board, int depth, bool verbose) {
	Board b = board.Copy();
	b.DrawCheck = false;
	auto t0 = Clock::now();
	int r = perft1(b, depth, verbose);
	auto t1 = Clock::now();
	float seconds = (float)((t1 - t0).count() / 1e9);
	float speed = r / seconds / 1000000;
	if (verbose) cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(2) << std::fixed << seconds << " s | " << std::setprecision(3) << speed << " mnps" << endl;
	else cout << r << endl;
}

int Engine::perft1(Board board, int depth, bool verbose) {
	std::vector<Move> moves = board.GenerateLegalMoves(board.Turn);
	if (verbose) cout << "Legal moves (" << moves.size() << "): " << endl;
	int count = 0;
	for (Move m : moves) {
		Board b = board.Copy();
		b.Push(m);
		int r;
		if (depth == 1) {
			r = 1;
			count += r;
		} else {
			r = perftRecursive(b, depth - 1);
			count += r;
		}
		if (verbose) cout << " - " << m.ToString() << " : " << r << endl;
	}
	return count;
}


int Engine::perftRecursive(Board b, int depth) {
	std::vector<Move> moves = b.GenerateLegalMoves(b.Turn);
	if (depth == 1) return (int)moves.size();
	int count = 0;
	for (const Move& m : moves) {
		Board child = b.Copy();
		child.Push(m);
		count += perftRecursive(child, depth - 1);
	}
	return count;
};

// Time allocation
SearchConstraints Engine::CalculateConstraints(SearchParams params, bool turn) {
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

Evaluation Engine::Search(Board board) {

	StartSearchTime = Clock::now();
	int elapsedMs = 0;
	EvaluatedNodes = 0;
	EvaluatedQuiescenceNodes = 0;
	Aborting = false;
	Depth = 0;
	bool finished = false;

	//cout << "info string Renegade searching for time: (" << Constraints.SearchTimeMin << ".." << Constraints.SearchTimeMax
	//	<< ") depth: " << Constraints.MaxDepth << " nodes: " << Constraints.MaxNodes << endl;
	
	// Check for book moves
	if (Settings.UseBook) {
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
		Heuristics.SetHashSize(Settings.Hash);

		// Check limits
		auto currentTime = Clock::now();
		elapsedMs = (int) ((currentTime - StartSearchTime).count() / 1e6);
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
		PrintInfo(e);
	}
	cout << "bestmove " << e.BestMove().ToString() << endl;

	Heuristics.ClearEntries();
	Heuristics.ClearPv();
	return e;
}

int Engine::SearchRecursive(Board board, int depth, int level, int alpha, int beta, bool canNullMove) {

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
		int e = StaticEvaluation(board, level);
		if (board.State == GameState::Playing) Explored = false;
		//Heuristics.AddEntry(hash, e, ScoreType::Exact);
		return e;
	}

	// Calculate hash
	unsigned __int64 hash = board.Hash(true);

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
	int reduction;
	if (depth >= 7) reduction = 3;
	else reduction = 2;
	if (!inCheck && (depth >= reduction + 1) && canNullMove && (level > 1)) {
		Move m = Move();
		m.SetFlag(MoveFlag::NullMove);
		Board b = board.Copy();
		b.Push(m);
		int nullMoveEval = SearchRecursive(b, depth - 1 - reduction, level + 1, -beta, -beta+1, false);
		if (-nullMoveEval >= beta) return beta;
	}

	// Initalize variables
	int bestScore = NoEval;
	std::vector<Move> bestMoves;

	// Generate moves - if there are no legal moves, we return the eval
	std::vector<Move> pseudoMoves = board.GenerateMoves(board.Turn);

	// Move ordering
	std::vector<std::tuple<Move, int>> order = vector<std::tuple<Move, int>>();
	for (const Move& m : pseudoMoves) {
		
		int orderScore = 0;
		int attackingPiece = TypeOfPiece(board.GetPieceAt(m.from)); // attackingPieceType
		int capturedPiece = TypeOfPiece(board.GetPieceAt(m.to)); // attackedPieceType
		const int values[] = { 0, 100, 300, 300, 500, 900, 10000 };
		if (capturedPiece != PieceType::None) {
			orderScore = values[capturedPiece] - values[attackingPiece] + 10;
		}

		if (m.flag == MoveFlag::PromotionToQueen) orderScore += 900;
		else if (m.flag == MoveFlag::PromotionToRook) orderScore += 500;
		else if (m.flag == MoveFlag::PromotionToBishop) orderScore += 300;
		else if (m.flag == MoveFlag::PromotionToKnight) orderScore += 300;
		
		if (attackingPiece == PieceType::Pawn) orderScore += PawnPSQT[m.to] - PawnPSQT[m.from];
		else if (attackingPiece == PieceType::Knight) orderScore += KnightPSQT[m.to] - KnightPSQT[m.from];
		else if (attackingPiece == PieceType::Bishop) orderScore += BishopPSQT[m.to] - BishopPSQT[m.from];
		else if (attackingPiece == PieceType::Rook) orderScore += RookPSQT[m.to] - RookPSQT[m.from];
		else if (attackingPiece == PieceType::Queen) orderScore += QueenPSQT[m.to] - QueenPSQT[m.from];

		if (Heuristics.IsKillerMove(m, level)) orderScore += 200000;
		//if (Heuristics.IsPvMove(m, level)) orderScore += 100000;

		order.push_back({m, orderScore});
	}
	std::sort(order.begin(), order.end(), [](auto const& t1, auto const& t2) {
		return get<1>(t1) > get<1>(t2);
	});
	
	// Iterate through legal moves
	bool pvSearch = false;
	int scoreType = ScoreType::UpperBound;
	int legalMoveCount = 0;
	for (const std::tuple<Move, int>&o : order) {
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
			childEval = SearchRecursive(b, depth - 1, level + 1, -alpha-1, -alpha, true);
			if ((alpha < -childEval) && (-childEval < beta)) {
				childEval = SearchRecursive(b, depth - 1, level + 1, -beta, -alpha, true);
			}
		}
		
		int childScore = -childEval;

		if (childScore > bestScore) {
			bestScore = childScore;

			if (bestScore >= beta) {
				//bool capture = b.GetPieceAt(get<0>(o).to) != Piece::None;
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

int Engine::StaticEvaluation(Board board, int level) {
	EvaluatedNodes += 1;
	if (level > SelDepth) SelDepth = level;
	// 1. is over?
	if (board.State == GameState::Draw) return 0;
	if (board.State == GameState::WhiteVictory) {
		if (board.Turn == Turn::White) return MateEval - (level + 1) / 2;
		if (board.Turn == Turn::Black) return -MateEval + (level + 1) / 2;
	} else if (board.State == GameState::BlackVictory) {
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
		unsigned __int64 i = 64 - __lzcnt64(occupancy) - 1;
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

// Start UCI protocol
void Engine::Start() {
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
	std::string cmd = "";
	Board board = Board(starting_fen);
	while (getline(cin, cmd)) {

		cmd = trimstr(cmd);
		if (cmd.size() == 0) continue;
		std::stringstream ss(cmd);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;
		std::vector<std::string> parts(begin, end);

		if (cmd == "quit") break;

		if (cmd == "uci") {
			cout << "id name Renegade " << Version << endl;
			cout << "id author Krisztian Peocz" << endl;
			cout << "option name Hash type spin default 64 min 0 max 256" << endl;
			cout << "option name OwnBook type check default false" << endl;
			//cout << "option name QSearch type check default false" << endl;
			cout << "uciok" << endl;
			continue;
		}

		if (cmd == "isready") {
			cout << "readyok" << endl;
			continue;
		}

		if (cmd == "ucinewgame") {
			continue;
		}

		if (cmd == "play") {
			Play();
			continue;
		}

		// Set option
		if (parts[0] == "setoption") {

			parts[2] = lowercase(parts[2]);

			if (parts[2] == "ownbook") {
				parts[4] = lowercase(parts[4]);
				if (parts[4] == "true") Settings.UseBook = true;
				else if (parts[4] == "false") Settings.UseBook = false;
				else cout << "Unknown value: '" << parts[4] << "'" << endl;
			}
			else if (parts[2] == "qsearch") {
				parts[4] = lowercase(parts[4]);
				if (parts[4] == "true") Settings.QSearch = true;
				else if (parts[4] == "false") Settings.QSearch = false;
				else cout << "Unknown value: '" << parts[4] << "'" << endl;
			}
			else if (parts[2] == "hash") {
				Settings.Hash = stoi(parts[4]);
				HashSize = Settings.Hash * 125000;
			}
			else {
				cout << "Invalid option: '" << parts[2] << "'" << endl;
			}
			continue;

		}

		// Debug commands
		if (parts[0] == "debug") {
			if (parts[1] == "draw") board.Draw(0ULL);
			if (parts[1] == "attackmap") board.Draw(board.AttackedSquares);
			if (parts[1] == "enpassant") cout << "En passant target: " << board.EnPassantSquare << endl;
			if (parts[1] == "pseudolegal") {
				std::vector<Move> v = board.GenerateMoves(board.Turn);
				for (Move m : v) cout << m.ToString() << " ";
				cout << endl;
			}
			if (parts[1] == "hash") {
				cout << "Hash (polyglot): " << std::hex << board.Hash(false) << endl;
				cout << "Hash (custom):   " << board.Hash(true) << std::dec << endl;
			}
			if (parts[1] == "book") {
				if (parts[2] == "count") {
					cout << "Book entries: " << BookEntries.size() << " (from: " << std::filesystem::current_path().string() << ")" << endl;
				}
				else if (parts[2] == "entry") {
					int n = stoi(parts[3]);
					cout << "Entry " << n << ": " << std::hex << BookEntries[n].hash << std::dec << PolyglotMoveToString(BookEntries[n].from, BookEntries[n].to, BookEntries[n].promotion) << endl;
				}
				else {
					std::string s = GetBookMove(board.Hash(false));
					if (s != "") cout << "Book move: " << s << endl;
					else cout << "No book move found" << endl;
				}
				
			}
			if (parts[1] == "settings") {
				cout << "Hash: " << Settings.Hash << endl;
				cout << "OwnBook: " << Settings.UseBook << endl;
				//cout << "QSearch: " << Settings.QSearch << endl;
			}
			if (parts[1] == "sizeof") {
				cout << "sizeof HashEntry:         " << sizeof(HashEntry) << endl;
				cout << "sizeof Move:              " << sizeof(Move) << endl;
				cout << "sizeof std::vector<Move>: " << sizeof(std::vector<Move>) << endl;
				cout << "sizeof int:               " << sizeof(int) << endl;
			}
			if (parts[1] == "eval") {
				cout << "Static evaluation: " << StaticEvaluation(board, 0) << endl;
			}
			if (parts[1] == "pasthashes") {
				cout << "Past hashes size: " << board.PastHashes.size() << endl;
				for (int i = 0; i < board.PastHashes.size(); i++) {
					cout << "- entry " << i << ": " << std::hex << board.PastHashes[i] << std::dec << endl;
				}
			}
			if (parts[1] == "runtime") {
				cout << "Timing test suite:" << endl;
				__int64 nanoseconds = 0;
				unsigned __int64 dummy = 0;
				for (int i = 0; i < 100000; i++) {
					auto t0 = Clock::now();
					dummy = StaticEvaluation(board, 0);
					auto t1 = Clock::now();
					nanoseconds += (t1 - t0).count();
					if (dummy > 10000000) break;
				}
				cout << "- static evaluation: " << nanoseconds / 1e8 << " us" << endl;
				nanoseconds = 0;
				std::vector<Move> dummyMoves;
				for (int i = 0; i < 100000; i++) {
					auto t0 = Clock::now();
					dummyMoves = board.GenerateLegalMoves(board.Turn);
					auto t1 = Clock::now();
					nanoseconds += (t1 - t0).count();
					if (dummyMoves.size() < 0) break;
				}
				cout << "- legal move gen: " << nanoseconds / 1e8 << " us" << endl;
				nanoseconds = 0;
				dummy = 0;
				for (int i = 0; i < 100000; i++) {
					auto t0 = Clock::now();
					board.GenerateMoves(board.Turn);
					auto t1 = Clock::now();
					nanoseconds += (t1 - t0).count();
				}
				cout << "- pseudo move gen: " << nanoseconds / 1e8 << " us" << endl;
				nanoseconds = 0;
				dummy = 0;
				for (int i = 0; i < 100000; i++) {
					auto t0 = Clock::now();
					dummy = board.CalculateAttackedSquares(TurnToPieceColor(!board.Turn));
					auto t1 = Clock::now();
					nanoseconds += (t1 - t0).count();
					if (dummy == 0) break; // impossible condition on purpose
				}
				cout << "- attack map calc: " << nanoseconds / 1e8 << " us" << endl;
				dummy = 0ULL;
				for (int i = 0; i < 100000; i++) {
					auto t0 = Clock::now();
					dummy = board.Hash(true);
					auto t1 = Clock::now();
					nanoseconds += (t1 - t0).count();
					if (dummy == 0) break; // (almost) impossible condition on purpose
				}
				cout << "- hashing: " << nanoseconds / 1e8 << " us" << endl;
				
			}
			continue;
		}

		if (cmd == "nb") { // shorthand for no book (useful for testing)
			Settings.UseBook = false;
			continue;
		}

		// Position command
		if (parts[0] == "position") {

			if ((parts[1] == "startpos") || (parts[1] == "kiwipete")) {
				if (parts[1] == "startpos") board = Board(starting_fen);
				if (parts[1] == "kiwipete") board = Board(kiwipete_fen);
				if (parts[2] == "moves") {
					for (int i = 3; i < parts.size(); i++) {
						bool r = board.PushUci(parts[i]);
						if (!r) cout << "!!! Error: invalid pushuci move !!!" << endl;
					}
				}
			}

			if (parts[1] == "fen") {
				std::string fen = parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " " + parts[6] + " " + parts[7];
				board = Board(fen);
				if (parts[8] == "moves") {
					for (int i = 9; i < parts.size(); i++) {
						bool r = board.PushUci(parts[i]);
						if (!r) cout << "!!! Error: invalid pushuci move !!!" << endl;
					}
				}
			}

			continue;
		}

		// Go command
		if (parts[0] == "go") {

			if (parts[1] == "perft") {
				int depth = stoi(parts[2]);
				perft(board, depth, true);
				continue;
			}
			if (parts[1] == "perfd") {
				int depth = stoi(parts[2]);
				perft(board, depth, false);
				continue;
			}

			SearchParams params;
			for (__int64 i = 1; i < parts.size(); i++) {
				if (parts[i] == "wtime") {
					params.wtime = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "btime") {
					params.btime = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "movestogo") {
					params.movestogo = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "winc") {
					params.winc = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "binc") {
					params.binc = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "nodes") {
					params.nodes = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "depth") {
					params.depth = stoi(parts[i + 1]);
					i += 1;
				}
				if (parts[i] == "movetime") {
					params.movetime = stoi(parts[i + 1]);
					i += 1;
				}
			}
			Constraints = CalculateConstraints(params, board.Turn);
			Search(board);
			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "Renegade UCI interface ended" << endl;
}

void Engine::PrintInfo(Evaluation e) {
	cout << "info depth " << e.depth /* << " seldepth " << e.seldepth*/ << " score cp " << e.score << " nodes " << e.nodes << /* " qnodes " << e.qnodes << */ " nps " << e.nps
		<< " time " << e.time << " hashfull " << e.hashfull << " pv";
	
	for (Move move : e.pv)
		cout << " " << move.ToString();
	cout << endl;
}

void Engine::Play() {
	Board board = Board(starting_fen);

	cout << "c - Computer, h - Human" << endl;
	cout << "White player? ";
	char white;
	cin >> white;
	cout << "Black player? ";
	char black;
	cin >> black;

	if ((white != 'c') && (white != 'h')) return;
	if ((black != 'c') && (black != 'h')) return;

	while (board.State == GameState::Playing) {
		cout << "\033[2J\033[1;1H" << endl;
		board.Draw(0);

		char player = board.Turn ? white : black;

		if (player == 'h') {
			// Player
			std::string str;
			bool success = false;
			while (!success) {
				cout << "Move to play? ";
				cin >> str;
				success = board.PushUci(str);
			}
		} else {
			Evaluation e = Search(board);
			Constraints = {3000, 3000, -1, -1};
			board.Push(e.BestMove());
			cout << "Renegade plays " << e.BestMove().ToString() << endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		}

	}

}

void Engine::InitOpeningBook() {
	std::ifstream ifs("book.bin", std::ios::in | std::ios::binary);

	if (!ifs) return;

	unsigned __int64 buffer[2];
	int c = 0;

	while (ifs.read(reinterpret_cast<char*>(&buffer), 16)) {
		BookEntry entry;
		int b = _byteswap_ushort(0xFFFF & buffer[1]);
		entry.hash = _byteswap_uint64(buffer[0]);
		entry.to =        (0b000000000111111 & b) >> 0;
		entry.from =      (0b000111111000000 & b) >> 6;
		entry.promotion = (0b111000000000000 & b) >> 12;
		entry.weight = 0;
		entry.learn = 0;
		BookEntries.push_back(entry);
	}
	ifs.close();


}

std::string Engine::GetBookMove(unsigned __int64 hash) {
	// should take about 2-3 ms for Human.bin (~900k entries) 

	std::vector<string> matches;
	for (const BookEntry &e : BookEntries) {
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