#include "commands.h"
#include "../repo.h"

#include <filesystem>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

// ./mygit init
//
// Creates the .mygit directory in the current working directory:
//   .mygit/objects/   object database (blobs, trees, commits)
//   .mygit/index      staging area (empty)
//   .mygit/HEAD       SHA of the current commit (empty until the first commit)
//
// Running init on an existing repository is safe: nothing is overwritten.
int cmdInit(const std::vector<std::string> &args) {
    if (!args.empty()) {
        std::cerr << "usage: ./mygit init\n";
        return 1;
    }

    const std::string absPath = fs::absolute(MYGIT_DIR).string();

    // .mygit exists but is a plain file
    if (fs::exists(MYGIT_DIR) && !fs::is_directory(MYGIT_DIR)) {
        std::cerr << "fatal: '" << MYGIT_DIR << "' exists and is not a directory\n";
        return 1;
    }

    const bool alreadyExists = repoExists();

    try {
        // only create the directories if they dont exist
        fs::create_directories(OBJECTS_DIR);

        if (!fs::exists(HEAD_FILE))
            writeFile(HEAD_FILE, "");
        if (!fs::exists(INDEX_FILE))
            writeFile(INDEX_FILE, "");
    }
    catch (const std::exception &e){
        std::cerr << "fatal: could not initialize repository: " << e.what() << "\n";
        return 1;
    }

    std::cout << (alreadyExists ? "Reinitialized existing" : "Initialized empty")
              << " mygit repository in " << absPath << "\n";
    return 0;
}
