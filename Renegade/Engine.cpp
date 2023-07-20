#include "Engine.h"

Engine::Engine(int argc, char* argv[]) {
	Settings = EngineSettings();
	Settings.Hash = 64;
	Settings.UseBook = false;
	Settings.ExtendedOutput = false;
	Settings.Colorful = true;
	Settings.UciOutput = !PrettySupport;
	std::srand(static_cast<unsigned int>(std::time(0)));
	GenerateMagicTables();
	Search.Heuristics.SetHashSize(Settings.Hash);
	Search.Heuristics.ClearTranspositionTable();

	if ((argc == 2) && (std::string(argv[1]) == "bench")) HandleBench();
	else PrintHeader();
}

void Engine::PrintHeader() {
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
}

// Start UCI protocol
void Engine::Start() {
	Board board = Board(FEN::StartPos);
	std::string cmd = "";

	while (getline(cin, cmd)) {

		cmd = Trim(cmd);
		if (cmd.size() == 0) continue;
		std::vector<std::string> parts = Split(cmd);

		if (cmd == "quit") break;

		if (cmd == "uci") {
			cout << "id name Renegade " << Version << '\n';
			cout << "id author Krisztian Peocz" << '\n';
			cout << "option name Clear Hash type button" << '\n';
			cout << "option name Hash type spin default 64 min 1 max 4096" << '\n';
			cout << "option name OwnBook type check default false" << '\n';
			cout << "uciok" << endl;
			Settings.UciOutput = true;
			continue;
		}

		if (cmd == "isready") {
			cout << "readyok" << endl;
			continue;
		}

		if (cmd == "ucinewgame") {
			ResetState();
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
				ConvertToLowercase(parts[3]);
				if (parts[3] == "hash") {
					ResetState();
					valid = true;
				}
			}
			
			if (!valid) cout << "Invalid option: '" << parts[2] << "'" << endl;
			
			continue;

		}

		// Debug commands
		if (parts[0] == "debug") {
			if (parts[1] == "attackmap") DrawBoard(board, board.CalculateAttackedSquares(TurnToPieceColor(board.Turn)));
			if (parts[1] == "enpassant") cout << "En passant target: " << board.EnPassantSquare << endl;
			if (parts[1] == "halfmovecounter") cout << "Half move counter: " << board.HalfmoveClock << endl;
			if (parts[1] == "fullmovecounter") cout << "Full move counter: " << board.FullmoveClock << endl;
			if (parts[1] == "plys") cout << "Game plys: " << board.GetPlys() << endl;
			if (parts[1] == "pseudolegal") {
				std::vector<Move> pseudoMoves;
				board.GenerateMoves(pseudoMoves, MoveGen::All, Legality::Pseudolegal);
				for (const Move &m : pseudoMoves) cout << m.ToString() << " ";
				cout << endl;
			}
			if (parts[1] == "hash") {
				cout << "Hash (polyglot): " << std::hex << board.Hash() << endl;
			}
			if (parts[1] == "book") {
				if (parts[2] == "count") {
					cout << "Book entries: " << Search.GetBookSize() << endl;
					// "from: " << std::filesystem::current_path().string()
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
				cout << "sizeof Board:              " << sizeof(Board) << endl;
				cout << "sizeof Move:               " << sizeof(Move) << endl;
				cout << "sizeof int:                " << sizeof(int) << endl;
			}
			if (parts[1] == "pasthashes") {
				cout << "Past hashes size: " << board.PreviousHashes.size() << endl;
				for (int i = 0; i < board.PreviousHashes.size(); i++) {
					cout << "- entry " << i << ": " << std::hex << board.PreviousHashes[i] << std::dec << endl;
				}
			}
			if (parts[1] == "phase") {
				cout << "Game phase: " << CalculateGamePhase(board) << endl;
			}
			if (parts[1] == "weight") {
				const int id = stoi(parts[2]);
				cout << "Weight: " << id << "/" << Weights.WeightSize << ": "
					<< "S(" << Weights.Weights[id].early << ", " << Weights.Weights[id].late << ")" << endl;
			}
			if (parts[1] == "hashalloc") {
				uint64_t ttTheoretical, ttUsable, ttBits, ttUsed;
				Search.Heuristics.GetTranspositionInfo(ttTheoretical, ttUsable, ttBits, ttUsed);
				cout << "Theoretical transposition size: " << ttTheoretical << endl;
				cout << "Usable transposition size:      " << ttUsable << endl;
				cout << "Usable bits:                    " << ttBits << endl;
				cout << "Used transposition entry count: " << ttUsed << endl;
				cout << "Bytes per entry:                " << sizeof(TranspositionEntry) << endl;

			}
			if (parts[1] == "isdraw") {
				cout << "Is drawn? " << board.IsDraw(true) << endl;
			}
			if (parts[1] == "see") {
				const Move m = Move(stoi(parts[2]), stoi(parts[3]), stoi(parts[4]));
				cout << "SEE: " << Search.StaticExchangeEval(board, m, stoi(parts[5])) << endl;

				int threshold = -2000;
				while (Search.StaticExchangeEval(board, m, threshold)) {
					threshold += 1;
				}
				cout << "SEE toggle threshold: " << threshold << endl;
			}
			if (parts[1] == "sqtoi") {
				for (int i = 2; i < parts.size(); i++)
					cout << parts[i] << " -> " << static_cast<int>(SquareToNum(parts[i])) << endl;
			}
			continue;
		}

		// Custom commands
		if (parts[0] == "help") {
			HandleHelp();
			continue;
		}
		if (parts[0] == "draw") {
			DrawBoard(board);
			continue;
		}
		if (parts[0] == "eval") {
			cout << "Static evaluation: " << EvaluateBoard(board) << " (for the side to come)" << endl;
			continue;
		}
		if (parts[0] == "fen") {
			cout << "Position FEN: " << board.GetFEN() << endl;
			continue;
		}
		if (parts[0] == "clear") {
			ClearScreen(false, Settings.Colorful);
			PrintHeader();
			continue;
		}
		if (parts[0] == "fancy") {
			Settings.Colorful = true;
			continue;
		}
		if (parts[0] == "plain") {
			Settings.Colorful = false;
			continue;
		}
		if (parts[0] == "ch") {
			ResetState();
			cout << "Transposition table cleared." << endl;
			continue;
		}
		if (parts[0] == "test") {
			cout << "Test trigger point" << endl;
			continue;
		}
		if (parts[0] == "bighash") {
			Search.Heuristics.SetHashSize(1024);
			cout << "Using big hash: 1024 MB" << endl;
			continue;
		}
		if (parts[0] == "hugehash") {
			Search.Heuristics.SetHashSize(4096);
			cout << "Using huge hash: 4096 MB" << endl;
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

			if ((parts.size() == 3) && ((parts[1] == "perft") || (parts[1] == "perftdiv"))) {
				int depth = stoi(parts[2]);
				PerftType type = PerftType::Normal;
				if (parts[1] == "perftdiv") type = PerftType::PerftDiv;
				Search.Perft(board, depth, type);
				continue;
			}

			SearchParams params;
			for (int i = 1; i < parts.size(); i++) {
				if (parts[i] == "wtime") { params.wtime = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "btime") { params.btime = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "movestogo") { params.movestogo = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "winc") { params.winc = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "binc") { params.binc = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "nodes") { params.nodes = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "depth") { params.depth = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "mate") { params.depth = stoi(parts[i + 1LL]); i++; } // To do: search for mates only
				if (parts[i] == "movetime") { params.movetime = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "searchmoves") { cout << "info string Searchmoves parameter is not yet implemented!" << endl; }
			}

			Search.SearchMoves(board, params, Settings, true);
			continue;
		}

		if (parts[0] == "bench") {
			HandleBench();
			continue;
		}

		if (parts[0] == "compiler") {
			HandleCompiler();
			continue;
		}

		if (parts[0] == "flip") {
			cout << "Turn flipped." << endl;
			board.Turn = !board.Turn;
			continue;
		}

		if (parts[0] == "goall") {
			std::vector<Move> moves;
			board.GenerateMoves(moves, MoveGen::All, Legality::Legal);
			cout << moves.size() << " legal moves";
			int time = 1000;
			if (parts.size() > 1) {
				time = stoi(parts[1]);
			}
			SearchParams params;
			params.movetime = time;
			EngineSettings settings;
			settings.Hash = 16;
			settings.ExtendedOutput = false;
			settings.UseBook = false;
			Search.Heuristics.ClearTranspositionTable();
			std::vector<std::tuple<Move, int>> scores;
			for (Move& m : moves) {
				Board b = Board(board);
				Search.Heuristics.ClearKillerAndCounterMoves();
				Search.Heuristics.ClearHistoryTable();
				Search.Heuristics.ClearPvLine();
				Search.Heuristics.ResetPvTable();
				b.Push(m);
				Results r = Search.SearchMoves(b, params, settings, false);
				scores.push_back(std::tuple<Move, int>(m, r.score));
				cout << ".";
			}
			cout << endl;
			std::sort(scores.begin(), scores.end(), [](auto const& t1, auto const& t2) {
				return get<1>(t1) < get<1>(t2);
				});
			for (std::tuple<Move, int>& s: scores) {
				cout << " - " << get<0>(s).ToString() << " : " << -get<1>(s) << endl;
			}
			cout << "Complete." << endl;
			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "Stopping engine." << endl;
}

void Engine::DrawBoard(const Board &b, const uint64_t customBits) const {

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

			std::string cellStyle;

			if ((i + j) % 2 == 1) {
				if (pieceColor == PieceColor::Black) cellStyle = BlackOnLightSquare;
				else cellStyle = WhiteOnLightSquare;
			}
			else {
				if (pieceColor == PieceColor::Black) cellStyle = BlackOnDarkSquare;
				else cellStyle = WhiteOnDarkSquare;
			}

			if (CheckBit(customBits, i * 8 + j)) {
				if (pieceColor == PieceColor::Black) cellStyle = BlackOnTarget;
				else cellStyle = WhiteOnTarget;
			}

			if (Settings.Colorful) cout << cellStyle << ' ' << piece << ' ' << Default;
			else cout << ' ' << piece << ' ';

		}
		cout << "|" << '\n';
	}
	cout << "    ------------------------ " << '\n';
	cout << "     a  b  c  d  e  f  g  h" << endl;

}

