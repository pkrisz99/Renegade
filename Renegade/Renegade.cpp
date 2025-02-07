// Renegade chess engine - a lighthearted attempt at creating a chess engine from scratch
// Since 2022

// Components:
// - Engine         : handles communication and interfacing
// - Board          : board representation
// - Position       : move generation & extracting data from the position
// - Search         : main search algorithm
// - Histories      : collecting statistics about the game tree
// - Transpositions : storing data about previously explored positions
// - Move           : move representation
// - Movepicker     : lazily sorting moves, currently very barebones
// - Classical      : handcrafted board evaluation (older and weaker, normally isn't used)
// - Neural         : NNUE board evaluation (default)
// - Datagen        : data generation tool for training NNUE networks
// - Reporting      : output structure used by search & displaying search results
// - Magics         : magic bitboard lookups for sliding pieces
// - Settings       : handling engine-wide options and parameter tuning
// - Utils          : other misc functions, lookup tables and shared variables

#include "Engine.h"

int main(int argc, char* argv[]) {
	std::srand(static_cast<unsigned int>(std::time(0)));
	GenerateMagicTables();
	LoadDefaultNetwork();

	Engine engine = Engine(argc, argv);
	engine.Start();

	return 0;
}
