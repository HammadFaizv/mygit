#include "index.h"

#include "repo.h"
#include "tree.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

Index loadIndex()
{
    Index index;
    std::ifstream in(INDEX_FILE);
    if (!in)
        return index;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;
        size_t s1 = line.find(' ');
        size_t s2 = (s1 == std::string::npos) ? s1 : line.find(' ', s1 + 1);
        if (s2 == std::string::npos || s2 + 1 >= line.size())
            throw std::runtime_error("corrupt index file");

        IndexEntry e{line.substr(0, s1), line.substr(s1 + 1, s2 - s1 - 1)};
        if (!isValidSha(e.sha))
            throw std::runtime_error("corrupt index file");
        index[line.substr(s2 + 1)] = e;
    }
    return index;
}

void saveIndex(const Index &index)
{
    std::string content;
    for (const auto &[path, e] : index)
        content += e.mode + " " + e.sha + " " + path + "\n";

    // Write to a temp file and rename so a crash never leaves a half-written index.
    const std::string tmp = INDEX_FILE + ".tmp";
    writeFile(tmp, content);
    fs::rename(tmp, INDEX_FILE);
}

namespace
{
void flatten(const std::string &treeSha, const std::string &prefix, Index &out)
{
    std::string type, content;
    loadObject(treeSha, type, content);
    if (type != "tree")
        throw std::runtime_error("object " + treeSha + " is not a tree");

    for (const TreeEntry &e : parseTree(content))
    {
        if (e.name.empty() || e.name == "." || e.name == ".." || e.name == MYGIT_DIR ||
            e.name.find('/') != std::string::npos)
            throw std::runtime_error("unsafe path in tree object: '" + e.name + "'");

        std::string path = prefix + e.name;
        if (e.isDir())
            flatten(e.sha, path + "/", out);
        else
            out[path] = {e.mode, e.sha};
    }
}

// In-memory directory tree built from the flat index paths.
struct Node
{
    std::map<std::string, Node> dirs;
    std::vector<TreeEntry> files;
};

std::string storeNode(const Node &node)
{
    std::vector<TreeEntry> entries = node.files;
    for (const auto &[name, child] : node.dirs)
        entries.push_back({"40000", name, storeNode(child)});
    return storeTree(entries);
}
} // namespace

Index indexFromTree(const std::string &treeSha)
{
    Index index;
    flatten(treeSha, "", index);
    return index;
}

std::string writeTreeFromIndex(const Index &index)
{
    Node root;
    for (const auto &[path, e] : index)
    {
        Node *cur = &root;
        size_t start = 0, slash;
        while ((slash = path.find('/', start)) != std::string::npos)
        {
            cur = &cur->dirs[path.substr(start, slash - start)];
            start = slash + 1;
        }
        cur->files.push_back({e.mode, path.substr(start), e.sha});
    }
    return storeNode(root);
}
