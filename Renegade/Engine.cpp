#include "Engine.h"

void Engine::StartSearch(Position pos, SearchParams params) {
	position = pos;
	SearchThread = std::thread([&]() {
		Searcher.SearchMoves(position, params, true);
	});
}

void Engine::StopSearch() {
	Searcher.Aborting.store(true, std::memory_order_relaxed);
	SearchThread.join();
}

void Engine::JoinThreads() {
	if (SearchThread.joinable()) SearchThread.join();
}

bool Engine::IsBusy() const {
	return !Searcher.Aborting.load(std::memory_order_relaxed);
}




void Engine::ClearHash() {
	Searcher.TranspositionTable.Clear();
}

void Engine::SetHashSize(const int megabytes) {
	Searcher.TranspositionTable.SetSize(megabytes);
}

void Engine::Reset() {
	Searcher.ResetState(true);
}