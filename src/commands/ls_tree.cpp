#include "commands.h"
#include "../repo.h"
#include "../tree.h"

#include <iostream>

// ./mygit ls-tree [--name-only] <tree_sha>
//
// Lists one level of a tree object: "<mode> <type> <sha>\t<name>" per entry,
// or just the names with --name-only.
int cmdLsTree(const std::vector<std::string> &args)
{
    const std::string usage = "usage: ./mygit ls-tree [--name-only] <tree_sha>\n";

    bool nameOnly = false;
    std::string sha;
    for (const std::string &a : args)
    {
        if (a == "--name-only" && !nameOnly)
            nameOnly = true;
        else if (sha.empty() && a != "--name-only")
            sha = a;
        else
        {
            std::cerr << usage;
            return 1;
        }
    }
    if (sha.empty())
    {
        std::cerr << usage;
        return 1;
    }
    if (!repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }
    if (!isValidSha(sha))
    {
        std::cerr << "fatal: '" << sha << "' is not a valid object id (expected 40 hex characters)\n";
        return 1;
    }

    std::string type, content;
    loadObject(sha, type, content);
    if (type != "tree")
    {
        std::cerr << "fatal: object " << sha << " is a " << type << ", not a tree\n";
        return 1;
    }

    printTreeEntries(content, nameOnly);
    return 0;
}
