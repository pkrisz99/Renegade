// Renegade chess engine - a lighthearted attempt at creating a chess engine from scratch
// Since 2022

// Files:
// - Engine.cpp     : handles communication and interfacing
// - Board.cpp      : board representation and move generation
// - Search.cpp     : search algorithm
// - Heuristics.cpp : collection of methods to make search more efficient (e.g. transposition table stuff)
// - Move.cpp       : move representation
// - Evaluation.cpp : static board evaluation
// - Tuner.cpp      : engine parameter tuner
// - Utils.cpp      : other misc functions, lookup tables and shared variables

// Requirements:
// Processors released after 2013:
// - Intel Haswell (Core 4th gen)
// - AMD Piledriver
// The code currently uses bit manipulation instructions, thus requiring a recentish CPU.
// It is only at a few places, so substituting them for a more compatible method shouldn't
// be terribly difficult.

#include "Engine.h"

int main() {
    Engine engine = Engine();
    engine.Start();
    return 0;
}
