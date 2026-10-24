#include "commands.h"
#include "../repo.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// ./mygit hash-object [-w] <file>
//
// Prints the SHA-1 of "blob <size>\0<content>". With -w the compressed blob
// is also written to the object store.
int cmdHashObject(const std::vector<std::string> &args)
{
    const std::string usage = "usage: ./mygit hash-object [-w] <file>\n";

    bool write = false;
    std::string path;
    for (const std::string &a : args)
    {
        if (a == "-w" && !write)
            write = true;
        else if (path.empty() && a != "-w")
            path = a;
        else
        {
            std::cerr << usage;
            return 1;
        }
    }
    if (path.empty())
    {
        std::cerr << usage;
        return 1;
    }

    if (!fs::exists(path))
    {
        std::cerr << "fatal: cannot open '" << path << "': no such file\n";
        return 1;
    }
    if (!fs::is_regular_file(path))
    {
        std::cerr << "fatal: '" << path << "' is not a regular file\n";
        return 1;
    }
    if (write && !repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }

    std::cout << storeObject("blob", readFile(path), write) << "\n";
    return 0;
}
