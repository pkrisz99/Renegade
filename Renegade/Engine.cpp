#include "Engine.h"

Engine::Engine(int argc, char* argv[]) {
	Settings::UseUCI = !PrettySupport;
	searchThreads.TranspositionTable.SetSize(Settings::Hash, 1);

	// Handling command line parameters
	if (argc > 1) {

		// Bench:
		if (std::string(argv[1]) == "bench") behavior = EngineBehavior::Bench;
		
		// Datagen:
		if (std::string(argv[1]) == "datagen") {
#ifdef RENEGADE_DATAGEN
			if (argc != 4) {
				cout << Console::Red << "Error: incorrect number of parameters" << Console::White << '\n';
				cout << "Usage: ./Renegade datagen <variant> <thread_count>" << endl;
				std::terminate();
			}
			datagenSettings.dfrc = [&] {
				std::string_view variant = argv[2];
				if (variant == "normal") return false;
				if (variant == "dfrc") return true;
				cout << Console::Red << "Error: unrecognized variant (accepted: normal, dfrc)" << Console::White << endl;
				std::terminate();
			}();
			datagenSettings.threads = [&] {
				return std::stoi(argv[3]);
			}();
			behavior = EngineBehavior::Datagen;
#else
			cout << Console::Red << "Error: engine is not compiled for datagen" << Console::White << endl;
			std::terminate();
#endif
		}
	}
}

void Engine::PrintHeader() const {
	cout << "Renegade chess engine " << Version << " [" << __DATE__ << " " << __TIME__ << "]" << endl;
#ifdef RENEGADE_DATAGEN
	cout << "Note: engine compiled for datagen" << endl;
#endif
}

