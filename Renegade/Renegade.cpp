// Renegade chess engine - a lighthearted attempt at creating a chess engine from scratch
// Since 2022

// Files:
// - Engine.cpp     : handles search, communication and interfacing
// - Board.cpp      : board representation and move generation
// - Heuristics.cpp : collection of methods to make search more efficient (e.g. transposition table stuff)
// - Move.cpp       : move representation
// - Evaluation.cpp : evaluation object
// - Utils.cpp      : other misc functions and shared variables

// Requirements:
// Processors released after 2013:
// - Intel Haswell (Core 4th gen)
// - AMD Piledriver
// The code currently uses bit manipulation instructions, thus requiring a recent-ish CPU.
// It is only at a few places, so substituting them for a more compatible method shouldn't
// be terribly difficult.

// Compilation:
// Renegade is written using Visual Studio 2019 ... 


#include "Engine.h"

int main() {
    Engine engine = Engine();
    engine.Start();
    return 0;
}
