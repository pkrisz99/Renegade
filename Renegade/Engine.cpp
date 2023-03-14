#include "Engine.h"

Engine::Engine() {
	Settings = EngineSettings();
	Settings.Hash = 64;
	Settings.UseBook = false;
	Settings.ExtendedOutput = false;
	std::srand(static_cast<unsigned int>(std::time(0)));
	GenerateMagicTables();
	Search.Heuristics.SetHashSize(Settings.Hash);
	Search.Heuristics.ClearTranspositionTable();
}

const void Engine::PrintHeader() {
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
}

// Start UCI protocol
void Engine::Start() {
	Board board = Board(FEN::StartPos);
	PrintHeader();
	std::string cmd = "";
	while (getline(cin, cmd)) {

		cmd = Trim(cmd);
		if (cmd.size() == 0) continue;
		std::vector<std::string> parts = Split(cmd);

		if (cmd == "quit") break;

		if (cmd == "uci") {
			cout << "id name Renegade " << Version << endl;
			cout << "id author Krisztian Peocz" << endl;
			cout << "option name Clear Hash type button" << endl;
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
			Search.Heuristics.ClearTranspositionTable();
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
			Tuning tuner = Tuning();
			continue;
		}

		// Set option
		if (parts[0] == "setoption") {

			ConvertToLowercase(parts[2]);
			bool valid = false;

			if (parts[2] == "ownbook") {
				ConvertToLowercase(parts[4]);
				if (parts[4] == "true") Settings.UseBook = true;
				else if (parts[4] == "false") Settings.UseBook = false;
				else cout << "Unknown value: '" << parts[4] << "'" << endl;
				valid = true;
			}
			else if (parts[2] == "extendedoutput") {
				ConvertToLowercase(parts[4]);
				if (parts[4] == "true") Settings.ExtendedOutput = true;
				else if (parts[4] == "false") Settings.ExtendedOutput = false;
				else cout << "Unknown value: '" << parts[4] << "'" << endl;
				valid = true;
			}
			else if (parts[2] == "hash") {
				Settings.Hash = stoi(parts[4]);
				Search.Heuristics.SetHashSize(Settings.Hash);
				valid = true;
			}
			else if (parts[2] == "clear") {
				if (parts[3] == "hash") {
					Search.Heuristics.ClearTranspositionTable();
					valid = true;
				}
			}
			
			if (!valid) cout << "Invalid option: '" << parts[2] << "'" << endl;
			
			continue;

		}

		// Debug commands
		if (parts[0] == "debug") {
			if (parts[1] == "attackmap") DrawBoard(board, board.AttackedSquares);
			if (parts[1] == "whitepassedpawn") {
				int sq = stoi(parts[2]);
				DrawBoard(board, WhitePassedPawnMask[sq]);
				DrawBoard(board, WhitePassedPawnFilter[sq]);
			}
			if (parts[1] == "blackpassedpawn") {
				int sq = stoi(parts[2]);
				DrawBoard(board, BlackPassedPawnMask[sq]);
				DrawBoard(board, BlackPassedPawnFilter[sq]);
			}
			if (parts[1] == "enpassant") cout << "En passant target: " << board.EnPassantSquare << endl;
			if (parts[1] == "halfmovecounter") cout << "Half move counter: " << board.HalfmoveClock << endl;
			if (parts[1] == "fullmovecounter") cout << "Full move counter: " << board.FullmoveClock << endl;
			if (parts[1] == "plys") cout << "Game plys: " << board.GetPlys() << endl;
			if (parts[1] == "pseudolegal") {
				std::vector<Move> v;
				board.GeneratePseudoLegalMoves(v, board.Turn, false);
				for (Move m : v) cout << m.ToString() << " ";
				cout << endl;
			}
			if (parts[1] == "hash") {
				cout << "Hash (polyglot): " << std::hex << board.Hash() << endl;
			}
			if (parts[1] == "book") {
				if (parts[2] == "count") {
					cout << "Book entries: " << Search.GetBookSize() << " (from: " << std::filesystem::current_path().string() << ")" << endl;
				}
				else if (parts[2] == "entry") {
					int n = stoi(parts[3]);
					if (n < Search.GetBookSize()) {
						BookEntry entry = Search.GetBookEntry(n);
						cout << "Entry " << n << ": " << std::hex << entry.hash << std::dec << PolyglotMoveToString(entry.from, entry.to, entry.promotion)
							<< " (weight = " << entry.weight << ")" << endl;
					}
					else {
						cout << "Entry index is out of range!" << endl;
					}
				}
				else {
					std::string s = Search.GetBookMove(board.Hash());
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
				cout << "sizeof TranspositionEntry: " << sizeof(TranspositionEntry) << endl;
				cout << "sizeof Move:               " << sizeof(Move) << endl;
				cout << "sizeof int:                " << sizeof(int) << endl;
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
			if (parts[1] == "hashalloc") {
				uint64_t trTheoretical, trUsable, trBits, trUsed;
				Search.Heuristics.GetTranspositionInfo(trTheoretical, trUsable, trBits, trUsed);
				cout << "Theoretical transposition size: " << trTheoretical << endl;
				cout << "Usable transposition size:      " << trUsable << endl;
				cout << "Usable bits:                    " << trBits << endl;
				cout << "Used transposition entry count: " << trUsed << endl;
				cout << "Bytes per entry:                " << sizeof(TranspositionEntry) << endl;

			}
			if (parts[1] == "isdraw") {
				cout << "Is draw? " << board.IsDraw() << endl;
			}
			continue;
		}

		// Custom commands
		if (parts[0] == "help") {
			cout << "\nRenegade is a basic chess engine written in C++. It is a command line "
				<< "application supporting the UCI protocol, for example 'position startpos' "
				<< "sets up the board and 'go depth 5' initiates a 5 ply deep search." << endl;
			cout << "There are some additional commands supported as well, including: "
				<< "\n- draw: draws the current board"
				<< "\n- fancy & plain: sets board drawing style"
				<< "\n- eval: prints the static evaluation of the position"
				<< "\n- fen: displays the current position's FEN string"
				<< "\n- go perft [n] & go perftdiv [n]: retuns the number of possible positions after n plys (incl. duplicates)" << endl;
			continue;
		}
		if (parts[0] == "draw") {
			DrawBoard(board);
			continue;
		}
		if (parts[0] == "eval") {
			cout << "Static evaluation: " << EvaluateBoard(board, 0) << " (for the side to come)" << endl;
			continue;
		}
		if (parts[0] == "fen") {
			cout << "Position FEN: " << board.GetFEN() << endl;
			continue;
		}
		if (parts[0] == "clear") {
			ClearScreen(false, Fancy);
			PrintHeader();
			continue;
		}
		if (parts[0] == "fancy") {
			Fancy = true;
			continue;
		}
		if (parts[0] == "plain") {
			Fancy = false;
			continue;
		}

		// Position command
		if (parts[0] == "position") {

			if ((parts[1] == "startpos") || (parts[1] == "kiwipete") || (parts[1] == "lasker")) {
				if (parts[1] == "startpos") board = Board(FEN::StartPos);
				else if (parts[1] == "kiwipete") board = Board(FEN::Kiwipete);
				else if (parts[1] == "lasker") board = Board(FEN::Lasker);

				if ((parts.size() > 2) && (parts[2] == "moves")) {
					for (int i = 3; i < parts.size(); i++) {
						bool r = board.PushUci(parts[i]);
						if (!r) cout << "!!! Error: invalid pushuci move: '" << parts[i] << "' !!!" << endl;
					}
				}
			}

			if (parts[1] == "fen") {
				std::string fen;
				if (parts.size() >= 8)
					fen = parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " " + parts[6] + " " + parts[7];
				else
					fen = parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " 0 1";
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
				if (parts[i] == "mate") { params.depth = stoi(parts[i + 1ull]); i++; } // To do: search for mates only
				if (parts[i] == "movetime") { params.movetime = stoi(parts[i + 1ull]); i++; }
				if (parts[i] == "searchmoves") { cout << "info string Searchmoves parameter is not yet implemented!" << endl; }
			}

			Search.SearchMoves(board, params, Settings);
			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "Stopping engine." << endl;
}

const void Engine::DrawBoard(Board b, uint64_t customBits) {

	const std::string side = b.Turn ? "white" : "black";
	cout << "    Move: " << b.FullmoveClock << " - " << side << " to play" << endl;;

	const std::string WhiteOnLightSquare = "\033[31;47m";
	const std::string WhiteOnDarkSquare = "\033[31;43m";
	const std::string BlackOnLightSquare = "\033[30;47m";
	const std::string BlackOnDarkSquare = "\033[30;43m";
	const std::string Default = "\033[0m";
	const std::string WhiteOnTarget = "\033[31;45m";
	const std::string BlackOnTarget = "\033[30;45m";

	cout << "    ------------------------ " << endl;
	// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
	for (int i = 7; i >= 0; i--) {
		cout << " " << i + 1 << " |";
		for (int j = 0; j <= 7; j++) {
			const int pieceId = b.GetPieceAt(i * 8 + j);
			const int pieceColor = ColorOfPiece(pieceId);
			char piece = PieceChars[pieceId];

			std::string CellStyle;

			if ((i + j) % 2 == 1) {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnLightSquare;
				else CellStyle = WhiteOnLightSquare;
			}
			else {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnDarkSquare;
				else CellStyle = WhiteOnDarkSquare;
			}

			if (CheckBit(customBits, static_cast<uint64_t>(i) * 8 + static_cast<uint64_t>(j))) {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnTarget;
				else  CellStyle = WhiteOnTarget;
			}

			if (Fancy) cout << CellStyle << ' ' << piece << ' ' << Default;
			else cout << ' ' << piece << ' ';

		}
		cout << "|" << '\n';
	}
	cout << "    ------------------------ " << '\n';
	cout << "     a  b  c  d  e  f  g  h" << endl;

}

void Engine::Play() {
	Board board = Board(FEN::StartPos);

	cout << "c - Computer, h - Human" << endl;
	cout << "White player? ";
	char white;
	int whiteMovetime = 200;
	cin >> white;
	if ((white != 'c') && (white != 'h')) {
		cout << "Invalid player." << endl;
		return;
	}
	if (white == 'c') {
		cout << "How long should the computer think in milliseconds? ";
		cin >> whiteMovetime;
	}

	cout << "Black player? ";
	char black;
	cin >> black;
	int blackMovetime = 200;
	if ((black != 'c') && (black != 'h')) {
		cout << "Invalid player." << endl;
		return;
	}
	if (black == 'c') {
		cout << "How long should the computer think in milliseconds? ";
		cin >> blackMovetime;
	}

	bool quitting = false;
	while ((board.GetGameState() == GameState::Playing) && !quitting) {
		ClearScreen(false, Fancy);
		PrintHeader();
		cout << "Playing a game through console: \n" << endl;
		DrawBoard(board);

		char player = board.Turn ? white : black;

		if (player == 'h') {
			// Human
			std::string str;
			bool success = false;
			cout << "\n";
			if (board.FullmoveClock == 1) {
				cout << "Type input in UCI notation (e.g. e2e4, e1g1, f7f8q)\n";
				cout << "If you want to return to the main interface, write 'quit'\n";
			}
			while (!success) {
				cout << "Move to play ? ";
				cin >> str;
				success = board.PushUci(str);
				if (str == "fancy") Fancy = true;
				if (str == "plain") Fancy = false;
				if (str == "quit") {
					quitting = true;
					success = true;
				}
			}
		} else {
			// Computer
			SearchParams p;
			EngineSettings s;
			p.movetime = board.Turn ? whiteMovetime : blackMovetime;
			s.Hash = 8;
			s.UseBook = false;
			s.ExtendedOutput = false;
			cout << '\n';
			Results e = Search.SearchMoves(board, p, s);
			board.Push(e.BestMove());
			cout << "\nEngine plays " << e.BestMove().ToString() << endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		}

	}

	if (!quitting) cout << "Game over: " << StateString(board.GetGameState()) << '\n' << endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	ClearScreen(false, Fancy);
	PrintHeader();

}