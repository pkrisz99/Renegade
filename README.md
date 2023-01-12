# Renegade chess engine ♞

Renegade is a chess engine written in C++ using Visual Studio 2019. It values readability and simplicity, and uses the UCI protocol to communicate, making it easy to connect it to chess frontens. Under construction since October 7, 2022.  

The engine is moderately strong, and regularly competes on Lichess over at https://lichess.org/@/RenegadeEngine.

## Features
**Board representation>** Bitboards with a generated lookup table, threefold repetition detection, checkmate and stalemate detection  

**Move generation>** Pseudolegal, attack map generation for legality checking, lookup tables, parallel sliding attack generation  

**Search>** Alpha-beta pruning, iterative deepening, move ordering, principal variation search, quiescence search, transposition table, killer move heuristic, null move pruning, delta pruning, check extensions  

**Evaluation>** Material, piece-square tables, tapered evaluation, tempo bonus, defended pawn bonus, king safety

**Misc>** Communication with UCI protocol, Texel's tuning method, time management, perft, debug commands

## Usage
Renegade - like most chess engines - is a command line application, and it uses the UCI protocol, which is widely supported by chess frontends.
If you are familiar with the protocol, you can also try running it by itself.

Example output:
```
Renegade chess engine 0.6.0 [Dec 24 2022 23:21:29]
go depth 6
info depth 1 seldepth 2 score cp 50 nodes 20 nps 192678 time 0 hashfull 0 pv g1f3
info depth 2 seldepth 3 score cp 0 nodes 59 nps 30182 time 1 hashfull 0 pv g1f3 g8f6
info depth 3 seldepth 4 score cp 50 nodes 523 nps 104189 time 5 hashfull 0 pv g1f3 g8f6 b1c3
info depth 4 seldepth 9 score cp 0 nodes 1388 nps 90673 time 15 hashfull 0 pv g1f3 g8f6 b1c3 b8c6
info depth 5 seldepth 12 score cp 35 nodes 9025 nps 153290 time 58 hashfull 0 pv g1f3 g8f6 b1c3 b8c6 e2e4
info depth 6 seldepth 18 score cp 0 nodes 23190 nps 151602 time 152 hashfull 0 pv g1f3 g8f6 b1c3 b8c6 e2e4 e7e5
bestmove g1f3
```
