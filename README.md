<div align = "center">
<p><h1>Renegade<br>
<i><h6><sup>Lighthearted attempt at creating a chess engine. ♞</sup></h6></i>
</h1>
</div>

Renegade is a chess engine written in C++ using Visual Studio 2019. It values readability and simplicity, and uses the UCI protocol to communicate, making it easy to connect it to chess frontends. Under construction since October 7, 2022 and released publicly on January 15, 2023.  

The engine is moderately strong, and regularly competes on Lichess over at https://lichess.org/@/RenegadeEngine, as well as in tournaments organized by the engine testing community.  


## Features
**Board representation >** Bitboards, redundant mailbox, board hash vector for threefold repetition detection  

**Move generation >** Pseudolegal, magic bitboards, insufficient material, checkmate and stalemate recognition, attack map generation for legality checking, lookup tables, parallel bitwise sliding attack generation  

**Search >** Alpha-beta pruning, iterative deepening, principal variation search, aspiration windows, quiescence search, transposition table, history heuristic, killer move heuristic, late-move reductions, null move pruning, delta pruning, check extensions, futility & reverse futility pruning, razoring, internal iterative deepening, static exchange evaluation  

**Evaluation >** Material, piece-square tables, tapered evaluation, piece mobility, tempo bonus, doubled & tripled pawn penalty, threats, passed pawn bonus, king safety, outposts, open files, pawn phalanx, drawish endgame recognition

**Misc >** Communication with UCI protocol, pretty print outputs, built-in tuner, time management, perft, opening book support, debug commands


### Known issues & future plans

While Renegade itself is quite good against human players, it has a significantly harder time against other engines. It struggles in complicated positions, and it has a tendency to overestimate its positional advantage and undervalue its material. The engine relies on hand-crafted evaluation and does about 1.5 million nps during search.  

The engine is generally stable and shouldn't make invalid moves or forfeit on time. Some of the issues include the inability to listen for `stop` commands or responding to `isready` while busy. These can cause issues in Banksia.


### Build instructions & system requirements

The project can be compiled using Visual Studio 2019 with C++20 features enabled. The engine makes heavy use of `popcnt` and `lzcnt` instructions, and currently only Windows binaries are compiled, but in the future, I would like to make them for Linux and Mac as well.


## Usage
Like most chess engines, this is a command line application implementing the UCI protocol, which is widely supported by graphical chess programs.  

As of version 0.11.0, if the engine doesn't receive the `uci` command, it defaults to pretty print outputs:  
```
Renegade chess engine 0.11.0 [May 29 2023 12:31:30]
go depth 7
  1/ 1         20      0ms   326knps  h= 0.0%  ->  +0.46  [g1f3]
  2/ 2         93      0ms   154knps  h= 0.0%  ->  +0.18  [g1f3 g8f6]
  3/ 3        272      0ms   281knps  h= 0.0%  ->  +0.39  [g1f3 g8f6 b1c3]
  4/ 6       1059      1ms   530knps  h= 0.0%  ->  +0.18  [g1f3 g8f6 b1c3 b8c6]
  5/ 7       1974      3ms   614knps  h= 0.0%  ->  +0.33  [g1f3 g8f6 b1c3 b8c6 d2d4]
  6/12       5190      6ms   751knps  h= 0.0%  ->  +0.18  [g1f3 g8f6 b1c3 b8c6 d2d4 (+1)]
  7/12      15014     14ms  1036knps  h= 0.0%  ->  +0.14  [g1f3 g8f6 b1c3 b8c6 d2d4 (+2)]
bestmove g1f3
```
Normally the output is colorful, but you can type `plain` to disable that. Depending on your terminal, this may fix possible display issues.  

Most UCI commands are supported including limiting search to a specific depth, time limit or node count. Hash size can be configured, and it automatically loads any  `book.bin` opening book it finds in the same folder, but only uses it when `OwnBook` is set to `true`.  

Furthermore, there are some custom commands, including `eval`, `draw` and `fen`. Perft test can be accessed by typing `go perft [n]` or `go perftdiv [n]`.

## Acknowledgments
Getting this far would not have been possible without the contributors of the [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) and the members of the [Engine Programming Discord](https://github.com/EngineProgramming/engine-list), and I'm deeply grateful for [Maksim Korzh](https://youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) and [Bluefever Software](https://youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) for making a video series on this subject.  

Additionally, I would also like to thank everyone who spent the time trying out and testing the engine, which provides valuable feedback for me.  