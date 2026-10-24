#include "commands.h"
#include "../repo.h"
#include "../tree.h"

#include <filesystem>
#include <iostream>

// ./mygit write-tree
//
// Snapshots the current directory (recursively, ignoring .mygit) into blob
// and tree objects and prints the SHA of the root tree.
int cmdWriteTree(const std::vector<std::string> &args)
{
    if (!args.empty())
    {
        std::cerr << "usage: ./mygit write-tree\n";
        return 1;
    }
    if (!repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }

    std::string sha;
    writeTreeFromDir(std::filesystem::path("."), true, sha);
    std::cout << sha << "\n";
    return 0;
}
