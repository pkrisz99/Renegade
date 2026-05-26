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
	}                                                                                      \
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

ADD_TUNABLE(rfp_linear, 131, 70, 145, 12)
ADD_TUNABLE(rfp_improving, 71, 50, 120, 12)
ADD_TUNABLE(rfp_lerp, 128, 30, 250, 12)

ADD_TUNABLE(nmp_eval_divider, 226, 150, 350, 35)

ADD_TUNABLE(lmp_histdiv, 8192, 3000, 17000, 1700)

ADD_TUNABLE(fp_const, 53, 0, 100, 12)
ADD_TUNABLE(fp_linear, 100, 70, 150, 12)
ADD_TUNABLE(fp_improving, 52, 10, 80, 12)

ADD_TUNABLE(hp_linear, 6460, 2000, 16000, 1100)

ADD_TUNABLE(lmr_multiplier, 42, 30, 60, 5)
ADD_TUNABLE(lmr_base, 78, 50, 100, 7)

ADD_TUNABLE(lmr_nottpv, 313, 100, 500, 45)
ADD_TUNABLE(lmr_cutoffcnt, 274, 100, 500, 45)
ADD_TUNABLE(lmr_cutnode, 346, 100, 512, 45)
ADD_TUNABLE(lmr_improving, 304, 100, 512, 45)
ADD_TUNABLE(lmr_check, 205, 64, 512, 35)

ADD_TUNABLE(lmr_history_div, 22610, 15000, 30000, 2500)
ADD_TUNABLE(lmr_history_cap, 490, 256, 768, 75)
ADD_TUNABLE(lmr_deeper, 34, 22, 46, 5)

ADD_TUNABLE(ext_triple, 187, 50, 230, 35)
ADD_TUNABLE(ext_triple_adj, 32, 12, 64, 5)

ADD_TUNABLE(see_histdiv, 64, 20, 90, 8)

ADD_TUNABLE(ldse_margin, 25, 10, 50, 4)

ADD_TUNABLE(qsfp_margin, 278, 150, 450, 36)


ADD_TUNABLE(capthist_mul, 18, 13, 25, 2)
ADD_TUNABLE(capthist_div, 31, 16, 48, 4)

ADD_TUNABLE(refut_killer, 16220, 8000, 40000, 4000)
ADD_TUNABLE(refut_positional, 16330, 4000, 32000, 4000)
ADD_TUNABLE(refut_counter, 19380, 4000, 32000, 4000)


ADD_TUNABLE(history_q_clamp, 3245, 2000, 4000, 330)
ADD_TUNABLE(history_q_main_cap, 15543, 8000, 30000, 1700)
ADD_TUNABLE(history_qb, 309, 150, 500, 33)
ADD_TUNABLE(history_qm, 309, 150, 500, 33)

ADD_TUNABLE(history_qc_clamp, 3245, 2000, 4000, 330)
ADD_TUNABLE(history_qc_main_cap, 15543, 8000, 30000, 1500)
ADD_TUNABLE(history_qcb, 309, 150, 500, 33)
ADD_TUNABLE(history_qcm, 309, 150, 500, 33)
ADD_TUNABLE(history_q_conthist_cap, 15543, 8000, 30000, 1700)

ADD_TUNABLE(history_n_clamp, 2832, 2000, 4000, 320)
ADD_TUNABLE(history_n_cap, 18235, 8000, 30000, 1900)
ADD_TUNABLE(history_nb, 302, 150, 500, 30)
ADD_TUNABLE(history_nm, 302, 150, 500, 30)

ADD_TUNABLE(corrhist_cap, 10837, 7000, 14000, 1200)
ADD_TUNABLE(corrhist_inertia, 160, 80, 300, 22)
ADD_TUNABLE(corrhist_p, 256, 80, 500, 30)
ADD_TUNABLE(corrhist_lm, 256, 80, 500, 30)