void Engine::Start() {

	// Handle externally receiving bench
	if (behavior == EngineBehavior::Bench) {
		HandleBench();
		searchThreads.StopThreads();
		return;
	}

	// Handle externally receiving datagen
#ifdef RENEGADE_DATAGEN
	if (behavior == EngineBehavior::Datagen) {
		StartDatagen(datagenSettings);
		searchThreads.StopThreads();
		return;
	}
#endif

	// Main program loop
	std::string input;
	PrintHeader();

	while (std::getline(std::cin, input)) {

		// Process input
		input = Trim(input);
		const std::string originalInput = input;
		input = ToLowercase(input);
		if (input.size() == 0) continue;
		std::vector<std::string> parts = Split(input);
		std::string command = parts[0];

		// Standard UCI commands:

		if (command == "quit" || command == "q") {
			searchThreads.StopSearch();
			break;
		}
		else if (command == "uci") {
			cout << "id name Renegade " << Version << '\n';
			cout << "id author Krisztian Peocz" << '\n';
			cout << "option name Clear Hash type button" << '\n';
			cout << "option name Hash type spin default " << HashDefault << " min " << HashMin << " max " << HashMax << '\n';
			cout << "option name Threads type spin default " << ThreadsDefault << " min " << ThreadsMin << " max " << ThreadsMax << '\n';
			cout << "option name UCI_ShowWDL type check default " << (ShowWDLDefault ? "true" : "false") << '\n';
			cout << "option name UCI_Chess960 type check default " << (Chess960Default ? "true" : "false") << '\n';
			if (IsTuningActive()) PrintTunableParameters();
			cout << "uciok" << endl;
			Settings::UseUCI = true;
		}
		else if (command == "ucinewgame") {
			searchThreads.WaitUntilReady();
			searchThreads.ResetState(true);
		}
		else if (command == "isready") {
			cout << "readyok" << endl;
		}
		else if (command == "stop" || command == "s") {
			searchThreads.StopSearch();
		}
		else if (command == "setoption") {
			HandleSetOption(parts);
		}
		else if (command == "position") {
			HandlePosition(originalInput);
		}
		else if (command == "go") {
			HandleGo(parts);
		}

		// Renegade's own extensions and debug commands:

		else if (command == "bench" || command == "b") {
			HandleBench();
		}
		else if (command == "draw" || command == "d") {
			HandleDraw(position);
		}
		else if (command == "tunetext") {
			GenerateOpenBenchTuningString();
		}
		else if (command == "compiler") {
			HandleCompiler();
		}
		else if (command == "help") {
			HandleHelp();
		}
		else if (command == "eval" || command == "e") {
			const int nnue = ClassicalEvaluate(position);
			cout << "-> Classical evaluation: " << ToCentipawns(nnue, position.GetPly()) << " cp / " << nnue << " internal units" << endl;
		}
		else if (command == "fen" || command == "f") {
			cout << "-> Position FEN: " << position.GetFEN() << endl;
		}
		else if (command == "clear") {
			Console::ClearScreen();
			PrintHeader();
		}
		else if (command == "attackmap") {
			HandleDraw(position, position.GetThreats());
		}
		else if (command == "movelist") {
			// All legal moves:
			MoveList moves{};
			position.GenerateAllLegalMoves(moves);
			cout << "-> Legal moves (" << moves.size() << "): ";
			for (const ScoredMove& m : moves) cout << m.move.ToString(Settings::Chess960) << " ";
			cout << endl;
			// Pseudolegal moves:
			moves.clear();
			position.GenerateAllPseudoLegalMoves(moves);
			cout << "-> Pseudolegal moves (" << moves.size() << "): ";
			for (const ScoredMove& m : moves) cout << m.move.ToString(Settings::Chess960) << " ";
			cout << endl;
		}
		else if (command == "settings") {
			cout << std::boolalpha;
			cout << "-> Hash:      " << Settings::Hash << endl;
			cout << "-> Show WDL:  " << Settings::ShowWDL << endl;
			cout << "-> Chess960:  " << Settings::Chess960 << endl;
			cout << "-> Using UCI: " << Settings::UseUCI << endl;
			cout << std::noboolalpha;
			for (const auto& [name, param] : TunableParameterList) cout << "-> " << name << " : " << param.value << endl;
		}
		else if (command == "pasthashes") {
			cout << "-> Past hashes list size: " << position.States.size() << endl;
			for (size_t i = 0; i < position.States.size(); i++) {
				cout << "   entry " << i << ": " << std::hex << position.States[i].BoardHash << std::dec << endl;
			}
		}
		else if (command == "isdraw") {
			cout << "-> Is drawn: " << position.IsDrawn(0) << endl;
		}

		// Shorthands for changing settings quickly:

		else if (command == "ch") {
			searchThreads.ResetState(true);
			cout << "-> Transposition and history tables cleared" << endl;
		}
		else if (command == "frc") {
			if (parts.size() == 2 && parts[1] == "on") {
				Settings::Chess960 = true;
				cout << "-> Chess960 on" << endl;
			}
			else if (parts.size() == 2 && parts[1] == "off") {
				Settings::Chess960 = false;
				cout << "-> Chess960 off" << endl;
			}
		}
		else if (command == "th") {
			Settings::Threads = std::stoi(parts[1]);
			searchThreads.SetThreadCount(Settings::Threads);
			cout << "-> Set thread count to " << Settings::Threads << endl;
		}
		else if (command == "hash") {
			Settings::Hash = std::stoi(parts[1]);
			searchThreads.TranspositionTable.SetSize(Settings::Hash, Settings::Threads);
			searchThreads.ResetState(false);
			cout << "-> Set hash size to " << Settings::Hash << endl;
		}

		else {
			cout << "Error: unrecognized command: '" << command << "'" << endl;
		}
	}

	searchThreads.StopThreads();
}

void Engine::HandleSetOption(const std::vector<std::string>& parts) {
	if (parts.size() == 1) {
		cout << "Error: Missing parameters" << endl;
		return;
	}
	if (parts[1] != "name") {
		cout << "Error: Expected 'name', got something else" << endl;
		return;
	}

	// Search for 'value', which divides the command into the name and value parts
	// This implicit handles if there's no value (e.g. Clear Hash)
	const auto valueIt = std::ranges::find(parts, "value");
	const size_t valuePos = valueIt - parts.begin();

	// Extracting the option name and value
	const std::string optionName = [&] {
		std::string name = "";
		for (std::size_t i = 2; i < valuePos; i++) name += parts[i] + " ";
		return Trim(name);
	}();
	const std::string optionValue = [&] {
		std::string value = "";
		for (std::size_t i = valuePos + 1; i < parts.size(); i++) value += parts[i] + " ";
		return Trim(value);
	}();

	// Various options implemented by the engine
	if (optionName == "clear hash") {
		searchThreads.ResetState(true);
	}
	else if (optionName == "hash") {
		Settings::Hash = std::stoi(optionValue);
		searchThreads.TranspositionTable.SetSize(Settings::Hash, Settings::Threads);
	}
	else if (optionName == "threads") {
		Settings::Threads = std::stoi(optionValue);
		searchThreads.SetThreadCount(Settings::Threads);
	}
	else if (optionName == "uci_chess960") {
		const std::optional<bool> value = ParseUCIBoolean(optionValue);
		if (value.has_value()) Settings::Chess960 = value.value();
	}
	else if (optionName == "uci_showwdl") {
		const std::optional<bool> value = ParseUCIBoolean(optionValue);
		if (value.has_value()) Settings::ShowWDL = value.value();
	}
	else if (IsTuningActive() && HasTunableParameter(parts[2])) {
		SetTunableParameter(optionName, std::stoi(optionValue));
	}
	else {
		cout << "Error: unrecognized option '" << optionName << "'" << endl;
	}
}

