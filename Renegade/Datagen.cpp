#include "Datagen.h"

// Display things ---------------------------------------------------------------------------------

static void SetTitle(const std::string title) {
	Console::ClearScreen();
	cout << Console::Highlight << " " << title << " " << Console::White;
	cout << " [" << Version << "]\n" << endl;
}

static void PressEnterToExit() {
	cout << "Press enter to exit." << endl;
	cin.ignore();
	cin.get();
}

static std::string FormatRuntime(const int seconds) {
	const auto [minutes, s] = std::div(seconds, 60);
	const auto [h, m] = std::div(minutes, 60);
	//return std::format("{}:{:02d}:{:02d}", h, m, s);
	std::stringstream oss;
	oss << h << ':' << std::setw(2) << std::setfill('0') << m << ':' << std::setw(2) << std::setfill('0') << s;
	return oss.str();
}

// Self-play datagen ------------------------------------------------------------------------------

void SelfPlay(const std::string filename);  // main loop per thread, forward declaring this

std::atomic<uint64_t> Games = 0, Positions = 0, Plies = 0, Depths = 0, Nodes = 0;
std::atomic<uint64_t> WhiteWins = 0, Draws = 0, BlackWins = 0;
Clock::time_point StartTime;
int ThreadCount = 0;
bool DFRC = false;
std::vector<std::string> Openings{};

// Apply a small random adjustment to node limits to avoid unwanted deterministic behavior
SearchParams RandomizeNodeLimits(const SearchParams& originalParams, std::uniform_int_distribution<int>& distribution, std::mt19937& generator) {
	SearchParams searchParams = originalParams;
	searchParams.softnodes += distribution(generator);
	return searchParams;
}

// Starts running datagen, launches threads
void StartDatagen(const DatagenLaunchMode launchMode) {

	// There is a streamlined way to launch datagen (normal and dfrc) from the command line using the default settings
	// When launched from UCI, the settings has to be provided manually

	SetTitle("Renegade's datagen utility");
	std::string filename;

	if (launchMode == DatagenLaunchMode::Ask) {
		cout << "Filename? " << Console::Yellow;
		cin >> filename;

		cout << Console::White << "How many threads? " << Console::Yellow;
		cin >> ThreadCount;

		cout << Console::White << "Do DRFC? " << Console::Yellow;
		cin >> DFRC;
		cout << Console::White << endl;
	}
	else {
		const auto time = std::chrono::system_clock::now().time_since_epoch();
		const uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(time).count();
		filename = "datagen_" + std::to_string(timestamp);
		ThreadCount = std::thread::hardware_concurrency();
		DFRC = launchMode == DatagenLaunchMode::DFRC;
		
		cout << "Quick start details:" << endl;
		cout << " - Filename: " << Console::Yellow << filename << Console::White << endl;
		cout << " - Thread count: " << Console::Yellow << ThreadCount << Console::White << endl;
		cout << " - Doing DFRC: " << Console::Yellow << DFRC << Console::White << '\n' << endl;
	}

	// Load opening book - using these seem to improve net strength
	if (!DFRC) {
		constexpr std::string_view book = "UHO_Lichess_4852_v1.epd";
		std::ifstream bookFile(book);
		std::string line;
		while (std::getline(bookFile, line)) Openings.push_back(line);

		if (Openings.size() == 0) {
			cout << Console::Red << "For non-DFRC datagen it is required to have the " << book << " book." << Console::White << endl;
			PressEnterToExit();
			return;
		}
	}

	Settings::Chess960 = DFRC;
	Settings::Hash = 1;
	Settings::Threads = 1;
	StartTime = Clock::now();

	cout << "Datagen settings:" << endl;
	if (!DFRC) cout << " - After the book exit do " << Console::Yellow << randomPlyBaseNormal << Console::White << " plies of random moves, then play normally" << endl;
	else cout << " - " << Console::Yellow << randomPlyBaseDFRC << Console::White << " or " << Console::Yellow << randomPlyBaseDFRC + 1
		<< Console::White << " plies of random rollout, then normal playout" << endl;
	cout << " - Verification @ some node limit of " << Console::Yellow << verificationParams.softnodes << Console::White
		<< " with a threshold of " << Console::Yellow << startingEvalLimit << Console::White << endl;
	cout << " - Playing @ soft node limit of " << Console::Yellow << playingParams.softnodes << Console::White << endl;
	cout << " - Using DFRC starting positions: " << Console::Yellow << std::boolalpha << DFRC
		<< std::noboolalpha << Console::White << endl;
	if (!DFRC) cout << " - The opening book has " << Console::Yellow << Console::FormatInteger(Openings.size()) << Console::White << " lines" << endl;
	cout << endl;

	std::vector<std::thread> threads = std::vector<std::thread>();
	for (int i = 0; i < ThreadCount; i++) {
		std::string filenameForThread = filename + "_" + std::to_string(i + 1);
		threads.emplace_back(&SelfPlay, filenameForThread);
	}
	for (std::thread& t : threads) t.join();
}

