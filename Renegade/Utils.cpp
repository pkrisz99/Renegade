#include "Utils.h"

std::string PolyglotMoveToString(const uint8_t from, const uint8_t to, const uint8_t promotion) {
	int file1 = from % 8;
	int rank1 = from / 8;
	int file2 = to % 8;
	int rank2 = to / 8;

	char f1 = 'a' + file1;
	char r1 = '1' + rank1;
	char f2 = 'a' + file2;
	char r2 = '1' + rank2;

	char extra = '?';
	if (promotion == 1) extra = 'n';
	else if (promotion == 2) extra = 'b';
	else if (promotion == 3) extra = 'r';
	else if (promotion == 4) extra = 'q';

	if (extra == '?') return { f1, r1, f2, r2 };
	else return { f1, r1, f2, r2 };
}

// https://stackoverflow.com/questions/25829143/trim-whitespace-from-a-string
std::string Trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (first == std::string::npos) return "";
	if (std::string::npos == first) return str;
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

std::vector<std::string> Split(const std::string cmd) {
	std::stringstream ss(cmd);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	return std::vector<std::string>(begin, end);
}

// https://stackoverflow.com/questions/8095088/how-to-check-string-start-in-c
bool StartsWith(const std::string big, const std::string small) {
	return big.compare(0, small.length(), small) == 0;
}

void ConvertToLowercase(std::string& str) {
	for (int x = 0; x < str.length(); x++) str[x] = tolower(str[x]);
}

std::string StateString(GameState s) {
	if (s == GameState::Playing) return "Playing";
	if (s == GameState::WhiteVictory) return "WhiteVictory";
	if (s == GameState::BlackVictory) return "BlackVictory";
	if (s == GameState::Draw) return "Draw";
	return "Unknown";
}

void ClearScreen(bool endline, bool fancy) {
	if (fancy) {
		cout << "\033[2J\033[1;1H";
		if (endline) cout << endl;
	}
	else {
		for (int i = 0; i < 6; i++) cout << '\n';
		cout << endl;
	}
}

void PrintBitboard(const uint64_t bits) {
	cout << "\n" << bits << '\n';
	for (int r = 7; r >= 0; r--) {
		for (int f = 0; f < 8; f++) {
			if (CheckBit(bits, Square(r, f))) cout << " X ";
			else cout << " . ";
		}
		cout << "\n";
	}
	cout << "\n" << endl;
}

std::ostream& operator<<(std::ostream& os, const TaperedScore& s) {
	return os << "S(" << s.early << ", " << s.late << ")";
}