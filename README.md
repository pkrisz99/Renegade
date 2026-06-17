<div align = "center">
  <h1>Renegade</h1>
</div>

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
- Its neural network was trained entirely on [self-play data](https://www.kaggle.com/datasets/pkrisz/renegade-chess-engine-training-data-viriformat), amounting to over 8.4 billion positions
- The network architecture is a `(768x14hm -> 1600)x2 -> 1x8` perspective net with input buckets and horizontal mirroring, featuring approximately 17.2 million parameters

## Usage

Renegade - like most chess engines - is a command line application implementing the UCI protocol. It should be used alongside a graphical user interface to be able to display the board and to enter moves more easily.

> [!NOTE]
> Precompiled executables for Windows and Linux can be found at [Releases](https://github.com/pkrisz99/Renegade/releases). These builds leverage modern x64 hardware instructions to maximize performance, if it fails to run, please try to [build it from source](https://github.com/pkrisz99/Renegade#compilation).

For advanced users, the engine can also be interacted with using the terminal. Most UCI commands are supported, including limiting search to a specific depth, time limit or node count.

Scores are scaled according to the estimated outcome of the game, an evaluation of 100 centipawns represents a 50% likelihood of winning.

If the engine doesn't receive the `uci` command, it defaults to pretty print outputs (if the compiler supports it):
```
Renegade chess engine 1.2.0 [May  5 2025 19:15:58]
go depth 7
  1/ 1            20   0.00s    57knps  h= 0%  ->  +0.14  15-76-9   e2e4
  2/ 4            74   0.00s    60knps  h= 0%  ->  +0.39  23-71-6   e2e4 c7c5
  3/ 3           188   0.00s    97knps  h= 0%  ->  +0.11  15-76-10  e2e4 c7c5 g1f3
  4/ 6           683   0.00s   224knps  h= 0%  ->  +0.39  23-71-6   e2e4 c7c5 g1f3 g8f6
  5/ 7         1,174   0.00s   302knps  h= 0%  ->  +0.30  20-73-7   e2e4 c7c5 g1f3 e7e6 d2d4
  6/10         2,182   0.01s   401knps  h= 0%  ->  +0.28  19-74-7   e2e4 c7c5 g1f3 e7e6 d2d4 +1
  7/ 9         2,805   0.01s   446knps  h= 0%  ->  +0.33  21-73-7   e2e4 c7c5 g1f3 e7e6 d2d4 +1
bestmove e2e4
```

Some useful custom commands are also implemented, such as `eval`, `draw` and `fen`.

## Compilation

If you would like to compile Renegade for yourself, run the following commands:

```bash
git clone https://github.com/pkrisz99/Renegade.git --depth 1
cd Renegade/Renegade
make
```

The recommended compiler is Clang 22, though older versions should work as long as they support C++20.

> [!NOTE]
> By default this will download the latest development version, rather than the most recent release. It generally should be stable, but there are no guarantees, please report any bugs that you encounter. 

## Acknowledgments
Getting this far would not have been possible without the members of the [Stockfish](https://github.com/official-stockfish/Stockfish) and [Engine Programming Discord](https://github.com/EngineProgramming/engine-list), and the decades of prior research done into chess programming.  

In particular, Renegade makes use of many ideas from [Akimbo](https://github.com/jw1912/akimbo), [Ethereal](https://github.com/AndyGrant/Ethereal), [Motor](https://github.com/martinnovaak/motor), [PlentyChess](https://github.com/Yoshie2000/PlentyChess), [Reckless](https://github.com/codedeliveryservice/Reckless), [Stockfish](https://github.com/official-stockfish/Stockfish), [Stormphrax](https://github.com/Ciekce/Stormphrax) and [Viridithas](https://github.com/cosmobobak/viridithas). These are all fantastic engines, and you are highly encouraged to check them out!  

The neural networks were trained with [bullet](https://github.com/jw1912/bullet), and win-draw-loss models have been calculated using [Stockfish's WDL fitting tool](https://github.com/official-stockfish/WDL_model). Testing changes requires playing a large number of games, and this is being managed using the [OpenBench](https://github.com/AndyGrant/OpenBench) testing framework.  

Additionally, I would also like to thank everyone who helped out, gave feedback, tested the engine, or has otherwise shown interest in this project!