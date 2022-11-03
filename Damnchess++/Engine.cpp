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
	std::vector<Move> moves = board.GenerateLegalMoves();
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
	std::vector<Move> moves = b.GenerateLegalMoves();
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

	std::vector<Move> legalMoves = board.GenerateLegalMoves();
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
		if (board.Turn == WhiteTurn) return MateEval;
		if (board.Turn == BlackTurn) return -MateEval;
	} else if (board.State == GameState::BlackVictory) {
		if (board.Turn == WhiteTurn) return -MateEval;
		if (board.Turn == BlackTurn) return MateEval;
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
		if (piece == WhitePawn) score += PawnPSQT[i];
		if (piece == WhiteKnight) score += KnightPSQT[i];
		if (piece == WhiteBishop) score += BishopPSQT[i];
		if (piece == WhiteRook) score += RookPSQT[i];
		if (piece == WhiteQueen) score += QueenPSQT[i];

		if (piece == BlackPawn) score -= PawnPSQT[63 - i];
		if (piece == BlackKnight) score -= KnightPSQT[63 - i];
		if (piece == BlackBishop) score -= BishopPSQT[63 - i];
		if (piece == BlackRook) score -= RookPSQT[63 - i];
		if (piece == BlackQueen) score -= QueenPSQT[63 - i];
	}

	if (!board.Turn) score *= -1;
	return score;
}

// Start UCI protocol
void Engine::Start() {
	cout << "Damnchess++ by Krisz, 2022" << endl;
	cout << "UCI interface begin" << endl;
	std::string cmd = "";
	Board board = Board(starting_fen);
	while (getline(cin, cmd)) {
		
		cmd = trimstr(cmd);
		if (cmd.size() == 0) continue;

		if (cmd == "quit") break;

		if (cmd == "uci") {
			cout << "id name Damnchess++" << endl;
			cout << "id author Krisz" << endl;
			cout << "uciok" << endl;
		}

		if (cmd == "isready") {
			cout << "readyok" << endl;
		}

		if (cmd == "ucinewgame") {}

		if (cmd == "play") Play();

		if (startsWith(cmd, "debug ")) {
			if (cmd == "debug attackmap") board.Draw(board.AttackedSquares);
			if (cmd == "debug whitepawn") board.Draw(board.WhitePawnBits);
			if (cmd == "debug whiteking") board.Draw(board.WhiteKingBits);
			if (cmd == "debug blackpawn") board.Draw(board.BlackPawnBits);
			if (cmd == "debug blackking") board.Draw(board.BlackKingBits);
			if (cmd == "debug enpassant") cout << "En passant target: " << board.EnPassantSquare << endl;
		}


		if (startsWith(cmd, "position startpos")) {
			std::string trimmedStr = cmd.substr(17);
			board = Board(starting_fen);
			if (startsWith(trimmedStr, " moves ")) {
				std::string moveStr = trimmedStr.substr(6);
				std::stringstream ss(moveStr);
				std::istream_iterator<std::string> begin(ss);
				std::istream_iterator<std::string> end;
				std::vector<std::string> parts(begin, end);
				for (string s : parts) {
					bool r = board.PushUci(s);
					if (!r) cout << "!!!!!!!!!!!!!!!!" << endl;
				}
			}
		}

		if (startsWith(cmd, "position fen ")) {
			std::string trimmedStr = cmd.substr(13);

			int pos = trimmedStr.find(" moves ");
			std::string fenStr = "";
			bool hasMoves = false;
			if (pos != std::string::npos) {
				fenStr = trimmedStr.substr(0, pos);
				hasMoves = true;
			}
			else {
				fenStr = trimmedStr;
			}
			board = Board(fenStr);

			if (hasMoves) {
				std::string moveStr = trimmedStr.substr(pos + 6);
				std::stringstream ss(moveStr);
				std::istream_iterator<std::string> begin(ss);
				std::istream_iterator<std::string> end;
				std::vector<std::string> parts(begin, end);

				for (std::string s : parts) {
					bool r = board.PushUci(s);
					if (!r) cout << "!!!!!!!!!!!!!!!!" << endl;
				}
			}
		}

		if (startsWith(cmd, "go")) {

			if (startsWith(cmd, "go perft")) {
				int depth = 4;
				std::string perftStr = cmd.substr(9);
				try {
					depth = stoi(perftStr);
				}
				catch (exception e) {}
				perft(board, depth, true);
			} else if (startsWith(cmd, "go perfd")) {
				int depth = 4;
				std::string perftStr = cmd.substr(9);
				try {
					depth = stoi(perftStr);
				}
				catch (exception e) {}
				perft(board, depth, false);
			}
		}

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