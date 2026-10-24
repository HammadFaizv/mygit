#include "commands.h"
#include "../repo.h"
#include "../tree.h"

#include <iostream>

// ./mygit cat-file <-p|-s|-t> <sha>
//   -p  pretty-print the content (trees are listed like ls-tree)
//   -s  size of the content in bytes
//   -t  object type
int cmdCatFile(const std::vector<std::string> &args)
{
    const std::string usage = "usage: ./mygit cat-file <-p|-s|-t> <sha>\n";

    if (args.size() != 2)
    {
        std::cerr << usage;
        return 1;
    }
    const std::string &flag = args[0];
    const std::string &sha = args[1];

    if (flag != "-p" && flag != "-s" && flag != "-t")
    {
        std::cerr << "error: unknown flag '" << flag << "'\n" << usage;
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
    loadObject(sha, type, content); // throws if missing/corrupt; main prints it

    if (flag == "-t")
        std::cout << type << "\n";
    else if (flag == "-s")
        std::cout << content.size() << "\n";
    else if (type == "tree")
        printTreeEntries(content, false);
    else
        std::cout.write(content.data(), content.size()); // blob/commit: exact bytes

    return 0;
}
