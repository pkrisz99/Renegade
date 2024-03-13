// Renegade chess engine - a lighthearted attempt at creating a chess engine from scratch
// Since 2022

// Files:
// - Engine.cpp     : handles communication and interfacing
// - Board.cpp      : board representation and move generation
// - Search.cpp     : search algorithm
// - Heuristics.cpp : collection of methods to make search more efficient (e.g. transposition table stuff)
// - Move.cpp       : move representation
// - Evaluation.cpp : handcrafted board evaluation (normally isn't used)
// - Neurals.cpp    : NNUE board evaluation (default)
// - Datagen.cpp    : data generation tool for NNUE networks
// - Reporting.cpp  : output structure used by search & displaying search results
// - Magics.cpp     : magic bitboard stuff for sliding pieces
// - Utils.cpp      : other misc functions, lookup tables and shared variables

#include "Engine.h"

int main(int argc, char* argv[]) {
    Engine engine = Engine(argc, argv);
    engine.Start();
    return 0;
}
