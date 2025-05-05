<div align = "center"><h1>Renegade</h1></div>

**Renegade is a free and open source chess engine. ♞**  

It is written in C++ from scratch, and values readability and simplicity. As the engine communicates through the UCI protocol, it can be connected to almost all chess user interfaces.  

The engine is fairly strong, and regularly competes in various tournaments organized by the engine testing community.  

## Features
### Board representation
- Uses bitboards to represent the location of the pieces on the board, which allows for fast and efficient operations
- Move generation relies on a number of precalculated tables, including plain magic bitboards for sliding pieces
- In addition to standard chess, it also supports the popular variants of FRC (Chess960) and DFRC

### Search
- The engine uses a fail-soft alpha-beta pruning framework with iterative deepening and principal variation search
- A large number of move ordering and pruning methods are implemented to make search more efficient (see `Search.cpp`)
- Supports multithreaded search and can utilize hundreds of threads on high-end workstations

### Evaluation
- Renegade makes use of modern NNUE (efficiently updatable neural network) technology for accurate position evaluation
- Its neural network was trained entirely on [self-play data](https://www.kaggle.com/datasets/pkrisz/renegade-chess-engine-training-data), amounting to over 5.6 billion positions
- The network architecture is a `(768x14hm -> 1600)x2 -> 1x8` perspective net with input buckets and horizontal mirroring, featuring approximately 18.5 million parameters

## Usage
Renegade - like most chess engines - is a command line application implementing the UCI protocol. It should be used alongside a graphical user interface to be able to display the board and to enter moves more easily.

For advanced users, the engine can also be interacted with using the terminal. Most UCI commands are supported, including limiting search to a specific depth, time limit or node count.

Scores are scaled according to the estimated outcome of the game, an evaluation of 100 centipawns represents a 50% likelihood of winning.

If the engine doesn't receive the `uci` command, it defaults to pretty print outputs (if the compiler supports it):
```
Renegade chess engine 1.1.0 [Jun 26 2024 20:50:06]
go depth 7
  1/ 1         20      0ms   156knps  h= 0.0%  ->  +0.27  29-54-17  [d2d4]
  2/ 4        110      0ms   378knps  h= 0.0%  ->  +0.34  31-54-16  [d2d4 d7d5]
  3/ 3        265      0ms   572knps  h= 0.0%  ->  +0.22  27-55-18  [e2e4 e7e5 g1f3]
  4/ 6        685      0ms   732knps  h= 0.0%  ->  +0.44  33-53-14  [e2e4 e7e5 g1f3 b8c6]
  5/ 7       1473      1ms   952knps  h= 0.0%  ->  +0.25  28-54-18  [d2d4 e7e6 g1f3 g8f6 a2a3]
  6/ 8       2113      1ms  1089knps  h= 0.0%  ->  +0.22  27-55-18  [d2d4 e7e6 g1f3 g8f6 a2a3 (+1)]
  7/11       5495      3ms  1607knps  h= 0.0%  ->  +0.20  27-55-18  [g1f3 d7d5 g2g3 g7g6 c2c4 (+2)]
bestmove g1f3
```

Some useful custom commands are also implemented, such as `eval`, `draw` and `fen`.

## Compilation

If you would like to compile Renegade for yourself, run the following commands:

```bash
git clone https://github.com/pkrisz99/Renegade.git
cd Renegade/Renegade
make
```

The recommended compiler is Clang 20, though older versions should work as long as they support C++20.

Precompiled executables for Windows and Linux can be found for each release, but note that these builds leverage modern hardware instructions to maximize performance.

## Acknowledgments
Getting this far would not have been possible without the members of the [Stockfish](https://github.com/official-stockfish/Stockfish) and [Engine Programming Discord](https://github.com/EngineProgramming/engine-list), and the decades of prior research done into chess programming.  

In particular, Renegade makes use of many ideas from [Akimbo](https://github.com/jw1912/akimbo), [Ethereal](https://github.com/AndyGrant/Ethereal), [Motor](https://github.com/martinnovaak/motor), [Stockfish](https://github.com/official-stockfish/Stockfish), [Stormphrax](https://github.com/Ciekce/Stormphrax), [Viridithas](https://github.com/cosmobobak/viridithas) and [Wahoo](https://github.com/spamdrew128/Wahoo). These are all fantastic engines, and you are highly encouraged to check them out!  

The neural networks were trained with [bullet](https://github.com/jw1912/bullet), and win-draw-loss models have been calculated using [Stockfish's WDL fitting tool](https://github.com/official-stockfish/WDL_model). Testing changes requires playing a large number of games, and this is being managed using the [OpenBench](https://github.com/AndyGrant/OpenBench) testing framework.  

Additionally, I would also like to thank everyone who gave me feedback, and those who spent the time trying out and testing the engine!