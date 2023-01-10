#include "Engine.h"

Engine::Engine() {
	Search.Reset();
	Settings = EngineSettings();
	Settings.Hash = 64;
	Settings.UseBook = false;
	Settings.ExtendedOutput = false;
}


// Start UCI protocol
void Engine::Start() {
	Board board = Board(starting_fen);
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
	std::string cmd = "";
	while (getline(cin, cmd)) {

		cmd = Trim(cmd);
		if (cmd.size() == 0) continue;
		std::vector<std::string> parts = Split(cmd);

		if (cmd == "quit") break;

		if (cmd == "uci") {
			cout << "id name Renegade " << Version << endl;
			cout << "id author Krisztian Peocz" << endl;
			cout << "option name Hash type spin default 64 min 0 max 256" << endl;
			cout << "option name OwnBook type check default false" << endl;
			cout << "option name ExtendedOutput type check default false" << endl;
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

		if (cmd == "stop") {
			// todo: handle this
			continue;
		}

		if (cmd == "tuner") {
			Tuning Tuner("d");
			continue;
		}

		// Set option
		if (parts[0] == "setoption") {

			ConvertToLowercase(parts[2]);

			if (parts[2] == "ownbook") {
				ConvertToLowercase(parts[4]);
				if (parts[4] == "true") Settings.UseBook = true;
				else if (parts[4] == "false") Settings.UseBook = false;
				else cout << "Unknown value: '" << parts[4] << "'" << endl;
			}
			else if (parts[2] == "extendedoutput") {
				ConvertToLowercase(parts[4]);
				if (parts[4] == "true") Settings.ExtendedOutput = true;
				else if (parts[4] == "false") Settings.ExtendedOutput = false;
				else cout << "Unknown value: '" << parts[4] << "'" << endl;
			}
			else if (parts[2] == "hash") {
				Settings.Hash = stoi(parts[4]);
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
			if (parts[1] == "whitepassedpawn") {
				int sq = stoi(parts[2]);
				board.Draw(WhitePassedPawnMask[sq]);
				board.Draw(WhitePassedPawnFilter[sq]);
			}
			if (parts[1] == "blackpassedpawn") {
				int sq = stoi(parts[2]);
				board.Draw(BlackPassedPawnMask[sq]);
				board.Draw(BlackPassedPawnFilter[sq]);
			}
			if (parts[1] == "enpassant") cout << "En passant target: " << board.EnPassantSquare << endl;
			if (parts[1] == "halfmovecounter") cout << "Half move counter: " << board.HalfmoveClock << endl;
			if (parts[1] == "fullmovecounter") cout << "Full move counter: " << board.FullmoveClock << endl;
			if (parts[1] == "pseudolegal") {
				std::vector<Move> v;
				board.GeneratePseudoLegalMoves(v, board.Turn, false);
				for (Move m : v) cout << m.ToString() << " ";
				cout << endl;
			}
			if (parts[1] == "hash") {
				cout << "Hash (polyglot): " << std::hex << board.Hash(false) << endl;
				cout << "Hash (custom):   " << board.Hash(true) << std::dec << endl;
			}
			if (parts[1] == "book") {
				if (parts[2] == "count") {
					cout << "Book entries: " << Search.GetBookSize() << " (from: " << std::filesystem::current_path().string() << ")" << endl;
				}
				else if (parts[2] == "entry") {
					int n = stoi(parts[3]);
					if (n < Search.GetBookSize()) {
						BookEntry entry = Search.GetBookEntry(n);
						cout << "Entry " << n << ": " << std::hex << entry.hash << std::dec << PolyglotMoveToString(entry.from, entry.to, entry.promotion) << endl;
					}
					else {
						cout << "Entry index is out of range!" << endl;
					}
				}
				else {
					std::string s = Search.GetBookMove(board.Hash(false));
					if (s != "") cout << "Book move: " << s << endl;
					else cout << "No book move found" << endl;
				}
				
			}
			if (parts[1] == "settings") {
				cout << "Hash: " << Settings.Hash << endl;
				cout << "OwnBook: " << Settings.UseBook << endl;
				cout << "ExtendedOutput: " << Settings.ExtendedOutput << endl;
			}
			if (parts[1] == "sizeof") {
				cout << "sizeof HashEntry:         " << sizeof(HashEntry) << endl;
				cout << "sizeof Move:              " << sizeof(Move) << endl;
				cout << "sizeof std::vector<Move>: " << sizeof(std::vector<Move>) << endl;
				cout << "sizeof int:               " << sizeof(int) << endl;
			}
			if (parts[1] == "eval") {
				cout << "Static evaluation: " << EvaluateBoard(board, 0) << endl;
			}
			if (parts[1] == "pasthashes") {
				cout << "Past hashes size: " << board.PastHashes.size() << endl;
				for (int i = 0; i < board.PastHashes.size(); i++) {
					cout << "- entry " << i << ": " << std::hex << board.PastHashes[i] << std::dec << endl;
				}
			}
			if (parts[1] == "phase") {
				cout << "Game phase: " << CalculateGamePhase(board) << endl;
			}
			if (parts[1] == "weight") {
				const int id = stoi(parts[2]);
				cout << "Weight: " << id << "/" << WeightsSize << ": " << Weights[id] << endl;
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
				if ((parts.size() > 2) && (parts[2] == "moves")) {
					for (int i = 3; i < parts.size(); i++) {
						bool r = board.PushUci(parts[i]);
						if (!r) cout << "!!! Error: invalid pushuci move: '" << parts[i] << "' !!!" << endl;
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

			if ((parts[1] == "perft") || (parts[1] == "perftdiv") || (parts[1] == "perfd")) {
				int depth = stoi(parts[2]);
				PerftType type = PerftType::Normal;
				if (parts[1] == "perftdiv") type = PerftType::PerftDiv;
				if (parts[1] == "perfd") type = PerftType::Debug;
				Search.Perft(board, depth, type);
				continue;
			}

			SearchParams params;
			for (uint64_t i = 1; i < parts.size(); i++) {
				if (parts[i] == "wtime") { params.wtime = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "btime") { params.btime = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "movestogo") { params.movestogo = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "winc") { params.winc = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "binc") { params.binc = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "nodes") { params.nodes = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "depth") { params.depth = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "movetime") { params.movetime = stoi(parts[i + 1ull]); i++; }
			}

			Search.SearchMoves(board, params, Settings);
			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "Renegade UCI interface ended" << endl;
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
			SearchParams p;
			p.depth = 5;
			EngineSettings s;
			s.Hash = 2;
			s.UseBook = false;
			Evaluation e = Search.SearchMoves(board, p, s);
			board.Push(e.BestMove());
			cout << "Renegade plays " << e.BestMove().ToString() << endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		}

	}

}