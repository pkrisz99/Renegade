#include "Reporting.h"

// Result structure -------------------------------------------------------------------------------

Results::Results() {
	pv = std::vector<Move>();
	stats = SearchStatistics{};
}

Results::Results(const int score, const int depth, const int seldepth, const uint64_t nodes, const int time,
	const int nps, const int hashfull, const int ply, const std::vector<Move>& pv, const SearchStatistics& stats) {

	// This constructor only exists to help with the order of parameters
	this->score = score;
	this->depth = depth;
	this->seldepth = seldepth;
	this->nodes = nodes;
	this->time = time;
	this->nps = nps;
	this->hashfull = hashfull;
	this->ply = ply;
	this->pv = pv;
	this->stats = stats;
}

Move Results::BestMove() const {
	return (pv.size() != 0) ? pv.front() : Move();
}

// Communicating the search results ---------------------------------------------------------------

void PrintInfo(const Results& e) {
	if (!Settings::UseUCI) {
		PrintPretty(e);
		return;
	}

	const int normalizedScore = ToCentipawns(e.score, e.ply);

	std::string score;
	if (IsMateScore(e.score)) {
		int movesToMate = (MateEval - std::abs(e.score) + 1) / 2;
		if (e.score > 0) score = "mate " + std::to_string(movesToMate);
		else score = "mate -" + std::to_string(movesToMate);
	}
	else {
		score = "cp " + std::to_string(normalizedScore);
	}

	/*
	std::string extended;
	if (settings.ExtendedOutput) {
		extended += " evals " + std::to_string(e.stats.Evaluations);
		extended += " qnodes " + std::to_string(e.stats.QuiescenceNodes);
		// extended += " betacutoffs " + std::to_string(e.stats.BetaCutoffs)
		// extended += " fmbc " + std::to_string(e.stats.FirstMoveBetaCutoffs);
		if (e.stats.BetaCutoffs != 0) extended += " fmbcrate " + std::to_string(static_cast<int>(e.stats.FirstMoveBetaCutoffs * 1000 / e.stats.BetaCutoffs));
		extended += " tthits " + std::to_string(e.stats.TranspositionHits);
		if (e.stats.TranspositionQueries != 0) extended += " ttrate " + std::to_string(static_cast<int>(e.stats.TranspositionHits * 1000 / e.stats.TranspositionQueries));
	}*/

	std::string pvString;
	for (const Move& move : e.pv)
		pvString += " " + move.ToString(Settings::Chess960);

	std::string wdlOutput{};
	if (Settings::ShowWDL) {
		const auto [w, l] = GetWDL(e.score, e.ply);
		const int d = 1000 - w - l;
		wdlOutput = " wdl " + std::to_string(w) + " " + std::to_string(d) + " " + std::to_string(l);
	}

#if defined(_MSC_VER)
	const std::string output = std::format("info depth {} seldepth {} score {}{} nodes {} nps {} time {} hashfull {} pv{}",
		e.depth, e.seldepth, score, wdlOutput, e.nodes, e.nps, e.time, e.hashfull, pvString);

#else
	const std::string output = "info depth " + std::to_string(e.depth) + " seldepth " + std::to_string(e.seldepth)
		+ " score " + score + wdlOutput + " nodes " + std::to_string(e.nodes) + " nps " + std::to_string(e.nps)
		+ " time " + std::to_string(e.time) + " hashfull " + std::to_string(e.hashfull)
		+ " pv" + pvString;
#endif

	cout << output << endl;
}

// Printing current search results ----------------------------------------------------------------

void PrintPretty(const Results& e) {
#if defined(_MSC_VER)

	// Basic search information:

	const std::string outputDepth = std::format("{:2d}/{:2d}", e.depth, e.seldepth);

	const std::string outputNodes = [&] {
		if (e.nodes < 1e9) return std::to_string(e.nodes);
		return std::format("{:8.3f}", e.nodes / 1e9) + "b";
	}();

	const std::string outputTime = [&] {
		if (e.time < 60000) return std::to_string(e.time) + "ms";

		int seconds = e.time / 1000;
		int minutes = seconds / 60;
		seconds = seconds - minutes * 60;
		return std::format("{}m{:02d}s", minutes, seconds);
	}();

	const std::string outputHash = [&] {
		if (e.hashfull != 1000) return std::format("{:4.1f}", e.hashfull / 10.0);
		else return std::string("100");
	}();

	const std::string outputNps = [&] {
		if (e.nps < 10'000'000) return std::format("{:>4d}knps", e.nps / 1000);
		else return std::format("{:>4.1f}mnps", e.nps / 1e6);
	}();

	const std::string outputSearch = std::format(" {}{}  {}{:>9}  {:>7}  {}  h={:>4}%  {}->",
		Console::White, outputDepth, Console::Gray, outputNodes, outputTime, outputNps, outputHash, Console::White);

	// Evaluation and win-draw-loss:

	constexpr int neutralMargin = 50;
	const int normalizedScore = ToCentipawns(e.score, e.ply);

	const std::string scoreColor = [&] {
		if (IsMateScore(normalizedScore)) return Console::Yellow;
		else if (normalizedScore > neutralMargin) return Console::Green;
		else if (normalizedScore < -neutralMargin) return Console::Red;
		else return Console::Blue;
	}();

	const std::string outputScore = [&] {
		if (!IsMateScore(normalizedScore)) {
			return std::format("  {}{:>+5.2f}", scoreColor, normalizedScore / 100.0);
		}
		else {
			const int movesToMate = (MateEval - std::abs(normalizedScore) + 1) / 2;
			const std::string outputMate = (normalizedScore > 0 ? "#+" : "#-") + std::to_string(movesToMate);
			return std::format("  {}{:>5}", scoreColor, outputMate);
		}
	}();

	const auto [modelW, modelL] = GetWDL(e.score, e.ply);
	const int modelD = 1000 - modelW - modelL;
	constexpr double q = 10.0;
	const int w = static_cast<int>(std::round(modelW / q));
	const int d = static_cast<int>(std::round(modelD / q));
	const int l = static_cast<int>(std::round(modelL / q));
	const std::string outputWdl = std::format("  {}{:<8}", 
		Console::Gray, std::to_string(w) + "-" + std::to_string(d) + "-" + std::to_string(l));

	// Principal variation:

	constexpr int movesShown = 5;
	const int pvMoveCount = e.pv.size();

	const std::string outputPv = [&] {
		if (pvMoveCount == 0) return "  " + Console::White;

		std::string list{};
		for (int i = 0; i < movesShown; i++) {
			if (i >= pvMoveCount) break;
			list += e.pv[i].ToString(Settings::Chess960) + " ";
		}
		list.pop_back();
		if (pvMoveCount > movesShown) list += " (+" + std::to_string(pvMoveCount - movesShown) + ")";
		return "  " + Console::White + "[" + list + "]";
	}();

	// Putting it together, and printing to standard output:

	const std::string output = outputSearch + outputScore + outputWdl + outputPv;
	cout << output << endl;

#endif
}

void PrintBestmove(const Move& move) {
	cout << "bestmove " << move.ToString(Settings::Chess960) << endl;
}