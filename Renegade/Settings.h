#pragma once
#include <tuple>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Utils.h"

// General engine settings ------------------------------------------------------------------------

constexpr int HashMin = 1;
constexpr int HashDefault = 64;
constexpr int HashMax = 1048576;
constexpr int ThreadsMin = 1;
constexpr int ThreadsDefault = 1;
constexpr int ThreadsMax = 1024;
constexpr bool Chess960Default = false;
constexpr bool ShowWDLDefault = true;

namespace Settings {
	inline int Hash = HashDefault;
	inline int Threads = ThreadsDefault;
	inline bool ShowWDL = ShowWDLDefault;
	inline bool UseUCI = false;
	inline bool Chess960 = Chess960Default;
}

// Search parameter tuning ------------------------------------------------------------------------

struct Tunable {
	const std::string_view name;
	const int default_value;
	const int min;
	const int max;
	const int step;
	int value = default_value;
};

inline std::unordered_map<std::string_view, Tunable&> TunableParameterList;

#define ADD_TUNABLE(NAME, DEFAULT, MIN, MAX, STEP)                                         \
	inline Tunable NAME = Tunable{ #NAME, DEFAULT, MIN, MAX, STEP };                       \
	namespace Internal {                                                                   \
		struct RegisterTunable_##NAME {                                                    \
			RegisterTunable_##NAME() { TunableParameterList.emplace(NAME.name, NAME); }    \
		};                                                                                 \
		inline RegisterTunable_##NAME registerTunable_##NAME;                              \
	};                                                                                     \
	inline int tune_##NAME() { return NAME.value; }


inline void PrintTunableParameters() {
	for (const auto& [name, param] : TunableParameterList) {
		cout << "option name " << param.name << " type spin default " << param.default_value
			<< " min " << param.min << " max " << param.max << endl;
	}
}

inline void GenerateOpenBenchTuningString() {
	cout << "-------------------------------------------" << endl;
	cout << "Currently " << TunableParameterList.size() << " tunable values are plugged in:" << endl;
	cout << "-------------------------------------------" << endl;
	for (const auto& [name, param] : TunableParameterList) {
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

inline bool IsTuningActive() {
	return TunableParameterList.size() != 0;
}

inline bool HasTunableParameter(const std::string name) {
	return (TunableParameterList.find(name) != TunableParameterList.end());
}

inline void SetTunableParameter(const std::string name, const int value) {
	TunableParameterList.at(name).value = value;
}

// Add tunable parameters here --------------------------------------------------------------------
// Definition here: ADD_TUNABLE(name, default, min, max, step)
// Usage in code:   tune_name()

ADD_TUNABLE(nnlmr_0, -192, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_1, -306, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_2, 762, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_3, 1325, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_4, 115, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_5, -696, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_6, 1591, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_7, 1276, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_8, -1040, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_9, 711, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_10, -1163, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_11, -806, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_12, 445, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_13, -1057, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_14, -1492, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_15, -774, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_16, -1302, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_17, -12, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_18, -1279, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_19, -1701, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_20, 103, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_21, -392, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_22, 118, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_23, -1319, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_24, -265, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_25, 782, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_26, -734, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_27, 41, -4000, 4000, 100)
ADD_TUNABLE(nnlmr_28, 411, -4000, 4000, 50)
