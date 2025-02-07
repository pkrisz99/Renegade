#include "Engine.h"

Engine::Engine(int argc, char* argv[]) {
	Settings::UseUCI = !PrettySupport;
	SearchThreads.TranspositionTable.SetSize(Settings::Hash);

	if (argc == 2 && std::string(argv[1]) == "bench") Behavior = EngineBehavior::Bench;
	else if (argc == 2 && std::string(argv[1]) == "datagen") Behavior = EngineBehavior::DatagenNormal;
	else if (argc == 2 && std::string(argv[1]) == "dfrcdatagen") Behavior = EngineBehavior::DatagenDFRC;
	else PrintHeader();
}

void Engine::PrintHeader() const {
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
}

// Start UCI protocol
void Engine::Start() {

	// Handle externally receiving bench
	if (Behavior == EngineBehavior::Bench) {
		HandleBench();
		SearchThreads.StopThreads();
		return;
	}

	// Handle externally receiving datagen
	/*if (Behavior == EngineBehavior::DatagenNormal || Behavior == EngineBehavior::DatagenDFRC) {
		const DatagenLaunchMode launchMode = (Behavior == EngineBehavior::DatagenNormal) ? DatagenLaunchMode::Normal : DatagenLaunchMode::DFRC;
		StartDatagen(launchMode);
		return;
	}*/

	Position position = Position(FEN::StartPos);
	std::string cmd;
	SearchParams params;

	// Main program loop
	
	// Please be aware that this code is utterly terrible and a rewrite is long overdue
	// I promise I'll get to this one day

	while (std::getline(cin, cmd)) {

		cmd = Trim(cmd);
		if (cmd.size() == 0) continue;
		std::vector<std::string> parts = Split(cmd);

		if (cmd == "quit" || cmd == "q") {
			break;
		}

		if (cmd == "uci") {
			cout << "id name Renegade " << Version << '\n';
			cout << "id author Krisztian Peocz" << '\n';
			cout << "option name Clear Hash type button" << '\n';
			cout << "option name Hash type spin default " << HashDefault << " min " << HashMin << " max " << HashMax << '\n';
			cout << "option name Threads type spin default " << ThreadsDefault << " min " << ThreadsMin << " max " << ThreadsMax << '\n';
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
			SearchThreads.WaitUntilReady();
			SearchThreads.ResetState(true);
			continue;
		}

		if (cmd == "stop" || cmd == "s") {
			SearchThreads.StopSearch();
			SearchThreads.WaitUntilReady();
			continue;
		}

		/*
		if (cmd == "datagen") {
			StartDatagen(DatagenLaunchMode::Ask);
			return;
		}

		if (cmd == "merge") {
			MergeDatagenFiles();
			return;
		}*/

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
				SearchThreads.TranspositionTable.SetSize(Settings::Hash);
				valid = true;
			}
			else if (parts[2] == "clear") {
				ConvertToLowercase(parts[3]);
				if (parts[3] == "hash") {
					SearchThreads.ResetState(true);
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
				Settings::Threads = stoi(parts[4]);
				SearchThreads.SetThreadCount(Settings::Threads);
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
		if (parts[0] == "debug" && parts.size() > 1) {
			if (parts[1] == "attackmap") {
				DrawBoard(position, position.GetThreats());
			}
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
			if (parts[1] == "isdraw") {
				cout << "Is drawn? " << position.IsDrawn(true) << endl;
			}
			continue;
		}

		// Custom commands
		if (parts[0] == "help") {
			HandleHelp();
			continue;
		}
		if (parts[0] == "draw" || parts[0] == "d") {
			DrawBoard(position);
			continue;
		}
		if (parts[0] == "eval" || parts[0] == "e") {
			const int nnue = NeuralEvaluate(position);
			cout << "-> Neural network evaluation: " << ToCentipawns(nnue, position.GetPly()) << " cp  (internal units: " << nnue << ")" << endl;
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
			SearchThreads.ResetState(true);
			cout << "Transposition table cleared." << endl;
			continue;
		}
		if (parts[0] == "bighash") {
			Settings::Hash = 1024;
			SearchThreads.TranspositionTable.SetSize(Settings::Hash);
			cout << "Using big hash: 1024 MB" << endl;
			continue;
		}
		if (parts[0] == "hugehash") {
			Settings::Hash = 4096;
			SearchThreads.TranspositionTable.SetSize(Settings::Hash);
			cout << "Using huge hash: 4096 MB" << endl;
			continue;
		}
		if (parts[0] == "frc") {
			if (parts[1] == "on") {
				Settings::Chess960 = true;
				cout << "-> Chess960 on" << endl;
			}
			else if (parts[1] == "off") {
				Settings::Chess960 = false;
				cout << "-> Chess960 off" << endl;
			}
			continue;
		}
		if (parts[0] == "th") {
			Settings::Threads = stoi(parts[1]);
			cout << "-> Set thread count to " << Settings::Threads << endl;
			SearchThreads.SetThreadCount(Settings::Threads);
			continue;
		}

		// Position command
		if (parts[0] == "position") {

			SearchThreads.WaitUntilReady();

			/*if (!SearchThreads.Aborting.load(std::memory_order_relaxed)) {
				std::cerr << "info string Search is busy!" << endl;
				continue;
			}*/

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
				const std::string fen = [&] {
					if (parts.size() >= 8) return parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " " + parts[6] + " " + parts[7];
					else return parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5] + " 0 1";
				}();
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

			SearchThreads.WaitUntilReady();

			/*if (!Search.Aborting.load(std::memory_order_relaxed)) {
				std::cerr << "info string Search is busy!" << endl;
				continue;
			}
			if (SearchThread.joinable()) SearchThread.join();*/

			if ((parts.size() == 3) && (parts[1] == "perft" || parts[1] == "perftdiv")) {
				const int depth = stoi(parts[2]);
				const PerftType type = (parts[1] == "perftdiv") ? PerftType::PerftDiv : PerftType::Normal;
				SearchThreads.Perft(position, depth, type);
				continue;
			}

			params = SearchParams();
			for (int i = 1; i < parts.size(); i++) {
				// This looks ugly, but I'll rewrite it
				if (parts[i] == "wtime") { 
					const int t = stoi(parts[i + 1]);
					params.wtime = (t > 0) ? t : 1;
					i++;
				}
				if (parts[i] == "btime") { 
					const int t = stoi(parts[i + 1]);
					params.btime = (t > 0) ? t : 1;
					i++;
				}
				if (parts[i] == "movestogo") { params.movestogo = stoi(parts[i + 1]); i++; }
				if (parts[i] == "winc") { params.winc = stoi(parts[i + 1]); i++; }
				if (parts[i] == "binc") { params.binc = stoi(parts[i + 1]); i++; }
				if (parts[i] == "nodes") { params.nodes = stoi(parts[i + 1]); i++; }
				if (parts[i] == "softnodes") { params.softnodes = stoi(parts[i + 1]); i++; }
				if (parts[i] == "depth") { params.depth = stoi(parts[i + 1]); i++; }
				if (parts[i] == "mate") { params.depth = stoi(parts[i + 1]); i++; } // To do: search for mates only
				if (parts[i] == "movetime") { params.movetime = stoi(parts[i + 1]); i++; }
				if (parts[i] == "searchmoves") { cout << "info string Searchmoves parameter is not yet implemented!" << endl; }
			}

			// Starting the search thread
			SearchThreads.StartSearch(position, params, true);
			continue;
		}

		if (parts[0] == "bench" || parts[0] == "b") {
			HandleBench();
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
			cout << "-> Arch: (" << FeatureSize << "x" << InputBucketCount << "hm -> " << HiddenSize << ")x2" << " -> 1x" << OutputBucketCount
				<< "  [SCReLU, QA=" << QA << ", QB=" << QB << "]" << endl;
			cout << "-> Net name: " << NETWORK_NAME << endl;
			cout << "-> Net size: " << Console::FormatInteger(sizeof(NetworkRepresentation)) << endl;
			continue;
		}

		cout << "Unknown command: '" << parts[0] << "'" << endl;

	}
	cout << "Stopping engine." << endl;
	SearchThreads.StopThreads();
}

