#include "Utils.h"

// String functions -------------------------------------------------------------------------------

// Trims spaces from the beginning and end of a std::string
std::string Trim(const std::string& str) {
	const size_t first = str.find_first_not_of(' ');
	const size_t last = str.find_last_not_of(' ');
	if (first == std::string::npos) return "";
	return str.substr(first, (last - first + 1));
}

// Splits a std::string by whitespaces
std::vector<std::string> Split(const std::string& cmd) {
	std::stringstream ss(cmd);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	return std::vector<std::string>(begin, end);
}

// Recreates an std::string to only contains lowercase variants of letters
std::string ToLowercase(const std::string& original) {
	std::string lowercase{};
	lowercase.reserve(original.length());
	for (unsigned char c : original) lowercase.push_back(std::tolower(c));
	return lowercase;
}

// Converts a string input into bool if possible (assumes lowercase inputs)
std::optional<bool> ParseUCIBoolean(const std::string_view str) {
	if (str == "true") return true;
	if (str == "false") return false;
	cout << "Error: Unable to parse boolean" << endl;
	return std::nullopt;
}

// Debugging --------------------------------------------------------------------------------------

[[maybe_unused]] void PrintBitboard(const uint64_t bits) {
	cout << "\n" << "Bitboard: ";
	cout << std::hex << std::showbase << bits;
	cout << std::dec << std::noshowbase << " (" << bits << ")\n";
	for (int r = 7; r >= 0; r--) {
		for (int f = 0; f < 8; f++) {
			if (CheckBit(bits, Square(r, f))) cout << " X ";
			else cout << " . ";
		}
		cout << "\n";
	}
	cout << "\n" << endl;
}

// WDL model and reporting ------------------------------------------------------------------------

// Getting the model for a given game ply
std::pair<double, double> ModelWDLForPly(const int ply) {
	const double m = std::min(240.0, static_cast<double>(ply)) / 64.0;
	return {
		(((as[0] * m + as[1]) * m + as[2]) * m) + as[3],
		(((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3]
	};
}

// Get win, draw and loss probabilities per mil for a given internal score and ply
std::tuple<int, int, int> GetWDL(const int score, const int ply) {
	const auto [a, b] = ModelWDLForPly(ply);
	const double x = std::clamp(static_cast<double>(score), -4000.0, 4000.0);

	const double w = std::round(1000.0 / (1.0 + std::exp((a - x) / b)));
	const double l = std::round(1000.0 / (1.0 + std::exp((a + x) / b)));
	const double d = 1000.0 - w - l;

	return {
		static_cast<int>(std::round(w)),
		static_cast<int>(std::round(d)),
		static_cast<int>(std::round(l))
	};
}

// Converts internal units into centipawns, following the convention of 100 cp = 50% chance of winning
int ToCentipawns(const int score, const int ply) {
	if (std::abs(score) >= MateThreshold || score == 0) return score;
	const auto [a, b] = ModelWDLForPly(ply);
	return static_cast<int>(std::round(100.0 * static_cast<double>(score) / a));
}