// One thread of datagen, running games and storing results
void SelfPlay(const std::string filename) {

	Search* Searcher1 = new Search;
	Search* Searcher2 = new Search;
	Search* SearcherV = new Search;
	Searcher1->TranspositionTable.SetSize(1, 1);
	Searcher2->TranspositionTable.SetSize(1, 1);
	SearcherV->TranspositionTable.SetSize(1, 1);
	Searcher1->SetThreadCount(1);
	Searcher2->SetThreadCount(1);
	SearcherV->SetThreadCount(1);

	const int randomPlyBase = DFRC ? randomPlyBaseDFRC : randomPlyBaseNormal;
	std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> nodeLimitNoise(-300, 300);
	int gamesOnThread = 0;
	std::vector<ViriformatGame> unsavedViriformatGames{};

	while (true) {

		bool failed = false;

		// 1. Reset state
		Position position = [&] {
			if (!DFRC) {
				std::uniform_int_distribution<std::size_t> distribution(0, Openings.size() - 1);
				const std::string opening = Openings[distribution(generator)];
				return Position(opening);
			}
			else {
				std::uniform_int_distribution<std::size_t> distribution(0, 959);
				const int whiteFrcIndex = distribution(generator);
				const int blackFrcIndex = distribution(generator);
				return Position(whiteFrcIndex, blackFrcIndex);
			}
		}();

		// 2. Generate random moves from the start
		const int randomPlies = [&] {
			if (!DFRC) return randomPlyBase;
			else return (gamesOnThread % 2 == 0) ? randomPlyBase : (randomPlyBase + 1);
		}();

		for (int i = 0; i < randomPlies; i++) {
			MoveList moves{};
			position.GenerateMoves(moves, MoveGen::All, Legality::Legal);
			if (moves.size() == 0) {
				failed = true;
				break;
			}
			std::uniform_int_distribution<std::size_t> distribution(0, moves.size() - 1);
			position.PushMove(moves[distribution(generator)].move);
		}
		if (failed) continue;

		// Rarely, the last move will result in a checkmate position - filter these
		MoveList moves{};
		position.GenerateMoves(moves, MoveGen::All, Legality::Legal);
		if (moves.size() == 0) continue;

		// 3. Verify evaluation if acceptable
		const Results verificationResults = SearcherV->SearchSinglethreaded(position, verificationParams);
		SearcherV->ResetState(true);

		if (std::abs(verificationResults.score) > startingEvalLimit) failed = true;
		if (failed) continue;

		Searcher1->ResetState(true);
		Searcher2->ResetState(true);

		ViriformatGame currentViriformatGame{};
		currentViriformatGame.SetStartingBoard(position.CurrentState(), position.CastlingConfig);
		GameState outcome = GameState::Playing;
		int winAdjudicationCounter = 0, drawAdjudicationCounter = 0;

		// 4. Play out the game
		while (true) {

			// Search
			const SearchParams currentParams = RandomizeNodeLimits(playingParams, nodeLimitNoise, generator);
			Search* currentSearcher = (position.Turn() == Side::White) ? Searcher1 : Searcher2;
			const Results results = currentSearcher->SearchSinglethreaded(position, currentParams);
			const Move move = results.BestMove();
			const int whiteScore = results.score * (position.Turn() == Side::Black ? -1 : 1);

			// Adjudication
			if (std::abs(whiteScore) > winAdjEvalThreshold) {
				winAdjudicationCounter += 1;
				if (winAdjudicationCounter >= winAdjEvalPlies) {
					outcome = (whiteScore > 0) ? GameState::WhiteVictory : GameState::BlackVictory;
					break;
				}
			}
			else winAdjudicationCounter = 0;

			if (std::abs(whiteScore) < drawAdjEvalThreshold) {
				drawAdjudicationCounter += 1;
				if (drawAdjudicationCounter >= drawAdjPlies) {
					outcome = GameState::Drawn;
					break;
				}
			}
			else drawAdjudicationCounter = 0;

			if (position.GetPly() > 600) {
				failed = true;
				break;
			}

			if (move.IsNull()) {
				failed = true;
				cout << Console::Yellow << "Got null-move for " << position.GetFEN() << " with eval of " << results.score << Console::White << endl;
				break;
			}

			// Update statistics
			Positions.fetch_add(1, std::memory_order_relaxed);
			Depths.fetch_add(results.depth, std::memory_order_relaxed);
			Nodes.fetch_add(results.nodes, std::memory_order_relaxed);
			
			currentViriformatGame.AddMove(move, whiteScore);
			position.PushMove(move);

			outcome = position.GetGameState();
			if (outcome != GameState::Playing) break;
		}

		if (failed) continue;

		gamesOnThread += 1;
		Games.fetch_add(1, std::memory_order_relaxed);
		Plies.fetch_add(position.GetPly(), std::memory_order_relaxed);

		if (outcome == GameState::WhiteVictory) WhiteWins.fetch_add(1, std::memory_order_relaxed);
		else if (outcome == GameState::Drawn) Draws.fetch_add(1, std::memory_order_relaxed);
		else if (outcome == GameState::BlackVictory) BlackWins.fetch_add(1, std::memory_order_relaxed);
		
		// 5. Store the game to the memory, and periodically to the hard drive
		currentViriformatGame.Finish(outcome);
		unsavedViriformatGames.push_back(currentViriformatGame);

		if (gamesOnThread % 100 == 0) {  // should take a few milliseconds
			std::ofstream vfFile(filename + ".vf", std::ios_base::app | std::ios::binary);
			for (const ViriformatGame& vfGame : unsavedViriformatGames) vfGame.WriteToFile(vfFile);
			vfFile.close();
			unsavedViriformatGames.clear();
		}

		// 6. Update display
		const int games = Games.load(std::memory_order_relaxed);
		if (games % 100 == 0) {
			const auto endTime = Clock::now();
			const uint64_t positions = Positions.load(std::memory_order_relaxed);
			const int seconds = static_cast<int>((endTime - StartTime).count() / 1e9);
			const int speed1 = positions * 3600 / std::max(seconds, 1);
			const int speed2 = speed1 / 3600 / ThreadCount;

			const std::string display = "Games: " + Console::FormatInteger(games)
				+ " | Positions: " + Console::FormatInteger(positions)
				+ " | Runtime: " + FormatRuntime(seconds)
				+ " | Speed: " + std::to_string(speed1 / 1000) + "k/h " + std::to_string(speed2) + "/s/th";
			cout << display << endl;

#ifdef __cpp_lib_format
			if (games % 1000 == 0) {
				const float whiteWinRate = WhiteWins.load(std::memory_order_relaxed) * 100.f / games;
				const float drawRate = Draws.load(std::memory_order_relaxed) * 100.f / games;
				const float blackWinRate = BlackWins.load(std::memory_order_relaxed) * 100.f / games;
				const float avgPlies = static_cast<float>(Plies.load(std::memory_order_relaxed)) / games;
				const int avgNodes = Nodes.load(std::memory_order_relaxed) / positions;
				const float avgDepths = static_cast<float>(Depths.load(std::memory_order_relaxed)) / positions;

				const std::string outcomeString = std::format("Outcomes: {:.1f}% - {:.1f}% - {:.1f}%", whiteWinRate, drawRate, blackWinRate);
				const std::string lengthString = std::format("Avg. game length: {:.1f} plies", avgPlies);
				const std::string depthString = std::format("Avg. depth: {:.2f}", avgDepths);
				const std::string nodesString = std::format("Avg. nodes: {}", avgNodes);

				cout << Console::Gray;
				cout << std::format(" -> {:32}  -> {:32}\n", outcomeString, lengthString);
				cout << std::format(" -> {:32}  -> {:32}\n", depthString, nodesString);
				cout << Console::White << std::flush;
			}
#endif
		}

	}
}

