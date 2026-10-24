#include "commands.h"
#include "../index.h"
#include "../repo.h"
#include "../tree.h"

#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;

namespace
{
std::string toKey(const std::string &arg)
{
    if (arg.empty())
        throw std::runtime_error("empty path");
    fs::path p(arg);
    p = p.is_absolute() ? p.lexically_normal().lexically_relative(fs::current_path())
                        : p.lexically_normal();

    std::string key = p.generic_string();
    if (key == "." || key == "./")
        key.clear();
    while (!key.empty() && key.back() == '/')
        key.pop_back();

    if (key == ".." || key.rfind("../", 0) == 0)
        throw std::runtime_error("'" + arg + "' is outside the repository");
    if (key == MYGIT_DIR || key.rfind(MYGIT_DIR + "/", 0) == 0)
        throw std::runtime_error("cannot add '" + arg + "': it is the repository metadata");
    return key;
}

bool isUnder(const std::string &path, const std::string &dirKey)
{
    return dirKey.empty() || path.rfind(dirKey + "/", 0) == 0;
}

// Stages one regular file: stores its blob and records mode + sha.
void stageFile(Index &index, const std::string &key)
{
    // A file can't coexist with a directory of the same name (or vice versa).
    for (size_t slash = key.find('/'); slash != std::string::npos; slash = key.find('/', slash + 1))
        index.erase(key.substr(0, slash));
    for (auto it = index.lower_bound(key + "/"); it != index.end() && isUnder(it->first, key);)
        it = index.erase(it);

    fs::file_status st = fs::symlink_status(key);
    index[key] = {modeForFile(st), storeObject("blob", readFile(key))};
}

// Stages every regular file below `dirKey` and unstages tracked files that vanished.
void stageDirectory(Index &index, const std::string &dirKey)
{
    std::set<std::string> seen;
    fs::path root = dirKey.empty() ? fs::path(".") : fs::path(dirKey);

    for (fs::recursive_directory_iterator it(root), end; it != end; ++it)
    {
        if (it->path().filename() == MYGIT_DIR)
        {
            it.disable_recursion_pending();
            continue;
        }
        if (!fs::is_regular_file(it->symlink_status())) // directories recurse; symlinks etc. skipped
            continue;

        std::string key = it->path().lexically_normal().generic_string();
        stageFile(index, key);
        seen.insert(key);
    }

    for (auto it = index.begin(); it != index.end();)
        it = (isUnder(it->first, dirKey) && !seen.count(it->first)) ? index.erase(it) : std::next(it);
}

// Handles a path that no longer exists on disk: stages the deletion if it was tracked.
void stageDeletion(Index &index, const std::string &key, const std::string &arg)
{
    bool removed = false;
    for (auto it = index.begin(); it != index.end();)
    {
        if (it->first == key || isUnder(it->first, key))
        {
            it = index.erase(it);
            removed = true;
        }
        else
            ++it;
    }
    if (!removed)
        throw std::runtime_error("pathspec '" + arg + "' did not match any files");
}
} // namespace

// ./mygit add . | <file|dir>...
// Hashes and stores each file as a blob and records "mode-sha-path" in the index.
// All paths are validated before anything is saved, so a bad path leaves the index.
int cmdAdd(const std::vector<std::string> &args)
{
    if (args.empty())
    {
        std::cerr << "usage: ./mygit add . | <file>...\nNothing specified, nothing added.\n";
        return 1;
    }
    if (!repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }

    Index index = loadIndex();
    try
    {
        for (const std::string &arg : args)
        {
            std::string key = toKey(arg);
            fs::file_status st = fs::symlink_status(key.empty() ? "." : key);

            if (fs::is_regular_file(st))
                stageFile(index, key);
            else if (fs::is_directory(st))
                stageDirectory(index, key);
            else if (!fs::exists(st))
                stageDeletion(index, key, arg);
            else
                throw std::runtime_error("'" + arg + "' is not a regular file or directory");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1; // index not saved
    }

    saveIndex(index);
    return 0;
}
