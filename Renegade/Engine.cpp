#include "Engine.h"

Engine::Engine(int argc, char* argv[]) {
	Settings::UseUCI = !PrettySupport;
	std::srand(static_cast<unsigned int>(std::time(0)));
	GenerateMagicTables();
	LoadDefaultNetwork();
	Search.TranspositionTable.SetSize(Settings::Hash);

	if ((argc == 2) && (std::string(argv[1]) == "bench")) {
		HandleBench(false);
		QuitAfterBench = true;
	}
	else PrintHeader();
}

void Engine::PrintHeader() const {
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
}

// Start UCI protocol
void Engine::Start() {
	if (QuitAfterBench) return;
	Position position = Position(FEN::StartPos);
	std::string cmd;
	SearchParams params;

	while (getline(cin, cmd)) {

		cmd = Trim(cmd);
		if (cmd.size() == 0) continue;
		std::vector<std::string> parts = Split(cmd);

		if (cmd == "quit") break;

		if (cmd == "uci") {
			cout << "id name Renegade " << Version << '\n';
			cout << "id author Krisztian Peocz" << '\n';
			cout << "option name Clear Hash type button" << '\n';
			cout << "option name Hash type spin default " << HashDefault << " min " << HashMin << " max " << HashMax << '\n';
			cout << "option name Threads type spin default 1 min 1 max 1" << '\n';
			cout << "option name UCI_ShowWDL type check default " << (ShowWDLDefault ? "true" : "false") << '\n';
			cout << "option name UCI_Chess960 type check default " << (Chess960Default ? "true" : "false") << '\n';
			if (Tune::Active()) Tune::PrintOptions();
			cout << "uciok" << endl;
			Settings::UseUCI = true;
			continue;
		}

		if (cmd == "isready") {
			cout << "readyok" << endl;
			continue;
		}

		if (cmd == "ucinewgame") {
			Search.ResetState(true);
			continue;
		}

		if (cmd == "stop" || cmd == "s") {
			if (!Search.Aborting.load(std::memory_order_relaxed)) {
				Search.Aborting.store(true, std::memory_order_relaxed);
				SearchThread.join();
			}
			continue;
		}

		if (cmd == "datagen") {
			Datagen datagen = Datagen();
			datagen.Start();
			continue;
		}

		if (cmd == "filter") {
			Datagen datagen = Datagen();
			datagen.LowPlyFilter();
			continue;
		}
		if (cmd == "merge") {
			Datagen datagen = Datagen();
			datagen.MergeFiles();
			continue;
		}

		if (cmd == "tunetext") {
			Tune::GenerateString();
			continue;
		}

		// Set option
		if (parts[0] == "setoption") {

			ConvertToLowercase(parts[2]);
			bool valid = false;

			if (parts[2] == "hash") {
				Settings::Hash = stoi(parts[4]);
				Search.TranspositionTable.SetSize(Settings::Hash);
				valid = true;
			}
			else if (parts[2] == "clear") {
				ConvertToLowercase(parts[3]);
				if (parts[3] == "hash") {
					Search.ResetState(true);
					valid = true;
				}
			}
			else if (parts[2] == "uci_showwdl") {
				ConvertToLowercase(parts[4]);
				if (parts[4] == "true") {
					Settings::ShowWDL = true;
					valid = true;
				}
				else if (parts[4] == "false") {
					Settings::ShowWDL = false;
					valid = true;
				}
			}
			else if (parts[2] == "uci_chess960") {
				ConvertToLowercase(parts[4]);
				if (parts[4] == "true") {
					Settings::Chess960 = true;
					valid = true;
				}
				else if (parts[4] == "false") {
					Settings::Chess960 = false;
					valid = true;
				}
			}
			else if (parts[2] == "threads") {
				valid = true;
			}
			else if (Tune::List.find(parts[2]) != Tune::List.end()) {
				Tune::List.at(parts[2]).value = stoi(parts[4]);
				valid = true;
			}
			
			if (!valid) cout << "Invalid option: '" << parts[2] << "'" << endl;
			
			continue;

		}

		// Debug commands
		if ((parts[0] == "debug") && (parts.size() > 1)) {
			if (parts[1] == "attackmap") {
				position.RequestThreats();
				DrawBoard(position, position.GetThreats());
			}
			if (parts[1] == "enpassant") cout << "En passant target: " << position.CurrentState().EnPassantSquare << endl;
			if (parts[1] == "halfmovecounter") cout << "Half move counter: " << position.CurrentState().HalfmoveClock << endl;
			if (parts[1] == "fullmovecounter") cout << "Full move counter: " << position.CurrentState().FullmoveClock << endl;
			if (parts[1] == "plys") cout << "Game plys: " << position.GetPlys() << endl;
			if (parts[1] == "pseudolegal") {
				MoveList pseudoMoves{};
				position.GenerateMoves(pseudoMoves, MoveGen::All, Legality::Pseudolegal);
				for (const auto& m : pseudoMoves) cout << m.move.ToString(Settings::Chess960) << " ";
				cout << endl;
			}
			if (parts[1] == "legal") {
				MoveList moves{};
				position.GenerateMoves(moves, MoveGen::All, Legality::Legal);
				for (const auto& m : moves) cout << m.move.ToString(Settings::Chess960) << " ";
				cout << endl;
			}
			if (parts[1] == "hash") {
				cout << "Hash (polyglot): " << std::hex << position.Hash() << std::dec << endl;
			}
			if (parts[1] == "settings") {
				cout << std::boolalpha;
				cout << "Hash:      " << Settings::Hash << endl;
				cout << "Show WDL:  " << Settings::ShowWDL << endl;
				cout << "Chess960:  " << Settings::Chess960 << endl;
				cout << "Using UCI: " << Settings::UseUCI << endl;
				cout << std::noboolalpha;
				for (const auto& [name, param] : Tune::List) cout << name << " -> " << param.value << endl;
			}
			if (parts[1] == "sizeof") {
				cout << "sizeof TranspositionEntry: " << sizeof(TranspositionEntry) << endl;
				cout << "sizeof Position:           " << sizeof(position) << endl;
				cout << "sizeof Move:               " << sizeof(Move) << endl;
				cout << "sizeof int:                " << sizeof(int) << endl;
			}
			if (parts[1] == "pasthashes") {
				cout << "Past hashes size: " << position.Hashes.size() << endl;
				for (int i = 0; i < position.Hashes.size(); i++) {
					cout << "- entry " << i << ": " << std::hex << position.Hashes[i] << std::dec << endl;
				}
			}
			if (parts[1] == "phase") {
				cout << "Game phase: " << CalculateGamePhase(position) << endl;
			}
			if (parts[1] == "weight") {
				const int id = stoi(parts[2]);
				cout << "Weight: " << id << "/" << Weights.WeightSize << ": "
					<< "S(" << Weights.Weights[id].early << ", " << Weights.Weights[id].late << ")" << endl;
			}
			if (parts[1] == "hashalloc") {
				uint64_t ttTheoretical, ttUsable, ttBits, ttUsed;
				Search.TranspositionTable.GetInfo(ttTheoretical, ttUsable, ttBits, ttUsed);
				cout << "Theoretical transposition size: " << ttTheoretical << endl;
				cout << "Usable transposition size:      " << ttUsable << endl;
				cout << "Usable bits:                    " << ttBits << endl;
				cout << "Used transposition entry count: " << ttUsed << endl;
				cout << "Bytes per entry:                " << sizeof(TranspositionEntry) << endl;

			}
			if (parts[1] == "isdraw") {
				cout << "Is drawn? " << position.IsDrawn(true) << endl;
			}
			if (parts[1] == "see") {
				const Move m = Move(stoi(parts[2]), stoi(parts[3]), stoi(parts[4]));
				cout << "SEE: " << Search.StaticExchangeEval(position, m, stoi(parts[5])) << endl;

				int threshold = -2000;
				while (Search.StaticExchangeEval(position, m, threshold)) {
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
			uint64_t bitboard = 0;
			if (parts.size() > 1) bitboard = stoull(parts[1]);
			DrawBoard(position, bitboard);
			continue;
		}
		if (parts[0] == "eval") {
			const int hce = ClassicalEvaluate(position);
			const int nnue = NeuralEvaluate(position);
			cout << "Hand-crafted evaluation:   " << ToCentipawns(hce, position.GetPlys()) << " cp  (internal units: " << hce << ")" << endl;
			cout << "Neural network evaluation: " << ToCentipawns(nnue, position.GetPlys()) << " cp  (internal units: " << nnue << ")\n" << endl;
			continue;
		}
		if (parts[0] == "fen") {
			cout << "Position FEN: " << position.GetFEN() << endl;
			continue;
		}
		if (parts[0] == "clear") {
			Console::ClearScreen();
			PrintHeader();
			continue;
		}
		if (parts[0] == "ch") {
			Search.ResetState(true);
			cout << "Transposition table cleared." << endl;
			continue;
		}
		if (parts[0] == "test") {
			cout << "Test trigger point" << endl;
			continue;
		}
		if (parts[0] == "bighash") {
			Search.TranspositionTable.SetSize(1024);
			cout << "Using big hash: 1024 MB" << endl;
			continue;
		}
		if (parts[0] == "hugehash") {
			Search.TranspositionTable.SetSize(4096);
			cout << "Using huge hash: 4096 MB" << endl;
			continue;
		}
		if (parts[0] == "gamestate") {
			const GameState state = position.GetGameState();
			switch (state) {
			case GameState::Playing: cout << "Playing." << endl; break;
			case GameState::Draw: cout << "Drawn." << endl; break;
			case GameState::WhiteVictory: cout << "White won." << endl; break;
			case GameState::BlackVictory: cout << "Black won." << endl; break;
			}
			continue;
		}

		if (parts[0] == "frc") {
			Settings::Chess960 = true;
			cout << "-> Chess960 on" << endl;
			continue;
		}
		if (parts[0] == "nofrc") {
			Settings::Chess960 = false;
			cout << "-> Chess960 off" << endl;
			continue;
		}

		// Position command
		if (parts[0] == "position") {

			if (!Search.Aborting.load(std::memory_order_relaxed)) {
				std::cerr << "info string Search is busy!" << endl;
				continue;
			}

			if ((parts[1] == "startpos") || (parts[1] == "kiwipete") || (parts[1] == "lasker") || (parts[1] == "frc")) {
				if (parts[1] == "startpos") position = Position(FEN::StartPos);
				else if (parts[1] == "kiwipete") position = Position(FEN::Kiwipete);
				else if (parts[1] == "lasker") position = Position(FEN::Lasker);
				else if (parts[1] == "frc") {
					Settings::Chess960 = true;
					position = Position(stoi(parts[2]), stoi(parts[3]));
					cout << "-> FEN: " << position.GetFEN() << endl;
					continue;
				}

				if ((parts.size() > 2) && (parts[2] == "moves")) {
					for (int i = 3; i < parts.size(); i++) {
						const bool r = position.PushUCI(parts[i]);
						if (!r) cout << "!!! Error: invalid pushuci move: '" << parts[i] << "' at position '" << position.GetFEN() << "' !!!" << endl;
					}
				}
			}

			if (parts[1] == "fen") {
				std::string fen;
				if (parts.size() >= 8)
					fen = parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " " + parts[6] + " " + parts[7];
				else
					fen = parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " 0 1";
				position = Position(fen);

				if ((parts.size() > 8) && (parts[8] == "moves")) {
					for (int i = 9; i < parts.size(); i++) {
						const bool r = position.PushUCI(parts[i]);
						if (!r) cout << "!!! Error: invalid pushuci move !!!" << endl;
					}
				}
			}

			continue;
		}

		// Go command
		if (parts[0] == "go") {

			if (!Search.Aborting.load(std::memory_order_relaxed)) {
				std::cerr << "info string Search is busy!" << endl;
				continue;
			}
			if (SearchThread.joinable()) SearchThread.join();

			if ((parts.size() == 3) && ((parts[1] == "perft") || (parts[1] == "perftdiv"))) {
				int depth = stoi(parts[2]);
				PerftType type = PerftType::Normal;
				if (parts[1] == "perftdiv") type = PerftType::PerftDiv;
				Search.Perft(position, depth, type);
				continue;
			}

			params = SearchParams();
			for (int i = 1; i < parts.size(); i++) {
				// This looks ugly, but I'll rewrite it
				if (parts[i] == "wtime") { params.wtime = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "btime") { params.btime = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "movestogo") { params.movestogo = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "winc") { params.winc = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "binc") { params.binc = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "nodes") { params.nodes = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "softnodes") { params.softnodes = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "depth") { params.depth = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "mate") { params.depth = stoi(parts[i + 1LL]); i++; } // To do: search for mates only
				if (parts[i] == "movetime") { params.movetime = stoi(parts[i + 1LL]); i++; }
				if (parts[i] == "searchmoves") { cout << "info string Searchmoves parameter is not yet implemented!" << endl; }
			}

			// Starting the search thread
			// old call: Search.SearchMoves(board, params, true);
			SearchThread = std::thread([&]() {
				Search.SearchMoves(position, params, true);
			});
			continue;
		}

		if (parts[0] == "bench") {
			HandleBench(false);
			continue;
		}

		if (parts[0] == "longbench") {
			HandleBench(true);
			continue;
		}

		if (parts[0] == "compiler") {
			HandleCompiler();
			continue;
		}

		if (parts[0] == "flip") {
			cout << "Turn flipped." << endl;
			position.CurrentState().Turn = !position.CurrentState().Turn;
			continue;
		}

		if (parts[0] == "nnue") {
			cout << "Arch: " << FeatureSize << "->" << HiddenSize << "x2" << "->1 (SCReLU activation)" << endl;
			cout << "Net name: " << NETWORK_NAME << endl;
			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "Stopping engine." << endl;
}

void Engine::DrawBoard(const Position& position, const uint64_t customBits) const {

	const std::string side = position.Turn() ? "white" : "black";
	cout << "    Move: " << position.CurrentState().FullmoveClock << " - " << side << " to play" << endl;;

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
			const int pieceId = position.GetPieceAt(i * 8 + j);
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

			cout << cellStyle << ' ' << piece << ' ' << Default;

		}
		cout << "|" << '\n';
	}
	cout << "    ------------------------ " << '\n';
	cout << "     a  b  c  d  e  f  g  h" << endl;

}

void Engine::HandleBench(const bool lengthy) {
	const int oldHashSize = Settings::Hash;
	const bool oldChess960Setting = Settings::Chess960;
	Settings::Chess960 = false;
	uint64_t nodes = 0;
	SearchParams params;
	params.depth = lengthy ? 21 : 14;
	Search.TranspositionTable.SetSize(16);
	const auto startTime = Clock::now();
	
	for (std::string fen : BenchmarkFENs) {
		Settings::Chess960 = StartsWith(fen, "[frc]") ? true : false;
		if (StartsWith(fen, "[frc]")) fen = fen.substr(6, fen.length() - 6);
		Search.ResetState(false);
		Position pos = Position(fen);
		const Results r = Search.SearchMoves(pos, params, false);
		nodes += r.stats.Nodes;
	}
	const auto endTime = Clock::now();
	const int nps = static_cast<int>(nodes / ((endTime - startTime).count() / 1e9));
	cout << nodes << " nodes " << nps << " nps" << endl;
	Search.ResetState(false);
	Search.TranspositionTable.SetSize(oldHashSize); // also clears the transposition table
	Settings::Chess960 = oldChess960Setting;
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
		<< "\n- eval: prints the static evaluation of the position"
		<< "\n- fen: displays the current position's FEN string"
		<< "\n- go perft [n] & go perftdiv [n]: retuns the number of possible positions after n plys (incl. duplicates)" << endl;
}