// Viriformat handling ----------------------------------------------------------------------------

void ViriformatGame::SetStartingBoard(const Board& b, const CastlingConfiguration& cc) {
	startingBoard = b;
	castlingConfig = cc;
}

void ViriformatGame::AddMove(const Move& move, const int eval) {
	moves.push_back({ ToViriformatMove(move), static_cast<int16_t>(eval) });
}

void ViriformatGame::Finish(const GameState state) {
	assert(state != GameState::Playing);
	outcome = state;
}

// Converts Renegade's move representation for Viriformat
uint16_t ViriformatGame::ToViriformatMove(const Move& m) const {
	const uint8_t flag = [&] {
		switch (m.flag) {
		case MoveFlag::ShortCastle: return 0b10'00;
		case MoveFlag::LongCastle: return 0b10'00;
		case MoveFlag::EnPassantPerformed: return 0b01'00;
		case MoveFlag::PromotionToKnight: return 0b11'00;
		case MoveFlag::PromotionToBishop: return 0b11'01;
		case MoveFlag::PromotionToRook: return 0b11'10;
		case MoveFlag::PromotionToQueen: return 0b11'11;
		default: return 0b00'00;
		}
	}();
	return (m.from) | (m.to << 6) | (flag << 12);
}

void ViriformatGame::WriteToFile(std::ofstream& stream) const {

	// Marlinformat starting entry (32 bytes)
	const uint64_t startingOccupancy = startingBoard.GetOccupancy();
	uint64_t occupancy = startingOccupancy;
	std::array<uint8_t, 16> pieces{}; // 32 x 4 bits
	int i = 0;
	while (occupancy) {
		const uint8_t sq = Popsquare(occupancy);
		const uint8_t piece = startingBoard.GetPieceAt(sq);
		uint8_t marlinformatPiece = TypeOfPiece(piece) - 1 + (ColorOfPiece(piece) == PieceColor::Black) * 8;

		// Extra checks for rooks, rooks that can still castle have a different encoding
		if (piece == Piece::WhiteRook) {
			if ((sq == castlingConfig.WhiteShortCastleRookSquare && startingBoard.WhiteRightToShortCastle)
				|| (sq == castlingConfig.WhiteLongCastleRookSquare && startingBoard.WhiteRightToLongCastle))
				marlinformatPiece = 6;
		}
		else if (piece == Piece::BlackRook) {
			if ((sq == castlingConfig.BlackShortCastleRookSquare && startingBoard.BlackRightToShortCastle)
				|| (sq == castlingConfig.BlackLongCastleRookSquare && startingBoard.BlackRightToLongCastle))
				marlinformatPiece = 14;
		}

		const auto [index, high] = std::div(i, 2);
		pieces[index] |= marlinformatPiece << (high ? 4 : 0);
		i++;
	}

	const uint8_t encodedSideToMove = startingBoard.Turn == Side::Black;
	const uint8_t encodedEpSquare = (startingBoard.EnPassantSquare == -1) ? 64 : static_cast<uint8_t>(startingBoard.EnPassantSquare);

	const uint8_t encodedOutcome = [&] {
		if (outcome == GameState::BlackVictory) return 0;
		if (outcome == GameState::Drawn) return 1;
		if (outcome == GameState::WhiteVictory) return 2;
		assert(false);
		return 3;
	}();

	const uint8_t encodedEpAndStm = (encodedSideToMove << 7) | encodedEpSquare;
	const int16_t encodedScore = 0; // unused
	const uint8_t extraByte = (DFRC << 7) | datagenVersion;

	stream.write(reinterpret_cast<const char*>(&startingOccupancy), 8);
	stream.write(reinterpret_cast<const char*>(pieces.data()), 16);
	stream.write(reinterpret_cast<const char*>(&encodedEpAndStm), 1);
	stream.write(reinterpret_cast<const char*>(&startingBoard.HalfmoveClock), 1);
	stream.write(reinterpret_cast<const char*>(&startingBoard.FullmoveClock), 2);
	stream.write(reinterpret_cast<const char*>(&encodedScore), 2);
	stream.write(reinterpret_cast<const char*>(&encodedOutcome), 1);
	stream.write(reinterpret_cast<const char*>(&extraByte), 1);

	// Moves (4 bytes each)
	stream.write(reinterpret_cast<const char*>(moves.data()), 4 * moves.size());

	// Null terminator (4 bytes)
	static constexpr std::array<uint8_t, 4> nullTerminator = { 0, 0, 0, 0 };
	stream.write(reinterpret_cast<const char*>(nullTerminator.data()), 4);
}