void Engine::HandlePosition(const std::string originalInput) {
	searchThreads.WaitUntilReady();

	std::vector<std::string> parts = Split(originalInput);  // FENs are case sensitive
	if (parts.size() < 2) {
		cout << "Error: missing parameters" << endl;
		return;
	}

	// Get base position: before any additional moves are applied
	const auto movesIt = std::ranges::find(parts, "moves");
	const size_t movesPos = movesIt - parts.begin();

	const std::string basePosition = [&] {
		std::string str = "";
		for (std::size_t i = 1; i < movesPos; i++) str += parts[i] + " ";
		return Trim(str);
	}();

	if (basePosition == "startpos") position = Position(FEN::StartPos);
	else if (basePosition == "kiwipete") position = Position(FEN::Kiwipete);
	else if (basePosition == "lasker") position = Position(FEN::Lasker);

	else if (basePosition.starts_with("fen")) {
		const std::string fen = basePosition.substr(4);
		const std::vector<std::string> fenParts = Split(fen);
		if (fenParts.size() == 6) position = Position(fen);
		else if (fenParts.size() == 4) position = Position(fen + " 0 1");
		else {
			cout << "Error: malformed FEN '" << fen << "'" << endl;
			return;
		}
	}

	else if (basePosition.starts_with("frc")) {
		const std::vector<std::string> frcIdParts = Split(basePosition.substr(4));
		if (frcIdParts.size() == 1) {
			Settings::Chess960 = true;
			const int frcIndex = std::stoi(frcIdParts[0]);
			position = Position(frcIndex, frcIndex);
			cout << "-> Set board to " << position.GetFEN() << endl;
		}
		else if (frcIdParts.size() == 2) {
			Settings::Chess960 = true;
			position = Position(std::stoi(frcIdParts[0]), std::stoi(frcIdParts[1]));
			cout << "-> Set board to " << position.GetFEN() << endl;
		}
		else {
			cout << "Error: too many parameters" << endl;
			return;
		}
	}

	else {
		cout << "Error: unable to decode position string '" << basePosition << "'" << endl;
		return;
	}

	// Apply additional moves
	for (size_t i = movesPos + 1; i < parts.size(); i++) {
		const bool success = position.PushUCI(parts[i]);
		if (!success) cout << "Error: invalid UCI move " << parts[i] << " for position " << position.GetFEN() << endl;
	}
}

void Engine::HandleGo(const std::vector<std::string>& parts) {
	searchThreads.WaitUntilReady();

	// Handle perft commands
	if (parts.size() == 3 && (parts[1] == "perft" || parts[1] == "perftdiv")) {
		const int depth = std::stoi(parts[2]);
		const PerftType type = (parts[1] == "perftdiv") ? PerftType::PerftDiv : PerftType::Normal;
		Perft(position, depth, type);
		return;
	}

	// Handle search parameters
	SearchParams params = SearchParams();
	for (size_t i = 1; i < parts.size(); i++) {

		if (parts[i] == "infinite") continue;  // default is to assume an unbounded search

		else if (parts[i] == "wtime") params.wtime = std::max(std::stoi(parts[++i]), 1);
		else if (parts[i] == "btime") params.btime = std::max(std::stoi(parts[++i]), 1);
		else if (parts[i] == "winc") params.winc = std::stoi(parts[++i]);
		else if (parts[i] == "binc") params.binc = std::stoi(parts[++i]);
		else if (parts[i] == "movetime") params.movetime = std::stoi(parts[++i]);
		else if (parts[i] == "movestogo") params.movestogo = std::stoi(parts[++i]);

		else if (parts[i] == "depth") params.depth = std::stoi(parts[++i]);
		else if (parts[i] == "nodes") params.nodes = std::stoull(parts[++i]);
		else if (parts[i] == "softnodes") params.softnodes = std::stoull(parts[++i]);

		else if (parts[i] == "mate") {
			cout << "info string Warning: mate parameter is not yet implemented" << endl;
		}
		else if (parts[i] == "searchmoves") {
			cout << "info string Warning: searchmoves parameter is not yet implemented" << endl;
		}
	}

	// Starting the search thread
	searchThreads.StartSearch(position, params);
}

