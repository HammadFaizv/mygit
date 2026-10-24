#include "commands.h"
#include "../commit.h"
#include "../index.h"
#include "../repo.h"
#include "../tree.h"

#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
// True if `path` is a regular file whose blob hash equals `sha`.
bool diskMatches(const std::string &path, const std::string &sha)
{
    if (!fs::is_regular_file(fs::symlink_status(path)))
        return false;
    return storeObject("blob", readFile(path), false) == sha;
}

bool existsOnDisk(const std::string &path) { return fs::exists(fs::symlink_status(path)); }

// Lists files whose local state would be lost by the checkout:
// tracked files with edits that would be deleted/overwritten, untracked files
// that would be overwritten, and paths blocked by a file/directory clash.
std::set<std::string> findConflicts(const Index &head, const Index &target)
{
    std::set<std::string> conflicts;
    std::set<std::string> deleted;

    for (const auto &[path, e] : head)
    {
        if (target.count(path))
            continue;
        deleted.insert(path);
        if (existsOnDisk(path) && !diskMatches(path, e.sha))
            conflicts.insert(path);
    }

    for (const auto &[path, e] : target)
    {
        auto h = head.find(path);
        if (h != head.end() && h->second.sha == e.sha && h->second.mode == e.mode)
            continue; // unchanged between the two commits: leave the file alone

        if (existsOnDisk(path))
        {
            bool safe = (h != head.end()) ? diskMatches(path, h->second.sha) : diskMatches(path, e.sha);
            if (!safe)
                conflicts.insert(path);
        }

        // A parent that is a file on disk would block creating the directory.
        for (size_t s = path.find('/'); s != std::string::npos; s = path.find('/', s + 1))
        {
            std::string parent = path.substr(0, s);
            if (existsOnDisk(parent) && !fs::is_directory(fs::symlink_status(parent)) && !deleted.count(parent))
                conflicts.insert(parent);
        }
    }
    return conflicts;
}

// Deletes `path` and any parent directories left empty (never the repo root).
void removeFileAndEmptyParents(const std::string &path)
{
    std::error_code ec;
    fs::remove(path, ec);
    for (fs::path p = fs::path(path).parent_path(); !p.empty(); p = p.parent_path())
    {
        if (!fs::is_directory(p) || !fs::is_empty(p, ec) || !fs::remove(p, ec))
            break;
    }
}

void writeBlobTo(const std::string &path, const IndexEntry &e)
{
    std::string type, content;
    loadObject(e.sha, type, content);
    if (type != "blob")
        throw std::runtime_error("object " + e.sha + " is not a blob");

    fs::path p(path);
    if (p.has_parent_path())
        fs::create_directories(p.parent_path());
    writeFile(path, content);

    fs::permissions(path, e.mode == "100755" ? fs::perms(0755) : fs::perms(0644));
}
} // namespace

// ./mygit checkout <commit_sha>
//
// Restores the working directory to the snapshot of the given commit:
// files tracked by the current commit but absent from the target are removed,
// the target's files are written, and HEAD and the index are moved to it.
// Refuses (changing nothing) if that would discard uncommitted work.
int cmdCheckout(const std::vector<std::string> &args)
{
    if (args.size() != 1)
    {
        std::cerr << "usage: ./mygit checkout <commit_sha>\n";
        return 1;
    }
    if (!repoExists())
    {
        std::cerr << "fatal: not a mygit repository (run './mygit init' first)\n";
        return 1;
    }

    const std::string &sha = args[0];
    if (!isValidSha(sha))
    {
        std::cerr << "fatal: '" << sha << "' is not a valid commit id (expected 40 hex characters)\n";
        return 1;
    }

    std::string type, content;
    loadObject(sha, type, content);
    if (type != "commit")
    {
        std::cerr << "fatal: object " << sha << " is a " << type << ", not a commit\n";
        return 1;
    }
    Commit target = parseCommit(content);
    Index targetFiles = indexFromTree(target.tree); // validates the whole tree before we touch anything

    // What the working directory currently tracks: the tree of HEAD.
    Index headFiles;
    std::string headSha = readHead();
    if (!headSha.empty())
    {
        std::string headType, headContent;
        loadObject(headSha, headType, headContent);
        if (headType != "commit")
            throw std::runtime_error("HEAD does not point to a commit");
        headFiles = indexFromTree(parseCommit(headContent).tree);
    }

    std::set<std::string> conflicts = findConflicts(headFiles, targetFiles);
    if (!conflicts.empty())
    {
        std::cerr << "error: checkout would overwrite uncommitted local changes in:\n";
        for (const std::string &p : conflicts)
            std::cerr << "\t" << p << "\n";
        std::cerr << "Commit or move them first.\n";
        return 1;
    }

    for (const auto &[path, e] : headFiles)
        if (!targetFiles.count(path))
            removeFileAndEmptyParents(path);

    for (const auto &[path, e] : targetFiles)
    {
        auto h = headFiles.find(path);
        bool unchanged = h != headFiles.end() && h->second.sha == e.sha && h->second.mode == e.mode;
        if (!unchanged)
            writeBlobTo(path, e);
    }

    writeHead(sha);
    saveIndex(targetFiles); // staging area now mirrors the checked-out commit

    std::cout << "HEAD is now at " << sha << " " << target.message.substr(0, target.message.find('\n')) << "\n";
    return 0;
}
