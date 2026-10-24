# Mini Version Control System (mygit)

A small Git-like version control system in C++17.

## Build & Run
```
make            # builds ./mygit  (make clean removes build output)
./mygit init
```
Requires g++ (C++17) and zlib (`-lz`). Run every command from the repository root
(the directory containing `.mygit`). Use alias for ease.

## Libraries
- **SHA-1**: implemented from scratch in `src/sha1.cpp`. Pads the message, expands each 64-byte block into an 80-word schedule and runs the 80 rounds. Checked against `sha1sum` on inputs from 0 bytes to 5 MB.
- **Compression**: zlib (`compress` / `inflate`), linked with `-lz`.
- **Filesystem**: `std::filesystem`.

## Repository format
Everything is stored in `.mygit/`, using Git's object formats, so hashes match real Git.

```
.mygit/objects/<2 hex>/<38 hex>   zlib-compressed objects
.mygit/index                      staging area
.mygit/HEAD                       SHA of the current commit (empty before the first commit)
```

**Objects** are stored as `<type> <size>\0<content>`; the SHA-1 of that whole string (header included) is the object id.
- **blob**: raw file content.
- **tree**: entries `<mode> <name>\0<20-byte raw sha>` sorted by name (directories compare as `name/`). Modes: `100644` file, `100755` executable, `40000` directory.
- **commit**:
  ```
  tree <sha>
  parent <sha>            (omitted for the first commit)
  author <name> <email> <unix time> <+hhmm>
  committer <name> <email> <unix time> <+hhmm>

  <message>
  ```

**Index** is plain text, one line per staged file: `<mode> <sha> <path>` (path relative to the repo root, `/`-separated; spaces allowed since the path is the rest of the line).

**HEAD** holds a bare commit SHA (there are no branches in this assignment).

## Working procedure and features

### `init`
Creates `.mygit/objects`, an empty `HEAD` and an empty `index`. Running it again prints "Reinitialized" and leaves existing history and staged files untouched. Fails clearly if `.mygit` exists as a file.

### `hash-object [-w] <file>`
Builds `blob <size>\0<content>`, prints its SHA-1. With `-w` it also compresses and stores it (skipped if the object already exists). Without `-w` it works outside a repository, like Git.

### `cat-file <-p|-s|-t> <sha>`
Loads and decompresses the object. `-t` prints the type, `-s` the content size in bytes, `-p` the content: raw bytes for blobs/commits, and an `ls-tree`-style listing for trees. Needs the full 40-character SHA.

### `write-tree`
Recursively snapshots the current directory: every file becomes a blob, every directory a tree (bottom-up), and the root tree SHA is printed. Skips `.mygit`, empty directories and anything that is not a regular file or directory (e.g. symlinks).

### `ls-tree [--name-only] <tree_sha>`
Lists one level of a tree as `<mode> <type> <sha><TAB><name>` (mode is 6 digits, e.g. `040000`), or only names with `--name-only`. Errors if the object is not a tree.

### `add . | <file|dir>...`
Stores each file as a blob and records `mode sha path` in the index. Directories are added recursively. Re-adding updates an entry. If a path no longer exists on disk, its deletion is staged (so `add .` after `rm` removes it from the index). All paths are validated first: if any is bad, nothing is saved.

### `commit [-m <message>]`
Builds the tree from the index, writes a commit object (parent = HEAD), moves HEAD to it and prints the commit SHA. Without `-m` the message is `Default commit message`. Identity comes from `MYGIT_NAME` / `MYGIT_EMAIL`, falling back to `$USER` and `<user>@localhost`. Refuses to commit when nothing is staged or when the staged tree equals the parent's tree.

### `log`
Follows parent links from HEAD and prints, newest first: commit SHA, parent SHA (if any), committer, date (in the committer's timezone) and the message.

### `checkout <commit_sha>`
Restores the working directory to the commit's snapshot:
1. Validates the commit and its whole tree before touching any file.
2. Aborts, changing nothing, if it would overwrite or delete uncommitted work (edited tracked files, or untracked files in the way) and lists them.
3. Deletes files tracked by the current commit that the target lacks (and directories left empty), then writes the target's files with the correct executable bit.
4. Sets HEAD to the commit and resets the index to match it.

Untracked files that don't clash with the target are left alone.

## Assumptions
- Commands are run from the repository root.
- Object ids must be given in full (40 lowercase hex characters).
- Only regular files and directories are tracked; symlinks and special files are skipped. Empty directories are not tracked (similar to Git).
- File names must not contain newlines (the index is line-based).
- HEAD is a bare commit SHA; no branches, merges, or `--force`.
- After `checkout` of an old commit, a new commit simply gets that commit as its parent.

## Error handling
Every command prints a message to stderr and returns a non-zero exit code for: running outside a repository, wrong arguments or unknown flags, invalid or missing object ids, wrong object type, missing or unreadable files, corrupt objects, and (for `checkout`) conflicting local changes.
