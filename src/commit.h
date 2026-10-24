#ifndef COMMIT_H
#define COMMIT_H

#include <string>

// "Name <email> <unix time> <+hhmm>" line from a commit object.
struct Identity
{
    std::string who; // "Name <email>"
    long time = 0;
    std::string tz; // e.g. "+0500"
};

// Commit object content:
//   tree <sha>
//   parent <sha>          (absent for the first commit)
//   author <identity>
//   committer <identity>
//
//   <message>
struct Commit
{
    std::string tree;
    std::string parent; // "" if none
    Identity author;
    Identity committer;
    std::string message;
};

// Throws std::runtime_error if `content` is not a valid commit.
Commit parseCommit(const std::string &content);

// Builds the identity ("Name <email>") from MYGIT_NAME / MYGIT_EMAIL, falling back to $USER.
std::string currentUser();

// Returns "<who> <time> <tz>" for the current moment.
std::string makeIdentityLine(const std::string &who);

#endif
