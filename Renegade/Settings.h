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

ADD_TUNABLE(nnlmr_0, 500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_1, -100, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_2, 600, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_3, 1500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_4, -200, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_5, -200, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_6, 1600, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_7, 800, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_8, -500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_9, 500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_10, -500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_11, -500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_12, 200, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_13, -1900, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_14, -1700, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_15, -600, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_16, -1000, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_17, 300, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_18, -900, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_19, -1400, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_20, 1500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_21, -200, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_22, 100, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_23, -1400, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_24, -500, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_25, 100, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_26, -1200, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_27, 400, -4000, 4000, 500)
ADD_TUNABLE(nnlmr_28, -600, -4000, 4000, 500)
