#include "commit.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <stdexcept>

static Identity parseIdentity(const std::string &line)
{
    // "<who> <time> <tz>": time and tz are the last two space-separated tokens.
    size_t tzPos = line.rfind(' ');
    size_t timePos = (tzPos == std::string::npos || tzPos == 0) ? std::string::npos : line.rfind(' ', tzPos - 1);
    if (timePos == std::string::npos)
        throw std::runtime_error("malformed commit identity");

    Identity id;
    id.who = line.substr(0, timePos);
    id.tz = line.substr(tzPos + 1);
    try
    {
        id.time = std::stol(line.substr(timePos + 1, tzPos - timePos - 1));
    }
    catch (const std::exception &)
    {
        throw std::runtime_error("malformed commit timestamp");
    }
    return id;
}

Commit parseCommit(const std::string &content)
{
    Commit c;
    size_t blank = content.find("\n\n");
    std::string header = content.substr(0, blank == std::string::npos ? content.size() : blank);
    if (blank != std::string::npos)
        c.message = content.substr(blank + 2);
    while (!c.message.empty() && c.message.back() == '\n')
        c.message.pop_back();

    std::istringstream in(header);
    std::string line;
    while (std::getline(in, line))
    {
        if (line.rfind("tree ", 0) == 0)
            c.tree = line.substr(5);
        else if (line.rfind("parent ", 0) == 0)
            c.parent = line.substr(7);
        else if (line.rfind("author ", 0) == 0)
            c.author = parseIdentity(line.substr(7));
        else if (line.rfind("committer ", 0) == 0)
            c.committer = parseIdentity(line.substr(10));
    }
    if (c.tree.empty())
        throw std::runtime_error("malformed commit object (no tree)");
    return c;
}

// Removes characters that would break the "Name <email>" line format.
static std::string sanitize(std::string s)
{
    std::string out;
    for (char ch : s)
        if (ch != '<' && ch != '>' && ch != '\n')
            out.push_back(ch);
    return out;
}

std::string currentUser()
{
    const char *n = std::getenv("MYGIT_NAME");
    if (!n || !*n)
        n = std::getenv("USER");
    std::string name = sanitize((n && *n) ? n : "unknown");

    const char *e = std::getenv("MYGIT_EMAIL");
    std::string email = (e && *e) ? sanitize(e) : name + "@localhost";
    return name + " <" + email + ">";
}

std::string makeIdentityLine(const std::string &who)
{
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);

    int off = static_cast<int>(tm.tm_gmtoff); // seconds east of UTC
    char sign = off < 0 ? '-' : '+';
    if (off < 0)
        off = -off;
    char tz[16];
    std::snprintf(tz, sizeof(tz), "%c%02d%02d", sign, (off / 3600) % 100, (off % 3600) / 60);

    return who + " " + std::to_string(static_cast<long>(now)) + " " + tz;
}
