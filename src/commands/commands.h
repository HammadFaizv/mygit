#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>

// Each command receives the arguments that follow the subcommand name
// and returns the process exit code (0 on success).
int cmdInit(const std::vector<std::string> &args);
int cmdHashObject(const std::vector<std::string> &args);
int cmdCatFile(const std::vector<std::string> &args);
int cmdWriteTree(const std::vector<std::string> &args);
int cmdLsTree(const std::vector<std::string> &args);
int cmdAdd(const std::vector<std::string> &args);
int cmdCommit(const std::vector<std::string> &args);
int cmdLog(const std::vector<std::string> &args);
int cmdCheckout(const std::vector<std::string> &args);

#endif
