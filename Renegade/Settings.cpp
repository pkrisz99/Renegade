#include "Settings.h"

namespace Settings {
    int Hash = HashDefault;
    bool ShowWDL = ShowWDLDefault;
    bool UseUCI = false;
}

namespace Tune {

    // Define values here:

    /*Tunable see_pawn = Tunable{"see_pawn", 50, 250, 100, 20};
    Tunable see_knight = Tunable{ "see_knight", 250, 500, 300, 20 };
    Tunable see_bishop = Tunable{ "see_bishop", 250, 500, 300, 20 };
    Tunable see_rook = Tunable{ "see_rook", 400, 850, 500, 40 };
    Tunable see_queen = Tunable{ "see_queen", 800, 1300, 1000, 40 };

    Tunable see_quiet = Tunable{ "see_quiet", 30, 80, 50, 10 };
    Tunable see_noisy = Tunable{ "see_noisy", 60, 130, 100, 10 };
    Tunable see_search_th = Tunable{ "see_search_th", 0, 100, 0, 20 };
    Tunable see_qsearch_th = Tunable{ "see_qsearch_th", 0, 100, 0, 20 };*/

    std::unordered_map<std::string_view, Tunable&> List = {
        /*
        {see_pawn.name, see_pawn},
        {see_knight.name, see_knight},
        {see_bishop.name, see_bishop},
        {see_rook.name, see_rook},
        {see_queen.name, see_queen},
        {see_quiet.name, see_quiet},
        {see_noisy.name, see_noisy},
        {see_search_th.name, see_search_th},
        {see_qsearch_th.name, see_qsearch_th},*/
    };

    // Helper functions:

    void PrintOptions() {
        for (const auto& [name, param] : List) {
            cout << "option name " << param.name << " type spin default " << param.default_value << " min " << param.min << " max " << param.max << endl;
        }
    }

    void GenerateString() {
        cout << "Currently " << List.size() << " tunable values are plugged in:" << endl;
        for (const auto& [name, param] : List) {
            cout << param.name << ", ";
            cout << "int" << ", ";
            cout << param.default_value << ", ";
            cout << param.min << ", ";
            cout << param.max << ", ";
            cout << param.step << ", ";
            cout << "0.002 " << endl;
        }
    }

    bool Active() {
        return List.size() != 0;
    }

}