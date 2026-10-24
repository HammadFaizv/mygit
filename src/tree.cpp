#include "tree.h"

#include "repo.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

bool isValidSha(const std::string &s)
{
    if (s.size() != 40)
        return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

std::string hexToRaw(const std::string &hex)
{
    auto val = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
    std::string raw;
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        raw.push_back(static_cast<char>(val(hex[i]) * 16 + val(hex[i + 1])));
    return raw;
}

std::string rawToHex(const std::string &raw)
{
    static const char digits[] = "0123456789abcdef";
    std::string hex;
    for (unsigned char c : raw)
    {
        hex.push_back(digits[c >> 4]);
        hex.push_back(digits[c & 0xf]);
    }
    return hex;
}

std::vector<TreeEntry> parseTree(const std::string &content)
{
    std::vector<TreeEntry> entries;
    size_t pos = 0;
    while (pos < content.size())
    {
        size_t sp = content.find(' ', pos);
        size_t nul = content.find('\0', pos);
        if (sp == std::string::npos || nul == std::string::npos || sp > nul || nul + 21 > content.size())
            throw std::runtime_error("malformed tree object");

        TreeEntry e;
        e.mode = content.substr(pos, sp - pos);
        e.name = content.substr(sp + 1, nul - sp - 1);
        e.sha = rawToHex(content.substr(nul + 1, 20));
        entries.push_back(e);
        pos = nul + 21;
    }
    return entries;
}

std::string modeForFile(const fs::file_status &st)
{
    bool exec = (st.permissions() & fs::perms::owner_exec) != fs::perms::none;
    return exec ? "100755" : "100644";
}

std::string storeTree(std::vector<TreeEntry> entries)
{
    // Git orders entries by name, comparing directories as if they ended in '/'.
    auto key = [](const TreeEntry &e) { return e.isDir() ? e.name + "/" : e.name; };
    std::sort(entries.begin(), entries.end(),
              [&](const TreeEntry &a, const TreeEntry &b) { return key(a) < key(b); });

    std::string content;
    for (const TreeEntry &e : entries)
        content += e.mode + " " + e.name + '\0' + hexToRaw(e.sha);
    return storeObject("tree", content);
}

void printTreeEntries(const std::string &treeContent, bool nameOnly)
{
    for (const TreeEntry &e : parseTree(treeContent))
    {
        if (nameOnly)
        {
            std::cout << e.name << "\n";
            continue;
        }
        // Git prints modes as 6 digits, e.g. 040000.
        std::string mode(e.mode.size() < 6 ? 6 - e.mode.size() : 0, '0');
        std::cout << mode + e.mode << " " << (e.isDir() ? "tree" : "blob") << " "
                  << e.sha << "\t" << e.name << "\n";
    }
}

bool writeTreeFromDir(const fs::path &dir, bool isRoot, std::string &shaOut)
{
    std::vector<TreeEntry> entries;

    for (const auto &item : fs::directory_iterator(dir))
    {
        std::string name = item.path().filename().string();
        if (name == MYGIT_DIR)
            continue;

        fs::file_status st = item.symlink_status(); // do not follow symlinks
        TreeEntry e;
        e.name = name;

        if (fs::is_directory(st))
        {
            if (!writeTreeFromDir(item.path(), false, e.sha))
                continue; // empty directory: not tracked
            e.mode = "40000";
            entries.push_back(e);
        }
        else if (fs::is_regular_file(st))
        {
            e.sha = storeObject("blob", readFile(item.path().string()));
            e.mode = modeForFile(st);
            entries.push_back(e);
        }
        // symlinks, sockets, etc. are skipped
    }

    if (entries.empty() && !isRoot)
        return false;

    shaOut = storeTree(entries);
    return true;
}
