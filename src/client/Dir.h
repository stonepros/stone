#ifndef STONE_CLIENT_DIR_H
#define STONE_CLIENT_DIR_H

struct Inode;

class Dir {
 public:
  Inode    *parent_inode;  // my inode
  stone::unordered_map<string, Dentry*> dentries;
  unsigned num_null_dentries = 0;

  vector<Dentry*> readdir_cache;

  explicit Dir(Inode* in) { parent_inode = in; }

  bool is_empty() {  return dentries.empty(); }
};

#endif