void Engine::HandleBench() {
	uint64_t nodes = 0;
	SearchParams params;
	params.depth = 14;
	EngineSettings settings;
	const int oldHashSize = Settings.Hash;
	Search.Heuristics.SetHashSize(16);
	settings.ExtendedOutput = false;
	settings.UseBook = false;
	auto startTime = Clock::now();
	for (const std::string& fen : BenchmarkFENs) {
		Search.Heuristics.ClearKillerAndCounterMoves();
		Search.Heuristics.ClearHistoryTable();
		Search.Heuristics.ClearPvLine();
		Search.Heuristics.ResetPvTable();
		Board b = Board(fen);
		Results r = Search.SearchMoves(b, params, settings, false);
		nodes += r.stats.Nodes;
	}
	auto endTime = Clock::now();
	int nps = static_cast<int>(nodes / ((endTime - startTime).count() / 1e9));
	cout << nodes << " nodes " << nps << " nps" << endl;
	Search.Heuristics.SetHashSize(oldHashSize); // also clears the transposition table
}

void Engine::HandleCompiler() const {
	cout << endl;
#if defined(__clang__)
	cout << "Compiler: clang" << endl;
	cout << "Version: " << __clang_major__ << endl;
#elif defined(__GNUC__) || defined(__GNUG__)
	cout << "Compiler: gcc" << endl;
	cout << "Version: " << __GNUC__ << endl;
#elif defined(_MSC_VER)
	cout << "Compiler: MSVC" << endl;
	cout << "Version: " << _MSC_VER << endl;
#elif
	cout << "Interesting compiler you've got there!" << endl;
#endif
	cout << endl;
}

void Engine::HandleHelp() const {
	cout << "\nRenegade is a chess engine written in C++. It is a command line "
		<< "application supporting the UCI protocol, for example 'position startpos' "
		<< "sets up the board and 'go depth 5' initiates a 5 ply deep search.\n" 
		<< "Read up on the UCI protocol for more information." << endl;
	cout << "There are some additional commands supported as well, including: "
		<< "\n- draw: draws the current board"
		<< "\n- fancy & plain: sets board drawing style"
		<< "\n- eval: prints the static evaluation of the position"
		<< "\n- fen: displays the current position's FEN string"
		<< "\n- go perft [n] & go perftdiv [n]: retuns the number of possible positions after n plys (incl. duplicates)" << endl;
}

void Engine::ResetState() {
	Search.Heuristics.ClearTranspositionTable();
	Search.Heuristics.ClearKillerAndCounterMoves();
	Search.Heuristics.ResetPvTable();
	Search.Heuristics.ClearPvLine();
	Search.Heuristics.ClearHistoryTable();
}