void Engine::DrawBoard(const Position& pos, const uint64_t highlight) const {

	constexpr std::string_view whiteOnLightSquare = "\033[31;47m";
	constexpr std::string_view whiteOnDarkSquare = "\033[31;43m";
	constexpr std::string_view blackOnLightSquare = "\033[30;47m";
	constexpr std::string_view blackOnDarkSquare = "\033[30;43m";
	constexpr std::string_view defaultStyle = "\033[0m";
	constexpr std::string_view whiteOnTarget = "\033[31;45m";
	constexpr std::string_view blackOnTarget = "\033[30;45m";

	const Board& b = pos.CurrentState();

	std::string castling{};
	if (b.WhiteRightToShortCastle) castling += "K";
	if (b.WhiteRightToLongCastle) castling += "Q";
	if (b.BlackRightToShortCastle) castling += "k";
	if (b.BlackRightToLongCastle) castling += "q";

	const GameState gameState = pos.GetGameState();
	const std::string_view gameStateStr = [&] {
		if (gameState == GameState::Playing) return "in progress";
		else if (gameState == GameState::WhiteVictory) return "white won";
		else if (gameState == GameState::WhiteVictory) return "black won";
		else return "drawn";
	}();

	const int raw = NeuralEvaluate(pos);
	const int cp = ToCentipawns(raw, pos.GetPly());

	cout << '\n';

	for (int i = 8; i >= -4; i--) {

		if (i == 8) cout << "    ------------------------     ";

		if (i <= 7 && i >= 0) {
			cout << " " << i + 1 << " |";
			for (int j = 0; j <= 7; j++) {
				const uint8_t sq = Square(i, j);
				const uint8_t piece = pos.GetPieceAt(sq);
				const uint8_t pieceColor = ColorOfPiece(piece);
				const char pieceChar = PieceChars[piece];

				const std::string_view cellStyle = [&] {
					if (CheckBit(highlight, sq))
						return (pieceColor == PieceColor::Black) ? blackOnTarget : whiteOnTarget;

					if ((i + j) % 2 == 1)
						return (pieceColor == PieceColor::Black) ? blackOnLightSquare : whiteOnLightSquare;
					else
						return (pieceColor == PieceColor::Black) ? blackOnDarkSquare : whiteOnDarkSquare;
				}();

				cout << cellStyle << ' ' << pieceChar << ' ' << defaultStyle;

			}
			cout << "|    ";
		}

		if (i == -1) cout << "    ------------------------     ";
		if (i == -2) cout << "     a  b  c  d  e  f  g  h      ";
		if (i < -2) cout << "                                 ";

		// information to be displayed for each line:
		switch (i) {
		case 8:
			cout << "Board information:";
			break;
		case 7:
			cout << " - " << (pos.Turn() == Side::White ? "White" : "Black") << " to move";
			break;
		case 6:
			cout << " - Move " << b.FullmoveClock;
			cout << " [halfmove counter: " << static_cast<int>(b.HalfmoveClock);
			cout << ", game plies: " << pos.GetPly() << "]";
			break;
		case 5:
			cout << " - In check: " << (pos.IsInCheck() ? "yes" : "no");
			break;
		case 4:
			cout << " - Castling rights: ";
			cout << (castling.empty() ? "none" : castling);
			break;
		case 3:
			cout << " - En passant target: ";
			cout << (b.EnPassantSquare == -1 ? "none" : SquareStrings[b.EnPassantSquare]);
			break;

		case 1:
			cout << "Internal state:";
			break;
		case 0:
			cout << " - Hash: " << std::hex << pos.Hash() << std::dec;
			break;
		case -1:
			cout << " - Static eval: " << cp << " cp [" << raw << " units]";
			break;
		case -2:
			cout << " - Phase: " << pos.GetGamePhase();
			break;
		case -3:
			cout << " - Game state: " << gameStateStr;
			break;
		case -4:
			cout << " - FEN: " << pos.GetFEN();
			break;
		}
		cout << '\n';
	}
	cout << endl;
}

