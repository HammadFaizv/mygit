#include "repo.h"

#include "compress.h"
#include "sha1.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

bool repoExists() { return fs::is_directory(MYGIT_DIR); }

std::string readFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open '" + path + "'");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeFile(const std::string &path, const std::string &content)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("cannot write '" + path + "'");
    out.write(content.data(), content.size());
}

std::string readHead()
{
    std::string head = readFile(HEAD_FILE);
    while (!head.empty() && (head.back() == '\n' || head.back() == '\r' || head.back() == ' '))
        head.pop_back();
    return head;
}

void writeHead(const std::string &sha) { writeFile(HEAD_FILE, sha + "\n"); }

std::string storeObject(const std::string &type, const std::string &content, bool write)
{
    std::string full = type + " " + std::to_string(content.size()) + '\0' + content;
    std::string sha = sha1Hex(full);
    if (write)
    {
        fs::path dir = fs::path(OBJECTS_DIR) / sha.substr(0, 2);
        fs::create_directories(dir);
        fs::path file = dir / sha.substr(2);
        if (!fs::exists(file)) // identical content is already stored
            writeFile(file.string(), zlibCompress(full));
    }
    return sha;
}

void loadObject(const std::string &sha, std::string &type, std::string &content)
{
    if (sha.size() != 40)
        throw std::runtime_error("invalid object id '" + sha + "'");
    std::string path = OBJECTS_DIR + "/" + sha.substr(0, 2) + "/" + sha.substr(2);
    if (!fs::exists(path))
        throw std::runtime_error("object not found: " + sha);

    std::string full = zlibDecompress(readFile(path));
    size_t nul = full.find('\0');
    size_t sp = full.find(' ');
    if (nul == std::string::npos || sp == std::string::npos || sp > nul)
        throw std::runtime_error("malformed object: " + sha);
    type = full.substr(0, sp);
    content = full.substr(nul + 1);
}
