#include "commands.h"
#include "../commit.h"
#include "../repo.h"
#include "../tree.h"

#include <ctime>
#include <iostream>
#include <set>
#include <sstream>

// "Thu Oct  9 10:59:00 2026 +0530", rendered in the committer's own timezone.
static std::string formatDate(const Identity &id)
{
    long offset = 0;
    if (id.tz.size() == 5 && (id.tz[0] == '+' || id.tz[0] == '-'))
    {
        try
        {
            offset = std::stol(id.tz.substr(1, 2)) * 3600 + std::stol(id.tz.substr(3, 2)) * 60;
        }
        catch (const std::exception &)
        {
            offset = 0;
        }
        if (id.tz[0] == '-')
            offset = -offset;
    }

    std::time_t shifted = static_cast<std::time_t>(id.time + offset);
    std::tm tm{};
    gmtime_r(&shifted, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a %b %e %H:%M:%S %Y", &tm);
    return std::string(buf) + " " + id.tz;
}

// ./mygit log
//
// Walks parent links from HEAD and prints each commit, newest first.
int cmdLog(const std::vector<std::string> &args)
{
    if (!args.empty())
    {
        std::cerr << "usage: ./mygit log\n";
        return 1;
    }
    if (!repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }

    std::string sha = readHead();
    if (sha.empty())
    {
        std::cerr << "fatal: no commits yet\n";
        return 1;
    }

    std::set<std::string> visited; // guards against a corrupt parent cycle
    while (!sha.empty())
    {
        if (!isValidSha(sha) || !visited.insert(sha).second)
        {
            std::cerr << "fatal: corrupt commit history at '" << sha << "'\n";
            return 1;
        }

        std::string type, content;
        loadObject(sha, type, content);
        if (type != "commit")
        {
            std::cerr << "fatal: object " << sha << " is a " << type << ", not a commit\n";
            return 1;
        }
        Commit c = parseCommit(content);

        std::cout << "commit " << sha << "\n";
        if (!c.parent.empty())
            std::cout << "Parent: " << c.parent << "\n";
        std::cout << "Committer: " << c.committer.who << "\n"
                  << "Date: " << formatDate(c.committer) << "\n\n";

        std::istringstream msg(c.message);
        std::string line;
        while (std::getline(msg, line))
            std::cout << "    " << line << "\n";
        std::cout << "\n";

        sha = c.parent;
    }
    return 0;
}