void Engine::HandleBench() {
	const int oldHashSize = Settings::Hash;
	const bool oldChess960Setting = Settings::Chess960;
	const int oldThreadCount = Settings::Threads;
	Settings::Threads = 1;
	Settings::Hash = 16;
	searchThreads.TranspositionTable.SetSize(16, 1);
	searchThreads.SetThreadCount(1);

	uint64_t nodes = 0;
	SearchParams params{};
	params.depth = 14;
	const auto startTime = Clock::now();

	for (std::string fen : BenchmarkFENs) {
		Settings::Chess960 = false;
		if (fen.starts_with("[frc]")) {
			Settings::Chess960 = true;
			fen = fen.substr(6, fen.length() - 6);
		}
		searchThreads.ResetState(false);
		const Position pos = Position(fen);
		const Results r = searchThreads.SearchSinglethreaded(pos, params);
		nodes += r.nodes;
	}

	const auto endTime = Clock::now();
	const int nps = static_cast<int>(nodes / ((endTime - startTime).count() / 1e9));
	cout << nodes << " nodes " << nps << " nps" << endl;

	searchThreads.ResetState(false);
	Settings::Threads = oldThreadCount;
	searchThreads.SetThreadCount(oldThreadCount);
	Settings::Hash = oldHashSize;
	searchThreads.TranspositionTable.SetSize(oldHashSize, oldThreadCount); // also clears the transposition table
	Settings::Chess960 = oldChess960Setting;
}

void Engine::HandleDraw(const Position& pos, const uint64_t highlight) const {

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
		else if (gameState == GameState::BlackVictory) return "black won";
		else return "drawn";
	}();

	const int raw = ClassicalEvaluate(pos);
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
#else
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
		<< "\n- go perft [n] & go perftdiv [n]: returns the number of possible positions after n plies (incl. duplicates)\n" << endl;
}

// Perft methods ----------------------------------------------------------------------------------

void Engine::Perft(Position& position, const int depth, const PerftType type) const {
	const bool isStartpos = (position.GetFEN() == FEN::StartPos);
	constexpr std::array<uint64_t, 8> startposPerfts = { 1, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860 };

	const auto startTime = Clock::now();
	const uint64_t r = PerftRecursive(position, depth, depth, type);
	const auto endTime = Clock::now();

	const float seconds = static_cast<float>((endTime - startTime).count() / 1e9);
	const float speed = r / seconds / 1000000;
	cout << "-> Perft(" << depth << ") = " << Console::FormatInteger(r) << " | "
		<< std::setprecision(2) << std::fixed << seconds << " s | "
		<< std::setprecision(3) << speed << " mnps (non-bulk)" << endl;

	if (isStartpos && depth < static_cast<int>(startposPerfts.size()) && startposPerfts[depth] != r)
		cout << "-> Uh-oh. (expected: " << Console::FormatInteger(startposPerfts[depth]) << ")" << endl;
}

uint64_t Engine::PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const {
	MoveList moves{};
	position.GenerateAllPseudoLegalMoves(moves);

	if (type == PerftType::PerftDiv && originalDepth == depth) cout << "-> Legal moves (" << moves.size() << "): " << endl;
	uint64_t count = 0;
	for (const auto& m : moves) {
		if (!position.IsLegalMove(m.move)) continue;
		uint64_t r;
		if (depth == 1) {
			r = 1;
			count += 1;
		}
		else {
			position.PushMove(m.move);
			r = PerftRecursive(position, depth - 1, originalDepth, type);
			position.PopMove();
			count += r;
		}
		if (originalDepth == depth && type == PerftType::PerftDiv)
			cout << " - " << m.move.ToString(Settings::Chess960) << " : " << r << endl;
	}
	return count;
}
