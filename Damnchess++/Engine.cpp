#include "Engine.h"

typedef std::chrono::high_resolution_clock Clock;

Engine::Engine() {
	//
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
	float seconds = (t1 - t0).count() / 1e9;
	float speed = r / seconds / 1000;
	if (verbose) cout << "Perft(" << depth << ") = " << r << "  | " << std::setprecision(3) << std::fixed << seconds << " s | " << std::setprecision(1) << speed << "kN/s" << endl;
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
	if (depth == 1) return moves.size();
	int count = 0;
	for (const Move& m : moves) {
		Board child = b.Copy();
		child.Push(m);
		count += perftRecursive(child, depth - 1);
	}
	return count;
}

Evaluation Engine::Search(Board board, int depth) {
	EvaluatedNodes = 0;
	auto t0 = Clock::now();
	eval result = SearchRecursive(board, depth, 0);
	auto t1 = Clock::now();
	double ms = (t1 - t0).count() / 1e6;
	int nps = EvaluatedNodes / ms * 1000;
	Evaluation e(get<0>(result), get<1>(result), EvaluatedNodes, (int) ms, nps);
	return e;
}

eval Engine::SearchRecursive(Board board, int depth, int level) {
	// Generate legal moves
	
	if (depth == 0) {
		return eval{ StaticEvaluation(board), Move(0, 0)};
	}

	int bestScore = NoEval;
	Move bestMove(0,0);

	std::vector<Move> legalMoves = board.GenerateLegalMoves(board.Turn);
	//cout << legalMoves.size() << endl;
	for (const Move &m : legalMoves) {
		Board b = board.Copy();
		b.Push(m);
		eval childEval = SearchRecursive(b, depth - 1, level + 1);
		int childScore = -get<0>(childEval);
		Move childMove = get<1>(childEval);
		if (childScore > bestScore) {
			bestScore = childScore;
			bestMove = m;
		}
	}

	return eval{ bestScore, bestMove };

}

int Engine::StaticEvaluation(Board board) {
	EvaluatedNodes += 1;
	// 1. is over?
	if (board.State == GameState::Draw) return 0;
	if (board.State == GameState::WhiteVictory) {
		if (board.Turn == Turn::White) return MateEval;
		if (board.Turn == Turn::Black) return -MateEval;
	} else if (board.State == GameState::BlackVictory) {
		if (board.Turn == Turn::White) return -MateEval;
		if (board.Turn == Turn::Black) return MateEval;
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

	for (int i = 0; i < 64; i++) {
		int piece = board.GetPieceAt(i);
		if (piece == Piece::WhitePawn) score += PawnPSQT[i];
		if (piece == Piece::WhiteKnight) score += KnightPSQT[i];
		if (piece == Piece::WhiteBishop) score += BishopPSQT[i];
		if (piece == Piece::WhiteRook) score += RookPSQT[i];
		if (piece == Piece::WhiteQueen) score += QueenPSQT[i];

		if (piece == Piece::BlackPawn) score -= PawnPSQT[63 - i];
		if (piece == Piece::BlackKnight) score -= KnightPSQT[63 - i];
		if (piece == Piece::BlackBishop) score -= BishopPSQT[63 - i];
		if (piece == Piece::BlackRook) score -= RookPSQT[63 - i];
		if (piece == Piece::BlackQueen) score -= QueenPSQT[63 - i];
	}

	if (!board.Turn) score *= -1;
	return score;
}

// Start UCI protocol
void Engine::Start() {
	cout << "Damnchess++ by Krisz, 2022  [Build: " << __DATE__ << " " << __TIME__ << "]" << endl;
	cout << "UCI interface begin" << endl;
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
			cout << "id name Damnchess" << endl;
			cout << "id author Krisz" << endl;
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

		// Debug commands
		if (parts[0] == "debug") {
			if (parts[1] == "attackmap") board.Draw(board.AttackedSquares);
			if (parts[1] == "whitepawn") board.Draw(board.WhitePawnBits);
			if (parts[1] == "whiteking") board.Draw(board.WhiteKingBits);
			if (parts[1] == "blackpawn") board.Draw(board.BlackPawnBits);
			if (parts[1] == "blackking") board.Draw(board.BlackKingBits);
			if (parts[1] == "enpassant") cout << "En passant target: " << board.EnPassantSquare << endl;
			if (parts[1] == "pseudolegal") {
				std::vector<Move> v = board.GenerateMoves(board.Turn);
				for (Move m : v) cout << m.ToString() << " ";
				cout << endl;
			}
			continue;
		}

		// Position command
		if (parts[0] == "position") {

			if (parts[1] == "startpos") {
				board = Board(starting_fen);
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
			}

			if (parts[1] == "perfd") {
				int depth = stoi(parts[2]);
				perft(board, depth, false);
			}

			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "UCI interface ended" << endl;
}

void Engine::PrintInfo(Evaluation e) {
	cout << "info depth " << 0 << " score cp " << e.score << " nodes " << e.nodes << " nps " << e.nps << " time " << e.time << endl;
}

void Engine::Play() {
	Board board = Board(starting_fen);

	cout << "Damnchess" << endl;
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
			Evaluation e = Search(board, 1);
			board.Push(e.move);
			cout << "Damnchess plays " << e.move.ToString() << endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		}

	}

}