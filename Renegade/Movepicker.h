#pragma once
#include "History.h"
#include "Position.h"
#include "Utils.h"
#include <algorithm>
#include <array>
#include <memory>


struct MovePicker {
	MoveList& moveList;
	int index;

	MovePicker(MoveList& scoredMoveList) : moveList(scoredMoveList) {
		index = 0;
	}

	std::pair<Move, int> get() {
		assert(hasNext());

		int bestOrderScore = moveList[index].orderScore;
		int bestIndex = index;

		for (int i = index + 1; i < moveList.size(); i++) {
			if (moveList[i].orderScore > bestOrderScore) {
				bestOrderScore = moveList[i].orderScore;
				bestIndex = i;
			}
		}

		std::swap(moveList[bestIndex], moveList[index]);
		index += 1;
		return { moveList[index - 1].move, moveList[index - 1].orderScore };
	}

	bool hasNext() const {
		return index < moveList.size();
	}
};


