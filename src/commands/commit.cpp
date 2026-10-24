#include "commands.h"
#include "../commit.h"
#include "../index.h"
#include "../repo.h"

#include <iostream>

// ./mygit commit [-m <message>]
//
// Builds a tree from the index, wraps it in a commit object (parent = HEAD),
// moves HEAD to the new commit and prints its SHA.
int cmdCommit(const std::vector<std::string> &args)
{
    const std::string usage = "usage: ./mygit commit [-m <message>]\n";
    const std::string defaultMessage = "Default commit message";

    std::string message = defaultMessage;
    if (args.size() == 2 && args[0] == "-m")
    {
        message = args[1];
        if (message.find_first_not_of(" \t\r\n") == std::string::npos)
        {
            std::cerr << "fatal: empty commit message\n";
            return 1;
        }
    }
    else if (!args.empty())
    {
        std::cerr << usage;
        return 1;
    }

    if (!repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }

    Index index = loadIndex();
    std::string parent = readHead();
    if (index.empty() && parent.empty())
    {
        std::cerr << "nothing to commit (use './mygit add' to stage files)\n";
        return 1;
    }

    std::string tree = writeTreeFromIndex(index);

    // Refuse to create a commit identical to its parent's snapshot.
    if (!parent.empty())
    {
        std::string type, content;
        loadObject(parent, type, content);
        if (type != "commit")
            throw std::runtime_error("HEAD does not point to a commit");
        if (parseCommit(content).tree == tree)
        {
            std::cerr << "nothing to commit, staged files match the last commit\n";
            return 1;
        }
    }

    std::string who = currentUser();
    std::string body = "tree " + tree + "\n";
    if (!parent.empty())
        body += "parent " + parent + "\n";
    body += "author " + makeIdentityLine(who) + "\n";
    body += "committer " + makeIdentityLine(who) + "\n\n";
    body += message + "\n";

    std::string sha = storeObject("commit", body);
    writeHead(sha);
    std::cout << sha << "\n";
    return 0;
}
