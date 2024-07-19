#include "Engine.h"

void Engine::StartSearch(Position pos, SearchParams sp) {
	position = pos;
	params = sp;
	SearchThread = std::thread([&]() {
		Searcher.SearchMoves(position, params, true);
	});
}

void Engine::StopSearch() {
	Searcher.State.store(SearchState::Aborting, std::memory_order_relaxed);
	SearchThread.join();
}

void Engine::JoinThreads() {
	if (SearchThread.joinable()) SearchThread.join();
}

bool Engine::IsBusy() const {
	return Searcher.State.load(std::memory_order_relaxed) != SearchState::Idle;
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