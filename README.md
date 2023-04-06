<div align = "center">
<p><h1>Renegade<br>
<i><h6><sup>Lighthearted attempt at creating a chess engine. ♞</sup></h6></i>
</h1>
</div>

Renegade is a chess engine written in C++ using Visual Studio 2019. It values readability and simplicity, and uses the UCI protocol to communicate, making it easy to connect it to chess frontends. Under construction since October 7, 2022 and released publicly on January 15, 2023.  

The engine is moderately strong, and regularly competes on Lichess over at https://lichess.org/@/RenegadeEngine.


## Features
**Board representation >** Bitboards, dynamically generated lookup tables, threefold repetition detection  

**Move generation >** Pseudolegal, magic bitboards, insufficient material, checkmate and stalemate recognition, attack map generation for legality checking, lookup tables, parallel bitwise sliding attack generation  

**Search >** Alpha-beta pruning, iterative deepening, move ordering, principal variation search, quiescence search, transposition table, history heuristic, killer move heuristic, late-move reductions, null move pruning, delta pruning, check extensions, futility & reverse futility pruning, razoring, internal iterative deepening, drawish endgame recognition  

**Evaluation >** Material, piece-square tables, tapered evaluation, piece mobility, tempo bonus, doubled & tripled pawn penalty, threats, passed pawn bonus, king safety, outposts, open files

**Misc >** Communication with UCI protocol, Texel's tuning method, time management, perft, opening book support, debug commands


### Known issues & future plans

While Renegade itself is quite good against human players, it has a significantly harder time against other engines. The move generation is quite slow, only achieving about 10-15 million nps in perft and 1 million nps during evaluation, and a lot of pruning ideas are also remain to be implemented as well. To compensate for this, the evaluation function is a bit more thorough. 

The engine is generally stable, and shouldn't make invalid moves or forfeit on time. Some of the issues include the inability to listen for `stop` commands and occasionally outputting incorrect PV lines.


### Build instructions & system requirements

The project can be compiled using Visual Studio 2019 with C++20 features enabled. The engine makes heavy use of `popcnt` and `lzcnt` instructions, and currently only Windows binaries are compiled, but in the future I would like to make them for Linux as well.


## Usage
Like most chess engines, this is a command line application implementing the UCI protocol, which is widely supported by graphical chess programs.
If you are familiar with the protocol, you can also try running it by itself.  

Example output:
```
Renegade chess engine 0.9.0 [Mar 15 2023 20:24:03]
go depth 5
info depth 1 seldepth 1 score cp 32 nodes 21 nps 405405 time 0 hashfull 0 pv g1f3
info depth 2 seldepth 2 score cp 18 nodes 81 nps 60187 time 1 hashfull 0 pv g1f3 g8f6
info depth 3 seldepth 3 score cp 32 nodes 605 nps 207284 time 2 hashfull 0 pv g1f3 g8f6 b1c3
info depth 4 seldepth 6 score cp 18 nodes 1920 nps 382371 time 5 hashfull 0 pv g1f3 g8f6 b1c3 b8c6
info depth 5 seldepth 9 score cp 22 nodes 6880 nps 759666 time 9 hashfull 0 pv g1f3 g8f6 b1c3 b8c6 e2e4
bestmove g1f3
```

Most UCI commands are supported including limiting search to a specific depth, time limit or node count. Hash size can be configured, and it automatically loads any  `book.bin` opening book it finds in the same folder, but only uses it when `OwnBook` is set to `true`.  

Furthermore, there are some custom commands, including `eval`, `draw` and `fen`. Perft test can be accessed by typing `go perft [n]` or `go perftdiv [n]`.

## Acknowledgments
Getting this far would not have been possible without the contributors of the [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page), and I'm deeply grateful for [Maksim Korzh](https://youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) and [Bluefever Software](https://youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) for making a video series on this subject.  

Additionally, I would also like to thank everyone who spent the time trying out and testing the engine, which provides valuable feedback for me.  