void Engine::HandleBench() {
	const int oldHashSize = Settings::Hash;
	const bool oldChess960Setting = Settings::Chess960;
	const int oldThreadCount = Settings::Threads;
	Settings::Threads = 1;
	Settings::Hash = 16;
	SearchThreads.TranspositionTable.SetSize(16);
	SearchThreads.SetThreadCount(1);

	uint64_t nodes = 0;
	SearchParams params{};
	params.depth = 14;
	const auto startTime = Clock::now();

	for (std::string fen : BenchmarkFENs) {
		Settings::Chess960 = StartsWith(fen, "[frc]");
		if (StartsWith(fen, "[frc]")) fen = fen.substr(6, fen.length() - 6);
		SearchThreads.ResetState(false);
		const Position pos = Position(fen);

		const Results r = SearchThreads.SearchSinglethreaded(pos, params);
		nodes += r.nodes;
	}

	const auto endTime = Clock::now();
	const int nps = static_cast<int>(nodes / ((endTime - startTime).count() / 1e9));
	cout << nodes << " nodes " << nps << " nps" << endl;

	SearchThreads.ResetState(false);
	Settings::Hash = oldHashSize;
	SearchThreads.TranspositionTable.SetSize(oldHashSize); // also clears the transposition table
	// Some issues with the following code, but only when starting from the command line:
	// Settings::Threads = oldThreadCount;
	// SearchThreads.SetThreadCount(oldThreadCount);
	// possibly starting and shutting down a thread too quickly?
	Settings::Chess960 = oldChess960Setting;
}

void Engine::HandleCompiler() const {
#if defined(__clang__)
	cout << "-> Compiler: clang" << endl;
	cout << "-> Version: " << __clang_major__ << endl;
#elif defined(__GNUC__) || defined(__GNUG__)
	cout << "-> Compiler: gcc" << endl;
	cout << "-> Version: " << __GNUC__ << endl;
#elif defined(_MSC_VER)
	cout << "-> Compiler: MSVC" << endl;
	cout << "-> Version: " << _MSC_VER << endl;
#elif
	cout << "-> Unknown - Interesting compiler you've got there!" << endl;
#endif
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
		<< "\n- go perft [n] & go perftdiv [n]: retuns the number of possible positions after n plys (incl. duplicates)\n" << endl;
}
