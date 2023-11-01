#pragma once
#include <fstream>
#include "Board.h"
#include "Search.h"
#include "Utils.h"

class Datagen
{
public:
	Datagen();
	void Start();
	bool Filter(const Board& board, const Move& move, const int eval) const;
	void ShuffleEntries();

	std::string ToMarlinformat(const std::pair<std::string, int>& position, const GameState outcome) const;
	// <fen> | <eval> | <wdl>
	// eval: white pov in cp, wdl 1.0 = white win, 0.0 = black win


	std::vector<std::string> Generated;
	std::vector<std::pair<std::string, int>> CurrentFENs;



};

