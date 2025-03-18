#pragma once
#include <tuple>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Utils.h"

// General engine settings:

constexpr int HashMin = 1;
constexpr int HashDefault = 64;
constexpr int HashMax = 65536;
constexpr int ThreadsMin = 1;
constexpr int ThreadsDefault = 1;
constexpr int ThreadsMax = 256;
constexpr bool Chess960Default = false;
constexpr bool ShowWDLDefault = true;

namespace Settings {
	inline int Hash = HashDefault;
	inline int Threads = ThreadsDefault;
	inline bool ShowWDL = ShowWDLDefault;
	inline bool UseUCI = false;
	inline bool Chess960 = Chess960Default;
}

// Search parameter tuning:

struct Tunable {
	const std::string_view name;
	const int default_value;
	const int min;
	const int max;
	const int step;
	int value = default_value;
};

#define ADD_TUNABLE(NAME, DEFAULT, MIN, MAX, STEP)                         \
	inline Tunable NAME = Tunable{ #NAME, DEFAULT, MIN, MAX, STEP };       \
	struct RegisterTunable_##NAME {                                        \
		RegisterTunable_##NAME() { Tune::List.emplace(NAME.name, NAME); }  \
	};                                                                     \
	inline RegisterTunable_##NAME registerTunable_##NAME;

namespace Tune {

	inline std::unordered_map<std::string_view, Tunable&> List;

	// === Add parameters to be tuned here ===
	// Format is: ADD_TUNABLE(name, default, min, max, step)

	// =======================================	

	inline void PrintParameters() {
		for (const auto& [name, param] : List) {
			cout << "option name " << param.name << " type spin default " << param.default_value << " min " << param.min << " max " << param.max << endl;
		}
	}

	inline void GenerateOpenBenchString() {
		cout << "-------------------------------------------" << endl;
		cout << "Currently " << List.size() << " tunable values are plugged in:" << endl;
		for (const auto& [name, param] : List) {
			cout << param.name << ", ";
			cout << "int" << ", ";
			cout << param.default_value << ", ";
			cout << param.min << ", ";
			cout << param.max << ", ";
			cout << param.step << ", ";
			cout << "0.002 " << endl;
		}
		cout << "-------------------------------------------" << endl;
	}

	inline bool IsActive() {
		return List.size() != 0;
	}

	inline bool HasParameter(const std::string name) {
		return (List.find(name) != List.end());
	}

	inline void SetParameter(const std::string name, const int value) {
		List.at(name).value = value;
	}

}
