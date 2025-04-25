#include "Reporting.h"

// Result structure -------------------------------------------------------------------------------

Results::Results() {
	pv = std::vector<Move>();
}

Results::Results(const int score, const int depth, const int seldepth, const uint64_t nodes, const uint64_t time,
	const uint64_t nps, const int hashfull, const int ply, const std::vector<Move>& pv) {

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
}

Move Results::BestMove() const {
	return (pv.size() != 0) ? pv.front() : NullMove;
}

// Communicating the search results ---------------------------------------------------------------

void PrintInfo(const Results& r) {
	if (!Settings::UseUCI) {
		PrintPretty(r);
		return;
	}

	const int normalizedScore = ToCentipawns(r.score, r.ply);

	std::string score{};
	if (IsMateScore(r.score)) {
		const int movesToMate = (MateEval - std::abs(r.score) + 1) / 2;
		if (r.score > 0) score = "mate " + std::to_string(movesToMate);
		else score = "mate -" + std::to_string(movesToMate);
	}
	else {
		score = "cp " + std::to_string(normalizedScore);
	}

	std::string pvString{};
	for (const Move& move : r.pv)
		pvString += " " + move.ToString(Settings::Chess960);

	std::string wdlOutput{};
	if (Settings::ShowWDL) {
		const auto [w, d, l] = GetWDL(r.score, r.ply);
		wdlOutput = " wdl " + std::to_string(w) + " " + std::to_string(d) + " " + std::to_string(l);
	}

#if defined(_MSC_VER)
	const std::string output = std::format("info depth {} seldepth {} score {}{} nodes {} nps {} time {} hashfull {} pv{}",
		r.depth, r.seldepth, score, wdlOutput, r.nodes, r.nps, r.time, r.hashfull, pvString);

#else
	const std::string output = "info depth " + std::to_string(r.depth) + " seldepth " + std::to_string(r.seldepth)
		+ " score " + score + wdlOutput + " nodes " + std::to_string(r.nodes) + " nps " + std::to_string(r.nps)
		+ " time " + std::to_string(r.time) + " hashfull " + std::to_string(r.hashfull)
		+ " pv" + pvString;
#endif

	cout << output << endl;
}

// Printing current search results ----------------------------------------------------------------

void PrintPretty([[maybe_unused]] const Results& r) {
#if __cpp_lib_format

	// Basic search information:

	const std::string outputDepth = std::format("{:2d}/{:2d}", r.depth, r.seldepth);

	const std::string outputNodes = Console::FormatInteger(r.nodes);

	const std::string outputTime = [&] {
		if (r.time < 60000) return std::format("{:>5.2f}s", r.time / 1000.0);
		const int seconds = r.time / 1000;
		const auto [m, s] = std::div(seconds, 60);
		return std::format("{}m{:02d}s", m, s);
	}();

	const std::string outputHash = std::format("{:>2d}", std::min(r.hashfull / 10, 99));

	const std::string outputNps = [&] {
		if (r.threads == 1) return std::format("{:>4d}knps", r.nps / 1000);
		else return std::format("{:>4.1f}mnps", r.nps / 1e6);
	}();

	const std::string outputSearch = std::format(" {}{} {}{:>13} {:>7} {:>9}  h={}%  {}->",
		Console::White, outputDepth, Console::Gray, outputNodes, outputTime, outputNps, outputHash, Console::White);

	// Evaluation and win-draw-loss:

	constexpr int neutralMargin = 50;
	const int normalizedScore = ToCentipawns(r.score, r.ply);

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

	const auto [modelW, modelD, modelL] = GetWDL(r.score, r.ply);
	const int w = static_cast<int>(std::round(modelW / 10.0));
	const int d = static_cast<int>(std::round(modelD / 10.0));
	const int l = static_cast<int>(std::round(modelL / 10.0));
	const std::string outputWdl = std::format("  {}{:<8}", 
		Console::Gray, std::to_string(w) + "-" + std::to_string(d) + "-" + std::to_string(l));

	// Principal variation:

	constexpr int movesShown = 5;
	const int pvMoveCount = r.pv.size();

	const std::string outputPv = [&] {
		if (pvMoveCount == 0) return "  " + Console::White;

		std::string list{};
		for (int i = 0; i < movesShown; i++) {
			if (i >= pvMoveCount) break;
			list += r.pv[i].ToString(Settings::Chess960) + " ";
		}
		list.pop_back();
		if (pvMoveCount > movesShown) list += Console::Gray + " +" + std::to_string(pvMoveCount - movesShown) + Console::White;
		return "  " + Console::White + list;
	}();

	// Putting it together, and printing to standard output:

	const std::string output = outputSearch + outputScore + outputWdl + outputPv;
	cout << output << endl;

#endif
}

void PrintBestmove(const Move& move) {
	cout << "bestmove " << move.ToString(Settings::Chess960) << endl;
}