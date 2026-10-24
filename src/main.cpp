#include "commands/commands.h"

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc < 2){
        std::cerr << "usage: ./mygit <command> [<args>]\n";
        return 1;
    }

    const std::map<std::string, int (*)(const std::vector<std::string> &)> commands = {
        {"init", cmdInit},         {"hash-object", cmdHashObject},
        {"cat-file", cmdCatFile},  {"write-tree", cmdWriteTree},
        {"ls-tree", cmdLsTree},    {"add", cmdAdd},
        {"commit", cmdCommit},     {"log", cmdLog},
        {"checkout", cmdCheckout},
    };

    auto it = commands.find(argv[1]);
    if (it == commands.end()){
        std::cerr << "mygit: '" << argv[1] << "' is not a mygit command\n";
        return 1;
    }

    std::vector<std::string> args(argv + 2, argv + argc);
    try{
        return it->second(args);
    }
    catch (const std::exception &e){
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}
